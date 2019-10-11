/*
** idea.c
*/

/********************************
**       BYTEmark (tm)         **
** BYTE NATIVE MODE BENCHMARKS **
**       VERSION 2             **
**                             **
** Included in this source     **
** file:                       **
**  IDEA Encyption             **
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

/********************
** IDEA Encryption **
*********************
** IDEA - International Data Encryption Algorithm.
** Based on code presented in Applied Cryptography by Bruce Schneier.
** Which was based on code developed by Xuejia Lai and James L. Massey.
** Other modifications made by Colin Plumb.
**
*/

/*
** DEFINES
*/
#define IDEAKEYSIZE 16
#define IDEABLOCKSIZE 8
#define ROUNDS 8
#define KEYLEN (6*ROUNDS+4)

/*
** MACROS
*/
#define low16(x) ((x) & 0x0FFFF)
#define MUL(x,y) (x=mul(low16(x),y))


/*
** TYPEDEFS
*/
typedef u16 IDEAkey[KEYLEN];

typedef struct {
    IDEAkey Z,DK;
    u16 userkey[8];
    int systemerror;
    faruchar *plain1;               /* First plaintext buffer */
    faruchar *crypt1;               /* Encryption buffer */
    faruchar *plain2;               /* Second plaintext buffer */
} IDEAData;

/*
** PROTOTYPES
*/
void DoIDEA(void);
void IDEADataSetup(TestControlStruct *locideastruct, IDEAData *ideadata);
void IDEADataCleanup(IDEAData *ideadata);
void DoIDEAAdjust(TestControlStruct *locideastruct);
void *IDEAFunc(void *data);
static void DoIDEAIteration(faruchar *plain1,
	faruchar *crypt1, faruchar *plain2,
	ulong arraysize, ulong nloops,
	IDEAkey Z, IDEAkey DK, StopWatchStruct *stopwatch);
static u16 mul(register u16 a, register u16 b);
static u16 inv(u16 x);
static void en_key_idea(u16 userkey[8], IDEAkey Z);
static void de_key_idea(IDEAkey Z, IDEAkey DK);
static void cipher_idea(u16 in[4], u16 out[4], IDEAkey Z);

/***********
** DoIDEA **
************
** Perform IDEA encryption.  Note that we time encryption & decryption
** time as being a single loop.
*/
void DoIDEA(void)
{
    TestControlStruct *locideastruct;      /* Loc pointer to global structure */

    /*
     ** Link to global data
     */
    locideastruct=&global_ideastruct;

    /*
     ** See if we need to do self adjustment code.
     */
    DoIDEAAdjust(locideastruct);

    /*
     ** Run the benchmark
     */
    run_bench_with_concurrency(locideastruct, IDEAFunc);
}


/******************
** IDEADataSetup **
*******************
** Setup IDEA test data
*/
void IDEADataSetup(TestControlStruct *locideastruct, IDEAData *ideadata)
{
    int i;
    int systemerror;

    /*
     ** Re-init random-number generator.
     */
    randnum((int32)3);

    /*
     ** Build an encryption/decryption key
     */
    for (i=0;i<8;i++)
            ideadata->userkey[i]=(u16)(abs_randwc((int32)60000) & 0xFFFF);
    for(i=0;i<KEYLEN;i++)
            ideadata->Z[i]=0;

    /*
     ** Compute encryption/decryption subkeys
     */
    en_key_idea(ideadata->userkey,ideadata->Z);
    de_key_idea(ideadata->Z,ideadata->DK);

    /*
     ** Allocate memory for buffers.  We'll make 3, called plain1,
     ** crypt1, and plain2.  It works like this:
     **   plain1 >>encrypt>> crypt1 >>decrypt>> plain2.
     ** So, plain1 and plain2 should match.
     ** Also, fill up plain1 with sample text.
     */
    ideadata->plain1=(faruchar *)AllocateMemory(locideastruct->arraysize,&systemerror);
    if(systemerror)
    {
        ReportError(locideastruct->errorcontext,systemerror);
        ErrorExit();
    }

    ideadata->crypt1=(faruchar *)AllocateMemory(locideastruct->arraysize,&systemerror);
    if(systemerror)
    {
        ReportError(locideastruct->errorcontext,systemerror);
        FreeMemory((farvoid *)ideadata->plain1,&systemerror);
        ErrorExit();
    }

    ideadata->plain2=(faruchar *)AllocateMemory(locideastruct->arraysize,&systemerror);
    if(systemerror)
    {
        ReportError(locideastruct->errorcontext,systemerror);
        FreeMemory((farvoid *)ideadata->plain1,&systemerror);
        FreeMemory((farvoid *)ideadata->crypt1,&systemerror);
        ErrorExit();
    }

    /*
     ** Note that we build the "plaintext" by simply loading
     ** the array up with random numbers.
     */
    for(i=0;i<locideastruct->arraysize;i++)
        ideadata->plain1[i]=(uchar)(abs_randwc(255) & 0xFF);
}

