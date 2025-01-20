/*!\page LICENSE LICENSE

Copyright (C) 2003 by the Board of Trustees of Massachusetts Institute of
Technology, hereafter designated as the Copyright Owners.

License to use, copy, modify, sell and/or distribute this software and
its documentation for any purpose is hereby granted without royalty,
subject to the following terms and conditions:

1.  The above copyright notice and this permission notice must
appear in all copies of the software and related documentation.

2.  The names of the Copyright Owners may not be used in advertising or
publicity pertaining to distribution of the software without the specific,
prior written permission of the Copyright Owners.

3.  THE SOFTWARE IS PROVIDED "AS-IS" AND THE COPYRIGHT OWNERS MAKE NO
REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED, BY WAY OF EXAMPLE, BUT NOT
LIMITATION.  THE COPYRIGHT OWNERS MAKE NO REPRESENTATIONS OR WARRANTIES OF
MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF THE
SOFTWARE WILL NOT INFRINGE ANY PATENTS, COPYRIGHTS TRADEMARKS OR OTHER
RIGHTS. THE COPYRIGHT OWNERS SHALL NOT BE LIABLE FOR ANY LIABILITY OR DAMAGES
WITH RESPECT TO ANY CLAIM BY LICENSEE OR ANY THIRD PARTY ON ACCOUNT OF, OR
ARISING FROM THE LICENSE, OR ANY SUBLICENSE OR USE OF THE SOFTWARE OR ANY
SERVICE OR SUPPORT.

LICENSEE shall indemnify, hold harmless and defend the Copyright Owners and
their trustees, officers, employees, students and agents against any and all
claims arising out of the exercise of any rights under this Agreement,
including, without limiting the generality of the foregoing, against any
damages, losses or liabilities whatsoever with respect to death or injury to
person or damage to property arising from or out of the possession, use, or
operation of Software or Licensed Program(s) by LICENSEE or its customers.

*/
/* # ***** sort to /src/main
   # ***** */
#include "mulGlobal.h"

#ifdef WITH_AVX
#include "simdmac.c"
#endif

#if OPCNT == ON
static directops=0, upops=0, downops=0, evalops=0, evaldops=0, evalmops=0;
#endif

/* SRW */
void mulDirect(ssystem*);
void mulPrecond(ssystem*, int);
void mulUp(ssystem*);
void mulEval(ssystem*);
void mulDown(ssystem*);
#if OPCNT == ON
void printops(void);
#endif


/* 
Compute the direct piece. 
*/
void mulDirect(ssystem *sys)
{
  /* SRW -- modified for speed. */
  cube *nextc;

/* Assumes the potential vector has been zero'd!!!! */
  for(nextc=sys->directlist; nextc != NULL; nextc = nextc->dnext) {
    int dsize = nextc->directnumeles[0];  /* Equals number of charges. */
    double *q = nextc->directq[0];   
    double *p = nextc->eval;
    int *is_dummy = nextc->nbr_is_dummy[0];
    int *is_dielec = nextc->is_dielec;
  /* Inside Cube piece. */
    double **mat = nextc->directmats[0];
    double *qxx = (double*)malloc(dsize*sizeof(double));
    int j;
    for (j = 0; j < dsize; j++)
      qxx[j] = is_dummy[j] ? 0.0 : q[j];
    for(j = dsize - 1; j >= 0; j--) {
#if NUMDPT == 2
      if(is_dielec[j]) continue;
#endif
#ifdef WITH_AVX
      p[j] += simd_mac(mat[j], qxx, dsize);
#else
      double *matj = mat[j];
      double *qp = qxx;
      int k;
      for(k = 0; k < dsize; k++)
        /* p[j] += mat[j][k] * qxx[k]; */
        p[j] += (*matj++) * (*qp++);
#endif
#if OPCNT == ON
      directops += dsize;
#endif
    }
    free(qxx);
  /* Through all nearest nbrs. */
    int i;
    for(i=nextc->directnumvects - 1; i > 0; i--) {
      int dsizei = nextc->directnumeles[i];
      mat = nextc->directmats[i];
      double *qn = nextc->directq[i];
      qxx = (double*)malloc(dsizei*sizeof(double));
      is_dummy = nextc->nbr_is_dummy[i];
      for (j = 0; j < dsizei; j++)
        qxx[j] = is_dummy[j] ? 0.0 : qn[j];
      for(j = dsize - 1; j >= 0; j--) {
#if NUMDPT == 2
        if(is_dielec[j]) continue;
#endif
#ifdef WITH_AVX
        p[j] += simd_mac(mat[j], qxx, dsizei);
#else
        double *matj = mat[j];
        double *qp = qxx;
        int k;
        for(k = 0; k < dsizei; k++)
          /* p[j] += mat[j][k] * qxx[k]; */
          p[j] += (*matj++) * (*qp++);
#endif
#if OPCNT == ON
        directops += dsizei;
#endif
      }
      free(qxx);
    }
  }
}

