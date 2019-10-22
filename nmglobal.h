/*
** nmglobal.h
** Global definitions for native mode benchmarks.
**
** BYTEmark (tm)
** BYTE's Native Mode Benchmarks
** Rick Grehan, BYTE Magazine
**
** Creation:
** Revision: 3/95;10/95
**  10/95 - Added memory array & alignment -- RG
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

/* is this a 64 bit architecture? If so, this will define LONG64 */
#ifndef DOS16
#include "pointer.h"
#endif

/* to avoid compiling issue in linux */
#include <stdlib.h>

/*
** SYSTEM DEFINES
*/

/* +++ MEMORY +++ */

/*
** You must define ONLY ONE of the following identifiers
** to specify the mechanism for allocating memory:
** MALLOCMEM
** MACMEM
*/

/*
** Define MALLOCMEM to use the standard malloc() call for
** memory.  This is the default for most systems.
*/
#define MALLOCMEM

/* Define MACMEM to use the Mac's GetPtr call to allocate
** memory (instead of malloc()).
*/
/* #define MACMEM */

/* +++ TIMING +++ */
/*
** You must define ONLY ONE of the following identifiers to pick
** the timing routine used.
**  CLOCKWCPS
**  CLOCKWCT
**  MACTIMEMGR
**  WIN31TIMER
**  CLOCK_GETTIME
*/

/*
** Define CLOCK_GETTIME if you are using the clock_gettime()
** This is the default in most cases and required for multi-thread support.
*/
#define CLOCK_GETTIME

/*
** Define CLOCKWCPS if you are using the clock() routine and the
** constant used as the divisor to determine seconds is
** CLOCKS_PER_SEC.
*/
/* #define CLOCKWCPS */

/*
** Define CLOCKWCT if you are using the clock() routine and the
** constant used as the divisor to determine seconds is CLK_TCK
*/
/* #define CLOCKWCT */

/*
** Define MACTIMEMGR to use the Mac Time manager routines.
** You'll need to be running at least system 6.0.3 or
** better...extended time manager is recommended (system 7 or
** better).
*/
/* #define MACTIMEMGR */

/*
** Define WIN31TIMER to user the timing routines in TOOLHELP.DLL.
** Gets accuracy down to the millisecond.
*/
/* #define WIN31TIMER */

/* +++ MISCELLANEOUS +++ */

/*
** Define DOS16 if you'll be compiling under DOS in 16-bit
** (non DOS-extended) mode.  This will enable proper definitions
** for the far*** typedefs
*/
/* #define DOS16 */

/*
** Define MAC if you're compiling on a Macintosh.  This
** does a number of things:
**  includes unix.h
**  Incorporates code to mimic the command line via either
**      the console library (Symantec/Think) or the SIOUX
**      library (Code Warrior).
*/
/* #define MAC */

/*
** Define LONG64 if your compiler emits 64-bit longs.
** This is typically true of Alpha compilers on Unix
** systems...though, who knows, this may change in the
** future. I MOVED THIS DEFINTION INTO THE FILE pointer.h. DO NOT
** DEFINE IT HERE. IT WILL AUTOMATICALLY BE DEFINED IF NECESSARY.
** Uwe F. Mayer, Dec 15, 1996, Nov 15, 1997
*/
/* #define LONG64 */

/*
** Define MACCWPROF if you are profiling on the Mac using
** Code Warrior.  This enables code that turns off the
** profiler in an evern of an error exit.
*/
/* #define MACCWPROF */

#ifdef MAC
#include <unix.h>
#endif

/*
** ERROR CODES
*/
#define ERROR_MEMORY    1
#define ERROR_MEMARRAY_FULL 2
#define ERROR_MEMARRAY_NFOUND 3
#define ERROR_FILECREATE 10
#define ERROR_FILEREAD 11
#define ERROR_FILEWRITE 12
#define ERROR_FILEOPEN 13
#define ERROR_FILESEEK 14

/*
** MINIMUM_ITERATION_SECONDS
**
** This sets the default value of minimum iteration time.
** It can, of course, be overridden by the input
** command file.
** This ultimately gets loaded into the variable
** global_min_itersec, which specifies the minimum
** time in seconds that must take place between
** a StartStopWatch() and StopStopWatch() call.
** The idea is to reduce error buildup.
*/
#define MINIMUM_ITERATION_SECONDS 0.1

/*
** MINIMUM_SECONDS
**
** Minimum number of seconds to run each test.
*/
#define MINIMUM_SECONDS 5

/*
** MAXPOSLONG
**
** This is the maximum positive long.
*/
#ifdef LONG64
#define MAXPOSLONG 0x7FFFFFFFFFFFFFFFL
#else
#define MAXPOSLONG 0x7FFFFFFFL
#endif

/*
** OTHER DEFINES
*/
#ifndef MAC
#define TRUE    1
#define FALSE   0
#endif

