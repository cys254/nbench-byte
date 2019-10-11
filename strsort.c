/*
** strsort.c
*/

/********************************
**       BYTEmark (tm)         **
** BYTE NATIVE MODE BENCHMARKS **
**       VERSION 2             **
**                             **
** Included in this source     **
** file:                       **
**  String Heapsort            **
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
#include <math.h>
#include "nmglobal.h"
#include "sysspec.h"
#include "misc.h"

#ifdef DEBUG
static int stringsort_status=0;
#endif

/********************
** STRING HEAPSORT **
********************/

void DoStringSort(void);
void DoStringSortAdjust(TestControlStruct *strsortstruct);

static void *StringSortFunc(void *data);
static void DoStringSortIteration(faruchar *arraybase,
		uint numarrays,
		ulong arraysize,
        StopWatchStruct *stopwatch);
static farulong *LoadStringArray(faruchar *strarray,
		uint numarrays,
		ulong *strings,
		ulong arraysize);
static void stradjust(farulong *optrarray,
		faruchar *strarray,
		ulong nstrings,
		ulong i,
		uchar l);
static void StrHeapSort(farulong *optrarray,
		faruchar *strarray,
		ulong numstrings,
		ulong bottom,
		ulong top);
static int str_is_less(farulong *optrarray,
		faruchar *strarray,
		ulong numstrings,
		ulong a,
		ulong b);
static void strsift(farulong *optrarray,
		faruchar *strarray,
		ulong numstrings,
		ulong i,
		ulong j);

/*****************
** DoStringSort **
******************
** This routine performs the CPU string sort test.
** Arguments:
**      requested_secs = # of seconds to execute test
**      stringspersec = # of strings per second sorted (RETURNED)
*/
void DoStringSort(void)
{
    TestControlStruct *strsortstruct;      /* Local for sort structure */

    /*
     ** Link to global structure
     */
    strsortstruct=&global_strsortstruct;

    /*
     ** See if we have to perform self-adjustment code
     */
    DoStringSortAdjust(strsortstruct);

    /*
     ** Run the benchmark
     */
    run_bench_with_concurrency(strsortstruct, StringSortFunc);

#ifdef DEBUG
    if (stringsort_status==0) printf("String sort: OK\n");
    stringsort_status=0;
#endif
}

void DoStringSortAdjust(TestControlStruct *strsortstruct)
{
    if(strsortstruct->adjust==0)
    {
        faruchar *arraybase;            /* Base pointer of char array */
        StopWatchStruct stopwatch;      /* Stop watch to time the test */
        int systemerror;                /* For holding error code */
        /*
         ** Initialize the number of arrays.
         */
        strsortstruct->numarrays=1;
        while(1)
        {
            /*
             ** Allocate space for array.  We'll add an extra 100
             ** bytes to protect memory as strings move around
             ** (this can happen during string adjustment)
             */
            arraybase=(faruchar *)AllocateMemory((strsortstruct->arraysize+100L) *
                    (long)strsortstruct->numarrays,&systemerror);
            if(systemerror)
            {
                ReportError(strsortstruct->errorcontext,systemerror);
                ErrorExit();
            }

            ResetStopWatch(&stopwatch);

            /*
             ** Do an iteration of the string sort.  If the
             ** elapsed time is less than or equal to the permitted
             ** minimum, then de-allocate the array, reallocate a
             ** an additional array, and try again.
             */
            DoStringSortIteration(arraybase,
                        strsortstruct->numarrays,
                        strsortstruct->arraysize, &stopwatch);

            FreeMemory((farvoid *)arraybase,&systemerror);

            if(stopwatch.realsecs > global_min_itersec)
                break;          /* We're ok...exit */

            strsortstruct->numarrays*=2;
        }
        strsortstruct->adjust=1;
    }
}

void *StringSortFunc(void *data)
{
    TestThreadData *testdata;       /* test data passed from thread func */
    faruchar *arraybase;            /* Base pointer of char array */
    StopWatchStruct stopwatch;      /* Stop watch to time the test */
    int systemerror;                /* For holding error code */
    TestControlStruct *strsortstruct;      /* Local pointer to global struct */

    testdata = (TestThreadData *)data;
    strsortstruct = testdata->control;

    /*
     ** Allocate the space for the array.
     */
    arraybase=(faruchar *)AllocateMemory((strsortstruct->arraysize+100L) *
                (long)strsortstruct->numarrays,&systemerror);
    if(systemerror)
    {
         ReportError(strsortstruct->errorcontext,systemerror);
         ErrorExit();
    }

    /*
     ** All's well if we get here.  Repeatedly perform sorts until the
     ** accumulated elapsed time is greater than # of seconds requested.
     */
    testdata->result.iterations = 0.0;
    ResetStopWatch(&stopwatch);

    do {
        DoStringSortIteration(arraybase,
                strsortstruct->numarrays,
                strsortstruct->arraysize,
                &stopwatch);
        testdata->result.iterations+=(double)strsortstruct->numarrays;
    } while(stopwatch.realsecs<strsortstruct->request_secs);

    /*
     ** Clean up, calculate results, and go home.
     ** Set flag to show we don't need to rerun adjustment code.
     */
    FreeMemory((farvoid *)arraybase,&systemerror);

    testdata->result.cpusecs = stopwatch.cpusecs;
    testdata->result.realsecs = stopwatch.realsecs;
    return 0;
}

