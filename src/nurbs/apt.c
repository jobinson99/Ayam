/*
 * Ayam, a free 3D modeler for the RenderMan interface.
 *
 * Ayam is copyrighted 1998-2021 by Randolf Schultz
 * (randolf.schultz@gmail.com) and others.
 *
 * All rights reserved.
 *
 * See the file License for details.
 *
 */

#include "ayam.h"

/** \file apt.c \brief approximating surface tools */


/** ay_apt_getpntfromindex:
 * Get memory address of a single APatch control point from its indices
 * (performing bounds checking).
 *
 * \param[in] patch  APatch object to process
 * \param[in] indexu  index of desired control point in U dimension (width)
 * \param[in] indexv  index of desired control point in V dimension (height)
 *
 * \param[in,out] p  where to store the resulting memory address
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_apt_getpntfromindex(ay_apatch_object *patch, int indexu, int indexv,
		       double **p)
{
 int stride = 3;
 char fname[] = "apt_getpntfromindex";

  if(!patch || !p)
    return AY_ENULL;

  if(indexu >= patch->width || indexu < 0)
    return ay_error_reportirange(fname, "\"indexu\"", 0, patch->width-1);

  if(indexv >= patch->height || indexv < 0)
    return ay_error_reportirange(fname, "\"indexv\"", 0, patch->height-1);

  *p = &(patch->controlv[(indexu*patch->height+indexv)*stride]);

 return AY_OK;
} /* ay_apt_getpntfromindex */


/** ay_apt_swapuv:
 * swap U and V dimensions of a APatch
 *
 * \param[in,out] ap  APatch object to process
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_apt_swapuv(ay_apatch_object *ap)
{
 int ay_status = AY_OK;
 int i;

  if(!ap)
    return AY_ENULL;

  ay_status = ay_npt_swaparray(&(ap->controlv), 3, ap->width, ap->height);

  if(ay_status)
    return ay_status;

  i = ap->width;
  ap->width = ap->height;
  ap->height = i;

  i = ap->awidth;
  ap->awidth = ap->aheight;
  ap->aheight = i;

  i = ap->order_u;
  ap->order_u = ap->order_v;
  ap->order_v = i;

  i = ap->close_u;
  ap->close_u = ap->close_v;
  ap->close_v = i;

  i = ap->ktype_u;
  ap->ktype_u = ap->ktype_v;
  ap->ktype_v = i;

 return ay_status;
} /* ay_apt_swapuv */


/** ay_apt_revertu:
 * flip APatch along U (width)
 *
 * \param[in,out] ap  APatch object to revert
 *
 */
void
ay_apt_revertu(ay_apatch_object *ap)
{
 int i, j, ii, jj, stride = 3;
 double t[4];

  if(!ap)
    return;

  for(i = 0; i < ap->height; i++)
    {
      for(j = 0; j < ap->width/2; j++)
	{
	  ii = (j*ap->height+i)*stride;
	  jj = ((ap->width-1-j)*ap->height+i)*stride;
	  memcpy(t, &(ap->controlv[ii]), stride*sizeof(double));
	  memcpy(&(ap->controlv[ii]), &(ap->controlv[jj]),
		 stride*sizeof(double));
	  memcpy(&(ap->controlv[jj]), t, stride*sizeof(double));
	}
    }

 return;
} /* ay_apt_revertu */


/** ay_apt_revertv:
 * flip APatch along V (height)
 *
 * \param[in,out] ap  APatch object to revert
 *
 */
void
ay_apt_revertv(ay_apatch_object *ap)
{
 int i, j, ii, jj, stride = 3;
 double t[4];

  if(!ap)
    return;

  for(i = 0; i < ap->width; i++)
    {
      ii = i*ap->height*stride;
      jj = ii + ((ap->height-1)*stride);
      for(j = 0; j < ap->height/2; j++)
	{
	  memcpy(t, &(ap->controlv[ii]), stride*sizeof(double));
	  memcpy(&(ap->controlv[ii]), &(ap->controlv[jj]),
		 stride*sizeof(double));
	  memcpy(&(ap->controlv[jj]), t, stride*sizeof(double));
	  ii += stride;
	  jj -= stride;
	}
    }

 return;
} /* ay_apt_revertv */


