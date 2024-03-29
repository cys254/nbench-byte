/*
** bitfield.c
*/

/********************************
**       BYTEmark (tm)         **
** BYTE NATIVE MODE BENCHMARKS **
**       VERSION 2             **
**                             **
** Included in this source     **
** file:                       **
**  Bitfield test              **
** ----------                  **
** Rick Grehan, BYTE Magazine  **
*********************************
**
** BYTEmark (tm)
** BYTE's Native Mode Benchmarks
** Rick Grehan, BYTE Magazine
**
** Creation:
** Revision: 3/95
**
** DISCLAIMER
** The source, executable, and documentation files that comprise
** the BYTEmark benchmarks are made available on an "as is" basis.
** This means that we at BYTE Magazine have made every reasonable
** effort to verify that the there are no errors in the source and
** executable code.  We cannot, however, guarantee that the programs
** are error-free.  Consequently, McGraw-HIll and BYTE Magazine make
** no claims in regard to the fitness of the source code, executable
** code, and documentation of the BYTEmark.
**  Furthermore, BYTE Magazine, McGraw-Hill, and all employees
** of McGraw-Hill cannot be held responsible for any damages resulting
** from the use of this code or the results obtained from using
** this code.
*/

/*
** INCLUDES
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nmglobal.h"
#include "sysspec.h"
#include "misc.h"

/************************
** BITFIELD OPERATIONS **
*************************/

void DoBitops(void);
void DoBitopsAdjust(TestControlStruct *strsortstruct);

static void *BitopsFunc(void *data);
static void DoBitfieldIteration(farulong *bitarraybase,
		farulong *bitoparraybase,
		long bitoparraysize,
		ulong *nbitops,
        StopWatchStruct *stopwatch);
static void ToggleBitRun(farulong *bitmap,
		ulong bit_addr,
		ulong nbits,
		uint val);
static void FlipBitRun(farulong *bitmap,
		ulong bit_addr,
		ulong nbits);

/*************
** DoBitops **
**************
** Perform the bit operations test portion of the CPU
** benchmark.  Returns the iterations per second.
*/
void DoBitops(void)
{
    TestControlStruct *locbitopstruct;    /* Local bitop structure */

    /*
     ** Link to global structure.
     */
    locbitopstruct=&global_bitopstruct;

    /*
     ** See if we need to run adjustment code.
     */
    DoBitopsAdjust(locbitopstruct);

    /*
     ** Run the benchmark
     */
    run_bench_with_concurrency(locbitopstruct, BitopsFunc);
}

void DoBitopsAdjust(TestControlStruct *locbitopstruct)
{
    if(locbitopstruct->adjust==0)
    {
        farulong *bitarraybase;         /* Base of bitmap array */
        farulong *bitoparraybase;       /* Base of bitmap operations array */
        ulong nbitops;                  /* # of bitfield operations */
        StopWatchStruct stopwatch;      /* Stop watch to time the test */
        int systemerror;                /* For holding error codes */
        bitarraybase=(farulong *)AllocateMemory(locbitopstruct->bitfieldarraysize *
                sizeof(ulong),&systemerror);
        if(systemerror)
        {
            ReportError(locbitopstruct->errorcontext,systemerror);
            ErrorExit();
        }

        /*
         ** Initialize bitfield operations array to [3] elements
         */
        locbitopstruct->bitoparraysize=3L;

        while(1)
        {
            /*
             ** Allocate space for operations array
             */
            bitoparraybase=(farulong *)AllocateMemory(locbitopstruct->bitoparraysize*2L*
                    sizeof(ulong),
                    &systemerror);
            if(systemerror)
            {
                ReportError(locbitopstruct->errorcontext,systemerror);
                FreeMemory((farvoid *)bitarraybase,&systemerror);
                ErrorExit();
            }
            /*
             ** Do an iteration of the bitmap test.  If the
             ** elapsed time is less than or equal to the permitted
             ** minimum, then de-allocate the array, reallocate a
             ** larger version, and try again.
             */
            ResetStopWatch(&stopwatch);
            DoBitfieldIteration(bitarraybase,
                    bitoparraybase,
                    locbitopstruct->bitoparraysize,
                    &nbitops,
                    &stopwatch);
#ifdef DEBUG
#ifdef LINUX
            if (locbitopstruct->bitoparraysize==3L){
                /* this is the first loop, write a debug file */
                FILE *file;
                unsigned long *running_base; /* same as farulong */
                long counter;
                file=fopen("debugbit.dat","w");
                running_base=bitarraybase;
                for (counter=0;counter<(long)(locbitopstruct->bitfieldarraysize);counter++){
#ifdef LONG64
                    fprintf(file,"%08X",(unsigned int)(*running_base&0xFFFFFFFFL));
                    fprintf(file,"%08X",(unsigned int)((*running_base>>32)&0xFFFFFFFFL));
                    if ((counter+1)%4==0) fprintf(file,"\n");
#else
                    fprintf(file,"%08lX",*running_base);
                    if ((counter+1)%8==0) fprintf(file,"\n");
#endif
                    running_base=running_base+1;
                }
                fclose(file);
                printf("\nWrote the file debugbit.dat, you may want to compare it to debugbit.good\n");
            }
#endif
#endif

            FreeMemory((farvoid *)bitoparraybase,&systemerror);

            if (stopwatch.realsecs>global_min_itersec) break;      /* We're ok...exit */

            locbitopstruct->bitoparraysize*=2;
        }

        FreeMemory((farvoid *)bitarraybase,&systemerror);

        /*
         ** Set adjustment flag to show that we don't have
         ** to do self adjusting in the future.
         */
        locbitopstruct->adjust=1;
    }
}

