/*
** huffman.c
*/

/********************************
**       BYTEmark (tm)         **
** BYTE NATIVE MODE BENCHMARKS **
**       VERSION 2             **
**                             **
** Included in this source     **
** file:                       **
**  Huffman compression        **
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
#include <strings.h>
#include <math.h>
#include "nmglobal.h"
#include "sysspec.h"
#include "misc.h"
#include "wordcat.h"

/************************
** HUFFMAN COMPRESSION **
************************/

/*
** DEFINES
*/
#define EXCLUDED 32000L          /* Big positive value */

/*
** TYPEDEFS
*/
typedef struct {
	uchar c;                /* Byte value */
	float freq;             /* Frequency */
	int parent;             /* Parent node */
	int left;               /* Left pointer = 0 */
	int right;              /* Right pointer = 1 */
} huff_node;

/*
** GLOBALS
*/
static huff_node *hufftree;             /* The huffman tree */
static long plaintextlen;               /* Length of plaintext */

/*
** PROTOTYPES
*/
void DoHuffman();
static void create_text_line(farchar *dt,long nchars);
static void create_text_block(farchar *tb, ulong tblen,
		ushort maxlinlen);
static ulong DoHuffIteration(farchar *plaintext,
	farchar *comparray, farchar *decomparray,
	ulong arraysize, ulong nloops, huff_node *hufftree);
static void SetCompBit(u8 *comparray, u32 bitoffset, char bitchar);
static int GetCompBit(u8 *comparray, u32 bitoffset);

/**************
** DoHuffman **
***************
** Execute a huffman compression on a block of plaintext.
** Note that (as with IDEA encryption) an iteration of the
** Huffman test includes a compression AND a decompression.
** Also, the compression cycle includes building the
** Huffman tree.
*/
void DoHuffman(void)
{
    HuffStruct *lochuffstruct;      /* Loc pointer to global data */
    char *errorcontext;
    int systemerror;
    ulong accumtime;
    double iterations;
    farchar *comparray;
    farchar *decomparray;
    farchar *plaintext;

    /*
     ** Link to global data
     */
    lochuffstruct=&global_huffstruct;

    /*
     ** Set error context.
     */
    errorcontext="CPU:Huffman";

    /*
     ** Allocate memory for the plaintext and the compressed text.
     ** We'll be really pessimistic here, and allocate equal amounts
     ** for both (though we know...well, we PRESUME) the compressed
     ** stuff will take less than the plain stuff.
     ** Also note that we'll build a 3rd buffer to decompress
     ** into, and we preallocate space for the huffman tree.
     ** (We presume that the Huffman tree will grow no larger
     ** than 512 bytes.  This is actually a super-conservative
     ** estimate...but, who cares?)
     */
    plaintext=(farchar *)AllocateMemory(lochuffstruct->arraysize,&systemerror);
    if(systemerror)
    {       ReportError(errorcontext,systemerror);
        ErrorExit();
    }
    comparray=(farchar *)AllocateMemory(lochuffstruct->arraysize,&systemerror);
    if(systemerror)
    {       ReportError(errorcontext,systemerror);
        FreeMemory(plaintext,&systemerror);
        ErrorExit();
    }
    decomparray=(farchar *)AllocateMemory(lochuffstruct->arraysize,&systemerror);
    if(systemerror)
    {       ReportError(errorcontext,systemerror);
        FreeMemory(plaintext,&systemerror);
        FreeMemory(comparray,&systemerror);
        ErrorExit();
    }

    hufftree=(huff_node *)AllocateMemory(sizeof(huff_node) * 512,
            &systemerror);
    if(systemerror)
    {       ReportError(errorcontext,systemerror);
        FreeMemory(plaintext,&systemerror);
        FreeMemory(comparray,&systemerror);
        FreeMemory(decomparray,&systemerror);
        ErrorExit();
    }

    /*
     ** Build the plaintext buffer.  Since we want this to
     ** actually be able to compress, we'll use the
     ** wordcatalog to build the plaintext stuff.
     */
    /*
     ** Reset random number generator so things repeat.
     ** added by Uwe F. Mayer
     */
    randnum((int32)13);
    create_text_block(plaintext,lochuffstruct->arraysize-1,(ushort)500);
    plaintext[lochuffstruct->arraysize-1L]='\0';
    plaintextlen=lochuffstruct->arraysize;

    /*
     ** See if we need to perform self adjustment loop.
     */
    if(lochuffstruct->adjust==0)
    {
        /*
         ** Do self-adjustment.  This involves initializing the
         ** # of loops and increasing the loop count until we
         ** get a number of loops that we can use.
         */
        for(lochuffstruct->loops=100L;
                lochuffstruct->loops<MAXHUFFLOOPS;
                lochuffstruct->loops+=10L)
            if(DoHuffIteration(plaintext,
                        comparray,
                        decomparray,
                        lochuffstruct->arraysize,
                        lochuffstruct->loops,
                        hufftree)>global_min_ticks) break;
    }

    /*
     ** All's well if we get here.  Do the test.
     */
    accumtime=0L;
    iterations=(double)0.0;

    do {
        accumtime+=DoHuffIteration(plaintext,
                comparray,
                decomparray,
                lochuffstruct->arraysize,
                lochuffstruct->loops,
                hufftree);
        iterations+=(double)lochuffstruct->loops;
    } while(TicksToSecs(accumtime)<lochuffstruct->request_secs);

    /*
     ** Clean up, calculate results, and go home.  Be sure to
     ** show that we don't have to rerun adjustment code.
     */
    FreeMemory((farvoid *)plaintext,&systemerror);
    FreeMemory((farvoid *)comparray,&systemerror);
    FreeMemory((farvoid *)decomparray,&systemerror);
    FreeMemory((farvoid *)hufftree,&systemerror);
    lochuffstruct->iterspersec=iterations / TicksToFracSecs(accumtime);

    if(lochuffstruct->adjust==0)
        lochuffstruct->adjust=1;

}

