
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
/*
  This is derived from code contributed by:
  Greg Rollins 10-10-2018,
  Synopsys Corp  Mountian View CA.

  For the DSS functions in the Intel MKL.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mkl.h"


typedef struct
{
    _MKL_DSS_HANDLE_t handle;
#ifdef SOLVER_HASH
    spHtab Htab;
#else
    matData **cols;
#endif
    double *t1;
    double *t2;
    int Complex;
    int PureSC;
    int Size;
    int NumElts;
} dmatrix;


char *matCreate(int size, int cplx, int *err)
{
    dmatrix *m = calloc(1, sizeof(dmatrix));
    *err = 0;
#ifdef DEBUG
    printf("matCreate(%d, %d, %lx)\n", size, cplx, err);
#endif
    m->Size = size;
    m->Complex = cplx;
    return (char*)m;
}

void matSetPureSC(void *c, int puresc)
{
    /* If puresc is nonzero, the problem contains lossless
     * superconductors only which allows use of a real matrix, which
     * may be faster.
     */

    if (puresc) {
        dmatrix *m = (dmatrix*)c;
        m->PureSC = 1;
        m->Complex = 0;
    }
}

void matSolve(void *c, matREAL *a, matREAL *b)
{
    dmatrix *m = (dmatrix*)c;
    _INTEGER_t opt = MKL_DSS_DEFAULTS;
    _INTEGER_t error;
    int i, nrhs=1;
#ifdef DEBUG
    printf("matSolve(%lx, %lx, %lx)\n", c, a, b);
#endif
    if (m->Complex) {
        error = dss_solve_complex(m->handle, opt, a, nrhs, m->t2);
        if (error != MKL_DSS_SUCCESS) {
            printf("dss_solve_complex failed\n");
            exit(1);
        }
        memcpy(b, m->t2, 2*m->Size*sizeof(double));
    }
    else if (m->PureSC) {
        for (i = 0; i < m->Size; i++) {
            m->t1[i] = a[i*2];
        }
        error = dss_solve_real(m->handle, opt, m->t1, nrhs, m->t2);
        if (error != MKL_DSS_SUCCESS) {
            printf("dss_solve_real failed\n");
            exit(1);
        }
        for (i = 0; i < m->Size; i++) {
            b[i*2] = 0.0;
            b[i*2+1] = m->t2[i];
        }
    }
    else {
        for (i = 0; i < m->Size; i++) {
            m->t1[i] = a[i*2];
        }
        error = dss_solve_real(m->handle, opt, m->t1, nrhs, m->t2);
        if (error != MKL_DSS_SUCCESS) {
            printf("dss_solve_real failed\n");
            exit(1);
        }
        for (i = 0; i < m->Size; i++) {
            b[i*2] = m->t2[i];
            b[i*2+1] = 0.0;
        }
    }
}

int matOrderAndFactor(void *c , matREAL *a, matREAL b, matREAL cc, int n)
{
#ifdef DEBUG
    printf("matOrderAndFactor(%lx. %lx, %g, %g, %d)\n", c, a, b, cc, n);
#endif
    matFactor(c);
    return 0;
}

int matFactor(void* c)
{
    dmatrix *m = (dmatrix*)c;
#ifdef SOLVER_HASH
    m->NumElts = m->Htab.allocated;
#endif
    int *ia = calloc(m->NumElts, sizeof(int));
    double *ar;
    if (m->Complex) {
        m->t1 = 0;
        m->t2 = calloc(2*m->Size, sizeof(double));
        ar = calloc(m->NumElts, 2*sizeof(double));
    }
    else {
        m->t1 = calloc(m->Size, sizeof(double));
        m->t2 = calloc(m->Size, sizeof(double));
        ar = calloc(m->NumElts, sizeof(double));
    }
    int *ja = calloc(m->Size+1, sizeof(int));
    _INTEGER_t opt, error;
#ifdef DEBUG
    printf("matFactor(%lx)\n", c);
#endif
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
            ja[lastcol] = elcnt;
        }
        ia[elcnt] = p->row;
        if (m->Complex) {
            ar[2*elcnt] = p->real;
            ar[2*elcnt + 1] = p->imag;
        }
        else if (m->PureSC) {
            /* For superconductors matrix is purely imaginary. */
            ar[elcnt] = p->imag;
        }
        else {
            ar[elcnt] = p->real;
        }
        elcnt++;
    }
    ja[m->Size] = elcnt;

    /* Clean up hash entries, we're done with them. */
    while (m->Htab.eblocks) {
        spHeltBlk *bx = m->Htab.eblocks;
        m->Htab.eblocks = m->Htab.eblocks->next;
        free (bx);
    }
    m->Htab.ecount = 0;
    
