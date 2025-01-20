
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

#include "klu.h"

#include <string.h>
#include <stdio.h>

enum {AMD, COLAMD};
#define ORDERING AMD

typedef struct
{
#ifdef SOLVER_HASH
    spHtab Htab;
#else
    matData **cols;
#endif
    int *Ap;
    int *Ai;
    double *Ax;

    double *RhsTmp;
    klu_symbolic *Symbolic;
    klu_numeric *Numeric;
    int Complex;
    int PureSC;
    int Size;
    int NumElts;
    klu_common Common;

} kmatrix;

static int status(int s)
{
    if (s == 0 || s == KLU_OK)
        return (0);
    return (1);
}

char *matCreate(int size, int cplx, int *err)
{
#ifdef DEBUG
    printf("matCreate(%d, %d, %lx)\n", size, cplx, err);
#endif
    kmatrix *m = calloc(1, sizeof(kmatrix));
    klu_defaults(&m->Common);
    m->Common.ordering = ORDERING;
    m->Size = size;
    m->Complex = cplx;
    return ((char*)m);
}

void matSetPureSC(void *c, int puresc)
{
    /* If puresc is nonzero, the problem contains lossless
     * superconductors only which allows use of a real matrix, which
     * may be faster.
     */

    if (puresc) {
        kmatrix *m = (kmatrix*)c;
        m->PureSC = 1;
        m->Complex = 0;
    }
}

void matSolve(void *c, matREAL *a, matREAL *b)
{
    kmatrix *m = (kmatrix*)c;
#ifdef DEBUG
    printf("matSolve(%lx, %lx, %lx)\n", c, a, b);
#endif
    int sz = (1 + m->Complex)*m->Size;
    if (m->PureSC) {
        int i;
        if (!m->RhsTmp)
            m->RhsTmp = calloc(1, sz*sizeof(double));
        for (i = 0; i < m->Size; i++)
            m->RhsTmp[i] = a[i+i];
        klu_solve(m->Symbolic, m->Numeric, m->Size, 1, m->RhsTmp, &m->Common);
        for (i = 0; i < m->Size; i++) {
            b[i+i] = 0.0;
            b[i+i+1] = m->RhsTmp[i];
        }
    }
    else if (m->Complex) {
        if (a == b)
            klu_z_solve(m->Symbolic, m->Numeric, m->Size, 1, a, &m->Common);
        else {
            if (!m->RhsTmp)
                m->RhsTmp = calloc(1, sz*sizeof(double));
            memcpy(m->RhsTmp, a, sz*sizeof(double));
            klu_z_solve(m->Symbolic, m->Numeric, m->Size, 1, m->RhsTmp,
                &m->Common);
            memcpy(b, m->RhsTmp, sz*sizeof(double));
        }
    }
    else {
        if (a == b)
            klu_solve(m->Symbolic, m->Numeric, m->Size, 1, a, &m->Common);
        else {
            if (!m->RhsTmp)
                m->RhsTmp = calloc(1, sz*sizeof(double));
            memcpy(m->RhsTmp, a, sz*sizeof(double));
            klu_solve(m->Symbolic, m->Numeric, m->Size, 1, m->RhsTmp,
                &m->Common);
            memcpy(b, m->RhsTmp, sz*sizeof(double));
        }
    }
}

int matOrderAndFactor(void *c, matREAL *p, matREAL v1, matREAL v2, int i)
{
#ifdef DEBUG
    printf("matOrderAndFactor(%lx, %lx, %g, %g, %d)\n", c, p, v1, v2, i);
#endif
    matFactor(c);
    return 0;
}


int matFactor(void *c)
{
    kmatrix *m = (kmatrix*)c;
#ifdef DEBUG
    printf("matFactor(%lx)\n", c);
#endif
#ifdef SOLVER_HASH
    m->NumElts = m->Htab.allocated;
#endif
    m->Ap = calloc(1, (m->Size + 1)*sizeof(int)); 
    m->Ai = calloc(1, m->NumElts*sizeof(int)); 
    m->Ax = calloc(1, (1+m->Complex)*m->NumElts*sizeof(double)); 

#ifdef SOLVER_HASH
    /* Retrieve and sort the hashed entries. */
    matData *list = htab_list(&m->Htab);

    /* Load the KLU matrix. */
    int elcnt = 0;
    int lastcol = -1;
    matData *p;
    for (p = list; p; p = p->next) {
        if (lastcol != p->col) {
            lastcol = p->col;
            m->Ap[lastcol] = elcnt;
        }
        m->Ai[elcnt] = p->row;
        if (m->Complex) {
            m->Ax[2*elcnt] = p->real;
            m->Ax[2*elcnt + 1] = p->imag;
        }
        else if (m->PureSC) {
            /* For superconductors matrix is purely imaginary. */
            m->Ax[elcnt] = p->imag;
        }
        else {
            m->Ax[elcnt] = p->real;
        }
        elcnt++;
    }
    m->Ap[m->Size] = elcnt;

    /* Clean up hash entries, we're done with them. */
    while (m->Htab.eblocks) {
        spHeltBlk *bx = m->Htab.eblocks;
        m->Htab.eblocks = m->Htab.eblocks->next;
        free (bx);
    }
    m->Htab.ecount = 0;
    
#else
    /* Load the KLU matrix. */
    int i, elcnt = 0; 
    for (i = 0; i < m->Size; i++) { 
        matData *p = m->cols[i], *pnxt;
        m->cols[i] = 0;
        m->Ap[i] = elcnt; 
        for ( ; p; p = pnxt) {
            pnxt = p->next;
            m->Ai[elcnt] = p->row;
            if (m->Complex) {
                m->Ax[2*elcnt] = p->real;
                m->Ax[2*elcnt + 1] = p->imag;
            }
            else if (m->PureSC) {
                /* For superconductors matrix is purely imaginary. */
                m->Ax[elcnt] = p->imag;
            }
            else {
                m->Ax[elcnt] = p->real;
            }
            free(p);
            elcnt++;
        }
    }
    m->Ap[m->Size] = elcnt;
#endif

    m->Common.status = KLU_OK;
    klu_free_symbolic(&m->Symbolic, &m->Common);
    m->Symbolic = klu_analyze(m->Size, m->Ap, m->Ai, &m->Common);
    if (m->Complex) {
        klu_z_free_numeric(&m->Numeric, &m->Common);
        m->Numeric = klu_z_factor(m->Ap, m->Ai, m->Ax, m->Symbolic,
            &m->Common);
    }
    else {
        klu_free_numeric(&m->Numeric, &m->Common);
        m->Numeric = klu_factor(m->Ap, m->Ai, m->Ax, m->Symbolic,
            &m->Common);
    }
    return (status(m->Common.status));
}

