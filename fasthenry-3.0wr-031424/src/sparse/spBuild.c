/*
 *  MATRIX BUILD MODULE
 *
 *  Author:                     Advising professor:
 *     Kenneth S. Kundert           Alberto Sangiovanni-Vincentelli
 *     UC Berkeley
 *
 *  This file contains the routines associated with clearing, loading and
 *  preprocessing the matrix for the sparse matrix routines.
 *
 *  >>> User accessible functions contained in this file:
 *  spClear
 *  spGetElement
 *  spGetAdmittance
 *  spGetQuad
 *  spGetOnes
 *  spInstallInitInfo
 *  spGetInitInfo
 *  spInitialize
 *
 *  >>> Other functions contained in this file:
 *  spcFindElementInCol
 *  spcCreateElement
 *  spcLinkRows
 */


/*
 *  Revision and copyright information.
 *
 *  Copyright (c) 1985,86,87,88
 *  by Kenneth S. Kundert and the University of California.
 *
 *  Permission to use, copy, modify, and distribute this software and
 *  its documentation for any purpose and without fee is hereby granted,
 *  provided that the copyright notices appear in all copies and
 *  supporting documentation and that the authors and the University of
 *  California are properly credited.  The authors and the University of
 *  California make no representations as to the suitability of this
 *  software for any purpose.  It is provided `as is', without express
 *  or implied warranty.
 */

/*
#ifndef lint
static char copyright[] =
    "Sparse1.3: Copyright (c) 1985,86,87,88 by Kenneth S. Kundert";
static char RCSid[] =
    "@(#)$Header: spBuild.c,v 1.3 88/06/24 05:00:31 kundert Exp $";
#endif
*/




/*
 *  IMPORTS
 *
 *  >>> Import descriptions:
 *  spConfig.h
 *     Macros that customize the sparse matrix routines.
 *  spMatrix.h
 *     Macros and declarations to be imported by the user.
 *  spDefs.h
 *     Matrix type and macro definitions for the sparse matrix routines.
 */

#define spINSIDE_SPARSE
#include "spConfig.h"
#include "spMatrix.h"
#include "spDefs.h"


static void Translate( MatrixPtr, int*, int* );
#if EXPANDABLE
static void EnlargeMatrix( MatrixPtr, int );
#endif
static void ExpandTranslationArrays( MatrixPtr, int );




/*
 *  CLEAR MATRIX
 *
 *  Sets every element of the matrix to zero and clears the error flag.
 *
 *  >>> Arguments:
 *  Matrix  <input>  (char *)
 *     Pointer to matrix that is to be cleared.
 *
 *  >>> Local variables:
 *  pElement  (ElementPtr)
 *     A pointer to the element being cleared.
 */

void
spClear( char *eMatrix )

{
MatrixPtr  Matrix = (MatrixPtr)eMatrix;
register  ElementPtr  pElement;
register  int  I;

/* Begin `spClear'. */
    ASSERT( IS_SPARSE( Matrix ) );

/* Clear matrix. */
#if spCOMPLEX
    if (Matrix->PreviousMatrixWasComplex OR Matrix->Complex)
    {   for (I = Matrix->Size; I > 0; I--)
        {   pElement = Matrix->FirstInCol[I];
            while (pElement != NULL)
            {   pElement->Real = 0.0;
                pElement->Imag = 0.0;
                pElement = pElement->NextInCol;
            }
        }
    }
    else
#endif
    {   for (I = Matrix->Size; I > 0; I--)
        {   pElement = Matrix->FirstInCol[I];
            while (pElement != NULL)
            {   pElement->Real = 0.0;
                pElement = pElement->NextInCol;
            }
        }
    }

/* Empty the trash. */
    Matrix->TrashCan.Real = 0.0;
#if spCOMPLEX
    Matrix->TrashCan.Imag = 0.0;
#endif

    Matrix->Error = spOKAY;
    Matrix->Factored = NO;
    Matrix->SingularCol = 0;
    Matrix->SingularRow = 0;
    Matrix->PreviousMatrixWasComplex = Matrix->Complex;
    return;
}











/*
 *  SINGLE ELEMENT ADDITION TO MATRIX BY INDEX
 *
 *  Finds element [Row,Col] and returns a pointer to it.  If element is
 *  not found then it is created and spliced into matrix.  This routine
 *  is only to be used after spCreate() and before spMNA_Preorder(),
 *  spFactor() or spOrderAndFactor().  Returns a pointer to the
 *  Real portion of a MatrixElement.  This pointer is later used by
 *  spADD_xxx_ELEMENT to directly access element.
 *
 *  >>> Returns:
 *  Returns a pointer to the element.  This pointer is then used to directly
 *  access the element during successive builds.
 *
 *  >>> Arguments:
 *  Matrix  <input>  (char *)
 *     Pointer to the matrix that the element is to be added to.
 *  Row  <input>  (int)
 *     Row index for element.  Must be in the range of [0..Size] unless
 *     the options EXPANDABLE or TRANSLATE are used. Elements placed in
 *     row zero are discarded.  In no case may Row be less than zero.
 *  Col  <input>  (int)
 *     Column index for element.  Must be in the range of [0..Size] unless
 *     the options EXPANDABLE or TRANSLATE are used. Elements placed in
 *     column zero are discarded.  In no case may Col be less than zero.
 *
 *  >>> Local variables:
 *  pElement  (RealNumber *)
 *     Pointer to the element.
 *
 *  >>> Possible errors:
 *  spNO_MEMORY
 *  Error is not cleared in this routine.
 */

/* SRW 080714
 * Modified to support the hash table.
 */

RealNumber *
spGetElement( char *eMatrix, int InRow, int InCol )

