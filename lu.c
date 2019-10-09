/*
** lu.c
*/

/********************************
**       BYTEmark (tm)         **
** BYTE NATIVE MODE BENCHMARKS **
**       VERSION 2             **
**                             **
** Included in this source     **
** file:                       **
**  Numeric Heapsort           **
**  String Heapsort            **
**  Bitfield test              **
**  Floating point emulation   **
**  Fourier coefficients       **
**  Assignment algorithm       **
**  IDEA Encyption             **
**  Huffman compression        **
**  Back prop. neural net      **
**  LU Decomposition           **
**    (linear equations)       **
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


/***********************
**  LU DECOMPOSITION  **
** (Linear Equations) **
************************
** These routines come from "Numerical Recipes in Pascal".
** Note that, as in the assignment algorithm, though we
** separately define LUARRAYROWS and LUARRAYCOLS, the two
** must be the same value (this routine depends on a square
** matrix).
*/


/*
** DEFINES
*/

#define LUARRAYROWS 101L
#define LUARRAYCOLS 101L

/*
** TYPEDEFS
*/
typedef struct
{       union
	{       fardouble *p;
		fardouble (*ap)[][LUARRAYCOLS];
	} ptrs;
} LUdblptr;

/*
** GLOBALS
*/
fardouble *LUtempvv;

/*
** PROTOTYPES
*/
void DoLU(void);
static void LUFreeMem(fardouble *a, fardouble *b,
	fardouble *abase, fardouble *bbase);
static ulong DoLUIteration(fardouble *a, fardouble *b,
	fardouble *abase, fardouble *bbase,
	ulong numarrays);
static void build_problem( double a[][LUARRAYCOLS],
	int n, double b[LUARRAYROWS]);
static int ludcmp(double a[][LUARRAYCOLS],
	int n, int indx[], int *d);
static void lubksb(double a[][LUARRAYCOLS],
	int n, int indx[LUARRAYROWS],
	double b[LUARRAYROWS]);
static int lusolve(double a[][LUARRAYCOLS],
	int n, double b[LUARRAYROWS]);

/*********
** DoLU **
**********
** Perform the LU decomposition benchmark.
*/
void DoLU(void)
{
    LUStruct *loclustruct;  /* Local pointer to global data */
    char *errorcontext;
    int systemerror;
    fardouble *a;
    fardouble *b;
    fardouble *abase;
    fardouble *bbase;
    LUdblptr ptra;
    int n;
    int i;
    ulong accumtime;
    double iterations;

    /*
     ** Link to global data
     */
    loclustruct=&global_lustruct;

    /*
     ** Set error context.
     */
    errorcontext="FPU:LU";

    /*
     ** Our first step is to build a "solvable" problem.  This
     ** will become the "seed" set that all others will be
     ** derived from. (I.E., we'll simply copy these arrays
     ** into the others.
     */
    a=(fardouble *)AllocateMemory(sizeof(double) * LUARRAYCOLS * LUARRAYROWS,
            &systemerror);
    b=(fardouble *)AllocateMemory(sizeof(double) * LUARRAYROWS,
            &systemerror);
    n=LUARRAYROWS;

    /*
     ** We need to allocate a temp vector that is used by the LU
     ** algorithm.  This removes the allocation routine from the
     ** timing.
     */
    LUtempvv=(fardouble *)AllocateMemory(sizeof(double)*LUARRAYROWS,
            &systemerror);

    /*
     ** Build a problem to be solved.
     */
    ptra.ptrs.p=a;                  /* Gotta coerce linear array to 2D array */
    build_problem(*ptra.ptrs.ap,n,b);

    /*
     ** Now that we have a problem built, see if we need to do
     ** auto-adjust.  If so, repeatedly call the DoLUIteration routine,
     ** increasing the number of solutions per iteration as you go.
     */
    if(loclustruct->adjust==0)
    {
        loclustruct->numarrays=0;
        for(i=1;i<=MAXLUARRAYS;i++)
        {
            abase=(fardouble *)AllocateMemory(sizeof(double) *
                    LUARRAYCOLS*LUARRAYROWS*(i+1),&systemerror);
            if(systemerror)
            {       ReportError(errorcontext,systemerror);
                LUFreeMem(a,b,(fardouble *)NULL,(fardouble *)NULL);
                ErrorExit();
            }
            bbase=(fardouble *)AllocateMemory(sizeof(double) *
                    LUARRAYROWS*(i+1),&systemerror);
            if(systemerror)
            {       ReportError(errorcontext,systemerror);
                LUFreeMem(a,b,abase,(fardouble *)NULL);
                ErrorExit();
            }
            if(DoLUIteration(a,b,abase,bbase,i)>global_min_ticks)
            {       loclustruct->numarrays=i;
                break;
            }
            /*
             ** Not enough arrays...free them all and try again
             */
            FreeMemory((farvoid *)abase,&systemerror);
            FreeMemory((farvoid *)bbase,&systemerror);
        }
        /*
         ** Were we able to do it?
         */
        if(loclustruct->numarrays==0)
        {       printf("FPU:LU -- Array limit reached\n");
            LUFreeMem(a,b,abase,bbase);
            ErrorExit();
        }
    }
    else
    {       /*
             ** Don't need to adjust -- just allocate the proper
             ** number of arrays and proceed.
             */
        abase=(fardouble *)AllocateMemory(sizeof(double) *
                LUARRAYCOLS*LUARRAYROWS*loclustruct->numarrays,
                &systemerror);
        if(systemerror)
        {       ReportError(errorcontext,systemerror);
            LUFreeMem(a,b,(fardouble *)NULL,(fardouble *)NULL);
            ErrorExit();
        }
        bbase=(fardouble *)AllocateMemory(sizeof(double) *
                LUARRAYROWS*loclustruct->numarrays,&systemerror);
        if(systemerror)
        {
            ReportError(errorcontext,systemerror);
            LUFreeMem(a,b,abase,(fardouble *)NULL);
            ErrorExit();
        }
    }
    /*
     ** All's well if we get here.  Do the test.
     */
    accumtime=0L;
    iterations=(double)0.0;

    do {
        accumtime+=DoLUIteration(a,b,abase,bbase,
                loclustruct->numarrays);
        iterations+=(double)loclustruct->numarrays;
    } while(TicksToSecs(accumtime)<loclustruct->request_secs);

    /*
     ** Clean up, calculate results, and go home.  Be sure to
     ** show that we don't have to rerun adjustment code.
     */
    loclustruct->iterspersec=iterations / TicksToFracSecs(accumtime);

    if(loclustruct->adjust==0)
        loclustruct->adjust=1;

    LUFreeMem(a,b,abase,bbase);
    return;
}