/*
** Memory array size.  Used in SYSSPEC for keeping track
** of re-aligned memory.
*/
#define MEM_ARRAY_SIZE 20

#define CONFIDENCE_LOOPS 3

/*
** TYPEDEFS
*/
#define ulong unsigned long
#define uchar unsigned char
#define uint unsigned int
#define ushort unsigned short

typedef void farvoid;
typedef double fardouble;
typedef long farlong;
typedef unsigned long farulong;
typedef char farchar;
typedef unsigned char faruchar;

/*
** The following typedefs are used when element size
** is critical.  You'll have to alter these for
** your specifical platform/compiler.
*/
typedef unsigned char u8;       /* Unsigned 8-bits */
typedef unsigned short u16;     /* Unsigned 16 bits */
#ifdef LONG64
typedef unsigned int u32;       /* Unsigned 32 bits */
typedef int int32;              /* Signed 32 bit integer */
#else
typedef unsigned long u32;      /* Unsigned 32 bits */
typedef long int32;              /* Signed 32 bit integer */
#endif

/*
** TYPEDEFS
*/
typedef struct {
    double iterations;     /* # of iterations */
    double cpusecs;        /* CPU time used in seconds */
    double realsecs;       /* Real time used in seconds */
} TestResultStruct;

typedef struct {
    int adjust;             /* Set adjust code */
    ulong request_secs;     /* # of seconds requested */
    TestResultStruct result;/* test result */
    ushort numarrays;       /* # of arrays */
    ulong arraysize;        /* # of elements in array */
    ulong loops;            /* Loops per iterations */
    ulong bitoparraysize;           /* Total # of bitfield ops */
    ulong bitfieldarraysize;        /* Bit field array size */
    double cpurate;         /* iteration or operations per second in cpu time */
    double realrate;        /* iteration or operations per second in real time */
    char *errorcontext;     /* Error context string pointer */
} TestControlStruct;

typedef struct {
    TestControlStruct *control; /* point to test control */
    TestResultStruct result;    /* test result to return */
} TestThreadData;

/*****************
** NUMERIC SORT **
*****************/
/*
** DEFINES
*/

/*
** The following constant, NUMNUMARRAYS (no, it is not a
** Peter Sellers joke) is the maximum number of arrays
** that can be built by the numeric sorting benchmark
** before it gives up.  This maximum is dependent on the
** amount of memory in the system.
*/
/*#define NUMNUMARRAYS    1000*/
#define NUMNUMARRAYS    10000

/*
** The following constant NUMARRAYSIZE determines the
** default # of elements in each numeric array.  Ordinarily
** this is something you shouldn't fool with, though as
** with most of the constants here, it is adjustable.
*/
#define NUMARRAYSIZE    8111L

/*
** TYPEDEFS
*/
typedef struct {
        int adjust;             /* Set adjust code */
        ulong request_secs;     /* # of seconds requested */
        double sortspersec;     /* # of sort iterations per sec */
        ushort numarrays;       /* # of arrays */
        ulong arraysize;        /* # of elements in array */
} SortStruct;

/****************
** STRING SORT **
*****************
** Note: The string sort benchmark uses the same structure to
** communicate parameters as does the numeric sort benchmark.
** (i.e., SortStruct...see above.
*/

/*
** DEFINES
*/
/*
** The following constant STRINGARRAYSIZE determines
** the default # of bytes allocated to each string array.
** Though the actual size can be pre-set from the command
** file, this constant should be left unchanged.
*/
#define STRINGARRAYSIZE 8111L

/************************
** BITFIELD OPERATIONS **
*************************
*/

/*
** DEFINES
*/

/*
** Following field sets the size of the bitfield array (in longs).
*/
#if defined(DOS16)
#define BITFARRAYSIZE 16000L   // 16000*4=64000 bytes < 64K
#elif defined(LONG64)
#define BITFARRAYSIZE 16384L
#else
#define BITFARRAYSIZE 32768L
#endif

#define BITOPS_PER_UNIT 1e-6

/*
** TYPEDEFS
*/
typedef struct {
        int adjust;             /* Set adjust code */
        ulong request_secs;     /* # of seconds requested */
        double bitopspersec;    /* # of bitfield ops per sec */
        ulong bitoparraysize;           /* Total # of bitfield ops */
        ulong bitfieldarraysize;        /* Bit field array size */
} BitOpStruct;

/****************************
** EMULATED FLOATING POINT **
****************************/
/*
** DEFINES
*/
#define INTERNAL_FPF_PRECISION 4

/*
** The following constant is the maximum number of loops
** of the emulated floating point test that the system
** will allow before flagging an error.  This is not a
** critical constant, and can be altered if your system is
** a real barn-burner.
*/
/*#define CPUEMFLOATLOOPMAX 50000L*/
#define CPUEMFLOATLOOPMAX 500000L