/*********************
** create_text_line **
**********************
** Create a random line of text, stored at *dt.  The line may be
** no more than nchars long.
*/
static void create_text_line(farchar *dt,
            long nchars)
{
    long charssofar;        /* # of characters so far */
    long tomove;            /* # of characters to move */
    char myword[40];        /* Local buffer for words */
    farchar *wordptr;       /* Pointer to word from catalog */

    charssofar=0;

    do {
        /*
         ** Grab a random word from the wordcatalog
         */
        /* wordptr=wordcatarray[abs_randwc((long)WORDCATSIZE)];*/
        wordptr=wordcatarray[abs_randwc((int32)WORDCATSIZE)];
        MoveMemory((farvoid *)myword,
                (farvoid *)wordptr,
                (unsigned long)strlen(wordptr)+1);

        /*
         ** Append a blank.
         */
        tomove=strlen(myword)+1;
        myword[tomove-1]=' ';

        /*
         ** See how long it is.  If its length+charssofar > nchars, we have
         ** to trim it.
         */
        if((tomove+charssofar)>nchars)
            tomove=nchars-charssofar;
        /*
         ** Attach the word to the current line.  Increment counter.
         */
        MoveMemory((farvoid *)dt,(farvoid *)myword,(unsigned long)tomove);
        charssofar+=tomove;
        dt+=tomove;

        /*
         ** If we're done, bail out.  Otherwise, go get another word.
         */
    } while(charssofar<nchars);

    return;
}

