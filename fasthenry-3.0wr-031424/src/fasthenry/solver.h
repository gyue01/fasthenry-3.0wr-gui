/*
 * Linear Algebra extensions to FastHenry
 * S. R. Whiteley 12/28/2018 wrcad.com
 */

#ifndef SOLVER_H
#define SOLVER_H

/* SRW 062620
** When defined, use hashing when building the preconditioner sparse
** matrix, which may be much faster than ordering the linked lists.
*/
#define SOLVER_HASH

#define matREAL double

/* Matrrix element data item. */
typedef struct matData
{
    int row;
    int col;
    double real;
    double imag;
    void *next;
} matData;

#ifdef SOLVER_HASH

/* Bulk element allocation. */
#define SP_HBLKSZ 1024
typedef struct spHeltBlk
{
    struct spHeltBlk *next;
    matData elts[SP_HBLKSZ];
} spHeltBlk;

/* The row indices for each column are hashed. */
typedef struct rowTab
{
    matData **entries;
    unsigned int mask;
    unsigned int allocated;
} rowTab;

/* Main hashing database container. */
typedef struct spHtab
{
    rowTab *cols;
    unsigned int numcols;
    unsigned int allocated;
    unsigned int recalls;
    unsigned int ecount;
    spHeltBlk *eblocks;
} spHtab;

void htab_init(spHtab*, unsigned int);
matData *htab_get(spHtab*, int, int);
void htab_set_row_elts(spHtab*, int, matData*, int);
matData *htab_list(spHtab*);

#endif

char *matCreate(int, int, int*);
void matSetPureSC(void*, int);
void matSolve(void*, matREAL*, matREAL*);
int matOrderAndFactor(void*, matREAL*, matREAL, matREAL, int);
int matFactor(void*);
void matClear(void*);
matREAL *matGetElement(void*, int, int);
void matSetRowElements(void*, int, matData*, int);
void matStats(void*, unsigned int*, unsigned int*, unsigned int*);
int matFileMatrix(void*, const char*, const char*, int, int, int);
int matElementCount(void*);
int matFillinCount(void*);

void solver_message();

#endif