{
MatrixPtr  Matrix = (MatrixPtr)eMatrix;
ElementPtr  pElement;
int Row, Col;

/* Begin `spGetElement'. */
    ASSERT( IS_SPARSE( Matrix ) AND InRow >= 0 AND InCol >= 0 );

    if ((InRow == 0) OR (InCol == 0))
        return &Matrix->TrashCan.Real;

#if NOT TRANSLATE
    ASSERT(Matrix->NeedsOrdering);
#endif

#if BUILDHASH
    pElement = sph_get(Matrix, InRow, InCol);
    if (pElement != NULL)
        return ((RealNumber*)pElement);
#endif

    Row = InRow;
    Col = InCol;
#if TRANSLATE
    Translate( Matrix, &Row, &Col );
    if (Matrix->Error == spNO_MEMORY) return NULL;
#endif

#if NOT TRANSLATE
#if NOT EXPANDABLE
    ASSERT(Row <= Matrix->Size AND Col <= Matrix->Size);
#endif

#if EXPANDABLE
/* Re-size Matrix if necessary. */
    if ((Row > Matrix->Size) OR (Col > Matrix->Size))
        EnlargeMatrix( Matrix, MAX(Row, Col) );
    if (Matrix->Error == spNO_MEMORY) return NULL;
#endif
#endif

/*
 * The condition part of the following if statement tests to see if the
 * element resides along the diagonal, if it does then it tests to see
 * if the element has been created yet (Diag pointer not NULL).  The
 * pointer to the element is then assigned to Element after it is cast
 * into a pointer to a RealNumber.  This casting makes the pointer into
 * a pointer to Real.  This statement depends on the fact that Real
 * is the first record in the MatrixElement structure.
 */

    if (Row == Col AND Matrix->Diag[Row] != NULL) {
        pElement = Matrix->Diag[Row];
#if BUILDHASH
        sph_add(Matrix, InRow, InCol, pElement);
#endif
        return ((RealNumber*)pElement);
    }

/*
 * Element does not exist or does not reside along diagonal.  Search
 * column for element.  As in the if statement above, the pointer to the
 * element which is returned by spcFindElementInCol is cast into a
 * pointer to Real, a RealNumber.
 */

    pElement = spcFindElementInCol( Matrix, &(Matrix->FirstInCol[Col]),
                                                     Row, Col, YES );
#if BUILDHASH
    sph_add(Matrix, InRow, InCol, pElement);
#endif
    return (RealNumber*)pElement;
}


/* SRW 080714
 * Without hashing, GetElement is a huge bottleneck when building a
 * large matrix.  This function can provide a big improvement, though
 * less so with the hashing addition.  This adds a list of row
 * elements for a given column, The rows should be in ascending order
 * for best results.  The game is to avoid searching each column from
 * the beginning in spcFindElementInCol, which is very time consuming.
 */

void
spSetRowElements( char *eMatrix, int InCol, struct spColData *ColData,
    int Dsize )
{
MatrixPtr  Matrix = (MatrixPtr)eMatrix;
int I, Row, InRow, Col;

/* Begin `spSetRowElements'. */
    ASSERT( IS_SPARSE( Matrix ) AND InCol >= 0 AND ColData AND Dsize > 0 );

    if (InCol == 0)
        return;

#if NOT TRANSLATE
    ASSERT(Matrix->NeedsOrdering);
#endif

    for (I = 0; I < Dsize; I++) {
        ElementPtr pElement, *Ptr;
        InRow = ColData[I].row;
#if BUILDHASH
        pElement = sph_get(Matrix, InRow, InCol);
        if (pElement != NULL) {
            pElement->Real += ColData[I].real;
            pElement->Imag += ColData[I].imag;
            ColData[I].ptr = pElement;
            continue;
        }
#endif
        Row = InRow;
        Col = InCol;
#if TRANSLATE
        /* This can make row order non-monotonic! */
        Translate( Matrix, &Row, &Col );
        if (Matrix->Error == spNO_MEMORY) return;
#endif

#if NOT TRANSLATE
#if NOT EXPANDABLE
        ASSERT(Row <= Matrix->Size AND Col <= Matrix->Size);
#endif

#if EXPANDABLE
/* Re-size Matrix if necessary. */
        if ((Row > Matrix->Size) OR (Col > Matrix->Size))
            EnlargeMatrix( Matrix, MAX(Row, Col) );
        if (Matrix->Error == spNO_MEMORY) return;
#endif
#endif
        if (Row == Col && Matrix->Diag[Col]) {
            pElement = Matrix->Diag[Col];
            pElement->Real += ColData[I].real;
            pElement->Imag += ColData[I].imag;
            ColData[I].ptr = pElement;
#if BUILDHASH
            sph_add(Matrix, InRow, InCol, pElement);
#endif
            continue;
        }

        int J = I-1;
        pElement = I > 0 ? (ElementPtr)ColData[J].ptr : 0;
        while (pElement && pElement->Row >= Row) {
            if (J == 0) {
                pElement = 0;
                break;
            }
            J--;
            pElement = (ElementPtr)ColData[J].ptr;
        }
        if (pElement)
            Ptr = &pElement->NextInCol;
        else if (Row > Col && Matrix->Diag[Col])
            Ptr = &(Matrix->Diag[Col]->NextInCol);
        else
            Ptr = &(Matrix->FirstInCol[Col]);

        pElement = spcFindElementInCol( Matrix, Ptr, Row, Col, YES );
        pElement->Real += ColData[I].real;
        pElement->Imag += ColData[I].imag;
        ColData[I].ptr = pElement;
#if BUILDHASH
        sph_add(Matrix, InRow, InCol, pElement);
#endif
    }
}


