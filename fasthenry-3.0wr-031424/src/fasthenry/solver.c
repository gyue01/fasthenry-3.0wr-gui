
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2018 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  This software is provided under the terms and conditions of the       *
 *  MIT license as it applies the the FastHenry computer software.        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 * Linear Algebra extensions to FastHenry
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "solver.h"
#include "stdio.h"

/* #define DEBUG */

/* Exactly one of the defines referenced below should be defined in the
 * Makefile.
 */

// Original Berkeley Sparse solver package.
#ifdef SPARSE_SOLVER

void solver_message()
{
    fprintf(stdout,
        "Using SPARSE linear algebra package by Ken Kundert, UC Berkeley.\n");
}

#include "solver_sparse.c"
#endif

/* Intel MLK DSS solver. */
#ifdef DSS_SOLVER

void solver_message()
{
    fprintf(stdout, "Using Intel MKL/DSS linear algebra package.\n");
}

#include "solver_dss.c"
#endif

/* KLU (SuiteSparse) solver. */
#ifdef KLU_SOLVER

void solver_message()
{
    fprintf(stdout,
        "Using SuiteSparse linear algebra package by Tim Davis, Texas A&M.\n");
}

#include "solver_klu.c"
#endif

#ifdef SOLVER_HASH

#define ST_MAX_DENS     5
#define ST_START_MASK   31

static unsigned int number_hash(unsigned int x, unsigned int hashmask)
{
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return (x & hashmask);
}

/*
** Initialize the hash tables.
*/
void htab_init(spHtab *h, unsigned int sz)
{
    if (h) {
        unsigned int i;
        unsigned int msk = 1;
        /* guessing rows are 1% nonzero */
        unsigned int est = sz/(ST_MAX_DENS*100);
        while (msk < est)
            msk <<= 1;
        msk--;
        if (msk < ST_START_MASK)
            msk = ST_START_MASK;

        h->cols = (rowTab*)malloc(sz*sizeof(rowTab));
        for (i = 0; i < sz; i++) {
            rowTab *t = h->cols + i;
            t->mask = msk;
            t->allocated = 0;
            t->entries = (matData**)calloc(t->mask+1, sizeof(matData*));
        }
        h->numcols = sz;
        h->allocated = 0;
        h->recalls = 0;
        h->ecount = 0;
        h->eblocks = 0;
    }
}

/*
** Function to increase the hash width by one bit.
*/
static void rehash(rowTab *t)
{
    if (t) {
        unsigned int i, oldmask = t->mask;
        t->mask = (oldmask << 1) | 1;
        matData **oldent = t->entries;
        t->entries = (matData**)calloc(t->mask+1, sizeof(matData*));
        for (i = 0; i <= t->mask; i++)
            t->entries[i] = 0;
        for (i = 0; i <= oldmask; i++) {
            matData *e, *en;
            for (e = oldent[i]; e; e = en) {
                en = e->next;
                unsigned int j = number_hash(e->row, t->mask);
                e->next = t->entries[j];
                t->entries[j] = e;
            }
        }
        if (oldent)
            free(oldent);
    }
}

/*
** Allocator for matData.
*/
static matData *newelt(spHtab *h, int row, int col)
{
    if (!h->eblocks || h->ecount == SP_HBLKSZ) {
        spHeltBlk *hb = (spHeltBlk*)calloc(1, sizeof(spHeltBlk));
        hb->next = h->eblocks;
        h->eblocks = hb;
        h->ecount = 0;
    }
    matData *e = h->eblocks->elts + h->ecount++;
    e->row = row;
    e->col = col;
    e->real = 0.0;
    e->imag = 0.0;
    e->next = 0;
    return (e);
};

/*
** Return the matrix element i,j, creating if necessary.
*/
matData *htab_get(spHtab *h, int row, int col)
{
    if (h) {
        rowTab *t = h->cols + col;
        unsigned int i = number_hash(row, t->mask);
        matData *e;
        for (e = t->entries[i]; e; e = e->next) {
            if (e->row == row) {
                h->recalls++;
                return (e);
            }
        }
        e = newelt(h, row, col);
        e->next = t->entries[i];
        t->entries[i] = e;
        t->allocated++;
        h->allocated++;
        if (t->allocated/(t->mask + 1) > ST_MAX_DENS)
            rehash(t);
        return (e);
    }
    return (0);
}

/*
** Import row data.  If col < 0, this is a flag that the matrix is
** symmetric and we are keeping only one copy of the items.
*/
void htab_set_row_elts(spHtab *h, int col, matData *data, int dsz)
{
    if (h) {
        int sym = 0;
        if (dsz < 0) {
            sym = 1;
            dsz = -dsz;
        }
        rowTab *t = h->cols + col;
        int k;
        for (k = 0; k < dsz; k++) {
            int row = data[k].row - 1;
            if (sym && (row < col))
                continue;
            unsigned int i = number_hash(row, t->mask);
            matData *e;
            for (e = t->entries[i]; e; e = e->next) {
                if (e->row == row) {
                    e->real += data[k].real;
                    e->imag += data[k].imag;
                    h->recalls++;
                    break;
                }
            }
            if (!e) {
                e = newelt(h, row, col);
                e->real = data[k].real;
                e->imag = data[k].imag;
                e->next = t->entries[i];
                t->entries[i] = e;
                t->allocated++;
                h->allocated++;
                if (t->allocated/(t->mask + 1) > ST_MAX_DENS)
                    rehash(t);
            }
        }
    }
}

static int eltsort(void *pe1, const void *pe2)
{
    matData *e1 = *(matData**)pe1;
    matData *e2 = *(matData**)pe2;
    if (e1->row < e2->row)
        return (-1);
    return ((e1->row == e2->row) ? 0 : 1);
}

static matData *sortlist(matData *list, matData **last)
{
    unsigned int sz = 0;
    matData *p, **ary;
    unsigned int i;

    for (p = list; p; p = p->next)
        sz++;
    if (sz < 2) {
        if (last)
            *last = list;
        return (list);
    }
    ary = (matData**)malloc(sz*sizeof(matData*));
    sz = 0;
    for (p = list; p; p = p->next)
        ary[sz++] = p;

    qsort(ary, sz, sizeof(matData*), (int(*)(const void*, const void*))eltsort);
    for (i = 1; i < sz; i++)
        ary[i-1]->next = ary[i];
    ary[sz-1]->next = 0;
    if (last)
        *last = ary[sz-1];
    list = ary[0];
    free(ary);
    return (list);
}

/* Return all matrix elements in a linked list, sorted ascending in
** col then row.
** THIS CLEARS THE TABLES.
*/
matData *htab_list(spHtab *h)
{
    if (h) {
        matData *pstart = 0;
        matData *plink = 0;
        unsigned int c;
        unsigned int i;
        for (c = 0; c < h->numcols; c++) {
            rowTab *t = h->cols + c;
            matData *ptmp = 0, *pend;
            for (i = 0; i <= t->mask; i++) {
                matData *e;
                for (e = t->entries[i]; e; e = pend) {
                    pend = e->next;
                    e->next = ptmp;
                    ptmp = e;
                }
            }
            free(t->entries);
            t->entries = 0;
            t->allocated = 0;
            ptmp = sortlist(ptmp, &pend);
            if (!pstart)
                pstart = ptmp;
            else
                plink->next = ptmp;
            plink = pend;
        }
        free(h->cols);
        h->cols = 0;
        return (pstart);
    }
    return (0);
}

#endif

