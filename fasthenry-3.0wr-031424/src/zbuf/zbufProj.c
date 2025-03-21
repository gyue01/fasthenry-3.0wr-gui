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

#include "mulGlobal.h"
#include "zbufGlobal.h"

/* SRW */
void image(face**, int, line**, int, double*, double, double*);
void initFaces(face**, int, double*);
void flatten(face**, int, line**, int, double, double, double*, double*);
void makePos(face**, int, line**, int);
void scale2d(face**, int, line**, int, double, double*);
double *getAvg(face**, int, line**, int, int);
double getSphere(double*, face**, int, line**, int);
double getNormal(double*, double, double*, double*, double);


/*
  takes a list of 3d lines and returns a list of 3d lines mapped onto a plane
  given by a normal when projected back to the given view point
*/
void image(face **faces, int numfaces, line **lines, int numlines,
    double *normal, double rhs, double *view)
/* double rhs;                  rhs of the plane's equation */
{
  int i, j, k;
  double alpha, temp[3];
  extern double ***axes;
  extern int x_;

  /* transform axes (done always, needed for alignment) */
  for(i = 0; i < 7; i++) {
    for(j = 0; j < 2; j++) {
      for(k = 0; k < 3; k++) temp[k] = axes[i][j][k]-view[k];
      alpha = (rhs-dot(view, normal))/dot(temp, normal);
      if(alpha <= MARGIN) {
        fprintf(stderr, 
      "image: warning, view point is btwn view plane and axis pt, alpha=%g\n",
                alpha);
        /*fprintf(stderr, 
                "%d pnt-view = (%g %g %g) n = (%g %g %g)\n view = (%g %g %g)",
                i,temp[0],temp[1],temp[2],normal[0],normal[1],normal[2],
                view[0],view[1],view[2]);
        fprintf(stderr, " rhs = %g\n", rhs);
        exit(0);*/
      }
      for(k = 0; k < 3; k++) axes[i][j][k] = view[k]+alpha*temp[k];
    }
  }

  /* transform faces */
  for(i = 0; i < numfaces; i++) {
    for(j = 0; j < faces[i]->numsides; j++) { /* loop on number of sides */
      for(k = 0; k < 3; k++) temp[k] = (faces[i]->c)[j][k]-view[k];
      alpha = (rhs-dot(view, normal))/dot(temp, normal);
      if(alpha <= MARGIN) {
        fprintf(stderr, 
                "image: view point is btwn view plane and object, alpha=%g\n",
                alpha);
        fprintf(stderr, 
                "%d pnt-view = (%g %g %g) n = (%g %g %g)\n view = (%g %g %g)",
                i,temp[0],temp[1],temp[2],normal[0],normal[1],normal[2],
                view[0],view[1],view[2]);
        fprintf(stderr, " rhs = %g\n", rhs);
        exit(0);
      }
      for(k = 0; k < 3; k++) (faces[i]->c)[j][k] = view[k]+alpha*temp[k];
    }
  }

  /* transform lines */
  for(i = 0; i < numlines; i++) {
    for(k = 0; k < 3; k++) temp[k] = (lines[i]->from)[k]-view[k];
    alpha = (rhs-dot(view, normal))/dot(temp, normal);
    if(alpha <= MARGIN) {
      fprintf(stderr, 
              "image: from point is btwn view plane and object, alpha=%g\n",
              alpha);
      fprintf(stderr, 
              "%d pnt-view = (%g %g %g) n = (%g %g %g)\n view = (%g %g %g)",
              i,temp[0],temp[1],temp[2],normal[0],normal[1],normal[2],
              view[0],view[1],view[2]);
      fprintf(stderr, " rhs = %g\n", rhs);
      exit(0);
    }
    for(k = 0; k < 3; k++) (lines[i]->from)[k] = view[k]+alpha*temp[k];
    for(k = 0; k < 3; k++) temp[k] = (lines[i]->to)[k]-view[k];
    alpha = (rhs-dot(view, normal))/dot(temp, normal);
    if(alpha <= MARGIN) {
      fprintf(stderr, 
              "image: to point is btwn view plane and object, alpha=%g\n",
              alpha);
      fprintf(stderr, 
              "%d pnt-view = (%g %g %g) n = (%g %g %g)\n view = (%g %g %g)",
              i,temp[0],temp[1],temp[2],normal[0],normal[1],normal[2],
              view[0],view[1],view[2]);
      fprintf(stderr, " rhs = %g\n", rhs);
      exit(0);
    }
    for(k = 0; k < 3; k++) (lines[i]->to)[k] = view[k]+alpha*temp[k];
  }

}

