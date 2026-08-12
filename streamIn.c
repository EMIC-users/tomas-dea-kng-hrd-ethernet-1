#include <xc.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "inc/streamIn.h"
#include "inc/streamOut.h"
#include "inc/conversionFunctions.h"
#include "string.h"

float streamIn_t_ptr_to_float(streamIn_t* dataIn)
{
    char str[16];
    char* strPtr = str;
    int i;
    for (i = 0 ; i < 16 && dataIn->count() ; i ++)
    {
        *strPtr = dataIn->get();
        strPtr ++;
    }
    *strPtr = 0;

    return (float)atof(str);

}

char streamIn_t_ptr_to_char(streamIn_t* dataIn)
{
    char str[16];
    char* strPtr = str;
    int i;
    for (i = 0 ; i < 16 && dataIn->count() ; i ++)
    {
        *strPtr = dataIn->get();
        strPtr ++;
    }
    *strPtr = 0;

    return str[0];
}

uint8_t streamIn_t_ptr_to_uint8_t(streamIn_t* dataIn)
{
    char str[16];
    char* strPtr = str;
    int i;
    for (i = 0 ; i < 16 && dataIn->count() ; i ++)
    {
        *strPtr = dataIn->get();
        strPtr ++;
    }
    *strPtr = 0;

    return ato_uint8_t(str);
}

uint16_t streamIn_t_ptr_to_uint16_t(streamIn_t* dataIn)
{
    char str[16];
    char* strPtr = str;
    int i;
    for (i = 0 ; i < 16 && dataIn->count() ; i ++)
    {
        *strPtr = dataIn->get();
        strPtr ++;
    }
    *strPtr = 0;

    return ato_uint16_t(str);

}

uint32_t streamIn_t_ptr_to_uint32_t(streamIn_t* dataIn)
{
    char str[16];
    char* strPtr = str;
    int i;
    for (i = 0 ; i < 16 && dataIn->count() ; i ++)
    {
        *strPtr = dataIn->get();
        strPtr ++;
    }
    *strPtr = 0;

    return ato_uint32_t(str);

}

uint64_t streamIn_t_ptr_to_uint64_t(streamIn_t* dataIn)
{
    char str[16];
    char* strPtr = str;
    int i;
    for (i = 0 ; i < 16 && dataIn->count() ; i ++)
    {
        *strPtr = dataIn->get();
        strPtr ++;
    }
    *strPtr = 0;

    return ato_uint64_t(str);

}

int8_t streamIn_t_ptr_to_int8_t(streamIn_t* dataIn)
{
    char str[16];
    char* strPtr = str;
    int i;
    for (i = 0 ; i < 16 && dataIn->count() ; i ++)
    {
        *strPtr = dataIn->get();
        strPtr ++;
    }
    *strPtr = 0;

    return ato_int8_t(str);

}

int16_t streamIn_t_ptr_to_int16_t(streamIn_t* dataIn)
{
    char str[16];
    char* strPtr = str;
    int i;
    for (i = 0 ; i < 16 && dataIn->count() ; i ++)
    {
        *strPtr = dataIn->get();
        strPtr ++;
    }
    *strPtr = 0;

    return ato_int16_t(str);

}

int32_t streamIn_t_ptr_to_int32_t(streamIn_t* dataIn)
{
    char str[16];
    char* strPtr = str;
    int i;
    for (i = 0 ; i < 16 && dataIn->count() ; i ++)
    {
        *strPtr = dataIn->get();
        strPtr ++;
    }
    *strPtr = 0;

    return ato_int32_t(str);

}

int64_t streamIn_t_ptr_to_int64_t(streamIn_t* dataIn)
{
    char str[16];
    char* strPtr = str;
    int i;
    for (i = 0 ; i < 16 && dataIn->count() ; i ++)
    {
        *strPtr = dataIn->get();
        strPtr ++;
    }
    *strPtr = 0;

    return ato_int64_t(str);

}