/**************
** LUFreeMem **
***************
** Release memory associated with LU benchmark.
*/
static void LUFreeMem(fardouble *a, fardouble *b,
            fardouble *abase,fardouble *bbase)
{
    int systemerror;

    FreeMemory((farvoid *)a,&systemerror);
    FreeMemory((farvoid *)b,&systemerror);
    FreeMemory((farvoid *)LUtempvv,&systemerror);

    if(abase!=(fardouble *)NULL) FreeMemory((farvoid *)abase,&systemerror);
    if(bbase!=(fardouble *)NULL) FreeMemory((farvoid *)bbase,&systemerror);
    return;
}

/******************
** DoLUIteration **
*******************
** Perform an iteration of the LU decomposition benchmark.
** An iteration refers to the repeated solution of several
** identical matrices.
*/
static ulong DoLUIteration(fardouble *a,fardouble *b,
        fardouble *abase, fardouble *bbase,
        ulong numarrays)
{
    fardouble *locabase;
    fardouble *locbbase;
    LUdblptr ptra;  /* For converting ptr to 2D array */
    ulong elapsed;
    ulong j,i;              /* Indexes */


    /*
     ** Move the seed arrays (a & b) into the destination
     ** arrays;
     */
    for(j=0;j<numarrays;j++)
    {       locabase=abase+j*LUARRAYROWS*LUARRAYCOLS;
        locbbase=bbase+j*LUARRAYROWS;
        for(i=0;i<LUARRAYROWS*LUARRAYCOLS;i++)
            *(locabase+i)=*(a+i);
        for(i=0;i<LUARRAYROWS;i++)
            *(locbbase+i)=*(b+i);
    }

    /*
     ** Do test...begin timing.
     */
    elapsed=StartStopwatch();
    for(i=0;i<numarrays;i++)
    {       locabase=abase+i*LUARRAYROWS*LUARRAYCOLS;
        locbbase=bbase+i*LUARRAYROWS;
        ptra.ptrs.p=locabase;
        lusolve(*ptra.ptrs.ap,LUARRAYROWS,locbbase);
    }

    return(StopStopwatch(elapsed));
}