/*
  makes sure normals point toward view point for all faces
*/
void initFaces(face **faces, int numfaces, double *view)
{
  int f, i;

  /* substitue the view point into each face's plane equation
     - if it's negative then normal points twrd object => used negative nrml*/
  for(f = 0; f < numfaces; f++) {
    if(dot(faces[f]->normal, view) - faces[f]->rhs < 0.0) {
      for(i = 0; i < 3; i++) faces[f]->normal[i] = 0.0-(faces[f]->normal[i]);
      faces[f]->rhs = 0.0-(faces[f]->rhs);
    }
  }
}


/*
  takes a list of 3d faces all in the same plane and flattens them to
  a 2d coordinate system in the plane 
  - sets up y axis in plane || to 1st line rotated according to rotation arg
*/
void flatten(face **faces, int numfaces, line **lines, int numlines,
    double rhs, double rotation, double *normal, double *view)
/* double rotation;             rotation of image y axis rel to 1st line */
{
  int i, j, k;
  double temp, tvec[3], crot, srot;
  double y[3], x[3], z[3];              /* unit vectors */
  double origin[3];
  extern double ***axes;
  extern int x_, up_axis;

  /* load origin on view plane from axis starting point */
  for(k = 0; k < 3; k++) origin[k] = axes[0][0][k];

  /* figure projection of view - faces[0]->c[0] onto normal to plane
     - if its negative then normal points towards object, use negative nrml */
  for(i = 0; i < 3; i++) tvec[i] = view[i] - (faces[0]->c)[0][i];
  if((temp = dot(normal, tvec)) < 0.0) {
    for(i = 0; i < 3; i++) z[i] = -normal[i];
  }
  else for(i = 0; i < 3; i++) z[i] = normal[i];

  /* find projection of upward-pointing axis (usually z) - becomes 2d y axis */
  for(k = 0; k < 3; k++) y[k] = axes[up_axis][1][k]-axes[up_axis][0][k];
  /* take out component normal to view plane */
  temp = dot(y, z);
  for(i = 0; i < 3; i++) y[i] -= temp*z[i];
  temp = sqrt(dot(y, y));
  for(i = 0; i < 3; i++) y[i] /= temp;

  /* get x axis - on the right as viewed from view point (view vec cross y) */
  /* do cross product to get x vector (x = y X z) */
  x[0] = z[2]*y[1]-z[1]*y[2];
  x[1] = z[0]*y[2]-z[2]*y[0];
  x[2] = z[1]*y[0]-z[0]*y[1];

  /* project all the faces onto these coordinates; overwrite x, y; zero z */
  for(i = 0; i < numfaces; i++) {
    for(j = 0; j < faces[i]->numsides; j++) {
      /* get new from point coordinates */
      for(k = 0; k < 3; k++) tvec[k] = faces[i]->c[j][k] - origin[k];
      temp = dot(tvec, x); /* x coordinate */
      (faces[i]->c)[j][1] = dot(tvec, y); /* y coordinate */
      (faces[i]->c)[j][2] = 0.0;        /* z */
      (faces[i]->c)[j][0] = temp;
    }
  }
  /* project all the lines onto these coordinates; overwrite x, y; zero z */
  for(i = 0; i < numlines; i++) {
    /* get new from point coordinates */
    for(k = 0; k < 3; k++) tvec[k] = lines[i]->from[k] - origin[k];
    temp = dot(tvec, x); /* x coordinate */
    (lines[i]->from)[1] = dot(tvec, y); /* y coordinate */
    (lines[i]->from)[2] = 0.0;  /* z */
    (lines[i]->from)[0] = temp;
    /* get new to point coordinates */
    for(k = 0; k < 3; k++) tvec[k] = lines[i]->to[k] - origin[k];
    temp = dot(tvec, x); /* x coordinate */
    (lines[i]->to)[1] = dot(tvec, y); /* y coordinate */
    (lines[i]->to)[2] = 0.0;    /* z */
    (lines[i]->to)[0] = temp;
  }
  /* if axes are specified, they must be included */
  for(i = 0; i < 7 && x_ == TRUE; i++) {
    for(j = 0; j < 2; j++) {
      for(k = 0; k < 3; k++) tvec[k] = axes[i][j][k] - origin[k];
      temp = dot(tvec, x);
      axes[i][j][1] = dot(tvec, y);
      axes[i][j][2] = 0.0;
      axes[i][j][0] = temp;
    }
  }

  /* rotate everything relative to the new origin
     - use the rotation matrix [cos(rot) -sin(rot); sin(rot) cos(rot)] */
  crot = cos(-M_PI*rotation/180.0); srot = sin(-M_PI*rotation/180.0);
  for(i = 0; i < numfaces; i++) {
    for(j = 0; j < faces[i]->numsides; j++) {
      temp = (faces[i]->c)[j][0]*crot - (faces[i]->c)[j][1]*srot;
      (faces[i]->c)[j][1] = (faces[i]->c)[j][0]*srot +(faces[i]->c)[j][1]*crot;
      (faces[i]->c)[j][0] = temp;
    }
  }
  /* include lines */
  for(i = 0; i < numlines; i++) {
    temp = (lines[i]->from)[0]*crot - (lines[i]->from)[1]*srot;
    (lines[i]->from)[1] = (lines[i]->from)[0]*srot +(lines[i]->from)[1]*crot;
    (lines[i]->from)[0] = temp;
    temp = (lines[i]->to)[0]*crot - (lines[i]->to)[1]*srot;
    (lines[i]->to)[1] = (lines[i]->to)[0]*srot +(lines[i]->to)[1]*crot;
    (lines[i]->to)[0] = temp;
  }
  /* if axes are specified, they must be included */
  for(i = 0; i < 7 && x_ == TRUE; i++) {
    for(j = 0; j < 2; j++) {
      temp = axes[i][j][0]*crot - axes[i][j][1]*srot;
      axes[i][j][1] = axes[i][j][0]*srot + axes[i][j][1]*crot;
      axes[i][j][0] = temp;
      /* axes[i][j][0] -= avg[0];   dont know why this is here
      axes[i][j][1] -= avg[1]; */
    }
  }

}

