/* Inner part of fast marching. */
/*
  Copyright (C) 2004 University of Texas at Austin
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <float.h>
#include <math.h>

#include <rsf.h>
/*^*/

#include "neighbors.h"

struct Upd {
    double stencil, value;
    double delta;
};

static int update (float value, int i);
static float qsolve(int i); 
static void stencil (float t, struct Upd *x); 
static bool updaten (int m, float* res, struct Upd *v[]);
static void grid (int *i, const int *n);

static int *in, *n, s[3], order;
static float *time, *vv, rdx[3];
static double v1;
static const float big_value = FLT_MAX;

void neighbors_init (int *in1     /* status flag [n[0]*n[1]*n[2]] */, 
		     float *rdx1  /* grid sampling [3] */, 
		     int *n1      /* grid samples [3] */, 
		     int order1   /* accuracy order */, 
		     float *time1 /* traveltime [n[0]*n[1]*n[2]] */)
/*< Initialize >*/
{
    in = in1; time = time1; 
    n = n1; order = order1;
    s[0] = 1; s[1] = n[0]; s[2] = n[0]*n[1];
    rdx[0] = 1./(rdx1[0]*rdx1[0]);
    rdx[1] = 1./(rdx1[1]*rdx1[1]);
    rdx[2] = 1./(rdx1[2]*rdx1[2]);
}

int  neighbours(int i) 
/*< Update neighbors of gridpoint i, return number of updated points >*/
{
    int j, k, ix, npoints;
    
    npoints = 0;
    for (j=0; j < 3; j++) {
	ix = (i/s[j])%n[j];
	if (ix+1 <= n[j]-1) {
	    k = i+s[j]; 
	    if (in[k] != SF_IN) npoints += update(qsolve(k),k);
	}
	if (ix-1 >= 0  ) {
	    k = i-s[j];
	    if (in[k] != SF_IN) npoints += update(qsolve(k),k);
	}
    }
    return npoints;
}

static int update (float value, int i)
/* update gridpoint i with new value */
{
    if (value < time[i]) {
	time[i]   = value;
	if (in[i] == SF_OUT) { 
	    in[i] = SF_FRONT;      
	    sf_pqueue_insert (time+i);
	    return 1;
	}
    }
    
    return 0;
}

static float qsolve(int i)
/* find new traveltime at gridpoint i */
{
    int j, k, ix;
    float a, b, t, res;
    struct Upd *v[3], x[3], *xj;

    for (j=0; j<3; j++) {
	ix = (i/s[j])%n[j];
	
	if (ix > 0) { 
	    k = i-s[j];
	    a = time[k];
	} else {
	    a = big_value;
	}

	if (ix < n[j]-1) {
	    k = i+s[j];
	    b = time[k];
	} else {
	    b = big_value;
	}

	xj = x+j;
	xj->delta = rdx[j];

	if (a < b) {
	    xj->stencil = xj->value = a;
	} else {
	    xj->stencil = xj->value = b;
	}

	if (order > 1) {
	    if (a < b  && ix-2 >= 0) { 
		k = i-2*s[j];
		if (in[k] != SF_OUT && a >= (t=time[k]))
		    stencil(t,xj);
	    }
	    if (a > b && ix+2 <= n[j]-1) { 
		k = i+2*s[j];
		if (in[k] != SF_OUT && b >= (t=time[k]))
		    stencil(t,xj);
	    }
	}
    }

    if (x[0].value <= x[1].value) {
	if (x[1].value <= x[2].value) {
	    v[0] = x; v[1] = x+1; v[2] = x+2;
	} else if (x[2].value <= x[0].value) {
	    v[0] = x+2; v[1] = x; v[2] = x+1;
	} else {
	    v[0] = x; v[1] = x+2; v[2] = x+1;
	}
    } else {
	if (x[0].value <= x[2].value) {
	    v[0] = x+1; v[1] = x; v[2] = x+2;
	} else if (x[2].value <= x[1].value) {
	    v[0] = x+2; v[1] = x+1; v[2] = x;
	} else {
	    v[0] = x+1; v[1] = x+2; v[2] = x;
	}
    }
    
    v1=vv[i];

    if(v[2]->value < big_value) {   /* ALL THREE DIRECTIONS CONTRIBUTE */
	if (updaten(3, &res, v) || 
	    updaten(2, &res, v) || 
	    updaten(1, &res, v)) return res;
    } else if(v[1]->value < big_value) { /* TWO DIRECTIONS CONTRIBUTE */
	if (updaten(2, &res, v) || 
	    updaten(1, &res, v)) return res;
    } else if(v[0]->value < big_value) { /* ONE DIRECTION CONTRIBUTES */
	if (updaten(1, &res, v)) return res;
    }
	
    return big_value;
}

static void stencil (float t, struct Upd *x)
/* second-order stencil */
{
    x->delta *= 2.25;
    x->stencil = (4.0*x->value - t)/3.0;
}

static bool updaten (int m, float* res, struct Upd *v[]) 
/* updating */
{
    double a, b, c, discr, t;
    int j;

    a = b = c = 0.;

    for (j=0; j<m; j++) {
	a += v[j]->delta;
	b += v[j]->stencil*v[j]->delta;
	c += v[j]->stencil*v[j]->stencil*v[j]->delta;
    }
    b /= a;

    discr=b*b+(v1-c)/a;

    if (discr < 0.) return false;
    
    t = b +sqrt(discr);
    if (t <= v[m-1]->value) return false;

    *res = t;
    return true;
}