/* SRW 080714 start of hash functions */
#if BUILDHASH

#define ST_MAX_DENS     5
#define ST_START_MASK   31

#if defined(__GNUC__) && (defined(i386) || defined(__x86_64__))
#define rol_bits(x, k) ({ unsigned int t; \
    asm("roll %1,%0" : "=g" (t) : "cI" (k), "0" (x)); t; })
#else
#define rol_bits(x, k) (((x)<<(k)) | ((x)>>(32-(k))))
#endif

// Hashing function for 4/8 byte integers, adapted from
// lookup3.c, by Bob Jenkins, May 2006, Public Domain.
//
unsigned int number_hash(unsigned int n, unsigned int hashmask)
{
    unsigned int a, b, c; // Assumes 32-bit int.

    /*
    if (sizeof(unsigned long) == 8) {
        union { unsigned long l; unsigned int i[2]; } u;
        a = b = c = 0xdeadbeef + 8;
        u.l = n;
        b += u.i[1];
        a += u.i[0];
    }
    else {
        */
        a = b = c = 0xdeadbeef + 4;
        a += n;
    /*
    }
    */

    c ^= b; c -= rol_bits(b, 14);
    a ^= c; a -= rol_bits(c, 11);
    b ^= a; b -= rol_bits(a, 25);
    c ^= b; c -= rol_bits(b, 16);
    a ^= c; a -= rol_bits(c, 4);
    b ^= a; b -= rol_bits(a, 14);
    c ^= b; c -= rol_bits(b, 24);

    return (c & hashmask);
}

#define HEBLKSZ 1024

struct heblk
{
    struct heblk *next;
    struct spHelt elts[HEBLKSZ];
};

static struct heblk *he_blocks;
static int he_blkcnt;

struct spHelt *newhelt()
{
    if (!he_blocks || he_blkcnt == HEBLKSZ) {
        struct heblk *hb = (struct heblk*)malloc(sizeof(struct heblk));
        hb->next = he_blocks;
        he_blocks = hb;
        he_blkcnt = 0;
    }
    return (he_blocks->elts + he_blkcnt++);
};

/*
 * Private function to increase the hash width by one bit.
 */
void sph_rehash(struct spHtab *tab)
{
    unsigned int i, oldmask = tab->mask;
    tab->mask = (oldmask << 1) | 1;
    struct spHelt **oldent = tab->entries;
    tab->entries =
        (struct spHelt**)calloc(tab->mask + 1, sizeof(struct spHelt*));
    for (i = 0; i <= tab->mask; i++)
        tab->entries[i] = 0;
    for (i = 0; i <= oldmask; i++) {
        struct spHelt *h, *hn;
        for (h = oldent[i]; h; h = hn) {
            hn = h->next;
            unsigned int j = number_hash(h->row + h->col, tab->mask);
            h->next = tab->entries[j];
            tab->entries[j] = h;
        }
    }
    free(oldent);
}

/*
 * Add data to the hash table, return 1 if added, 0 if already there
 * or null.  Note that the Row and Col in data are actually ignored.
 */
int sph_add(MatrixPtr Matrix, int row, int col, ElementPtr data)
{
    struct spHtab *tab = Matrix->ElementHashTab;
    if (data == NULL)
        return (0);
    if (!tab) {
        tab = (struct spHtab*)malloc(sizeof(struct spHtab));
        tab->mask = ST_START_MASK;
        tab->entries =
            (struct spHelt**)calloc((tab->mask + 1), sizeof(struct spHelt*));
        tab->allocated = 0;
        tab->recalls = 0;
        Matrix->ElementHashTab = tab;
    }
    unsigned int i = number_hash(row + col, tab->mask);
    struct spHelt *h;
    for (h = tab->entries[i]; h; h = h->next) {
        if (h->row == row && h->col == col) {
            tab->recalls++;
            return (0);
        }
    }
    h = newhelt();
    h->next = tab->entries[i];
    tab->entries[i] = h;
    h->row = row;
    h->col = col;
    h->eptr = data;
    tab->allocated++;
    if (tab->allocated/(tab->mask + 1) > ST_MAX_DENS)
        sph_rehash(tab);
    return (1);
}

/*
 * Return the element at row,col or null if none exists.
 */
ElementPtr sph_get(MatrixPtr Matrix, int row, int col)
{
    struct spHtab *tab = Matrix->ElementHashTab;
    if (tab && tab->allocated) {
        tab->recalls++;
        unsigned int i = number_hash(row + col, tab->mask);
        struct spHelt *h;
        for (h = tab->entries[i]; h; h = h->next) {
            if (h->row == row && h->col == col)
                return (h->eptr);
        }
    }
    return (0);
}

/*
 * Export for FastHenry.
 */
void matStats(void *mp, unsigned int *sz, unsigned int *nm, unsigned int *rc)
{
    MatrixPtr Matrix = (MatrixPtr)mp;
    if (Matrix) {
        if (sz)
            *sz = Matrix->Size;
        struct spHtab *tab = Matrix->ElementHashTab;
        if (tab) {
            if (nm)
                *nm = tab->allocated;
            if (rc)
                *rc = tab->recalls;
        }
        else {
            if (nm)
                *nm = 0;
            if (rc)
                *rc = 0;
        }
    }
    else {
        if (sz)
            *sz = 0;
        if (nm)
            *nm = 0;
        if (rc)
            *rc = 0;
    }
}

void sph_destroy(MatrixPtr Matrix)
{
    struct spHtab *tab = Matrix->ElementHashTab;
    if (!tab)
        return;
    while (he_blocks) {
        struct heblk *hx = he_blocks;
        he_blocks = he_blocks->next;
        free(hx);
    }
    free(tab->entries);
    free(tab);
    Matrix->ElementHashTab = 0;
}
#endif
/* SRW end of hash functions */











