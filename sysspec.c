
/*
** sysspec.c
** System-specific routines.
**
** BYTEmark (tm)
** BYTE's Native Mode Benchmarks
** Rick Grehan, BYTE Magazine
**
** Creation:
** Revision: 3/95;10/95
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

/***********************************
**    SYSTEM-SPECIFIC ROUTINES    **
************************************
**
** These are the routines that provide functions that are
** system-specific.  If the benchmarks are to be ported
** to new hardware/new O.S., this is the first place to
** start.
*/
#include "nmglobal.h"
#include "sysspec.h"

#ifdef DOS16
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#endif

#ifdef USE_PTHREAD
#include <pthread.h>
#endif

/*
** Global parameters.
*/
int global_align;		/* Memory alignment */
#ifdef CLOCK_GETTIME
int global_realtime_cid = CLOCK_MONOTONIC;  /* Clock ID used in clock_gettime */
#endif

#ifdef MACTIMEMGR
#include <Types.h>
#include <Timer.h>
/*
** Timer globals for Mac
*/
struct TMTask myTMTask;
long MacHSTdelay,MacHSTohead;

#endif

/*
** Windows 3.1 timer defines
*/
#ifdef WIN31TIMER
#include <windows.h>
#include <toolhelp.h>
TIMERINFO win31tinfo;
HANDLE hThlp;
FARPROC lpfn;
#endif

/*
** Following global is the memory array.  This is used to store
** original and aligned (modified) memory addresses.
*/
ulong mem_array[2][MEM_ARRAY_SIZE];
int mem_array_ents;		/* # of active entries */
#ifdef USE_PTHREAD
pthread_mutex_t master_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/*********************************
**  MEMORY MANAGEMENT ROUTINES  **
*********************************/

/****************************
** AllocateMemory
** This routine returns a void pointer to a memory
** block.  The size of the memory block is given in bytes
** as the first argument.  This routine also returns an
** error code in the second argument.
** 10/95 Update:
**  Added an associative array for memory alignment reasons.
**  mem_array[2][MEM_ARRAY_SIZE]
**   mem_array[0][n] = Actual address (from malloc)
**   mem_array[1][n] = Aligned address
** Currently, mem_array[][] is only used if you use malloc;
**  it is not used for the MAC versions.
*/
void *AllocateMemory(unsigned long nbytes,   /* # of bytes to alloc */
		int *errorcode)                 /* Returned error code */
{
#ifdef MACMEM
    /*
     ** For MAC CodeWarrior, we'll use the MacOS NewPtr call
     */
    void *returnval;
    returnval=(void *)NewPtr((Size)nbytes);
    if(returnval==(void *)NULL)
        *errorcode=ERROR_MEMORY;
    else
        *errorcode=0;
    return(returnval);
#endif

#ifdef MALLOCMEM
    /*
     ** Everyone else, its pretty straightforward, given
     ** that you use a 32-bit compiler which treats size_t as
     ** a 4-byte entity.
     */
    void *returnval;             /* Return value */
    ulong true_addr;		/* True address */
    ulong adj_addr;			/* Adjusted address */

    returnval=(void *)malloc((size_t)(nbytes+2L*(long)global_align));
    if(returnval==(void *)NULL)
        *errorcode=ERROR_MEMORY;
    else
        *errorcode=0;

    /*
     ** Check for alignment
     */
    adj_addr=true_addr=(ulong)returnval;
    if(global_align==0)
    {
        if(AddMemArray(true_addr, adj_addr))
            *errorcode=ERROR_MEMARRAY_FULL;
        return(returnval);
    }

    if(global_align==1)
    {
        if(true_addr%2==0) adj_addr++;
    }
    else
    {
        while(adj_addr%global_align!=0) ++adj_addr;
        if(adj_addr%(global_align*2)==0) adj_addr+=global_align;
    }
    returnval=(void *)adj_addr;
    if(AddMemArray(true_addr,adj_addr))
        *errorcode=ERROR_MEMARRAY_FULL;
    return(returnval);
#endif

}