void streamIn_t_ptr_to_ascii(streamIn_t* dataIn,char* string,int count)
{
    char* strPtr = string;
    int i;
    for (i = 0 ; i < count && dataIn->count() ; i ++)
    {
        *strPtr = dataIn->get();
        strPtr ++;
    }
    *strPtr = 0;
}


void sendDataToStream(const streamOut_t* stream, char* format_, va_list arg)
{
    char* format = format_;
    int i;
    char strFormat[10] = {0};  // pequeño buffer para el formato (ej: "%llu")
    char auxStr[32];           // string temporal para el valor convertido
    int okFormat = 0;

    for (; *format != '\0'; format++)
    {
        if (*format == '%')
        {
            okFormat = 0;
            uint16_t typeSelector = 16;
            memset(strFormat, 0, sizeof(strFormat));

            for (i = 0; i < sizeof(strFormat) - 1 && !okFormat; format++, i++)
            {
                strFormat[i] = *format;

                switch (*format)
                {
                    case 'c':
                    {
                        okFormat = 1;
                        *auxStr = (char)va_arg(arg, int); // char se promociona a int
                        auxStr[1] = '\0';
                        break;
                    }
                    case 'l':
                        typeSelector <<= 1;
                        break;
                    case 'h':
                        typeSelector >>= 1;
                        break;

                    case 'u':
                    {
                        okFormat = 1;
                        strFormat[i + 1] = '\0';
                        switch (typeSelector)
                        {
                            case 8:
                            case 16:
                            {
                                unsigned int val = va_arg(arg, unsigned int); // promoción
                                snprintf(auxStr, sizeof(auxStr), strFormat, val);
                                break;
                            }
                            case 32:
                            {
                                uint32_t val = va_arg(arg, uint32_t);
                                snprintf(auxStr, sizeof(auxStr), strFormat, val);
                                break;
                            }
                            case 64:
                            {
                                uint64_t val = va_arg(arg, uint64_t);
                                snprintf(auxStr, sizeof(auxStr), strFormat, val);
                                break;
                            }
                        }
                        break;
                    }

                    case 'd':
                    {
                        okFormat = 1;
                        strFormat[i + 1] = '\0';
                        switch (typeSelector)
                        {
                            case 8:
                            case 16:
                            {
                                int val = va_arg(arg, int); // promoci�n
                                snprintf(auxStr, sizeof(auxStr), strFormat, val);
                                break;
                            }
                            case 32:
                            {
                                int32_t val = va_arg(arg, int32_t);
                                snprintf(auxStr, sizeof(auxStr), strFormat, val);
                                break;
                            }
                            case 64:
                            {
                                int64_t val = va_arg(arg, int64_t);
                                snprintf(auxStr, sizeof(auxStr), strFormat, val);
                                break;
                            }
                        }
                        break;
                    }

                    case 'f':
                    {
                        okFormat = 1;
                        strFormat[i + 1] = '\0';
                        switch (typeSelector)
                        {
                            case 16:
                            case 32:
                            {
                                double val = va_arg(arg, double); // float se promociona a double
                                snprintf(auxStr, sizeof(auxStr), strFormat, val);
                                break;
                            }
                        }
                        break;
                    }
                }
            }

            // salida de caracteres convertidos
            for (char* str = auxStr; *str; str++)
            {
                stream->put(*str);
            }

            format--; // retrocede uno porque el for externo hace format++
            continue;
        }

        else if (*format == '$')
        {
            format++;
            if (*format == '\0') break;

            switch (*format)
            {
                case 's':
                {
                    char* str = va_arg(arg, char*);
                    while (*str)
                    {
                        stream->put(*str++);
                    }
                    break;
                }
                case 'r':
                {
                    streamIn_t* dataIn = va_arg(arg, streamIn_t*);
                    while (dataIn->count())
                    {
                        stream->put(dataIn->get());
                    }
                    break;
                }
                default:
                    stream->put(*format);
                    break;
            }
        }

        else
        {
            stream->put(*format);
        }
    }
}