/*
 *  FIND ELEMENT BY SEARCHING COLUMN
 *
 *  Searches column starting at element specified at PtrAddr and finds element
 *  in Row. If Element does not exists, it is created. The pointer to the
 *  element is returned.
 *
 *  >>> Returned:
 *  A pointer to the desired element:
 *
 *  >>> Arguments:
 *  Matrix  <input>  (MatrixPtr)
 *      Pointer to Matrix.
 *  LastAddr  <input-output>  (ElementPtr *)
 *      Address of pointer that initially points to the element in Col at which
 *      the search is started.  The pointer in this location may be changed if
 *      a fill-in is required in and adjacent element. For this reason it is
 *      important that LastAddr be the address of a FirstInCol or a NextInCol
 *      rather than a temporary variable.
 *  Row  <input>  (int)
 *      Row being searched for.
 *  Col  (int)
 *      Column being searched.
 *  CreateIfMissing  <input>  (BOOLEAN)
 *      Indicates what to do if element is not found, create one or return a
 *      NULL pointer.
 *
 *  Local variables:
 *  pElement  (ElementPtr)
 *      Pointer used to search through matrix.
 */

ElementPtr
spcFindElementInCol( MatrixPtr Matrix, ElementPtr *LastAddr, int Row, int Col,
    BOOLEAN CreateIfMissing )

{
register  ElementPtr  pElement;

/* Begin `spcFindElementInCol'. */
    pElement = *LastAddr;

/* Search for element. */
    while (pElement != NULL)
    {   if (pElement->Row < Row)
        {
/* Have not reached element yet. */
            LastAddr = &(pElement->NextInCol);
            pElement = pElement->NextInCol;
        }
        else if (pElement->Row == Row)
        {
/* Reached element. */
            return pElement;
        }
        else break;  /* while loop */
    }

/* Element does not exist and must be created. */
    if (CreateIfMissing)
       return spcCreateElement( Matrix, Row, Col, LastAddr, NO );
    else return NULL;
}








#if TRANSLATE

/*
 *  TRANSLATE EXTERNAL INDICES TO INTERNAL
 *
 *  Convert internal row and column numbers to internal row and column numbers.
 *  Also updates Ext/Int maps.
 *
 *
 *  >>> Arguments:
 *  Matrix  <input>    (MatrixPtr)
 *      Pointer to the matrix.
 *  Row  <input/output>  (int *)
 *     Upon entry Row is either a external row number of an external node
 *     number.  Upon entry, the internal equivalent is supplied.
 *  Col  <input/output>  (int *)
 *     Upon entry Column is either a external column number of an external node
 *     number.  Upon entry, the internal equivalent is supplied.
 *
 *  >>> Local variables:
 *  ExtCol  (int)
 *     Temporary variable used to hold the external column or node number
 *     during the external to internal column number translation.
 *  ExtRow  (int)
 *     Temporary variable used to hold the external row or node number during
 *     the external to internal row number translation.
 *  IntCol  (int)
 *     Temporary variable used to hold the internal column or node number
 *     during the external to internal column number translation.
 *  IntRow  (int)
 *     Temporary variable used to hold the internal row or node number during
 *     the external to internal row number translation.
 */

static void
Translate( MatrixPtr Matrix, int *Row, int *Col )

{
register int IntRow, IntCol, ExtRow, ExtCol;

/* Begin `Translate'. */
    ExtRow = *Row;
    ExtCol = *Col;

/* Expand translation arrays if necessary. */
    if ((ExtRow > Matrix->AllocatedExtSize) OR
        (ExtCol > Matrix->AllocatedExtSize))
    {
        ExpandTranslationArrays( Matrix, MAX(ExtRow, ExtCol) );
        if (Matrix->Error == spNO_MEMORY) return;
    }

/* Set ExtSize if necessary. */
    if ((ExtRow > Matrix->ExtSize) OR (ExtCol > Matrix->ExtSize))
        Matrix->ExtSize = MAX(ExtRow, ExtCol);

/* Translate external row or node number to internal row or node number. */
    if ((IntRow = Matrix->ExtToIntRowMap[ExtRow]) == -1)
    {   Matrix->ExtToIntRowMap[ExtRow] = ++Matrix->CurrentSize;
        Matrix->ExtToIntColMap[ExtRow] = Matrix->CurrentSize;
        IntRow = Matrix->CurrentSize;

#if NOT EXPANDABLE
        ASSERT(IntRow <= Matrix->Size);
#endif

#if EXPANDABLE
/* Re-size Matrix if necessary. */
        if (IntRow > Matrix->Size)
            EnlargeMatrix( Matrix, IntRow );
        if (Matrix->Error == spNO_MEMORY) return;
#endif

        Matrix->IntToExtRowMap[IntRow] = ExtRow;
        Matrix->IntToExtColMap[IntRow] = ExtRow;
    }

/* Translate external column or node number to internal column or node number.*/
    if ((IntCol = Matrix->ExtToIntColMap[ExtCol]) == -1)
    {   Matrix->ExtToIntRowMap[ExtCol] = ++Matrix->CurrentSize;
        Matrix->ExtToIntColMap[ExtCol] = Matrix->CurrentSize;
        IntCol = Matrix->CurrentSize;

#if NOT EXPANDABLE
        ASSERT(IntCol <= Matrix->Size);
#endif

#if EXPANDABLE
/* Re-size Matrix if necessary. */
        if (IntCol > Matrix->Size)
            EnlargeMatrix( Matrix, IntCol );
        if (Matrix->Error == spNO_MEMORY) return;
#endif

        Matrix->IntToExtRowMap[IntCol] = ExtCol;
        Matrix->IntToExtColMap[IntCol] = ExtCol;
    }

    *Row = IntRow;
    *Col = IntCol;
    return;
}
#endif