#else
    int i, j, k=0;
    matData *pnxt = 0, *p;
    for (i = 0; i < m->Size; i++) {
        for (p = m->cols[i]; p; p = pnxt) {
            pnxt = p->next;
            ia[k] = p->row;
            if (m->Complex) {
                ar[2*k] = p->real;
                ar[2*k+1] = p->imag;
            }
            else if (m->PureSC) {
                /* For superconductors matrix is purely imaginary. */
                ar[k] =  p->imag;
            }
            else {
                ar[2*k] = p->real;
            }
#ifdef DEBUG
/*            printf("A %d %d  = %g %g\n", i , p->row , p->real,  p->imag); */
#endif
            free(p);
            k++;
        }
        ja[i+1] = k;
        m->cols[i] = 0;
    }
#endif
#ifdef DEBUG
    printf("matFactor: non_zero=%d\n", m->NumElts);
#endif
     
    opt = MKL_DSS_MSG_LVL_WARNING + MKL_DSS_TERM_LVL_ERROR +
        MKL_DSS_ZERO_BASED_INDEXING;
    error = dss_create(m->handle, opt);
    if (error != MKL_DSS_SUCCESS) {
        printf("dss_create failed\n");
        exit(1);
    }
     
    if (m->Complex)
        opt = MKL_DSS_SYMMETRIC_COMPLEX;
    else 
        opt = MKL_DSS_SYMMETRIC;
    error = dss_define_structure(m->handle, opt, ja, m->Size, m->Size, ia,
        m->NumElts);
    if (error != MKL_DSS_SUCCESS) {
        printf("dss_define_structure failed\n");
        exit(1);
    }

    opt = MKL_DSS_AUTO_ORDER;
    error = dss_reorder(m->handle, opt, 0);
    if (error != MKL_DSS_SUCCESS) {
        printf("dss_reorder failed\n");
        exit(1);
    }
     
    opt = MKL_DSS_INDEFINITE ;
    if (m->Complex) {
        error = dss_factor_complex(m->handle, opt, ar);
        if (error != MKL_DSS_SUCCESS) {
            printf("dss_factor_complex failed\n");
            exit(1);
        }
    }
    else {
        error = dss_factor_real(m->handle, opt, ar);
        if (error != MKL_DSS_SUCCESS) {
            printf("dss_factor_real failed\n");
            exit(1);
        }
    }
    return 0;
}

void matClear(void* c)
{
    dmatrix *m = (dmatrix*)c;
#ifdef DEBUG
    printf("matClear\n");
#endif
    if (m->handle != 0) {
        _INTEGER_t opt = MKL_DSS_MSG_LVL_WARNING + MKL_DSS_TERM_LVL_ERROR;
        _INTEGER_t error = dss_delete(m->handle, opt);
        if (error != MKL_DSS_SUCCESS) {
            printf("dss_delete failed\n");
            exit(1);
        }
        m->handle = 0;
    }
#ifdef SOLVER_HASH
    htab_init(&m->Htab, m->Size);
#else
    if (!m->cols)
        m->cols = calloc(m->Size, sizeof(matData*));
    else
        memset(m->cols, 0, m->Size*sizeof(matData*));
#endif

    m->NumElts = 0;
    if (m->t1)
        memset(m->t1, 0, m->Size*sizeof(double));
    if (m->t2)
        memset(m->t2, 0, m->Size*sizeof(double));
}


matREAL *matGetElement(void *c, int i1, int j1)
{
    dmatrix *m = (dmatrix*)c;
    static double junk[2];
    if (i1 > j1)
        return junk; /* Matrix is symetric trash this entry. */
    int i=i1-1, j=j1-1;
#ifdef DEBUG
/*    printf("matGetElement(%d, %d)\n", i, j); */
#endif
#ifdef SOLVER_HASH
    matData *p = htab_get(&m->Htab, j, i);
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

void
matSetRowElements(void *c, int i1, matData* co, int j1)
{
    dmatrix *m = (dmatrix*)c;
    int i = i1 - 1;
#ifdef SOLVER_HASH
    /* passing -j1 signals assumption of a symmetric matrix and
     * filtering of duplicate entries.  */
    htab_set_row_elts(&m->Htab, i, co, -j1);
#else
    matData *pp = NULL, *p = m->cols[i];
  
    int j;
    for (j = 0; j < j1; j++) {
        int r = co[j].row - 1;
        if (r < i)
            continue;
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
    dmatrix *m = (dmatrix*)c;
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

int matElementCount(void *c)
{
    dmatrix *m = (dmatrix*)c;
#ifdef DEBUG
    printf("matElementCount(%lx)\n", c);
#endif
    return m->NumElts;
}

int matFillinCount(void *c)
{
#ifdef DEBUG
    printf("matFillinCount(%lx)\n", c);
#endif
    return 0;
}

int matFileMatrix(void *Matrix, const char *outfname, const char *cc,
    int i, int j, int k)
{
    printf("matFileMatrix not supported for MKL\n");
    exit(1);
    return 0;
}

