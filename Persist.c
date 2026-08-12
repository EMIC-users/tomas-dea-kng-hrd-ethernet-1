#include "inc/Persist.h"
#include "inc/Flash.h"

// Símbolos externos definidos por el linker (no ocupan RAM)
extern char __persist_start__;
extern char __persist_end__;

void save(void)
{
	char* i;
	offsetPutc3B = 0;

    ErasePage(__builtin_tblpage(&myFlashMemory),__builtin_tbloffset(&myFlashMemory));
    
    offset = __builtin_tbloffset(&myFlashMemory);

	putc_program3B('N');

	// Un solo bucle para toda la sección persist
	for (i = &__persist_start__; i < &__persist_end__; i++)
	{
		putc_program3B(*i);
	}
	flushProgramPutc3B();
}

void load(void)
{
	char* i;
	
	offsetPutc3B = 0;

    offset = __builtin_tbloffset(&myFlashMemory);

	if ( getc_program3B() != 'N')
		return;

	// Un solo bucle para toda la sección persist
	for (i = &__persist_start__; i < &__persist_end__; i++)
	{
		*i = getc_program3B();
	}
	return;
}