/****************************
** FreeMemory
** This is the reverse of AllocateMemory.  The memory
** block passed in is freed.  Should an error occur,
** that error is returned in errorcode.
*/
void FreeMemory(void *mempointer,    /* Pointer to memory block */
		int *errorcode)
{

#ifdef MACMEM
    DisposPtr((Ptr)mempointer);
    *errorcode=0;
    return;
#endif

#ifdef MALLOCMEM
    ulong adj_addr, true_addr;

    /* Locate item in memory array */
    adj_addr=(ulong)mempointer;
    if(RemoveMemArray(adj_addr, &true_addr))
    {
        *errorcode=ERROR_MEMARRAY_NFOUND;
        return;
    }
    mempointer=(void *)true_addr;
    free(mempointer);
    *errorcode=0;
    return;
#endif
}

/****************************
** MoveMemory
** Moves n bytes from a to b.  Handles overlap.
** In most cases, this is just a memmove operation.
** But, not in DOS....noooo....
*/
void MoveMemory( void *destination,  /* Destination address */
		void *source,        /* Source address */
		unsigned long nbytes)
{

    memmove(destination, source, nbytes);
}

/***********************************
** MEMORY ARRAY HANDLING ROUTINES **
***********************************/
/****************************
** InitMemArray
** Initialize the memory array.  This simply amounts to
** setting mem_array_ents to zero, indicating that there
** isn't anything in the memory array.
*/
void InitMemArray(void)
{
    mem_array_ents=0;
    return;
}

/***************************
** AddMemArray
** Add a pair of items to the memory array.
**  true_addr is the true address (mem_array[0][n])
**  adj_addr is the adjusted address (mem_array[0][n])
** Returns 0 if ok
** -1 if not enough room
*/
int AddMemArray(ulong true_addr,
		ulong adj_addr)
{
    int err = -1;

#ifdef USE_PTHREAD
    pthread_mutex_lock(&master_lock);
#endif

    if(mem_array_ents<MEM_ARRAY_SIZE) {
        mem_array[0][mem_array_ents]=true_addr;
        mem_array[1][mem_array_ents]=adj_addr;
        mem_array_ents++;
        err = 0;
    }

#ifdef USE_PTHREAD
    pthread_mutex_unlock(&master_lock);
#endif

    return(err);
}

/*************************
** RemoveMemArray
** Given an adjusted address value (mem_array[1][n]), locate
** the entry and remove it from the mem_array.
** Also returns the associated true address.
** Returns 0 if ok
** -1 if not found.
*/
int RemoveMemArray(ulong adj_addr,ulong *true_addr)
{
    int i,j;
    int err = -1;

#ifdef USE_PTHREAD
    pthread_mutex_lock(&master_lock);
#endif

    /* Locate the item in the array. */
    for(i=0;i<mem_array_ents;i++)
        if(mem_array[1][i]==adj_addr)
        {       /* Found it..bubble stuff down */
            *true_addr=mem_array[0][i];
            j=i;
            while(j+1<mem_array_ents)
            {       mem_array[0][j]=mem_array[0][j+1];
                mem_array[1][j]=mem_array[1][j+1];
                j++;
            }
            mem_array_ents--;
            err = 0;
            break;  /* break if found */
        }

#ifdef USE_PTHREAD
    pthread_mutex_unlock(&master_lock);
#endif

    return(err);
}

/**********************************
**    FILE HANDLING ROUTINES     **
**********************************/

/****************************
** CreateFile
** This routine accepts a filename for an argument and
** creates that file in the current directory (unless the
** name contains a path that overrides the current directory).
** Note that the routine does not OPEN the file.
** If the file exists, it is truncated to length 0.
*/
void CreateFile(char *filename,
		int *errorcode)
{

#ifdef DOS16
    /*
     ** DOS VERSION!!
     */
    int fhandle;            /* File handle used internally */

    fhandle=open(filename,O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);

    if(fhandle==-1)
        *errorcode=ERROR_FILECREATE;
    else
        *errorcode=0;

    /*
     ** Since all we're doing here is creating the file,
     ** go ahead and close it.
     */
    close(fhandle);

    return;
#endif

#ifdef LINUX
    FILE *fhandle;            /* File handle used internally */

    fhandle=fopen(filename,"w");

    if(fhandle==NULL)
        *errorcode=ERROR_FILECREATE;
    else
        *errorcode=0;

    /*
     ** Since all we're doing here is creating the file,
     ** go ahead and close it.
     */
    fclose(fhandle);

    return;
#endif
}