/**************************
** DoStringSortIteration **
***************************
** This routine executes one iteration of the string
** sort benchmark.  It returns the number of ticks
** Note that this routine also builds the offset pointer
** array.
*/
static void DoStringSortIteration(faruchar *arraybase,
        uint numarrays,ulong arraysize,StopWatchStruct *stopwatch)
{
    farulong *optrarray;            /* Offset pointer array */
    unsigned long nstrings;         /* # of strings in array */
    int syserror;                   /* System error code */
    unsigned int i;                 /* Index */
    farulong *tempobase;            /* Temporary offset pointer base */
    faruchar *tempsbase;            /* Temporary string base pointer */

    /*
     ** Load up the array(s) with random numbers
     */
    optrarray=LoadStringArray(arraybase,numarrays,&nstrings,arraysize);

    /*
     ** Set temp base pointers...they will be modified as the
     ** benchmark proceeds.
     */
    tempobase=optrarray;
    tempsbase=arraybase;

    /*
     ** Start the stopwatch
     */
    StartStopWatch(stopwatch);

    /*
     ** Execute heapsorts
     */
    for(i=0;i<numarrays;i++)
    {
        StrHeapSort(tempobase,tempsbase,nstrings,0L,nstrings-1);
        tempobase+=nstrings;    /* Advance base pointers */
        tempsbase+=arraysize+100;
    }

    /*
     ** Record elapsed time
     */
    StopStopWatch(stopwatch);

#ifdef DEBUG
    {
        unsigned long i;
        for(i=0;i<nstrings-1;i++)
        {       /*
                 ** Compare strings to check for proper
                 ** sort.
                 */
            if(str_is_less(optrarray,arraybase,nstrings,i+1,i))
            {
                printf("Sort Error\n");
                stringsort_status=1;
                break;
            }
        }
    }
#endif

    /*
     ** Release the offset pointer array built by
     ** LoadStringArray()
     */
    FreeMemory((farvoid *)optrarray,&syserror);
}

/********************
** LoadStringArray **
*********************
** Initialize the string array with random strings of
** varying sizes.
** Returns the pointer to the offset pointer array.
** Note that since we're creating a number of arrays, this
** routine builds one array, then copies it into the others.
*/
static farulong *LoadStringArray(faruchar *strarray, /* String array */
    uint numarrays,                 /* # of arrays */
    ulong *nstrings,                /* # of strings */
    ulong arraysize)                /* Size of array */
{
    faruchar *tempsbase;            /* Temporary string base pointer */
    farulong *optrarray;            /* Local for pointer */
    farulong *tempobase;            /* Temporary offset pointer base pointer */
    unsigned long curroffset;       /* Current offset */
    int fullflag;                   /* Indicates full array */
    unsigned char stringlength;     /* Length of string */
    unsigned char i;                /* Index */
    unsigned long j;                /* Another index */
    unsigned int k;                 /* Yet another index */
    unsigned int l;                 /* Ans still one more index */
    int systemerror;                /* For holding error code */

    /*
     ** Initialize random number generator.
     */
    /* randnum(13L); */
    randnum((int32)13);

    /*
     ** Start with no strings.  Initialize our current offset pointer
     ** to 0.
     */
    *nstrings=0L;
    curroffset=0L;
    fullflag=0;

    do
    {
        /*
         ** Allocate a string with a random length no
         ** shorter than 4 bytes and no longer than
         ** 80 bytes.  Note we have to also make sure
         ** there's room in the array.
         */
        /* stringlength=(unsigned char)((1+abs_randwc(76L)) & 0xFFL);*/
        stringlength=(unsigned char)((1+abs_randwc((int32)76)) & 0xFFL);
        if((unsigned long)stringlength+curroffset+1L>=arraysize)
        {
            stringlength=(unsigned char)((arraysize-curroffset-1L) & 0xFF);
            fullflag=1;     /* Indicates a full */
        }

        /*
         ** Store length at curroffset and advance current offset.
         */
        *(strarray+curroffset)=stringlength;
        curroffset++;

        /*
         ** Fill up the rest of the string with random bytes.
         */
        for(i=0;i<stringlength;i++)
        {
            *(strarray+curroffset)= /* (unsigned char)(abs_randwc((long)0xFE)); */
                                       (unsigned char)(abs_randwc((int32)0xFE));
            curroffset++;
        }

        /*
         ** Increment the # of strings counter.
         */
        *nstrings+=1L;

    } while(fullflag==0);

    /*
     ** We now have initialized a single full array.  If there
     ** is more than one array, copy the original into the
     ** others.
     */
    k=1;
    tempsbase=strarray;
    while(k<numarrays)
    {       tempsbase+=arraysize+100;         /* Set base */
        for(l=0;l<arraysize;l++)
            tempsbase[l]=strarray[l];
        k++;
    }

    /*
     ** Now the array is full, allocate enough space for an
     ** offset pointer array.
     */
    optrarray=(farulong *)AllocateMemory(*nstrings * sizeof(unsigned long) *
            numarrays,
            &systemerror);
    if(systemerror)
    {
        ReportError("ppCPU:Stringsort",systemerror);
        FreeMemory((void *)strarray,&systemerror);
        ErrorExit();
    }

    /*
     ** Go through the newly-built string array, building
     ** offsets and putting them into the offset pointer
     ** array.
     */
    curroffset=0;
    for(j=0;j<*nstrings;j++)
    {
        *(optrarray+j)=curroffset;
        curroffset+=(unsigned long)(*(strarray+curroffset))+1L;
    }

    /*
     ** As above, we've made one copy of the offset pointers,
     ** so duplicate this array in the remaining ones.
     */
    k=1;
    tempobase=optrarray;
    while(k<numarrays)
    {
        tempobase+=*nstrings;
        for(l=0;l<*nstrings;l++)
            tempobase[l]=optrarray[l];
        k++;
    }

    /*
     ** All done...go home.  Pass local pointer back.
     */
    return(optrarray);
}

