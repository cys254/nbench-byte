/*
** assign.c
*/

/********************************
**       BYTEmark (tm)         **
** BYTE NATIVE MODE BENCHMARKS **
**       VERSION 2             **
**                             **
** Included in this source     **
** file:                       **
**  Assignment algorithm       **
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
#include "nmglobal.h"
#include "sysspec.h"
#include "misc.h"

/*************************
** ASSIGNMENT ALGORITHM **
*************************/

/*
** DEFINES
*/

#define ASSIGNROWS 101L
#define ASSIGNCOLS 101L

/*
** TYPEDEFS
*/
typedef struct {
	union {
		long *p;
		long (*ap)[ASSIGNROWS][ASSIGNCOLS];
	} ptrs;
} longptr;

/*
** PROTOTYPES
*/
void DoAssign(void);
void DoAssignAdjust(TestControlStruct *locassignstruct);
void* AssignFunc(void *data);
static void DoAssignIteration(farlong *arraybase,
		ulong numarrays,StopWatchStruct *stopwatch);
static void LoadAssignArrayWithRand(farlong *arraybase,
		ulong numarrays);
static void LoadAssign(farlong arraybase[][ASSIGNCOLS]);
static void CopyToAssign(farlong arrayfrom[][ASSIGNCOLS],
		long arrayto[][ASSIGNCOLS]);
static void Assignment(farlong arraybase[][ASSIGNCOLS]);
static void calc_minimum_costs(long tableau[][ASSIGNCOLS]);
static int first_assignments(long tableau[][ASSIGNCOLS],
		short assignedtableau[][ASSIGNCOLS]);
static void second_assignments(long tableau[][ASSIGNCOLS],
		short assignedtableau[][ASSIGNCOLS]);


/*************
** DoAssign **
**************
** Perform an assignment algorithm.
** The algorithm was adapted from the step by step guide found
** in "Quantitative Decision Making for Business" (Gordon,
**  Pressman, and Cohn; Prentice-Hall)
**
**
** NOTES:
** 1. Even though the algorithm distinguishes between
**    ASSIGNROWS and ASSIGNCOLS, as though the two might
**    be different, it does presume a square matrix.
**    I.E., ASSIGNROWS and ASSIGNCOLS must be the same.
**    This makes for some algorithmically-correct but
**    probably non-optimal constructs.
**
*/
void DoAssign(void)
{
    TestControlStruct *locassignstruct;  /* Local structure ptr */

    /*
     ** Link to global structure
     */
    locassignstruct=&global_assignstruct;

    /*
     ** See if we need to do self adjustment code.
     */
    DoAssignAdjust(locassignstruct);

    /*
     ** All's well if we get here.  Do the tests.
     */
    run_bench_with_concurrency(locassignstruct, AssignFunc);
}

/*******************
** DoAssignAdjust **
********************
** Perform self adjust
*/
void DoAssignAdjust(TestControlStruct *locassignstruct)
{
    /*
     ** See if we need to do self adjustment code.
     */
    if(locassignstruct->adjust==0)
    {
        farlong *arraybase;
        int systemerror;
        StopWatchStruct stopwatch;             /* Stop watch to time the test */
        /*
         ** Self-adjustment code.  The system begins by working on 1
         ** array.  If it does that in no time, then two arrays
         ** are built.  This process continues until
         ** enough arrays are built to handle the tolerance.
         */
        locassignstruct->numarrays=1;
        while(1)
        {
            /*
             ** Allocate space for arrays
             */
            arraybase=(farlong *) AllocateMemory(sizeof(long)*
                    ASSIGNROWS*ASSIGNCOLS*locassignstruct->numarrays,
                    &systemerror);
            if(systemerror)
            {
                ReportError(locassignstruct->errorcontext,systemerror);
                FreeMemory((farvoid *)arraybase,
                        &systemerror);
                ErrorExit();
            }

            /*
             ** Do an iteration of the assignment alg.  If the
             ** elapsed time is less than or equal to the permitted
             ** minimum, then allocate for more arrays and
             ** try again.
             */
            ResetStopWatch(&stopwatch);
            DoAssignIteration(arraybase,
                        locassignstruct->numarrays,&stopwatch);

            FreeMemory((farvoid *)arraybase, &systemerror);

            if(stopwatch.realsecs>global_min_itersec)
                break;          /* We're ok...exit */

            locassignstruct->numarrays++;
        }
        /*
         ** Be sure to show that we don't have to rerun adjustment code.
         */
        locassignstruct->adjust=1;
    }
}