/********************
** IDEADataCleanup **
*********************
** Cleanup IDEA test data
*/
void IDEADataCleanup(IDEAData *ideadata)
{
    int systemerror;
    FreeMemory((farvoid *)ideadata->plain1,&systemerror);
    FreeMemory((farvoid *)ideadata->crypt1,&systemerror);
    FreeMemory((farvoid *)ideadata->plain2,&systemerror);
}


/*****************
** DoIDEAAdjust **
******************
** Perform self-adjust
*/
void DoIDEAAdjust(TestControlStruct *locideastruct)
{
    /*
     ** See if we need to perform self adjustment loop.
     */
    if(locideastruct->adjust==0)
    {
        IDEAData ideadata;          /* test data */
        StopWatchStruct stopwatch;      /* Stop watch to time the test */

        /*
         ** Setup test data
         */
        IDEADataSetup(locideastruct, &ideadata);

        /*
         ** Do self-adjustment.  This involves initializing the
         ** # of loops and increasing the loop count until we
         ** get a number of loops that we can use.
         */
        for(locideastruct->loops=1L;
                locideastruct->loops<MAXIDEALOOPS;
                locideastruct->loops*=2L) {
            ResetStopWatch(&stopwatch);
            DoIDEAIteration(ideadata.plain1,ideadata.crypt1,ideadata.plain2,
                        locideastruct->arraysize,
                        locideastruct->loops,
                        ideadata.Z,ideadata.DK,&stopwatch);
            if(stopwatch.realsecs>global_min_itersec) break;
        }

        /*
         ** Clean up, and go home.  Be sure to
         ** show that we don't have to rerun adjustment code.
         */
        IDEADataCleanup(&ideadata);

        locideastruct->adjust=1;
    }
}

/***********
** IDEAFunc **
************
** Perform IDEA encryption.  Note that we time encryption & decryption
** time as being a single loop.
*/
void *IDEAFunc(void *data)
{
    TestThreadData *testdata;              /* test data passed from thread func */
    TestControlStruct *locideastruct;      /* Loc pointer to global structure */
    StopWatchStruct stopwatch;             /* Stop watch to time the test */
    IDEAData ideadata;                     /* test data */

    testdata = (TestThreadData *)data;
    locideastruct = testdata->control;

    /*
     ** Setup test data
     */
    IDEADataSetup(locideastruct, &ideadata);

    /*
     ** All's well if we get here.  Do the test.
     */
    testdata->result.iterations=(double)0.0;
    ResetStopWatch(&stopwatch);

    do {
        DoIDEAIteration(ideadata.plain1,ideadata.crypt1,ideadata.plain2,
                locideastruct->arraysize,
                locideastruct->loops,ideadata.Z,ideadata.DK,&stopwatch);
        testdata->result.iterations+=(double)locideastruct->loops;
    } while(stopwatch.realsecs<locideastruct->request_secs);

    /*
     ** Clean up, calculate results, and go home.  Be sure to
     ** show that we don't have to rerun adjustment code.
     */
    IDEADataCleanup(&ideadata);

    testdata->result.cpusecs = stopwatch.cpusecs;
    testdata->result.realsecs = stopwatch.realsecs;

    return 0;
}

/********************
** DoIDEAIteration **
*********************
** Execute a single iteration of the IDEA encryption algorithm.
** Actually, a single iteration is one encryption and one
** decryption.
*/
static void DoIDEAIteration(faruchar *plain1,
            faruchar *crypt1,
            faruchar *plain2,
            ulong arraysize,
            ulong nloops,
            IDEAkey Z,
            IDEAkey DK,
            StopWatchStruct *stopwatch)
{
    register ulong i;
    register ulong j;
#ifdef DEBUG
    int status=0;
#endif

    /*
     ** Start the stopwatch.
     */
    StartStopWatch(stopwatch);

    /*
     ** Do everything for nloops.
     */
    for(i=0;i<nloops;i++)
    {
        for(j=0;j<arraysize;j+=(sizeof(u16)*4))
            cipher_idea((u16 *)(plain1+j),(u16 *)(crypt1+j),Z);       /* Encrypt */

        for(j=0;j<arraysize;j+=(sizeof(u16)*4))
            cipher_idea((u16 *)(crypt1+j),(u16 *)(plain2+j),DK);      /* Decrypt */
    }

#ifdef DEBUG
    for(j=0;j<arraysize;j++)
        if(*(plain1+j)!=*(plain2+j)){
            printf("IDEA Error! \n");
            status=1;
        }
    if (status==0) printf("IDEA: OK\n");
#endif

    /*
     ** Get elapsed time.
     */
    StopStopWatch(stopwatch);
}

