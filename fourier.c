/*
** fourier.c
*/

/********************************
**       BYTEmark (tm)         **
** BYTE NATIVE MODE BENCHMARKS **
**       VERSION 2             **
**                             **
** Included in this source     **
** file:                       **
**  Fourier coefficients       **
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
#include <math.h>
#include "nmglobal.h"
#include "sysspec.h"
#include "misc.h"

/*************************
** FOURIER COEFFICIENTS **
*************************/

void DoFourier(void);

void DoFourierAdjust(TestControlStruct *locfourierstruct);
void *FourierFunc(void *data);

static void DoFPUTransIteration(fardouble *abase,
		fardouble *bbase,
		ulong arraysize,
                StopWatchStruct *stopwatch);
static double TrapezoidIntegrate(double x0,
		double x1,
		int nsteps,
		double omegan,
		int select);
static double thefunction(double x,
		double omegan,
		int select);

/**************
** DoFourier **
***************
** Perform the transcendental/trigonometric portion of the
** benchmark.  This benchmark calculates the first n
** fourier coefficients of the function (x+1)^x defined
** on the interval 0,2.
*/
void DoFourier(void)
{
    TestControlStruct *locfourierstruct;        /* Local fourier struct */

    /*
     ** Link to global structure
     */
    locfourierstruct=&global_fourierstruct;

    /*
     ** See if we need to do self-adjustment code.
     */
    DoFourierAdjust(locfourierstruct);

    /*
     ** Run the benchmark
     */
    run_bench_with_concurrency(locfourierstruct, FourierFunc);
}

/********************
** DoFourierAdjust **
*********************
** Perform the transcendental/trigonometric portion of the
** benchmark.  This benchmark calculates the first n
** fourier coefficients of the function (x+1)^x defined
** on the interval 0,2.
*/
void DoFourierAdjust(TestControlStruct *locfourierstruct)
{
    /*
     ** See if we need to do self-adjustment code.
     */
    if(locfourierstruct->adjust==0)
    {
        fardouble *abase;               /* Base of A[] coefficients array */
        fardouble *bbase;               /* Base of B[] coefficients array */
        int systemerror;                /* For error code */
        StopWatchStruct stopwatch;

        locfourierstruct->arraysize=100L;       /* Start at 100 elements */
        while(1)
        {

            abase=(fardouble *)AllocateMemory(locfourierstruct->arraysize*sizeof(double),
                    &systemerror);
            if(systemerror)
            {
                ReportError(locfourierstruct->errorcontext,systemerror);
                ErrorExit();
            }

            bbase=(fardouble *)AllocateMemory(locfourierstruct->arraysize*sizeof(double),
                    &systemerror);
            if(systemerror)
            {
                ReportError(locfourierstruct->errorcontext,systemerror);
                FreeMemory((void *)abase,&systemerror);
                ErrorExit();
            }
            /*
             ** Do an iteration of the tests.  If the elapsed time is
             ** less than or equal to the permitted minimum, re-allocate
             ** larger arrays and try again.
             */
            ResetStopWatch(&stopwatch);
            DoFPUTransIteration(abase,bbase,
                        locfourierstruct->arraysize,&stopwatch);

            FreeMemory((farvoid *)abase,&systemerror);
            FreeMemory((farvoid *)bbase,&systemerror);

            if(stopwatch.realsecs>global_min_itersec)
                break;          /* We're ok...exit */

            /*
             ** Make bigger arrays and try again.
             */
            locfourierstruct->arraysize+=50L;
        }
        locfourierstruct->adjust=1;
    }
}

/****************
** FourierFunc **
*****************
** Perform the transcendental/trigonometric portion of the
** benchmark.  This benchmark calculates the first n
** fourier coefficients of the function (x+1)^x defined
** on the interval 0,2.
*/
void *FourierFunc(void *data)
{
    TestThreadData *testdata;       /* test data passed from thread func */
    fardouble *abase;               /* Base of A[] coefficients array */
    fardouble *bbase;               /* Base of B[] coefficients array */
    StopWatchStruct stopwatch;      /* Stop watch to time the test */
    int systemerror;                /* For error code */
    TestControlStruct *locfourierstruct;        /* Local fourier struct */

    testdata = (TestThreadData*)data;

    locfourierstruct = testdata->control;

    /*
     ** Allocate the arrays and go.
     */
    abase=(fardouble *)AllocateMemory(locfourierstruct->arraysize*sizeof(double),
                &systemerror);
    if(systemerror)
    {
        ReportError(locfourierstruct->errorcontext,systemerror);
        ErrorExit();
    }

    bbase=(fardouble *)AllocateMemory(locfourierstruct->arraysize*sizeof(double),
                &systemerror);
    if(systemerror)
    {
        ReportError(locfourierstruct->errorcontext,systemerror);
        FreeMemory((void *)abase,&systemerror);
        ErrorExit();
    }

    /*
     ** All's well if we get here.  Repeatedly perform integration
     ** tests until the accumulated time is greater than the
     ** # of seconds requested.
     */
    testdata->result.iterations=0.0;
    ResetStopWatch(&stopwatch);

    do {
        DoFPUTransIteration(abase,bbase,locfourierstruct->arraysize,&stopwatch);
        testdata->result.iterations+=(double)locfourierstruct->arraysize*(double)2.0-(double)1.0;
    } while(stopwatch.realsecs<locfourierstruct->request_secs);

    /*
     ** Clean up, calculate results, and go home.
     */
    FreeMemory((farvoid *)abase,&systemerror);
    FreeMemory((farvoid *)bbase,&systemerror);

    testdata->result.cpusecs = stopwatch.cpusecs;
    testdata->result.realsecs = stopwatch.realsecs;

    return 0;
}