/******************
** build_problem **
*******************
** Constructs a solvable set of linear equations.  It does this by
** creating an identity matrix, then loading the solution vector
** with random numbers.  After that, the identity matrix and
** solution vector are randomly "scrambled".  Scrambling is
** done by (a) randomly selecting a row and multiplying that
** row by a random number and (b) adding one randomly-selected
** row to another.
*/
static void build_problem(double a[][LUARRAYCOLS],
        int n,
        double b[LUARRAYROWS])
{
    long i,j,k,k1;  /* Indexes */
    double rcon;     /* Random constant */

    /*
     ** Reset random number generator
     */
    /* randnum(13L); */
    randnum((int32)13);

    /*
     ** Build an identity matrix.
     ** We'll also use this as a chance to load the solution
     ** vector.
     */
    for(i=0;i<n;i++)
    {       /* b[i]=(double)(abs_randwc(100L)+1L); */
        b[i]=(double)(abs_randwc((int32)100)+(int32)1);
        for(j=0;j<n;j++)
            if(i==j)
                /* a[i][j]=(double)(abs_randwc(1000L)+1L); */
                a[i][j]=(double)(abs_randwc((int32)1000)+(int32)1);
            else
                a[i][j]=(double)0.0;
    }

#ifdef DEBUG
    printf("Problem:\n");
    for(i=0;i<n;i++)
    {
        /*
           for(j=0;j<n;j++)
           printf("%6.2f ",a[i][j]);
         */
        printf("%.0f/%.0f=%.2f\t",b[i],a[i][i],b[i]/a[i][i]);
        /*
           printf("\n");
         */
    }
#endif

    /*
     ** Scramble.  Do this 8n times.  See comment above for
     ** a description of the scrambling process.
     */

    for(i=0;i<8*n;i++)
    {
        /*
         ** Pick a row and a random constant.  Multiply
         ** all elements in the row by the constant.
         */
        /*       k=abs_randwc((long)n);
                 rcon=(double)(abs_randwc(20L)+1L);
                 for(j=0;j<n;j++)
                 a[k][j]=a[k][j]*rcon;
                 b[k]=b[k]*rcon;
         */
        /*
         ** Pick two random rows and add second to
         ** first.  Note that we also occasionally multiply
         ** by minus 1 so that we get a subtraction operation.
         */
        /* k=abs_randwc((long)n); */
        /* k1=abs_randwc((long)n); */
        k=abs_randwc((int32)n);
        k1=abs_randwc((int32)n);
        if(k!=k1)
        {
            if(k<k1) rcon=(double)1.0;
            else rcon=(double)-1.0;
            for(j=0;j<n;j++)
                a[k][j]+=a[k1][j]*rcon;;
            b[k]+=b[k1]*rcon;
        }
    }

    return;
}


/***********
** ludcmp **
************
** From the procedure of the same name in "Numerical Recipes in Pascal",
** by Press, Flannery, Tukolsky, and Vetterling.
** Given an nxn matrix a[], this routine replaces it by the LU
** decomposition of a rowwise permutation of itself.  a[] and n
** are input.  a[] is output, modified as follows:
**   --                       --
**  |  b(1,1) b(1,2) b(1,3)...  |
**  |  a(2,1) b(2,2) b(2,3)...  |
**  |  a(3,1) a(3,2) b(3,3)...  |
**  |  a(4,1) a(4,2) a(4,3)...  |
**  |  ...                      |
**   --                        --
**
** Where the b(i,j) elements form the upper triangular matrix of the
** LU decomposition, and the a(i,j) elements form the lower triangular
** elements.  The LU decomposition is calculated so that we don't
** need to store the a(i,i) elements (which would have laid along the
** diagonal and would have all been 1).
**
** indx[] is an output vector that records the row permutation
** effected by the partial pivoting; d is output as +/-1 depending
** on whether the number of row interchanges was even or odd,
** respectively.
** Returns 0 if matrix singular, else returns 1.
*/
static int ludcmp(double a[][LUARRAYCOLS],
        int n,
        int indx[],
        int *d)
{

    double big;     /* Holds largest element value */
    double sum;
    double dum;     /* Holds dummy value */
    int i,j,k;      /* Indexes */
    int imax=0;     /* Holds max index value */
    double tiny;    /* A really small number */

    tiny=(double)1.0e-20;

    *d=1;           /* No interchanges yet */

    for(i=0;i<n;i++)
    {       big=(double)0.0;
        for(j=0;j<n;j++)
            if((double)fabs(a[i][j]) > big)
                big=fabs(a[i][j]);
        /* Bail out on singular matrix */
        if(big==(double)0.0) return(0);
        LUtempvv[i]=1.0/big;
    }

    /*
     ** Crout's algorithm...loop over columns.
     */
    for(j=0;j<n;j++)
    {       if(j!=0)
        for(i=0;i<j;i++)
        {       sum=a[i][j];
            if(i!=0)
                for(k=0;k<i;k++)
                    sum-=(a[i][k]*a[k][j]);
            a[i][j]=sum;
        }
        big=(double)0.0;
        for(i=j;i<n;i++)
        {       sum=a[i][j];
            if(j!=0)
                for(k=0;k<j;k++)
                    sum-=a[i][k]*a[k][j];
            a[i][j]=sum;
            dum=LUtempvv[i]*fabs(sum);
            if(dum>=big)
            {       big=dum;
                imax=i;
            }
        }
        if(j!=imax)             /* Interchange rows if necessary */
        {       for(k=0;k<n;k++)
            {       dum=a[imax][k];
                a[imax][k]=a[j][k];
                a[j][k]=dum;
            }
            *d=-*d;         /* Change parity of d */
            dum=LUtempvv[imax];
            LUtempvv[imax]=LUtempvv[j]; /* Don't forget scale factor */
            LUtempvv[j]=dum;
        }
        indx[j]=imax;
        /*
         ** If the pivot element is zero, the matrix is singular
         ** (at least as far as the precision of the machine
         ** is concerned.)  We'll take the original author's
         ** recommendation and replace 0.0 with "tiny".
         */
        if(a[j][j]==(double)0.0)
            a[j][j]=tiny;

        if(j!=(n-1))
        {       dum=1.0/a[j][j];
            for(i=j+1;i<n;i++)
                a[i][j]=a[i][j]*dum;
        }
    }

    return(1);
}

