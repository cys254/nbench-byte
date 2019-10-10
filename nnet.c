/*
** nnet.c
*/

/********************************
**       BYTEmark (tm)         **
** BYTE NATIVE MODE BENCHMARKS **
**       VERSION 2             **
**                             **
** Included in this source     **
** file:                       **
**  Back prop. neural net      **
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

/********************************
** BACK PROPAGATION NEURAL NET **
*********************************
** This code is a modified version of the code
** that was submitted to BYTE Magazine by
** Maureen Caudill.  It accomanied an article
** that I CANNOT NOW RECALL.
** The author's original heading/comment was
** as follows:
**
**  Backpropagation Network
**  Written by Maureen Caudill
**  in Think C 4.0 on a Macintosh
**
**  (c) Maureen Caudill 1988-1991
**  This network will accept 5x7 input patterns
**  and produce 8 bit output patterns.
**  The source code may be copied or modified without restriction,
**  but no fee may be charged for its use.
**
** ++++++++++++++
** I have modified the code so that it will work
** on systems other than a Macintosh -- RG
*/

/*
** DEFINES
*/
#define T 1                     /* TRUE */
#define F 0                     /* FALSE */
#define ERR -1
#define MAXPATS 10              /* max number of patterns in data file */
#define IN_X_SIZE 5             /* number of neurodes/row of input layer */
#define IN_Y_SIZE 7             /* number of neurodes/col of input layer */
#define IN_SIZE 35              /* equals IN_X_SIZE*IN_Y_SIZE */
#define MID_SIZE 8              /* number of neurodes in middle layer */
#define OUT_SIZE 8              /* number of neurodes in output layer */
#define MARGIN 0.1              /* how near to 1,0 do we have to come to stop? */
#define BETA 0.09               /* beta learning constant */
#define ALPHA 0.09              /* momentum term constant */
#define STOP 0.1                /* when worst_error less than STOP, training is done */

/*
** GLOBALS
*/
typedef struct {
    double  mid_wts[MID_SIZE][IN_SIZE];     /* middle layer weights */
    double  out_wts[OUT_SIZE][MID_SIZE];    /* output layer weights */
    double  mid_out[MID_SIZE];              /* middle layer output */
    double  out_out[OUT_SIZE];              /* output layer output */
    double  mid_error[MID_SIZE];            /* middle layer errors */
    double  out_error[OUT_SIZE];            /* output layer errors */
    double  mid_wt_change[MID_SIZE][IN_SIZE]; /* storage for last wt change */
    double  out_wt_change[OUT_SIZE][MID_SIZE]; /* storage for last wt change */
    double  in_pats[MAXPATS][IN_SIZE];      /* input patterns */
    double  out_pats[MAXPATS][OUT_SIZE];    /* desired output patterns */
    double  tot_out_error[MAXPATS];         /* measure of whether net is done */
    double  out_wt_cum_change[OUT_SIZE][MID_SIZE]; /* accumulated wt changes */
    double  mid_wt_cum_change[MID_SIZE][IN_SIZE];  /* accumulated wt changes */

    double  worst_error; /* worst error each pass through the data */
    double  average_error; /* average error each pass through the data */
    double  avg_out_error[MAXPATS]; /* average error each pattern */

    int iteration_count;    /* number of passes thru network so far */
    int numpats;            /* number of patterns in data file */
    int numpasses;          /* number of training passes through data file */
    int learned;            /* flag--if TRUE, network has learned all patterns */
} NNetData;

#ifdef DOS16
typedef NNetData huge * farnnetdataptr;
#else
typedef NNetData * farnnetdataptr;
#endif