/************************
** DoFPUTransIteration **
*************************
** Perform an iteration of the FPU Transcendental/trigonometric
** benchmark.  Here, an iteration consists of calculating the
** first n fourier coefficients of the function (x+1)^x on
** the interval 0,2.  n is given by arraysize.
** NOTE: The # of integration steps is fixed at
** 200.
*/
static void DoFPUTransIteration(fardouble *abase,      /* A coeffs. */
            fardouble *bbase,               /* B coeffs. */
            ulong arraysize,                /* # of coeffs */
            StopWatchStruct *stopwatch)
{
    double omega;           /* Fundamental frequency */
    unsigned long i;        /* Index */

    /*
     ** Start the stopwatch
     */
    StartStopWatch(stopwatch);

    /*
     ** Calculate the fourier series.  Begin by
     ** calculating A[0].
     */

    *abase=TrapezoidIntegrate((double)0.0,
            (double)2.0,
            200,
            (double)0.0,    /* No omega * n needed */
            0 )/(double)2.0;

    /*
     ** Calculate the fundamental frequency.
     ** ( 2 * pi ) / period...and since the period
     ** is 2, omega is simply pi.
     */
    omega=(double)3.1415926535897932;

    for(i=1;i<arraysize;i++)
    {

        /*
         ** Calculate A[i] terms.  Note, once again, that we
         ** can ignore the 2/period term outside the integral
         ** since the period is 2 and the term cancels itself
         ** out.
         */
        *(abase+i)=TrapezoidIntegrate((double)0.0,
                (double)2.0,
                200,
                omega * (double)i,
                1);

        /*
         ** Calculate the B[i] terms.
         */
        *(bbase+i)=TrapezoidIntegrate((double)0.0,
                (double)2.0,
                200,
                omega * (double)i,
                2);

    }
#ifdef DEBUG
    {
        int i;
        printf("\nA[i]=\n");
        for (i=0;i<arraysize;i++) printf("%7.3g ",abase[i]);
        printf("\nB[i]=\n(undefined) ");
        for (i=1;i<arraysize;i++) printf("%7.3g ",bbase[i]);
    }
#endif
    /*
     ** All done, stop the stopwatch
     */
    StopStopWatch(stopwatch);
}

/***********************
** TrapezoidIntegrate **
************************
** Perform a simple trapezoid integration on the
** function (x+1)**x.
** x0,x1 set the lower and upper bounds of the
** integration.
** nsteps indicates # of trapezoidal sections
** omegan is the fundamental frequency times
**  the series member #
** select = 0 for the A[0] term, 1 for cosine terms, and
**   2 for sine terms.
** Returns the value.
*/
static double TrapezoidIntegrate( double x0,            /* Lower bound */
            double x1,              /* Upper bound */
            int nsteps,             /* # of steps */
            double omegan,          /* omega * n */
            int select)
{
    double x;               /* Independent variable */
    double dx;              /* Stepsize */
    double rvalue;          /* Return value */


    /*
     ** Initialize independent variable
     */
    x=x0;

    /*
     ** Calculate stepsize
     */
    dx=(x1 - x0) / (double)nsteps;

    /*
     ** Initialize the return value.
     */
    rvalue=thefunction(x0,omegan,select)/(double)2.0;

    /*
     ** Compute the other terms of the integral.
     */
    if(nsteps!=1)
    {       --nsteps;               /* Already done 1 step */
        while(--nsteps )
        {
            x+=dx;
            rvalue+=thefunction(x,omegan,select);
        }
    }
    /*
     ** Finish computation
     */
    rvalue=(rvalue+thefunction(x1,omegan,select)/(double)2.0)*dx;

    return(rvalue);
}

/****************
** thefunction **
*****************
** This routine selects the function to be used
** in the Trapezoid integration.
** x is the independent variable
** omegan is omega * n
** select chooses which of the sine/cosine functions
**  are used.  note the special case for select=0.
*/
static double thefunction(double x,             /* Independent variable */
        double omegan,          /* Omega * term */
        int select)             /* Choose term */
{

    /*
     ** Use select to pick which function we call.
     */
    switch(select)
    {
        case 0: return(pow(x+(double)1.0,x));

        case 1: return(pow(x+(double)1.0,x) * cos(omegan * x));

        case 2: return(pow(x+(double)1.0,x) * sin(omegan * x));
    }

    /*
     ** We should never reach this point, but the following
     ** keeps compilers from issuing a warning message.
     */
    return(0.0);
}