/***************
** AssignFunc **
****************
** Perform an assignment algorithm.
** The algorithm was adapted from the step by step guide found
** in "Quantitative Decision Making for Business" (Gordon,
**  Pressman, and Cohn; Prentice-Hall)
**
**
** NOTES:
** 1. Even though the algorithm distinguishes between
**    ASSIGNROWS and ASSIGNCOLS, as though the two might
**    be different, it does presume a square matrix.
**    I.E., ASSIGNROWS and ASSIGNCOLS must be the same.
**    This makes for some algorithmically-correct but
**    probably non-optimal constructs.
**
*/
void* AssignFunc(void *data)
{
    TestThreadData *testdata;              /* test data passed from thread func */
    TestControlStruct *locassignstruct;    /* Local pointer to global struct */
    StopWatchStruct stopwatch;             /* Stop watch to time the test */
    farlong *arraybase;     /* Base pointers of array */
    int systemerror;        /* For holding error codes */

    testdata = (TestThreadData *)data;
    locassignstruct = testdata->control;

    /*
     ** Allocate space for arrays
     */
    arraybase=(farlong *)AllocateMemory(sizeof(long)*
                ASSIGNROWS*ASSIGNCOLS*locassignstruct->numarrays,
                &systemerror);
    if(systemerror)
    {
        ReportError(locassignstruct->errorcontext,systemerror);
        FreeMemory((farvoid *)arraybase,
                    &systemerror);
        ErrorExit();
    }

    /*
     ** All's well if we get here.  Do the tests.
     */
    testdata->result.iterations=(double)0.0;
    ResetStopWatch(&stopwatch);

    do {
        DoAssignIteration(arraybase,
                locassignstruct->numarrays,&stopwatch);
        testdata->result.iterations+=(double)1.0;
    } while(stopwatch.realsecs<locassignstruct->request_secs);

    /*
     ** Clean up, calculate results, and go home.  Be sure to
     ** show that we don't have to rerun adjustment code.
     */
    FreeMemory((farvoid *)arraybase,&systemerror);

    testdata->result.cpusecs = stopwatch.cpusecs;
    testdata->result.realsecs = stopwatch.realsecs;
    return 0;
}


/**********************
** DoAssignIteration **
***********************
** This routine executes one iteration of the assignment test.
** It returns the number of ticks elapsed in the iteration.
*/
static void DoAssignIteration(farlong *arraybase,
    ulong numarrays, StopWatchStruct *stopwatch)
{
    longptr abase;                  /* local pointer */
    ulong i;

    /*
     ** Set up local pointer
     */
    abase.ptrs.p=arraybase;

    /*
     ** Load up the arrays with a random table.
     */
    LoadAssignArrayWithRand(arraybase,numarrays);

    /*
     ** Start the stopwatch
     */
    StartStopWatch(stopwatch);

    /*
     ** Execute assignment algorithms
     */
    for(i=0;i<numarrays;i++)
    {       /* abase.ptrs.p+=i*ASSIGNROWS*ASSIGNCOLS; */
        /* Fixed  by Eike Dierks */
        Assignment(*abase.ptrs.ap);
        abase.ptrs.p+=ASSIGNROWS*ASSIGNCOLS;
    }

    /*
     ** Get elapsed time
     */
    StopStopWatch(stopwatch);
}