void *BitopsFunc(void *data)
{
    TestThreadData *testdata;       /* test data passed from thread func */
    farulong *bitarraybase;         /* Base of bitmap array */
    farulong *bitoparraybase;       /* Base of bitmap operations array */
    ulong nbitops;                  /* # of bitfield operations */
    StopWatchStruct stopwatch;      /* Stop watch to time the test */
    int systemerror;                /* For holding error code */
    TestControlStruct *locbitopstruct;      /* Local pointer to global struct */

    testdata = (TestThreadData *)data;
    locbitopstruct = testdata->control;

    /*
     ** allocate the array space.
     */
    bitarraybase=(farulong *)AllocateMemory(locbitopstruct->bitfieldarraysize *
                sizeof(ulong),&systemerror);
    if(systemerror)
    {
        ReportError(locbitopstruct->errorcontext,systemerror);
        ErrorExit();
    }
    bitoparraybase=(farulong *)AllocateMemory(locbitopstruct->bitoparraysize*2L*
                sizeof(ulong),
                &systemerror);
    if(systemerror)
    {
        ReportError(locbitopstruct->errorcontext,systemerror);
        FreeMemory((farvoid *)bitarraybase,&systemerror);
        ErrorExit();
    }

    /*
     ** All's well if we get here.  Repeatedly perform sorts until the
     ** accumulated elapsed time is greater than # of seconds requested.
     */
    testdata->result.iterations = 0.0;
    ResetStopWatch(&stopwatch);

    do {
        DoBitfieldIteration(bitarraybase, bitoparraybase,
                locbitopstruct->bitoparraysize,&nbitops,&stopwatch);
        testdata->result.iterations+=(double)nbitops;
    } while(stopwatch.realsecs<locbitopstruct->request_secs);

    /*
     ** Clean up, calculate results, and go home.
     */
    FreeMemory((farvoid *)bitarraybase,&systemerror);
    FreeMemory((farvoid *)bitoparraybase,&systemerror);

    testdata->result.cpusecs = stopwatch.cpusecs;
    testdata->result.realsecs = stopwatch.realsecs;

    return 0;
}


