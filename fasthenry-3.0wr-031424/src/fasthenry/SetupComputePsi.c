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
/* this sets up the vectors to call Fastcap's ComputePsi */
/* It will be called twice for each coordinate direction.  Once for real
   and once for imaginary */

#include "induct.h"
#include "solver.h"

/* SRW */
void SetupComputePsi(CX*, ssystem*, CX*, int, charge*, double, double*, SYS*);
void realmatCXvec(CX*, double**, CX*, int);
void fixEvalDirect(charge**, int, int*, charge**, int, double**);


/* Vs will contain the result,  Im is the 'q',  Size is the size of vectors. */
/* This will alter Im.  Im = Precond*Im */
void SetupComputePsi(CX *Vs, ssystem *sys, CX *Im, int size, charge *chglist,
    double w, double *R, SYS *indsys)
/* double w;  radian frequency */
/* double *R; resistance vector */
{
  /*
  extern double dirtime;
  double dtime;
  */
  double *q, *p;
  static CX *Ib = NULL, *Vb = NULL, *ctemp;
  int branches;
  CX temp;
  MELEMENT *mtemp;
  charge *chg;
  int i;
  double rtemp;
  MELEMENT **Mtrans, **Mlist;
#ifdef NODEBUG
  static CX *Vdirect = NULL;
  double maxdiff,pdiff;
  int maxindx;
#endif
#if OPCNT == ON
  int ind_opcnt_mult = 0, ind_opcnt_real = 0;
#endif

  branches = indsys->num_fils;
  Mtrans = indsys->Mtrans;
  Mlist = indsys->Mlist;

  if (Ib == NULL) {
    Ib = (CX *)MattAlloc(branches, sizeof(CX));
    Vb = (CX *)MattAlloc(branches, sizeof(CX));
    ctemp = (CX *)MattAlloc(size, sizeof(CX));
#ifdef NODEBUG
    Vdirect = (CX *)MattAlloc(branches, sizeof(CX));
#endif
  }

  for(i = 0; i < branches; i++)
    Vb[i] = CXZERO;

  q = sys->q;
  p = sys->p;
  ASSERT(size == indsys->num_mesh);

  if (indsys->precond_type == LOC) {
    multPrecond(indsys->Precond, Im, ctemp, size);
    for(i = 0; i < size; i++)
      Im[i] = ctemp[i];
  }
  else if (indsys->precond_type == SPARSE) 
    matSolve(indsys->sparMatrix, (matREAL*)Im, (matREAL*)Im);

  /* do  Ib = Mtrans*Im */
  for(i = 0; i < branches; i++) {
    Ib[i] = CXZERO;
    for(mtemp = Mtrans[i]; mtemp != NULL; mtemp = mtemp->mnext) {
      if (mtemp->sign == 1) 
        cx_add(Ib[i], Ib[i], Im[mtemp->filindex]);
      else
        cx_sub(Ib[i], Ib[i], Im[mtemp->filindex]);
    }
  }

  /* Evaluate M*L*Mt*Im = M*L*Ib using the multipole algorithm */

  /* Do all of the non-direct parts first */
  sys->DirectEval = FALSE;
  for(i = 0; i < 3; i++) {  /* for each of the coordinate directions */

    /* do the real part */
    for(chg = chglist; chg != NULL; chg = chg->next) {
      /* fill the pseudo-charge vector */
      q[chg->index] = Ib[chg->fil->filnumber].real*chg->fil->lenvect[i];
#if OPCNT == ON
      ind_opcnt_mult++;
#endif

    }
    computePsi(sys, q, p, branches, chglist);
    for(chg = chglist; chg != NULL; chg = chg->next) {
      /* add potential due to i direction */
      Vb[chg->fil->filnumber].real += p[chg->index]*chg->fil->lenvect[i]*MUOVER4PI;
#if OPCNT == ON
      ind_opcnt_mult++;
#endif
    }

    /* do the imaginary part */
    for(chg = chglist; chg != NULL; chg = chg->next) {
      /* fill the pseudo-charge vector */
      q[chg->index] = Ib[chg->fil->filnumber].imag*chg->fil->lenvect[i];
#if OPCNT == ON
      ind_opcnt_mult++;
#endif
    }
    computePsi(sys, q, p, branches, chglist);
    for(chg = chglist; chg != NULL; chg = chg->next) {
      /* add potential due to i direction */
      Vb[chg->fil->filnumber].imag += p[chg->index]*chg->fil->lenvect[i]*MUOVER4PI;
#if OPCNT == ON
      ind_opcnt_mult++;
#endif
    }
    
  }

  /* do the direct parts */
  sys->DirectEval = TRUE;

  /* do the real part of the Direct part */
  for(i = 1; i <= branches; i++)
    p[i] = 0;
  for(chg = chglist; chg != NULL; chg = chg->next) 
    /* fill the pseudo-charge vector */
    q[chg->index] = Ib[chg->fil->filnumber].real;

  /* starttimer(&dtime); */
  mulDirect(sys);
  mulEval(sys);
  /* stoptimer(&dtime);
  dirtime += dtime;
  */

  for(chg = chglist; chg != NULL; chg = chg->next) {
    /* add potential due to i direction */
    Vb[chg->fil->filnumber].real += p[chg->index];
  }
  
  /* do the imaginary part of the Direct part */
  for(i = 1; i <= branches; i++)
    p[i] = 0;
  for(chg = chglist; chg != NULL; chg = chg->next)
    /* fill the pseudo-charge vector */
    q[chg->index] = Ib[chg->fil->filnumber].imag;

  /* starttimer(&dtime); */
  mulDirect(sys);
  mulEval(sys);
  /* stoptimer(&dtime);
  dirtime += dtime;
  */

  for(chg = chglist; chg != NULL; chg = chg->next) { 
    /* add potential due to i direction */
    Vb[chg->fil->filnumber].imag += p[chg->index];
  }

  /* do Vs = M*Vb*jw */
  for(i = 0; i < size; i++) {
    Vs[i] = CXZERO;
    for(mtemp = Mlist[i]; mtemp != NULL; mtemp = mtemp->mnext)
      if (mtemp->sign == 1) 
        cx_add(Vs[i], Vs[i], Vb[mtemp->filindex]);
      else
        cx_sub(Vs[i], Vs[i], Vb[mtemp->filindex]);

    /* multiply by jw */
    rtemp = -Vs[i].imag*w;
    Vs[i].imag = Vs[i].real*w;
    Vs[i].real = rtemp;
  }

  /* add in M*R*Mt*Im = M*R*Ib */
  for(i = 0; i < size; i++) {
    for(mtemp = Mlist[i]; mtemp != NULL; mtemp = mtemp->mnext) {
     cx_scalar_mult(temp, mtemp->sign*R[mtemp->filindex], Ib[mtemp->filindex]);
     cx_add(Vs[i], Vs[i], temp);
#if OPCNT == ON
      ind_opcnt_mult+=2;
      ind_opcnt_real+=2;
#endif
    }
  }

#if OPCNT == ON
  printf("Inductance (mesh to branch) mults: %d\n",ind_opcnt_mult);
  printf("Just doing MRMtIm: %d\n",ind_opcnt_real);
  printops();
  exit(0);
#endif

#ifdef NODEBUG
  /* for debugging, compare to direct Vb = ZM Ib */
  realmatCXvec(Vdirect, indsys->Z, Ib, branches);
  maxdiff = 0; 
  maxindx = 0;
  for(i = 0; i < branches; i++) {
    if (cx_abs(Vb[i]) > 1e-23) {
      cx_sub(temp, Vdirect[i], Vb[i]);
      pdiff = cx_abs(temp )/cx_abs(Vb[i]) ;
    }
    else
      pdiff = cx_abs(Vb[i]);

    if (pdiff > maxdiff) {
      maxdiff = pdiff;
      maxindx = i;
    }
  }
  if (maxdiff < .3)
    printf("maxdiff: %g  Vb[%d]=%g  Vdirect[%d]=%g\n",
           maxdiff,maxindx,cx_abs(Vb[maxindx]),maxindx,cx_abs(Vdirect[maxindx]));
  else
    printf("***maxdiff: %g  Vb[%d]=%g  Vdirect[%d]=%g***\n",
           maxdiff,maxindx,cx_abs(Vb[maxindx]),maxindx,cx_abs(Vdirect[maxindx]));    


#endif
}