#if QUAD_ELEMENT
/*
 *  ADDITION OF ADMITTANCE TO MATRIX BY INDEX
 *
 *  Performs same function as spGetElement except rather than one
 *  element, all four Matrix elements for a floating component are
 *  added.  This routine also works if component is grounded.  Positive
 *  elements are placed at [Node1,Node2] and [Node2,Node1].  This
 *  routine is only to be used after spCreate() and before
 *  spMNA_Preorder(), spFactor() or spOrderAndFactor().
 *
 *  >>> Returns:
 *  Error code.
 *
 *  >>> Arguments:
 *  Matrix  <input>  (char *)
 *     Pointer to the matrix that component is to be entered in.
 *  Node1  <input>  (int)
 *     Row and column indices for elements. Must be in the range of [0..Size]
 *     unless the options EXPANDABLE or TRANSLATE are used. Node zero is the
 *     ground node.  In no case may Node1 be less than zero.
 *  Node2  <input>  (int)
 *     Row and column indices for elements. Must be in the range of [0..Size]
 *     unless the options EXPANDABLE or TRANSLATE are used. Node zero is the
 *     ground node.  In no case may Node2 be less than zero.
 *  Template  <output>  (struct spTemplate *)
 *     Collection of pointers to four elements that are later used to directly
 *     address elements.  User must supply the template, this routine will
 *     fill it.
 *
 *  Possible errors:
 *  spNO_MEMORY
 *  Error is not cleared in this routine.
 */

int
spGetAdmittance( char *Matrix, int Node1, int Node2,
    struct spTemplate *Template )

{

/* Begin `spGetAdmittance'. */
    Template->Element1 = spGetElement(Matrix, Node1, Node1 );
    Template->Element2 = spGetElement(Matrix, Node2, Node2 );
    Template->Element3Negated = spGetElement( Matrix, Node2, Node1 );
    Template->Element4Negated = spGetElement( Matrix, Node1, Node2 );
    if
    (   (Template->Element1 == NULL)
        OR (Template->Element2 == NULL)
        OR (Template->Element3Negated == NULL)
        OR (Template->Element4Negated == NULL)
    )   return spNO_MEMORY;

    if (Node1 == 0)
        SWAP( RealNumber*, Template->Element1, Template->Element2 );

    return spOKAY;
}
#endif /* QUAD_ELEMENT */









#if QUAD_ELEMENT
/*
 *  ADDITION OF FOUR ELEMENTS TO MATRIX BY INDEX
 *
 *  Similar to spGetAdmittance, except that spGetAdmittance only
 *  handles 2-terminal components, whereas spGetQuad handles simple
 *  4-terminals as well.  These 4-terminals are simply generalized
 *  2-terminals with the option of having the sense terminals different
 *  from the source and sink terminals.  spGetQuad adds four
 *  elements to the matrix.  Positive elements occur at Row1,Col1
 *  Row2,Col2 while negative elements occur at Row1,Col2 and Row2,Col1.
 *  The routine works fine if any of the rows and columns are zero.
 *  This routine is only to be used after spCreate() and before
 *  spMNA_Preorder(), spFactor() or spOrderAndFactor()
 *  unless TRANSLATE is set true.
 *
 *  >>> Returns:
 *  Error code.
 *
 *  >>> Arguments:
 *  Matrix  <input>  (char *)
 *     Pointer to the matrix that component is to be entered in.
 *  Row1  <input>  (int)
 *     First row index for elements. Must be in the range of [0..Size]
 *     unless the options EXPANDABLE or TRANSLATE are used. Zero is the
 *     ground row.  In no case may Row1 be less than zero.
 *  Row2  <input>  (int)
 *     Second row index for elements. Must be in the range of [0..Size]
 *     unless the options EXPANDABLE or TRANSLATE are used. Zero is the
 *     ground row.  In no case may Row2 be less than zero.
 *  Col1  <input>  (int)
 *     First column index for elements. Must be in the range of [0..Size]
 *     unless the options EXPANDABLE or TRANSLATE are used. Zero is the
 *     ground column.  In no case may Col1 be less than zero.
 *  Col2  <input>  (int)
 *     Second column index for elements. Must be in the range of [0..Size]
 *     unless the options EXPANDABLE or TRANSLATE are used. Zero is the
 *     ground column.  In no case may Col2 be less than zero.
 *  Template  <output>  (struct spTemplate *)
 *     Collection of pointers to four elements that are later used to directly
 *     address elements.  User must supply the template, this routine will
 *     fill it.
 *  Real  <input>  (RealNumber)
 *     Real data to be added to elements.
 *  Imag  <input>  (RealNumber)
 *     Imag data to be added to elements.  If matrix is real, this argument
 *     may be deleted.
 *
 *  Possible errors:
 *  spNO_MEMORY
 *  Error is not cleared in this routine.
 */

int
spGetQuad( char *Matrix, int Row1, int Row2, int Col1, int Col2,
    struct spTemplate *Template )

{
/* Begin `spGetQuad'. */
    Template->Element1 = spGetElement( Matrix, Row1, Col1);
    Template->Element2 = spGetElement( Matrix, Row2, Col2 );
    Template->Element3Negated = spGetElement( Matrix, Row2, Col1 );
    Template->Element4Negated = spGetElement( Matrix, Row1, Col2 );
    if
    (   (Template->Element1 == NULL)
        OR (Template->Element2 == NULL)
        OR (Template->Element3Negated == NULL)
        OR (Template->Element4Negated == NULL)
    )   return spNO_MEMORY;