/****************************
** bmOpenFile
** Opens the file given by fname, returning its handle.
** If an error occurs, returns its code in errorcode.
** The file is opened in read-write exclusive mode.
*/
#ifdef DOS16
/*
** DOS VERSION!!
*/

int bmOpenFile(char *fname,       /* File name */
	int *errorcode)         /* Error code returned */
{

    int fhandle;            /* Returned file handle */

    fhandle=open(fname,O_BINARY | O_RDWR, S_IREAD | S_IWRITE);

    if(fhandle==-1)
        *errorcode=ERROR_FILEOPEN;
    else
        *errorcode=0;

    return(fhandle);
}
#endif


#ifdef LINUX

FILE *bmOpenFile(char *fname,       /* File name */
	    int *errorcode)         /* Error code returned */
{

    FILE *fhandle;            /* Returned file handle */

    fhandle=fopen(fname,"w+");

    if(fhandle==NULL)
        *errorcode=ERROR_FILEOPEN;
    else
        *errorcode=0;

    return(fhandle);
}
#endif


/****************************
** CloseFile
** Closes the file identified by fhandle.
** A more inocuous routine there never was.
*/
#ifdef DOS16
/*
** DOS VERSION!!!
*/
void CloseFile(int fhandle,             /* File handle */
		int *errorcode)         /* Returned error code */
{

    close(fhandle);
    *errorcode=0;
    return;
}
#endif
#ifdef LINUX
void CloseFile(FILE *fhandle,             /* File handle */
		int *errorcode)         /* Returned error code */
{
    fclose(fhandle);
    *errorcode=0;
    return;
}
#endif

/****************************
** readfile
** Read bytes from an opened file.  This routine
** is a combination seek-and-read.
** Note that this routine expects the offset to be from
** the beginning of the file.
*/
#ifdef DOS16
/*
** DOS VERSION!!
*/

void readfile(int fhandle,              /* File handle */
	unsigned long offset,           /* Offset into file */
	unsigned long nbytes,           /* # of bytes to read */
	void *buffer,                   /* Buffer to read into */
	int *errorcode)                 /* Returned error code */
{

    long newoffset;                         /* New offset by lseek */
    int readcode;                           /* Return code from read */

    /*
     ** Presume success.
     */
    *errorcode=0;

    /*
     ** Seek to the proper offset.
     */
    newoffset=lseek(fhandle,(long)offset,SEEK_SET);
    if(newoffset==-1L)
    {       *errorcode=ERROR_FILESEEK;
        return;
    }

    /*
     ** Do the read.
     */
    readcode=read(fhandle,buffer,(unsigned)(nbytes & 0xFFFF));
    if(readcode==-1)
        *errorcode=ERROR_FILEREAD;

    return;
}
#endif
#ifdef LINUX
void readfile(FILE *fhandle,            /* File handle */
	unsigned long offset,           /* Offset into file */
	unsigned long nbytes,           /* # of bytes to read */
	void *buffer,                   /* Buffer to read into */
	int *errorcode)                 /* Returned error code */
{

    long newoffset;                         /* New offset by fseek */
    size_t nelems;                          /* Expected return code from read */
    size_t readcode;                        /* Actual return code from read */

    /*
     ** Presume success.
     */
    *errorcode=0;

    /*
     ** Seek to the proper offset.
     */
    newoffset=fseek(fhandle,(long)offset,SEEK_SET);
    if(newoffset==-1L)
    {       *errorcode=ERROR_FILESEEK;
        return;
    }

    /*
     ** Do the read.
     */
    nelems=(size_t)(nbytes & 0xFFFF);
    readcode=fread(buffer,(size_t)1,nelems,fhandle);
    if(readcode!=nelems)
        *errorcode=ERROR_FILEREAD;

    return;
}
#endif

/****************************
** writefile
** writes bytes to an opened file.  This routine is
** a combination seek-and-write.
** Note that this routine expects the offset to be from
** the beinning of the file.
*/
#ifdef DOS16
/*
** DOS VERSION!!
*/

