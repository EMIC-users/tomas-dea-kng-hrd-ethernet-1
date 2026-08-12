
/*==================[inclusions]=============================================*/
#include <xc.h>
#include <string.h>
#include "inc/MQTT_Mqtt.h"
#include "inc/enc28j60.h"
#include "inc/systemTimer.h"

/*==================[macros]=================================================*/
#define MQ_KEEPALIVE_S    60          /* keepalive anunciado en CONNECT       */
#define MQ_PING_MS        30000UL     /* PINGREQ si no hubo TX en este lapso  */
#define MQ_RETX_MS        500UL       /* timeout de retransmision TCP         */
#define MQ_RETX_MAX       4           /* retransmisiones antes de reconectar  */
#define MQ_ARPSYN_MS      1000UL      /* timeout de ARP request / SYN         */
#define MQ_BACKOFF_MS     3000UL      /* espera entre intentos de conexion    */
#define MQ_PEND_SIZE      256         /* cola de bytes MQTT salientes         */
#define MQ_CHUNK_MAX      200         /* maximo de bytes MQTT por segmento    */
#define MQ_TOPIC_MAX      80          /* topic entrante (truncado + '\0')     */
#define MQ_PAYLOAD_MAX    200         /* payload entrante (truncado + '\0')   */

/* Estados del socket/sesion (un unico cliente) */
#define ST_DOWN     0                 /* esperando backoff para reintentar    */
#define ST_ARP      1                 /* ARP request al broker enviado        */
#define ST_SYN      2                 /* SYN enviado, esperando SYN|ACK       */
#define ST_CONNECT  3                 /* TCP arriba, esperando CONNACK        */
#define ST_READY    4                 /* sesion MQTT establecida              */

/*==================[internal data]==========================================*/
static uint8_t  my_mac[6] = { 0x00, 0x04, 0xA3, 0x11, 0x22, 0x33 };
static uint8_t  my_ip[4]  = { 192, 168, 0, 30 };

char persist_ MQTT_Mqtt_ip[16]        = "192.168.0.30";
char persist_ MQTT_Mqtt_mac[18]       = "00:04:A3:11:22:33";
char persist_ MQTT_Mqtt_brokerIp[16]  = "192.168.0.135";
char persist_ MQTT_Mqtt_netmask[16]   = "255.255.255.0";
char persist_ MQTT_Mqtt_gateway[16]   = "192.168.0.1";
uint16_t persist_ MQTT_Mqtt_brokerPort = 1883;
char persist_ MQTT_Mqtt_clientId[24]  = "emic-Mqtt";
char persist_ MQTT_Mqtt_username[32]  = { 0, 1 };
char persist_ MQTT_Mqtt_password[32]  = { 0, 1 };

static uint8_t  broker_ip[4];         /* brokerIp parseada en el init         */
static uint8_t  net_mask[4];          /* netmask parseada en el init          */
static uint8_t  nexthop_ip[4];        /* broker en subred, gateway si no      */
static uint8_t  nexthop_mac[6];       /* resuelta por ARP en cada conexion    */
static uint16_t broker_port = 1883;

static uint8_t  rxbuf[600];
static uint8_t  txbuf[600];

/* Cola MQTT saliente (stop-and-wait: sent = bytes en vuelo al frente) */
static uint8_t  pend[MQ_PEND_SIZE];
static uint16_t pend_len;
static uint16_t sent;                 /* 0 = nada en vuelo                    */

/* Socket TCP */
static uint8_t  state;
static uint16_t local_port;
static uint16_t port_seed;            /* varia el puerto local por conexion   */
static uint32_t snd_seq;              /* seq del proximo byte a enviar        */
static uint32_t rcv_nxt;              /* proximo seq esperado del broker      */
static uint32_t t_state;              /* timestamp del estado / retransmision */
static uint32_t t_lasttx;             /* ultima TX MQTT (para keepalive)      */
static uint8_t  retries;
static uint16_t mq_pktid;             /* packet id para SUBSCRIBE             */

/* Strings entrantes para el evento onMessage */
static char in_topic[MQ_TOPIC_MAX + 1];
static char in_payload[MQ_PAYLOAD_MAX + 1];