/*
Block diagonal or Overlapped Preconditioner.
*/
void mulPrecond(ssystem *sys, int type)
{
  int i, j, k, dsize, *is_dummy;
  double *p, *q, *qn, **mat;
  cube *nc;

  if(type == BD) {
    for(nc=sys->precondlist; nc != NULL; nc = nc->pnext) {
      solve(nc->precond, nc->prevectq, nc->prevectq, nc->presize);
    }
  }
  else {
    /* Assumes the potential vector has been zero'd!!!! */
    for(nc=sys->directlist; nc != NULL; nc = nc->dnext) {
      dsize = nc->directnumeles[0];  /* Equals number of charges. */
      q = nc->directq[0];   
      p = nc->eval;
      is_dummy = nc->nbr_is_dummy[0];
      /* Inside Cube piece. */
      mat = nc->precondmats[0];
      for(j = dsize - 1; j >= 0; j--) {
        for(k = dsize - 1; k >= 0; k--) {
          if(!is_dummy[k]) p[j] += mat[j][k] * q[k];
        }
      }
      /* Through all nearest nbrs. */
      for(i=nc->directnumvects - 1; i > 0; i--) {
        mat = nc->precondmats[i];
        is_dummy = nc->nbr_is_dummy[i];
        if(mat != NULL) {
          qn = nc->directq[i];
          for(j = dsize - 1; j >= 0; j--) {
            for(k = nc->directnumeles[i] - 1; k >= 0; k--) {
              if(!is_dummy[k]) p[j] += mat[j][k] * qn[k];
            }
          }
        }
      }
    }
    /* Copy ps back to qs and zero ps. */
    for(nc=sys->directlist; nc != NULL; nc = nc->dnext) {
      dsize = nc->directnumeles[0];  /* Equals number of charges. */
      q = nc->directq[0];   
      p = nc->eval;
      for(j = dsize - 1; j >= 0; j--) {
        q[j] = p[j];
        p[j] = 0.0;
      }
    }
  }
}

/* 
Loop through upward pass. 
*/
void mulUp(ssystem *sys)
{
int i, j, k, l;
int msize;
double *multi, *rhs, **mat;
cube *nextc;

  if(sys->depth < 2) return;    /* ret if upward pass not possible/worth it */

/* Through all the depths, starting from the bottom and not doing top. */
  for(i = sys->depth; i > 0; i--) {  
  /* Through all the cubes at depth. */
    for(nextc=sys->multilist[i]; nextc != NULL; nextc = nextc->mnext) {
      msize = nextc->multisize;
      multi = nextc->multi;
      for(j=0; j < msize; j++) multi[j] = 0;
    /* Through all the nonempty children of cube. */
      for(j=nextc->upnumvects - 1; j >= 0; j--) {
        mat = nextc->upmats[j];
        rhs = nextc->upvects[j];
        for(k = nextc->upnumeles[j] - 1; k >= 0; k--) {
          for(l = msize - 1; l >= 0; l--) {
            multi[l] += mat[l][k] * rhs[k];
#if OPCNT == ON
            upops++;
#endif
          }
        }
      }
    }
  }
}