void writefile(int fhandle,             /* File handle */
	unsigned long offset,           /* Offset into file */
	unsigned long nbytes,           /* # of bytes to read */
	void *buffer,                   /* Buffer to read into */
	int *errorcode)                 /* Returned error code */
{

    long newoffset;                         /* New offset by lseek */
    int writecode;                          /* Return code from write */

    /*
     ** Presume success.
     */
    *errorcode=0;

    /*
     ** Seek to the proper offset.
     */
    newoffset=lseek(fhandle,(long)offset,SEEK_SET);
    if(newoffset==-1L)
    {       *errorcode=ERROR_FILESEEK;
        return;
    }

    /*
     ** Do the write.
     */
    writecode=write(fhandle,buffer,(unsigned)(nbytes & 0xFFFF));
    if(writecode==-1)
        *errorcode=ERROR_FILEWRITE;

    return;
}
#endif

#ifdef LINUX

void writefile(FILE *fhandle,           /* File handle */
	unsigned long offset,           /* Offset into file */
	unsigned long nbytes,           /* # of bytes to read */
	void *buffer,                   /* Buffer to read into */
	int *errorcode)                 /* Returned error code */
{

    long newoffset;                         /* New offset by lseek */
    size_t nelems;                          /* Expected return code from write */
    size_t writecode;                       /* Actual return code from write */

    /*
     ** Presume success.
     */
    *errorcode=0;

    /*
     ** Seek to the proper offset.
     */
    newoffset=fseek(fhandle,(long)offset,SEEK_SET);
    if(newoffset==-1L)
    {       *errorcode=ERROR_FILESEEK;
        return;
    }

    /*
     ** Do the write.
     */
    nelems=(size_t)(nbytes & 0xFFFF);
    writecode=fwrite(buffer,(size_t)1,nelems,fhandle);
    if(writecode==nelems)
        *errorcode=ERROR_FILEWRITE;

    return;
}
#endif


/********************************
**   ERROR HANDLING ROUTINES   **
********************************/

/****************************
** ReportError
** Report error message condition.
*/
void ReportError(char *errorcontext,    /* Error context string */
		int errorcode)          /* Error code number */
{

    /*
     ** Display error context
     */
    printf("ERROR CONDITION\nContext: %s\n",errorcontext);

    /*
     ** Display code
     */
    printf("Code: %d",errorcode);

    return;
}

/****************************
** ErrorExit
** Peforms an exit from an error condition.
*/
void ErrorExit()
{

    /*
     ** For profiling on the Mac with MetroWerks -- 11/17/94 RG
     ** Have to do this to turn off profiler.
     */
#ifdef MACCWPROF
#if __profile__
    ProfilerTerm();
#endif
#endif

    /*
     ** FOR NOW...SIMPLE EXIT
     */
    exit(1);
}

/*****************************
**    STOPWATCH ROUTINES    **
*****************************/

/**********************************************
** InitStopWatch
** This function to be called when upon startup
** When CLOCK_GETTIME is enabled, it will
** a) choose realtime clock between CLOCK_MONOTONIC(preferred)
**    or CLOCK_REALTIME
** b) determine minimum iteration time based on clock resolution
**    aim to reduce clock rounding error
*/
void InitStopWatch()
{
#ifdef CLOCK_GETTIME
    int err;
    struct timespec ts;
    float timeres;

    global_realtime_cid = CLOCK_MONOTONIC;
    err = clock_getres(global_realtime_cid, &ts);
    if (err) {
        // if CLOCK_MONOTONIC is not supported, fall back to CLOCK_REALTIME
        global_realtime_cid = CLOCK_REALTIME;
        err = clock_getres(global_realtime_cid, &ts);
    }

    if (!err) {
        /*
      .  * determine minimum iteration time to be 100x clock resolution but cap to 1s
        */
        timeres = ts.tv_sec + ts.tv_nsec * 1e-9;
        global_min_itersec = timeres * 100.0;
        if (global_min_itersec > MINIMUM_ITERATION_SECONDS) {
            global_min_itersec = MINIMUM_ITERATION_SECONDS;
        }
    }
#endif

#ifdef MACTIMEMGR
    /* Set up high res timer */
    MacHSTdelay=600*1000*1000;      /* Delay is 10 minutes */

    memset((char *)&myTMTask,0,sizeof(TMTask));

    /* Prime and remove the task, calculating overhead */
    PrimeTime((QElemPtr)&myTMTask,-MacHSTdelay);
    RmvTime((QElemPtr)&myTMTask);
    MacHSTohead=MacHSTdelay+myTMTask.tmCount;
#endif

#ifdef WIN31TIMER
    /* Load library */
    if((hThlp=LoadLibrary("TOOLHELP.DLL"))<32)
    {
        printf("Error loading TOOLHELP\n");
        exit(0);
    }
    if(!(lpfn=GetProcAddress(hThlp,"TimerCount")))
    {
        printf("TOOLHELP error\n");
        exit(0);
    }
#endif
}