/****************************
** LoadAssignArrayWithRand **
*****************************
** Load the assignment arrays with random numbers.  All positive.
** These numbers represent costs.
*/
static void LoadAssignArrayWithRand(farlong *arraybase,
    ulong numarrays)
{
    longptr abase,abase1;   /* Local for array pointer */
    ulong i;

    /*
     ** Set local array pointer
     */
    abase.ptrs.p=arraybase;
    abase1.ptrs.p=arraybase;

    /*
     ** Set up the first array.  Then just copy it into the
     ** others.
     */
    LoadAssign(*(abase.ptrs.ap));
    if(numarrays>1)
        for(i=1;i<numarrays;i++)
        {     /* abase1.ptrs.p+=i*ASSIGNROWS*ASSIGNCOLS; */
            /* Fixed  by Eike Dierks */
            abase1.ptrs.p+=ASSIGNROWS*ASSIGNCOLS;
            CopyToAssign(*(abase.ptrs.ap),*(abase1.ptrs.ap));
        }

    return;
}

/***************
** LoadAssign **
****************
** The array given by arraybase is loaded with positive random
** numbers.  Elements in the array are capped at 5,000,000.
*/
static void LoadAssign(farlong arraybase[][ASSIGNCOLS])
{
    ushort i,j;

    /*
     ** Reset random number generator so things repeat.
     */
    /* randnum(13L); */
    randnum((int32)13);

    for(i=0;i<ASSIGNROWS;i++)
        for(j=0;j<ASSIGNROWS;j++){
            /* arraybase[i][j]=abs_randwc(5000000L);*/
            arraybase[i][j]=abs_randwc((int32)5000000);
        }

    return;
}

/*****************
** CopyToAssign **
******************
** Copy the contents of one array to another.  This is called by
** the routine that builds the initial array, and is used to copy
** the contents of the intial array into all following arrays.
*/
static void CopyToAssign(farlong arrayfrom[ASSIGNROWS][ASSIGNCOLS],
        farlong arrayto[ASSIGNROWS][ASSIGNCOLS])
{
    ushort i,j;

    for(i=0;i<ASSIGNROWS;i++)
        for(j=0;j<ASSIGNCOLS;j++)
            arrayto[i][j]=arrayfrom[i][j];

    return;
}

/***************
** Assignment **
***************/
static void Assignment(farlong arraybase[][ASSIGNCOLS])
{
    short assignedtableau[ASSIGNROWS][ASSIGNCOLS];

    /*
     ** First, calculate minimum costs
     */
    calc_minimum_costs(arraybase);

    /*
     ** Repeat following until the number of rows selected
     ** equals the number of rows in the tableau.
     */
    while(first_assignments(arraybase,assignedtableau)!=ASSIGNROWS)
    {         second_assignments(arraybase,assignedtableau);
    }

#ifdef DEBUG
    {
        int i,j;
        printf("\nColumn choices for each row\n");
        for(i=0;i<ASSIGNROWS;i++)
        {
            printf("R%03d: ",i);
            for(j=0;j<ASSIGNCOLS;j++)
                if(assignedtableau[i][j]==1)
                    printf("%03d ",j);
        }
    }
#endif

    return;
}

/***********************
** calc_minimum_costs **
************************
** Revise the tableau by calculating the minimum costs on a
** row and column basis.  These minima are subtracted from
** their rows and columns, creating a new tableau.
*/
static void calc_minimum_costs(long tableau[][ASSIGNCOLS])
{
    ushort i,j;              /* Index variables */
    long currentmin;        /* Current minimum */
    /*
     ** Determine minimum costs on row basis.  This is done by
     ** subtracting -- on a row-per-row basis -- the minum value
     ** for that row.
     */
    for(i=0;i<ASSIGNROWS;i++)
    {
        currentmin=MAXPOSLONG;  /* Initialize minimum */
        for(j=0;j<ASSIGNCOLS;j++)
            if(tableau[i][j]<currentmin)
                currentmin=tableau[i][j];

        for(j=0;j<ASSIGNCOLS;j++)
            tableau[i][j]-=currentmin;
    }

    /*
     ** Determine minimum cost on a column basis.  This works
     ** just as above, only now we step through the array
     ** column-wise
     */
    for(j=0;j<ASSIGNCOLS;j++)
    {
        currentmin=MAXPOSLONG;  /* Initialize minimum */
        for(i=0;i<ASSIGNROWS;i++)
            if(tableau[i][j]<currentmin)
                currentmin=tableau[i][j];

        /*
         ** Here, we'll take the trouble to see if the current
         ** minimum is zero.  This is likely worth it, since the
         ** preceding loop will have created at least one zero in
         ** each row.  We can save ourselves a few iterations.
         */
        if(currentmin!=0)
            for(i=0;i<ASSIGNROWS;i++)
                tableau[i][j]-=currentmin;
    }

    return;
}