/*
** PROTOTYPES
*/
void DoNNET(void);
void *NNetFunc(void *data);
static void DoNNetIteration(NNetData *nnetdata, ulong nloops, StopWatchStruct *stopwatch);
static void do_mid_forward(NNetData *nnetdata, int patt);
static void do_out_forward(NNetData *nnetdata);
void display_output(int patt);
static void do_forward_pass(NNetData *nnetdata, int patt);
static void do_out_error(NNetData *nnetdata, int patt);
static void worst_pass_error(NNetData *nnetdata);
static void do_mid_error(NNetData *nnetdata);
static void adjust_out_wts(NNetData *nnetdata);
static void adjust_mid_wts(NNetData *nnetdata, int patt);
static void do_back_pass(NNetData *nnetdata, int patt);
static void move_wt_changes(NNetData *nnetdata);
static int check_out_error(NNetData *nnetdata);
static void zero_changes(NNetData *nnetdata);
static void randomize_wts(NNetData *nnetdata);
static int read_data_file(NNetData *nnetdata);
/* static int initialize_net(); */

/*
** The Neural Net test requires an input data file.
** The name is specified here.
*/
char *inpath="NNET.DAT";

/***********
** DoNNet **
************
** Perform the neural net benchmark.
** Note that this benchmark is one of the few that
** requires an input file.  That file is "NNET.DAT" and
** should be on the local directory (from which the
** benchmark program in launched).
*/
void DoNNET(void)
{
    TestControlStruct *locnnetstruct;      /* Local ptr to global data */
    StopWatchStruct stopwatch;             /* Stop watch to time the test */
    NNetData nnetdata;                     /* NNet test data */

    /*
     ** Link to global data
     */
    locnnetstruct=&global_nnetstruct;

    memset(&nnetdata, 0, sizeof(NNetData));

    /*
     ** Init random number generator.
     ** NOTE: It is important that the random number generator
     **  be re-initialized for every pass through this test.
     **  The NNET algorithm uses the random number generator
     **  to initialize the net.  Results are sensitive to
     **  the initial neural net state.
     */
    randnum((int32)3);

    /*
     ** Read in the input and output patterns.  We'll do this
     ** only once here at the beginning.  These values don't
     ** change once loaded.
     */
    if(read_data_file(&nnetdata)!=0)
        ErrorExit();

    /*
     ** See if we need to perform self adjustment loop.
     */
    if(locnnetstruct->adjust==0)
    {
        /*
         ** Do self-adjustment.  This involves initializing the
         ** # of loops and increasing the loop count until we
         ** get a number of loops that we can use.
         */
        for(locnnetstruct->loops=1L;
                locnnetstruct->loops<MAXNNETLOOPS;
                locnnetstruct->loops++)
        {
            randnum((int32)3);
            ResetStopWatch(&stopwatch);
            DoNNetIteration(&nnetdata, locnnetstruct->loops, &stopwatch);
             
            if(stopwatch.realsecs>global_min_itersec) break;
        }
    }

    /*
     ** All's well if we get here.  Do the test.
     */
    run_bench_with_concurrency(locnnetstruct, NNetFunc);
}

/***********
** NNetFunc **
************
** Perform the neural net benchmark.
** Note that this benchmark is one of the few that
** requires an input file.  That file is "NNET.DAT" and
** should be on the local directory (from which the
** benchmark program in launched).
*/
void *NNetFunc(void *data)
{
    TestThreadData *testdata;              /* test data passed from thread func */
    TestControlStruct *locnnetstruct;      /* Local ptr to global data */
    StopWatchStruct stopwatch;             /* Stop watch to time the test */
    NNetData nnetdata;                     /* NNet test data */

    testdata = (TestThreadData*)data;
    locnnetstruct = testdata->control;

    memset(&nnetdata, 0, sizeof(NNetData));

    /*
     ** Init random number generator.
     ** NOTE: It is important that the random number generator
     **  be re-initialized for every pass through this test.
     **  The NNET algorithm uses the random number generator
     **  to initialize the net.  Results are sensitive to
     **  the initial neural net state.
     */
    randnum((int32)3);

    /*
     ** Read in the input and output patterns.  We'll do this
     ** only once here at the beginning.  These values don't
     ** change once loaded.
     */
    if(read_data_file(&nnetdata)!=0)
        ErrorExit();

    /*
     ** All's well if we get here.  Do the test.
     */
    testdata->result.iterations=(double)0.0;
    ResetStopWatch(&stopwatch);

    do {
        randnum((int32)3);    /* Gotta do this for Neural Net */
        DoNNetIteration(&nnetdata, locnnetstruct->loops, &stopwatch);
        testdata->result.iterations+=(double)locnnetstruct->loops;
    } while(stopwatch.realsecs<locnnetstruct->request_secs);

    return 0;
}

