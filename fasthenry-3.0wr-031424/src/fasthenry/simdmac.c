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

/* A fast SIMD multiply-accumulate function for FastHenry.
 * Contributed by Stephen Whiteley, 11/29/2020 (http://wrcad.com)
 *
 * For gcc and compatible, requires -mavx on the compilation command
 * line.  This applies to gcc 4.4.7 and later (perhaps earlier) and
 * any non-antique Intel-compatible (with SSE2) CPU.  Other CPUs may
 * be supported as well.
 */

#include <immintrin.h>

//#define DBG_SIMD_TEST

inline double simd_mac (const double* a, const double* b, unsigned int len)
{
  int resid = len & 0x3;
  __m256d vsum = {0.0, 0.0, 0.0, 0.0};
  int i;
  len >>= 2;
  for (i = 0; i < len; i++) {
    __m256d va = _mm256_loadu_pd(a);
    __m256d vb = _mm256_loadu_pd(b);
/*    vsum = _mm256_fmadd_pd (va, vb, vsum);  // Illegal instruction? */
    vsum += va * vb;
    a += 4;
    b += 4;
  }
  {
    double sum = (vsum[0] + vsum[1] + vsum[2] + vsum[3]);
    while (resid-- > 0)
      sum += (*a++) * (*b++);
    return (sum);
  }
}

#ifdef DBG_SIMD_TEST
#include <stdio.h>

/* compile:  gcc -mavx -O3 simdmac.c */

int main(int argc, char **argv)
{
    double a[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
    double b[] = {10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0};

    int i;
    for (i = 1; i <= 10; i++) {
        double s = simd_mac(a, b, i);
        printf("%d %.1f\n", i, s);
    }
    return (0);
}

#endif