/**********************
** first_assignments **
***********************
** Do first assignments.
** The assignedtableau[] array holds a set of values that
** indicate the assignment of a value, or its elimination.
** The values are:
**      0 = Item is neither assigned nor eliminated.
**      1 = Item is assigned
**      2 = Item is eliminated
** Returns the number of selections made.  If this equals
** the number of rows, then an optimum has been determined.
*/
static int first_assignments(long tableau[][ASSIGNCOLS],
        short assignedtableau[][ASSIGNCOLS])
{
    ushort i,j,k;                   /* Index variables */
    ushort numassigns;              /* # of assignments */
    ushort totnumassigns;           /* Total # of assignments */
    ushort numzeros;                /* # of zeros in row */
    int selected=0;                 /* Flag used to indicate selection */

    /*
     ** Clear the assignedtableau, setting all members to show that
     ** no one is yet assigned, eliminated, or anything.
     */
    for(i=0;i<ASSIGNROWS;i++)
        for(j=0;j<ASSIGNCOLS;j++)
            assignedtableau[i][j]=0;

    totnumassigns=0;
    do {
        numassigns=0;
        /*
         ** Step through rows.  For each one that is not currently
         ** assigned, see if the row has only one zero in it.  If so,
         ** mark that as an assigned row/col.  Eliminate other zeros
         ** in the same column.
         */
        for(i=0;i<ASSIGNROWS;i++)
        {       numzeros=0;
            for(j=0;j<ASSIGNCOLS;j++)
                if(tableau[i][j]==0L)
                    if(assignedtableau[i][j]==0)
                    {       numzeros++;
                        selected=j;
                    }
            if(numzeros==1)
            {       numassigns++;
                totnumassigns++;
                assignedtableau[i][selected]=1;
                for(k=0;k<ASSIGNROWS;k++)
                    if((k!=i) &&
                            (tableau[k][selected]==0))
                        assignedtableau[k][selected]=2;
            }
        }
        /*
         ** Step through columns, doing same as above.  Now, be careful
         ** of items in the other rows of a selected column.
         */
        for(j=0;j<ASSIGNCOLS;j++)
        {       numzeros=0;
            for(i=0;i<ASSIGNROWS;i++)
                if(tableau[i][j]==0L)
                    if(assignedtableau[i][j]==0)
                    {       numzeros++;
                        selected=i;
                    }
            if(numzeros==1)
            {       numassigns++;
                totnumassigns++;
                assignedtableau[selected][j]=1;
                for(k=0;k<ASSIGNCOLS;k++)
                    if((k!=j) &&
                            (tableau[selected][k]==0))
                        assignedtableau[selected][k]=2;
            }
        }
        /*
         ** Repeat until no more assignments to be made.
         */
    } while(numassigns!=0);

    /*
     ** See if we can leave at this point.
     */
    if(totnumassigns==ASSIGNROWS) return(totnumassigns);

    /*
     ** Now step through the array by row.  If you find any unassigned
     ** zeros, pick the first in the row.  Eliminate all zeros from
     ** that same row & column.  This occurs if there are multiple optima...
     ** possibly.
     */
    for(i=0;i<ASSIGNROWS;i++)
    {       selected=-1;
        for(j=0;j<ASSIGNCOLS;j++)
            if((tableau[i][j]==0L) &&
                    (assignedtableau[i][j]==0))
            {       selected=j;
                break;
            }
        if(selected!=-1)
        {       assignedtableau[i][selected]=1;
            totnumassigns++;
            for(k=0;k<ASSIGNCOLS;k++)
                if((k!=selected) &&
                        (tableau[i][k]==0L))
                    assignedtableau[i][k]=2;
            for(k=0;k<ASSIGNROWS;k++)
                if((k!=i) &&
                        (tableau[k][selected]==0L))
                    assignedtableau[k][selected]=2;
        }
    }

    return(totnumassigns);
}