/**************
** stradjust **
***************
** Used by the string heap sort.  Call this routine to adjust the
** string at offset i to length l.  The members of the string array
** are moved accordingly and the length of the string at offset i
** is set to l.
*/
static void stradjust(farulong *optrarray,      /* Offset pointer array */
    faruchar *strarray,                     /* String array */
    ulong nstrings,                         /* # of strings */
    ulong i,                                /* Offset to adjust */
    uchar l)                                /* New length */
{
    unsigned long nbytes;           /* # of bytes to move */
    unsigned long j;                /* Index */
    int direction;                  /* Direction indicator */
    unsigned char adjamount;        /* Adjustment amount */

    /*
     ** If new length is less than old length, the direction is
     ** down.  If new length is greater than old length, the
     ** direction is up.
     */
    direction=(int)l - (int)*(strarray+*(optrarray+i));
    adjamount=(unsigned char)abs(direction);

    /*
     ** See if the adjustment is being made to the last
     ** string in the string array.  If so, we don't have to
     ** do anything more than adjust the length field.
     */
    if(i==(nstrings-1L))
    {       *(strarray+*(optrarray+i))=l;
        return;
    }

    /*
     ** Calculate the total # of bytes in string array from
     ** location i+1 to end of array.  Whether we're moving "up" or
     ** down, this is how many bytes we'll have to move.
     */
    nbytes=*(optrarray+nstrings-1L) +
        (unsigned long)*(strarray+*(optrarray+nstrings-1L)) + 1L -
        *(optrarray+i+1L);

    /*
     ** Calculate the source and the destination.  Source is
     ** string position i+1.  Destination is string position i+l
     ** (i+"ell"...don't confuse 1 and l).
     ** Hand this straight to memmove and let it handle the
     ** "overlap" problem.
     */
    MoveMemory((farvoid *)(strarray+*(optrarray+i)+l+1),
            (farvoid *)(strarray+*(optrarray+i+1)),
            (unsigned long)nbytes);

    /*
     ** We have to adjust the offset pointer array.
     ** This covers string i+1 to numstrings-1.
     */
    for(j=i+1;j<nstrings;j++)
        if(direction<0)
            *(optrarray+j)=*(optrarray+j)-adjamount;
        else
            *(optrarray+j)=*(optrarray+j)+adjamount;

    /*
     ** Store the new length and go home.
     */
    *(strarray+*(optrarray+i))=l;
    return;
}