/*
  evaluation pass - use after mulDown or alone. 
*/
void mulEval(ssystem *sys)
{
  /* SRW -- modified for speed. */
  cube *nc;

  if(sys->depth < 2) return;    /* ret if upward pass not possible/worth it */

  for(nc = sys->directlist; nc != NULL; nc = nc->dnext) {
    int size = nc->upnumeles[0];     /* number of eval pnts (chgs) in cube */
    double *eval = nc->eval;         /* vector of evaluation pnt potentials */
    int *is_dielec = nc->is_dielec;  /* vector of DIELEC/BOTH panel flags */

    /* do the evaluations */
    int i;
    for(i = nc->evalnumvects - 1; i >= 0; i--) {
      if (nc->eval_isQ2P[i] == sys->DirectEval) {   /* added 12/92  MK */
        double **mat = nc->evalmats[i];
        double *vec = nc->evalvects[i];
        int nume = nc->evalnumeles[i];
        int j;
        for(j = size - 1; j >= 0; j--) {
#if NUMDPT == 2
          if(is_dielec[j]) continue;
#endif
#ifdef WITH_AVX
          eval[j] += simd_mac(mat[j], vec, nume);
#else
          double *matj = mat[j];
          double *vc = vec;
          int k;
          for(k = 0; k < nume; k++)
            /* eval[j] += mat[j][k] * vec[k]; */
            eval[j] += (*matj++) * (*vc++);
#endif
#if OPCNT == ON
          evalops += nume;
          if (sys->DirectEval == TRUE)
            evaldops += nume;
          else if (sys->DirectEval == FALSE)
            evalmops += nume;
          else
            {printf("huh?"); exit(1);}
#endif
        }
      }
    }
  }
}

/* 
Loop through downward pass. 
*/
void mulDown(ssystem *sys)
{
  /* SRW -- modified for speed. */
  cube *nc;
  int depth;

  if(sys->depth < 2) return;    /* ret if upward pass not possible/worth it */

  for(depth=2; depth <= sys->depth; depth++) {
    for(nc=sys->locallist[depth]; nc != NULL; nc = nc->lnext) {
      int lsize = nc->localsize;
      double *local = nc->local;
      int j;
      for(j=0; j < lsize; j++) local[j] = 0;
  /* Through all the locals for the cube. */
      int i;
      for(i=nc->downnumvects - 1; i >= 0; i--) {
        double **mat = nc->downmats[i];
        double *rhs = nc->downvects[i];
        int nume = nc->downnumeles[i];
        int j;
        for(j = lsize - 1; j >= 0; j--) {
#ifdef WITH_AVX
          local[j] += simd_mac(mat[j], rhs, nume);
#else
          double *matj = mat[j];
          double *r = rhs;
          int k;
          for(k = 0; k < nume; k++) {
            /* local[j] += mat[j][k] * rhs[k]; */
            local[j] += (*matj++) * (*r++);
          }
#endif
#if OPCNT == ON
          downops += nume;
#endif
        }
      }
    }
  }
}


#if OPCNT == ON
void printops(void)
{
  printf("Number of Direct Multi-Adds = %d\n", directops);
  printf("Number of Upward Pass Multi-Adds = %d\n", upops);
  printf("Number of Downward Pass Multi-Adds = %d\n", downops);
  printf("Number of Evaluation Pass Multi-Adds = %d\n", evalops);
  printf("Number of Evaluation Pass Direct Multi-Adds = %d\n", evaldops);
  printf("Number of Evaluation Pass Other Multi-Adds = %d\n", evalmops);
  printf("Total Number of Multi-Adds = %d\n", directops+upops+downops+evalops);
}
#endif