    if (Template->Element1 == &((MatrixPtr)Matrix)->TrashCan.Real)
        SWAP( RealNumber *, Template->Element1, Template->Element2 );

    return spOKAY;
}
#endif /* QUAD_ELEMENT */









#if QUAD_ELEMENT
/*
 *  ADDITION OF FOUR STRUCTURAL ONES TO MATRIX BY INDEX
 *
 *  Performs similar function to spGetQuad() except this routine is
 *  meant for components that do not have an admittance representation.
 *
 *  The following stamp is used:
 *         Pos  Neg  Eqn
 *  Pos  [  .    .    1  ]
 *  Neg  [  .    .   -1  ]
 *  Eqn  [  1   -1    .  ]
 *
 *  >>> Returns:
 *  Error code.
 *
 *  >>> Arguments:
 *  Matrix  <input>  (char *)
 *     Pointer to the matrix that component is to be entered in.
 *  Pos  <input>  (int)
 *     See stamp above. Must be in the range of [0..Size]
 *     unless the options EXPANDABLE or TRANSLATE are used. Zero is the
 *     ground row.  In no case may Pos be less than zero.
 *  Neg  <input>  (int)
 *     See stamp above. Must be in the range of [0..Size]
 *     unless the options EXPANDABLE or TRANSLATE are used. Zero is the
 *     ground row.  In no case may Neg be less than zero.
 *  Eqn  <input>  (int)
 *     See stamp above. Must be in the range of [0..Size]
 *     unless the options EXPANDABLE or TRANSLATE are used. Zero is the
 *     ground row.  In no case may Eqn be less than zero.
 *  Template  <output>  (struct spTemplate *)
 *     Collection of pointers to four elements that are later used to directly
 *     address elements.  User must supply the template, this routine will
 *     fill it.
 *
 *  Possible errors:
 *  spNO_MEMORY
 *  Error is not cleared in this routine.
 */

int
spGetOnes(char *Matrix, int Pos, int Neg, int Eqn, struct spTemplate *Template)

{
/* Begin `spGetOnes'. */
    Template->Element4Negated = spGetElement( Matrix, Neg, Eqn );
    Template->Element3Negated = spGetElement( Matrix, Eqn, Neg );
    Template->Element2 = spGetElement( Matrix, Pos, Eqn );
    Template->Element1 = spGetElement( Matrix, Eqn, Pos );
    if
    (   (Template->Element1 == NULL)
        OR (Template->Element2 == NULL)
        OR (Template->Element3Negated == NULL)
        OR (Template->Element4Negated == NULL)
    )   return spNO_MEMORY;

    spADD_REAL_QUAD( *Template, 1.0 );
    return spOKAY;
}
#endif /* QUAD_ELEMENT */







/*
 *
 *  CREATE AND SPLICE ELEMENT INTO MATRIX
 *
 *  This routine is used to create new matrix elements and splice them into the
 *  matrix.
 *
 *  >>> Returned:
 *  A pointer to the element that was created is returned.
 *
 *  >>> Arguments:
 *  Matrix  <input>  (MatrixPtr)
 *      Pointer to matrix.
 *  Row  <input>  (int)
 *      Row index for element.
 *  Col  <input>  (int)
 *      Column index for element.
 *  LastAddr  <input-output>  (ElementPtr *)
 *      This contains the address of the pointer to the element just above the
 *      one being created. It is used to speed the search and it is updated with
 *      address of the created element.
 *  Fillin  <input>  (BOOLEAN)
 *      Flag that indicates if created element is to be a fill-in.
 *
 *  >>> Local variables:
 *  pElement  (ElementPtr)
 *      Pointer to an element in the matrix. It is used to refer to the newly
 *      created element and to restring the pointers of the element's row and
 *      column.
 *  pLastElement  (ElementPtr)
 *      Pointer to the element in the matrix that was just previously pointed
 *      to by pElement. It is used to restring the pointers of the element's
 *      row and column.
 *  pCreatedElement  (ElementPtr)
 *      Pointer to the desired element, the one that was just created.
 *
 *  >>> Possible errors:
 *  spNO_MEMORY
 */

ElementPtr
spcCreateElement( MatrixPtr Matrix, int Row, int Col, ElementPtr *LastAddr,
    BOOLEAN Fillin )

{
register  ElementPtr  pElement, pLastElement;
ElementPtr  pCreatedElement;

/* Begin `spcCreateElement'. */

    if (Matrix->RowsLinked)
    {
/* Row pointers cannot be ignored. */
        if (Fillin)
        {   pElement = spcGetFillin( Matrix );
            Matrix->Fillins++;
        }
        else
        {   pElement = spcGetElement( Matrix );
            Matrix->NeedsOrdering = YES;
        }
        if (pElement == NULL) return NULL;

/* If element is on diagonal, store pointer in Diag. */
        if (Row == Col) Matrix->Diag[Row] = pElement;

/* Initialize Element. */
        pCreatedElement = pElement;
        pElement->Row = Row;
        pElement->Col = Col;
        pElement->Real = 0.0;
#if spCOMPLEX
        pElement->Imag = 0.0;
#endif
#if INITIALIZE
        pElement->pInitInfo = NULL;
#endif

/* Splice element into column. */
        pElement->NextInCol = *LastAddr;
        *LastAddr = pElement;

 /* Search row for proper element position. */
        pElement = Matrix->FirstInRow[Row];
        pLastElement = NULL;
        while (pElement != NULL)
        {
/* Search for element row position. */
            if (pElement->Col < Col)
            {
/* Have not reached desired element. */
                pLastElement = pElement;
                pElement = pElement->NextInRow;
            }
            else pElement = NULL;
        }

/* Splice element into row. */
        pElement = pCreatedElement;
        if (pLastElement == NULL)
        {
/* Element is first in row. */
            pElement->NextInRow = Matrix->FirstInRow[Row];
            Matrix->FirstInRow[Row] = pElement;
        }
        else
/* Element is not first in row. */
        {
            pElement->NextInRow = pLastElement->NextInRow;
            pLastElement->NextInRow = pElement;
        }

    }
    else
    {
/*
 * Matrix has not been factored yet.  Thus get element rather than fill-in.
 * Also, row pointers can be ignored.
 */

/* Allocate memory for Element. */
        pElement = spcGetElement( Matrix );
        if (pElement == NULL) return NULL;

/* If element is on diagonal, store pointer in Diag. */
        if (Row == Col) Matrix->Diag[Row] = pElement;

/* Initialize Element. */
        pCreatedElement = pElement;
        pElement->Row = Row;
#if DEBUG
        pElement->Col = Col;
#endif
        pElement->Real = 0.0;
#if spCOMPLEX
        pElement->Imag = 0.0;
#endif
#if INITIALIZE
        pElement->pInitInfo = NULL;
#endif

/* Splice element into column. */
        pElement->NextInCol = *LastAddr;
        *LastAddr = pElement;
    }

    Matrix->Elements++;
    return pCreatedElement;
}