/****************
** strheapsort **
*****************
** Pass this routine a pointer to an array of unsigned char.
** The array is presumed to hold strings occupying at most
** 80 bytes (counts a byte count).
** This routine also needs a pointer to an array of offsets
** which represent string locations in the array, and
** an unsigned long indicating the number of strings
** in the array.
*/
static void StrHeapSort(farulong *optrarray, /* Offset pointers */
    faruchar *strarray,             /* Strings array */
    ulong numstrings,               /* # of strings in array */
    ulong bottom,                   /* Region to sort...bottom */
    ulong top)                      /* Region to sort...top */
{
    unsigned char temp[80];                 /* Used to exchange elements */
    unsigned char tlen;                     /* Temp to hold length */
    unsigned long i;                        /* Loop index */


    /*
     ** Build a heap in the array
     */
    for(i=(top/2L); i>0; --i)
        strsift(optrarray,strarray,numstrings,i,top);

    /*
     ** Repeatedly extract maximum from heap and place it at the
     ** end of the array.  When we get done, we'll have a sorted
     ** array.
     */
    for(i=top; i>0; --i)
    {
        strsift(optrarray,strarray,numstrings,0,i);

        /* temp = string[0] */
        tlen=*strarray;
        MoveMemory((farvoid *)&temp[0], /* Perform exchange */
                (farvoid *)strarray,
                (unsigned long)(tlen+1));


        /* string[0]=string[i] */
        tlen=*(strarray+*(optrarray+i));
        stradjust(optrarray,strarray,numstrings,0,tlen);
        MoveMemory((farvoid *)strarray,
                (farvoid *)(strarray+*(optrarray+i)),
                (unsigned long)(tlen+1));

        /* string[i]=temp */
        tlen=temp[0];
        stradjust(optrarray,strarray,numstrings,i,tlen);
        MoveMemory((farvoid *)(strarray+*(optrarray+i)),
                (farvoid *)&temp[0],
                (unsigned long)(tlen+1));

    }
    return;
}

/****************
** str_is_less **
*****************
** Pass this function:
**      1) A pointer to an array of offset pointers
**      2) A pointer to a string array
**      3) The number of elements in the string array
**      4) Offsets to two strings (a & b)
** This function returns TRUE if string a is < string b.
*/
static int str_is_less(farulong *optrarray, /* Offset pointers */
    faruchar *strarray,                     /* String array */
    ulong numstrings,                       /* # of strings */
    ulong a, ulong b)                       /* Offsets */
{
    int slen;               /* String length */

    /*
     ** Determine which string has the minimum length.  Use that
     ** to call strncmp().  If they match up to that point, the
     ** string with the longer length wins.
     */
    slen=(int)*(strarray+*(optrarray+a));
    if(slen > (int)*(strarray+*(optrarray+b)))
        slen=(int)*(strarray+*(optrarray+b));

    slen=strncmp((char *)(strarray+*(optrarray+a)),
            (char *)(strarray+*(optrarray+b)),slen);

    if(slen==0)
    {
        /*
         ** They match.  Return true if the length of a
         ** is greater than the length of b.
         */
        if(*(strarray+*(optrarray+a)) >
                *(strarray+*(optrarray+b)))
            return(TRUE);
        return(FALSE);
    }

    if(slen<0) return(TRUE);        /* a is strictly less than b */

    return(FALSE);                  /* Only other possibility */
}

/************
** strsift **
*************
** Pass this function:
**      1) A pointer to an array of offset pointers
**      2) A pointer to a string array
**      3) The number of elements in the string array
**      4) Offset within which to sort.
** Sift the array within the bounds of those offsets (thus
** building a heap).
*/
static void strsift(farulong *optrarray,        /* Offset pointers */
    faruchar *strarray,                     /* String array */
    ulong numstrings,                       /* # of strings */
    ulong i, ulong j)                       /* Offsets */
{
    unsigned long k;                /* Temporaries */
    unsigned char temp[80];
    unsigned char tlen;             /* For string lengths */


    while((i+i)<=j)
    {
        k=i+i;
        if(k<j)
            if(str_is_less(optrarray,strarray,numstrings,k,k+1L))
                ++k;
        if(str_is_less(optrarray,strarray,numstrings,i,k))
        {
            /* temp=string[k] */
            tlen=*(strarray+*(optrarray+k));
            MoveMemory((farvoid *)&temp[0],
                    (farvoid *)(strarray+*(optrarray+k)),
                    (unsigned long)(tlen+1));

            /* string[k]=string[i] */
            tlen=*(strarray+*(optrarray+i));
            stradjust(optrarray,strarray,numstrings,k,tlen);
            MoveMemory((farvoid *)(strarray+*(optrarray+k)),
                    (farvoid *)(strarray+*(optrarray+i)),
                    (unsigned long)(tlen+1));

            /* string[i]=temp */
            tlen=temp[0];
            stradjust(optrarray,strarray,numstrings,i,tlen);
            MoveMemory((farvoid *)(strarray+*(optrarray+i)),
                    (farvoid *)&temp[0],
                    (unsigned long)(tlen+1));
            i=k;
        }
        else
            i=j+1;
    }
    return;
}