/*
** Set size of array
*/
#define EMFARRAYSIZE 3000L

/*
** TYPEDEFS
*/
typedef struct {
        int adjust;             /* Set adjust code */
        ulong request_secs;     /* # of seconds requested */
        ulong arraysize;        /* Size of array */
        ulong loops;            /* Loops per iterations */
        double emflops;         /* Results */
} EmFloatStruct;

/*************************
** FOURIER COEFFICIENTS **
*************************/

/*
** TYPEDEFS
*/
typedef struct {
        int adjust;             /* Set adjust code */
        ulong request_secs;     /* # of requested seconds */
        ulong arraysize;        /* Size of coeff. arrays */
        double fflops;          /* Results */
} FourierStruct;

/*************************
** ASSIGNMENT ALGORITHM **
*************************/

#define FLOPS_PER_UNIT 1e-3

/*
** TYPEDEFS
*/
typedef struct {
        int adjust;             /* Set adjust code */
        ulong request_secs;     /* Requested # of seconds */
        ulong numarrays;        /* # of arrays */
        double iterspersec;     /* Results */
} AssignStruct;

/********************
** IDEA ENCRYPTION **
********************/

/*
** DEFINES
*/
/* Following constant defines the max number of loops the
** system will attempt. Keeps things from going off into the
** weeds. */
/*#define MAXIDEALOOPS 50000L*/
#define MAXIDEALOOPS 500000L

/*
** Following constant sets the size of the arrays.
** NOTE: For the IDEA algorithm to work properly, this
**  number MUST be some multiple of 8.
*/
#define IDEAARRAYSIZE 4000L

/*
** TYPEDEFS
*/
typedef struct {
        int adjust;             /* Set adjust code */
        ulong request_secs;     /* Requested # of seconds */
        ulong arraysize;        /* Size of array */
        ulong loops;            /* # of times to convert */
        double iterspersec;     /* Results */
} IDEAStruct;


/************************
** HUFFMAN COMPRESSION **
************************/

/*
** DEFINES
*/
/*
** MAXHUFFLOOPS
**
** This constant specifies the maximum number of Huffman
** compression loops the system will try for.  This keeps
** the test from going off into the weeds.  This is not
** a critical constant, and can be increased if your
** system is a real barn-burner.
*/
/*#define MAXHUFFLOOPS 50000L*/
#define MAXHUFFLOOPS 500000L

/*
** Following constant sets the size of the arrays to
** be compressed/uncompressed.
*/
#define HUFFARRAYSIZE 5000L

/*
** TYPEDEFS
*/

typedef struct {
        int adjust;             /* Set adjust code */
        ulong request_secs;     /* Requested # of seconds */
        ulong arraysize;        /* Size of array */
        ulong loops;            /* # of times to compress/decompress */
        double iterspersec;     /* Results */
} HuffStruct;

/********************************
** BACK PROPAGATION NEURAL NET **
********************************/

/*
**  MAXNNETLOOPS
**
** This constant sets the max number of loops through the neural
** net that the system will attempt before giving up.  This
** is not a critical constant.  You can alter it if your system
** has sufficient horsepower.
*/
/*#define MAXNNETLOOPS  50000L*/
#define MAXNNETLOOPS  500000L

/*
** TYPEDEFS
*/
typedef struct {
        int adjust;             /* Set adjust code */
        ulong request_secs;     /* Requested # of seconds */
        ulong loops;            /* # of times to learn */
        double iterspersec;     /* Results */
} NNetStruct;

/***********************
**  LU DECOMPOSITION  **
** (Linear Equations) **
***********************/

/*
** MAXLUARRAYS
**
** This sets the upper limit on the number of arrays
** that the benchmark will attempt to build before
** flagging an error.  It is not a critical constant, and
** may be increased if your system has the horsepower.
*/
/*#define MAXLUARRAYS 1000*/
#define MAXLUARRAYS 10000

/*
** TYPEDEFS
*/
typedef struct {
        int adjust;             /* Set adjust code */
        ulong request_secs;     /* Requested # of seconds */
        ulong numarrays;        /* # of arrays */
        double iterspersec;     /* Results */
} LUStruct;

/*
** EXTERNALS
*/
extern float global_min_itersec;
extern int global_concurrency;        /* Number of concurrent test threads */

extern TestControlStruct global_numsortstruct;
extern TestControlStruct global_strsortstruct;
extern TestControlStruct global_bitopstruct;
extern TestControlStruct global_emfloatstruct;
extern TestControlStruct global_fourierstruct;
extern TestControlStruct global_assignstruct;
extern TestControlStruct global_ideastruct;
extern TestControlStruct global_huffstruct;
extern TestControlStruct global_nnetstruct;
extern TestControlStruct global_lustruct;