/************************
** DoBitfieldIteration **
*************************
** Perform a single iteration of the bitfield benchmark.
** Return the # of ticks accumulated by the operation.
*/
static void DoBitfieldIteration(farulong *bitarraybase,
        farulong *bitoparraybase,
        long bitoparraysize,
        ulong *nbitops,
        StopWatchStruct *stopwatch)
{
    long i;                         /* Index */
    ulong bitoffset;                /* Offset into bitmap */
    /*
     ** Clear # bitops counter
     */
    *nbitops=0L;

    /*
     ** Construct a set of bitmap offsets and run lengths.
     ** The offset can be any random number from 0 to the
     ** size of the bitmap (in bits).  The run length can
     ** be any random number from 1 to the number of bits
     ** between the offset and the end of the bitmap.
     ** Note that the bitmap has 8192 * 32 bits in it.
     ** (262,144 bits)
     */
    /*
     ** Reset random number generator so things repeat.
     ** Also reset the bit array we work on.
     ** added by Uwe F. Mayer
     */
    randnum((int32)13);
    for (i=0;i<global_bitopstruct.bitfieldarraysize;i++)
    {
#ifdef LONG64
        *(bitarraybase+i)=(ulong)0x5555555555555555;
#else
        *(bitarraybase+i)=(ulong)0x55555555;
#endif
    }
    randnum((int32)13);
    /* end of addition of code */

    for (i=0;i<bitoparraysize;i++)
    {
        /* First item is offset */
        /* *(bitoparraybase+i+i)=bitoffset=abs_randwc(262140L); */
        *(bitoparraybase+i+i)=bitoffset=abs_randwc((int32)262140);

        /* Next item is run length */
        /* *nbitops+=*(bitoparraybase+i+i+1L)=abs_randwc(262140L-bitoffset);*/
        *nbitops+=*(bitoparraybase+i+i+1L)=abs_randwc((int32)262140-bitoffset);
    }

    /*
     ** Array of offset and lengths built...do an iteration of
     ** the test.
     ** Start the stopwatch.
     */
    StartStopWatch(stopwatch);

    /*
     ** Loop through array off offset/run length pairs.
     ** Execute operation based on modulus of index.
     */
    for(i=0;i<bitoparraysize;i++)
    {
        switch(i % 3)
        {

            case 0: /* Set run of bits */
                ToggleBitRun(bitarraybase,
                        *(bitoparraybase+i+i),
                        *(bitoparraybase+i+i+1),
                        1);
                break;

            case 1: /* Clear run of bits */
                ToggleBitRun(bitarraybase,
                        *(bitoparraybase+i+i),
                        *(bitoparraybase+i+i+1),
                        0);
                break;

            case 2: /* Complement run of bits */
                FlipBitRun(bitarraybase,
                        *(bitoparraybase+i+i),
                        *(bitoparraybase+i+i+1));
                break;
        }
    }

    /*
     ** Stop stopwatch
     */
    StopStopWatch(stopwatch);
}


/*****************************
**     ToggleBitRun          *
******************************
** Set or clear a run of nbits starting at
** bit_addr in bitmap.
*/
static void ToggleBitRun(farulong *bitmap, /* Bitmap */
        ulong bit_addr,         /* Address of bits to set */
        ulong nbits,            /* # of bits to set/clr */
        uint val)               /* 1 or 0 */
{
    unsigned long bindex;   /* Index into array */
    unsigned long bitnumb;  /* Bit number */

    while(nbits--)
    {
#ifdef LONG64
        bindex=bit_addr>>6;     /* Index is number /64 */
        bitnumb=bit_addr % 64;   /* Bit number in word */
#else
        bindex=bit_addr>>5;     /* Index is number /32 */
        bitnumb=bit_addr % 32;  /* bit number in word */
#endif
        if(val)
            bitmap[bindex]|=(1L<<bitnumb);
        else
            bitmap[bindex]&=~(1L<<bitnumb);
        bit_addr++;
    }
    return;
}

/***************
** FlipBitRun **
****************
** Complements a run of bits.
*/
static void FlipBitRun(farulong *bitmap,        /* Bit map */
        ulong bit_addr,                 /* Bit address */
        ulong nbits)                    /* # of bits to flip */
{
    unsigned long bindex;   /* Index into array */
    unsigned long bitnumb;  /* Bit number */

    while(nbits--)
    {
#ifdef LONG64
        bindex=bit_addr>>6;     /* Index is number /64 */
        bitnumb=bit_addr % 64;  /* Bit number in longword */
#else
        bindex=bit_addr>>5;     /* Index is number /32 */
        bitnumb=bit_addr % 32;  /* Bit number in longword */
#endif
        bitmap[bindex]^=(1L<<bitnumb);
        bit_addr++;
    }

    return;
}