/*
 *
 *  LINK ROWS
 *
 *  This routine is used to generate the row links.  The spGetElement()
 *  routines do not create row links, which are needed by the spFactor()
 *  routines.
 *
 *  >>>  Arguments:
 *  Matrix  <input>  (MatrixPtr)
 *      Pointer to the matrix.
 *
 *  >>> Local variables:
 *  pElement  (ElementPtr)
 *      Pointer to an element in the matrix.
 *  FirstInRowEntry  (ElementPtr *)
 *      A pointer into the FirstInRow array.  Points to the FirstInRow entry
 *      currently being operated upon.
 *  FirstInRowArray  (ArrayOfElementPtrs)
 *      A pointer to the FirstInRow array.  Same as Matrix->FirstInRow but
 *      resides in a register and requires less indirection so is faster to
 *      use.
 *  Col  (int)
 *      Column currently being operated upon.
 */

void spcLinkRows( MatrixPtr Matrix )

{
register  ElementPtr  pElement, *FirstInRowEntry;
register  ArrayOfElementPtrs  FirstInRowArray;
register  int  Col;

/* Begin `spcLinkRows'. */
    FirstInRowArray = Matrix->FirstInRow;
    for (Col = Matrix->Size; Col >= 1; Col--)
    {
/* Generate row links for the elements in the Col'th column. */
        pElement = Matrix->FirstInCol[Col];

        while (pElement != NULL)
        {   pElement->Col = Col;
            FirstInRowEntry = &FirstInRowArray[pElement->Row];
            pElement->NextInRow = *FirstInRowEntry;
            *FirstInRowEntry = pElement;
            pElement = pElement->NextInCol;
        }
    }
    Matrix->RowsLinked = YES;
    return;
}






#if EXPANDABLE


/*
 *  ENLARGE MATRIX
 *
 *  Increases the size of the matrix.
 *
 *  >>> Arguments:
 *  Matrix  <input>    (MatrixPtr)
 *      Pointer to the matrix.
 *  NewSize  <input>  (int)
 *     The new size of the matrix.
 *
 *  >>> Local variables:
 *  OldAllocatedSize  (int)
 *     The allocated size of the matrix before it is expanded.
 */

static void
EnlargeMatrix( MatrixPtr Matrix, int NewSize )

{
register int I, OldAllocatedSize = Matrix->AllocatedSize;

/* Begin `EnlargeMatrix'. */
    Matrix->Size = NewSize;

    if (NewSize <= OldAllocatedSize)
        return;

/* Expand the matrix frame. */
    NewSize = MAX( NewSize, EXPANSION_FACTOR * OldAllocatedSize );
    Matrix->AllocatedSize = NewSize;

    if (( REALLOC(Matrix->IntToExtColMap, int, NewSize+1)) == NULL)
    {   Matrix->Error = spNO_MEMORY;
        return;
    }
    if (( REALLOC(Matrix->IntToExtRowMap, int, NewSize+1)) == NULL)
    {   Matrix->Error = spNO_MEMORY;
        return;
    }
    if (( REALLOC(Matrix->Diag, ElementPtr, NewSize+1)) == NULL)
    {   Matrix->Error = spNO_MEMORY;
        return;
    }
    if (( REALLOC(Matrix->FirstInCol, ElementPtr, NewSize+1)) == NULL)
    {   Matrix->Error = spNO_MEMORY;
        return;
    }
    if (( REALLOC(Matrix->FirstInRow, ElementPtr, NewSize+1)) == NULL)
    {   Matrix->Error = spNO_MEMORY;
        return;
    }

/*
 * Destroy the Markowitz and Intermediate vectors, they will be recreated
 * in spOrderAndFactor().
 */
    FREE( Matrix->MarkowitzRow );
    FREE( Matrix->MarkowitzCol );
    FREE( Matrix->MarkowitzProd );
    FREE( Matrix->DoRealDirect );
    FREE( Matrix->DoCmplxDirect );
    FREE( Matrix->Intermediate );
    Matrix->InternalVectorsAllocated = NO;

/* Initialize the new portion of the vectors. */
    for (I = OldAllocatedSize+1; I <= NewSize; I++)
    {   Matrix->IntToExtColMap[I] = I;
        Matrix->IntToExtRowMap[I] = I;
        Matrix->Diag[I] = NULL;
        Matrix->FirstInRow[I] = NULL;
        Matrix->FirstInCol[I] = NULL;
    }

    return;
}

