
/*
** numsort.c
*/

/********************************
**       BYTEmark (tm)         **
** BYTE NATIVE MODE BENCHMARKS **
**       VERSION 2             **
**                             **
** Included in this source     **
** file:                       **
**  Numeric Heapsort           **
** ----------                  **
** Rick Grehan, BYTE Magazine  **
*********************************
**
** BYTEmark (tm)
** BYTE's Native Mode Benchmarks
** Rick Grehan, BYTE Magazine
**
** Creation:
** Revision: 3/95;10/95
**  10/95 - Removed allocation that was taking place inside
**   the LU Decomposition benchmark. Though it didn't seem to
**   make a difference on systems we ran it on, it nonetheless
**   removes an operating system dependency that probably should
**   not have been there.
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
#include <strings.h>
#include <math.h>
#include "nmglobal.h"
#include "sysspec.h"
#include "misc.h"

#ifdef DEBUG
static int numsort_status=0;
#endif

/*********************
** NUMERIC HEAPSORT **
**********************
** This test implements a heapsort algorithm, performed on an
** array of longs.
*/

void DoNumSort(void);

static ulong DoNumSortIteration(farlong *arraybase,
		ulong arraysize,
		uint numarrays);
static void LoadNumArrayWithRand(farlong *array,
		ulong arraysize,
		uint numarrays);
static void NumHeapSort(farlong *array,
		ulong bottom,
		ulong top);
static void NumSift(farlong *array,
		ulong i,
		ulong j);

/**************
** DoNumSort **
***************
** This routine performs the CPU numeric sort test.
** NOTE: Last version incorrectly stated that the routine
**  returned result in # of longword sorted per second.
**  Not so; the routine returns # of iterations per sec.
*/

void DoNumSort(void)
{
    SortStruct *numsortstruct;      /* Local pointer to global struct */
    farlong *arraybase;     /* Base pointers of array */
    long accumtime;         /* Accumulated time */
    double iterations;      /* Iteration counter */
    char *errorcontext;     /* Error context string pointer */
    int systemerror;        /* For holding error codes */

    /*
     ** Link to global structure
     */
    numsortstruct=&global_numsortstruct;

    /*
     ** Set the error context string.
     */
    errorcontext="CPU:Numeric Sort";

    /*
     ** See if we need to do self adjustment code.
     */
    if(numsortstruct->adjust==0)
    {
        /*
         ** Self-adjustment code.  The system begins by sorting 1
         ** array.  If it does that in no time, then two arrays
         ** are built and sorted.  This process continues until
         ** enough arrays are built to handle the tolerance.
         */
        numsortstruct->numarrays=1;
        while(1)
        {
            /*
             ** Allocate space for arrays
             */
            arraybase=(farlong *)AllocateMemory(sizeof(long) *
                    numsortstruct->numarrays * numsortstruct->arraysize,
                    &systemerror);
            if(systemerror)
            {       ReportError(errorcontext,systemerror);
                FreeMemory((farvoid *)arraybase,
                        &systemerror);
                ErrorExit();
            }

            /*
             ** Do an iteration of the numeric sort.  If the
             ** elapsed time is less than or equal to the permitted
             ** minimum, then allocate for more arrays and
             ** try again.
             */
            if(DoNumSortIteration(arraybase,
                        numsortstruct->arraysize,
                        numsortstruct->numarrays)>global_min_ticks)
                break;          /* We're ok...exit */

            FreeMemory((farvoid *)arraybase,&systemerror);
            if(numsortstruct->numarrays++>NUMNUMARRAYS)
            {       printf("CPU:NSORT -- NUMNUMARRAYS hit.\n");
                ErrorExit();
            }
        }
    }
    else
    {       /*
             ** Allocate space for arrays
             */
        arraybase=(farlong *)AllocateMemory(sizeof(long) *
                numsortstruct->numarrays * numsortstruct->arraysize,
                &systemerror);
        if(systemerror)
        {       ReportError(errorcontext,systemerror);
            FreeMemory((farvoid *)arraybase,
                    &systemerror);
            ErrorExit();
        }

    }
    /*
     ** All's well if we get here.  Repeatedly perform sorts until the
     ** accumulated elapsed time is greater than # of seconds requested.
     */
    accumtime=0L;
    iterations=(double)0.0;

    do {
        accumtime+=DoNumSortIteration(arraybase,
                numsortstruct->arraysize,
                numsortstruct->numarrays);
        iterations+=(double)1.0;
    } while(TicksToSecs(accumtime)<numsortstruct->request_secs);

    /*
     ** Clean up, calculate results, and go home.  Be sure to
     ** show that we don't have to rerun adjustment code.
     */
    FreeMemory((farvoid *)arraybase,&systemerror);

    numsortstruct->sortspersec=iterations *
        (double)numsortstruct->numarrays / TicksToFracSecs(accumtime);

    if(numsortstruct->adjust==0)
        numsortstruct->adjust=1;

#ifdef DEBUG
    if (numsort_status==0) printf("Numeric sort: OK\n");
    numsort_status=0;
#endif
    return;
}