/********************
** DoNNetIteration **
*********************
** Do a single iteration of the neural net benchmark.
** By iteration, we mean a "learning" pass.
*/
static void DoNNetIteration(NNetData *nnetdata, ulong nloops, StopWatchStruct *stopwatch)
{
    int patt;

    /*
     ** Run nloops learning cycles.  Notice that, counted with
     ** the learning cycle is the weight randomization and
     ** zeroing of changes.  This should reduce clock jitter,
     ** since we don't have to stop and start the clock for
     ** each iteration.
     */
    StartStopWatch(stopwatch);
    while(nloops--)
    {
        randomize_wts(nnetdata);
        zero_changes(nnetdata);
        nnetdata->iteration_count=1;
        nnetdata->learned = F;
        nnetdata->numpasses = 0;
        while (nnetdata->learned == F)
        {
            for (patt=0; patt<nnetdata->numpats; patt++)
            {
                nnetdata->worst_error = 0.0;      /* reset this every pass through data */
                move_wt_changes(nnetdata);      /* move last pass's wt changes to momentum array */
                do_forward_pass(nnetdata, patt);
                do_back_pass(nnetdata, patt);
                nnetdata->iteration_count++;
            }
            nnetdata->numpasses ++;
            nnetdata->learned = check_out_error(nnetdata);
        }
#ifdef DEBUG
        printf("Learned in %d passes\n",numpasses);
#endif
    }
    StopStopWatch(stopwatch);
}

/*************************
** do_mid_forward(patt) **
**************************
** Process the middle layer's forward pass
** The activation of middle layer's neurode is the weighted
** sum of the inputs from the input pattern, with sigmoid
** function applied to the inputs.
**/
static void  do_mid_forward(NNetData *nnetdata, int patt)
{
    double  sum;
    int     neurode, i;

    for (neurode=0;neurode<MID_SIZE; neurode++)
    {
        sum = 0.0;
        for (i=0; i<IN_SIZE; i++)
        {       /* compute weighted sum of input signals */
            sum += nnetdata->mid_wts[neurode][i]*nnetdata->in_pats[patt][i];
        }
        /*
         ** apply sigmoid function f(x) = 1/(1+exp(-x)) to weighted sum
         */
        sum = 1.0/(1.0+exp(-sum));
        nnetdata->mid_out[neurode] = sum;
    }
    return;
}

/*********************
** do_out_forward() **
**********************
** process the forward pass through the output layer
** The activation of the output layer is the weighted sum of
** the inputs (outputs from middle layer), modified by the
** sigmoid function.
**/
static void  do_out_forward(NNetData *nnetdata)
{
    double sum;
    int neurode, i;

    for (neurode=0; neurode<OUT_SIZE; neurode++)
    {
        sum = 0.0;
        for (i=0; i<MID_SIZE; i++)
        {       /*
                 ** compute weighted sum of input signals
                 ** from middle layer
                 */
            sum += nnetdata->out_wts[neurode][i]*nnetdata->mid_out[i];
        }
        /*
         ** Apply f(x) = 1/(1+exp(-x)) to weighted input
         */
        sum = 1.0/(1.0+exp(-sum));
        nnetdata->out_out[neurode] = sum;
    }
    return;
}

/*************************
** display_output(patt) **
**************************
** Display the actual output vs. the desired output of the
** network.
** Once the training is complete, and the "learned" flag set
** to TRUE, then display_output sends its output to both
** the screen and to a text output file.
**
** NOTE: This routine has been disabled in the benchmark
** version. -- RG
**/
/*
void  display_output(int patt)
{
int             i;

fprintf(outfile,"\n Iteration # %d",iteration_count);
fprintf(outfile,"\n Desired Output:  ");

for (i=0; i<OUT_SIZE; i++)
{
fprintf(outfile,"%6.3f  ",out_pats[patt][i]);
}
fprintf(outfile,"\n Actual Output:   ");

for (i=0; i<OUT_SIZE; i++)
{
fprintf(outfile,"%6.3f  ",out_out[i]);
}
fprintf(outfile,"\n");
return;
}
*/