/***********
** lubksb **
************
** Also from "Numerical Recipes in Pascal".
** This routine solves the set of n linear equations A X = B.
** Here, a[][] is input, not as the matrix A, but as its
** LU decomposition, created by the routine ludcmp().
** Indx[] is input as the permutation vector returned by ludcmp().
**  b[] is input as the right-hand side an returns the
** solution vector X.
** a[], n, and indx are not modified by this routine and
** can be left in place for different values of b[].
** This routine takes into account the possibility that b will
** begin with many zero elements, so it is efficient for use in
** matrix inversion.
*/
static void lubksb( double a[][LUARRAYCOLS],
        int n,
        int indx[LUARRAYROWS],
        double b[LUARRAYROWS])
{

    int i,j;        /* Indexes */
    int ip;         /* "pointer" into indx */
    int ii;
    double sum;

    /*
     ** When ii is set to a positive value, it will become
     ** the index of the first nonvanishing element of b[].
     ** We now do the forward substitution. The only wrinkle
     ** is to unscramble the permutation as we go.
     */
    ii=-1;
    for(i=0;i<n;i++)
    {       ip=indx[i];
        sum=b[ip];
        b[ip]=b[i];
        if(ii!=-1)
            for(j=ii;j<i;j++)
                sum=sum-a[i][j]*b[j];
        else
            /*
             ** If a nonzero element is encountered, we have
             ** to do the sums in the loop above.
             */
            if(sum!=(double)0.0)
                ii=i;
        b[i]=sum;
    }
    /*
     ** Do backsubstitution
     */
    for(i=(n-1);i>=0;i--)
    {
        sum=b[i];
        if(i!=(n-1))
            for(j=(i+1);j<n;j++)
                sum=sum-a[i][j]*b[j];
        b[i]=sum/a[i][i];
    }
    return;
}

/************
** lusolve **
*************
** Solve a linear set of equations: A x = b
** Original matrix A will be destroyed by this operation.
** Returns 0 if matrix is singular, 1 otherwise.
*/
static int lusolve(double a[][LUARRAYCOLS],
        int n,
        double b[LUARRAYROWS])
{
    int indx[LUARRAYROWS];
    int d;
#ifdef DEBUG
    int i,j;
#endif

    if(ludcmp(a,n,indx,&d)==0) return(0);

    /* Matrix not singular -- proceed */
    lubksb(a,n,indx,b);

#ifdef DEBUG
    printf("Solution:\n");
    for(i=0;i<n;i++)
    {
        for(j=0;j<n;j++){
            /*
               printf("%6.2f ",a[i][j]);
             */
        }
        printf("%6.2f\t",b[i]);
        /*
           printf("\n");
         */
    }
    printf("\n");
#endif

    return(1);
}