/*
  translates a list of 2d faces, lines to make all the coordinates positive
*/
void makePos(face **faces, int numfaces, line **lines, int numlines)
{
  int i,j;
  double minx, miny, trans[2];
  double offset[2];             /* the margins from 0 for smallest x and y */
  extern double ***axes;
  extern int x_;

  offset[1] = offset[0] = 0.0;  /* offsetting now done in scale2d */

  /* find the smallest x and y coordinates */
  for(i = 0; i < numfaces; i++) {
    for(j = 0; j < faces[i]->numsides; j++) {
      if(i == 0 && j == 0) {
        minx = faces[i]->c[j][0];
        miny = faces[i]->c[j][1];
      }
      else {
        minx = MIN(minx, faces[i]->c[j][0]);
        miny = MIN(miny, faces[i]->c[j][1]);
      }
    }
  }
  /* include lines */
  for(i = 0; i < numlines; i++) {
    if(i == 0 && numfaces == 0) {
      minx = MIN(lines[i]->from[0], lines[i]->to[0]);
      miny = MAX(lines[i]->from[1], lines[i]->to[1]);
    }
    else {
      minx = MIN(minx, lines[i]->from[0]);
      minx = MIN(minx, lines[i]->to[0]);
      miny = MIN(miny, lines[i]->from[1]);
      miny = MIN(miny, lines[i]->to[1]);
    }
  }
  /* include axes if they will be printed */
  for(i = 0; i < 7 && x_ == TRUE; i++) {
    for(j = 0; j < 2; j++) {
      minx = MIN(minx, axes[i][j][0]);
      miny = MIN(miny, axes[i][j][1]);
    }
  }
    
  /* transformation makes minx = offset[0], miny = offset[1] */
  trans[0] = offset[0]-minx;
  trans[1] = offset[1]-miny;
  for(i = 0; i < numfaces; i++) {
    for(j = 0; j < faces[i]->numsides; j++) {
      (faces[i]->c)[j][0] += trans[0];
      (faces[i]->c)[j][1] += trans[1];  
    }  
  }
  /* xform lines */
  for(i = 0; i < numlines; i++) {
    (lines[i]->from)[0] += trans[0];
    (lines[i]->from)[1] += trans[1];  
    (lines[i]->to)[0] += trans[0];
    (lines[i]->to)[1] += trans[1];  
  }
  for(i = 0; i < 7 && x_ == TRUE; i++) {  /* include axes if to be printed */
    for(j = 0; j < 2; j++) {
      axes[i][j][0] += trans[0];
      axes[i][j][1] += trans[1];  
    }  
  }
}