/**********************
** do_forward_pass() **
***********************
** control function for the forward pass through the network
** NOTE: I have disabled the call to display_output() in
**  the benchmark version -- RG.
**/
static void  do_forward_pass(NNetData *nnetdata, int patt)
{
    do_mid_forward(nnetdata, patt);   /* process forward pass, middle layer */
    do_out_forward(nnetdata);         /* process forward pass, output layer */
    /* display_output(patt);        ** display results of forward pass */
    return;
}

/***********************
** do_out_error(patt) **
************************
** Compute the error for the output layer neurodes.
** This is simply Desired - Actual.
**/
static void do_out_error(NNetData *nnetdata, int patt)
{
int neurode;
double error,tot_error, sum;

tot_error = 0.0;
sum = 0.0;
for (neurode=0; neurode<OUT_SIZE; neurode++)
{
    nnetdata->out_error[neurode] = nnetdata->out_pats[patt][neurode] - nnetdata->out_out[neurode];
    /*
    ** while we're here, also compute magnitude
    ** of total error and worst error in this pass.
    ** We use these to decide if we are done yet.
    */
    error = nnetdata->out_error[neurode];
    if (error <0.0)
    {
        sum += -error;
        if (-error > tot_error)
            tot_error = -error; /* worst error this pattern */
    }
    else
    {
        sum += error;
        if (error > tot_error)
            tot_error = error; /* worst error this pattern */
    }
}
nnetdata->avg_out_error[patt] = sum/OUT_SIZE;
nnetdata->tot_out_error[patt] = tot_error;
return;
}

/***********************
** worst_pass_error() **
************************
** Find the worst and average error in the pass and save it
**/
static void  worst_pass_error(NNetData *nnetdata)
{
    double error,sum;

    int i;

    error = 0.0;
    sum = 0.0;
    for (i=0; i<nnetdata->numpats; i++)
    {
        if (nnetdata->tot_out_error[i] > error) error = nnetdata->tot_out_error[i];
        sum += nnetdata->avg_out_error[i];
    }
    nnetdata->worst_error = error;
    nnetdata->average_error = sum/nnetdata->numpats;
    return;
}

/*******************
** do_mid_error() **
********************
** Compute the error for the middle layer neurodes
** This is based on the output errors computed above.
** Note that the derivative of the sigmoid f(x) is
**        f'(x) = f(x)(1 - f(x))
** Recall that f(x) is merely the output of the middle
** layer neurode on the forward pass.
**/
static void do_mid_error(NNetData *nnetdata)
{
    double sum;
    int neurode, i;

    for (neurode=0; neurode<MID_SIZE; neurode++)
    {
        sum = 0.0;
        for (i=0; i<OUT_SIZE; i++)
            sum += nnetdata->out_wts[i][neurode]*nnetdata->out_error[i];

        /*
         ** apply the derivative of the sigmoid here
         ** Because of the choice of sigmoid f(I), the derivative
         ** of the sigmoid is f'(I) = f(I)(1 - f(I))
         */
        nnetdata->mid_error[neurode] = nnetdata->mid_out[neurode]*(1-nnetdata->mid_out[neurode])*sum;
    }
    return;
}

/*********************
** adjust_out_wts() **
**********************
** Adjust the weights of the output layer.  The error for
** the output layer has been previously propagated back to
** the middle layer.
** Use the Delta Rule with momentum term to adjust the weights.
**/
static void adjust_out_wts(NNetData *nnetdata)
{
    int weight, neurode;
    double learn,delta,alph;

    learn = BETA;
    alph  = ALPHA;
    for (neurode=0; neurode<OUT_SIZE; neurode++)
    {
        for (weight=0; weight<MID_SIZE; weight++)
        {
            /* standard delta rule */
            delta = learn * nnetdata->out_error[neurode] * nnetdata->mid_out[weight];

            /* now the momentum term */
            delta += alph * nnetdata->out_wt_change[neurode][weight];
            nnetdata->out_wts[neurode][weight] += delta;

            /* keep track of this pass's cum wt changes for next pass's momentum */
            nnetdata->out_wt_cum_change[neurode][weight] += delta;
        }
    }
    return;
}