/*==================[internal functions]=====================================*/
// Parse de "a.b.c.d" decimal a 4 octetos. Devuelve 1 si el texto es valido.
static uint8_t parse_ip(const char *s, uint8_t out[4])
{
    uint16_t v;
    uint8_t i, d;

    for (i = 0; i < 4; i++) {
        v = 0;
        d = 0;
        while (*s >= '0' && *s <= '9' && d < 4) {
            v = v * 10 + (uint16_t)(*s++ - '0');
            d++;
        }
        if (d == 0 || d > 3 || v > 255)
            return 0;
        out[i] = (uint8_t)v;
        if (i < 3 && *s++ != '.')
            return 0;
    }
    return *s == 0;
}

static int8_t hex_nib(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// Parse de "XX:XX:XX:XX:XX:XX" (sep ':' o '-') a 6 bytes. 1 = valido.
static uint8_t parse_mac(const char *s, uint8_t out[6])
{
    uint8_t i;
    int8_t h, l;

    for (i = 0; i < 6; i++) {
        h = hex_nib(*s++);
        if (h < 0)
            return 0;
        l = hex_nib(*s++);
        if (l < 0)
            return 0;
        out[i] = ((uint8_t)h << 4) | (uint8_t)l;
        if (i < 5) {
            if (*s != ':' && *s != '-')
                return 0;
            s++;
        }
    }
    return *s == 0;
}

// suma de a 16 bits big-endian, sin complementar (para encadenar)
static uint32_t sum16(const uint8_t *p, uint16_t n, uint32_t s)
{
    while (n > 1) {
        s += ((uint16_t)p[0] << 8) | p[1];
        p += 2;
        n -= 2;
    }
    if (n)
        s += (uint16_t)p[0] << 8;
    return s;
}

static uint16_t fin16(uint32_t s)
{
    while (s >> 16)
        s = (s & 0xFFFF) + (s >> 16);
    return (uint16_t)~s;
}

/*------------------ ARP / ICMP (responder, igual que ModbusTCP) ------------*/
// ¿ARP request preguntando por my_ip?
static int is_arp_for_me(const uint8_t *f, uint16_t n)
{
    return n >= 42 &&
           f[12] == 0x08 && f[13] == 0x06 &&     // ethertype ARP
           f[20] == 0x00 && f[21] == 0x01 &&     // oper: request
           memcmp(&f[38], my_ip, 4) == 0;        // TPA = mi IP
}

// Convierte el request (in place) en su reply y lo manda
static void arp_reply(uint8_t *f)
{
    memcpy(&f[0], &f[6], 6);        // dst eth = quien preguntó
    memcpy(&f[6], my_mac, 6);
    f[21] = 0x02;                   // oper: reply
    memcpy(&f[32], &f[22], 10);     // THA+TPA = SHA+SPA del request
    memcpy(&f[22], my_mac, 6);
    memcpy(&f[28], my_ip, 4);
    enc28j60_send(f, 42);
}

// ¿ICMP echo request a my_ip? (IPv4 sin opciones, frame completo en buf)
static int is_icmp_echo_for_me(const uint8_t *f, uint16_t n)
{
    uint16_t ip_len;

    if (n < 42 ||
        memcmp(&f[0], my_mac, 6) != 0 ||
        f[12] != 0x08 || f[13] != 0x00 ||       // ethertype IPv4
        f[14] != 0x45 ||                        // versión 4, IHL 20
        f[23] != 1 ||                           // protocolo ICMP
        memcmp(&f[30], my_ip, 4) != 0 ||
        f[34] != 8)                             // echo request
        return 0;
    ip_len = ((uint16_t)f[16] << 8) | f[17];
    return ip_len >= 28 && 14 + ip_len <= n;    // no truncado
}

// Convierte el echo request (in place) en echo reply y lo manda
static void icmp_reply(uint8_t *f)
{
    uint16_t ip_len = ((uint16_t)f[16] << 8) | f[17];
    uint16_t c;

    memcpy(&f[0], &f[6], 6);
    memcpy(&f[6], my_mac, 6);

    memcpy(&f[30], &f[26], 4);      // IP destino = quien preguntó
    memcpy(&f[26], my_ip, 4);
    f[22] = 64;                     // TTL
    f[24] = 0; f[25] = 0;
    c = fin16(sum16(&f[14], 20, 0));
    f[24] = c >> 8; f[25] = c & 0xFF;

    f[34] = 0;                      // echo reply
    f[36] = 0; f[37] = 0;
    c = fin16(sum16(&f[34], ip_len - 20, 0));
    f[36] = c >> 8; f[37] = c & 0xFF;

    enc28j60_send(f, 14 + ip_len);
}

// ARP request broadcast preguntando por el next-hop (broker o gateway)
static void arp_request(void)
{
    uint8_t *f = txbuf;

    memset(&f[0], 0xFF, 6);                     // dst: broadcast
    memcpy(&f[6], my_mac, 6);
    f[12] = 0x08; f[13] = 0x06;                 // ARP
    f[14] = 0x00; f[15] = 0x01;                 // htype ethernet
    f[16] = 0x08; f[17] = 0x00;                 // ptype IPv4
    f[18] = 6;    f[19] = 4;                    // hlen, plen
    f[20] = 0x00; f[21] = 0x01;                 // oper: request
    memcpy(&f[22], my_mac, 6);                  // SHA
    memcpy(&f[28], my_ip, 4);                   // SPA
    memset(&f[32], 0x00, 6);                    // THA: desconocida
    memcpy(&f[38], nexthop_ip, 4);              // TPA: next-hop
    enc28j60_send(f, 42);
}

// ¿ARP reply del next-hop contestando nuestra request?
static int is_arp_from_nexthop(const uint8_t *f, uint16_t n)
{
    return n >= 42 &&
           f[12] == 0x08 && f[13] == 0x06 &&
           f[20] == 0x00 && f[21] == 0x02 &&    // oper: reply
           memcmp(&f[28], nexthop_ip, 4) == 0;  // SPA = next-hop
}

/*------------------ TCP cliente (stop-and-wait) ----------------------------*/
// Arma y envia un segmento al broker. dlen bytes de data (puede ser 0),
// with_mss agrega la opcion MSS (solo en el SYN). seq explicita.
static void tcp_out(uint8_t flags, uint32_t seq, const uint8_t *data,
                    uint16_t dlen, uint8_t with_mss)
{
    uint8_t *f = txbuf;
    uint16_t hl = with_mss ? 24 : 20;
    uint16_t ip_len = 20 + hl + dlen;
    uint16_t c;
    uint32_t s;

    memcpy(&f[0], nexthop_mac, 6);
    memcpy(&f[6], my_mac, 6);
    f[12] = 0x08; f[13] = 0x00;                 // IPv4

    f[14] = 0x45; f[15] = 0;
    f[16] = ip_len >> 8; f[17] = ip_len & 0xFF;
    f[18] = 0; f[19] = 0;                       // id
    f[20] = 0x40; f[21] = 0;                    // DF
    f[22] = 64;                                 // TTL
    f[23] = 6;                                  // TCP
    f[24] = 0; f[25] = 0;
    memcpy(&f[26], my_ip, 4);
    memcpy(&f[30], broker_ip, 4);
    c = fin16(sum16(&f[14], 20, 0));
    f[24] = c >> 8; f[25] = c & 0xFF;

    f[34] = local_port >> 8; f[35] = local_port & 0xFF;
    f[36] = broker_port >> 8; f[37] = broker_port & 0xFF;
    f[38] = seq >> 24; f[39] = seq >> 16; f[40] = seq >> 8; f[41] = seq;
    f[42] = rcv_nxt >> 24; f[43] = rcv_nxt >> 16;
    f[44] = rcv_nxt >> 8;  f[45] = rcv_nxt;
    f[46] = (uint8_t)((hl / 4) << 4);
    f[47] = flags;
    f[48] = sizeof rxbuf >> 8; f[49] = sizeof rxbuf & 0xFF;  // ventana
    f[50] = 0; f[51] = 0;
    f[52] = 0; f[53] = 0;
    if (with_mss) {
        f[54] = 0x02; f[55] = 0x04;             // opcion MSS = 536
        f[56] = 0x02; f[57] = 0x18;
    }
    if (dlen)
        memcpy(&f[34 + hl], data, dlen);

    s = sum16(&f[26], 8, 0);                    // pseudo-header
    s += 6;
    s += hl + dlen;
    s = sum16(&f[34], hl + dlen, s);
    c = fin16(s);
    f[50] = c >> 8; f[51] = c & 0xFF;

    enc28j60_send(f, 34 + hl + dlen);
}

// ¿a y b comparten subred segun net_mask?
static uint8_t same_subnet(const uint8_t a[4], const uint8_t b[4])
{
    uint8_t i;

    for (i = 0; i < 4; i++)
        if ((a[i] ^ b[i]) & net_mask[i])
            return 0;
    return 1;
}

// Cae la conexion: volver a backoff (se reintenta sola desde el poll)
static void conn_down(void)
{
    state    = ST_DOWN;
    t_state  = getSystemMilis();
    pend_len = 0;
    sent     = 0;
}

/*------------------ MQTT ---------------------------------------------------*/
// Encola bytes MQTT salientes. QoS 0: si no hay lugar se descarta entero.
static void mq_queue(const uint8_t *p, uint16_t n)
{
    if (pend_len + n > MQ_PEND_SIZE)
        return;
    memcpy(&pend[pend_len], p, n);
    pend_len += n;
}

// Header fijo MQTT: type+flags y remaining length (varint 1-2 bytes) en out.
// Devuelve el largo del header (2 o 3). rl debe ser < 16384.
static uint8_t mq_fixhdr(uint8_t *out, uint8_t type_flags, uint16_t rl)
{
    out[0] = type_flags;
    if (rl < 128) {
        out[1] = (uint8_t)rl;
        return 2;
    }
    out[1] = (uint8_t)(rl & 0x7F) | 0x80;
    out[2] = (uint8_t)(rl >> 7);
    return 3;
}

// Encola el CONNECT (clean session, keepalive, client id persistente y
// usuario/clave opcionales: username vacio = anonimo; password solo se
// envia si hay username, como exige MQTT 3.1.1)
static void mq_send_connect(void)
{
    uint8_t h[16];
    uint16_t cid_len = (uint16_t)strlen(MQTT_Mqtt_clientId);
    uint16_t usr_len = (uint16_t)strlen(MQTT_Mqtt_username);
    uint16_t pwd_len = (uint16_t)strlen(MQTT_Mqtt_password);
    uint16_t rl = 10 + 2 + cid_len;
    uint8_t flags = 0x02;                       // clean session
    uint8_t n;

    if (usr_len) {
        flags |= 0x80;                          // username flag
        rl += 2 + usr_len;
        if (pwd_len) {
            flags |= 0x40;                      // password flag
            rl += 2 + pwd_len;
        }
    }

    n = mq_fixhdr(h, 0x10, rl);
    h[n++] = 0x00; h[n++] = 0x04;               // protocol name "MQTT"
    h[n++] = 'M'; h[n++] = 'Q'; h[n++] = 'T'; h[n++] = 'T';
    h[n++] = 0x04;                              // level 3.1.1
    h[n++] = flags;
    h[n++] = 0x00; h[n++] = MQ_KEEPALIVE_S;     // keepalive
    h[n++] = cid_len >> 8; h[n++] = cid_len & 0xFF;
    mq_queue(h, n);
    mq_queue((const uint8_t *)MQTT_Mqtt_clientId, cid_len);
    if (usr_len) {
        h[0] = usr_len >> 8; h[1] = usr_len & 0xFF;
        mq_queue(h, 2);
        mq_queue((const uint8_t *)MQTT_Mqtt_username, usr_len);
        if (pwd_len) {
            h[0] = pwd_len >> 8; h[1] = pwd_len & 0xFF;
            mq_queue(h, 2);
            mq_queue((const uint8_t *)MQTT_Mqtt_password, pwd_len);
        }
    }
}

// Procesa un paquete MQTT entrante completo (p apunta al header fijo)
static void mq_packet(const uint8_t *p, uint8_t hdr, uint16_t rl)
{
    uint8_t type = p[0] >> 4;
    const uint8_t *v = p + hdr;                 // variable header + payload
    uint16_t tl, pl;

    if (type == 2) {                            // CONNACK
        if (rl >= 2 && v[1] == 0) {
            state   = ST_READY;
            t_lasttx = getSystemMilis();
            MQTT_Mqtt_onConnected();
        } else {
            conn_down();                        // broker rechazo la sesion
        }
    } else if (type == 3) {                     // PUBLISH (QoS 0 entrante)
        if ((p[0] & 0x06) != 0 || rl < 2)       // QoS > 0 no soportado
            return;
        tl = ((uint16_t)v[0] << 8) | v[1];
        if (2 + tl > rl)
            return;
        pl = rl - 2 - tl;
        if (tl > MQ_TOPIC_MAX)   tl = MQ_TOPIC_MAX;     // truncar con '\0'
        if (pl > MQ_PAYLOAD_MAX) pl = MQ_PAYLOAD_MAX;
        memcpy(in_topic, &v[2], tl);
        in_topic[tl] = 0;
        memcpy(in_payload, &v[2] + (((uint16_t)v[0] << 8) | v[1]), pl);
        in_payload[pl] = 0;
    }
    /* SUBACK (9) / PINGRESP (13) / resto: nada que hacer en QoS 0 */
}

// Recorre los paquetes MQTT contenidos en el segmento TCP (pueden venir
// varios coalescidos). Un paquete partido entre segmentos se descarta (v1:
// los paquetes de control y QoS 0 entrantes caben en un segmento).
static void mq_rx(const uint8_t *d, uint16_t n)
{
    uint16_t rl;
    uint8_t hdr;

    while (n >= 2) {
        if (d[1] & 0x80) {
            if (n < 3 || (d[2] & 0x80))         // varint > 2 bytes: no soportado
                return;
            rl  = (uint16_t)(d[1] & 0x7F) | ((uint16_t)d[2] << 7);
            hdr = 3;
        } else {
            rl  = d[1];
            hdr = 2;
        }
        if ((uint16_t)(hdr + rl) > n)           // paquete incompleto
            return;
        mq_packet(d, hdr, rl);
        d += hdr + rl;
        n -= hdr + rl;
    }
}

// Maneja un segmento TCP del broker hacia nuestro socket
static void tcp_rx(uint8_t *f, uint16_t n)
{
    uint16_t ip_len, hl, dlen;
    uint32_t seq, ack;
    uint8_t flags;

    ip_len = ((uint16_t)f[16] << 8) | f[17];
    if (ip_len < 40 || 14 + ip_len > n)
        return;
    hl    = (uint16_t)(f[46] >> 4) * 4;
    dlen  = ip_len - 20 - hl;
    flags = f[47];
    seq = ((uint32_t)f[38] << 24) | ((uint32_t)f[39] << 16) |
          ((uint32_t)f[40] << 8)  | f[41];
    ack = ((uint32_t)f[42] << 24) | ((uint32_t)f[43] << 16) |
          ((uint32_t)f[44] << 8)  | f[45];

    if (flags & 0x04) {                         // RST
        conn_down();
        return;
    }

    if (state == ST_SYN) {
        if ((flags & 0x12) == 0x12 && ack == snd_seq + 1) {
            snd_seq++;                          // SYN consumio un seq
            rcv_nxt = seq + 1;
            tcp_out(0x10, snd_seq, 0, 0, 0);    // ACK del SYN|ACK
            mq_send_connect();                  // CONNECT a la cola
            state   = ST_CONNECT;
            t_state = getSystemMilis();
            retries = 0;
        }
        return;
    }

    /* ST_CONNECT / ST_READY */
    if ((flags & 0x10) && sent != 0 &&
        (int32_t)(ack - (snd_seq + sent)) >= 0) {   // >= modulo 2^32
        snd_seq += sent;                        // en vuelo confirmado
        memmove(pend, &pend[sent], pend_len - sent);
        pend_len -= sent;
        sent = 0;
    }

    if (dlen > 0) {
        if (seq == rcv_nxt) {
            rcv_nxt += dlen;
            tcp_out(0x10, snd_seq, 0, 0, 0);    // ACK de los datos
            mq_rx(&f[34 + hl], dlen);
        } else {
            tcp_out(0x10, snd_seq, 0, 0, 0);    // duplicado: re-ACK
        }
    }

    if (flags & 0x01) {                         // FIN del broker
        rcv_nxt++;
        tcp_out(0x11, snd_seq, 0, 0, 0);        // FIN|ACK y a reconectar
        conn_down();
    }
}

/*==================[external functions]=====================================*/
void MQTT_Mqtt_init(void)
{
    uint8_t tmp[6];
    uint8_t gw[4];

    if (parse_ip(MQTT_Mqtt_ip, tmp))
        memcpy(my_ip, tmp, 4);
    if (parse_mac(MQTT_Mqtt_mac, tmp))
        memcpy(my_mac, tmp, 6);
    if (!parse_ip(MQTT_Mqtt_brokerIp, broker_ip))
        parse_ip("192.168.0.135", broker_ip);
    if (!parse_ip(MQTT_Mqtt_netmask, net_mask))
        parse_ip("255.255.255.0", net_mask);
    if (!parse_ip(MQTT_Mqtt_gateway, gw))
        parse_ip("192.168.0.1", gw);
    if (MQTT_Mqtt_brokerPort != 0)
        broker_port = MQTT_Mqtt_brokerPort;

    if (same_subnet(my_ip, broker_ip) ||
        (gw[0] | gw[1] | gw[2] | gw[3]) == 0)
        memcpy(nexthop_ip, broker_ip, 4);
    else
        memcpy(nexthop_ip, gw, 4);

    enc28j60_init(my_mac);

    state   = ST_DOWN;
    t_state = getSystemMilis() - MQ_BACKOFF_MS; // primer intento inmediato
}

void MQTT_Mqtt_poll(void)
{
    uint32_t now = getSystemMilis();
    uint16_t n;

    /* ----- RX ----- */
    n = enc28j60_recv(rxbuf, sizeof rxbuf);
    if (n > 0) {
        if (is_arp_for_me(rxbuf, n)) {
            arp_reply(rxbuf);
        } else if (state == ST_ARP && is_arp_from_nexthop(rxbuf, n)) {
            memcpy(nexthop_mac, &rxbuf[22], 6);
            snd_seq = now;                      // ISN
            tcp_out(0x02, snd_seq, 0, 0, 1);    // SYN (con MSS)
            state   = ST_SYN;
            t_state = now;
        } else if (is_icmp_echo_for_me(rxbuf, n)) {
            icmp_reply(rxbuf);
        } else if (n >= 54 &&
                   memcmp(&rxbuf[0], my_mac, 6) == 0 &&
                   rxbuf[12] == 0x08 && rxbuf[13] == 0x00 &&
                   rxbuf[14] == 0x45 && rxbuf[23] == 6 &&
                   memcmp(&rxbuf[30], my_ip, 4) == 0 &&
                   memcmp(&rxbuf[26], broker_ip, 4) == 0 &&
                   ((((uint16_t)rxbuf[34] << 8) | rxbuf[35]) == broker_port) &&
                   ((((uint16_t)rxbuf[36] << 8) | rxbuf[37]) == local_port) &&
                   state >= ST_SYN) {
            tcp_rx(rxbuf, n);
        }
    }

    /* ----- timers / TX ----- */
    switch (state) {
    case ST_DOWN:
        if (now - t_state >= MQ_BACKOFF_MS) {
            port_seed++;
            local_port = 0xC000 | (port_seed & 0x0FFF);
            arp_request();
            state   = ST_ARP;
            t_state = now;
            retries = 0;
        }
        break;

    case ST_ARP:
        if (now - t_state >= MQ_ARPSYN_MS) {
            if (++retries > 3) { conn_down(); break; }
            arp_request();
            t_state = now;
        }
        break;

    case ST_SYN:
        if (now - t_state >= MQ_ARPSYN_MS) {
            if (++retries > 3) { conn_down(); break; }
            tcp_out(0x02, snd_seq, 0, 0, 1);
            t_state = now;
        }
        break;

    case ST_CONNECT:
    case ST_READY:
        if (sent != 0) {
            if (now - t_state >= MQ_RETX_MS) {  // retransmitir lo en vuelo
                if (++retries > MQ_RETX_MAX) { conn_down(); break; }
                tcp_out(0x18, snd_seq, pend, sent, 0);
                t_state = now;
            }
        } else if (pend_len > 0) {              // despachar la cola
            sent = (pend_len > MQ_CHUNK_MAX) ? MQ_CHUNK_MAX : pend_len;
            tcp_out(0x18, snd_seq, pend, sent, 0);  // PSH|ACK
            t_state  = now;
            t_lasttx = now;
            retries  = 0;
        } else if (state == ST_READY && now - t_lasttx >= MQ_PING_MS) {
            uint8_t ping[2] = { 0xC0, 0x00 };   // PINGREQ
            mq_queue(ping, 2);
            t_lasttx = now;
        }
        break;
    }
}




