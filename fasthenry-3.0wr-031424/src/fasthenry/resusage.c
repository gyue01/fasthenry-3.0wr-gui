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
/* # ***** sort to /src/header
   # ***** */
/* header where rusage and time structs are defined */

/* SRW ** Revised this:
 * getrusage is preferred, as it computes cpu time rather than wall-clock
 * time.  If getrusage is not available (as in MinGW) use gettimeofday.
 */

#include "mulGlobal.h"
#include <sys/time.h>


#ifndef NO_RUSAGE
#include <sys/resource.h>


static double seconds()
{
    struct rusage timestuff;

    getrusage(RUSAGE_SELF, &timestuff);
    double sec = (timestuff.ru_utime.tv_sec + timestuff.ru_stime.tv_sec) +
      1e-6*(timestuff.ru_utime.tv_usec + timestuff.ru_stime.tv_usec);
    return (sec);
}

#else /* NO_RUSAGE */

static double seconds()
{
    struct timeval ru_tv;
    struct timezone ru_tz;

    gettimeofday(&ru_tv, &ru_tz);
    return (ru_tv.tv_sec + 1.0e-6*ru_tv.tv_usec);
}

#endif /* NO_RUSAGE */

void starttimer(double *t)
{
    *t = seconds();
}

void stoptimer(double *t)
{
    *t = (seconds() - *t);
}