/*************************
** adjust_mid_wts(patt) **
**************************
** Adjust the middle layer weights using the previously computed
** errors.
** We use the Generalized Delta Rule with momentum term
**/
static void adjust_mid_wts(NNetData *nnetdata, int patt)
{
    int weight, neurode;
    double learn,alph,delta;

    learn = BETA;
    alph  = ALPHA;
    for (neurode=0; neurode<MID_SIZE; neurode++)
    {
        for (weight=0; weight<IN_SIZE; weight++)
        {
            /* first the basic delta rule */
            delta = learn * nnetdata->mid_error[neurode] * nnetdata->in_pats[patt][weight];

            /* with the momentum term */
            delta += alph * nnetdata->mid_wt_change[neurode][weight];
            nnetdata->mid_wts[neurode][weight] += delta;

            /* keep track of this pass's cum wt changes for next pass's momentum */
            nnetdata->mid_wt_cum_change[neurode][weight] += delta;
        }
    }
    return;
}

/*******************
** do_back_pass() **
********************
** Process the backward propagation of error through network.
**/
void  do_back_pass(NNetData *nnetdata, int patt)
{

    do_out_error(nnetdata, patt);
    do_mid_error(nnetdata);
    adjust_out_wts(nnetdata);
    adjust_mid_wts(nnetdata, patt);

    return;
}


/**********************
** move_wt_changes() **
***********************
** Move the weight changes accumulated last pass into the wt-change
** array for use by the momentum term in this pass. Also zero out
** the accumulating arrays after the move.
**/
static void move_wt_changes(NNetData *nnetdata)
{
    int i,j;

    for (i = 0; i<MID_SIZE; i++)
        for (j = 0; j<IN_SIZE; j++)
        {
            nnetdata->mid_wt_change[i][j] = nnetdata->mid_wt_cum_change[i][j];
            /*
             ** Zero it out for next pass accumulation.
             */
            nnetdata->mid_wt_cum_change[i][j] = 0.0;
        }

    for (i = 0; i<OUT_SIZE; i++)
        for (j=0; j<MID_SIZE; j++)
        {
            nnetdata->out_wt_change[i][j] = nnetdata->out_wt_cum_change[i][j];
            nnetdata->out_wt_cum_change[i][j] = 0.0;
        }

    return;
}

/**********************
** check_out_error() **
***********************
** Check to see if the error in the output layer is below
** MARGIN*OUT_SIZE for all output patterns.  If so, then
** assume the network has learned acceptably well.  This
** is simply an arbitrary measure of how well the network
** has learned -- many other standards are possible.
**/
static int check_out_error(NNetData *nnetdata)
{
    int result,i,error;

    result  = T;
    error   = F;
    worst_pass_error(nnetdata);     /* identify the worst error in this pass */

    /*
#ifdef DEBUG
printf("\n Iteration # %d",iteration_count);
#endif
     */
    for (i=0; i<nnetdata->numpats; i++)
    {
        /*      printf("\n Error pattern %d:   Worst: %8.3f; Average: %8.3f",
                i+1,tot_out_error[i], avg_out_error[i]);
                fprintf(outfile,
                "\n Error pattern %d:   Worst: %8.3f; Average: %8.3f",
                i+1,tot_out_error[i]);
         */

        if (nnetdata->worst_error >= STOP) result = F;
        if (nnetdata->tot_out_error[i] >= 16.0) error = T;
    }

    if (error == T) result = ERR;


#ifdef DEBUG
    /* printf("\n Error this pass thru data:   Worst: %8.3f; Average: %8.3f",
       worst_error,average_error);
     */
    /* fprintf(outfile,
       "\n Error this pass thru data:   Worst: %8.3f; Average: %8.3f",
       worst_error, average_error); */
#endif

    return(result);
}