/*
  scales the 2d image so that it's offsetted and scale*IMAGEX x scale*IMAGEY
  - the global defines OFFSETX/Y set the offset
  - assumes smallest x and y coordinates are zero (called after makePos())
*/
void scale2d(face **faces, int numfaces, line **lines, int numlines,
    double scale, double *offset)
{
  int i, j;
  double ymax, xmax, xscale, yscale, master;
  extern double ***axes;
  extern int x_;

  /* find ymax, xmax */
  for(i = 0, xmax = ymax = 0.0; i < numfaces; i++) {
    for(j = 0; j < faces[i]->numsides; j++) {
      xmax = MAX(xmax, faces[i]->c[j][0]);
      ymax = MAX(ymax, faces[i]->c[j][1]);
    }
  }
  /* include lines */
  for(i = 0; i < numlines; i++) {
    xmax = MAX(xmax, lines[i]->to[0]);
    ymax = MAX(ymax, lines[i]->to[1]);
    xmax = MAX(xmax, lines[i]->from[0]);
    ymax = MAX(ymax, lines[i]->from[1]);
  }
  for(i = 0; i < 7 && x_ == TRUE; i++) { /* if axes to be prnted */
    for(j = 0; j < 2; j++) {
      xmax = MAX(xmax, axes[i][j][0]);
      ymax = MAX(ymax, axes[i][j][1]);
    }
  }

  if(xmax <= MARGIN || ymax <= MARGIN) {
    fprintf(stderr, "\nscale2d: strange xmax = %g or ymax = %g\n",
            xmax, ymax);
    exit(0);
  }

  /* find the x and y scales that would make those dimensions dead on */
  xscale = scale*IMAGEX/xmax;
  yscale = scale*IMAGEY/ymax;
  
  /* figure the final scaling and apply it - do the offsetting too */
  master = MIN(xscale, yscale);
  for(i = 0; i < numfaces; i++) {
    for(j = 0; j < faces[i]->numsides; j++) {
      faces[i]->c[j][0] *= master; 
      faces[i]->c[j][1] *= master;
      faces[i]->c[j][0] += OFFSETX; 
      faces[i]->c[j][1] += OFFSETY;
    }
  }
  for(i = 0; i < numlines; i++) {
    lines[i]->to[0] *= master; 
    lines[i]->to[1] *= master;
    lines[i]->to[0] += OFFSETX; 
    lines[i]->to[1] += OFFSETY;
    lines[i]->from[0] *= master; 
    lines[i]->from[1] *= master;
    lines[i]->from[0] += OFFSETX; 
    lines[i]->from[1] += OFFSETY;
  }
  for(i = 0; i < 7 && x_ == TRUE; i++) { /* do axes if needed */
    for(j = 0; j < 2; j++) {
      axes[i][j][0] *= master; 
      axes[i][j][1] *= master;
      axes[i][j][0] += OFFSETX; 
      axes[i][j][1] += OFFSETY;
    }
  }
}

/*
  returns the center of a rectangular prism contianing all the face corners
*/
double *getAvg(face **faces, int numfaces, line **lines, int numlines, int flag)
/* int flag;                    ON => include axes */
{
  double *avg, max[3], min[3];
  int i, j, k;
  extern double ***axes;
  extern int x_;

  CALLOC(avg, 3, double, ON, AMSC);

  /* check the faces' coordinates */
  for(i = 0; i < numfaces; i++) {
    for(j = 0; j < faces[i]->numsides; j++) {
      if(i == 0 && j == 0) {
        for(k = 0; k < 3; k++) max[k] = min[k] = (faces[0]->c)[0][k];
      }
      else {
        for(k = 0; k < 3; k++) {
          max[k] = MAX(max[k], faces[i]->c[j][k]);
          min[k] = MIN(min[k], faces[i]->c[j][k]);
        }
      }
    }
  }

  /* check the lines' coordinates */
  for(i = 0; i < numlines; i++) {
    if(i == 0 && numfaces == 0) {
      for(k = 0; k < 3; k++) {
        max[k] = MAX((lines[0]->from)[k], (lines[0]->to)[k]);
        min[k] = MIN((lines[0]->from)[k], (lines[0]->to)[k]);
      }
    }
    else {
      for(k = 0; k < 3; k++) {
        max[k] = MAX(max[k], lines[0]->from[k]);
        max[k] = MAX(max[k], lines[0]->to[k]);
        min[k] = MIN(min[k], lines[0]->from[k]);
        min[k] = MIN(min[k], lines[0]->to[k]);
      }
    }
  }

  /* if axes are specified, they must be included */
  for(i = 0; i < 7 && x_ == TRUE && flag == ON; i++) {
    for(j = 0; j < 2; j++) {
      for(k = 0; k < 3; k++) {
        max[k] = MAX(max[k], axes[i][j][k]);
        min[k] = MIN(min[k], axes[i][j][k]);
      }
    }
  }

  /* average each coordinates min an max to get center of picture */
  for(k = 0; k < 3; k++) avg[k] = (max[k] + min[k])/2;

  return(avg);
}

