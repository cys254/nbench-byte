/*
** nbench1.h
** Header for nbench1.c
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

/*
** DEFINES
*/
/* #define DEBUG */

/*
** EXTERNALS
*/
extern ulong global_min_ticks;

extern SortStruct global_numsortstruct;
extern SortStruct global_strsortstruct;
extern BitOpStruct global_bitopstruct;
extern EmFloatStruct global_emfloatstruct;
extern FourierStruct global_fourierstruct;
extern AssignStruct global_assignstruct;
extern IDEAStruct global_ideastruct;
extern HuffStruct global_huffstruct;
extern NNetStruct global_nnetstruct;
extern LUStruct global_lustruct;

/* External PROTOTYPES */
/*extern unsigned long abs_randwc(unsigned long num);*/     /* From MISC */
/*extern long randnum(long lngval);*/
extern int32 randwc(int32 num);
extern u32 abs_randwc(u32 num);
extern int32 randnum(int32 lngval);

extern farvoid *AllocateMemory(unsigned long nbytes,    /* From SYSSPEC */
	int *errorcode);
extern void FreeMemory(farvoid *mempointer,
	int *errorcode);
extern void MoveMemory(farvoid *destination,
		farvoid *source, unsigned long nbytes);
extern void ReportError(char *context, int errorcode);
extern void ErrorExit();
extern unsigned long StartStopwatch();
extern unsigned long StopStopwatch(unsigned long startticks);
extern unsigned long TicksToSecs(unsigned long tickamount);
extern double TicksToFracSecs(unsigned long tickamount);

/*****************
** NUMERIC SORT **
*****************/

/*
** PROTOTYPES
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