/*******************
** zero_changes() **
********************
** Zero out all the wt change arrays
**/
static void zero_changes(NNetData *nnetdata)
{
    int i,j;

    for (i = 0; i<MID_SIZE; i++)
    {
        for (j=0; j<IN_SIZE; j++)
        {
            nnetdata->mid_wt_change[i][j] = 0.0;
            nnetdata->mid_wt_cum_change[i][j] = 0.0;
        }
    }

    for (i = 0; i< OUT_SIZE; i++)
    {
        for (j=0; j<MID_SIZE; j++)
        {
            nnetdata->out_wt_change[i][j] = 0.0;
            nnetdata->out_wt_cum_change[i][j] = 0.0;
        }
    }
    return;
}


/********************
** randomize_wts() **
*********************
** Intialize the weights in the middle and output layers to
** random values between -0.25..+0.25
** Function rand() returns a value between 0 and 32767.
**
** NOTE: Had to make alterations to how the random numbers were
** created.  -- RG.
**/
static void randomize_wts(NNetData *nnetdata)
{
    int neurode,i;
    double value;

    /*
     ** Following not used int benchmark version -- RG
     **
     **        printf("\n Please enter a random number seed (1..32767):  ");
     **        scanf("%d", &i);
     **        srand(i);
     */

    for (neurode = 0; neurode<MID_SIZE; neurode++)
    {
        for(i=0; i<IN_SIZE; i++)
        {
            /* value=(double)abs_randwc(100000L); */
            value=(double)abs_randwc((int32)100000);
            value=value/(double)100000.0 - (double) 0.5;
            nnetdata->mid_wts[neurode][i] = value/2;
        }
    }
    for (neurode=0; neurode<OUT_SIZE; neurode++)
    {
        for(i=0; i<MID_SIZE; i++)
        {
            /* value=(double)abs_randwc(100000L); */
            value=(double)abs_randwc((int32)100000);
            value=value/(double)10000.0 - (double) 0.5;
            nnetdata->out_wts[neurode][i] = value/2;
        }
    }

    return;
}