/** ay_apt_createknots:
 * Create a chordal or centripetal knot vector by knot averaging.
 *
 * \param[in] ml  mean lengths across the surface
 *            (e.g. obtained via \a ay_npt_avglens*())
 * \param[in] mllen  length of \a ml
 * \param[in] kt  knot type (0 - chordal, 1 - centripetal)
 * \param[in] alen  approximation length
 * \param[in] aorder  approximation order
 * \param[in] closed  whether to create a periodic knot vector
 *
 * \param[in,out] ubr  where to store the resulting parameter array
 * \param[in,out] Ur  where to store the resulting knot array
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_apt_createknots(double *ml, int mllen, int kt, int alen, int aorder,
		   int closed, double **ubr, double **Ur)
{
 int ay_status = AY_OK;
 double *ub = NULL, *U = NULL, d;
 int m, n, p;
 int i, j, k, i2, da;
 double t, tlen = 0.0, alpha;

  m = mllen;
  p = aorder-1;
  n = alen;

  for(i = 0; i < (mllen-1); i++)
    tlen += ml[i];

  if(!(ub = malloc(mllen * sizeof(double))))
    {
      return AY_EOMEM;
    }

  if(kt)
    {
      /* centripetal */
      ub[0] = 0.0;
      j = 0;
      t = 0.0;
      for(i = 1; i < (mllen-1); i++)
	{
	  t += ml[j]/tlen;
	  ub[i] = t;

	  j++;
	}
      ub[mllen-1] = 1.0;
    }
  else
    {
      /* chordal */
      ub[0] = 0.0;
      j = 0;
      t = 0.0;
      for(i = 1; i < (mllen-1); i++)
	{
	  t += ml[j]/tlen;
	  ub[i] = t;
	  /*printf("%d: %lg (diff %lg)\n",i,ub[i],ub[i]-ub[i-1]);*/
	  j++;
	}
      ub[mllen-1] = 1.0;
    }

  /* create clamped knot vector by averaging */
  if(!(U = calloc(n+p+1, sizeof(double))))
    {
      ay_status = AY_EOMEM;
      free(ub);
      goto cleanup;
    }

  if(closed)
    {
      d = (m+1) / (double)(n-p);
      for(j = 1; j < n-p; j++)
	{
	  i = (int)(j*d);
	  alpha = j*d-i;
	  /*      (*U)[p+j] = (1-alpha)*ub[(i-1)%(n+p)] + alpha*ub[(i)%(n+p)];*/
	  U[p+j] = (1-alpha)*ub[(i-1)%(m)] + alpha*ub[(i)%(m)];
	}

      for(i = 0; i < p; i++)
	{
	  U[i] = U[i+(n-p)] - 1;
	}

      for(i = n; i < n+p+1; i++)
	{
	  U[i] = 1 + U[i-(n-p)];
	}
    }
  else
    {
      for(i = 0; i < p+1; i++)
	{
	  U[i] = 0.0;
	}

      /* from NURBS++ */
      d = m / (double)n;
      for(j = 1; j < n-p; j++)
	{
	  U[p+j] = 0.0;
	  for(k = j; k < j+p; k++)
	    {
	      i = (int)(k*d);
	      da = (int)(k*d-i);
	      i2 = (int)((k-1)*d);
	      U[p+j] += da*ub[i2] + (1.0-da)*ub[i];
	    }
	  U[p+j] /= p;
	}

      for(i = n; i < n+p+1; i++)
	{
	  U[i] = 1.0;
	}
    }

  /* return results*/
  *Ur = U;
  *ubr = ub;

cleanup:

 return ay_status;
} /* ay_apt_createknots */


typedef struct ay_lsarrays {
 double *ub;
 double *Ns, *Nt, *NN;
 double *R, *rk, *N, *X;
 double *funs;
} ay_lsarrays;