/***********************
** DoNumSortIteration **
************************
** This routine executes one iteration of the numeric
** sort benchmark.  It returns the number of ticks
** elapsed for the iteration.
*/
static ulong DoNumSortIteration(farlong *arraybase,
        ulong arraysize,
        uint numarrays)
{
    ulong elapsed;          /* Elapsed ticks */
    ulong i;
    /*
     ** Load up the array with random numbers
     */
    LoadNumArrayWithRand(arraybase,arraysize,numarrays);

    /*
     ** Start the stopwatch
     */
    elapsed=StartStopwatch();

    /*
     ** Execute a heap of heapsorts
     */
    for(i=0;i<numarrays;i++)
        NumHeapSort(arraybase+i*arraysize,0L,arraysize-1L);

    /*
     ** Get elapsed time
     */
    elapsed=StopStopwatch(elapsed);
#ifdef DEBUG
    {
        for(i=0;i<arraysize-1;i++)
        {       /*
                 ** Compare to check for proper
                 ** sort.
                 */
            if(arraybase[i+1]<arraybase[i])
            {       printf("Sort Error\n");
                numsort_status=1;
                break;
            }
        }
    }
#endif

    return(elapsed);
}

/*************************
** LoadNumArrayWithRand **
**************************
** Load up an array with random longs.
*/
static void LoadNumArrayWithRand(farlong *array,     /* Pointer to arrays */
        ulong arraysize,
        uint numarrays)         /* # of elements in array */
{
    long i;                 /* Used for index */
    farlong *darray;        /* Destination array pointer */
    /*
     ** Initialize the random number generator
     */
    /* randnum(13L); */
    randnum((int32)13);

    /*
     ** Load up first array with randoms
     */
    for(i=0L;i<arraysize;i++)
        /* array[i]=randnum(0L); */
        array[i]=randnum((int32)0);

    /*
     ** Now, if there's more than one array to load, copy the
     ** first into each of the others.
     */
    darray=array;
    while(--numarrays)
    {       darray+=arraysize;
        for(i=0L;i<arraysize;i++)
            darray[i]=array[i];
    }

    return;
}

/****************
** NumHeapSort **
*****************
** Pass this routine a pointer to an array of long
** integers.  Also pass in minimum and maximum offsets.
** This routine performs a heap sort on that array.
*/
static void NumHeapSort(farlong *array,
    ulong bottom,           /* Lower bound */
    ulong top)              /* Upper bound */
{
    ulong temp;                     /* Used to exchange elements */
    ulong i;                        /* Loop index */

    /*
     ** First, build a heap in the array
     */
    for(i=(top/2L); i>0; --i)
        NumSift(array,i,top);

    /*
     ** Repeatedly extract maximum from heap and place it at the
     ** end of the array.  When we get done, we'll have a sorted
     ** array.
     */
    for(i=top; i>0; --i)
    {       NumSift(array,bottom,i);
        temp=*array;                    /* Perform exchange */
        *array=*(array+i);
        *(array+i)=temp;
    }
    return;
}

/************
** NumSift **
*************
** Peforms the sift operation on a numeric array,
** constructing a heap in the array.
*/
static void NumSift(farlong *array,     /* Array of numbers */
    ulong i,                /* Minimum of array */
    ulong j)                /* Maximum of array */
{
    unsigned long k;
    long temp;                              /* Used for exchange */

    while((i+i)<=j)
    {
        k=i+i;
        if(k<j)
            if(array[k]<array[k+1L])
                ++k;
        if(array[i]<array[k])
        {
            temp=array[k];
            array[k]=array[i];
            array[i]=temp;
            i=k;
        }
        else
            i=j+1;
    }
    return;
}