/*********************
** read_data_file() **
**********************
** Read in the input data file and store the patterns in
** in_pats and out_pats.
** The format for the data file is as follows:
**
** line#   data expected
** -----   ------------------------------
** 1               In-X-size,in-y-size,out-size
** 2               number of patterns in file
** 3               1st X row of 1st input pattern
** 4..             following rows of 1st input pattern pattern
**                 in-x+2  y-out pattern
**                                 1st X row of 2nd pattern
**                 etc.
**
** Each row of data is separated by commas or spaces.
** The data is expected to be ascii text corresponding to
** either a +1 or a 0.
**
** Sample input for a 1-pattern file (The comments to the
** right may NOT be in the file unless more sophisticated
** parsing of the input is done.):
**
** 5,7,8                      input is 5x7 grid, output is 8 bits
** 1                          one pattern in file
** 0,1,1,1,0                  beginning of pattern for "O"
** 1,0,0,0,1
** 1,0,0,0,1
** 1,0,0,0,1
** 1,0,0,0,1
** 1,0,0,0,0
** 0,1,1,1,0
** 0,1,0,0,1,1,1,1            ASCII code for "O" -- 0100 1111
**
** Clearly, this simple scheme can be expanded or enhanced
** any way you like.
**
** Returns -1 if any file error occurred, otherwise 0.
**/
static int read_data_file(NNetData *nnetdata)
{
    FILE *infile;

    int xinsize,yinsize,youtsize;
    int patt, element, i, row;
    int vals_read;
    int val1,val2,val3,val4,val5,val6,val7,val8;

    /* printf("\n Opening and retrieving data from file."); */

    infile = fopen(inpath, "r");
    if (infile == NULL)
    {
        printf("\n CPU:NNET--error in opening file!");
        return -1 ;
    }
    vals_read =fscanf(infile,"%d  %d  %d",&xinsize,&yinsize,&youtsize);
    if (vals_read != 3)
    {
        printf("\n CPU:NNET -- Should read 3 items in line one; did read %d",vals_read);
        return -1;
    }
    vals_read=fscanf(infile,"%d",&nnetdata->numpats);
    if (vals_read !=1)
    {
        printf("\n CPU:NNET -- Should read 1 item in line 2; did read %d",vals_read);
        return -1;
    }
    if (nnetdata->numpats > MAXPATS)
        nnetdata->numpats = MAXPATS;

    for (patt=0; patt<nnetdata->numpats; patt++)
    {
        element = 0;
        for (row = 0; row<yinsize; row++)
        {
            vals_read = fscanf(infile,"%d  %d  %d  %d  %d",
                    &val1, &val2, &val3, &val4, &val5);
            if (vals_read != 5)
            {
                printf ("\n CPU:NNET -- failure in reading input!");
                return -1;
            }
            element=row*xinsize;

            nnetdata->in_pats[patt][element] = (double) val1; element++;
            nnetdata->in_pats[patt][element] = (double) val2; element++;
            nnetdata->in_pats[patt][element] = (double) val3; element++;
            nnetdata->in_pats[patt][element] = (double) val4; element++;
            nnetdata->in_pats[patt][element] = (double) val5; element++;
        }
        for (i=0;i<IN_SIZE; i++)
        {
            if (nnetdata->in_pats[patt][i] >= 0.9)
                nnetdata->in_pats[patt][i] = 0.9;
            if (nnetdata->in_pats[patt][i] <= 0.1)
                nnetdata->in_pats[patt][i] = 0.1;
        }
        element = 0;
        vals_read = fscanf(infile,"%d  %d  %d  %d  %d  %d  %d  %d",
                &val1, &val2, &val3, &val4, &val5, &val6, &val7, &val8);

        nnetdata->out_pats[patt][element] = (double) val1; element++;
        nnetdata->out_pats[patt][element] = (double) val2; element++;
        nnetdata->out_pats[patt][element] = (double) val3; element++;
        nnetdata->out_pats[patt][element] = (double) val4; element++;
        nnetdata->out_pats[patt][element] = (double) val5; element++;
        nnetdata->out_pats[patt][element] = (double) val6; element++;
        nnetdata->out_pats[patt][element] = (double) val7; element++;
        nnetdata->out_pats[patt][element] = (double) val8; element++;
    }

    /* printf("\n Closing the input file now. "); */

    fclose(infile);
    return(0);
}

/*********************
** initialize_net() **
**********************
** Do all the initialization stuff before beginning
*/
/*
static int initialize_net()
{
int err_code;

randomize_wts();
zero_changes();
err_code = read_data_file();
iteration_count = 1;
return(err_code);
}
*/

/**********************
** display_mid_wts() **
***********************
** Display the weights on the middle layer neurodes
** NOTE: This routine is not used in the benchmark
**  test -- RG
**/
/* static void display_mid_wts()
{
int             neurode, weight, row, col;

fprintf(outfile,"\n Weights of Middle Layer neurodes:");

for (neurode=0; neurode<MID_SIZE; neurode++)
{
    fprintf(outfile,"\n  Mid Neurode # %d",neurode);
    for (row=0; row<IN_Y_SIZE; row++)
    {
        fprintf(outfile,"\n ");
        for (col=0; col<IN_X_SIZE; col++)
        {
            weight = IN_X_SIZE * row + col;
            fprintf(outfile," %8.3f ", mid_wts[neurode][weight]);
        }
    }
}
return;
}
*/
/**********************
** display_out_wts() **
***********************
** Display the weights on the output layer neurodes
** NOTE: This code is not used in the benchmark
**  test -- RG
*/
/* void  display_out_wts()
{
int             neurode, weight;

    fprintf(outfile,"\n Weights of Output Layer neurodes:");

    for (neurode=0; neurode<OUT_SIZE; neurode++)
    {
        fprintf(outfile,"\n  Out Neurode # %d \n",neurode);
        for (weight=0; weight<MID_SIZE; weight++)
        {
            fprintf(outfile," %8.3f ", out_wts[neurode][weight]);
        }
    }
    return;
}
*/