/*
  returns the radius of the smallest sphere with center at avg that
  encloses all the faces given in faces, all lines and axes, if required
*/
double getSphere(double *avg, face **faces, int numfaces, line **lines,
    int numlines)
{
  double radius = 0.0, temp[3];
  int i, j, k;
  extern int x_;
  extern double ***axes;

  /* loop through all the points, save the farthest away */
  for(i = 0; i <  numfaces; i++) {
    for(j = 0; j < faces[i]->numsides; j++) {
      for(k = 0; k < 3; k++) temp[k] = avg[k] - faces[i]->c[j][k];
      radius = MAX(radius, dot(temp, temp));
    }
  }

  /* loop through all the line points, save the farthest away */
  for(i = 0; i < numlines; i++) {
    for(k = 0; k < 3; k++) temp[k] = avg[k] - lines[i]->to[k];
    radius = MAX(radius, dot(temp, temp));
    for(k = 0; k < 3; k++) temp[k] = avg[k] - lines[i]->from[k];
    radius = MAX(radius, dot(temp, temp));
  }

  /* if axes are specified, they must be included */
  for(i = 0; i < 7 && x_ == TRUE; i++) {
    for(j = 0; j < 2; j++) {
      for(k = 0; k < 3; k++) temp[k] = avg[k] - axes[i][j][k];
      radius = MAX(radius, dot(temp, temp));
    }
  }

  return(sqrt(radius));
}

/* 
  get the normal to the image plane, adjust view point to be 2*radius away
  from object center - view coordinates are azimuth and elevation
  relative to object center at first
  - also change axes lengths so that they are smaller than view distance
    (ie so they won't cross the view plane)
*/
double getNormal(double *normal, double radius, double *avg, double *view,
    double distance)
{
  int i, k, j, axes_too_big, first;
  double rhs, norm, anorm, max_anorm;
  extern int x_;
  extern double ***axes;

  /* figure the normal to the view plane 
     - remember view is rel to avg
     - use view[0] as azimuth relative to poitive x direction
     - use view[1] as elevation relative to positive z direction */
  normal[0] = normal[1] = sin(M_PI*view[1]/180.0);
  normal[0] *= cos(M_PI*view[0]/180.0); /* x = sin(beta)cos(alpha) */
  normal[1] *= sin(M_PI*view[0]/180.0); /* y = sin(beta)sin(alpha) */
  normal[2] = cos(M_PI*view[1]/180.0); /* z = cos(beta) */

  /* figure the rhs of the plane's equation (use view for temp storage) */
  for(i = 0; i < 3; i++) view[i] = (1.0+distance/2.0)*radius*normal[i]+avg[i];
  rhs = dot(normal, view);

  /* adjust view point - change to absolute coordinates */
  for(i = 0; i < 3; i++) view[i] = (1.0+distance)*radius*normal[i] + avg[i];

  if(x_ == FALSE) {
    /* adjust the axes if needed to keep from going beyond the view plane */
    /*    get norm of view-avg */
    for(norm = 0.0, i = 0; i < 3; i++) 
        norm += (view[i]-avg[i])*(view[i]-avg[i]);
    /*    check sizes of current axes */
    axes_too_big = FALSE;
    first = TRUE;
    for(i = 0; i < 7; i++) {
      anorm = 0.0;
      for(k = 0; k < 3; k++) {
        anorm += (axes[i][0][k]-axes[i][1][k])*(axes[i][0][k]-axes[i][1][k]);
      }
      if(anorm >= norm) {
        axes_too_big = TRUE;
        if(first) {
          first = FALSE;
          max_anorm = anorm;
        }
        else max_anorm = MAX(max_anorm, anorm);
      }
    }
    /*    adjust if too big */
    if(axes_too_big) {
      /* make the biggest anorm = half the view-avg norm */
      max_anorm = sqrt(max_anorm); /* the biggest norm */
      anorm = sqrt(norm)/max_anorm; /* the scale factor */
      anorm /= 2.0;
      for(i = 0; i < 7; i++) {
        for(j = 0; j < 2; j++) {
          for(k = 0; k < 3; k++) {
            axes[i][j][k] *= anorm;
          }
        }
      }
    }
  }
  return(rhs);
}