static void grid (int *i, const int *n)
/* restrict i[3] to the grid n[3] */
{ 
    int j;

    for (j=0; j < 3; j++) {
	if (i[j] < 0) {
	    i[j]=0;
	} else if (i[j] >= n[j]) {
	    i[j]=n[j]-1;
	}
    }
}

static int dist(int k, float x1, float x2, float x3) 
/* assign distance to a neighboring grid point */
{
    float ti;

    ti = hypotf(x1,hypotf(x2,x3));
    if (SF_OUT == in[k]) {
	in[k] = SF_IN;
	time[k] = ti;
	return 1;
    } else if (ti < time[k]) {
	time[k] = ti;
    }

    return 0;
}

int neighbors_distance(int np         /* number of points */, 
		       float **points /* point coordinates[np][3] */,
		       float *d       /* grid sampling [3] */,
		       float *o       /* grid origin [3] */)
/*< initialize distance computation >*/
{
    int npoints, ip, i, j, n123, ix[3], k;
    float x[3];

    n123 = n[0]*n[1]*n[2];

    vv = sf_floatalloc(n123);

    /* initialize everywhere */
    for (i=0; i < n123; i++) {
	in[i] = SF_OUT;
	time[i] = big_value;
	vv[i] = 1.;
    }

    npoints = 0;
    for (ip=0; ip < np; ip++) {
	for (j=0; j < 3; j++) {
	    x[j] = (points[ip][j]-o[j])/d[j];
	    ix[j] = x[j];
	}
	if (x[0] < 0. || ix[0] >= n[0] ||
	    x[1] < 0. || ix[1] >= n[1] ||
	    x[2] < 0. || ix[2] >= n[2]) continue;
	k = 0;
	for (j=0; j < 3; j++) {
	    x[j] = (x[j]-ix[j])*d[j];
	    k += ix[j]*s[j];
	}
	npoints += dist(k,x[0],x[1],x[2]);
	if (ix[0] != n[0]-1) {
	    npoints += dist(k+s[0],d[0]-x[0],x[1],x[2]);
	    if (x[1] != n[1]-1) {
		npoints += dist(k+s[0]+s[1],d[0]-x[0],d[1]-x[1],x[2]);
		if (ix[2] != n[2]-1) 
		    npoints += 
			dist(k+s[0]+s[1]+s[2],d[0]-x[0],d[1]-x[1],d[2]-x[2]);
	    }
	    if (ix[2] != n[2]-1) 
		npoints += dist(k+s[0]+s[2],d[0]-x[0],x[1],d[2]-x[2]);
	}
	if (ix[1] != n[1]-1) {
	    npoints += dist(k+s[1],x[0],d[1]-x[1],x[2]);
	    if (ix[2] != n[2]-1) 
		npoints += dist(k+s[1]+s[2],x[0],d[1]-x[1],d[2]-x[2]);
	}
	if (ix[2] != n[2]-1) npoints += dist(k+s[2],x[0],x[1],d[2]-x[2]);
    }

    return npoints;
}

int nearsource(float* xs   /* source location [3] */, 
	       int* b      /* constant-velocity box around it [3] */, 
	       float* d    /* grid sampling [3] */, 
	       float* vv1  /* slowness [n[0]*n[1]*n[2]] */, 
	       bool *plane /* if plane-wave source */)
/*< initialize the source >*/
{
    int npoints, ic, i, j, is, start[3], endx[3], ix, iy, iz;
    double delta[3], delta2;
    

    /* initialize everywhere */
    for (i=0; i < n[0]*n[1]*n[2]; i++) {
	in[i] = SF_OUT;
	time[i] = big_value;
    }

    vv = vv1;

    /* Find index of the source location and project it to the grid */
    for (j=0; j < 3; j++) {
	is = xs[j]/d[j]+0.5;
	start[j] = is-b[j]; 
	endx[j]  = is+b[j];
    } 
    
    grid(start, n);
    grid(endx, n);
    
    ic = (start[0]+endx[0])/2 + 
	n[0]*((start[1]+endx[1])/2 +
	      n[1]*(start[2]+endx[2])/2);
    
    v1 = vv[ic];

    /* loop in a small box around the source */
    npoints = n[0]*n[1]*n[2];
    for (ix=start[2]; ix <= endx[2]; ix++) {
	for (iy=start[1]; iy <= endx[1]; iy++) {
	    for (iz=start[0]; iz <= endx[0]; iz++) {
		npoints--;
		i = iz + n[0]*(iy + n[1]*ix);

		delta[0] = xs[0]-iz*d[0];
		delta[1] = xs[1]-iy*d[1];
		delta[2] = xs[2]-ix*d[2];

		delta2 = 0.;
		for (j=0; j < 3; j++) {
		    if (!plane[j]) delta2 += delta[j]*delta[j];
		}

		/* analytical formula (Euclid) */ 
		time[i] = sqrtf(v1*delta2);
		in[i] = SF_IN;

		if ((n[0] > 1 && (iz == start[0] || iz == endx[0])) ||
		    (n[1] > 1 && (iy == start[1] || iy == endx[1])) ||
		    (n[2] > 1 && (ix == start[2] || ix == endx[2]))) {
		    sf_pqueue_insert (time+i);
		}
	    }
	}
    }
    
    return npoints;
}

/* 	$Id$	 */