void realmatCXvec(CX *y, double **A, CX *x, int size)
{
  int i, j;
  CX temp;

  for (i = 0; i < size; i++) {
    y[i] = CXZERO;
    for(j = 0; j < size; j++) {
      cx_scalar_mult(temp, A[i][j], x[j]);
      cx_add(y[i], y[i], temp);
    }
  }
}

/* this function fixes Eval matrices which are computed directly */
/* This is necessary since direct mutual terms are not componentwise,
   but the multipole routines are called once for each component direction.
   Basically, componentwise multiplication will cause the elements
   to be multiplied by the dot product of the fil->lenvect vectors of 
   the two filaments.  This will divide that product out.  Also, MUOVER4PI
   must also be divided out 
*/

void fixEvalDirect(charge **qchgs, int numqchgs, int *is_dummy,
    charge **pchgs, int numpchgs, double **mat)
{
  int i,j, k;
  double dotprod, magi, magj;
  double *lenvecti, *lenvectj;

  for(i = 0; i < numpchgs; i++) {
    lenvecti = pchgs[i]->fil->lenvect;
    magi = 0;
    for(k = 0; k < 3; k++)
      magi += lenvecti[k]*lenvecti[k];
    for(j = 0; j < numqchgs; j++) {
      lenvectj = qchgs[j]->fil->lenvect;
      magj = dotprod = 0;
      for(k = 0; k < 3; k++) {
        magj += lenvectj[k]*lenvectj[k];
        dotprod += lenvecti[k]*lenvectj[k];
      }
      if (fabs(dotprod)/sqrt(magi*magj) > EPS) /* filaments aren't perpendicular */
        mat[i][j] = mat[i][j]/(dotprod*MUOVER4PI);
      else { /* if they are, mat[i][j] == 0.0, hopefully */
        if (mat[i][j] != 0.0)
          printf("Warning: dot product = %lg < EPS, but mat[i][j] = %lg\n",dotprod, mat[i][j]);
      }
    }
  }
}
