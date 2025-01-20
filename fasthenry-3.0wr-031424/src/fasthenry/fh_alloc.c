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

/* Memory allocation functions for FastHenry.
 * Contributed by Stephen Whiteley, 11/29/2020 (http://wrcad.com)
 */

#include "mulGlobal.h"

void fh_dump_mem()
{
  fprintf(stdout, "Total memory allocated: %ld kilobytes ", memcount/1024);
  fprintf(stderr, " Q2M  matrix memory allocated: %7.ld kilobytes\n",
    memQ2M/1024);
  memcount = memQ2M;
  fprintf(stderr, " Q2L  matrix memory allocated: %7.ld kilobytes\n",
    memQ2L/1024);
  memcount += memQ2L;
  fprintf(stderr, " Q2P  matrix memory allocated: %7.ld kilobytes\n",
    memQ2P/1024);
  memcount += memQ2P;
  fprintf(stderr, " L2L  matrix memory allocated: %7.ld kilobytes\n",
    memL2L/1024);
  memcount += memL2L;
  fprintf(stderr, " M2M  matrix memory allocated: %7.ld kilobytes\n",
    memM2M/1024);
  memcount += memM2M;
  fprintf(stderr, " M2L  matrix memory allocated: %7.ld kilobytes\n",
    memM2L/1024);
  memcount += memM2L;
  fprintf(stderr, " M2P  matrix memory allocated: %7.ld kilobytes\n",
    memM2P/1024);
  memcount += memM2P;
  fprintf(stderr, " L2P  matrix memory allocated: %7.ld kilobytes\n",
    memL2P/1024);
  memcount += memL2P;
  fprintf(stderr, " Q2PD matrix memory allocated: %7.ld kilobytes\n",
    memQ2PD/1024);
  memcount += memQ2PD;
  fprintf(stderr, " Miscellaneous mem. allocated: %7.ld kilobytes\n",
    memMSC/1024);
  memcount += memMSC;
  fprintf(stderr, " Inductance mem. allocated: %7.ld kilobytes\n",
    memIND/1024);
  memcount += memIND;
  fprintf(stderr, " Total memory (check w/above): %7.ld kilobytes\n",
    memcount/1024);
}

void *fh_calloc(size_t sz, int mtyp)
{
  void *p = calloc(1, sz);
  if (p) {
    memcount += sz;
    if(mtyp == AQ2M) memQ2M += sz;
    else if(mtyp == AQ2L) memQ2L += sz; 
    else if(mtyp == AQ2P) memQ2P += sz;
    else if(mtyp == AL2L) memL2L += sz;
    else if(mtyp == AM2M) memM2M += sz;
    else if(mtyp == AM2L) memM2L += sz;
    else if(mtyp == AM2P) memM2P += sz;
    else if(mtyp == AL2P) memL2P += sz;
    else if(mtyp == AQ2PD) memQ2PD += sz;
    else if(mtyp == AMSC) memMSC += sz;
    else if(mtyp == IND) memIND += sz;
    else {
      fprintf(stderr, "CALLOC: unknown memory type %d\n", mtyp);
      exit(1);
    }
  }
  return (p);
}

void *fh_malloc(size_t sz, int mtyp)
{
  void *p = malloc(sz);
  if (p) {
    memcount += sz;
    if(mtyp == AQ2M) memQ2M += sz;
    else if(mtyp == AQ2L) memQ2L += sz; 
    else if(mtyp == AQ2P) memQ2P += sz;
    else if(mtyp == AL2L) memL2L += sz;
    else if(mtyp == AM2M) memM2M += sz;
    else if(mtyp == AM2L) memM2L += sz;
    else if(mtyp == AM2P) memM2P += sz;
    else if(mtyp == AL2P) memL2P += sz;
    else if(mtyp == AQ2PD) memQ2PD += sz;
    else if(mtyp == AMSC) memMSC += sz;
    else if(mtyp == IND) memIND += sz;
    else {
      fprintf(stderr, "CALLOC: unknown memory type %d\n", mtyp);
      exit(1);
    }
  }
  return (p);
}