/****************************
** StartStopWatch
** Starts a software stopwatch.
** Store start time in StopWatchStruct passed in
*/
void StartStopWatch(StopWatchStruct *stopwatch)
{
#if defined(MACTIMEMGR)
    /*
     ** For Mac code warrior, use timer.
     */
    InsTime((QElemPtr)&myTMTask);
    PrimeTime((QElemPtr)&myTMTask,-MacHSTdelay);
#elif defined(WIN31TIMER)
    /*
     ** Win 3.x timer returns a DWORD, which we coax into a long.
     */
    _Call16(lpfn,"p",&stopwatch->win31tinfo);
    stopwatch->ticks = (unsigned long)win31tinfo.dwmsSinceStart;
#elif defined(CLOCK_GETTIME)
    int err;
    clock_gettime(global_realtime_cid, &stopwatch->realtime);
    err = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &stopwatch->cputime);
    if (err) {
        stopwatch->ticks = (unsigned long)clock();
    }
#else
    stopwatch->ticks = (unsigned long)clock();
#endif
}

/****************************
** StopStopWatch
** Stops the software stopwatch.
** Store accumated cpu/real time in seconds in StopWatchStruct passed in
*/
void StopStopWatch(StopWatchStruct *stopwatch)
{
#if defined(MACTIMEMGR)
    /*
     ** For Mac code warrior...ignore startticks.  Return val. in microseconds
     */
    RmvTime((QElemPtr)&stopwatch->myTMTask);
    stopwatch->cpusecs += (double)(MacHSTdelay+myTMTask.tmCount-MacHSTohead)*1e-6;
    stopwatch->realsecs = stopwatch->cpusecs;
#elif defined(WIN31TIMER)
    _Call16(lpfn,"p",&win31tinfo);
    stopwatch->cpusecs += (double)((unsigned long)win31tinfo.dwmsSinceStart-stopwatch->ticks)*1e-3;
    stopwatch->realsecs = stopwatch->cpusecs;
#elif defined(CLOCK_GETTIME)
    int err;
    struct timespec cputime, realtime;

    clock_gettime(global_realtime_cid, &realtime);
    stopwatch->realsecs += (double)(realtime.tv_sec - stopwatch->realtime.tv_sec) + (double)(realtime.tv_nsec - stopwatch->realtime.tv_nsec)*1e-9;

    err = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cputime);
    if (err) {
        stopwatch->cpusecs += (double)((unsigned long)clock() - stopwatch->ticks)/(double)CLOCKS_PER_SEC;
    } else {
        stopwatch->cpusecs  += (double)(cputime.tv_sec  - stopwatch->cputime.tv_sec)  + (double)(cputime.tv_nsec  - stopwatch->cputime.tv_nsec)*1e-9;
    }
#elif defined(CLOCKWCT)
    stopwatch->cpusecs += (double)((unsigned long)clock() - stopwatch->ticks)/(double)CLK_TCK;
    stopwatch->realsecs = stopwatch->cpusecs;
#elif defined(CLOCKWCPS)
    stopwatch->cpusecs += (double)((unsigned long)clock() - stopwatch->ticks)/(double)CLOCKS_PER_SEC;
    stopwatch->realsecs = stopwatch->cpusecs;
#endif
}

/****************************
** ResetStopWatch
** Reset the software stopwatch.
** set accumated cpu/real time to zero
*/
void ResetStopWatch(StopWatchStruct *stopwatch)
{
    stopwatch->cpusecs = 0.0;
    stopwatch->realsecs = 0.0;
#ifdef WIN31TIMER
    /* Set up the size of the timer info structure */
    stopwatch0->win31tinfo.dwSize=(DWORD)sizeof(TIMERINFO);
#endif
}