void matClear(void *c)
{
    kmatrix *m = (kmatrix*)c;
#ifdef DEBUG
    printf("matClear\n");
#endif
    free(m->Ap);
    free(m->Ai);
    free(m->Ax);
#ifdef SOLVER_HASH
    htab_init(&m->Htab, m->Size);
#else
    if (!m->cols)
        m->cols = calloc(m->Size, sizeof(matData*));
    else
        memset(m->cols, 0, m->Size*sizeof(matData*)); 
#endif
}

matREAL *matGetElement(void *c, int i1, int j1)
{
    kmatrix *m = (kmatrix*)c;
    int i=i1-1, j=j1-1;
#ifdef DEBUG
/*    printf("matGetElement(%d, %d)\n", i1, j1); */
#endif
#ifdef SOLVER_HASH
    matData *p = htab_get(&m->Htab, i, j);
    return &(p->real);
#else
    if (m->cols[i] == 0) {
        m->cols[i] = calloc(1, sizeof(matData));
        m->cols[i]->row = j;
        m->NumElts++;
        return &(m->cols[i]->real);
    }
    matData *pp = 0, *p;
    for (p = m->cols[i]; p; p = p->next) {
        if (p->row == j)
            return &(p->real);
        if (p->row > j) {
            if (pp) {
                matData *p1 = calloc(1, sizeof(matData));
                    pp->next = p1;
                    p1->row = j;
                    p1->next = p;
                    m->NumElts++;
                    return &(p1->real);
            }
            else {
                    matData *p1 = calloc(1, sizeof(matData));
                    m->cols[i] = p1;
                    p1->row = j;
                    p1->next = p;
                    m->NumElts++;
                    return &(p1->real);
            }
        }
        pp = p;
    }
    matData *p1 = calloc(1, sizeof(matData));
    pp->next = p1;
    p1->row = j;
    p1->next = 0;
    m->NumElts++;
    return &(p1->real);
#endif
}

void matSetRowElements(void *c, int i1, matData *co, int j1)
{
    kmatrix *m = (kmatrix*)c;
    int j, i = i1 - 1;
#ifdef DEBUG
/*    printf("matSetRowElements\n"); */
#endif
#ifdef SOLVER_HASH
    htab_set_row_elts(&m->Htab, i, co, j1);
#else
    matData *pp = NULL, *p = m->cols[i];
  
    for (j = 0; j < j1; j++) {
        int r = co[j].row - 1;
        for (;;) {
            if (!p) { 
                /* reached end of linked list add a new entry at end */
                p = calloc(1, sizeof(matData));
                m->NumElts++;
                if (!m->cols[i])
                    m->cols[i] = p;
                else
                    pp->next = p;
                p->row = r;
                p->real = co[j].real;
                p->imag = co[j].imag;
                pp = p;
                p = NULL;
            }
            else if (p->row == r) {
                p->real += co[j].real;
                p->imag += co[j].imag;
                pp = p;
                p = p->next;
            }
            else if (p->row > r) {
                /* linked list entry is past this entry */
                matData *p1 = calloc(1, sizeof(matData));
                m->NumElts++;

                p1->real = co[j].real;
                p1->imag = co[j].imag;
                p1->row = r;
                p1->next = p;
                if (!pp)
                    m->cols[i] = p1;
                else
                    pp->next = p1;
                pp = p1;
            }
            else {
                pp = p;
                p = p->next;
                continue;
            }
            break;
        }
    }
#endif
}

void matStats(void *c, unsigned int *sz, unsigned int *nm, unsigned int *rc)
{
    kmatrix *m = (kmatrix*)c;
    if (m) {
        if (sz)
            *sz = m->Size;
        if (nm)
            *nm = m->Htab.allocated;
        if (rc)
            *rc = m->Htab.recalls;
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

int matFileMatrix(void *c, const char *s1, const char *s2, int i,
    int j, int k)
{
    kmatrix *m = (kmatrix*)c;
#ifdef DEBUG
    printf("matFileMatrix\n");
#endif
    return (0);
}

int matElementCount(void *c)
{
    kmatrix *m = (kmatrix*)c;
#ifdef DEBUG
    printf("matElementCount\n");
#endif
    return (m->NumElts);
}

int matFillinCount(void *c)
{
    kmatrix *m = (kmatrix*)c;
#ifdef DEBUG
    printf("matFillinCount\n");
#endif
    return (0);
}