/**********************
** create_text_block **
***********************
** Build a block of text randomly loaded with words.  The words
** come from the wordcatalog (which must be loaded before you
** call this).
** *tb points to the memory where the text is to be built.
** tblen is the # of bytes to put into the text block
** maxlinlen is the maximum length of any line (line end indicated
**  by a carriage return).
*/
static void create_text_block(farchar *tb,
            ulong tblen,
            ushort maxlinlen)
{
    ulong bytessofar;       /* # of bytes so far */
    ulong linelen;          /* Line length */

    bytessofar=0L;
    do {

        /*
         ** Pick a random length for a line and fill the line.
         ** Make sure the line can fit (haven't exceeded tablen) and also
         ** make sure you leave room to append a carriage return.
         */
        linelen=abs_randwc(maxlinlen-6)+6;
        if((linelen+bytessofar)>tblen)
            linelen=tblen-bytessofar;

        if(linelen>1)
        {
            create_text_line(tb,linelen);
        }
        tb+=linelen-1;          /* Add the carriage return */
        *tb++='\n';

        bytessofar+=linelen;

    } while(bytessofar<tblen);

}

/********************
** DoHuffIteration **
*********************
** Perform the huffman benchmark.  This routine
**  (a) Builds the huffman tree
**  (b) Compresses the text
**  (c) Decompresses the text and verifies correct decompression
*/
static ulong DoHuffIteration(farchar *plaintext,
    farchar *comparray,
    farchar *decomparray,
    ulong arraysize,
    ulong nloops,
    huff_node *hufftree)
{
    int i;                          /* Index */
    long j;                         /* Bigger index */
    int root;                       /* Pointer to huffman tree root */
    float lowfreq1, lowfreq2;       /* Low frequency counters */
    int lowidx1, lowidx2;           /* Indexes of low freq. elements */
    long bitoffset;                 /* Bit offset into text */
    long textoffset;                /* Char offset into text */
    long maxbitoffset;              /* Holds limit of bit offset */
    long bitstringlen;              /* Length of bitstring */
    int c;                          /* Character from plaintext */
    char bitstring[30];             /* Holds bitstring */
    ulong elapsed;                  /* For stopwatch */
#ifdef DEBUG
    int status=0;
#endif

    /*
     ** Start the stopwatch
     */
    elapsed=StartStopwatch();

    /*
     ** Do everything for nloops
     */
    while(nloops--)
    {

        /*
         ** Calculate the frequency of each byte value. Store the
         ** results in what will become the "leaves" of the
         ** Huffman tree.  Interior nodes will be built in those
         ** nodes greater than node #255.
         */
        for(i=0;i<256;i++)
        {
            hufftree[i].freq=(float)0.0;
            hufftree[i].c=(unsigned char)i;
        }

        for(j=0;j<arraysize;j++)
            hufftree[(int)plaintext[j]].freq+=(float)1.0;

        for(i=0;i<256;i++)
            if(hufftree[i].freq != (float)0.0)
                hufftree[i].freq/=(float)arraysize;

        /* Reset the second half of the tree. Otherwise the loop below that
         ** compares the frequencies up to index 512 makes no sense. Some
         ** systems automatically zero out memory upon allocation, others (like
         ** for example DEC Unix) do not. Depending on this the loop below gets
         ** different data and different run times. On our alpha the data that
         ** was arbitrarily assigned led to an underflow error at runtime. We
         ** use that zeroed-out bits are in fact 0 as a float.
         ** Uwe F. Mayer */
        bzero((char *)&(hufftree[256]),sizeof(huff_node)*256);
        /*
         ** Build the huffman tree.  First clear all the parent
         ** pointers and left/right pointers.  Also, discard all
         ** nodes that have a frequency of true 0.  */
        for(i=0;i<512;i++)
        {       if(hufftree[i].freq==(float)0.0)
            hufftree[i].parent=EXCLUDED;
            else
                hufftree[i].parent=hufftree[i].left=hufftree[i].right=-1;
        }

        /*
         ** Go through the tree. Finding nodes of really low
         ** frequency.
         */
        root=255;                       /* Starting root node-1 */
        while(1)
        {
            lowfreq1=(float)2.0; lowfreq2=(float)2.0;
            lowidx1=-1; lowidx2=-1;
            /*
             ** Find first lowest frequency.
             */
            for(i=0;i<=root;i++)
                if(hufftree[i].parent<0)
                    if(hufftree[i].freq<lowfreq1)
                    {       lowfreq1=hufftree[i].freq;
                        lowidx1=i;
                    }

            /*
             ** Did we find a lowest value?  If not, the
             ** tree is done.
             */
            if(lowidx1==-1) break;

            /*
             ** Find next lowest frequency
             */
            for(i=0;i<=root;i++)
                if((hufftree[i].parent<0) && (i!=lowidx1))
                    if(hufftree[i].freq<lowfreq2)
                    {       lowfreq2=hufftree[i].freq;
                        lowidx2=i;
                    }

            /*
             ** If we could only find one item, then that
             ** item is surely the root, and (as above) the
             ** tree is done.
             */
            if(lowidx2==-1) break;

            /*
             ** Attach the two new nodes to the current root, and
             ** advance the current root.
             */
            root++;                 /* New root */
            hufftree[lowidx1].parent=root;
            hufftree[lowidx2].parent=root;
            hufftree[root].freq=lowfreq1+lowfreq2;
            hufftree[root].left=lowidx1;
            hufftree[root].right=lowidx2;
            hufftree[root].parent=-2;       /* Show root */
        }

        /*
         ** Huffman tree built...compress the plaintext
         */
        bitoffset=0L;                           /* Initialize bit offset */
        for(i=0;i<arraysize;i++)
        {
            c=(int)plaintext[i];                 /* Fetch character */
            /*
             ** Build a bit string for byte c
             */
            bitstringlen=0;
            while(hufftree[c].parent!=-2)
            {       if(hufftree[hufftree[c].parent].left==c)
                bitstring[bitstringlen]='0';
                else
                    bitstring[bitstringlen]='1';
                c=hufftree[c].parent;
                bitstringlen++;
            }

            /*
             ** Step backwards through the bit string, setting
             ** bits in the compressed array as you go.
             */
            while(bitstringlen--)
            {       SetCompBit((u8 *)comparray,(u32)bitoffset,bitstring[bitstringlen]);
                bitoffset++;
            }
        }

        /*
         ** Compression done.  Perform de-compression.
         */
        maxbitoffset=bitoffset;
        bitoffset=0;
        textoffset=0;
        do {
            i=root;
            while(hufftree[i].left!=-1)
            {       if(GetCompBit((u8 *)comparray,(u32)bitoffset)==0)
                i=hufftree[i].left;
                else
                    i=hufftree[i].right;
                bitoffset++;
            }
            decomparray[textoffset]=hufftree[i].c;

#ifdef DEBUG
            if(hufftree[i].c != plaintext[textoffset])
            {
                /* Show error */
                printf("Error at textoffset %ld\n",textoffset);
                status=1;
            }
#endif
            textoffset++;
        } while(bitoffset<maxbitoffset);

    }       /* End the big while(nloops--) from above */

    /*
     ** All done
     */
#ifdef DEBUG
    if (status==0) printf("Huffman: OK\n");
#endif
    return(StopStopwatch(elapsed));
}

/***************
** SetCompBit **
****************
** Set a bit in the compression array.  The value of the
** bit is set according to char bitchar.
*/
static void SetCompBit(u8 *comparray,
        u32 bitoffset,
        char bitchar)
{
    u32 byteoffset;
    int bitnumb;

    /*
     ** First calculate which element in the comparray to
     ** alter. and the bitnumber.
     */
    byteoffset=bitoffset>>3;
    bitnumb=bitoffset % 8;

    /*
     ** Set or clear
     */
    if(bitchar=='1')
        comparray[byteoffset]|=(1<<bitnumb);
    else
        comparray[byteoffset]&=~(1<<bitnumb);

    return;
}

/***************
** GetCompBit **
****************
** Return the bit value of a bit in the comparession array.
** Returns 0 if the bit is clear, nonzero otherwise.
*/
static int GetCompBit(u8 *comparray,
        u32 bitoffset)
{
    u32 byteoffset;
    int bitnumb;

    /*
     ** Calculate byte offset and bit number.
     */
    byteoffset=bitoffset>>3;
    bitnumb=bitoffset % 8;

    /*
     ** Fetch
     */
    return((1<<bitnumb) & comparray[byteoffset] );
}