/** ay_apt_leastSquares:
 *  approximate the data points in Q[m] with a NURBS curve of degree p
 *  with <n> control points, return results in P
 *
 * \param[in] Q  data points [Qstride*m]
 * \param[in] Qstride  size of a data point (3 or 4)
 * \param[in] Pstride  desired output point size (3 or 4)
 * \param[in] m  number of data points
 * \param[in] n  desired number of resulting points
 * \param[in] p  desired order
 * \param[in] closed  whether to create a closed curve
 * \param[in] lsa  unused
 * \param[in] ub  parameter vector (created by \a ay_apt_createknots())
 * \param[in] U  knot vector (created by \a ay_apt_createknots())
 *
 * \param[in,out] P  where to store the result
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_apt_leastSquares(double *Q, int Qstride, int Pstride,
		    int m, int n, int p, int closed,
		    ay_lsarrays *lsa, double *ub, double *U, double **P)
{
 int ay_status = AY_OK;
 int a, i, j, span;
 double *Ns = NULL, *Nt = NULL, *NN = NULL;
 double *R = NULL, *rk = NULL, *N = NULL, *X = NULL;
 double *funs = NULL;

  if(!Q || !U || !P)
    return AY_ENULL;

  if(closed)
    {
      if(n > m+p)
	return AY_ERROR;
    }
  else
    {
      if(n > m)
	return AY_ERROR;
    }

  if(!*P)
    {
      if(!(*P = calloc(n*Pstride, sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}
    }

  /* solve N^T*N*P = R */

  if(closed)
    {
      /* closed */
      /* the following section operates on a reduced number of output points
	 since the last p output points will be equal to the first p output
	 points anyway */
      n -= p;

      if(!(R = calloc(n*3, sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      /* Note well: N is nxm! (not mxn, as suggested by the NURBS book) */
      if(!(N = calloc(n*m, sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      if(!(funs = calloc(p+1, sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      /* set up N */
      for(i = 0; i < m; i++)
	{
	  span = ay_nb_FindSpan(n, p, ub[i], &(U[p-1])/**U*/);

	  /* protect BasisFuns() from bad spans */
	  if(span >= n)
	    span = n-1;

	  memset(funs, 0, (p+1)*sizeof(double));
	  ay_nb_BasisFuns(span, ub[i], p, &(U[p-1])/**U*/, funs);

	  for(j = 0; j <= p; j++)
	    {
	      a = (span-p+j)*m+i;
	      N[a] = funs[j];
	      /*
		if(isnan(N[a]))
		{
		ay_status = AY_ERROR; goto cleanup;
		}
	      */
	    }
	} /* for */

      /* set up R */
      for(i = 0; i < n; i++)
	{
	  /*R[i*Qstride] = 0.0;*/
	  memset(&(R[i*3]), 0, 3*sizeof(double));

	  for(j = 0; j < m; j++)
	    {
	      /*R[i] += N(i,j)*Q[j] ;*/
	      R[i*3]   += N[i*m+j]*Q[j*Qstride];
	      R[i*3+1] += N[i*m+j]*Q[j*Qstride+1];
	      R[i*3+2] += N[i*m+j]*Q[j*Qstride+2];
	    }
	  /*
	    if(R[i*3]   * R[i*3]   < AY_EPSILON &&
	    R[i*3+1] * R[i*3+1] < AY_EPSILON &&
	    R[i*3+2] * R[i*3+2] < AY_EPSILON)
	    {
	    ay_status = AY_ERROR;
	    goto cleanup;
	    }
	  */
	} /* for */

      /* solve N^T*N*P = R */

      if(!(Nt = calloc(m*n, sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      if(!(NN = calloc(n*n, sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      /* fill Nt */
      a = 0;
      for(i = 0; i < m; i++)
	{
	  for(j = 0; j < n; j++)
	    {
	      Nt[a] = N[j*m+i];
	      a++;
	    }
	}

      /* do NN=N^T*N */
      ay_act_multmatrixmn(m, n, Nt, N, NN);

      if(!(X = calloc(n*3, sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      /* solve the linear equation system NN*X=R */
      ay_status = ay_act_solve(n, n, NN, R, X);

      if(ay_status)
	{ goto cleanup; }

      /* save results from X */
      j = 0;
      for(i = 0; i < n; i++)
	{
	  memcpy(&((*P)[i*Pstride]), &(X[j*3]), 3*sizeof(double));
	  j++;
	}
      /* for the final operations we increase n again to the full
	 output length */
      n += p;

      /* copy the periodic points */
      memcpy(&((*P)[(n-p)*Pstride]), *P, p*Pstride*sizeof(double));
    }
  else
    {
      /* open */

      if(!(R = calloc(n*3, sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      if(!(rk = calloc(m*3, sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      /* Note well: N is nxm! (not mxn, as suggested by the NURBS book) */
      if(!(N = calloc(n*m, sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      if(!(funs = calloc(p+1, sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      /* set up N and rk */
      for(i = 0; i < m; i++)
	{
	  span = ay_nb_FindSpan(n, p, ub[i], U);

	  /* protect BasisFuns() from bad spans */
	  if(span >= n)
	    span = n-1;

	  memset(funs, 0, (p+1)*sizeof(double));
	  ay_nb_BasisFuns(span, ub[i], p, U, funs);

	  for(j = 0; j <= p; j++)
	    {
	      a = (span-p+j)*m+i;
	      N[a] = funs[j];
	      /*
		if(isnan(N[a]))
		{
		ay_status = AY_ERROR; goto cleanup;
		}
	      */
	    }

	  /*rk[i] = Q[i]-N(0,i)*Q[0]-N(n-1,i)*Q[m-1];*/
	  rk[i*3]   = Q[i*Qstride]   - N[i] * Q[0] -
	    N[(n-1)*m+i] * Q[(m-1)*Qstride];

	  rk[i*3+1] = Q[i*Qstride+1] - N[i] * Q[1] -
	    N[(n-1)*m+i] * Q[(m-1)*Qstride+1];

	  rk[i*3+2] = Q[i*Qstride+2] - N[i] * Q[2] -
	    N[(n-1)*m+i] * Q[(m-1)*Qstride+2];
	} /* for */

      /* set up R */
      for(i = 0; i < n; i++)
	{
	  /*R[i*3] = 0.0;*/
	  memset(&(R[i*3]), 0, 3*sizeof(double));

	  for(j = 0; j < m; j++)
	    {
	      /*R[i] += N(i,j)*rk[j] ;*/
	      R[i*3]   += N[i*m+j]*rk[j*3];
	      R[i*3+1] += N[i*m+j]*rk[j*3+1];
	      R[i*3+2] += N[i*m+j]*rk[j*3+2];
	    }
	  /*
	    if(R[i*3]   * R[i*3]   < AY_EPSILON &&
	    R[i*3+1] * R[i*3+1] < AY_EPSILON &&
	    R[i*3+2] * R[i*3+2] < AY_EPSILON)
	    {
	    ay_status = AY_ERROR;
	    goto cleanup;
	    }
	  */
	} /* for */

      if(n > 2)
	{
	  if(!(Ns = calloc((n-2)*(m-2), sizeof(double))))
	    {
	      ay_status = AY_EOMEM;
	      goto cleanup;
	    }
	  if(!(Nt = calloc((m-2)*(n-2), sizeof(double))))
	    {
	      ay_status = AY_EOMEM;
	      goto cleanup;
	    }
	  if(!(NN = calloc((n-2)*(n-2), sizeof(double))))
	    {
	      ay_status = AY_EOMEM;
	      goto cleanup;
	    }
	  if(!(X = calloc((n-2)*3, sizeof(double))))
	    {
	      ay_status = AY_EOMEM;
	      goto cleanup;
	    }

	  /* fill Ns & Nt */
	  a = 0;
	  for(i = 1; i < n-1; i++)
	    {
	      for(j = 1; j < m-1; j++)
		{
		  Ns[a] = N[i*m+j];
		  a++;
		}
	    }
	  a = 0;
	  for(i = 1; i < m-1; i++)
	    {
	      for(j = 1; j < n-1; j++)
		{
		  Nt[a] = N[j*m+i];
		  a++;
		}
	    }

	  /* do NN=N^T*N */
	  ay_act_multmatrixmn(m-2, n-2, Nt, Ns, NN);

	  /* solve the linear equation system NN*X=R */
	  ay_status = ay_act_solve(n-2, n-2, NN, &(R[3]), X);

	  if(ay_status)
	    { goto cleanup; }

	  /* save results from X */
	  j = 0;
	  for(i = 1; i < n-1; i++)
	    {
	      memcpy(&((*P)[i*Pstride]), &(X[j*3]), 3*sizeof(double));
	      j++;
	    }
	} /* if */

      /* first and last points are data points */
      /*P[0] = Q[0];*/
      memcpy(*P, Q, 3*sizeof(double));
      /*P[n-1] = Q[m-1];*/
      memcpy(&((*P)[(n-1)*Pstride]), &(Q[(m-1)*Qstride]), 3*sizeof(double));
    } /* if */

  /* set weights */
  if(Pstride > 3)
    {
      a = 3;
      for(i = 0; i < n; i++)
	{
	  (*P)[a] = 1.0;
	  a += Pstride;
	}
    }

cleanup:

  if(ay_status)
    {
      if(*P)
	free(*P);
      *P = NULL;
    }

  if(R)
    free(R);

  if(rk)
    free(rk);

  if(N)
    free(N);

  if(funs)
    free(funs);

  if(Ns)
    free(Ns);

  if(Nt)
    free(Nt);

  if(NN)
    free(NN);

  if(X)
    free(X);

 return ay_status;
} /* ay_apt_leastSquares */


/** ay_apt_approximateuv:
 * Approximate a 2D array of data points with a NURBS surface of
 * desired width, height, and orders first in U then in V direction.
 *
 * \param[in] cv  control points to approximate [width*height*3]
 * \param[in] width  width of \a cv
 * \param[in] height  height of \a cv
 * \param[in] awidth  desired approximation width,
 *            must be <= \a width
 * \param[in] aheight  desired approximation height,
 *            must be <= \a height
 * \param[in] uorder  desired approximation order in U direction,
 *            must be <= \a awidth (unchecked)
 * \param[in] vorder  desired approximation order in V direction,
 *            must be <= \a aheight (unchecked)
 * \param[in] uknottype  desired knot type for U direction,
 *            must be 0 - chordal or 1 - centripetal
 * \param[in] vknottype  desired knot type for V direction,
 *            must be 0 - chordal or 1 - centripetal
 * \param[in] closeu  create a closed surface in U direction
 * \param[in] closev  create a closed surface in V direction
 *
 * \param[in,out] np  where to store the new NURBS patch object
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_apt_approximateuv(double *cv, int width, int height,
		     int awidth, int aheight,
		     int uorder, int vorder,
		     int uknottype, int vknottype,
		     int closeu, int closev,
		     ay_nurbpatch_object **np)
{
 int ay_status = AY_OK;
 double *mean = NULL;
 int a, b, i, j;
 double *ub = NULL, *vb = NULL;
 double *U = NULL, *V = NULL, *Pt = NULL, *P = NULL, *Q = NULL;
 double *pp;
 ay_lsarrays lsa = {0};

  if(!cv || !np)
    return AY_ENULL;

  if((awidth > width) || (aheight > height))
    return AY_ERROR;

  if(closeu)
    awidth += uorder-1;

  if(closev)
    aheight += vorder-1;

  if(!(Pt = calloc(awidth*height*3, sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  ay_status = ay_npt_avglensu(cv, width, height, 3, &mean);

  if(ay_status)
    goto cleanup;

  ay_status = ay_apt_createknots(mean, width, uknottype, awidth,
				 uorder, closeu,
				 &ub, &U);

  if(ay_status)
    goto cleanup;

  if(!(Q = calloc(awidth*3, sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  a = 0;
  for(i = 0; i < height; i++)
    {
      ay_status = ay_apt_leastSquares(&(cv[a]), height*3, 3, width, awidth,
				      uorder-1, closeu,
				      &lsa, ub, U, &Q);

      if(ay_status)
	goto cleanup;

      b = i*3;
      for(j = 0; j < awidth; j++)
	{
	  memcpy(&(Pt[b]), &(Q[j*3]), 3*sizeof(double));
	  /*printf("%d,%d: %lg %lg %lg\n",i,j,Q[j*3],Q[j*3+1],Q[j*3+2]);*/
	  b += height*3;
	}
      a += 3;
    }

  free(mean);
  mean = NULL;

  ay_status = ay_npt_avglensv(Pt, awidth, height, 3, &mean);

  if(ay_status)
    goto cleanup;

  ay_status = ay_apt_createknots(mean, height, vknottype, aheight,
				 vorder, closev,
				 &vb, &V);

  if(ay_status)
    goto cleanup;

  if(!(P = malloc(awidth*aheight*4*sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  a = 0;
  b = 0;
  for(i = 0; i < awidth; i++)
    {
      pp = &(P[b]);
      ay_status = ay_apt_leastSquares(&(Pt[a]), 3, 4, height, aheight,
				      vorder-1, closev, &lsa, vb, V, &pp);

      if(ay_status)
	goto cleanup;

      b += aheight*4;
      a += height*3;
    }

  ay_status = ay_npt_create(uorder, vorder, awidth, aheight,
			    AY_KTCUSTOM, AY_KTCUSTOM,
			    P, U, V, np);

  free(vb);

cleanup:

  if(ay_status)
    {
      if(P)
	free(P);
      if(U)
	free(U);
      if(V)
	free(V);
    }

  if(Pt)
    free(Pt);
  if(mean)
    free(mean);
  if(Q)
    free(Q);
  if(ub)
    free(ub);

 return ay_status;
} /* ay_apt_approximateuv */


/** ay_apt_approximatevu:
 * Approximate a 2D array of data points with a NURBS surface of
 * desired width, height, and orders first in V then in U direction.
 *
 * \param[in] cv  control points to approximate [width*height*3]
 * \param[in] width  width of \a cv
 * \param[in] height  height of \a cv
 * \param[in] awidth  desired approximation width,
 *            must be <= \a width
 * \param[in] aheight  desired approximation height,
 *            must be <= \a height
 * \param[in] uorder  desired approximation order in U direction,
 *            must be <= \a awidth (unchecked)
 * \param[in] vorder  desired approximation order in V direction,
 *            must be <= \a aheight (unchecked)
 * \param[in] uknottype  desired knot type for U direction,
 *            must be 0 - chordal or 1 - centripetal
 * \param[in] vknottype  desired knot type for V direction,
 *            must be 0 - chordal or 1 - centripetal
 * \param[in] closeu  create a closed surface in U direction
 * \param[in] closev  create a closed surface in V direction
 *
 * \param[in,out] np  where to store the new NURBS patch object
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_apt_approximatevu(double *cv, int width, int height,
		     int awidth, int aheight,
		     int uorder, int vorder,
		     int uknottype, int vknottype,
		     int closeu, int closev,
		     ay_nurbpatch_object **np)
{
 int ay_status = AY_OK;
 double *mean = NULL;
 int a, b, i, j;
 double *ub = NULL, *vb = NULL;
 double *U = NULL, *V = NULL, *Pt = NULL, *P = NULL, *Q = NULL;
 double *pp;
 ay_lsarrays lsa = {0};

  if(!cv || !np)
    return AY_ENULL;

  if((awidth > width) || (aheight > height))
    return AY_ERROR;

  if(closeu)
    awidth += uorder-1;

  if(closev)
    aheight += vorder-1;

  if(!(Pt = calloc(width*aheight*3, sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  ay_status = ay_npt_avglensv(cv, width, height, 3, &mean);

  if(ay_status)
    goto cleanup;

  ay_status = ay_apt_createknots(mean, height, vknottype, aheight,
				 vorder, closev,
				 &vb, &V);

  if(ay_status)
    goto cleanup;

  a = 0;
  b = 0;
  for(i = 0; i < width; i++)
    {
      pp = &(Pt[b]);
      ay_status = ay_apt_leastSquares(&(cv[a]), 3, 3, height, aheight,
				      vorder-1, closev,
				      &lsa, vb, V, &pp);

      if(ay_status)
	goto cleanup;

      b += aheight*3;
      a += height*3;
    }

  free(mean);
  mean = NULL;

  ay_status = ay_npt_avglensu(Pt, width, aheight, 3, &mean);

  if(ay_status)
    goto cleanup;

  ay_status = ay_apt_createknots(mean, width, uknottype, awidth,
				 uorder, closeu,
				 &ub, &U);

  if(ay_status)
    goto cleanup;

  if(!(Q = calloc(awidth*4, sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  if(!(P = malloc(awidth*aheight*4*sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  for(i = 0; i < aheight; i++)
    {
      a = i*3;
      ay_status = ay_apt_leastSquares(&(Pt[a]), 3*aheight, 4, width, awidth,
				      uorder-1, closeu, &lsa, ub, U, &Q);

      if(ay_status)
	goto cleanup;

      b = i*4;
      for(j = 0; j < awidth; j++)
	{
	  memcpy(&(P[b]), &(Q[j*4]), 4*sizeof(double));
	  b += aheight*4;
	}
    }

  ay_status = ay_npt_create(uorder, vorder, awidth, aheight,
			    AY_KTCUSTOM, AY_KTCUSTOM,
			    P, U, V, np);

cleanup:

  if(ay_status)
    {
      if(P)
	free(P);
      if(U)
	free(U);
      if(V)
	free(V);
    }

  if(Pt)
    free(Pt);
  if(mean)
    free(mean);
  if(Q)
    free(Q);
  if(ub)
    free(ub);
  if(vb)
    free(vb);

 return ay_status;
} /* ay_apt_approximatevu */


/** ay_apt_approximateu:
 * Approximate a 2D array of data points with a NURBS surface of
 * desired width, height, and orders in U direction.
 *
 * \param[in] cv  control points to approximate [width*height*3]
 * \param[in] width  width of \a cv
 * \param[in] height  height of \a cv
 * \param[in] awidth  desired approximation width,
 *            must be <= \a width
 * \param[in] uorder  desired approximation order in U direction,
 *            must be <= \a awidth (unchecked)
 * \param[in] vorder  desired order in V direction
 * \param[in] uknottype  desired knot type for U direction,
 *            must be 0 - chordal or 1 - centripetal
 * \param[in] vknottype  desired knot type for V direction,
 *            must be 0 - chordal or 1 - centripetal
 * \param[in] closeu  create a closed surface in U direction
 * \param[in] closev  create a closed surface in V direction
 * \param[in,out] np  where to store the new NURBS patch object
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_apt_approximateu(double *cv, int width, int height,
		    int awidth,
		    int uorder, int vorder,
		    int uknottype, int vknottype,
		    int closeu, int closev,
		    ay_nurbpatch_object **np)
{
 int ay_status = AY_OK;
 double *mean = NULL;
 int a, b, i, j;
 double *ub = NULL;
 double *U = NULL, *P = NULL, *Q = NULL;
 ay_lsarrays lsa = {0};

  if(!cv || !np)
    return AY_ENULL;

  if(awidth > width)
    return AY_ERROR;

  if(closeu)
    awidth += uorder-1;

  if(!(P = calloc(awidth*height*4, sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  ay_status = ay_npt_avglensu(cv, width, height, 3, &mean);

  if(ay_status)
    goto cleanup;

  ay_status = ay_apt_createknots(mean, width, uknottype, awidth,
				 uorder, closeu,
				 &ub, &U);

  if(ay_status)
    goto cleanup;

  if(!(Q = calloc(awidth*4, sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  a = 0;
  for(i = 0; i < height; i++)
    {
      ay_status = ay_apt_leastSquares(&(cv[a]), height*3, 4, width, awidth,
				      uorder-1, closeu,
				      &lsa, ub, U, &Q);

      if(ay_status)
	goto cleanup;

      b = i*4;
      for(j = 0; j < awidth; j++)
	{
	  memcpy(&(P[b]), &(Q[j*4]), 4*sizeof(double));
	  /*printf("%d,%d: %lg %lg %lg\n",i,j,Q[j*3],Q[j*3+1],Q[j*3+2]);*/
	  b += height*4;
	}
      a += 3;
    }

  ay_status = ay_npt_create(uorder, vorder, awidth, height,
			    AY_KTCUSTOM, vknottype?AY_KTCENTRI:AY_KTCHORDAL,
			    P, U, NULL, np);

cleanup:

  if(ay_status)
    {
      if(P)
	free(P);
      if(U)
	free(U);
    }

  if(ub)
    free(ub);

  if(mean)
    free(mean);

  if(Q)
    free(Q);

 return ay_status;
} /* ay_apt_approximateu */


/** ay_apt_approximatev:
 * Approximate a 2D array of data points with a NURBS surface of
 * desired width, height, and orders in V direction.
 *
 * \param[in] cv  control points to approximate [width*height*3]
 * \param[in] width  width of \a cv
 * \param[in] height  height of \a cv
 * \param[in] aheight  desired approximation height,
 *            must be <= \a height
 * \param[in] uorder  desired order in U direction,
 *            must be <= \a width (unchecked)
 * \param[in] vorder  desired approximation order in V direction,
 *            must be <= \a aheight (unchecked)
 * \param[in] uknottype  desired knot type for U direction,
 *            must be 0 - chordal or 1 - centripetal
 * \param[in] vknottype  desired knot type for V direction,
 *            must be 0 - chordal or 1 - centripetal
 * \param[in] closeu  create a closed surface in U direction
 * \param[in] closev  create a closed surface in V direction
 * \param[in,out] np  where to store the new NURBS patch object
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_apt_approximatev(double *cv, int width, int height,
		    int aheight,
		    int uorder, int vorder,
		    int uknottype, int vknottype,
		    int closeu, int closev,
		    ay_nurbpatch_object **np)
{
 int ay_status = AY_OK;
 double *mean = NULL;
 int a, b, i;
 double *vb = NULL;
 double *V = NULL, *P = NULL;
 double *pp;
 ay_lsarrays lsa = {0};

  if(!cv || !np)
    return AY_ENULL;

  if(aheight > height)
    return AY_ERROR;

  if(closev)
    aheight += vorder-1;

  if(!(P = malloc(width*aheight*4*sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  ay_status = ay_npt_avglensv(cv, width, height, 3, &mean);

  if(ay_status)
    goto cleanup;

  ay_status = ay_apt_createknots(mean, height, vknottype, aheight,
				 vorder, closev,
				 &vb, &V);

  if(ay_status)
    goto cleanup;

  a = 0;
  b = 0;
  for(i = 0; i < width; i++)
    {
      pp = &(P[b]);
      ay_status = ay_apt_leastSquares(&(cv[a]), 3, 4, height, aheight,
				      vorder-1, closev,
				      &lsa, vb, V, &pp);

      if(ay_status)
	goto cleanup;

      b += aheight*4;
      a += height*3;
    }

  ay_status = ay_npt_create(uorder, vorder, width, aheight,
			    uknottype?AY_KTCENTRI:AY_KTCHORDAL, AY_KTCUSTOM,
			    P, NULL, V, np);

cleanup:

  if(ay_status)
    {
      if(P)
	free(P);
      if(V)
	free(V);
    }

  if(vb)
    free(vb);

  if(mean)
    free(mean);

 return ay_status;
} /* ay_apt_approximatev */


/** ay_apt_approxtcmd:
 *  Approximate the selected NURBS patches along U/V/UV/VU.
 *  Implements the \a approxNP scripting interface command.
 *  See also the corresponding section in the \ayd{scapproxnp}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_apt_approxtcmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 int notify_parent = AY_FALSE, i = 1, a, b, tp = 0, tpu = 0, tpv = 0;
 ay_object *o = NULL;
 ay_nurbpatch_object *patch = NULL, *apatch = NULL;
 ay_list_object *sel = ay_selection;
 int awh, mode = 0, order, ktype, closed;
 int awidth = 4, uorder = 4, uktype = AY_KTCHORDAL, uclosed = AY_FALSE;
 int aheight = 4, vorder = 4, vktype = AY_KTCHORDAL, vclosed = AY_FALSE;
 int tessw, tessh;
 double *cv, *tessv = NULL;

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  /* parse args */
  while(i+1 < argc)
    {
      if(argv[i] && argv[i][0] != '\0')
	{
	  switch(argv[i][1])
	    {
	    case 'a':
	      /* -awidth/-aheight */
	      tcl_status = Tcl_GetInt(interp, argv[i+1], &awh);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      if(argv[i][2] == 'w')
		awidth = awh;
	      else
		aheight = awh;
	      break;

	    case 'c':
	      /* -closed */
	      tcl_status = Tcl_GetBoolean(interp, argv[i+1], &closed);
	      if(tcl_status != TCL_OK)
		tcl_status = Tcl_GetInt(interp, argv[i+1], &closed);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      if(strchr(argv[i], 'u') != NULL)
		uclosed = closed;
	      else
		vclosed = closed;
	      break;

	    case 'm':
	      /* -mode */
	      tcl_status = Tcl_GetInt(interp, argv[i+1], &mode);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      break;

	    case 'o':
	      /* -order */
	      tcl_status = Tcl_GetInt(interp, argv[i+1], &order);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      if(order <= 2)
		{
		  ay_error(AY_ERROR, argv[0], "Order must be > 2.");
		  return TCL_OK;
		}
	      if(strchr(argv[i], 'u') != NULL)
		uorder = order;
	      else
		vorder = order;
	      break;

	    case 'k':
	      /* -ktype */
	      tcl_status = Tcl_GetInt(interp, argv[i+1], &ktype);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      if(strchr(argv[i], 'u') != NULL)
		uktype = ktype;
	      else
		vktype = ktype;
	      break;

	    case 't':
	      if(!strcmp(argv[i], "-tpu"))
		{
		  tcl_status = Tcl_GetInt(interp, argv[i+1], &tpu);
		  AY_CHTCLERRRET(tcl_status, argv[0], interp);
		}
	      else
		{
		  if(!strcmp(argv[i], "-tpv"))
		    {
		      tcl_status = Tcl_GetInt(interp, argv[i+1], &tpv);
		      AY_CHTCLERRRET(tcl_status, argv[0], interp);
		    }
		  else
		    {
		      /* -tessparam */
		      tcl_status = Tcl_GetInt(interp, argv[i+1], &tp);
		      AY_CHTCLERRRET(tcl_status, argv[0], interp);
		      tpu = tp;
		      tpv = tp;
		    }
		}
	      break;

	    default:
	      break;
	    } /* switch */
	} /* if */
      i += 2;
    } /* while */

  while(sel)
    {
      o = sel->object;
      if(o->type == AY_IDNPATCH)
	{
	  patch = (ay_nurbpatch_object*)o->refine;

	  if((mode == 0 || mode == 1) && tp == 0)
	    {
	      if(!(cv = malloc(patch->width*patch->height*3*sizeof(double))))
		{
		  ay_error(AY_EOMEM, argv[0], NULL);
		  return TCL_OK;
		}
	      for(i = 0; i < patch->width*patch->height; i++)
		{
		  memcpy(&(cv[i*3]), &(patch->controlv[i*4]), 3*sizeof(double));
		}
	      tessw = patch->width;
	      tessh = patch->height;
	    }
	  else
	    {
	      switch(mode)
		{
		case 0:
		  tpu = tp;
		  tpv = 2;
		  break;
		case 1:
		  tpu = 2;
		  tpv = tp;
		  break;
		default:
		  break;
		}
	      tessv = NULL;
	      /* XXXX Todo: replace by SurfacePoints(3/4)DM if available */
	      if(patch->is_rat)
		{
		  ay_status = ay_stess_SurfacePoints4D(
					      patch->width, patch->height,
					      patch->uorder-1, patch->vorder-1,
					      patch->uknotv, patch->vknotv,
					      patch->controlv, tpu, tpv,
					      &tessw, &tessh, &tessv);
		}
	      else
		{
		  ay_status = ay_stess_SurfacePoints3D(
                                              patch->width, patch->height,
					      patch->uorder-1, patch->vorder-1,
					      patch->uknotv, patch->vknotv,
					      patch->controlv, tpu, tpv,
					      &tessw, &tessh, &tessv);
		} /* if israt */

	      if(ay_status || !tessv)
		{
		  ay_error(AY_EWARN, argv[0], "Tesselation failed.");
		  return TCL_OK;
		}

	      if(!(cv = malloc(tessw*tessh*3*sizeof(double))))
		{
		  ay_error(AY_EOMEM, argv[0], NULL);
		  return TCL_OK;
		}
	      a = 0;
	      b = 0;
	      for(i = 0; i < tessw*tessh; i++)
		{
		  memcpy(&(cv[b]), &(tessv[a]), 3*sizeof(double));
		  a += 6;
		  b += 3;
		}
	      free(tessv);
	    } /* if tesselate */

	  apatch = NULL;
	  switch(mode)
	    {
	    case 0:
	      if(vorder > tessh)
		vorder = tessh;
	      ay_status = ay_apt_approximateu(cv, tessw, tessh,
					      awidth, uorder, vorder,
					      uktype, vktype,
					      uclosed, vclosed,
					      &apatch);
	      break;
	    case 1:
	      if(uorder > tessw)
		uorder = tessw;
	      ay_status = ay_apt_approximatev(cv, tessw, tessh,
					      aheight, uorder, vorder,
					      uktype, vktype,
					      uclosed, vclosed,
					      &apatch);
	      break;
	    case 2:
	      ay_status = ay_apt_approximateuv(cv, tessw,tessh,
					       awidth, aheight, uorder, vorder,
					       uktype, vktype,
					       uclosed, vclosed,
					       &apatch);
	      break;
	    case 3:
	      ay_status = ay_apt_approximatevu(cv, tessw, tessh,
					       awidth, aheight, uorder, vorder,
					       uktype, vktype,
					       uclosed, vclosed,
					       &apatch);
	      break;
	    default:
	      break;
	    } /* switch mode */

	  if(cv)
	    {
	      free(cv);
	      cv = NULL;
	    }

	  if(apatch)
	    {
	      ay_npt_destroy(o->refine);
	      o->refine = apatch;
	    }

	  if(!ay_status)
	    {
	      ay_npt_recreatemp(apatch);

	      /* remove all selected points */
	      if(o->selp)
		ay_selp_clear(o);

	      o->modified = AY_TRUE;

	      /* re-create tesselation of patch */
	      (void)ay_notify_object(o);

	      notify_parent = AY_TRUE;
	    }
	  else
	    {
	      ay_error(AY_ERROR, argv[0], "Approximation failed.");
	      goto cleanup;
	    }
	}
      else
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	} /* if is NPatch */
      sel = sel->next;
    } /* while */

cleanup:
  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_apt_approxtcmd */
