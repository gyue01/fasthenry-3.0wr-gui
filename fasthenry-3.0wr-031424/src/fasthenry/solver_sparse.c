
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

#include "../sparse/spMatrix.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    void *matrix;
    double *tmp1;
    double *tmp2;
    int size;
    int cplx;
    int puresc;
} spm;

char *matCreate(int size, int cplx, int *err)
{
#ifdef DEBUG
    printf("matCreate(%d, %d, %lx)\n", size, cplx, err);
#endif
    spm *m = calloc(1, sizeof(spm));
    m->matrix = spCreate(size, cplx, err);
    m->size = size;
    m->cplx = cplx;
    return ((char*)m);
}

void matSetPureSC(void *c, int puresc)
{
#ifdef SRW_BATCH_SPARSE
    /* This is available only when using matSetRowElements, where we
     * can intercept and change the element values.
     */

    spm *m = (spm*)c;
    if (puresc) {
        m->puresc = 1;
        m->cplx = 0;
    }
#endif
}

void matSolve(void *c, matREAL *a, matREAL *b)
{
    spm *m = (spm*)c;
#ifdef DEBUG
    printf("matSolve(%lx, %lx, %lx)\n", m, a, b);
#endif
    if (m->puresc) {
        int i;
        if (!m->tmp1) {
            m->tmp1 = calloc(1, m->size*sizeof(double));
            m->tmp2 = calloc(1, m->size*sizeof(double));
        }
        for (i = 0; i < m->size; i++)
            m->tmp1[i] = a[i+i];
        spSolve((char*)m->matrix, m->tmp1, m->tmp2);
        for (i = 0; i < m->size; i++) {
            b[i+i] = 0.0;
            b[i+i+1] = m->tmp2[i];
        }
    }
    else
        spSolve((char*)m->matrix, (spREAL*)a, (spREAL*)b);
}

int matOrderAndFactor(void *c, matREAL *p, matREAL v1, matREAL v2, int i)
{
    spm *m = (spm*)c;
#ifdef DEBUG
    printf("matOrderAndFactor(%lx. %lx, %g, %g, %d)\n", m, p, v1, v2, i);
#endif
    if (m->cplx)
        spSetComplex((char*)m->matrix);
    else
        spSetReal((char*)m->matrix);
    return (spOrderAndFactor((char*)m->matrix, (spREAL*)p, v1, v2, i));
}

int matFactor(void *c)
{
    spm *m = (spm*)c;
#ifdef DEBUG
    printf("matFactor(%lx)\n", m);
#endif
    return (spFactor((char*)m->matrix));
}

void matClear(void *c)
{
    spm *m = (spm*)c;
#ifdef DEBUG
    printf("matClear\n");
#endif
    spClear((char*)m->matrix);
}

matREAL *matGetElement(void *c, int i, int j)
{
    spm *m = (spm*)c;
#ifdef DEBUG
/*    printf("matGetElement(%d, %d)\n", i, j); */
#endif
    return (spGetElement((char*)m->matrix, i, j));
}

void matSetRowElements(void *c, int col, matData *co, int nrows)
{
    spm *m = (spm*)c;
#ifdef DEBUG
/*    printf("matSetRowElements()\n"); */
#endif
    if (m->puresc) {
        /* Using a real matrix, might be faster.  The given real
         * values are all zero.
         */

        int i;
        for (i = 0; i < nrows; i++) {
            co[i].real = co[i].imag;
            co[i].imag = 0.0;
        }
    }
    spSetRowElements((char*)m->matrix, col, (struct spColData*)co, nrows);
}

int matFileMatrix(void *c, const char *s1, const char *s2, int i,
    int j, int k)
{
    spm *m = (spm*)c;
#ifdef DEBUG
    printf("matFileMatrix()\n");
#endif
    return (spFileMatrix((char*)m->matrix, (char*)s1, (char*)s2, i, j, k));
}

int matElementCount(void *c)
{
    spm *m = (spm*)c;
    return (spElementCount((char*)m->matrix));
}

int matFillinCount(void *c)
{
    spm *m = (spm*)c;
    return (spFillinCount((char*)m->matrix));
}