/***********************
** second_assignments **
************************
** This section of the algorithm creates the revised
** tableau, and is difficult to explain.  I suggest you
** refer to the algorithm's source, mentioned in comments
** toward the beginning of the program.
*/
static void second_assignments(long tableau[][ASSIGNCOLS],
        short assignedtableau[][ASSIGNCOLS])
{
    int i,j;                                /* Indexes */
    short linesrow[ASSIGNROWS];
    short linescol[ASSIGNCOLS];
    long smallest;                          /* Holds smallest value */
    ushort numassigns;                      /* Number of assignments */
    ushort newrows;                         /* New rows to be considered */
    /*
     ** Clear the linesrow and linescol arrays.
     */
    for(i=0;i<ASSIGNROWS;i++)
        linesrow[i]=0;
    for(i=0;i<ASSIGNCOLS;i++)
        linescol[i]=0;

    /*
     ** Scan rows, flag each row that has no assignment in it.
     */
    for(i=0;i<ASSIGNROWS;i++)
    {       numassigns=0;
        for(j=0;j<ASSIGNCOLS;j++)
            if(assignedtableau[i][j]==1)
            {       numassigns++;
                break;
            }
        if(numassigns==0) linesrow[i]=1;
    }

    do {

        newrows=0;
        /*
         ** For each row checked above, scan for any zeros.  If found,
         ** check the associated column.
         */
        for(i=0;i<ASSIGNROWS;i++)
        {       if(linesrow[i]==1)
            for(j=0;j<ASSIGNCOLS;j++)
                if(tableau[i][j]==0)
                    linescol[j]=1;
        }

        /*
         ** Now scan checked columns.  If any contain assigned zeros, check
         ** the associated row.
         */
        for(j=0;j<ASSIGNCOLS;j++)
            if(linescol[j]==1)
                for(i=0;i<ASSIGNROWS;i++)
                    if((assignedtableau[i][j]==1) &&
                            (linesrow[i]!=1))
                    {
                        linesrow[i]=1;
                        newrows++;
                    }
    } while(newrows!=0);

    /*
     ** linesrow[n]==0 indicate rows covered by imaginary line
     ** linescol[n]==1 indicate cols covered by imaginary line
     ** For all cells not covered by imaginary lines, determine smallest
     ** value.
     */
    smallest=MAXPOSLONG;
    for(i=0;i<ASSIGNROWS;i++)
        if(linesrow[i]!=0)
            for(j=0;j<ASSIGNCOLS;j++)
                if(linescol[j]!=1)
                    if(tableau[i][j]<smallest)
                        smallest=tableau[i][j];

    /*
     ** Subtract smallest from all cells in the above set.
     */
    for(i=0;i<ASSIGNROWS;i++)
        if(linesrow[i]!=0)
            for(j=0;j<ASSIGNCOLS;j++)
                if(linescol[j]!=1)
                    tableau[i][j]-=smallest;

    /*
     ** Add smallest to all cells covered by two lines.
     */
    for(i=0;i<ASSIGNROWS;i++)
        if(linesrow[i]==0)
            for(j=0;j<ASSIGNCOLS;j++)
                if(linescol[j]==1)
                    tableau[i][j]+=smallest;

    return;
}