#endif








#if TRANSLATE

/*
 *  EXPAND TRANSLATION ARRAYS
 *
 *  Increases the size arrays that are used to translate external to internal
 *  row and column numbers.
 *
 *  >>> Arguments:
 *  Matrix  <input>    (MatrixPtr)
 *      Pointer to the matrix.
 *  NewSize  <input>  (int)
 *     The new size of the translation arrays.
 *
 *  >>> Local variables:
 *  OldAllocatedSize  (int)
 *     The allocated size of the translation arrays before being expanded.
 */

static void
ExpandTranslationArrays( MatrixPtr Matrix, int NewSize )

{
register int I, OldAllocatedSize = Matrix->AllocatedExtSize;

/* Begin `ExpandTranslationArrays'. */
    Matrix->ExtSize = NewSize;

    if (NewSize <= OldAllocatedSize)
        return;

/* Expand the translation arrays ExtToIntRowMap and ExtToIntColMap. */
    NewSize = MAX( NewSize, EXPANSION_FACTOR * OldAllocatedSize );
    Matrix->AllocatedExtSize = NewSize;

    if (( REALLOC(Matrix->ExtToIntRowMap, int, NewSize+1)) == NULL)
    {   Matrix->Error = spNO_MEMORY;
        return;
    }
    if (( REALLOC(Matrix->ExtToIntColMap, int, NewSize+1)) == NULL)
    {   Matrix->Error = spNO_MEMORY;
        return;
    }

/* Initialize the new portion of the vectors. */
    for (I = OldAllocatedSize+1; I <= NewSize; I++)
    {   Matrix->ExtToIntRowMap[I] = -1;
        Matrix->ExtToIntColMap[I] = -1;
    }

    return;
}
#endif









#if INITIALIZE
/*
 *   INITIALIZE MATRIX
 *
 *   With the INITIALIZE compiler option (see spConfig.h) set true,
 *   Sparse allows the user to keep initialization information with each
 *   structurally nonzero matrix element.  Each element has a pointer
 *   that is set and used by the user.  The user can set this pointer
 *   using spInstallInitInfo and may be read using spGetInitInfo.  Both
 *   may be used only after the element exists.  The function
 *   spInitialize() is a user customizable way to initialize the matrix.
 *   Passed to this routine is a function pointer.  spInitialize() sweeps
 *   through every element in the matrix and checks the pInitInfo
 *   pointer (the user supplied pointer).  If the pInitInfo is NULL,
 *   which is true unless the user changes it (almost always true for
 *   fill-ins), then the element is zeroed.  Otherwise, the function
 *   pointer is called and passed the pInitInfo pointer as well as the
 *   element pointer and the external row and column numbers.  If the
 *   user sets the value of each element, then spInitialize() replaces
 *   spClear().
 *
 *   The user function is expected to return a nonzero integer if there
 *   is a fatal error and zero otherwise.  Upon encountering a nonzero
 *   return code, spInitialize() terminates and returns the error code.
 *
 *   >>> Arguments:
 *   Matrix  <input>  (char *)
 *       Pointer to matrix.
 *
 *   >>> Possible Errors:
 *   Returns nonzero if error, zero otherwise.
 */

void
spInstallInitInfo( RealNumber *pElement, char *pInitInfo )

{
/* Begin `spInstallInitInfo'. */
    ASSERT(pElement != NULL);

    ((ElementPtr)pElement)->pInitInfo = pInitInfo;
}


char *
spGetInitInfo( RealNumber *pElement )

{
/* Begin `spGetInitInfo'. */
    ASSERT(pElement != NULL);

    return (char *)((ElementPtr)pElement)->pInitInfo;
}


int
spInitialize( char *eMatrix, InitFunc pInit )

{
MatrixPtr Matrix = (MatrixPtr)eMatrix;
register ElementPtr pElement;
int J, Error, Col;

/* Begin `spInitialize'. */
    ASSERT( IS_SPARSE( Matrix ) );

#if spCOMPLEX
/* Clear imaginary part of matrix if matrix is real but was complex. */
    if (Matrix->PreviousMatrixWasComplex AND NOT Matrix->Complex)
    {   for (J = Matrix->Size; J > 0; J--)
        {   pElement = Matrix->FirstInCol[J];
            while (pElement != NULL)
            {   pElement->Imag = 0.0;
                pElement = pElement->NextInCol;
            }
        }
    }
#endif /* spCOMPLEX */

/* Initialize the matrix. */
    for (J = Matrix->Size; J > 0; J--)
    {   pElement = Matrix->FirstInCol[J];
        Col = Matrix->IntToExtColMap[J];
        while (pElement != NULL)
        {   if (pElement->pInitInfo == NULL)
            {   pElement->Real = 0.0;
#               if spCOMPLEX
                    pElement->Imag = 0.0;
#               endif
            }
            else
            {   Error = (*pInit)((RealNumber *)pElement, pElement->pInitInfo,
                                 Matrix->IntToExtRowMap[pElement->Row], Col);
                if (Error)
                {   Matrix->Error = spFATAL;
                    return Error;
                }

            }
            pElement = pElement->NextInCol;
        }
    }

/* Empty the trash. */
    Matrix->TrashCan.Real = 0.0;
#if spCOMPLEX
    Matrix->TrashCan.Imag = 0.0;
#endif

    Matrix->Error = spOKAY;
    Matrix->Factored = NO;
    Matrix->SingularCol = 0;
    Matrix->SingularRow = 0;
    Matrix->PreviousMatrixWasComplex = Matrix->Complex;
    return 0;
}
#endif /* INITIALIZE */