/********
** mul **
*********
** Performs multiplication, modulo (2**16)+1.  This code is structured
** on the assumption that untaken branches are cheaper than taken
** branches, and that the compiler doesn't schedule branches.
*/
static u16 mul(register u16 a, register u16 b)
{
    register u32 p;
    if(a)
    {       if(b)
        {       p=(u32)(a*b);
            b=low16(p);
            a=(u16)(p>>16);
            return(b-a+(b<a));
        }
        else
            return(1-a);
    }
    else
        return(1-b);
}

/********
** inv **
*********
** Compute multiplicative inverse of x, modulo (2**16)+1
** using Euclid's GCD algorithm.  It is unrolled twice
** to avoid swapping the meaning of the registers.  And
** some subtracts are changed to adds.
*/
static u16 inv(u16 x)
{
    u16 t0, t1;
    u16 q, y;

    if(x<=1)
        return(x);      /* 0 and 1 are self-inverse */
    t1=0x10001 / x;
    y=0x10001 % x;
    if(y==1)
        return(low16(1-t1));
    t0=1;
    do {
        q=x/y;
        x=x%y;
        t0+=q*t1;
        if(x==1) return(t0);
        q=y/x;
        y=y%x;
        t1+=q*t0;
    } while(y!=1);
    return(low16(1-t1));
}

/****************
** en_key_idea **
*****************
** Compute IDEA encryption subkeys Z
*/
static void en_key_idea(u16 *userkey, u16 *Z)
{
    int i,j;

    /*
     ** shifts
     */
    for(j=0;j<8;j++)
        Z[j]=*userkey++;
    for(i=0;j<KEYLEN;j++)
    {       i++;
        Z[i+7]=(Z[i&7]<<9)| (Z[(i+1) & 7] >> 7);
        Z+=i&8;
        i&=7;
    }
    return;
}

/****************
** de_key_idea **
*****************
** Compute IDEA decryption subkeys DK from encryption
** subkeys Z.
*/
static void de_key_idea(IDEAkey Z, IDEAkey DK)
{
    IDEAkey TT;
    int j;
    u16 t1, t2, t3;
    u16 *p;
    p=(u16 *)(TT+KEYLEN);

    t1=inv(*Z++);
    t2=-*Z++;
    t3=-*Z++;
    *--p=inv(*Z++);
    *--p=t3;
    *--p=t2;
    *--p=t1;

    for(j=1;j<ROUNDS;j++)
    {       t1=*Z++;
        *--p=*Z++;
        *--p=t1;
        t1=inv(*Z++);
        t2=-*Z++;
        t3=-*Z++;
        *--p=inv(*Z++);
        *--p=t2;
        *--p=t3;
        *--p=t1;
    }
    t1=*Z++;
    *--p=*Z++;
    *--p=t1;
    t1=inv(*Z++);
    t2=-*Z++;
    t3=-*Z++;
    *--p=inv(*Z++);
    *--p=t3;
    *--p=t2;
    *--p=t1;
    /*
     ** Copy and destroy temp copy
     */
    for(j=0,p=TT;j<KEYLEN;j++)
    {       *DK++=*p;
        *p++=0;
    }

    return;
}

/*
** MUL(x,y)
** This #define creates a macro that computes x=x*y modulo 0x10001.
** Requires temps t16 and t32.  Also requires y to be strictly 16
** bits.  Here, I am using the simplest form.  May not be the
** fastest. -- RG
*/
/* #define MUL(x,y) (x=mul(low16(x),y)) */

/****************
** cipher_idea **
*****************
** IDEA encryption/decryption algorithm.
*/
static void cipher_idea(u16 in[4],
        u16 out[4],
        register IDEAkey Z)
{
    register u16 x1, x2, x3, x4, t1, t2;
    /* register u16 t16;
       register u16 t32; */
    int r=ROUNDS;

    x1=*in++;
    x2=*in++;
    x3=*in++;
    x4=*in;

    do {
        MUL(x1,*Z++);
        x2+=*Z++;
        x3+=*Z++;
        MUL(x4,*Z++);

        t2=x1^x3;
        MUL(t2,*Z++);
        t1=t2+(x2^x4);
        MUL(t1,*Z++);
        t2=t1+t2;

        x1^=t1;
        x4^=t2;

        t2^=x2;
        x2=x3^t1;
        x3=t2;
    } while(--r);
    MUL(x1,*Z++);
    *out++=x1;
    *out++=x3+*Z++;
    *out++=x2+*Z++;
    MUL(x4,*Z);
    *out=x4;
    return;
}
