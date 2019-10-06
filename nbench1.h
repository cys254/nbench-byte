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


/*****************
** NUMERIC SORT **
*****************/

void DoNumSort(void);


/****************
** STRING SORT **
*****************
*/

void DoStringSort(void);

/************************
** BITFIELD OPERATIONS **
*************************
*/

void DoBitops(void);

/****************************
** EMULATED FLOATING POINT **
****************************/

void DoEmFloat(void);

/*************************
** FOURIER COEFFICIENTS **
*************************/

void DoFourier(void);

/*************************
** ASSIGNMENT ALGORITHM **
*************************/

void DoAssign(void);

/********************
** IDEA ENCRYPTION **
********************/

void DoIDEA(void);

/************************
** HUFFMAN COMPRESSION **
************************/

void DoHuffman();

/********************************
** BACK PROPAGATION NEURAL NET **
********************************/

void DoNNET(void);

/***********************
**  LU DECOMPOSITION  **
** (Linear Equations) **
***********************/

void DoLU(void);
