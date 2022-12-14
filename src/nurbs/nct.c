/*
 * Ayam, a free 3D modeler for the RenderMan interface.
 *
 * Ayam is copyrighted 1998-2001 by Randolf Schultz
 * (randolf.schultz@gmail.com) and others.
 *
 * All rights reserved.
 *
 * See the file License for details.
 *
 */

#include "ayam.h"

/** \file nct.c \brief NURBS curve tools */

/* global variables for this module: */


/* local types: */
typedef void (ay_nct_gndcb) (char dir, ay_nurbcurve_object *nc,
			     double *p, double **dp);

/* prototypes of functions local to this module: */
int ay_nct_offsetsection(ay_object *o, double offset,
			 ay_nurbcurve_object **nc);

int ay_nct_splitdisc(ay_object *src, double u, ay_object **result);

void ay_nct_gnd(char dir, ay_nurbcurve_object *nc, double *p,
		double **dp);

void ay_nct_gndc(char dir, ay_nurbcurve_object *nc, double *p,
		 double **dp);

void ay_nct_gndp(char dir, ay_nurbcurve_object *nc, double *p,
		 double **dp);

/* local variables: */
char ay_nct_ncname[] = "NCurve";


/* functions: */

/** ay_nct_create:
 *  Allocate and fill a \a ay_nurbcurve_object structure.
 *  (create a NURBS curve object)
 *
 * \param[in] order  Order of new curve (2 - 10, unchecked)
 * \param[in] length  Length of new curve (2 - 1000, unchecked)
 * \param[in] knot_type  Knot type of new curve (AY_KT*)
 * \param[in] controlv  Pointer to control points [length*4],
 *                      may be NULL
 * \param[in] knotv  Pointer to knots [length+order],
 *                   may be NULL, unless knot_type is AY_KTCUSTOM
 *
 * \param[in,out] curveptr  where to store the new NURBS curve object
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_create(int order, int length, int knot_type,
	      double *controlv, double *knotv,
	      ay_nurbcurve_object **curveptr)
{
 int ay_status = AY_OK;
 double *newcontrolv = NULL;
 ay_nurbcurve_object *curve = NULL;

  if(!(curve = calloc(1, sizeof(ay_nurbcurve_object))))
    return AY_EOMEM;

  curve->order = order;
  curve->length = length;
  curve->knot_type = knot_type;

  if(!controlv)
    {
      if(!(newcontrolv = calloc(4*length, sizeof(double))))
	{
	  free(curve);
	  return AY_EOMEM;
	}

      curve->controlv = newcontrolv;
    }
  else
    {
      curve->controlv = controlv;
      /* XXXX check user supplied control vector? */
      curve->is_rat = ay_nct_israt(curve);
    } /* if */

  if((knot_type != AY_KTCUSTOM) && !knotv)
    {
      /* we need to create knots */
      ay_status = ay_knots_createnc(curve);
      if(ay_status)
	{
	  if(newcontrolv)
	    free(newcontrolv);
	  free(curve);
	  return ay_status;
	}
    }
  else
    {
      /* caller specified own knots */
      curve->knotv = knotv;
    } /* if */

  if(controlv)
    {
      ay_nct_settype(curve);
    }

  /* return result */
  *curveptr = curve;

 return AY_OK;
} /* ay_nct_create */


/** ay_nct_destroy:
 *  gracefully destroy a NURBS curve object
 *
 * \param[in,out] curve  NURBS curve object to destroy
 */
void
ay_nct_destroy(ay_nurbcurve_object *curve)
{

  if(!curve)
    return;

  /* free multipe points */
  if(curve->mpoints)
    ay_nct_clearmp(curve);

  /* free gluNurbsRenderer */
  if(curve->no)
    gluDeleteNurbsRenderer(curve->no);

  /* free (simple) tesselations */
  if(curve->stess[0].tessv)
    free(curve->stess[0].tessv);
  if(curve->stess[1].tessv)
    free(curve->stess[1].tessv);

  if(curve->breakv)
    free(curve->breakv);

  if(curve->fltcv)
    free(curve->fltcv);

  if(curve->controlv)
    free(curve->controlv);

  if(curve->knotv)
    free(curve->knotv);

  free(curve);

 return;
} /* ay_nct_destroy */


/** ay_nct_clearmp:
 * delete all mpoints from curve
 *
 * \param[in,out] curve  NURBS curve object
 */
void
ay_nct_clearmp(ay_nurbcurve_object *curve)
{
 ay_mpoint *next = NULL, *p = NULL;

  if(!curve)
    return;

  p = curve->mpoints;

  while(p)
    {
      next = p->next;
      if(p->points)
	free(p->points);
      if(p->indices)
	free(p->indices);
      free(p);
      p = next;
    } /* while */

  curve->mpoints = NULL;

 return;
} /* ay_nct_clearmp */


/** ay_nct_recreatemp:
 * recreate mpoints of curve from identical control points
 *
 * \param[in,out] nc  NURBS curve object
 */
void
ay_nct_recreatemp(ay_nurbcurve_object *nc)
{
 ay_mpoint *p = NULL, *new = NULL;
 double *ta, *tb, **tmpp = NULL;
 unsigned int *tmpi = NULL;
 int found = AY_FALSE, i, j, count;
 int stride = 4;

  if(!nc)
    return;

  ay_nct_clearmp(nc);

  if(!nc->createmp)
    return;

  if(!(tmpp = calloc(nc->length, sizeof(double *))))
    { goto cleanup; }
  if(!(tmpi = calloc(nc->length, sizeof(unsigned int))))
    { goto cleanup; }

  ta = nc->controlv;
  for(j = 0; j < (nc->length-1); j++)
    {
      tb = ta;

      /* count identical points */
      count = 0;
      for(i = j; i < nc->length; i++)
	{
	  if(AY_V4COMP(ta, tb))
	    {
	      tmpp[count] = tb;
	      tmpi[count] = i;
	      count++;
	    }
	  tb += stride;
	} /* for */

      /* create new mp, if it is not already there */
      if(count > 1)
	{
	  p = nc->mpoints;
	  found = AY_FALSE;
	  while(p && !found)
	    {
	      if(AY_V4COMP(ta, p->points[0]))
		{
		  found = AY_TRUE;
		  break;
		}

	      p = p->next;
	    } /* while */

	  if(!found)
	    {
	      if(!(new = calloc(1, sizeof(ay_mpoint))))
		{ goto cleanup; }
	      if(!(new->points = malloc(count*sizeof(double *))))
		{ goto cleanup; }
	      if(!(new->indices = malloc(count*sizeof(unsigned int))))
		{ goto cleanup; }
	      new->multiplicity = count;
	      memcpy(new->points, tmpp, count*sizeof(double *));
	      memcpy(new->indices, tmpi, count*sizeof(unsigned int));

	      new->next = nc->mpoints;
	      nc->mpoints = new;
	      new = NULL;
	    } /* if */
	} /* if count */

      ta += stride;
    } /* for */

cleanup:

  if(tmpp)
    free(tmpp);
  if(tmpi)
    free(tmpi);

  if(new)
    {
      if(new->points)
	free(new->points);
      if(new->indices)
	free(new->indices);
      free(new);
    }

 return;
} /* ay_nct_recreatemp */


/** ay_nct_collapseselp:
 * collapse selected points of NURBS curve
 *
 * \param[in,out] o  NURBS curve object
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_collapseselp(ay_object *o)
{
 int ay_status = AY_OK;
 ay_nurbcurve_object *nc = NULL;
 ay_mpoint *new = NULL, *p = NULL, *t = NULL, **last = NULL;
 ay_point *selp = NULL;
 double *first = NULL;
 int count = 0, i, found = AY_FALSE;
 char fname[] = "collapseselp";

  if(!o)
    return AY_ENULL;

  if(o->type != AY_IDNCURVE)
    {
      ay_error(AY_EWTYPE, fname, ay_nct_ncname);
      return AY_ERROR;
    }

  nc = (ay_nurbcurve_object *)o->refine;

  selp = o->selp;

  /* count points to collapse */
  while(selp)
    {
      count++;
      selp = selp->next;
    }

  if((!o->selp) || (count < 2))
    {
      ay_error(AY_ERROR, fname, "Select (<t>ag) at least two points first.");
      return AY_ERROR;
    }

  if(!(new = calloc(1, sizeof(ay_mpoint))))
    return AY_EOMEM;
  if(!(new->points = calloc(count, sizeof(double *))))
    { free(new); return AY_EOMEM; }
  if(!(new->indices = calloc(count, sizeof(unsigned int))))
    { free(new->points); free(new); return AY_EOMEM; }

  /* fill mpoint */
  selp = o->selp;
  i = 0;
  first = selp->point;
  while(selp)
    {
      new->points[i] = selp->point;
      new->indices[i] = selp->index;
      i++;
      if(selp->point != first)
	{
	  if(selp->type == AY_PTRAT)
	    memcpy(selp->point, first, 4*sizeof(double));
	  else
	    memcpy(selp->point, first, 3*sizeof(double));
	}
      selp = selp->next;
    } /* while */
  new->multiplicity = count;

  /* find and delete all mpoints eventually
     containing a selected point */
  selp = o->selp;
  while(selp)
    {
      p = nc->mpoints;
      last = &(nc->mpoints);
      while(p)
	{
	  found = AY_FALSE;
	  for(i = 0; i < p->multiplicity; i++)
	    {
	      if(p->points[i] == selp->point)
		found = AY_TRUE;
	    }

	  if(found)
	    {
	      *last = p->next;
	      t = p->next;
	      free(p->points);
	      free(p->indices);
	      free(p);
	      p = t;
	    }
	  else
	    {
	      last = &(p->next);
	      p = p->next;
	    } /* if */
	} /* while */
      selp = selp->next;
    } /* while */

  /* link new mpoint */
  new->next = nc->mpoints;
  nc->mpoints = new;

 return ay_status;
} /* ay_nct_collapseselp */


/** ay_nct_explodemp:
 * explode selected mpoints of NURBS curve
 *
 * \param[in,out] o  NURBS curve object
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_explodemp(ay_object *o)
{
 int ay_status = AY_OK;
 ay_nurbcurve_object *nc = NULL;
 ay_mpoint *p = NULL, *t = NULL, **last = NULL;
 ay_point *selp = NULL;
 int found = AY_FALSE, i, err = AY_TRUE;
 char fname[] = "explodemp";

  if(!o)
    return AY_ENULL;

  if(o->type != AY_IDNCURVE)
    {
      ay_error(AY_EWTYPE, fname, ay_nct_ncname);
      return AY_ERROR;
    }

  nc = (ay_nurbcurve_object *)o->refine;

  selp = o->selp;

  if(!selp)
    {
      ay_error(AY_ERROR, fname, "Select (<t>ag) some multiple points first.");
      return AY_ERROR;
    }

  /* we simply delete all mpoints, that contain a selected point */
  while(selp)
    {
      p = nc->mpoints;
      last = &(nc->mpoints);
      while(p)
	{
	  found = AY_FALSE;
	  for(i = 0; i < p->multiplicity; i++)
	    {
	      if(p->points[i] == selp->point)
		{
		  found = AY_TRUE;
		}
	    } /* for */

	  if(found)
	    {
	      *last = p->next;
	      t = p->next;
	      free(p->points);
	      free(p->indices);
	      free(p);
	      p = t;
	      err = AY_FALSE;
	    }
	  else
	    {
	      last = &(p->next);
	      p = p->next;
	    } /* if */
	} /* while */
      selp = selp->next;
    } /* while */

  if(err)
    {
      ay_status = AY_EWARN;
    }

 return ay_status;
} /* ay_nct_explodemp */


/** ay_nct_resize:
 *  Change the number of control points of a NURBS curve.
 *  Note that the knot vector of the curve is not adapted; this can
 *  be done by computing a new knot vector via ay_knots_createnc().
 *
 * \param[in,out] curve  NURBS curve object
 * \param[in] new_length  new length of curve
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_resize(ay_nurbcurve_object *curve, int new_length)
{
 int ay_status = AY_OK;
 char fname[] = "nct_resize";
 int a, b, i, j;
 int *newpersec = NULL, new = 0;
 double *ncontrolv = NULL, v[3] = {0}, t = 0.0, *cv = NULL;

  if(!curve)
    return AY_ENULL;

  /* nothing to do? */
  if(new_length == curve->length)
    return AY_OK;

  if(!(ncontrolv = malloc(4*new_length*sizeof(double))))
    return AY_EOMEM;

  if(new_length < curve->length)
    {
      a = 0;
      for(i = 0; i < new_length; i++)
	{
	  memcpy(&(ncontrolv[a]), &(curve->controlv[a]), 4*sizeof(double));
	  a += 4;
	}
    }
  else
    {
      /* distribute new points */
      new = new_length - curve->length;

      if(!(newpersec = calloc((curve->length-1), sizeof(int))))
	{
	  free(ncontrolv);
	  return AY_EOMEM;
	}
      cv = curve->controlv;

      while(new)
	{
	  for(i = 0; i < (curve->length-1); i++)
	    {
	      if((cv[i*4] != cv[(i+1)*4]) ||
		 (cv[i*4+1] != cv[(i+1)*4+1]) ||
		 (cv[i*4+2] != cv[(i+1)*4+2]))
		{
		  if(!((curve->type == AY_CTPERIODIC) &&
		       (i > (curve->length-curve->order))))
		    if(new)
		      {
			(newpersec[i])++;
			new--;
		      } /* if */
		} /* if */
	    } /* for */
	} /* while */

      a = 0;
      b = 0;
      for(i = 0; i < (curve->length-1); i++)
	{
	  memcpy(&(ncontrolv[b]), &(curve->controlv[a]), 4*sizeof(double));
	  b += 4;

	  if((cv[i*4] != cv[(i+1)*4]) ||
	     (cv[i*4+1] != cv[(i+1)*4+1]) ||
	     (cv[i*4+2] != cv[(i+1)*4+2]))
	    {
	      if(!((curve->type == AY_CTPERIODIC) &&
		   (i > (curve->length-curve->order))))
		{
		  for(j = 1; j <= newpersec[i]; j++)
		    {
		      v[0] = curve->controlv[a+4] - curve->controlv[a];
		      v[1] = curve->controlv[a+4+1] - curve->controlv[a+1];
		      v[2] = curve->controlv[a+4+2] - curve->controlv[a+2];

		      t = j/(newpersec[i]+1.0);

		      AY_V3SCAL(v,t);

		      ncontrolv[b] = curve->controlv[a]+v[0];
		      ncontrolv[b+1] = curve->controlv[a+1]+v[1];
		      ncontrolv[b+2] = curve->controlv[a+2]+v[2];
		      ncontrolv[b+3] = 1.0;

		      b += 4;
		    } /* for */
		} /* if */
	    } /* if */

	  a += 4;
	} /* for */

      memcpy(&(ncontrolv[(new_length-1)*4]),
	     &(curve->controlv[(curve->length-1)*4]),
	     4*sizeof(double));

      free(newpersec);
    } /* if add points */

  free(curve->controlv);
  curve->controlv = ncontrolv;

  curve->length = new_length;

  if(curve->type)
    {
      ay_status = ay_nct_close(curve);
      if(ay_status)
	ay_error(AY_ERROR, fname, "Could not close curve.");
    }

 return ay_status;
} /* ay_nct_resize */


/** ay_nct_close:
 *  close a NURBS curve, or make it periodic (according to
 *  the current value of the type field)
 *  If closing fails because there are not enough control
 *  points in the curve, the curve type will be reset to "open".
 *
 *  Does not maintain multiple points or calculate new knots!
 *
 * \param[in,out] curve  NURBS curve to close
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_close(ay_nurbcurve_object *curve)
{
 int ay_status = AY_OK;
 double *end = NULL;

  if(!curve)
    return AY_ENULL;

  switch(curve->type)
    {
    case AY_CTCLOSED:
      /* close curve */
      if(curve->length > 2)
	{
	  end = &(curve->controlv[(curve->length*4)-4]);
	  memcpy(end, curve->controlv, 4*sizeof(double));
	}
      else
	{
	  curve->type = AY_CTOPEN;
	  ay_status = AY_ERROR;
	} /* if */
      break;
    case AY_CTPERIODIC:
      /* make curve periodic */
      if(curve->length >= ((curve->order-1)*2))
	{
	  end = &(curve->controlv[(curve->length-(curve->order-1))*4]);
	  memcpy(end, curve->controlv, (curve->order-1)*4*sizeof(double));
	}
      else
	{
	  curve->type = AY_CTOPEN;
	  ay_status = AY_ERROR;
	} /* if */
      break;
    default:
      break;
    } /* switch */

 return ay_status;
} /* ay_nct_close */


/** ay_nct_open:
 *  Move the last control point(s) of a currently closed or
 *  periodic NURBS curve away from the first point(s), effectively
 *  opening the curve.
 *  Curves of type AY_CTOPEN will not be changed.
 *  Does not maintain multiple points or calculate new knots!
 *
 * \param[in,out] curve  NURBS curve to open
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_open(ay_nurbcurve_object *curve)
{
 int ay_status = AY_OK;
 int stride = 4, i;
 double *end = NULL, *d, *f, v[3] = {0}, u[3] = {0};
 ay_nct_gndcb *gndcb = ay_nct_gnd;

  if(!curve)
    return AY_ENULL;

  switch(curve->type)
    {
    case AY_CTCLOSED:
      gndcb = ay_nct_gndc;
      end = &(curve->controlv[(curve->length*4)-4]);
      gndcb(1, curve, end, &d);
      if(!d)
	return AY_ERROR;
      AY_V3SUB(v, d, end);
      AY_V3SCAL(v, 0.2);
      while(d != end)
	{
	  AY_V3ADD(end, end, v);
	  end -= stride;
	}
      break;
    case AY_CTPERIODIC:
      gndcb = ay_nct_gndp;
      end = &(curve->controlv[(curve->length*4)-4]);
      gndcb(0, curve, end, &d);
      if(!d)
	return AY_ERROR;
      f = end-stride;
      for(i = 0; i < curve->order-1; i++)
	{
	  AY_V3SUB(u, d, end);
	  AY_V3SUB(v, f, end);
	  AY_V3ADD(u, u, v);
	  AY_V3SCAL(u, -0.1);
	  AY_V3ADD(end, end, u);
	  end -= stride;
	  f -= stride;
	}
      break;
    default:
      break;
    } /* switch */

 return ay_status;
} /* ay_nct_open */


/** ay_nct_revert:
 *  Change the direction of a NURBS curve.
 *
 *  Does not maintain multiple points!
 *
 * \param[in,out] curve  NURBS curve to revert
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_revert(ay_nurbcurve_object *curve)
{
 int ay_status = AY_OK;
 int i, j;
 double dtemp;

  if(!curve)
    return AY_ENULL;

  /* revert knots */
  if(curve->knot_type >= AY_KTCUSTOM)
    {
      ay_status = ay_knots_revert(curve->knotv, curve->length+curve->order);
      if(ay_status)
	return ay_status;
    } /* if */

  /* revert control */
  j = (curve->length - 1)*4;
  i = 0;
  while(i < j)
    {
      dtemp = curve->controlv[j];
      curve->controlv[j] = curve->controlv[i];
      curve->controlv[i] = dtemp;

      dtemp = curve->controlv[j+1];
      curve->controlv[j+1] = curve->controlv[i+1];
      curve->controlv[i+1] = dtemp;

      dtemp = curve->controlv[j+2];
      curve->controlv[j+2] = curve->controlv[i+2];
      curve->controlv[i+2] = dtemp;

      dtemp = curve->controlv[j+3];
      curve->controlv[j+3] = curve->controlv[i+3];
      curve->controlv[i+3] = dtemp;

      i += 4;
      j -= 4;
    } /* while */

 return ay_status;
} /* ay_nct_revert */


/** ay_nct_revertarr:
 *  revert an 1D array of control points in place
 *
 * \param[in,out] cv  array to revert
 * \param[in] cvlen  number of elements in \a cv
 * \param[in] stride  size of an element in \a cv
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_revertarr(double *cv, int cvlen, int stride)
{
 int i, j, k;
 double dtemp;

  if(!cv)
    return AY_ENULL;

  /* revert control */
  j = (cvlen - 1)*stride;
  i = 0;
  while(i < j)
    {
      for(k = 0; k < stride; k++)
	{
	  dtemp = cv[j+k];
	  cv[j+k] = cv[i+k];
	  cv[i+k] = dtemp;
	}
      i += stride;
      j -= stride;
    } /* while */

 return AY_OK;
} /* ay_nct_revertarr */


/** ay_nct_refinekn:
 *  Refine a NURBS curve by inserting knots at the right places,
 *  thus not changing the shape of the curve;
 *  if \a newknotv is NULL, a knot is inserted into every possible
 *  place in the knot vector (multiple knots will not be changed).
 *  The knot type may change in the process.
 *
 * \param[in,out] curve  NURBS curve object to refine
 * \param[in] maintain_ends  Maintain periodic ends
 * \param[in] newknotv  Vector of new knot values, or range to refine
 *                      (may be NULL)
 * \param[in] newknotvlen  Length of newknotv vector; may be -1 to signal
 *                         \a newknotv being a two element vector that
 *                         contains minimum and maximum knot values of a
 *                         selected knot range to refine
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_refinekn(ay_nurbcurve_object *curve, int maintain_ends,
		double *newknotv, int newknotvlen)
{
 int ay_status = AY_OK;
 double u, *X = NULL, *Ubar = NULL, *Qw = NULL, *knotv;
 int count = 0, i, j, start, end;

  if(!curve)
    return AY_ENULL;

  knotv = curve->knotv;
  if(newknotv && (newknotvlen != -1))
    {
      /* use provided new knots */
      if(newknotvlen <= 0)
	return AY_ERROR;

      X = newknotv;
      count = newknotvlen;
    }
  else
    {
      /* calculate new knots */
      start = curve->order-1;
      end = curve->length;

      if(newknotv && (newknotvlen == -1))
	{
	  /* newknotv contains a selected knot range to refine */
	  u = knotv[start];
	  while(u+AY_EPSILON < newknotv[0])
	    {
	      start++;
	      u = knotv[start];
	    }
	  end = start;
	  while(u+AY_EPSILON < newknotv[1])
	    {
	      end++;
	      u = knotv[end];
	    }
	  if(start >= end)
	    return AY_ERROR;
	}

      if(maintain_ends && (curve->type == AY_CTPERIODIC))
	{
	  /* periodic curves get special treatment:
	     do not insert knots that would change
	     the multiple control points at the ends */
	  start += (int)ceil(curve->order/2.0);
	  end -= (int)ceil(curve->order/2.0);

	  if(start >= end || end <= start)
	    return AY_ERROR;
	}

      for(i = start; i < end; i++)
	{
	  if(knotv[i] != knotv[i+1])
	    count++;
	}

      if(count == 0)
	return AY_ERROR;

      if(!(X = malloc(count*sizeof(double))))
	{
	  return AY_EOMEM;
	}
      /* fill X (contains just the new u values) */
      j = 0;
      for(i = start; i < end; i++)
	{
	  if(knotv[i] != knotv[i+1])
	    {
	      X[j] = knotv[i]+((knotv[i+1]-knotv[i])/2.0);
	      j++;
	    }
	} /* for */
    } /* if */

  if(!(Ubar = malloc((curve->length + curve->order + count) *
		     sizeof(double))))
    {
      if(!newknotv)
	free(X);
      return AY_EOMEM;
    }
  if(!(Qw = malloc((curve->length + count)*4*sizeof(double))))
    {
      if(!newknotv)
	free(X);
      free(Ubar);
      return AY_EOMEM;
    }

  if(curve->is_rat)
    (void)ay_nct_euctohom(curve);

  /* fill Ubar & Qw */
  ay_nb_RefineKnotVectCurve(curve->is_rat, curve->length-1, curve->order-1,
			    curve->knotv, curve->controlv,
			    X, count-1, Ubar, Qw);

  if(ay_knots_check(curve->length + count, curve->order,
		    curve->length + count + curve->order, Ubar) != 0)
    {
      free(Ubar);
      free(Qw);
      ay_status = AY_ERROR;
    }
  else
    {
      free(curve->knotv);
      curve->knotv = Ubar;

      free(curve->controlv);
      curve->controlv = Qw;

      curve->length += count;
    }

  if(!newknotv)
    {
      free(X);
    }

  if(curve->is_rat)
    (void)ay_nct_homtoeuc(curve);

  if(!ay_status)
    {
      curve->knot_type = ay_knots_classify(curve->order, curve->knotv,
					   curve->order+curve->length,
					   AY_EPSILON);

      ay_nct_recreatemp(curve);
    }

 return ay_status;
} /* ay_nct_refinekn */


/** ay_nct_refinearray:
 *  Helper to refine an 1D array of control points.
 *
 * \param[in] Pw  array to refine
 * \param[in] len  number of elements in Pw
 * \param[in] stride  size of an element in Pw
 * \param[in] selp  region to be refined, may be NULL
 *
 * \param[in,out] Qw  new refined array
 * \param[in,out] Qwlen  length of new array
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_refinearray(double *Pw, int len, int stride, ay_point *selp,
		   double **Qw, int *Qwlen)
{
 char fname[] = "refinearray";
 double *Q;
 int count;
 int i, j, start = 0, end = len;
 ay_point *endpnt = NULL;

  if(!Qw)
    return AY_ENULL;

  if(selp)
    {
      start = len;
      end = 0;
      while(selp)
	{
	  if(start > (int)selp->index)
	    start = selp->index;
	  if(end < (int)selp->index)
	    {
	      end = selp->index;
	      endpnt = selp;
	    }

	  selp = selp->next;
	}
    }

  if(end >= len)
    end = len-1;

  /* first, count new points... */
  count = 0;
  for(i = start; i < end; i++)
    {
      /* ...taking care of multiple consecutive points, where we
	 do not insert new points... */
      if((fabs(Pw[i*stride]   - Pw[(i+1)*stride])   > AY_EPSILON) ||
	 (fabs(Pw[i*stride+1] - Pw[(i+1)*stride+1]) > AY_EPSILON) ||
	 (fabs(Pw[i*stride+2] - Pw[(i+1)*stride+2]) > AY_EPSILON))
	{
	  count++;
	}
    }

  if(count)
    {
      /* allocate new control vector */
      if(!(Q = malloc((len + count)*stride*sizeof(double))))
	{
	  ay_error(AY_EOMEM, fname, NULL);
	  return AY_ERROR;
	}

      /* copy first points */
      if(start > 0)
	memcpy(Q, Pw, start*stride*sizeof(double));

      /* copy old & create new points */
      i = start;
      j = start;
      while(i < end)
	{
	  memcpy(&(Q[j*stride]), &(Pw[i*stride]), stride*sizeof(double));

	  if((fabs(Pw[i*stride]   - Pw[(i+1)*stride])   > AY_EPSILON) ||
	     (fabs(Pw[i*stride+1] - Pw[(i+1)*stride+1]) > AY_EPSILON) ||
	     (fabs(Pw[i*stride+2] - Pw[(i+1)*stride+2]) > AY_EPSILON))
	    {
	      Q[(j+1)*stride] = Pw[i*stride] +
		((Pw[(i+1)*stride] - Pw[i*stride])/2.0);

	      Q[(j+1)*stride+1] = Pw[i*stride+1] +
		((Pw[(i+1)*stride+1] - Pw[i*stride+1])/2.0);

	      Q[(j+1)*stride+2] = Pw[i*stride+2] +
		((Pw[(i+1)*stride+2] - Pw[i*stride+2])/2.0);

	      if(stride > 3)
		Q[(j+1)*stride+3] = Pw[i*stride+3] +
		  ((Pw[(i+1)*stride+3] - Pw[i*stride+3])/2.0);

	      j++;
	    }

	  i++;
	  j++;
	} /* while */

      /* copy last point(s) */
      memcpy(&(Q[(end+count)*stride]),
	     &(Pw[end*stride]),
	     (len-end)*stride*sizeof(double));

      if(endpnt)
	{
	  endpnt->index += count;
	}

      /* return result */
      *Qw = Q;
      *Qwlen = len + count;
    } /* if count */

 return AY_OK;
} /* ay_nct_refinearray */


/** ay_nct_refinecv:
 *  Refine a NURBS curve by inserting control points at the right places,
 *  changing the shape of the curve.
 *  The knot type does not change, but new knots will be generated.
 *
 * \param[in,out] curve  NURBS curve object to refine
 * \param[in] selp  selected points that define a (single) region to refine,
 *                  may be NULL
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_refinecv(ay_nurbcurve_object *curve, ay_point *selp)
{
 double *Qw = NULL, *Q = NULL, *U = NULL;
 int start, end, count = 0, i, j, k, l, p, npslen = 0;
 int *nps = NULL;
 char fname[] = "nct_refinecv";
 ay_point *endpnt = NULL;

  if(!curve)
    return AY_ENULL;

  if(selp)
    {
      /* adjust start/end to user provided selected points */
      start = curve->length;
      end = 0;
      while(selp)
	{
	  if(start > (int)selp->index)
	    start = selp->index;
	  if(end < (int)selp->index)
	    {
	      end = selp->index;
	      endpnt = selp;
	    }

	  selp = selp->next;
	}
    }
  else
    {
      start = 0;
      end = curve->length;
      if(curve->type == AY_CTPERIODIC)
	{
	  /* special case: curves marked periodic;
	   * we consider the last p multiple points to
	   * not be free and add new points into all
	   * the other sections, the last p points will
	   * then be adjusted by ay_nct_close() to match
	   * the first p points and everything will be fine
	   */
	  end -= (curve->order-1);
	}
    }

  if(end >= curve->length)
    end = curve->length-1;

  /* first, count new points... */
  count = 0;
  Q = curve->controlv;
  for(i = start; i < end; i++)
    {
      /* ...taking care of multiple consecutive points, where we
	 do not insert new points... */
      if((fabs(Q[i*4]   - Q[(i+1)*4])   > AY_EPSILON) ||
	 (fabs(Q[i*4+1] - Q[(i+1)*4+1]) > AY_EPSILON) ||
	 (fabs(Q[i*4+2] - Q[(i+1)*4+2]) > AY_EPSILON))
	{
	  count++;
	}
    }

  if(count)
    {
      /* allocate new control vector */
      if(!(Qw = malloc((curve->length + count)*4*sizeof(double))))
	{
	  ay_error(AY_EOMEM, fname, NULL);
	  return AY_ERROR;
	}

      /* copy first points */
      if(start > 0)
	memcpy(Qw, Q, start*4*sizeof(double));

      /* copy old & create new points */
      i = start;
      j = start;
      while(i < end)
	{
	  memcpy(&(Qw[j*4]), &(Q[i*4]), 4*sizeof(double));

	  if((fabs(Q[i*4]   - Q[(i+1)*4])   > AY_EPSILON) ||
	     (fabs(Q[i*4+1] - Q[(i+1)*4+1]) > AY_EPSILON) ||
	     (fabs(Q[i*4+2] - Q[(i+1)*4+2]) > AY_EPSILON))
	    {
	      Qw[(j+1)*4] = Q[i*4] +
		((Q[(i+1)*4] - Q[i*4])/2.0);

	      Qw[(j+1)*4+1] = Q[i*4+1] +
		((Q[(i+1)*4+1] - Q[i*4+1])/2.0);

	      Qw[(j+1)*4+2] = Q[i*4+2] +
		((Q[(i+1)*4+2] - Q[i*4+2])/2.0);

	      Qw[(j+1)*4+3] = Q[i*4+3] +
		((Q[(i+1)*4+3] - Q[i*4+3])/2.0);

	      j++;
	    }

	  i++;
	  j++;
	} /* while */

      /* copy last point(s) */
      memcpy(&(Qw[(end+count)*4]),
	     &(Q[end*4]),
	     (curve->length-end)*4*sizeof(double));

      /* create new knots */
      if(curve->knot_type != AY_KTCUSTOM)
	{
	  curve->length += count;
	  free(curve->controlv);
	  curve->controlv = Qw;

	  ay_knots_createnc(curve);
	}
      else
	{
	  /* count sections in start-end range */
	  p = curve->order-1;
	  for(i = start+1; i < end+p; i++)
	    {
	      if(fabs(curve->knotv[i]-curve->knotv[i+1]) > AY_EPSILON)
		npslen++;
	    }
	  if(npslen == 0)
	    {
	      free(Qw);
	      return AY_ERROR;
	    }
	  if(!(nps = calloc(npslen, sizeof(int))))
	    {
	      free(Qw);
	      return AY_EOMEM;
	    }
	  /* distribute knots to sections */
	  /* XXXX make the result more symmetric for cases of
	     over-commitment (count>npslen) */
	  i = 0;
	  j = 0;
	  while(i < count)
	    {
	      nps[j]++;
	      j++;
	      if(j == npslen)
		j = 0;
	      i++;
	    }

	  /* allocate and fill new knot vector */
	  if(!(U = malloc((curve->length+curve->order+count)*sizeof(double))))
	    {
	      free(nps);
	      free(Qw);
	      return AY_EOMEM;
	    }

	  /* copy first knots */
	  memcpy(U, curve->knotv, (start+1)*sizeof(double));

	  /* copy middle knots and insert new knots with the
	     help of the sections (nps) array */
	  k = (start+1);
	  l = 0;
	  for(i = (start+1); i < (end+p); i++)
	    {
	      U[k] = curve->knotv[i];
	      k++;
	      while(fabs(curve->knotv[i]-curve->knotv[i+1]) < AY_EPSILON)
		{
		  U[k] = curve->knotv[i];
		  k++;
		  i++;
		}
	      for(j = 0; j < nps[l]; j++)
		{
		  U[k] = curve->knotv[i] +
		    (j+1)*((curve->knotv[i+1] - curve->knotv[i])/(nps[l]+1));
		  k++;
		}
	      l++;
	      if(l >= npslen)
		break;
	    } /* for */

	  /* copy remaining knots */
	  memcpy(&(U[k]), &(curve->knotv[(end+p)]),
		 ((curve->length+curve->order)-(end+p))*sizeof(double));

	  free(curve->knotv);
	  curve->knotv = U;

	  curve->length += count;
	  free(curve->controlv);
	  curve->controlv = Qw;

	  free(nps);
	} /* if custom */

      if(curve->type == AY_CTPERIODIC)
	ay_nct_close(curve);

      /* since we do not create new multiple points
	 we only need to re-create them if there were
	 already multiple points in the original curve */
      if(curve->mpoints)
	ay_nct_recreatemp(curve);

      if(endpnt)
	{
	  endpnt->index += count;
	}
    } /* if count */

 return AY_OK;
} /* ay_nct_refinecv */


/** ay_nct_refinekntcmd:
 *  Refine the knots of selected NURBS curves.
 *  Implements the \a refineknNC scripting interface command.
 *  See also the corresponding section in the \ayd{screfineknnc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_refinekntcmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL;
 ay_nurbcurve_object *curve = NULL;
 int maintain_ends = AY_FALSE, aknotc = 0, k, i;
 int notify_parent = AY_FALSE;
 double *X = NULL, Xmm[2] = {0};
 char **aknotv = NULL;

  if(argc > 1)
    {
      i = 1;
      if(argv[1][0] == '-' && argv[1][0] == 'm')
	{
	  maintain_ends = AY_TRUE;
	  i++;
	}

      Tcl_SplitList(interp, argv[i], &aknotc, &aknotv);

      if(!(X = malloc(aknotc*sizeof(double))))
	{
	  ay_error(AY_EOMEM, argv[0], NULL);
	  if(aknotv)
	    Tcl_Free((char *) aknotv);
	  return TCL_OK;
	}

      for(i = 0; i < aknotc; i++)
	{
	  tcl_status = Tcl_GetDouble(interp, aknotv[i], &X[i]);
	  AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	  if(X[i] != X[i])
	    {
	      ay_error(AY_ERROR, argv[0], "Knot is NaN!");
	      goto cleanup;
	    }
	} /* for */

      Tcl_Free((char *) aknotv);
      aknotv = NULL;
    } /* if have args */

  while(sel)
    {
      o = sel->object;
      o->modified = AY_FALSE;

      if(o->type == AY_IDNCURVE)
	{
	  curve = (ay_nurbcurve_object *)o->refine;
	  k = aknotc;
	  if((aknotc == 0) && ay_tags_hastag(o, ay_umm_tagtype))
	    {
	      ay_status = ay_knots_getuminmax(o, curve->order,
				  curve->length+curve->order, curve->knotv,
				  &(Xmm[0]), &(Xmm[1]));
	      if(!ay_status)
		{
		  k = -1;
		  X = Xmm;
		}
	    }

	  ay_status = ay_nct_refinekn(curve, maintain_ends, X, k);

	  if(ay_status)
	    {
	      goto cleanup;
	    }
	  else
	    {
	      o->modified = AY_TRUE;
	    }
	}
      else
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	} /* if ncurve */

      if(o->modified)
	{
	  if(o->selp)
	    ay_selp_clear(o);

	  (void)ay_notify_object(o);
	  notify_parent = AY_TRUE;
	} /* if modified */

      sel = sel->next;
    } /* while */

cleanup:

  if(ay_status)
    {
      ay_error(AY_ERROR, argv[0], "Refine knots operation failed.");
    }

  if(notify_parent)
    {
      (void)ay_notify_parent();
    }

  if(X && (X != Xmm))
    {
      free(X);
    }

  if(aknotv)
    {
      Tcl_Free((char *) aknotv);
    }

 return TCL_OK;
} /* ay_nct_refinekntcmd */


/** ay_nct_clamp:
 * Change knots and control points of a NURBS curve so that the curve
 * interpolates its respective end control points without changing the
 * current curve shape.
 * It is safe to call this with half clamped curves.
 *
 * \param[in,out] curve  NURBS curve object to clamp
 * \param[in] side  side to clamp:
 *             - 0: clamp both sides
 *             - 1: clamp only start
 *             - 2: clamp only end
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_clamp(ay_nurbcurve_object *curve, int side)
{
 int ay_status = AY_OK;
 double *newcontrolv = NULL, *newknotv = NULL;
 double u;
 int stride = 4, i, j, k, s, nq, rs = 0, re = 0;

  if(!curve)
    return AY_ENULL;

  if(curve->order < 3)
    {
      newknotv = curve->knotv;
      if(side == 0 || side == 1)
	{
	  memcpy(&(newknotv[1]), &(newknotv[0]), sizeof(double));
	}
      if(side == 0 || side == 2)
	{
	  memcpy(&(newknotv[curve->length+curve->order-2]),
		 &(newknotv[curve->length+curve->order-1]),
		 sizeof(double));
	}
      return AY_OK;
    }

  if(side == 0)
    {
      if((curve->knot_type == AY_KTBSPLINE) ||
	 ((curve->type == AY_CTPERIODIC) &&
	  ((curve->knot_type == AY_KTCHORDAL) ||
	   (curve->knot_type == AY_KTCENTRI))))
	{
	  return ay_nct_clampperiodic(curve);
	}
    }

  /* clamp start? */
  if(side == 0 || side == 1)
    {
      /* the next fragment also counts the phantom knot! */
      u = curve->knotv[0];
      s = 1;
      for(i = 1; i < curve->order; i++)
	{
	  if(fabs(u - curve->knotv[i]) < AY_EPSILON)
	    s++;
	  else
	    break;
	}

      if(s < curve->order)
	{
	  /* clamp start */
	  u = curve->knotv[curve->order-1];

	  k = ay_nb_FindSpanMult(curve->length-1, curve->order-1, u,
				 curve->knotv, &s);

	  rs = (curve->order - 1) - s;
	  curve->length += rs;

	  if(!(newcontrolv = malloc(curve->length*stride*sizeof(double))))
	    return AY_EOMEM;

	  if(!(newknotv = malloc((curve->length+curve->order)*sizeof(double))))
	    { free(newcontrolv); return AY_EOMEM; }

	  if(curve->is_rat)
	    ay_status = ay_nb_InsertKnotCurve4D(curve->length-rs-1,
			 curve->order-1, curve->knotv, curve->controlv, u, k,
			 s, rs, &nq, newknotv, newcontrolv);
	  else
	    ay_status = ay_nb_InsertKnotCurve3D(curve->length-rs-1,
			 curve->order-1, curve->knotv, curve->controlv, u, k,
			 s, rs, &nq, newknotv, newcontrolv);

	  if(ay_status)
	    {
	      free(newknotv);
	      free(newcontrolv);
	      return ay_status;
	    }

	  free(curve->controlv);
	  curve->controlv = newcontrolv;

	  free(curve->knotv);
	  curve->knotv = newknotv;
	} /* if */
    } /* if */

  /* clamp end? */
  if(side == 0 || side == 2)
    {
      /* the next fragment also counts the phantom knot! */
      s = 1;
      j = curve->length+curve->order-1;
      u = curve->knotv[j];
      for(i = 1; i < curve->order; i++)
	{
	  if(fabs(u - curve->knotv[j-i]) < AY_EPSILON)
	    s++;
	  else
	    break;
	}

      if(s < curve->order)
	{
	  /* clamp end */
	  u = curve->knotv[curve->length];

	  k = ay_nb_FindSpanMult(curve->length-1, curve->order-1, u,
				 curve->knotv, &s);

	  re = (curve->order - 1) - s;
	  curve->length += re;

	  if(!(newcontrolv = malloc(curve->length*stride*sizeof(double))))
	    return AY_EOMEM;

	  if(!(newknotv = malloc((curve->length+curve->order)*sizeof(double))))
	    { free(newcontrolv); return AY_EOMEM; }

	  if(curve->is_rat)
	    ay_status = ay_nb_InsertKnotCurve4D(curve->length-re-1,
			 curve->order-1, curve->knotv, curve->controlv, u, k,
			 s, re, &nq, newknotv, newcontrolv);
	  else
	    ay_status = ay_nb_InsertKnotCurve3D(curve->length-re-1,
			 curve->order-1, curve->knotv, curve->controlv, u, k,
			 s, re, &nq, newknotv, newcontrolv);

	  if(ay_status)
	    {
	      free(newknotv);
	      free(newcontrolv);
	      return ay_status;
	    }

	  free(curve->controlv);
	  curve->controlv = newcontrolv;

	  free(curve->knotv);
	  curve->knotv = newknotv;
	} /* if */
    } /* if */

  if(rs == 0 && re == 0)
    {
      return AY_OK;
    }

  /* create new controlv, knotv, discarding the first rs and last re knots */
  switch(side)
    {
    case 0:
      curve->length -= rs+re;
      break;
    case 1:
      curve->length -= rs;
      break;
    case 2:
      curve->length -= re;
      break;
    default:
      break;
    }

  if(!(newcontrolv = malloc(curve->length*stride*sizeof(double))))
    return AY_EOMEM;

  if(!(newknotv = malloc((curve->length+curve->order)*sizeof(double))))
    { free(newcontrolv); return AY_EOMEM; }

  switch(side)
    {
    case 0:
    case 1:
      /* clamped both or start: ignore first rs knots */
      memcpy(newcontrolv, &(curve->controlv[rs*stride]),
	     curve->length*stride*sizeof(double));

      memcpy(newknotv, &(curve->knotv[rs]),
	     (curve->length+curve->order)*sizeof(double));
      /* improve phantom knot(s) */
      newknotv[0] = newknotv[1];
      if(side == 0)
	newknotv[curve->length+curve->order-1] =
	  newknotv[curve->length+curve->order-2];
      break;
    case 2:
      /* clamped end: ignore last re knots */
      memcpy(newcontrolv, curve->controlv,
	     curve->length*stride*sizeof(double));

      memcpy(newknotv, curve->knotv,
	     (curve->length+curve->order)*sizeof(double));
      /* improve phantom knot */
      newknotv[curve->length+curve->order-1] =
	newknotv[curve->length+curve->order-2];
      break;
    default:
      break;
    }

  free(curve->controlv);
  curve->controlv = newcontrolv;

  free(curve->knotv);
  curve->knotv = newknotv;

 return AY_OK;
} /* ay_nct_clamp */


/** ay_nct_clampperiodic:
 * This variant of #ay_nct_clamp() above is a fast complete clamp for
 * curves with periodic knot vectors (e.g. AY_KTBSPLINE),
 * always clamps both ends
 *
 * \param[in,out] curve  NURBS curve object to clamp
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_clampperiodic(ay_nurbcurve_object *curve)
{
 int ay_status = AY_OK;
 double *newcontrolv = NULL, *newknotv = NULL;
 int stride = 4, p, np, nq;

  if(!curve)
    return AY_ENULL;

  if(curve->order < 3)
    {
      newknotv = curve->knotv;
      memcpy(&(newknotv[1]), &(newknotv[0]), sizeof(double));
      memcpy(&(newknotv[curve->length+curve->order-2]),
	     &(newknotv[curve->length+curve->order-1]),
	     sizeof(double));
      return AY_OK;
    }

  p = curve->order-1;
  np = curve->length;
  nq = np+(p*2);

  /* get some fresh memory to work on */
  if(!(newcontrolv = malloc(nq*stride*sizeof(double))))
    return AY_EOMEM;

  if(!(newknotv = malloc((nq+curve->order)*sizeof(double))))
    { free(newcontrolv); return AY_EOMEM; }

  /* insert knots at start */
  if(curve->is_rat)
    ay_status = ay_nb_InsertKnotCurve4D(np-1, p, curve->knotv, curve->controlv,
                        curve->knotv[p], p, 0, p, &nq, newknotv, newcontrolv);
  else
    ay_status = ay_nb_InsertKnotCurve3D(np-1, p, curve->knotv, curve->controlv,
                        curve->knotv[p], p, 0, p, &nq, newknotv, newcontrolv);

  if(ay_status)
    {
      free(newknotv);
      free(newcontrolv);
      return ay_status;
    }

  /* insert knots at end (nq is now np+p-1!) */
  if(curve->is_rat)
    ay_status = ay_nb_InsertKnotCurve4D(nq, p, newknotv, newcontrolv,
			newknotv[nq+1], nq, 0, p, &nq, newknotv, newcontrolv);
  else
    ay_status = ay_nb_InsertKnotCurve3D(nq, p, newknotv, newcontrolv,
			newknotv[nq+1], nq, 0, p, &nq, newknotv, newcontrolv);

  if(ay_status)
    {
      free(newknotv);
      free(newcontrolv);
      return ay_status;
    }

  /* copy results back to curve, ignoring the first p and last p cv/knots */
  memcpy(curve->controlv, &(newcontrolv[p*stride]),
	 curve->length*stride*sizeof(double));
  memcpy(curve->knotv, &(newknotv[p]),
	 (curve->length+curve->order)*sizeof(double));

  free(newcontrolv);
  free(newknotv);

  /* correct curve type */
  if(curve->type == AY_CTPERIODIC)
    curve->type = AY_CTCLOSED;

 return AY_OK;
} /* ay_nct_clampperiodic */


/** ay_nct_clamptcmd:
 *  Clamp selected NURBS curves, i.e. change their knots and control points
 *  so that they interpolate their respective end control points without
 *  changing the current curve shapes.
 *  Implements the \a clampNC scripting interface command.
 *  See also the corresponding section in the \ayd{scclampnc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_clamptcmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_nurbcurve_object *curve = NULL;
 int side = 0;
 int notify_parent = AY_FALSE;

  if(argc >= 2)
    {
      if((argv[1][0] == '-') && (argv[1][1] == 's'))
	side = 1;
      else
	if((argv[1][0] == '-') && (argv[1][1] == 'e'))
	  side = 2;
	else
	  {
	    tcl_status = Tcl_GetInt(interp, argv[1], &side);
	    AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  }
    }

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  while(sel)
    {
      if(sel->object->type == AY_IDNCURVE)
	{
	  curve = (ay_nurbcurve_object *)sel->object->refine;

	  /* clamped by nature? */
	  if((curve->knot_type == AY_KTNURB) ||
	     (curve->knot_type == AY_KTBEZIER))
	    {
	      sel = sel->next;
	      continue;
	    }

	  ay_status = ay_nct_clamp(curve, side);

	  if(ay_status)
	    {
	      ay_error(ay_status, argv[0], "Clamp operation failed.");
	      break;
	    }
	  else
	    {
	      curve->knot_type = ay_knots_classify(curve->order, curve->knotv,
						   curve->order+curve->length,
						   AY_EPSILON);

	      /* update pointers to controlv */
	      ay_nct_recreatemp(curve);

	      /* update selected points pointers to controlv */
	      if(sel->object->selp)
		{
		  (void)ay_pact_getpoint(3, sel->object, NULL, NULL);
		}

	      sel->object->modified = AY_TRUE;

	      /* re-create tesselation of curve */
	      (void)ay_notify_object(sel->object);

	      notify_parent = AY_TRUE;
	    } /* if */
	}
      else
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	} /* if */

    sel = sel->next;
  } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_clamptcmd */


/** ay_nct_elevate:
 *  increase the order of a NURBS curve
 *
 * \param[in,out] curve  NURBS curve object to elevate
 * \param[in] new_order  new order
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_elevate(ay_nurbcurve_object *curve, int new_order)
{
 int ay_status = AY_OK;
 char fname[] = "elevate";
 int i, j, a, b, clamp_me;
 double u, *Uh = NULL, *Qw = NULL, *realQw = NULL, *realUh = NULL;
 int t = 1, nh = 0;

  if(!curve)
    return AY_ENULL;

  if(curve->order >= new_order)
    return AY_OK;
  else
    t = new_order - curve->order;

  /* clamp the curve? */
  clamp_me = AY_FALSE;

  if((curve->knot_type == AY_KTBSPLINE) ||
     ((curve->type == AY_CTPERIODIC) &&
      ((curve->knot_type == AY_KTCHORDAL) ||
       (curve->knot_type == AY_KTCENTRI))))
    {
      ay_status = ay_nct_clampperiodic(curve);
    }
  else
    {
      if(curve->knot_type == AY_KTCUSTOM)
	{
	  a = 1;
	  u = curve->knotv[0];
	  for(i = 1; i < curve->order; i++)
	    if(fabs(u - curve->knotv[i]) < AY_EPSILON)
	      a++;

	  j = curve->length+curve->order-1;
	  b = 1;
	  u = curve->knotv[j];
	  for(i = j; i >= curve->length; i--)
	    if(fabs(u - curve->knotv[i]) < AY_EPSILON)
	      b++;

	  if((a < curve->order) || (b < curve->order))
	    {
	      clamp_me = AY_TRUE;
	    } /* if */
	} /* if */
    } /* if */

  if(clamp_me)
    {
      ay_status = ay_nct_clamp(curve, 0);
    } /* if */

  if(ay_status)
    {
      ay_error(AY_ERROR, fname, "Clamp operation failed.");
      return AY_ERROR;
    } /* if */

  /* alloc new knotv & new controlv */
  if(!(Uh = malloc((curve->length + curve->length*t + curve->order + t) *
		   sizeof(double))))
    {
      ay_error(AY_EOMEM, fname, NULL);
      return AY_ERROR;
    }
  if(!(Qw = malloc((curve->length + curve->length*t) * 4 *
		   sizeof(double))))
    {
      free(Uh);
      ay_error(AY_EOMEM, fname, NULL);
      return AY_ERROR;
    }

  /* fill Uh & Qw */
  ay_status = ay_nb_DegreeElevateCurve4D(4, curve->length-1,
					 curve->order-1, curve->knotv,
					 curve->controlv, t, &nh, Uh, Qw);

  if(ay_status || (nh <= 0))
    {
      ay_error(ay_status, fname, "Degree elevation failed.");
      free(Uh);
      free(Qw);
      return AY_ERROR;
    }

  if(!(realQw = realloc(Qw, nh * 4 * sizeof(double))))
    {
      ay_error(AY_EOMEM, fname, NULL);
      free(Uh);
      free(Qw);
      return AY_ERROR;
    }

  if(!(realUh = realloc(Uh, (nh + curve->order + t) * sizeof(double))))
    {
      ay_error(AY_EOMEM, fname, NULL);
      free(Uh);
      free(realQw);
      return AY_ERROR;
    }

  free(curve->knotv);
  curve->knotv = realUh;

  free(curve->controlv);
  curve->controlv = realQw;

  curve->knot_type = AY_KTCUSTOM;

  curve->order += t;

  curve->length = nh;

  Qw = NULL;
  Uh = NULL;
  realQw = NULL;
  realUh = NULL;

  ay_nct_recreatemp(curve);

 return AY_OK;
} /* ay_nct_elevate */


/** ay_nct_elevatetcmd:
 *  Increase/decrease the order of the selected NURBS curves.
 *  Implements the \a elevateNC scripting interface command.
 *  Also implements the \a reduceNC scripting interface command.
 *  See also the corresponding section in the \ayd{scelevatenc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_elevatetcmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_object *o;
 ay_list_object *sel = ay_selection;
 ay_nurbcurve_object *curve = NULL;
 int clamp_me;
 double *Uh = NULL, *Qw = NULL, *realQw = NULL, *realUh = NULL;
 double tol = 0.0;
 int t = 1, nh = 0;
 int degree_reduce = AY_FALSE, notify_parent = AY_FALSE;

  if(argv[0][0] == 'r')
    degree_reduce = AY_TRUE;

  if(argc >= 2)
    {
      if(degree_reduce)
	{
	  tcl_status = Tcl_GetDouble(interp, argv[1], &tol);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  if(tol < 0.0)
	    {
	      ay_error(AY_ERROR, argv[0], "Argument tol must be >= 0.");
	      return TCL_OK;
	    }
	  if(tol != tol)
	    {
	      ay_error_reportnan(argv[0], "tol");
	      return TCL_OK;
	    }
	}
      else
	{
	  tcl_status = Tcl_GetInt(interp, argv[1], &t);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  if(t <= 0)
	    {
	      ay_error(AY_ERROR, argv[0], "Argument must be > 0.");
	      return TCL_OK;
	    }
	}
    }

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  while(sel)
    {
      o = sel->object;
      sel = sel->next;
      if(o->type == AY_IDNCURVE)
	{
	  curve = (ay_nurbcurve_object *)o->refine;

	  if(degree_reduce && curve->order < 3)
	    continue;

	  /* clamp the curve? */
	  clamp_me = AY_FALSE;
	  ay_status = AY_OK;

	  if((curve->knot_type == AY_KTBSPLINE) ||
	     ((curve->type == AY_CTPERIODIC) &&
	      ((curve->knot_type == AY_KTCHORDAL) ||
	       (curve->knot_type == AY_KTCENTRI))))
	    {
	      ay_status = ay_nct_clampperiodic(curve);
	    }
	  else
	    {
	      if(curve->knot_type == AY_KTCUSTOM)
		{
		  clamp_me = AY_TRUE;
		}
	    }

	  if(clamp_me)
	    {
	      ay_status = ay_nct_clamp(curve, 0);

	      if(ay_status)
		{
		  ay_error(AY_ERROR, argv[0], "Clamp operation failed.");
		  return TCL_OK;
		}
	    }

	  /* alloc new knotv & new controlv */
	  if(!(Uh = malloc((curve->length + curve->length*t +
			    curve->order + t) *
			   sizeof(double))))
	    {
	      ay_error(AY_EOMEM, argv[0], NULL);
	      return TCL_OK;
	    }
	  if(!(Qw = malloc((curve->length + curve->length*t)*4*
			   sizeof(double))))
	    {
	      free(Uh);
	      ay_error(AY_EOMEM, argv[0], NULL);
	      return TCL_OK;
	    }

	  /* fill Uh & Qw */
	  if(degree_reduce)
	    {
	      ay_status = ay_nb_DegreeReduceCurve4D(curve->length-1,
					    curve->order-1, curve->knotv,
					    curve->controlv, tol, &nh, Uh, Qw);

	      if(ay_status || (nh <= 0))
		{
		  ay_error(ay_status, argv[0], "Degree reduction failed.");
		  free(Uh); free(Qw);
		  return TCL_OK;
		}
	    }
	  else
	    {
	      ay_status = ay_nb_DegreeElevateCurve4D(4, curve->length-1,
					      curve->order-1, curve->knotv,
					      curve->controlv, t, &nh, Uh, Qw);

	      if(ay_status || (nh <= 0))
		{
		  ay_error(ay_status, argv[0], "Degree elevation failed.");
		  free(Uh); free(Qw);
		  return TCL_OK;
		}
	    }

	  if(!(realQw = realloc(Qw, nh * 4 * sizeof(double))))
	    {
	      free(Uh); free(Qw);
	      ay_error(AY_EOMEM, argv[0], NULL);
	      return TCL_OK;
	    }

	  if(degree_reduce)
	    {
	      if(!(realUh = realloc(Uh, (nh+curve->order-1)*sizeof(double))))
		{
		  free(Uh); free(realQw);
		  ay_error(AY_EOMEM, argv[0], NULL);
		  return TCL_OK;
		}
	      curve->order--;
	    }
	  else
	    {
	      if(!(realUh = realloc(Uh, (nh+curve->order+t)*sizeof(double))))
		{
		  free(Uh); free(realQw);
		  ay_error(AY_EOMEM, argv[0], NULL);
		  return TCL_OK;
		}
	      curve->order += t;
	    }

	  free(curve->knotv);
	  curve->knotv = realUh;

	  free(curve->controlv);
	  curve->controlv = realQw;

	  curve->knot_type = AY_KTCUSTOM;

	  curve->length = nh;

	  Qw = NULL;
	  Uh = NULL;
	  realQw = NULL;
	  realUh = NULL;

	  if(o->selp)
	    ay_selp_clear(o);

	  /* update pointers to controlv */
	  ay_nct_recreatemp(curve);
	  o->modified = AY_TRUE;

	  /* re-create tesselation of curve */
	  (void)ay_notify_object(o);
	  notify_parent = AY_TRUE;
	}
      else
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	} /* if */
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_elevatetcmd */


/** ay_nct_insertkntcmd:
 *  Insert knot into selected NURBS curves.
 *  Implements the \a insknNC scripting interface command.
 *  See also the corresponding section in the \ayd{scinsknnc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_insertkntcmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_object *src = NULL;
 ay_nurbcurve_object *curve = NULL;
 double u, *knots = NULL, *newcontrolv = NULL, *newknotv = NULL;
 int stride = 4, k = 0, s = 0, r = 0, nq = 0;
 int notify_parent = AY_FALSE;

  if(argc < 3)
    {
      ay_error(AY_EARGS, argv[0], "u r");
      return TCL_OK;
    }

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  tcl_status = Tcl_GetDouble(interp, argv[1], &u);
  AY_CHTCLERRRET(tcl_status, argv[0], interp);
  if(u != u)
    {
      ay_error_reportnan(argv[0], "u");
      return TCL_OK;
    }

  tcl_status = Tcl_GetInt(interp, argv[2], &r);
  AY_CHTCLERRRET(tcl_status, argv[0], interp);

  if(r <= 0)
    {
      ay_error(AY_ERROR, argv[0], "Parameter r must be > 0.");
      return TCL_OK;
    }

  while(sel)
    {
      src = sel->object;
      if(src->type != AY_IDNCURVE)
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	}
      else
	{
	  curve = (ay_nurbcurve_object*)src->refine;
	  knots = curve->knotv;

	  if((u < knots[curve->order-1]) || (u > knots[curve->length]))
	    {
	      (void)ay_error_reportdrange(argv[0], "\"u\"",
					  knots[curve->order-1],
					  knots[curve->length]);
	      return TCL_OK;
	    }

	  k = ay_nb_FindSpanMult(curve->length-1, curve->order-1, u,
				 knots, &s);

	  if(curve->order < r+s)
	    {
	      ay_error(AY_ERROR, argv[0],
			 "Knot insertion leads to illegal knot sequence.");
	      return TCL_OK;
	    }

	  curve->length += r;

	  if(!(newcontrolv = malloc(curve->length*stride*sizeof(double))))
	    {
	      ay_error(AY_EOMEM, argv[0], NULL);
	      return TCL_OK;
	    }

	  if(!(newknotv = malloc((curve->length+curve->order)*sizeof(double))))
	    {
	      free(newcontrolv);
	      ay_error(AY_EOMEM, argv[0], NULL);
	      return TCL_OK;
	    }

	  if(curve->is_rat)
	    ay_status = ay_nb_InsertKnotCurve4D(curve->length-r-1,
			  curve->order-1, curve->knotv, curve->controlv, u, k,
			  s, r, &nq, newknotv, newcontrolv);
	  else
	    ay_status = ay_nb_InsertKnotCurve3D(curve->length-r-1,
			  curve->order-1, curve->knotv, curve->controlv, u, k,
			  s, r, &nq, newknotv, newcontrolv);

	  if(ay_status)
	    {
	      ay_error(ay_status, argv[0], "Knot insert operation failed.");
	      return TCL_OK;
	    }

	  free(curve->controlv);
	  curve->controlv = newcontrolv;

	  free(curve->knotv);
	  curve->knotv = newknotv;
	  curve->knot_type = AY_KTCUSTOM;

	  ay_nct_recreatemp(curve);

	  /* remove all selected points */
	  if(src->selp)
	    ay_selp_clear(src);

	  src->modified = AY_TRUE;

	  /* re-create tesselation of curve */
	  (void)ay_notify_object(sel->object);
	  notify_parent = AY_TRUE;
	} /* if */

      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_insertkntcmd */


/** ay_nct_findu3d:
 *  transforms the given 3D coordinates
 *  to the corresponding parametric value
 *  on a NURBS curve
 *
 * \param[in] o  NURBS curve object to process
 * \param[in] xyz  3D coordinates in object space
 * \param[in,out] u  where to store the parametric value
 *
 * \returns AY_OK on success, error code otherwise
 */
int
ay_nct_findu3d(ay_object *o, double *xyz, double *u)
{
 int ay_status = AY_OK;
 double m[16];
 ay_nurbcurve_object *nc = NULL;
 int i = 0, j = 0, k = 0, stride = 4, samples = 10;
 int starti = 0, endi = 0, freecv = AY_FALSE;
 double distance = 0.0, min_distance = 0.0;
 double *cp = NULL, U[10/* XXXX samples! */] = {0}, startu, endu;
 double *cv;

  if(!o || !xyz || !u)
    return AY_ENULL;

  if(o->type != AY_IDNCURVE)
    return AY_EWTYPE;

  nc = (ay_nurbcurve_object *)o->refine;

  if(AY_ISTRAFO(o))
    {
      ay_trafo_creatematrix(o, m);

      if(!(cv = malloc(nc->length*stride*sizeof(double))))
	{ ay_status = AY_EOMEM; goto cleanup; }

      for(i = 0; i < nc->length; i++)
	{
	  memcpy(&(cv[k]), &(nc->controlv[k]), stride*sizeof(double));
	  ay_trafo_apply3(&(cv[k]), m);
	  k += stride;
	}

      freecv = AY_TRUE;
    }
  else
    {
      cv = nc->controlv;
    }

  if(!(cp = calloc(samples*stride, sizeof(double))))
    { ay_status = AY_EOMEM; goto cleanup; }

  startu = nc->knotv[nc->order-1];
  endu = nc->knotv[nc->length];

  for(k = 0; k < 7; k++)
    {
      U[0] = startu;
      for(i = 1; i < samples-1; i++)
	{
	  U[i] = U[i-1]+(endu - startu)/(samples-1);
	}
      U[samples-1] = endu;

      for(i = 0; i < samples-1; i++)
       {
	 ay_status = ay_nb_CurvePoint4D(nc->length-1, nc->order-1, nc->knotv,
					cv,
					U[i]+((U[i+1]-U[i])/2.0),
					&(cp[i*stride]));
	 if(ay_status)
	   goto cleanup;
       }

      j = 0;
      min_distance = DBL_MAX;
      for(i = 0; i < (samples-1); i++)
	{
	  distance = AY_VLEN((xyz[0] - cp[j]),
			     (xyz[1] - cp[j+1]),
			     (xyz[2] - cp[j+2]));

	  if(distance < min_distance)
	    {
	      *u = U[i]+((U[i+1]-U[i])/2.0);
	      starti = i-1;
	      endi = i+1;
	      min_distance = distance;
	    }

	  j += stride;
	} /* for */

      if(starti < 0)
	{
	  startu = U[0];
	}
      else
	{
	  startu = U[starti];
	}

      if(endi >= samples)
	{
	  endu = U[samples-1];
	}
      else
	{
	  endu = U[endi];
	}
    } /* for */

cleanup:

  if(freecv && cv)
    free(cv);

  free(cp);

 return ay_status;
} /* ay_nct_findu3d */


/* ay_nct_findu:
 *  transforms the window coordinates (winX, winY)
 *  to the corresponding parametric value u
 *  on the NURBS curve o
 *  This function needs a valid OpenGL rendering context!
 *
 *  XXXX ToDo: use gluPickMatrix() to speed this up
 *
 */
int
ay_nct_findu(struct Togl *togl, ay_object *o,
	     double *winXY, double *worldXYZ, double *u)
{
 int ay_status = AY_OK;
 /*  int width = Togl_Width(togl);*/
 int height = Togl_Height(togl);
 GLint viewport[4];
 GLdouble modelMatrix[16], projMatrix[16], winx, winy;
 GLfloat winz = 0.0f;
 double m[16];
 GLfloat pixel1[3] = {0.9f,0.9f,0.9f}, pixel[3] = {0.0f,0.0f,0.0f};
 ay_nurbcurve_object *c = NULL;
 int dx[25] = {0,1,1,0,-1,-1,-1,0,1,2,2,2,1,0,-1,-2,-2,-2,-2,-2,-1,0,1,2,2};
 int dy[25] = {0,0,-1,-1,-1,0,1,1,1,0,-1,-2,-2,-2,-2,-2,-1,0,1,2,2,2,2,2,1};
 int found, i = 0, j = 0, k = 0, /*maxtry = 1000,*/ stride, samples = 10;
 int starti = 0, endi = 0;
 double point[4] = {0}/*, guess = 0.0, e1 = 0.05, e2 = 0.05*/;
 double distance = 0.0, min_distance = 0.0;
 double *cp = NULL, U[10/* XXXX samples! */] = {0}, startu, endu;
 ay_voidfp *arr = NULL;
 ay_drawcb *cb = NULL;

  if(!o)
    return AY_ENULL;

  if(o->type != AY_IDNCURVE)
    return AY_EWTYPE;

  c = (ay_nurbcurve_object *)o->refine;

  arr = ay_drawcbt.arr;
  cb = (ay_drawcb *)(arr[o->type]);

  pixel1[0] = (float)ay_prefs.ser;
  pixel1[1] = (float)ay_prefs.seg;
  pixel1[2] = (float)ay_prefs.seb;

  /*
  winx = winX;
  winy = height - winY;
  */

  /*
   * first, we try to find a point on the curve in window coordinates;
   * we do this by comparing colors of rendered pixels
   */
  found = AY_FALSE;
  while(!found)
    {
      winx = winXY[0]+dx[i];
      winy = height-winXY[1]-dy[i];
      i++;

      if(i > 24)
	{
	  return AY_ERROR;
	}

      glReadPixels((GLint)winx, (GLint)winy, 1, 1, GL_RGB, GL_FLOAT, &pixel);

      if((pixel1[0] <= pixel[0]) &&
	 (pixel1[1] <= pixel[1]) &&
	 (pixel1[2] <= pixel[2]))
	{
	  found = AY_TRUE;
	}
    }

  /* get object coordinates of point on curve */
  glGetIntegerv(GL_VIEWPORT, viewport);

  glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
   if(ay_currentlevel->object != ay_root)
     ay_trafo_concatparent(ay_currentlevel->next);

   glTranslated(o->movx, o->movy, o->movz);
   ay_quat_torotmatrix(o->quat, m);
   glMultMatrixd(m);
   glScaled(o->scalx, o->scaly, o->scalz);

   /* we operate on selected objects, but those are drawn with
      disabled depth test, which means: no correct z buffer data;
      thus, we simply call the draw callback of the selected object
      here again on an empty z buffer to get correct z buffer data */
   glClear(GL_DEPTH_BUFFER_BIT);
   if(cb)
     (void)cb(togl, o);

   glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
  glPopMatrix();

  /* get winz */
  glReadPixels((GLint)winx,(GLint)winy,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&winz);

  gluUnProject(winx, winy, (GLdouble)winz, modelMatrix, projMatrix, viewport,
	       &(point[0]), &(point[1]), &(point[2]));

  /* get guess */
  stride = 4;
  if(!(cp = calloc(samples*stride, sizeof(double))))
    return AY_EOMEM;

  startu = c->knotv[c->order-1];
  endu = c->knotv[c->length];

  for(k = 0; k < 7; k++)
    {
      U[0] = startu;
      for(i = 1; i < samples-1; i++)
	{
	  U[i] = U[i-1]+(endu - startu)/(samples-1);
	}
      U[samples-1] = endu;

      for(i = 0; i < samples-1; i++)
       {
	 ay_status = ay_nb_CurvePoint4D(c->length-1, c->order-1, c->knotv,
					c->controlv,
					U[i]+((U[i+1]-U[i])/2.0),
					&(cp[i*stride]));

	 if(ay_status)
	   goto cleanup;
       }

      j = 0;
      min_distance = DBL_MAX;
      for(i = 0; i < (samples-1); i++)
	{
	  distance = AY_VLEN((point[0] - cp[j]),
			     (point[1] - cp[j+1]),
			     (point[2] - cp[j+2]));

	  if(distance < min_distance)
	    {
	      *u = U[i]+((U[i+1]-U[i])/2.0);
	      starti = i-1;
	      endi = i+1;
	      min_distance = distance;
	    }

	  j += stride;
	} /* for */

      if(starti < 0)
	{
	  startu = U[0];
	}
      else
	{
	  startu = U[starti];
	}

      if(endi >= samples)
	{
	  endu = U[samples-1];
	}
      else
	{
	  endu = U[endi];
	}
    } /* for */

  winXY[0] = winx;
  winXY[1] = winy;

  ay_status = ay_nb_CurvePoint4D(c->length-1, c->order-1, c->knotv,
				 c->controlv, *u, point);

  if(ay_status)
    goto cleanup;

  ay_trafo_apply3(point, modelMatrix);

  worldXYZ[0] = point[0];
  worldXYZ[1] = point[1];
  worldXYZ[2] = point[2];

cleanup:

  if(cp)
    free(cp);

 return ay_status;
} /* ay_nct_findu */


/* ay_nct_finducb:
 *  Togl callback to implement find parametric value
 *  u for a picked point on a NURBS curve
 */
int
ay_nct_finducb(struct Togl *togl, int argc, char *argv[])
{
 int ay_status = AY_OK;
 char fname[] = "findU_cb";
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 Tcl_Interp *interp = Togl_Interp(togl);
 int i, drag = AY_FALSE, silence = AY_FALSE, success = AY_FALSE;
 int extend = AY_FALSE;
 int height = Togl_Height(togl);
 unsigned int j;
 double winXY[8] = {0}, worldXYZ[3] = {0}, dt;
 static int fvalid = AY_FALSE;
 static double fX = 0.0, fY = 0.0, fwX = 0.0, fwY = 0.0, fwZ = 0.0;
 double obj[24] = {0}, pl[16] = {0}, mm[2] = {0};
 double minlevelscale = 1.0, u = 0.0, oldu = 0.0;
 Tcl_Obj *to = NULL;
 char cmd[] = "puts \"u: $u\"";
 ay_list_object *sel = ay_selection;
 ay_object *o, *p = NULL;
 ay_pointedit pe = {0};
 ay_nurbcurve_object *nc;
 ay_tag *tag = NULL, **last = NULL;

  if(argc > 2)
    {
      if(!strcmp(argv[2], "-start"))
	{
	  /*	  view->drawhandles = AY_FALSE;
		  display_cb(togl);*/
	  fvalid = AY_FALSE;
	  return TCL_OK;
	}
      if(!strcmp(argv[2], "-end"))
	{
	  /* draw cross */
	  if(fvalid && (argc < 6))
	    {
	      view->markx = fX;
	      view->marky = height-fY;
	      view->markworld[0] = fwX;
	      view->markworld[1] = fwY;
	      view->markworld[2] = fwZ;
	      view->drawmark = AY_TRUE;

	      if(ay_prefs.normalizemark)
		{
		  for(i = 0; i < 3; i++)
		    {
		      view->markworld[i] = ay_trafo_round(view->markworld[i],
						     ay_prefs.normalizedigits);
		    }
		}

	      ay_viewt_updatemark(togl, AY_FALSE);
	      ay_viewt_printmark(view);
	    }

	  fvalid = AY_FALSE;
	  return TCL_OK;
	}
      if(!strcmp(argv[argc-1], "-extend"))
	{
	  to = Tcl_GetVar2Ex(interp, "u", NULL,
			     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	  if(to)
	    {
	      Tcl_GetDoubleFromObj(interp, to, &oldu);
	      extend = AY_TRUE;
	    }
	  else
	    {
	      ay_error(AY_ERROR, fname,
		       "Could not get previous u for extension.");
	      return TCL_OK;
	    }
	}
    }
  else
    {
      return TCL_OK;
    }

  if(!ay_selection)
    {
      ay_error(AY_ENOSEL, fname, NULL);
      return TCL_OK;
    }

  if(argc > 4)
    silence = AY_TRUE;

  Tcl_GetDouble(interp, argv[2], &(winXY[0]));
  Tcl_GetDouble(interp, argv[3], &(winXY[1]));

  if(argc > 5)
    {
      drag = AY_TRUE;
      Tcl_GetDouble(interp, argv[4], &(winXY[2]));
      Tcl_GetDouble(interp, argv[5], &(winXY[3]));

      if(argc > 6)
	silence = AY_TRUE;
      else
	silence = AY_FALSE;
    }

  if(ay_prefs.errorlevel < 3)
    silence = AY_TRUE;

  memcpy(&(winXY[4]), winXY, 4*sizeof(double));

  minlevelscale = ay_pact_getminlevelscale();

  while(sel)
    {
      o = sel->object;

      if(o->type != AY_IDNCURVE)
	{
	  p = ay_peek_singleobject(o, AY_IDNCURVE);
	  if(p)
	    {
	      o = p;
	    }
	  else
	    {
	      sel = sel->next;
	      continue;
	    }
	}

      nc = (ay_nurbcurve_object*)o->refine;
      if(!nc->breakv)
	{
	  (void)ay_nct_computebreakpoints(nc);
	}

      memcpy(winXY, &(winXY[4]), 4*sizeof(double));

      /* first try to pick a knot point */
      pe.type = AY_PTKNOT;
      if(!drag)
	{
	  ay_viewt_wintoobj(togl, sel->object, winXY[0], winXY[1],
			    &(obj[0]), &(obj[1]), &(obj[2]));

	  (void)ay_pact_pickpoint(o, view, minlevelscale, obj, &pe);
	}
      else
	{
	  /* drag selection */
	  if(winXY[2] < winXY[0])
	    {
	      dt = winXY[2];
	      winXY[2] = winXY[0];
	      winXY[0] = dt;
	    }

	  if(winXY[3] < winXY[1])
	    {
	      dt = winXY[3];
	      winXY[3] = winXY[1];
	      winXY[1] = dt;
	    }

	  ay_viewt_winrecttoobj(togl, sel->object, winXY[0], winXY[1],
				winXY[2], winXY[3], obj);

	  ay_viewt_objrecttoplanes(obj, pl);

	  (void)ay_pact_getpoint(2, o, pl, &pe);
	} /* if drag */

      if(pe.num)
	{
	  /* knot picking succeeded, get parametric value from knot */
	  success = AY_TRUE;
	  u = pe.coords[0][3];
	  memcpy(worldXYZ, pe.coords[0], 3*sizeof(double));
	  ay_trafo_identitymatrix(pl);
	  ay_trafo_getall(ay_currentlevel, sel->object, pl);
	  ay_trafo_apply3(worldXYZ, pl);
	  /*
 printf("picked world %lg %lg %lg\n",worldXYZ[0],worldXYZ[1],worldXYZ[2]);
	  */
	  ay_viewt_worldtowin(worldXYZ, winXY);

	  i = 0;
	  while((nc->knotv[i] < u) && (i < (nc->length + nc->order)))
	    i++;

	  to = Tcl_NewIntObj(i);
	  Tcl_SetVar2Ex(interp, "ui", NULL, to,
			TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

	  if(pe.num > 1)
	    {
	      /* multiple hits, derive knot range */
	      mm[0] = DBL_MAX;
	      mm[1] = -DBL_MAX;
	      for(j = 0; j < pe.num; j++)
		{
		  if(pe.coords[j][3] < mm[0])
		    {
		      mm[0] = pe.coords[j][3];
		    }
		  if(pe.coords[j][3] > mm[1])
		    {
		      mm[1] = pe.coords[j][3];
		    }
		}
	      if(fabs(mm[0]-mm[1]) > AY_EPSILON)
		{
		  to = Tcl_NewDoubleObj(mm[0]);
		  Tcl_SetVar2Ex(interp, "umin", NULL, to,
				TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
		  to = Tcl_NewDoubleObj(mm[1]);
		  Tcl_SetVar2Ex(interp, "umax", NULL, to,
				TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

		  if(sel->object->type == AY_IDNCURVE)
		    (void)ay_knots_setuminmax(sel->object, mm[0], mm[1]);
		}
	    }
	}
      else
	{
	  /* knot picking failed, infer parametric value from curve point */
	  if(!(ay_status = ay_nct_findu(togl, o, winXY, worldXYZ, &u)))
	    {
	      ay_viewt_worldtowin(worldXYZ, obj);
	      if(fabs(winXY[0]-obj[0])<3.0 && fabs(winXY[1]-obj[1])<3.0)
		success = AY_TRUE;
	    }
	  else
	    {
	      if(drag)
		{
		  /* failed drag => remove all UMM tags */
		  tag = o->tags;
		  last = &(o->tags);
		  while(tag)
		    {
		      if(tag->type == ay_umm_tagtype)
			{
			  *last = tag->next;
			  ay_tags_free(tag);
			  tag = *last;
			}
		      else
			{
			  last = &(tag->next);
			  tag = tag->next;
			} /* if */
		    } /* while */
		} /* if drag */
	    } /* if */
	} /* if pick succeeded */

      if(success)
	{
	  if(!extend && !(pe.num > 1))
	    {
	      fvalid = AY_TRUE;
	      fX = winXY[0];
	      fY = winXY[1];
	      fwX = worldXYZ[0];
	      fwY = worldXYZ[1];
	      fwZ = worldXYZ[2];
	    }
	  else
	    {
	      fvalid = AY_FALSE;
	      view->drawmark = AY_FALSE;
	    }

	  to = Tcl_NewDoubleObj(u);
	  Tcl_SetVar2Ex(interp, "u", NULL, to,
			TCL_LEAVE_ERR_MSG|TCL_GLOBAL_ONLY);
	  if(!silence)
	    Tcl_Eval(interp, cmd);

	  if(extend)
	    {
	      if(pe.num > 1)
		{
		  if(oldu < mm[0])
		    {
		      mm[0] = oldu;
		    }
		  else
		    {
		      if(oldu > mm[1])
			mm[1] = oldu;
		      else
			extend = AY_FALSE;
		    }
		}
	      else
		{
		  if(fabs(oldu-u) > AY_EPSILON)
		    {
		      if(oldu < u)
			{
			  mm[0] = oldu;
			  mm[1] = u;
			}
		      else
			{
			  mm[0] = u;
			  mm[1] = oldu;
			}
		    }
		  else
		    {
		      extend = AY_FALSE;
		    }
		}
	      if(extend)
		{
		  to = Tcl_NewDoubleObj(mm[0]);
		  Tcl_SetVar2Ex(interp, "umin", NULL, to,
				TCL_LEAVE_ERR_MSG|TCL_GLOBAL_ONLY);
		  to = Tcl_NewDoubleObj(mm[1]);
		  Tcl_SetVar2Ex(interp, "umax", NULL, to,
				TCL_LEAVE_ERR_MSG|TCL_GLOBAL_ONLY);
		  if(o->type == AY_IDNCURVE)
		    (void)ay_knots_setuminmax(o, mm[0], mm[1]);
		}

	      extend = AY_TRUE;
	    } /* if extend */
	}
      else
	{
	  /* need to repair the "damage" done by ay_nct_findu() above */
	  ay_toglcb_display(togl);
	}

      ay_pact_clearpointedit(&pe);

      if(success)
	break;

      sel = sel->next;
    } /* while */

 return TCL_OK;
} /* ay_nct_finducb */


/** ay_nct_splitdisc:
 * split NURBCurve object \a src at discontinuous parametric value \a u
 * into two curves;
 * \a u must appear curve->order times in the knot vector;
 * modifies \a src, returns second curve in \a result
 *
 * \param[in,out] src  curve object to split
 * \param[in] u  parametric value
 * \param[in,out] result  where to store the second curve
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_splitdisc(ay_object *src, double u, ay_object **result)
{
 int ay_status = AY_OK;
 ay_object *new = NULL;
 ay_nurbcurve_object *nc1 = NULL, *nc2 = NULL;
 double *newcv1 = NULL, *newkv1 = NULL;
 double *newcv2 = NULL, *newkv2 = NULL;
 int i, stride = 4;

  ay_status = ay_object_copy(src, &new);
  if(ay_status)
    return ay_status;

  nc1 = (ay_nurbcurve_object *)src->refine;
  nc1->knot_type = AY_KTCUSTOM;
  nc1->type = AY_CTOPEN;

  nc2 = (ay_nurbcurve_object *)new->refine;
  nc2->knot_type = AY_KTCUSTOM;
  nc2->type = AY_CTOPEN;

  /* find knot */
  for(i = 0; i < nc1->length+nc1->order; i++)
    {
      if(fabs(nc1->knotv[i] - u) < AY_EPSILON)
	break;
    }

  nc1->length = i;

  if(nc1->length < 2)
    { nc1->length = nc2->length; ay_status = AY_ERROR; goto cleanup; }

  nc2->length -= i;

  if(nc2->length < 2)
    { ay_status = AY_ERROR; goto cleanup; }

  /* create new controls/knots for first curve */
  if(!(newcv1 = malloc(nc1->length*stride*sizeof(double))))
    { ay_status = AY_EOMEM; goto cleanup; }

  memcpy(newcv1, nc1->controlv, nc1->length*stride*sizeof(double));

  if(!(newkv1 = malloc((nc1->length+nc1->order)*sizeof(double))))
    { free(newcv1); ay_status = AY_EOMEM; goto cleanup; }

  memcpy(newkv1, nc1->knotv, (nc1->length+nc1->order)*sizeof(double));

  /* create new controls/knots for second curve */
  if( !(newcv2 = malloc(nc2->length*stride*sizeof(double))))
    { free(newcv1); free(newkv1); ay_status = AY_EOMEM; goto cleanup; }

  memcpy(newcv2, &(nc1->controlv[i*stride]),
	 nc2->length*stride*sizeof(double));

  if(!(newkv2 = malloc((nc2->length+nc2->order)*sizeof(double))))
    {
      free(newcv1); free(newkv1); free(newcv2);
      ay_status = AY_EOMEM; goto cleanup;
    }

  memcpy(newkv2, &(nc1->knotv[nc1->length]),
	 (nc2->length+nc2->order)*sizeof(double));

  /* swap in new controls/knots */
  free(nc1->controlv);
  nc1->controlv = newcv1;

  free(nc1->knotv);
  nc1->knotv = newkv1;

  ay_nct_recreatemp(nc1);

  free(nc2->controlv);
  nc2->controlv = newcv2;

  free(nc2->knotv);
  nc2->knotv = newkv2;

  ay_nct_recreatemp(nc2);

  new->selected = AY_FALSE;
  new->modified = AY_TRUE;
  src->modified = AY_TRUE;

  /* return result */
  *result = new;

  new = NULL;

cleanup:

  if(new)
    ay_object_delete(new);

 return ay_status;
} /* ay_nct_splitdisc */


/** ay_nct_split:
 *  split NURBCurve object \a src at parametric value \a u into two;
 *  modifies \a src, returns second curve in \a result
 *
 * \param[in,out] src  curve object to split
 * \param[in] u  parametric value at which to split
 * \param[in] relative  if AY_TRUE, interpret \a u in a relative way
 * \param[in,out] result  where to store the second curve
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_split(ay_object *src, double u, int relative, ay_object **result)
{
 int ay_status = AY_OK;
 ay_object *new = NULL;
 ay_nurbcurve_object *curve = NULL;
 ay_nurbcurve_object *nc1 = NULL, *nc2 = NULL;
 double *knots = NULL, *newcontrolv = NULL, *newknotv = NULL;
 int stride = 4, k = 0, r = 0, s = 0, nq = 0, nc1len = 0;
 char fname[] = "split";

  if(!src || !result)
    return AY_ENULL;

  if(src->type != AY_IDNCURVE)
    {
      ay_error(AY_EWTYPE, fname, ay_nct_ncname);
      return AY_ERROR;
    }
  else
    {
      curve = (ay_nurbcurve_object*)src->refine;
      stride = 4;
      knots = curve->knotv;

      if(relative)
	{
	  u = knots[curve->order-1] +
	    (knots[curve->length] - knots[curve->order-1])*u;
	}
      else
	{
	  if((u <= knots[curve->order-1]) || (u >= knots[curve->length]))
	    {
	      return ay_error_reportdrange(fname, "\"u\"",
				 knots[curve->order-1], knots[curve->length]);
	    }
	}

      k = ay_nb_FindSpanMult(curve->length-1, curve->order-1, u,
			     knots, &s);

      if(s == curve->order)
	{
	  return ay_nct_splitdisc(src, u, result);
	}

      if(s > curve->order)
	return AY_ERROR;

      r = curve->order-1-s;

      curve->length += r;

      if(r > 0)
	{
	  if(!(newcontrolv = malloc(curve->length*stride*sizeof(double))))
	    return AY_EOMEM;

	  if(!(newknotv = malloc((curve->length+curve->order)*
				 sizeof(double))))
	    { free(newcontrolv); return AY_EOMEM; }

	  if(curve->is_rat)
	    ay_status = ay_nb_InsertKnotCurve4D(curve->length-r-1,
			  curve->order-1, curve->knotv, curve->controlv, u, k,
			  s, r, &nq, newknotv, newcontrolv);
	  else
	    ay_status = ay_nb_InsertKnotCurve3D(curve->length-r-1,
			  curve->order-1, curve->knotv, curve->controlv, u, k,
			  s, r, &nq, newknotv, newcontrolv);

	  if(ay_status)
	    {
	      free(newcontrolv); free(newknotv);
	      return ay_status;
	    }

	  free(curve->controlv);
	  curve->controlv = newcontrolv;

	  free(curve->knotv);
	  curve->knotv = newknotv;
	}

      /* create two new curves */
      nc1 = curve;
      nc1->knot_type = AY_KTCUSTOM;
      nc1->type = AY_CTOPEN;
      ay_status = ay_object_copy(src, &new);

      if(ay_status)
	return ay_status;

      if(r > 0)
	{
	  nc1len = k - (nc1->order-1) + 1 + (curve->order-1-s+r-1)/2 + 1;
	}
      else
	{
	  nc1len = k - (nc1->order-1) + 1;
	}

      if(nc1len < 2)
	{ (void)ay_object_delete(new); return AY_ERROR; }

      nc2 = (ay_nurbcurve_object*)new->refine;
      nc2->length = (nc1->length+1) - nc1len;

      if(nc2->length < 2)
	{ (void)ay_object_delete(new); return AY_ERROR; }

      nc1->length = nc1len;

      if(!(newcontrolv = malloc(nc1->length*stride*sizeof(double))))
	{ (void)ay_object_delete(new); return AY_EOMEM; }

      if(!(newknotv = malloc((nc1->length+nc1->order)*sizeof(double))))
	{ (void)ay_object_delete(new); free(newcontrolv); return AY_EOMEM; }

      memcpy(newcontrolv, nc1->controlv, nc1->length*stride*sizeof(double));

      memcpy(newknotv, nc1->knotv, (nc1->length+nc1->order)*sizeof(double));

      /* improve phantom knot */
      newknotv[curve->length+curve->order-1] =
	newknotv[curve->length+curve->order-2];

      /* check new knots for validity */
      if(ay_knots_check(nc1->length, nc1->order,
			nc1->length+nc1->order,	newknotv))
	{
	  (void)ay_object_delete(new); free(newcontrolv); free(newknotv);
	  return AY_ERROR;
	}

      free(nc2->controlv);
      nc2->controlv = NULL;
      free(nc2->knotv);
      nc2->knotv = NULL;

      if(!(nc2->controlv = malloc((nc2->length*stride)*sizeof(double))))
	{ (void)ay_object_delete(new); free(newcontrolv); free(newknotv);
	  return AY_EOMEM; }

      if(!(nc2->knotv = malloc((nc2->length+nc2->order)*sizeof(double))))
	{ (void)ay_object_delete(new); free(newcontrolv); free(newknotv);
	  free(nc2->controlv); return AY_EOMEM; }

      memcpy(nc2->controlv, &(nc1->controlv[(nc1->length-1)*stride]),
	     nc2->length*stride*sizeof(double));

      memcpy(nc2->knotv, &(nc1->knotv[nc1->length-1]),
	     (nc2->length+nc2->order)*sizeof(double));

      /* improve phantom knot */
      nc2->knotv[0] = nc2->knotv[1];

      /* check new knots for validity */
      if(ay_knots_check(nc2->length, nc2->order,
			nc2->length+nc2->order,	nc2->knotv))
	{
	  (void)ay_object_delete(new); free(newcontrolv); free(newknotv);
	  return AY_ERROR;
	}

      free(nc1->controlv);
      nc1->controlv = newcontrolv;
      free(nc1->knotv);
      nc1->knotv = newknotv;

      ay_nct_recreatemp(nc1);
      ay_nct_recreatemp(nc2);

      new->selected = AY_FALSE;
      new->modified = AY_TRUE;
      src->modified = AY_TRUE;

      /* return result */
      *result = new;
    } /* if ncurve */

 return ay_status;
} /* ay_nct_split */


/** ay_nct_splittcmd:
 *  Split selected NURBS curves.
 *  Implements the \a splitNC scripting interface command.
 *  See also the corresponding section in the \ayd{scsplitnc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_splittcmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_object *new = NULL;
 double u = 0.0;
 int i = 1;
 int notify_parent = AY_FALSE, append = AY_FALSE, relative = AY_FALSE;

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  /* parse args */
  if(argc > 2)
    {
      while(i+1 < argc)
	{
	  if(argv[i][0] == '-' && argv[i][1] == 'a')
	    {
	      append = AY_TRUE;
	    }
	  if(argv[i][0] == '-' && argv[i][1] == 'r')
	    {
	      relative = AY_TRUE;
	    }
	  i++;
	} /* while */
    } /* if have args */

  if(argc <= i)
    {
      ay_error(AY_EARGS, argv[0], "[-a|-r] u");
      return TCL_OK;
    }

  tcl_status = Tcl_GetDouble(interp, argv[i], &u);
  AY_CHTCLERRRET(tcl_status, argv[0], interp);
  if(u != u)
    {
      ay_error_reportnan(argv[0], "u");
      return TCL_OK;
    }

  while(sel)
    {
      if(sel->object->type == AY_IDNCURVE)
	{
	  new = NULL;

	  if((ay_status = ay_nct_split(sel->object, u, relative, &new)))
	    {
	      if(!new)
		ay_error(ay_status, argv[0], NULL);
	      return TCL_OK;
	    } /* if */

	  if(append)
	    {
	      ay_object_link(new);
	    }
	  else
	    {
	      new->next = sel->object->next;
	      sel->object->next = new;
	    }

	  /* remove all selected points */
	  if(sel->object->selp)
	    ay_selp_clear(sel->object);

	  sel->object->modified = AY_TRUE;

	  /* re-create tesselation of original curve */
	  (void)ay_notify_object(sel->object);
	  notify_parent = AY_TRUE;
	}
      else
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	} /* if */

      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_splittcmd */


/** ay_nct_concattcmd:
 *  Concatenate selected NURBS curves.
 *  Implements the \a concatNC scripting interface command.
 *  See also the corresponding section in the \ayd{scconcatnc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_concattcmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[])
{
 int ay_status;
 double *newcontrolv = NULL;
 double *controlv1, *controlv2;
 int ktype = AY_KTNURB;
 int i = 0, a = 0, b = 0;
 double m1[16], m2[16];
 ay_list_object *sel = ay_selection;
 ay_nurbcurve_object *nc1 = NULL, *nc2 = NULL;
 ay_object *o = NULL;

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  if(!sel->next)
    {
      ay_error(AY_ERROR, argv[0], "Select two NURBS curves.");
      return TCL_OK;
    }

  if((sel->object->type != AY_IDNCURVE) ||
     (sel->next->object->type != AY_IDNCURVE))
    {
      ay_error(AY_EWTYPE, argv[0], ay_nct_ncname);
      return TCL_OK;
    }

  if(!(o = calloc(1, sizeof(ay_object))))
    {
      ay_error(AY_EOMEM, argv[0], NULL);
      return TCL_OK;
    }

  o->type = AY_IDNCURVE;
  ay_object_defaults(o);

  /* get curves transformation-matrix */
  ay_trafo_creatematrix(sel->object, m1);
  ay_trafo_creatematrix(sel->next->object, m2);

  nc1 = (ay_nurbcurve_object *)sel->object->refine;
  nc2 = (ay_nurbcurve_object *)sel->next->object->refine;

  if(nc1->knot_type != AY_KTCUSTOM)
    ktype = nc1->knot_type;

  controlv1 = nc1->controlv;
  controlv2 = nc2->controlv;

  if(!(newcontrolv = malloc((nc1->length*4+nc2->length*4)*sizeof(double))))
    {
      ay_error(AY_EOMEM, argv[0], NULL);
      free(o);
      return TCL_OK;
    }

  a = 0; b = 0;
  for(i = 0; i < (nc1->length); i++)
    {
      newcontrolv[b] = controlv1[a];
      newcontrolv[b+1] = controlv1[a+1];
      newcontrolv[b+2] = controlv1[a+2];
      newcontrolv[b+3] = controlv1[a+3];

      ay_trafo_apply4(&(newcontrolv[b]), m1);

      a += 4;
      b += 4;
    }
  a = 0;
  for(i = 0; i < (nc2->length); i++)
    {
      newcontrolv[b] = controlv2[a];
      newcontrolv[b+1] = controlv2[a+1];
      newcontrolv[b+2] = controlv2[a+2];
      newcontrolv[b+3] = controlv2[a+3];

      ay_trafo_apply4(&(newcontrolv[b]), m2);

      a += 4;
      b += 4;
    }

  ay_status = ay_nct_create(nc1->order, nc1->length+nc2->length,
			    ktype, newcontrolv, NULL,
			    (ay_nurbcurve_object **)(void*)&(o->refine));

  if(ay_status)
    {
      free(o);
      free(newcontrolv);
      ay_error(ay_status, argv[0], NULL);
      return TCL_OK;
    }

  ay_nct_recreatemp((ay_nurbcurve_object *)o->refine);

  ay_object_link(o);

  (void)ay_notify_object(o);

  o->modified = AY_TRUE;

  (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_concattcmd */


/** ay_nct_concatctcmd:
 *  Concatenate selected curves.
 *  Implements the \a concatC scripting interface command.
 *  See also the corresponding section in the \ayd{scconcatc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_concatctcmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
 int ay_status, tcl_status;
 int i = 1, closed = AY_FALSE, fillets = AY_FALSE, knot_type = 0;
 int order = 0;
 double flen = 0.3;
 ay_nurbcurve_object *nc;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL, *curves = NULL, **next = NULL;
 ay_object *newo = NULL;

  /* parse args */
  if(argc > 2)
    {
      while(i+1 < argc)
	{
	  if(!strcmp(argv[i], "-c"))
	    {
	      tcl_status = Tcl_GetBoolean(interp, argv[i+1], &closed);
	      if(tcl_status != TCL_OK)
		tcl_status = Tcl_GetInt(interp, argv[i+1], &closed);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	    }
	  if(!strcmp(argv[i], "-f"))
	    {
	      tcl_status = Tcl_GetBoolean(interp, argv[i+1], &fillets);
	      if(tcl_status != TCL_OK)
		tcl_status = Tcl_GetInt(interp, argv[i+1], &fillets);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	    }
	  if(!strcmp(argv[i], "-fl"))
	    {
	      tcl_status = Tcl_GetDouble(interp, argv[i+1], &flen);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      if(flen < AY_EPSILON || flen != flen)
		{
		  ay_error(AY_ERROR, argv[0], "flen must be > 0.0");
		  return TCL_OK;
		}
	    }
	  /*
	  if(!strcmp(argv[i], "-o"))
	    {
	      tcl_status = Tcl_GetInt(interp, argv[i+1], &order);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);

	      if(type < 0)
		{
		  ay_error(AY_ERROR, argv[0],
			   "Parameter <order> must be >= 0.");
		  return TCL_OK;
		}
	    }
	  */
	  if(!strcmp(argv[i], "-k"))
	    {
	      tcl_status = Tcl_GetInt(interp, argv[i+1], &knot_type);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	    }

	  i += 2;
	} /* while */
    } /* if have args */

  /* check selection */
  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  /* collect curves */
  next = &curves;
  while(sel)
    {
      o = sel->object;

      /* copy or provide */
      if(o->type == AY_IDNCURVE)
	{
	  ay_status = ay_object_copy(o, next);
	  if(ay_status)
	    goto cleanup;
	  ay_nct_applytrafo(*next);
	  next = &((*next)->next);
	}
      else
	{
	  if(ay_provide_object(o, AY_IDNCURVE, NULL) == AY_OK)
	    {
	      (void)ay_provide_object(o, AY_IDNCURVE, next);
	      while(*next)
		{
		  ay_nct_applytrafo(*next);
		  next = &((*next)->next);
		}
	    } /* if */
	} /* if is NCurve */

      sel = sel->next;
    } /* while */

  /* gracefully exit, if there are no
     (or not enough?) curves to concat */
  if(!curves)
    {
      ay_error(AY_EWARN, argv[0], "Not enough curves to concat!");
      return TCL_OK;
    }

  nc = (ay_nurbcurve_object*)curves->refine;
  order = nc->order;

  if(fillets)
    {
      ay_status = ay_nct_fillgaps(closed, order, flen, curves);

      if(ay_status)
	{
	  ay_error(AY_ERROR, argv[0], "Failed to create fillets!");
	  goto cleanup;
	}
    }

  ay_status = ay_nct_concatmultiple(closed, knot_type, fillets, curves, &newo);

  if(ay_status)
    {
      ay_error(AY_ERROR, argv[0], "Failed to concat!");
    }
  else
    {
      ay_object_link(newo);

      (void)ay_notify_parent();
    }

cleanup:

  /* free list of temporary curves */
  (void)ay_object_deletemulti(curves, AY_FALSE);

 return TCL_OK;
} /* ay_nct_concatctcmd */


/** ay_nct_crtncircle
 * Create a full standard 9 point NURBS circle of desired radius.
 *
 * \param[in] radius  desired radius
 * \param[in,out] curve  pointer where to store the new curve object
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_crtncircle(double radius, ay_nurbcurve_object **curve)
{
 int ay_status = AY_OK;
 ay_nurbcurve_object *new = NULL;
 double *controlv = NULL, *knotv = NULL;
 int i = 0, stride = 4;
 double controls[36] = {
   0.0,1.0,0.0,1.0,
   1.0,1.0,0.0,1.0,
   1.0,0.0,0.0,1.0,
   1.0,-1.0,0.0,1.0,
   0.0,-1.0,0.0,1.0,
   -1.0,-1.0,0.0,1.0,
   -1.0,0.0,0.0,1.0,
   -1.0,1.0,0.0,1.0,
   0.0,1.0,0.0,1.0
 };
 double knots[12] = {
   0.0, 0.0, 0.0, 0.25, 0.25, 0.5, 0.5, 0.75, 0.75, 1.0, 1.0, 1.0
 };

  if(!curve)
    return AY_ENULL;

  controls[7] = sqrt(2.0)/2.0;
  controls[15] = sqrt(2.0)/2.0;
  controls[23] = sqrt(2.0)/2.0;
  controls[31] = sqrt(2.0)/2.0;

  for(i = 0; i < 9; i++)
    {
      controls[i*stride] *= radius;
      controls[i*stride+1] *= radius;
    }

  if(!(new = calloc(1, sizeof(ay_nurbcurve_object))))
    return AY_EOMEM;
  if(!(controlv = malloc(9*4*sizeof(double))))
    { free(new); return AY_EOMEM; }
  if(!(knotv = malloc(12*sizeof(double))))
    { free(new); free(controlv); return AY_EOMEM; }

  new->order = 3;
  new->length = 9;
  new->knot_type = AY_KTCUSTOM;
  new->type = AY_CTCLOSED;

  memcpy(controlv, controls, 9*4*sizeof(double));
  new->controlv = controlv;

  memcpy(knotv, knots, 12*sizeof(double));
  new->knotv = knotv;

  new->createmp = AY_TRUE;
  new->is_rat = AY_TRUE;

  *curve = new;

 return ay_status;
} /* ay_nct_crtncircle */


/** ay_nct_crtncirclearc
 * Create a circular arc of desired angle and radius.
 *
 * \param[in] radius  desired radius
 * \param[in] arc  desired angle
 * \param[in,out] curve  pointer where to store the new curve object
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_crtncirclearc(double radius, double arc, ay_nurbcurve_object **curve)
{
 int ay_status = AY_OK;
 ay_nurbcurve_object *new = NULL;

  if(!curve)
    return AY_ENULL;

  if(!(new = calloc(1, sizeof(ay_nurbcurve_object))))
    return AY_EOMEM;

  if(arc < 0.0)
    {
      ay_status = ay_nb_CreateNurbsCircleArc(radius, arc, 0.0, &(new->length),
					     &new->knotv, &new->controlv);
    }
  else
    {
      ay_status = ay_nb_CreateNurbsCircleArc(radius, 0.0, arc, &(new->length),
					     &new->knotv, &new->controlv);
    }

  if(ay_status)
    { free(new); return ay_status; }

  new->order = 3;
  new->knot_type = AY_KTCUSTOM;
  new->is_rat = AY_TRUE;

  *curve = new;

 return ay_status;
} /* ay_nct_crtncirclearc */


/** ay_nct_crtnhcircle:
 * Create a half NURBS circle of desired radius.
 *
 * \param[in] radius  desired radius (>0.0, unchecked)
 * \param[in,out] curve  pointer where to store the new curve object
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_crtnhcircle(double radius, ay_nurbcurve_object **curve)
{
 int ay_status = AY_OK;
 ay_nurbcurve_object *new = NULL;
 double *controlv = NULL, *knotv = NULL;
 int i = 0, stride = 4;
 double controls[20] = {
   0.0,1.0,0.0,1.0,
   1.0,1.0,0.0,1.0,
   1.0,0.0,0.0,1.0,
   1.0,-1.0,0.0,1.0,
   0.0,-1.0,0.0,1.0,
 };
 double knots[8] = {
   0.0, 0.0, 0.0, 0.5, 0.5, 1.0, 1.0, 1.0
 };

  if(!curve)
    return AY_ENULL;

  controls[7] = sqrt(2.0)/2.0;
  controls[15] = sqrt(2.0)/2.0;

  for(i = 0; i < 5; i++)
    {
      controls[i*stride] *= radius;
      controls[i*stride+1] *= radius;
    }

  if(!(new = calloc(1, sizeof(ay_nurbcurve_object))))
    return AY_EOMEM;
  if(!(controlv = malloc(5*4*sizeof(double))))
    { free(new); return AY_EOMEM; }
  if(!(knotv = malloc(8*sizeof(double))))
    { free(new); free(controlv); return AY_EOMEM; }

  new->order = 3;
  new->length = 5;
  new->knot_type = AY_KTCUSTOM;

  memcpy(controlv, controls, 5*4*sizeof(double));
  new->controlv = controlv;

  memcpy(knotv, knots, 8*sizeof(double));
  new->knotv = knotv;

  new->is_rat = AY_TRUE;

  *curve = new;

 return ay_status;
} /* ay_nct_crtnhcircle */


/** ay_nct_crtncircletcmd:
 *  Create a NURBS circle.
 *  Implements the \a crtNCircle scripting interface command.
 *  See also the corresponding section in the \ayd{sccrtncircle}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_crtncircletcmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
 int tcl_status = TCL_OK;
 int ay_status;
 ay_object *o = NULL;
 double arc = 360.0;
 double radius = 1.0;
 int i = 1;

  if(argc > 2)
    {
      /* parse args */
      while(i+1 < argc)
	{
	  if(!strcmp(argv[i], "-r"))
	    {
	      tcl_status = Tcl_GetDouble(interp, argv[i+1], &radius);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      if(radius <= AY_EPSILON || radius != radius)
		{
		  ay_error(AY_ERROR, argv[0], "Parameter radius must be > 0.");
		  return TCL_OK;
		}
	    }
	  else
	  if(!strcmp(argv[i], "-a"))
	    {
	      tcl_status = Tcl_GetDouble(interp, argv[i+1], &arc);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      if(fabs(arc) <= AY_EPSILON || arc != arc)
		{
		  ay_error(AY_ERROR, argv[0], "Parameter arc must be != 0.");
		  return TCL_OK;
		}
	    }
	  i += 2;
	} /* while */
    } /* if */

  if(!(o = calloc(1, sizeof(ay_object))))
    {
      ay_error(AY_EOMEM, argv[0], NULL);
      return TCL_OK;
    }

  o->type = AY_IDNCURVE;
  ay_object_defaults(o);

  /* we create the nurbcurve-object */
  if(argc < 2)
    {
      ay_status = ay_nct_crtncircle(radius,
				 (ay_nurbcurve_object **)(void*)&(o->refine));
    }
  else
    {
      if(arc >= 360.0 || arc <= -360.0 || arc == 0.0)
	{
	  ay_status = ay_nct_crtncircle(radius,
				  (ay_nurbcurve_object **)(void*)&(o->refine));
	}
      else
	{
	  ay_status = ay_nct_crtncirclearc(radius, arc,
				  (ay_nurbcurve_object **)(void*)&(o->refine));
	}
    } /* if */

  if(ay_status)
    {
      free(o);
      ay_error(ay_status, argv[0], NULL);
      return TCL_OK;
    }

  ay_nct_recreatemp((ay_nurbcurve_object *)o->refine);

  ay_object_link(o);

  (void)ay_notify_object(o);

  (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_crtncircletcmd */


/** ay_nct_crtrecttcmd:
 *  Create a NURBS rectangle.
 *  Implements the \a crtNRect scripting interface command.
 *  Also implements the \a crtTrimRect scripting interface command.
 *  See also the corresponding section in the \ayd{sccrtnrect}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_crtrecttcmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
 int tcl_status = TCL_OK;
 int ay_status = AY_OK;
 ay_list_object *lev = ay_currentlevel;
 ay_object *parent = NULL, *p = NULL;
 ay_nurbpatch_object *patch = NULL;
 ay_nurbcurve_object *curve = NULL;
 double def_controls[20] = { 1.0, 1.0, 0.0, 1.0,
			     1.0,-1.0, 0.0, 1.0,
			     -1.0,-1.0, 0.0, 1.0,
			     -1.0, 1.0, 0.0, 1.0,
			     1.0, 1.0, 0.0, 1.0};
 double knots[7] = {0.0, 0.0, 0.25, 0.5, 0.75, 1.0, 1.0};
 double width = 1.0, height = 1.0;
 int is_peek = AY_FALSE;
 int i = 0, create_trim = AY_FALSE, notify_parent = AY_FALSE;
 ay_object *o = NULL;
 ay_list_object *sel = ay_selection;

  /* distinguish between
     crtNRect and
     crtTrimRect
     012^       */
  if(argv[0][3] == 'T')
    {
      create_trim = AY_TRUE;
    }
  else
    {
      if(argc > 1)
	{
	  i = 1;
	  while(i+1 < argc)
	    {
	      if((argv[i][0] == '-') && (argv[i][1] == 'w'))
		{
		  tcl_status = Tcl_GetDouble(interp, argv[i+1], &width);
		  AY_CHTCLERRRET(tcl_status, argv[0], interp);
		  if(fabs(width) <= AY_EPSILON || width != width)
		    {
		      ay_error(AY_ERROR, argv[0],
			       "Parameter width must be != 0.");
		      return TCL_OK;
		    }
		}
	      if((argv[i][0] == '-') && (argv[i][1] == 'h'))
		{
		  tcl_status = Tcl_GetDouble(interp, argv[i+1], &height);
		  AY_CHTCLERRRET(tcl_status, argv[0], interp);
		  if(fabs(height) <= AY_EPSILON || height != height)
		    {
		      ay_error(AY_ERROR, argv[0],
			       "Parameter height must be != 0.");
		      return TCL_OK;
		    }
		}
	      i += 2;
	    } /* while */
	} /* if have args */
      sel = NULL;
    } /* if crtNRect */

  do
    {
      is_peek = AY_FALSE;
      if(create_trim)
	{
	  /* find patch */
	  if(sel && (sel->object->type == AY_IDNPATCH))
	    {
	      patch = (ay_nurbpatch_object *)sel->object->refine;
	    }
	  else
	    {
	      if(lev->next)
		{
		  parent = lev->next->object;
		  if(parent)
		    {
		      if(parent->type == AY_IDNPATCH)
			{
			  patch = (ay_nurbpatch_object *)parent->refine;
			  sel = NULL;
			} /* if */
		    } /* if */
		} /* if */

	      if(!patch && sel)
		{
		  p = ay_peek_singleobject(sel->object, AY_IDNPATCH);
		  if(p)
		    {
		      is_peek = AY_TRUE;
		      patch = (ay_nurbpatch_object *)p->refine;
		    }
		}
	    } /* if */

	  if(!patch)
	    {
	      if(sel)
		sel = sel->next;
	      if(sel)
		continue;
	      else
		break;
	    }
	} /* if create_trim */

      /* create new curve object */
      if(!(o = calloc(1, sizeof(ay_object))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      o->type = AY_IDNCURVE;
      ay_object_defaults(o);

      if(!(curve = calloc(1, sizeof(ay_nurbcurve_object))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      if(!(curve->controlv = malloc(20*sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      if(!(curve->knotv = malloc(7*sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      curve->length = 5;
      curve->order = 2;
      curve->knot_type = AY_KTCUSTOM;
      curve->type = AY_CTCLOSED;
      curve->createmp = AY_TRUE;

      /* fill knotv */
      for(i = 0; i < 7; i++)
	curve->knotv[i] = knots[i];

      /* fill controlv */
      if(create_trim)
	{
	  curve->controlv[0] = patch->uknotv[patch->uorder-1];
	  curve->controlv[1] = patch->vknotv[patch->vorder-1];
	  curve->controlv[2] = 0.0;
	  curve->controlv[3] = 1.0;
	  curve->controlv[4] = patch->uknotv[patch->width];
	  curve->controlv[5] = patch->vknotv[patch->vorder-1];
	  curve->controlv[6] = 0.0;
	  curve->controlv[7] = 1.0;
	  curve->controlv[8] = patch->uknotv[patch->width];
	  curve->controlv[9] = patch->vknotv[patch->height];
	  curve->controlv[10] = 0.0;
	  curve->controlv[11] = 1.0;
	  curve->controlv[12] = patch->uknotv[patch->uorder-1];
	  curve->controlv[13] = patch->vknotv[patch->height];
	  curve->controlv[14] = 0.0;
	  curve->controlv[15] = 1.0;
	  curve->controlv[16] = patch->uknotv[patch->uorder-1];
	  curve->controlv[17] = patch->vknotv[patch->vorder-1];
	  curve->controlv[18] = 0.0;
	  curve->controlv[19] = 1.0;
	}
      else
	{
	  if(width != 1.0)
	    for(i = 0; i < 5; i++)
	      def_controls[i*4] *= width;
	  if(height != 1.0)
	    for(i = 0; i < 5; i++)
	      def_controls[i*4+1] *= height;

	  memcpy(curve->controlv, def_controls, 20*sizeof(double));
	}

      o->type = AY_IDNCURVE;
      o->refine = curve;
      ay_nct_recreatemp(curve);
      curve = NULL;

      /* link new curve object */
      if(create_trim && !is_peek)
	{
	  if(sel)
	    {
	      /* link to selected NPatch */
	      o->next = sel->object->down;
	      sel->object->down = o;
	    }
	  else
	    {
	      /* link as first child of our parent (the NPatch) */
	      o->next = parent->down;
	      parent->down = o;
	    }
	}
      else
	{
	  /* normal rectangles and trim-rects for tool objects =>
	     link to the end of the current level */
	  ay_object_link(o);
	} /* if */

      (void)ay_notify_object(o);
      notify_parent = AY_TRUE;
      o = NULL;

      if(sel)
	sel = sel->next;

cleanup:
      if(o)
	free(o);

      if(curve)
	ay_nct_destroy(curve);

      if(ay_status)
	{
	  ay_error(ay_status, argv[0], NULL);
	  break;
	}

      patch = NULL;
    } while(sel && create_trim);

  if(notify_parent)
    {
      (void)ay_notify_parent();
    }
  else
    {
      if(create_trim)
	ay_error(AY_ERROR, argv[0], "No NPatch selected or parent.");
    }

 return TCL_OK;
} /* ay_nct_crtrecttcmd */


/** ay_nct_crtcircbspcv
 *  Create control points for a circular closed B-Spline curve with
 *  desired number of control points, radius, arc, and order.
 *
 *  To improve performance for repeated calls with nearly identical
 *  parameters (e.g. by ay_npt_revolve()) this function supports
 *  *result to be non NULL, to avoid constant reallocation in repeated calls.
 *  Since no error check can be performed regarding the size of this memory
 *  region it is highly suggested to simply reuse the region allocated by
 *  a first call of this function.
 *
 * \param[in] sections  desired number of sections, must be > 0
 * \param[in] radius  desired radius
 * \param[in] arc  desired angle
 * \param[in] order  desired order, must be > 1
 * \param[in,out] result  pointer where to store the new control point array
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_crtcircbspcv(int sections, double radius, double arc, int order,
		    double **result)
{
 double *controlv, m[16], angle;
 int len, a, i;

  if(fabs(arc) > 360.0)
    return AY_ERROR;

  if((sections < 2) && (fabs(arc) > 180.0))
    return AY_ERROR;

  if(order < 2)
    return AY_ERROR;

  if(!result)
    return AY_ENULL;

  len = sections+(order-1);

  if(!*result)
    {
      if(!(controlv = malloc(len*4*sizeof(double))))
	{
	  return AY_EOMEM;
	}
    }
  else
    {
      controlv = *result;
    }

  angle = arc/sections;

  ay_trafo_identitymatrix(m);

  /* let the curve start at the x axis */
  ay_trafo_rotatematrix((order/2.0)*-angle, 0.0, 0.0, 1.0, m);

  for(i = 0; i < len; i++)
    {
      a = i*4;
      controlv[a] = radius;
      controlv[a+1] = 0.0;
      controlv[a+2] = 0.0;
      controlv[a+3] = 1.0;
      ay_trafo_rotatematrix(angle, 0.0, 0.0, 1.0, m);
      ay_trafo_apply3(&(controlv[a]), m);
    } /* for */

  /* return result */
  if(!*result)
    *result = controlv;

 return AY_OK;
} /* ay_nct_crtcircbspcv */


/** ay_nct_crtcircbsp
 * Create a circular closed B-Spline curve with desired number of
 * control points, radius, arc, and order.
 *
 * \param[in] sections  number of sections, must be > 0
 * \param[in] radius  desired radius
 * \param[in] arc  desired angle
 * \param[in] order  desired order, must be > 1
 * \param[in,out] result  pointer where to store the new curve object
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_crtcircbsp(int sections, double radius, double arc, int order,
		  ay_nurbcurve_object **result)
{
 int ay_status = AY_OK;
 ay_nurbcurve_object *curve = NULL;
 double *controlv = NULL;
 int len = 0;

  ay_status = ay_nct_crtcircbspcv(sections, radius, arc, order, &controlv);

  if(!ay_status)
    {
      len = sections+(order-1);

      ay_status = ay_nct_create(order, len, AY_KTBSPLINE, controlv, NULL,
				&curve);

      if(ay_status)
	{
	  free(controlv);
	  return ay_status;
	}

      /* return result */
      *result = curve;
    }

 return ay_status;
} /* ay_nct_crtcircbsp */


/** ay_nct_crtclosedbsptcmd:
 *  Create a circular B-Spline curve.
 *  Implements the \a crtClosedBS scripting interface command.
 *  See also the corresponding section in the \ayd{sccrtclosedbs}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_crtclosedbsptcmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_object *o = NULL;
 ay_nurbcurve_object *curve = NULL;
 int i, sections = 6, order = 4;
 double radius = 1.0, arc = 360.0;

  /* parse args */
  if(argc > 1)
    {
      i = 1;
      while(i+1 < argc)
	{
	  if(argv[i][0] == '-')
	    {
	      switch(argv[i][1])
		{
		case 'a':
		  tcl_status = Tcl_GetDouble(interp, argv[i+1], &arc);
		  AY_CHTCLERRRET(tcl_status, argv[0], interp);
		  if(fabs(arc) <= AY_EPSILON || arc != arc)
		    {
		      ay_error(AY_ERROR, argv[0],
			       "Parameter arc must be != 0.");
		      return TCL_OK;
		    }
		  break;
		case 'r':
		  tcl_status = Tcl_GetDouble(interp, argv[i+1], &radius);
		  AY_CHTCLERRRET(tcl_status, argv[0], interp);
		  if(radius <= AY_EPSILON || radius != radius)
		    {
		      ay_error(AY_ERROR, argv[0],
			       "Parameter radius must be > 0.");
		      return TCL_OK;
		    }
		  break;
		case 's':
		  tcl_status = Tcl_GetInt(interp, argv[i+1], &sections);
		  AY_CHTCLERRRET(tcl_status, argv[0], interp);
		  if(sections < 1)
		    {
		      ay_error(AY_ERROR, argv[0],
			       "Parameter sections must be >= 1.");
		      return TCL_OK;
		    }
		  break;
		case 'o':
		  tcl_status = Tcl_GetInt(interp, argv[i+1], &order);
		  AY_CHTCLERRRET(tcl_status, argv[0], interp);
		  if(sections < 2)
		    {
		      ay_error(AY_ERROR, argv[0],
			       "Parameter order must be >= 2.");
		      return TCL_OK;
		    }
		  break;
		default:
		  break;
		} /* switch */
	      i++;
	    } /* if is opt */
	  i++;
	} /* while */
    } /* if have args */

  /* create object */
  if(!(o = calloc(1, sizeof(ay_object))))
    {
      ay_error(AY_EOMEM, argv[0], NULL);
      return TCL_OK;
    }

  o->type = AY_IDNCURVE;
  ay_object_defaults(o);

  ay_status = ay_nct_crtcircbsp(sections, radius, arc, order,
				(ay_nurbcurve_object**)(void*)&(o->refine));

  if(ay_status || !o->refine)
    {
      ay_error(AY_ERROR, argv[0], "Failed to create closed B-Spline.");
      free(o);
      return TCL_OK;
    }

  curve = (ay_nurbcurve_object*)(o->refine);

  (void)ay_nct_close(curve);

  curve->createmp = AY_TRUE;
  ay_nct_recreatemp(curve);

  ay_object_link(o);

  (void)ay_notify_object(o);

  (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_crtclosedbsptcmd */


/** ay_nct_getorientation:
 *  return a negative value if curve is ccw (or cw? :)
 *  this is my fourth attempt on (and a complete rewrite of) this routine
 *  this time with support from Paul Bourke
 *
 * \param[in] curve  NURBS curve to interrogate
 * \param[in] stride  size of a point in the curve
 * \param[in] report  if AY_TRUE, errors are reported to the user
 * \param[in] plane  curve must lie in this plane (0 - XY, 1 - XZ, 2 - YZ)
 * \param[in,out] orient  -1 if curve is ccw wrt. the selected plane
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_getorientation(ay_nurbcurve_object *curve, int stride,
		      int report, int plane, double *orient)
{
 int i, j, k, l;
 double *cv = NULL, A = 0.0;
 char fname[] = "nct_getorientation";

  if(!curve || !orient)
    return AY_ENULL;

  cv = curve->controlv;
  if(curve->length <= 2)
    {
      if(report)
	ay_error(AY_EWARN, fname, "Need more than 2 control points.");
      return AY_ERROR;
    }

  switch(plane)
    {
    case 0:
      /* XY - Front */
      j = 0;
      k = stride;
      l = 1;
      break;
    case 1:
      /* XZ - Top */
      j = 0;
      k = stride;
      l = 2;
      break;
    case 2:
      /* YZ - Side */
      j = 1;
      k = stride+1;
      l = 1;
      break;
    default:
      if(report)
	ay_error(AY_EWARN, fname, "Plane must be 0, 1, or 2.");
      return AY_ERROR;
      break;
    } /* switch */

  for(i = 0; i < curve->length-1; i++)
    {
      if((fabs(cv[j]-cv[k]) > AY_EPSILON) ||
	 (fabs(cv[j+l]-cv[k+l]) > AY_EPSILON))
	{
	  A += cv[j]*cv[k+l]-cv[k]*cv[j+l];
	}
      j += stride;
      k += stride;
    } /* for */

  if(fabs(A) < AY_EPSILON)
    {
      if(report)
	ay_error(AY_ERROR, fname, "Could not determine orientation.");
      return AY_ERROR;
    }

  *orient = A;

 return AY_OK;
} /* ay_nct_getorientation */


/** Calculate orientation of NURBS curve.
 */
int
ay_nct_getorientation3d(ay_nurbcurve_object *curve, int stride,
			int report, double *orient)
{
 int ay_status;
 double A = 0.0;

  *orient = 0.0;

  ay_status = ay_nct_getorientation(curve, stride, AY_FALSE,
				    /*plane=*/0, &A);
  if(!ay_status)
    {
      if(fabs(A) > 0.0)
	*orient = A;
    }
  ay_status = ay_nct_getorientation(curve, stride, AY_FALSE,
				    /*plane=*/1, &A);
  if(!ay_status)
    {
      if(fabs(A) > fabs(*orient))
	*orient = A;
    }

  ay_status = ay_nct_getorientation(curve, stride, AY_FALSE,
				    /*plane=*/2, &A);
  if(!ay_status)
    {
      if(fabs(A) > fabs(*orient))
	*orient = A;
    }

  if(*orient == 0.0)
    return AY_ERROR;

 return AY_OK;
} /* ay_nct_getorientation3d */


/** ay_nct_getwinding:
 *  Calculate winding of arbitrary (3D) NURBS curve.
 *
 * \param[in] nc  NURBS curve to interrogate
 * \param[in] v  vector specifiying the direction from where to look
 *
 * \returns -1 if curve winding is negative, 1 if it is positive, 0
 *  if the curve is degenerate
 */
int
ay_nct_getwinding(ay_nurbcurve_object *nc, double *v)
{
 double *cv, *p1, *p2;
 double lxy = 0.0, lzy = 0.0, lxz = 0.0;
 int i, stride = 4, plane = -1;
 int a = 0, b = stride, winding = 0;

  if(!nc || !v)
    return 0;

  cv = nc->controlv;
  for(i = 0; i < nc->length; i++)
    {
      p1 = &(cv[a]);
      if(i == nc->length-1)
	p2 = cv;
      else
	p2 = &(cv[b]);
      if(!AY_V3COMP(p1, p2))
	{
	  lxy += p1[0]*p2[1]-p2[0]*p1[1];
	  lzy += p1[2]*p2[1]-p2[2]*p1[1];
	  lxz += p1[0]*p2[2]-p2[0]*p1[2];
	} /* if */

      a += stride;
      b += stride;
    } /* for */

  if((fabs(v[0]) < AY_EPSILON) &&
     (fabs(v[1]) > AY_EPSILON) && (fabs(v[2]) > AY_EPSILON))
    {
      if(fabs(v[1]) > fabs(v[2]))
	plane = AY_XY;
      else
	plane = AY_XZ;
    }

  if((plane == -1) && (fabs(v[1]) < AY_EPSILON) &&
     (fabs(v[1]) > AY_EPSILON) && (fabs(v[2]) > AY_EPSILON))
    {
      if(fabs(v[1]) > fabs(v[2]))
	plane = AY_YZ;
      else
	plane = AY_XZ;
    }

  if((plane == -1) && (fabs(v[2]) < AY_EPSILON) &&
     (fabs(v[0]) > AY_EPSILON) && (fabs(v[1]) > AY_EPSILON))
    {
      if(fabs(v[0]) > fabs(v[1]))
	plane = AY_YZ;
      else
	plane = AY_XZ;
    }

  if((plane == -1) && (fabs(v[2]) >= fabs(v[0])) && (fabs(v[2]) >= fabs(v[1])))
    {
      plane = AY_XY;
    }

  if((plane == -1) && (fabs(v[0]) >= fabs(v[1])) && (fabs(v[0]) >= fabs(v[2])))
    {
      plane = AY_YZ;
    }

  if((plane == -1) && (fabs(v[1]) >= fabs(v[0])) && (fabs(v[1]) >= fabs(v[2])))
    {
      plane = AY_XZ;
    }

  switch(plane)
    {
    case AY_XY:
      if(lxy > 0)
	winding = 1;
      else
	winding = -1;
      if(v[2] < 0)
	winding = -winding;
      break;
    case AY_YZ:
      if(lzy > 0)
	winding = 1;
      else
	winding = -1;
      if(v[0] > 0)
	winding = -winding;
      break;
    case AY_XZ:
      if(lxz > 0)
	winding = 1;
      else
	winding = -1;
      if(v[1] > 0)
	winding = -winding;
      break;
    default:
      break;
    }

 return winding;
} /* ay_nct_getwinding */


/** ay_nct_isclosed:
 *  Check whether or not a NURBS curve is closed by computing and
 *  and comparing the start and end points.
 *
 * \param[in] nc  NURBS curve to process
 *
 * \returns AY_TRUE if the curve is closed, AY_FALSE if not (and in error)
 */
int
ay_nct_isclosed(ay_nurbcurve_object *nc)
{
 int ay_status;
 double u, P1[4], P2[4];

  if(!nc)
    return AY_FALSE;

  u = nc->knotv[nc->order-1];
  ay_status = ay_nb_CurvePoint4D(nc->length-1, nc->order-1,
				 nc->knotv, nc->controlv,
				 u, P1);
  if(ay_status)
    return AY_FALSE;

  u = nc->knotv[nc->length];
  ay_status = ay_nb_CurvePoint4D(nc->length-1, nc->order-1,
				 nc->knotv, nc->controlv,
				 u, P2);
  if(ay_status)
    return AY_FALSE;

  if(AY_V3COMP(P1, P2))
    return AY_TRUE;
  else
    return AY_FALSE;

} /* ay_nct_isclosed */


/** ay_nct_settype:
 *  set the type attribute according to the actual control point
 *  configuration of a NURBS curve
 *
 * \param[in,out] nc  NURBS curve to process
 *
 */
void
ay_nct_settype(ay_nurbcurve_object *nc)
{
 int i, stride = 4;
 double *s, *e;

  if(!nc)
    return;

  nc->type = AY_CTOPEN;
  if(nc->length > 2)
    {
      s = nc->controlv;
      e = s + ((nc->length-1)*stride);
      if(AY_V4COMP(s, e))
	{
	  nc->type = AY_CTCLOSED;
	}
      else
	{
	  nc->type = AY_CTPERIODIC;
	  e = s + ((nc->length-1-(nc->order-2))*stride);
	  for(i = 0; i < nc->order-1; i++)
	    {
	      if(!AY_V4COMP(s, e))
		{
		  nc->type = AY_CTOPEN;
		  break;
		}
	      s += stride;
	      e += stride;
	    } /* for */
	} /* if */
    } /* if */

 return;
} /* ay_nct_settype */


/** ay_nct_applytrafo:
 *  applies transformations from object to all control points,
 *  then reset the objects transformation attributes
 *
 * \param[in,out] c  object of type NCurve to process
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_applytrafo(ay_object *c)
{
 int i, stride;
 double m[16];
 ay_nurbcurve_object *nc = NULL;

  if(!c)
    return AY_ENULL;

  if(c->type != AY_IDNCURVE)
    return AY_EWTYPE;

  nc = (ay_nurbcurve_object *)(c->refine);

  /* get curves transformation-matrix */
  ay_trafo_creatematrix(c, m);

  stride = 4;
  i = 0;
  while(i < nc->length*stride)
    {
      ay_trafo_apply3(&(nc->controlv[i]), m);

      i += stride;
    } /* while */

  /* reset curves transformation attributes */
  ay_trafo_defaults(c);

 return AY_OK;
} /* ay_nct_applytrafo */


/** ay_nct_getpntfromindex:
 * Get memory address of a single NCurve control point from its index
 * (performing bounds checking).
 *
 * \param[in] curve  NURBS curve to process
 * \param[in] index  index of desired control point
 * \param[in,out] p  where to store the resulting memory address
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_getpntfromindex(ay_nurbcurve_object *curve, int index, double **p)
{
 int stride = 4;
 char fname[] = "nct_getpntfromindex";

  if(!curve || !p)
    return AY_ENULL;

  if(index >= curve->length || index < 0)
    return ay_error_reportirange(fname, "\"index\"", 0, curve->length-1);

  *p = &(curve->controlv[index*stride]);

 return AY_OK;
} /* ay_nct_getpntfromindex */


/** ay_nct_euctohom:
 *  convert rational coordinates from euclidean to homogeneous style
 *
 * \param[in,out] nc  NURBS curve to process
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_euctohom(ay_nurbcurve_object *nc)
{
 int stride = 4;
 double *cv;
 int i, a;

  if(!nc)
    return AY_ENULL;

  if(!nc->is_rat)
    return AY_OK;

  cv = nc->controlv;

  a = 0;
  for(i = 0; i < nc->length; i++)
    {
      cv[a]   *= cv[a+3];
      cv[a+1] *= cv[a+3];
      cv[a+2] *= cv[a+3];
      a += stride;
    }

 return AY_OK;
} /* ay_nct_euctohom */


/** ay_nct_homtoeuc:
 *  convert rational coordinates from homogeneous to euclidean style
 *
 * \param[in,out] nc  NURBS curve to process
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_homtoeuc(ay_nurbcurve_object *nc)
{
 int stride = 4;
 double *cv;
 int i, a;

  if(!nc)
    return AY_ENULL;

  if(!nc->is_rat)
    return AY_OK;

  cv = nc->controlv;

  a = 0;
  for(i = 0; i < nc->length; i++)
    {
      cv[a]   /= cv[a+3];
      cv[a+1] /= cv[a+3];
      cv[a+2] /= cv[a+3];
      a += stride;
    }

 return AY_OK;
} /* ay_nct_homtoeuc */


/** ay_nct_concatobjs:
 * Concatenate multiple objects that are NURBS curves or provide
 * NURBS curves.
 *
 * \param[in] o  list of objects to concatenate
 * \param[in,out] result  pointer where to store the resulting object
 *
 * \returns AY_OK on success, error code otherwise.
 */
void
ay_nct_concatobjs(ay_object *o, ay_object **result)
{
 ay_object c = {0};
 ay_concatnc_object cn = {0};

  ay_object_defaults(&c);
  c.type = AY_IDCONCATNC;
  c.refine = &cn;
  c.down = o;
  cn.closed = AY_TRUE;
  cn.knot_type = 1; /* AY_KTCUSTOM */

  (void)ay_notify_object(&c);
  *result = cn.ncurve;

 return;
} /* ay_nct_concatobjs */


/** ay_nct_concatmultiple:
 * Concatenate multiple NURBS curve objects to a single curve.
 * NURBS curve providing objects are not supported here!
 * Order, tolerance, and display mode are taken from the first curve.
 *
 * \param[in] closed  determines whether to create a closed curve
 *  (0 - no, 1 - yes)
 * \param[in] knot_type  if 0, a knot vector of type NURB will be generated,
 *  otherwise a custom knot vector will be created that incorporates all
 *  features of the parameter curves
 * \param[in] fillgaps  determines whether gaps between the parameter curves
 *  should be filled with additional fillet curves
 * \param[in] curves  a list of curve objects
 * \param[in,out] result  pointer where to store the resulting object
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_concatmultiple(int closed, int knot_type, int fillgaps,
		      ay_object *curves,
		      ay_object **result)
{
 int ay_status;
 char fname[] = "nct_concatmultiple";
 double *newknotv = NULL, *newcontrolv = NULL, *ncv = NULL, lastu = 0.0;
 int numcurves = 0, i, k, a, order = -1, length = 0, ktype;
 int glu_display_mode = 0;
 double glu_sampling_tolerance = 0.0;
 ay_nurbcurve_object *nc;
 ay_object *o = NULL, *newo = NULL;

  /* check arguments */
  if(!curves || !result)
    {
      ay_error(AY_ENULL, fname, NULL);
      return AY_ERROR;
    }

  /* create new object */
  if(!(newo = calloc(1, sizeof(ay_object))))
    {
      ay_error(AY_EOMEM, fname, NULL);
      return AY_ERROR;
    }

  newo->type = AY_IDNCURVE;
  ay_object_defaults(newo);

  /* count curves; calculate length of concatenated curve */
  o = curves;
  while(o)
    {
      if(o->type == AY_IDNCURVE)
	{
	  nc = (ay_nurbcurve_object *)o->refine;
	  length += nc->length;
	  numcurves++;
	  /* take order, tolerance, and display_mode from first curve */
	  if(order == -1)
	    {
	      order = nc->order;
	      glu_sampling_tolerance = nc->glu_sampling_tolerance;
	      glu_display_mode = nc->display_mode;
	    }
	}
      o = o->next;
    } /* while */

  if(length == 0)
    return AY_ERROR;

  if(closed && !fillgaps)
    {
      length++;
    }

  /* construct new knot vector */
  if(knot_type == 0)
    {
      ktype = AY_KTNURB;
    }
  else
    {
      ktype = AY_KTCUSTOM;

      if(!(newknotv = malloc((length + order) * sizeof(double))))
	{
	  ay_error(AY_EOMEM, fname, NULL);
	  free(newo);
	  return AY_ERROR;
	}

      a = 0;
      lastu = 0.0;
      k = 0;
      o = curves;
      while(o)
	{
	  if(o->type == AY_IDNCURVE)
	    {
	      nc = (ay_nurbcurve_object *)o->refine;

	      for(i = k; i < nc->length+nc->order; i++)
		{
		  newknotv[a] = nc->knotv[i]+lastu;
		  a++;
		}

	      o = o->next;
	      while(o && (o->type != AY_IDNCURVE))
		o = o->next;

	      if(o && (o->type == AY_IDNCURVE))
		{
		  lastu = newknotv[a-1];
		  nc = (ay_nurbcurve_object *)o->refine;
		  k = nc->order;
		}
	    }
	  else
	    {
	      o = o->next;
	    }
	} /* while */

      if(closed && !fillgaps)
	{
	  for(i = a; i < length+order; i++)
	    {
	      newknotv[i] = newknotv[a-1]+1;
	    }
	}
    } /* if */

  /* construct new control point vector */
  if(!(newcontrolv = malloc(length * 4 * sizeof(double))))
    {
      ay_error(AY_EOMEM, fname, NULL);
      free(newo);
      if(knot_type != 0)
	free(newknotv);
      return AY_ERROR;
    }

  ncv = newcontrolv;
  o = curves;
  while(o)
    {
      if(o->type == AY_IDNCURVE)
	{
	  nc = (ay_nurbcurve_object *)o->refine;
	  memcpy(ncv, nc->controlv, nc->length * 4 * sizeof(double));
	  ncv += (nc->length * 4);
	}
      o = o->next;
    } /* while */

  if(closed && !fillgaps)
    {
      memcpy(&(newcontrolv[(length-1)*4]), newcontrolv, 4 * sizeof(double));
    }

  ay_status = ay_nct_create(order, length,
			    ktype, newcontrolv, newknotv,
			    (ay_nurbcurve_object **)(void*)&(newo->refine));

  if(ay_status)
    {
      free(newo);
      if(knot_type != 0)
	free(newknotv);
      free(newcontrolv);
      ay_error(ay_status, fname, NULL);
      return AY_ERROR;
    }

  nc = (ay_nurbcurve_object *)newo->refine;
  if(nc)
    {
      ay_nct_recreatemp(nc);

      nc->glu_sampling_tolerance = glu_sampling_tolerance;
      nc->display_mode = glu_display_mode;
    }

  *result = newo;

 return AY_OK;
} /* ay_nct_concatmultiple */


/** ay_nct_fillgap:
 *  create a fillet curve between the last point of curve \a c1
 *  and the first point of curve \a c2; if \a order is 2, a simple linear
 *  curve will be created, else if tanlen is != 0.0 the fillet
 *  will be a four point NURBS curve with internal points matching the
 *  tangents of the endpoints of the curves c1/c2 (the tangents will
 *  be scaled additionally by \a tanlen before placing the internal
 *  control points), resulting in a G1 continous fillet;
 *  if tanlen is 0.0, global interpolation will be used to create
 *  a fillet curve with exactly matching tangents, resulting in a C1
 *  continous fillet; both, G1 and C1, fillets will finally be
 *  elevated to the desired order
 *
 * \param[in] order  desired order of fillet curve
 * \param[in] tanlen
 *  if > 0.0, scale of tangents, expressed as ratio
 *  of the distance between last point in c1 and first point in c2;
 *  if < 0.0, scale of tangents, expressed directly (the provided
 *  value is negated before use, -0.1 => 0.1)
 * \param[in] c1  first curve
 * \param[in] c2  second curve
 * \param[in,out] result  where to store the fillet curve
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_fillgap(int order, double tanlen,
	       ay_nurbcurve_object *c1, ay_nurbcurve_object *c2,
	       ay_object **result)
{
 double p1[4] = {0}, p2[4] = {0}, p3[4], p4[4], n1[3], n2[3], l[3];
 double *U, *Pw, u, d, len;
 double *controlv = NULL;
 int n, p, numcontrol;
 ay_object *o = NULL;
 ay_nurbcurve_object *nc = NULL;
 int ay_status = AY_OK;

  if(!c1 || !c2 || !result)
    return AY_ENULL;

  /* get coordinates of the last point of c1 and first point of c2
     as well as the first derivative in those points */
  n = c1->length;
  p = c1->order-1;
  U = c1->knotv;
  Pw = c1->controlv;
  u = U[n];
  ay_status = ay_nb_CurvePoint4D(n-1, p, U, Pw, u, p1);
  p1[3] = 1.0;
  /*ay_status +=*/ ay_nb_FirstDer4D(n-1, p, U, Pw, u, n1);

  n = c2->length;
  p = c2->order-1;
  U = c2->knotv;
  Pw = c2->controlv;
  u = U[p];
  ay_status += ay_nb_CurvePoint4D(n-1, p, U, Pw, u, p2);
  p2[3] = 1.0;
  /*ay_status +=*/ ay_nb_FirstDer4D(n-1, p, U, Pw, u, n2);

  if(ay_status)
    return AY_ERROR;

  /* check whether p1 and p2 are sufficiently different */
  if((fabs(p1[0] - p2[0]) < AY_EPSILON) &&
     (fabs(p1[1] - p2[1]) < AY_EPSILON) &&
     (fabs(p1[2] - p2[2]) < AY_EPSILON))
    {
      /* No, no fillet needs to be created, just bail out! */
      return AY_OK;
    }

  if(tanlen == 0.0 && order > 2)
    {
      /* create C1 fillet by interpolation */

      AY_V3SCAL(n1, -1.0)
      AY_V3SCAL(n1, 0.25)
      AY_V3SCAL(n2, 0.25)
      AY_V3SUB(p3, p1, n1)
      AY_V3SUB(p4, p2, n2)

      if(!(Pw = malloc(2*3*sizeof(double))))
	return AY_EOMEM;

      memcpy(Pw, p1, 3*sizeof(double));
      memcpy(&(Pw[3]), p2, 3*sizeof(double));
      ay_status = ay_ict_interpolateG3D(/*iorder=*/3, /*length=*/2,
	  /*sdlen=*/0.0, /*edlen=*/0.0, /*have_end_derivs=*/AY_TRUE,
	  /*param_type=*/AY_KTCHORDAL, /*controlv=*/Pw,
	  /*sderiv=*/p3, /*ederiv=*/p4, &nc);
      free(Pw);
      if(ay_status || !nc)
	return AY_ERROR;

      /* elevate fillet to target order */
      if(order > 3)
	{
	  ay_status = ay_nct_elevate(nc, /*new_order=*/order);
	  if(ay_status)
	    {
	      ay_nct_destroy(nc);
	      return AY_ERROR;
	    }
	}
    }
  else
    {
      /* create simple G1 fillet */

      /* normalize n1 */
      len = AY_V3LEN(n1);
      AY_V3SCAL(n1, 1.0/len);

      /* flip n1 */
      AY_V3SCAL(n1, -1.0)

      /* normalize n2 */
      len = AY_V3LEN(n2);
      AY_V3SCAL(n2, 1.0/len);

      p3[3] = 1.0;
      p4[3] = 1.0;

      if(tanlen > 0.0)
	{
          AY_V3SUB(l, p2, p1)
          d = AY_V3LEN(l);
	  AY_V3SCAL(n1, d*tanlen)
	  AY_V3SCAL(n2, d*tanlen)
	}
      else
	{
	  AY_V3SCAL(n1, -tanlen)
	  AY_V3SCAL(n2, -tanlen)
	}
      AY_V3SUB(p3, p1, n1)
      AY_V3SUB(p4, p2, n2)

      if(order == 2)
	numcontrol = 2;
      else
	numcontrol = 4;

      /* fill new controlv */
      if(!(controlv = malloc(numcontrol*4*sizeof(double))))
	return AY_EOMEM;

      if(order == 2)
	{
	  memcpy(&(controlv[0]), p1, 4*sizeof(double));
	  memcpy(&(controlv[4]), p2, 4*sizeof(double));
	}
      else
	{
	  memcpy(&(controlv[0]), p1, 4*sizeof(double));
	  memcpy(&(controlv[4]), p3, 4*sizeof(double));
	  memcpy(&(controlv[8]), p4, 4*sizeof(double));
	  memcpy(&(controlv[12]), p2, 4*sizeof(double));
	}

      if(order == 3)
	{
	  ay_status = ay_nct_create(3, numcontrol, AY_KTNURB, controlv,
				    /*knotv=*/NULL, &nc);
	}
      else
	{
	  ay_status = ay_nct_create(numcontrol, numcontrol, AY_KTNURB,
				    controlv, /*knotv=*/NULL, &nc);
	}

      if(ay_status)
	{ free(controlv); return AY_ERROR; }

      /* elevate fillet to target order */
      if(order > 4)
	{
	  ay_status = ay_nct_elevate(nc, /*new_order=*/order);
	  if(ay_status)
	    {
	      ay_nct_destroy(nc);
	      return AY_ERROR;
	    }
	}
    } /* if tanlen != 0.0 */

  if(!(o = calloc(1, sizeof(ay_object))))
    {
      ay_nct_destroy(nc);
      return AY_EOMEM;
    }

  ay_object_defaults(o);

  o->refine = nc;
  o->type = AY_IDNCURVE;

  *result = o;

 return AY_OK;
} /* ay_nct_fillgap */


/** ay_nct_fillgaps:
 *  Create fillet curves (using ay_nct_fillgap() above) for all
 *  gaps in the list of curves pointed to by \a curves and insert
 *  the fillets right in this list.
 *
 * \param[in] closed  if AY_TRUE, attempt to create a fillet between
 *  the end of the last curve and the start of the first curve in the
 *  provided list of curves
 * \param[in] order  desired order of the fillets
 * \param[in] tanlen  length of tangents
 * \param[in,out] curves  list of curves, the fillets will be mixed in
 *  the list in the right places
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_fillgaps(int closed, int order, double tanlen, ay_object *curves)
{
 ay_object *c = NULL, *fillet = NULL, *last = NULL;
 int ay_status = AY_OK;

  if(!curves)
    return AY_ENULL;

  c = curves;
  while(c)
    {
      if(c->next)
	{
	  fillet = NULL;
	  ay_status = ay_nct_fillgap(order, tanlen,
				     (ay_nurbcurve_object *)c->refine,
				     (ay_nurbcurve_object *)c->next->refine,
				     &fillet);
	  if(ay_status)
	    return AY_ERROR;
	  if(fillet)
	    {
	      fillet->next = c->next;
	      c->next = fillet;
	      c = c->next;
	    } /* if */
	} /* if */
      last = c;
      c = c->next;
    } /* while */

  if(closed)
    {
      fillet = NULL;
      ay_status = ay_nct_fillgap(order, tanlen,
				 (ay_nurbcurve_object *)last->refine,
				 (ay_nurbcurve_object *)curves->refine,
				 &fillet);
      if(ay_status)
	return AY_ERROR;

      if(fillet)
	last->next = fillet;
    } /* if closed */

 return ay_status;
} /* ay_nct_fillgaps */


/* ay_nct_arrange:
 *  arrange objects in o along trajectory t (a NURBS curve);
 *  if rotate is AY_TRUE, additionally rotate all objects in
 *  o so that their local X axis is parallel to the curve
 *  points tangent
 */
int
ay_nct_arrange(ay_object *o, ay_object *t, int rotate)
{
 int ay_status = AY_OK;
 ay_object *l;
 ay_nurbcurve_object *tr;
 int i = 0, a = 0, stride;
 double u, p1[4];
 double T0[3] = {0.0,0.0,-1.0};
 double T1[3] = {0.0,0.0,0.0};
 double A[3] = {0.0,0.0,0.0};
 double len = 0.0, plen = 0.0;
 double mtr[16];
 double *trcv = NULL, angle, quat[4], euler[3];
 unsigned int n = 0;

  if(!o || !t)
    return AY_ENULL;

  if(t->type != AY_IDNCURVE)
    return AY_ERROR;

  /* count objects */
  l = o;
  while(l)
    {
      n++;
      l = l->next;
    }

  stride = 4;
  tr = (ay_nurbcurve_object *)(t->refine);

  /* apply all transformations to trajectory curves controlv */
  if(AY_ISTRAFO(t))
    {
      if(!(trcv = malloc(tr->length*stride*sizeof(double))))
	return AY_EOMEM;
      memcpy(trcv, tr->controlv, tr->length*stride*sizeof(double));

      ay_trafo_creatematrix(t, mtr);
      a = 0;
      for(i = 0; i < tr->length; i++)
	{
	  ay_trafo_apply4(&(trcv[a]), mtr);
	  a += stride;
	}
    }
  else
    {
      trcv = tr->controlv;
    }

  plen = fabs(tr->knotv[tr->length] - tr->knotv[tr->order-1]);

  T0[0] = 1.0;
  T0[1] = 0.0;
  T0[2] = 0.0;

  l = NULL;
  i = 0;

  while(o)
    {
      if(tr->type == AY_CTOPEN)
	{
	  if(n > 1)
	    u = tr->knotv[tr->order-1]+(((double)i/(n-1))*plen);
	  else
	    u = tr->knotv[tr->order-1];
	}
      else
	{
	  u = tr->knotv[tr->order-1]+(((double)i/n)*plen);
	}

      /* calculate new translation */
      ay_status = ay_nb_CurvePoint4D(tr->length-1, tr->order-1,
				     tr->knotv, trcv,
				     u, p1);
      if(ay_status)
	goto cleanup;

      o->movx = p1[0];
      o->movy = p1[1];
      o->movz = p1[2];

      if(l)
	{
	  memcpy(o->quat, l->quat, 4*sizeof(double));
	  o->rotx = l->rotx;
	  o->roty = l->roty;
	  o->rotz = l->rotz;
	}
      else
	{
	  memset(o->quat, 0, 3*sizeof(double));
	  o->quat[3] = 1.0;
	}

      if(rotate)
	{
	  ay_nb_FirstDer4D(tr->length-1, tr->order-1, tr->knotv,
			   trcv, u, T1);

	  len = AY_V3LEN(T1);
	  AY_V3SCAL(T1,(1.0/len));

	  if(((fabs(T1[0]-T0[0]) > AY_EPSILON) ||
	      (fabs(T1[1]-T0[1]) > AY_EPSILON) ||
	      (fabs(T1[2]-T0[2]) > AY_EPSILON)))
	    {
	      AY_V3CROSS(A,T0,T1)
	      len = AY_V3LEN(A);
	      AY_V3SCAL(A,(1.0/len))

	      angle = AY_V3DOT(T0,T1);
	      if(angle <= -1.0)
		angle = -AY_PI;
	      else
		if(angle >= 1.0)
		  angle = 1.0;
		else
		  angle = acos(angle);

	      if(fabs(angle) > AY_EPSILON)
		{
		  ay_quat_axistoquat(A, -angle, quat);
		  ay_quat_add(quat, o->quat, o->quat);
		  ay_quat_toeuler(o->quat, euler);
		  o->rotx = AY_R2D(euler[0]);
		  o->roty = AY_R2D(euler[1]);
		  o->rotz = AY_R2D(euler[2]);
		} /* if */
	    } /* if */

	  memcpy(T0, T1, 3*sizeof(double));
	} /* if rotate */

      i++;
      l = o;
      o = o->next;
    } /* while */

  /* clean up */

cleanup:

  if(trcv != tr->controlv)
    free(trcv);

 return ay_status;
} /* ay_nct_arrange */


/** ay_nct_rescaleknvtcmd:
 *  rescale the knot vectors of the selected NURBS curves
 *  - to the range 0.0 - 1.0 (no arguments)
 *  - to a specific range (-r min max)
 *  - so that all knots have a minimum guaranteed distance (-d mindist)
 *  Implements the \a rescaleknNC scripting interface command.
 *  See also the corresponding section in the \ayd{screscaleknnc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_rescaleknvtcmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_object *src = NULL;
 ay_nurbcurve_object *curve = NULL;
 double rmin = 0.0, rmax = 1.0, mindist = 1.0e-04;
 int i = 1, mode = 0;
 int notify_parent = AY_FALSE;

  /* parse args */
  if(argc > 2)
    {
      while(i+1 < argc)
	{
	  if(!strcmp(argv[i], "-r"))
	    {
	      if(argc > i+2)
		{
		  mode = 0;
		  tcl_status = Tcl_GetDouble(interp, argv[i+1], &rmin);
		  AY_CHTCLERRRET(tcl_status, argv[0], interp);
		  if(rmin != rmin)
		    {
		      ay_error_reportnan(argv[0], "rmin");
		      return TCL_OK;
		    }
		  tcl_status = Tcl_GetDouble(interp, argv[i+2], &rmax);
		  AY_CHTCLERRRET(tcl_status, argv[0], interp);
		  if(rmax != rmax)
		    {
		      ay_error_reportnan(argv[0], "rmax");
		      return TCL_OK;
		    }
		}
	      else
		{
		  ay_error(AY_EARGS, argv[0], "-r min max");
		}
	    }
	  if(!strcmp(argv[i], "-d"))
	    {
	      mode = 1;
	      tcl_status = Tcl_GetDouble(interp, argv[i+1], &mindist);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      if(mindist != mindist)
		{
		  ay_error_reportnan(argv[0], "mindist");
		  return TCL_OK;
		}
	    }
	  i += 2;
	} /* while */
    } /* if have args */

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  while(sel)
    {
      src = sel->object;
      if(src->type != AY_IDNCURVE)
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	}
      else
	{
	  curve = (ay_nurbcurve_object*)src->refine;

	  if(mode)
	    {
	      ay_status = ay_knots_rescaletomindist(curve->length+
						    curve->order,
						    curve->knotv,
						    mindist);
	    }
	  else
	    {
	      ay_status = ay_knots_rescaletorange(curve->length+
						  curve->order,
						  curve->knotv,
						  rmin, rmax);
	    }

	  if(ay_status)
	    {
	      ay_error(ay_status, argv[0], "Could not rescale knots.");
	    }

	  (void)ay_notify_object(src);
	  notify_parent = AY_TRUE;
	  src->modified = AY_TRUE;
	} /* if is NCurve */

      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_rescaleknvtcmd */


/** ay_nct_gettorsion:
 * compute torsion of a NURBS curve
 *
 * \param[in] nc  NURBS curve to interrogate
 * \param[in] relative  if AY_TRUE, interpret \a u in a relative way
 * \param[in] u  parametric value
 *
 * \returns torsion at designated position on the curve
 */
double
ay_nct_gettorsion(ay_nurbcurve_object *nc, int relative, double u)
{
 double C[12] = {0}, *C0, *C1, *C2, *C3, *A = NULL, v[3];
 double bin[4*4];
 double t = 0;
 double wders[4] = {0};
 int o, span = 0, i, j, k, r;
 double *nders = NULL;
 double *Pw, *P, *U;
 double numer, denom;

  if(!nc)
    return 0.0;

  if(relative)
    {
      if((u < 0.0) || (u > 1.0))
	return 0.0;

      u = nc->knotv[nc->order-1] +
	(nc->knotv[nc->length] - nc->knotv[nc->order-1])*u;
    }
  else
    {
      if((u < nc->knotv[0]) || (u > nc->knotv[nc->length+nc->order-1]))
	return 0.0;
    }

  if(nc->order < 3)
    return 0.0;

  P = nc->controlv;
  Pw = P;
  U = nc->knotv;
  o = nc->order;

  if(!(nders = calloc(4 * o, sizeof(double))))
    return 0.0;

  span = ay_nb_FindSpan(nc->length-1, (o-1), u, U);

  ay_nb_DersBasisFuns(span, u, (o-1), 3, U, nders);

  if(nc->is_rat)
    {
      if(!(A = calloc(4 * 3, sizeof(double))))
	return 0.0;

      ay_nb_Bin(4, 4, bin);

      C0 = &(A[0]);
      C1 = &(A[3]);
      C2 = &(A[6]);
      C3 = &(A[9]);

      for(j = 0; j < o; j++)
	{
	  k = (span-(o-1)+j)*4;
	  C0[0] = C0[0] + nders[j] * (Pw[k]*Pw[k+3]);
	  C0[1] = C0[1] + nders[j] * (Pw[k+1]*Pw[k+3]);
	  C0[2] = C0[2] + nders[j] * (Pw[k+2]*Pw[k+3]);
	  wders[0] = wders[0] + nders[j] * (Pw[k+3]);

	  C1[0] = C1[0] + nders[o+j] * (Pw[k]*Pw[k+3]);
	  C1[1] = C1[1] + nders[o+j] * (Pw[k+1]*Pw[k+3]);
	  C1[2] = C1[2] + nders[o+j] * (Pw[k+2]*Pw[k+3]);
	  wders[1] = wders[1] + nders[o+j] * (Pw[k+3]);

	  C2[0] = C2[0] + nders[o*2+j] * (Pw[k]*Pw[k+3]);
	  C2[1] = C2[1] + nders[o*2+j] * (Pw[k+1]*Pw[k+3]);
	  C2[2] = C2[2] + nders[o*2+j] * (Pw[k+2]*Pw[k+3]);
	  wders[2] = wders[2] + nders[o*2+j] * (Pw[k+3]);

	  C3[0] = C3[0] + nders[o*3+j] * (Pw[k]*Pw[k+3]);
	  C3[1] = C3[1] + nders[o*3+j] * (Pw[k+1]*Pw[k+3]);
	  C3[2] = C3[2] + nders[o*3+j] * (Pw[k+2]*Pw[k+3]);
	  wders[3] = wders[3] + nders[o*3+j] * (Pw[k+3]);
	} /* for */

      for(k = 0; k <= 3; k++)
	{
	  memcpy(v, &(A[k*3]), 3 * sizeof(double));
	  for(i = k; i > 0; i--)
	    {
	      v[0] = v[0] - bin[k*4+i]*wders[i]*C[(k-i)*3];
	      v[1] = v[1] - bin[k*4+i]*wders[i]*C[(k-i)*3+1];
	      v[2] = v[2] - bin[k*4+i]*wders[i]*C[(k-i)*3+2];
	    }
	  memcpy(&(C[k*3]), v, 3*sizeof(double));
	  C[k*3]   /= wders[0];
	  C[k*3+1] /= wders[0];
	  C[k*3+2] /= wders[0];
	}

      for(k = 0; k < 3; k++)
	memcpy(&(C[k*3]), &(C[k*3+3]), 3*sizeof(double));

      free(A);
    }
  else
    {
      for(j = 0; j < o; j++)
	{
	  r = (span-(o-1)+j)*4;

	  C[0] = C[0] + nders[o+j] * P[r];
	  C[1] = C[1] + nders[o+j] * P[r+1];
	  C[2] = C[2] + nders[o+j] * P[r+2];

	  C[3] = C[3] + nders[(o*2)+j] * P[r];
	  C[4] = C[4] + nders[(o*2)+j] * P[r+1];
	  C[5] = C[5] + nders[(o*2)+j] * P[r+2];

	  C[6] = C[6] + nders[(o*3)+j] * P[r];
	  C[7] = C[7] + nders[(o*3)+j] * P[r+1];
	  C[8] = C[8] + nders[(o*3)+j] * P[r+2];
	}
    }

  if(nders)
    free(nders);

  /*
       x'''*(y'z'' - y''z') + y'''*(x''z' - x'z'') + z'''*(x'y''-x''y')
   t = ----------------------------------------------------------------
          (y'z'' - y''z')^2 + (x''z' - x'z'')^2 + (x'y'' - x''y')^2
  */
  numer = C[6]*(C[1]*C[5]-C[4]*C[2]) +
          C[7]*(C[3]*C[2]-C[0]*C[5]) +
          C[8]*(C[0]*C[4]-C[3]*C[1]);
  denom = (C[1]*C[5]-C[4]*C[2])*(C[1]*C[5]-C[4]*C[2]) +
	  (C[3]*C[2]-C[0]*C[5])*(C[3]*C[2]-C[0]*C[5]) +
	  (C[0]*C[4]-C[3]*C[1])*(C[0]*C[4]-C[3]*C[1]);

  if(fabs(denom) > AY_EPSILON)
    t = numer/denom;

 return t;
} /* ay_nct_gettorsion */


/** ay_nct_getcurvature:
 * compute curvature of a NURBS curve
 *
 * \param[in] nc  NURBS curve to interrogate
 * \param[in] relative  if AY_TRUE, interpret \a u in a relative way
 * \param[in] u  parametric value
 *
 * \returns curvature at designated position on the curve
 */
double
ay_nct_getcurvature(ay_nurbcurve_object *nc, int relative, double u)
{
 double vel[3], acc[3], cross[3];
 double velsqrlen, numer, denom;

  if(!nc)
    return 0.0;

  if(relative)
    {
      if((u < 0.0) || (u > 1.0))
	return 0.0;

      u = nc->knotv[nc->order-1] +
	(nc->knotv[nc->length] - nc->knotv[nc->order-1])*u;
    }
  else
    {
      if((u < nc->knotv[0]) || (u > nc->knotv[nc->length+nc->order-1]))
	return 0.0;
    }

  if(nc->order < 3)
    return 0.0;

  if(nc->is_rat)
    ay_nb_FirstDer4D(nc->length-1, nc->order-1, nc->knotv, nc->controlv,
		     u, vel);
  else
    ay_nb_FirstDer3D(nc->length-1, nc->order-1, nc->knotv, nc->controlv,
		     u, vel);

  velsqrlen = (vel[0]*vel[0])+(vel[1]*vel[1])+(vel[2]*vel[2]);

  if(velsqrlen > AY_EPSILON)
    {
      if(nc->is_rat)
	ay_nb_SecondDer4D(nc->length-1, nc->order-1, nc->knotv, nc->controlv,
			  u, acc);
      else
	ay_nb_SecondDer3D(nc->length-1, nc->order-1, nc->knotv, nc->controlv,
			  u, acc);
      AY_V3CROSS(cross, vel, acc);
      numer = AY_V3LEN(cross);
      denom = pow(velsqrlen, 1.5);
      return (numer/denom);
    }

 return 0.0;
} /* ay_nct_getcurvature */


/* ay_nct_curvplottcmd:
 *  create a curvature plot NURBS curve from the selected NURBS curve(s)
 */
int
ay_nct_curvplottcmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
 int ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_object *o, *po = NULL;
 ay_nurbcurve_object *c, *c2 = NULL;
 double width = 5.0, scale = 1.0, t, dt, *controlv, umin = 0.0, umax = 0.0;
 int a = 0, b = 0, samples = 100, freepo;
 char *cname;
 Tcl_DString ds;

  if(argc >= 2)
    Tcl_GetInt(interp, argv[1], &samples);

  if(argc >= 3)
    Tcl_GetDouble(interp, argv[2], &width);

  if(argc >= 4)
    Tcl_GetDouble(interp, argv[3], &scale);

  while(sel)
    {
      freepo = AY_FALSE;
      c = NULL;
      cname = NULL;
      if(sel->object->type == AY_IDNCURVE)
	{
	  c = (ay_nurbcurve_object *)sel->object->refine;
	  cname = sel->object->name;
	}
      else
	{
	  (void)ay_provide_object(sel->object, AY_IDNCURVE, &po);
	  if(po)
	    {
	      freepo = AY_TRUE;
	      c = (ay_nurbcurve_object *)po->refine;
	      cname = sel->object->name;
	    }
	}

      if(c)
	{
	  if(!(controlv = calloc((samples+1)*4, sizeof(double))))
	    {
	      ay_error(AY_EOMEM, argv[0], NULL);
	      return TCL_OK;
	    }

	  if(!(o = calloc(1, sizeof(ay_object))))
	    {
	      free(controlv);
	      ay_error(AY_EOMEM, argv[0], NULL);
	      return TCL_OK;
	    }
	  ay_object_defaults(o);
	  o->type = AY_IDNCURVE;

	  umin = c->knotv[c->order-1];
	  umax = c->knotv[c->length];

	  dt = (umax-umin)/((double)samples);
	  a = 0;
	  b = 0;
	  for(t = umin; t < umax; t += dt)
	    {
	      controlv[a] = (double)b*width/samples;
	      controlv[a+1] = ay_nct_getcurvature(c, AY_FALSE, t)*scale;
	      controlv[a+3] = 1.0;
	      a += 4;
	      b++;
	    }

	  ay_status = ay_nct_create(4, samples, AY_KTNURB, controlv, NULL,
				    &c2);

	  if(!c2 || ay_status)
	    {
	      free(o); free(controlv);
	      if(freepo)
		(void)ay_object_deletemulti(po, AY_FALSE);
	      sel = sel->next;
	      continue;
	    }

	  Tcl_DStringInit(&ds);
	  Tcl_DStringAppend(&ds, "Curvature", -1);
	  if(cname)
	    {
	      Tcl_DStringAppend(&ds, "_of_", -1);
	      Tcl_DStringAppend(&ds, cname, -1);
	    }
	  if((o->name = calloc(Tcl_DStringLength(&ds)+1, sizeof(char))))
	    {
	      strcpy(o->name, Tcl_DStringValue(&ds));
	    }
	  o->refine = c2;
	  ay_object_link(o);
	  Tcl_DStringFree(&ds);
	} /* if */

      if(freepo)
	(void)ay_object_deletemulti(po, AY_FALSE);

      sel = sel->next;
    } /* while */

  (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_curvplottcmd */


/** ay_nct_getcurvaturetcmd:
 *  compute gaussian curvature of a NURBS curve
 *  Implements the \a curvatNC scripting interface command.
 *  Also implements the \a torsionNC scripting interface command.
 *  See also the corresponding section in the \ayd{sccurvatnc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_getcurvaturetcmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
 int tcl_status = TCL_OK;
 ay_object *o = NULL, *po = NULL;
 ay_nurbcurve_object *curve = NULL;
 ay_list_object *sel = ay_selection;
 double u = 0.0, k;
 int i = 1, relative = AY_FALSE;
 int is_gettorsion = AY_FALSE;
 Tcl_Obj *to = NULL, *res = NULL;

  if(argv[0][0] == 't')
    is_gettorsion = AY_TRUE;

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  while(i < argc)
    {
      if(argv[i][0] == '-' && argv[i][1] == 'u')
	{
	  tcl_status = Tcl_GetDouble(interp, argv[i+1], &u);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  if(u != u)
	    {
	      ay_error_reportnan(argv[0], "u");
	      return TCL_OK;
	    }
	  i++;
	}
      else
      if(argv[i][0] == '-' && argv[i][1] == 'r')
	{
	  relative = AY_TRUE;
	}
      else
	{
	  ay_error(AY_EARGS, argv[0], "[-r] -u u");
	  return TCL_OK;
	}
      i++;
    }

  while(sel)
    {
      o = sel->object;
      curve = NULL;

      if(o->type == AY_IDNCURVE)
	{
	  curve = (ay_nurbcurve_object*)o->refine;
	}
      else
	{
	  po = ay_peek_singleobject(sel->object, AY_IDNCURVE);
	  if(po)
	    {
	      curve = (ay_nurbcurve_object *)po->refine;
	    }
	}

      if(curve)
	{
	  if(is_gettorsion)
	    k = ay_nct_gettorsion(curve, relative, u);
	  else
	    k = ay_nct_getcurvature(curve, relative, u);

	  if(to)
	    {
	      if(!res)
		res = Tcl_NewListObj(0, NULL);
	      to = Tcl_NewDoubleObj(k);
	      Tcl_ListObjAppendElement(interp, res, to);
	    }
	  else
	    {
	      to = Tcl_NewDoubleObj(k);
	    }
	}
      else
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	} /* if have NCurve */

      sel = sel->next;
    } /* while */

  /* return result */
  if(res)
    Tcl_SetObjResult(interp, res);
  else
    if(to)
      Tcl_SetObjResult(interp, to);

 return TCL_OK;
} /* ay_nct_getcurvaturetcmd */


/* ay_nct_intersect:
 *
 */
int
ay_nct_intersect(ay_nurbcurve_object *cu, ay_nurbcurve_object *cv,
		 double *intersection)
{
 int ay_status = AY_OK;
#if 0
 double *bu = NULL, *bv = NULL; /* Bezier segment arrays */
 double *su, *sv;
 double cuminx, cumaxx, cvminx, cvmaxx;
 double cuminy, cumaxy, cvminy, cvmaxy;
 double cuminz, cumaxz, cvminz, cvmaxz;
 int nbu = 0, nbv = 0;
 int stride = 4, i, j, k, overlap;

  if(!cu || !cv || !intersection)
    return AY_ENULL;

  /* decompose both curves into Bezier segments */
  if(!(bu = calloc(cu->order*stride, sizeof(double))))
    return AY_EOMEM;

  ay_status = ay_nb_DecomposeCurve(4, cu->length, cu->order-1, cu->knotv,
				   cu->controlv, &nbu, &bu);

  if(!(bv = calloc(cv->order*stride, sizeof(double))))
    return AY_EOMEM;

  ay_status = ay_nb_DecomposeCurve(4, cv->length, cv->order-1, cv->knotv,
				   cv->controlv, &nbv, &bv);

  /* find intersecting Bezier segments (use convex hull property to
     exclude all pairs that cannot intersect) */
  for(i = 0; i < nbu; i++)
    {
      /* get min/max of segment i in bu (X) */
      su = &bu[i*stride];
      cuminx = DBL_MAX;
      cumaxx = DBL_MIN;
      for(k = 0; k < cu->order; k++)
	{
	  if(*su < cuminx)
	    cuminx = *su;
	  if(*su > cumaxx)
	    cumaxx = *su;
	  su += stride;
	}
      /* get min/max of segment i in bu (Y) */
      su = &bu[i*stride+1];
      cuminy = DBL_MAX;
      cumaxy = DBL_MIN;
      for(k = 0; k < cu->order; k++)
	{
	  if(*su < cuminy)
	    cuminy = *su;
	  if(*su > cumaxy)
	    cumaxy = *su;
	  su += stride;
	}
      /* get min/max of segment i in bu (Z) */
      su = &bu[i*stride+2];
      cuminz = DBL_MAX;
      cumaxz = DBL_MIN;
      for(k = 0; k < cu->order; k++)
	{
	  if(*su < cuminz)
	    cuminz = *su;
	  if(*su > cumaxz)
	    cumaxz = *su;
	  su += stride;
	}

      /* now check against all segments in nbv */
      for(j = 0; j < nbv; j++)
	{
	  overlap = AY_TRUE;
	  /* get min/max of segment j in bv (Z) */
	  sv = &bv[j*stride];
	  cvminx = DBL_MAX;
	  cvmaxx = DBL_MIN;
	  for(k = 0; k < cv->order; k++)
	    {
	      if(*sv < cvminx)
		cvminx = *su;
	      if(*sv > cvmaxx)
		cvmaxx = *sv;
	      sv += stride;
	    }
	  if(!((cumaxx < cvminx)||(cvmaxx < cuminx)))
	    {
	      /* overlap in X => need to check Y */
	      sv = &bv[j*stride+1];
	      cvminy = DBL_MAX;
	      cvmaxy = DBL_MIN;
	      for(k = 0; k < cv->order; k++)
		{
		  if(*sv < cvminy)
		    cvminy = *su;
		  if(*sv > cvmaxy)
		    cvmaxy = *sv;
		  sv += stride;
		}
	      if(!((cumaxy < cvminy)||(cvmaxy < cuminy)))
		{
		  /* overlap in Y => need to check Z */
		  sv = &bv[j*stride+2];
		  cvminz = DBL_MAX;
		  cvmaxz = DBL_MIN;
		  for(k = 0; k < cv->order; k++)
		    {
		      if(*sv < cvminz)
			cvminz = *su;
		      if(*sv > cvmaxz)
			cvmaxz = *sv;
		      sv += stride;
		    }
		  if((cumaxz < cvminz)||(cvmaxz < cuminz))
		    {
		      /* no overlap in Z */
		      overlap = AY_FALSE;
		    }
		}
	      else
		{
		  overlap = AY_FALSE;
		}
	    }
	  else
	    {
	      overlap = AY_FALSE;
	    }

	  if(overlap)
	    {
	      /* segments bu[i] and bv[j] overlap at least in one
		 dimension => calculate smallest distance */


	    }

	} /* for */
    } /* for */


  /* first, calc middle point of control hull */
  su = &(bu[i*stride]);
  for(i = 0; i < nbu; i++)
    {
      k = 0;
      for(j = 0; j < cu->order; j++)
	{


	}
      k+=stride;
    } /* for */

  sv = &(bv[j*stride]);

  /* compare x component */

#endif
 return ay_status;
} /* ay_nct_intersect */


/** ay_nct_intersectsets:
 * Calculate the intersections of two sets of ordered intersecting
 * and isoparametric curves.
 * This is needed for the Gordon surface creation.
 *
 * XXXX Todo: this could be made more robust by also computing
 * intersection points from the u curves and then calculating
 * the mean values?
 *
 * \param[in] ncu  number of curves in \a cu
 * \param[in] cu  a list of NURBS curves (u direction)
 * \param[in] ncv  number of curves in \a cv
 * \param[in] cv  a list of NURBS curves (v direction)
 * \param[in,out] intersections  where to store the intersection points,
 * allocated outside, of size ncu*ncv*4
 *
 * \returns AY_OK on success, error code else
 */
int
ay_nct_intersectsets(int ncu, ay_object *cu, int ncv, ay_object *cv,
		     double *intersections)
{
 int ay_status = AY_OK;
 int stride = 4, sample_start;
 double *us = NULL, *vs = NULL, u, v, pnt[3];
 ay_object *cuo, *cvo;
 ay_nurbcurve_object *nc;
 double *a;
 int i, j;

  if(!(us = calloc(ncv, sizeof(double))))
    { ay_status = AY_EOMEM; goto cleanup; }

  if(!(vs = calloc(ncu, sizeof(double))))
    { ay_status = AY_EOMEM; goto cleanup; }

  cuo = cu;
  cvo = cv;

  /* fill vs */
  nc = (ay_nurbcurve_object *)cu->refine;
  vs[0] = nc->knotv[nc->order-1];
  vs[ncu-1] = nc->knotv[nc->length];
  cuo = cuo->next;

  sample_start = AY_TRUE;
  if(ay_nct_isdegen((ay_nurbcurve_object*)cvo->refine))
    {
      sample_start = AY_FALSE;
      while(cvo->next)
	cvo = cvo->next;
    }

  /*
  if(ay_nct_isdegen(cvo))
    return AY_ERROR;
  */

  for(i = 1; i < ncu-1; i++)
    {
      nc = (ay_nurbcurve_object *)cuo->refine;
      if(sample_start)
	u = nc->knotv[nc->order-1];
      else
	u = nc->knotv[nc->length];
      ay_status = ay_nb_CurvePoint4D(nc->length-1, nc->order-1, nc->knotv,
				     nc->controlv, u,
				     pnt);
      ay_status = ay_nct_findu3d(cvo, pnt, &v);
      /*
      printf("found v: %lg for xyz: %lg, %lg, %lg\n",v,pnt[0],pnt[1],pnt[2]);
      */
      vs[i] = v;
      cuo = cuo->next;
    }

#if 0
  nc = (ay_nurbcurve_object *)cv->refine;
  us[0] = nc->knotv[nc->order-1];
  us[ncv-1] = nc->knotv[nc->length];
  cvo = cvo->next;
  for(i = 0; i < ncv-1; i++)
    {
      nc = (ay_nurbcurve_object *)cvo->refine;
      ay_status = ay_nb_CurvePoint4D(nc->length-1, nc->order-1, nc->knotv,
				     nc->controlv, nc->knotv[nc->order-1],
				     pnt);
      ay_status = ay_nct_findu3d(cv, pnt, &u);
      us[i] = u;
      cvo = cvo->next;
    }
#endif

  /* compute intersections */
  a = intersections;
  cvo = cv;
  for(i = 0; i < ncv; i++)
    {
      nc = (ay_nurbcurve_object *)cvo->refine;
      for(j = 0; j < ncu; j++)
	{
	  ay_status = ay_nb_CurvePoint4D(nc->length-1, nc->order-1, nc->knotv,
					 nc->controlv, vs[j], a);
	  a[3] = 1.0;
	  /*
	  printf("(%d,%d)=(%lg %lg %lg)\n",i,j,a[0],a[1],a[2]);
	  */
	  a += stride;
	}
      cvo = cvo->next;
    }

cleanup:

  if(us)
    free(us);

  if(vs)
    free(vs);

 return ay_status;
} /* ay_nct_intersectsets */


/** ay_nct_iscompatible:
 * Checks the curve objects for compatibility (whether or not they
 * are of the same order, length, and defined on the same knot vector).
 *
 * \param[in] curves  a number of NURBS curve objects
 * \param[in] level  determines which level of compatibility to check:
 *            0 - check only the order,
 *            1 - check order and length,
 *            2 - check order, length, and knot values
 * \param[in,out] result  is set to AY_TRUE if the curves are compatible,
 *  AY_FALSE else
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_iscompatible(ay_object *curves, int level, int *result)
{
 ay_object *o1, *o2;
 ay_nurbcurve_object *curve1 = NULL, *curve2 = NULL;

  if(!curves)
    return AY_ENULL;

  *result = AY_TRUE;

  o1 = curves;

  while(o1->next)
    {
      o2 = o1->next;
      curve1 = (ay_nurbcurve_object *) o1->refine;
      curve2 = (ay_nurbcurve_object *) o2->refine;

      if(curve1->order != curve2->order)
	{
	  *result = AY_FALSE;
	  return AY_OK;
	}

      if(level > 0)
	{
	  if(curve1->length != curve2->length)
	    {
	      *result = AY_FALSE;
	      return AY_OK;
	    }
	  if(level > 1)
	    {
	      /* XXXX TODO: replace with AY_EPSILON based check? */
	      if(memcmp(curve1->knotv, curve2->knotv,
			(curve1->length+curve1->order)*sizeof(double)))
		{
		  *result = AY_FALSE;
		  return AY_OK;
		}
	    }
	}

      o1 = o1->next;
    } /* while */

 return AY_OK;
} /* ay_nct_iscompatible */


/** ay_nct_makecompatible:
 *  make a number of curves compatible i.e. of the same order
 *  and defined on the same knot vector
 *
 * \param[in,out] curves  a number of NURBS curve objects
 * \param[in] level  desired level of compatibility:
 *  0 - order,
 *  1 - length,
 *  2 - length/order/knots
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_makecompatible(ay_object *curves, int level)
{
 int ay_status = AY_OK;
 ay_object *o;
 ay_nurbcurve_object *curve = NULL;
 int max_order = 0, max_length = 0;
 int stride, nh = 0, numknots = 0, t = 0, i, j, a, b;
 int Ualen = 0, Ublen = 0, Ubarlen = 0, clamped;
 double *Uh = NULL, *Qw = NULL, *realUh = NULL, *realQw = NULL;
 double *Ubar = NULL, *Ua = NULL, *Ub = NULL;
 double u = 0.0, ud, *u1, *u2;

  if(!curves)
    return AY_ENULL;

  /* clamp curves */
  o = curves;
  while(o)
    {
      curve = (ay_nurbcurve_object *) o->refine;
      clamped = AY_FALSE;
      if((curve->knot_type == AY_KTBSPLINE) ||
	 ((curve->type == AY_CTPERIODIC) &&
	  ((curve->knot_type == AY_KTCHORDAL) ||
	   (curve->knot_type == AY_KTCENTRI))))
	{
	  clamped = AY_TRUE;
	  ay_status = ay_nct_clampperiodic(curve);
	}
      else
	{
	  if(curve->knot_type == AY_KTCUSTOM)
	    {
	      a = 1;
	      u = curve->knotv[0];
	      for(i = 1; i < curve->order; i++)
		if(fabs(u - curve->knotv[i]) < AY_EPSILON)
		  a++;

	      j = curve->length+curve->order-1;
	      b = 1;
	      u = curve->knotv[j];
	      for(i = j-1; i >= curve->length; i--)
		if(fabs(u - curve->knotv[i]) < AY_EPSILON)
		  b++;

	      if((a < (curve->order)) || (b < (curve->order)))
		{
		  clamped = AY_TRUE;
		  ay_status = ay_nct_clamp(curve, 0);
		}
	    } /* if */
	} /* if */

      if(ay_status)
	return ay_status;

      if(clamped)
	{
	  curve->knot_type = ay_knots_classify(curve->order, curve->knotv,
					       curve->order+curve->length,
					       AY_EPSILON);
	}

      o = o->next;
    } /* while */

  /* rescale knots to range 0.0 - 1.0 */
  o = curves;
  while(o)
    {
      curve = (ay_nurbcurve_object *) o->refine;
      if((curve->knotv[0] != 0.0) ||
	 (curve->knotv[curve->length+curve->order-1] != 1.0))
	{
	  ay_status = ay_knots_rescaletorange(curve->length+curve->order,
					      curve->knotv, 0.0, 1.0);
	}
      o = o->next;
    }

  /* find max order */
  o = curves;
  while(o)
    {
      curve = (ay_nurbcurve_object *) o->refine;
      if(curve->order > max_order)
	max_order = curve->order;

      o = o->next;
    }

  /* degree elevate */
  o = curves;
  while(o)
    {
      curve = (ay_nurbcurve_object *) o->refine;
      if(curve->order < max_order)
	{
	  stride = 4;
	  t = max_order - curve->order;

	  /* alloc new knotv & new controlv */
	  if(!(Uh = malloc((curve->length + curve->length * t +
			    curve->order + t) *
			   sizeof(double))))
	    {
	      return AY_EOMEM;
	    }
	  if(!(Qw = malloc((curve->length + curve->length*t)*4*
			   sizeof(double))))
	    {
	      free(Uh);
	      return AY_EOMEM;
	    }

	  ay_status = ay_nb_DegreeElevateCurve4D(stride, curve->length-1,
			         curve->order-1, curve->knotv, curve->controlv,
				 t, &nh, Uh, Qw);

	  if(ay_status)
	    {
	      free(Uh); free(Qw); return ay_status;
	    }

	  if(!(realQw = realloc(Qw, nh*4*sizeof(double))))
	    {
	      free(Uh); free(Qw); return AY_EOMEM;
	    }

	  if(!(realUh = realloc(Uh, (nh+curve->order+t)*sizeof(double))))
	    {
	      free(Uh); free(realQw); return AY_EOMEM;
	    }

	  free(curve->knotv);
	  curve->knotv = realUh;

	  free(curve->controlv);
	  curve->controlv = realQw;

	  curve->knot_type = AY_KTCUSTOM;

	  curve->order += t;

	  curve->length = nh;

	  numknots += (curve->order + curve->length);

	  Qw = NULL;
	  Uh = NULL;
	  realQw = NULL;
	  realUh = NULL;
	} /* if */
      o = o->next;
    } /* while */

  if(level < 1)
    goto cleanup;

  if(level < 2)
    {
      /* find max length */
      o = curves;
      while(o)
	{
	  curve = (ay_nurbcurve_object *) o->refine;
	  if(curve->length > max_length)
	    max_length = curve->length;

	  o = o->next;
	}

      o = curves;
      while(o)
	{
	  curve = (ay_nurbcurve_object *) o->refine;

	  if(curve->length < max_length)
	    {
	      if(!(Ub = malloc((curve->length + curve->order +
				(max_length - curve->length))*sizeof(double))))
		{
		  ay_status = AY_EOMEM;
		  goto cleanup;
		}

	      memcpy(Ub, curve->knotv, (curve->length + curve->order) *
		     sizeof(double));

	      Ubarlen = max_length - curve->length;
	      if(!(Ubar = malloc(Ubarlen*sizeof(double))))
		{
		  free(Ub);
		  ay_status = AY_EOMEM;
		  goto cleanup;
		}

	      for(i = 0; i < Ubarlen; i++)
		{
		  ud = 0;
		  u1 = &Ub[curve->order-1];
		  u2 = &Ub[curve->length+i];
		  for(j = curve->order-1; j < curve->length-1+i; j++)
		    {
		      if(fabs(Ub[j]-Ub[j+1]) > AY_EPSILON)
			{
			  if(fabs(Ub[j]-Ub[j+1]) > ud)
			    {
			      ud = fabs(Ub[j] - Ub[j+1]);
			      u1 = &(Ub[j]);
			      u2 = &(Ub[j+1]);
			    }
			}
		    }
		  Ubar[i] = *u1 + (*u2 - *u1) / 2.0;
		  memmove(u2+1, u2, (Ubarlen-i)*sizeof(double));
		  *u2 = Ubar[i];
		}

	      ay_status = ay_nct_refinekn(curve, /*maintain_ends=*/AY_TRUE,
					  Ubar, Ubarlen);

	      free(Ub);
	      Ub = NULL;
	      free(Ubar);
	      Ubar = NULL;

	      if(ay_status)
		goto cleanup;
	    } /* if */

	  o = o->next;
	} /* while */

      goto cleanup;
    }

  /* unify knots */
  o = curves;
  curve = (ay_nurbcurve_object *) o->refine;
  Ua = curve->knotv;
  Ualen = curve->length+curve->order;

  o = o->next;
  while(o)
    {
      curve = (ay_nurbcurve_object *)o->refine;
      Ub = curve->knotv;
      Ublen = curve->length+curve->order;

      ay_status = ay_knots_unify(Ua, Ualen, Ub, Ublen, &Ubar, &Ubarlen);

      if(ay_status)
	{
	  goto cleanup;
	}

      Ua = Ubar;
      Ualen = Ubarlen;

      o = o->next;
    } /* while */

  /* merge knots */
  o = curves;
  while(o)
    {
      curve = (ay_nurbcurve_object *) o->refine;

      ay_status = ay_knots_mergenc(curve, Ubar, Ubarlen);
      if(ay_status)
	{
	  goto cleanup;
	}

      ay_nct_recreatemp(curve);

      o = o->next;
    } /* while */

cleanup:

  if(Ubar)
    free(Ubar);

 return ay_status;
} /* ay_nct_makecompatible */


/** ay_nct_shiftarr:
 *  shift the control points of a 1D (curve) control vector
 *
 * \param[in] dir  direction of the shift (0: left, 1: right)
 * \param[in] stride  size of a point
 * \param[in] cvlen  number of points in control vector \a cv
 * \param[in,out] cv  control vector to shift
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_shiftarr(int dir, int stride, int cvlen, double *cv)
{
 int ay_status = AY_OK;
 int a, i;
 double t4[4];
 double *t = NULL;

  if(!cv)
    return AY_ENULL;

  if(stride > 4)
    {
      if(!(t = malloc(stride*sizeof(double))))
	return AY_EOMEM;
    }
  else
    {
      t = t4;
    }

  if(dir)
    {
      memcpy(t, cv, stride * sizeof(double));
      a = 0;
      for(i = 0; i < cvlen-1; i++)
	{
	  memcpy(&(cv[a]), &(cv[a+stride]), stride * sizeof(double));
	  a += stride;
	}
      /* saved first becomes new last */
      memcpy(&(cv[(cvlen-1)*stride]), t, stride * sizeof(double));
    }
  else
    {
      memcpy(t, &(cv[(cvlen-1)*stride]), stride * sizeof(double));
      a = (cvlen-2)*stride;
      for(i = 0; i < cvlen-1; i++)
	{
	  memcpy(&(cv[a+stride]), &(cv[a]), stride * sizeof(double));
	  a -= stride;
	}
      /* saved last becomes new first */
      memcpy(cv, t, stride * sizeof(double));
    }

  if(stride > 4)
    free(t);

 return ay_status;
} /* ay_nct_shiftarr */


/** ay_nct_shiftctcmd:
 *  Shift control points of selected curves.
 *  Implements the \a shiftC scripting interface command.
 *  See also the corresponding section in the \ayd{scshiftc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_shiftctcmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_nurbcurve_object *ncurve = NULL;
 ay_icurve_object *icurve = NULL;
 ay_acurve_object *acurve = NULL;
 ay_object *src = NULL;
 double *cv;
 int i, times = 1;
 int cvlen, stride;
 int notify_parent = AY_FALSE;

  if(argc > 1)
    {
      tcl_status = Tcl_GetInt(interp, argv[1], &times);
      AY_CHTCLERRRET(tcl_status, argv[0], interp);
    }

  if(times == 0)
    {
      ay_error(AY_ERROR, argv[0], "Parameter must be different from 0.");
      return TCL_OK;
    }

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  while(sel)
    {
      src = sel->object;
      cvlen = 0;
      switch(src->type)
	{
	case AY_IDNCURVE:
	  ncurve = (ay_nurbcurve_object*)src->refine;
	  stride = 4;
	  cv = ncurve->controlv;
	  cvlen = ncurve->length;
	  break;
	case AY_IDICURVE:
	  icurve = (ay_icurve_object*)src->refine;
	  stride = 3;
	  cv = icurve->controlv;
	  cvlen = icurve->length;
	  break;
	case AY_IDACURVE:
	  acurve = (ay_acurve_object*)src->refine;
	  stride = 3;
	  cv = acurve->controlv;
	  cvlen = acurve->length;
	  break;

	default:
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	  break;
	}

      if(cvlen)
	{
	  for(i = 0; i < abs(times); i++)
	    {
	      if(times > 0)
		ay_status = ay_nct_shiftarr(1, stride, cvlen, cv);
	      else
		ay_status = ay_nct_shiftarr(0, stride, cvlen, cv);
	      if(ay_status)
		{
		  ay_error(ay_status, argv[0], "Could not shift curve.");
		  break;
		}

	      /* fix multiple points at the ends
		 of closed/periodic NURBS curves */
	      if((src->type == AY_IDNCURVE) && (ncurve->type != AY_CTOPEN))
		{
		  if(times > 0)
		    {
		      ay_nct_close(ncurve);
		    }
		  else
		    {
		      if(ncurve->type == AY_CTPERIODIC)
			memcpy(cv,
			       &(cv[(ncurve->length-ncurve->order+1)*stride]),
			       (ncurve->order-1)*sizeof(double));
		      else
			memcpy(cv,
			       &(cv[(ncurve->length-1)*stride]),
			       stride*sizeof(double));
		    }
		} /* if */
	    } /* for */

	  if(ay_status)
	    break;

	  src->modified = AY_TRUE;

	  (void)ay_notify_object(src);
	  notify_parent = AY_TRUE;
	} /* if */

      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_shiftctcmd */


int
ay_nct_getplane(int cvlen, int cvstride, double *cv)
{
#if 0
 int plane = AY_XY;

  if(cvlen < 3 || cvstride <= 0 || !cv)
    return -1;


  /* try to get three "good" points,
     they should not be equal and not be colinear
     (i.e. span a triangle to get the orientation from */
  tp1 = nc->controlv;
  tp2 = &(nc->controlv[(cvlen/2)*stride]);

  while(AY_V3COMP(tp1, tp2))
    {

    }


 return plane;
#endif
 return 0;
} /* ay_nct_getplane */


/** ay_nct_toplane:
 *  modify the planar curve \a c, so that it is defined in the
 *  XY, YZ, or XZ plane
 *  by detecting the current orientation, adding the relevant rotation
 *  information to the transformation attributes and rotating the control
 *  points of the planar curve to the desired plane
 *
 * \param[in] plane  target plane (AY_XY, AY_YZ, AY_XZ)
 * \param[in] allow_flip  if AY_TRUE also curves in the target plane with
 *  non clockwise winding will be changed (flipped)
 * \param[in,out] c  curve object to process
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_toplane(int plane, int allow_flip, ay_object *c)
{
 int ay_status = AY_OK;
 ay_acurve_object *ac = NULL;
 ay_icurve_object *ic = NULL;
 ay_nurbcurve_object *nc = NULL;
 double *p, *tp1, *tp2, *tp3, V1[3], V2[3], A[3], B[3];
 double X[3] = {1,0,0}, Y[3] = {0,1,0}, Z[3] = {0,0,1};
 double angle, len, m[16], quat[4], euler[3];
 double *cv = NULL;
 int cvlen = 0;
 int i, stride = 4, have_good_points = AY_FALSE, num_tries = 0;

  if(!c)
    return AY_ENULL;

  switch(c->type)
    {
    case AY_IDNCURVE:
      nc = (ay_nurbcurve_object *)c->refine;
      cv = nc->controlv;
      cvlen = nc->length;
      stride = 4;
      break;
    case AY_IDICURVE:
      ic = (ay_icurve_object *)c->refine;
      cv = ic->controlv;
      cvlen = ic->length;
      stride = 3;
      break;
    case AY_IDACURVE:
      ac = (ay_acurve_object *)c->refine;
      cv = ac->controlv;
      cvlen = ac->length;
      stride = 3;
      break;
    default:
      return AY_EWTYPE;
    }

  if(cvlen < 3)
    return AY_ERROR;

  /* try to get three "good" points,
     they should not be equal and not be collinear
     (i.e. span a triangle to get the orientation from */
  tp1 = cv;
  tp2 = &(cv[(cvlen/2)*stride]);
  tp3 = &(cv[(cvlen-2)*stride]);

  /* check, whether we, probably, operate on a closed B-Spline curve,
     if yes, we choose a different set of initial points */
  if(nc)
    {
      p = &(cv[(nc->length-(nc->order-1))*stride]);
      if(AY_V3COMP(tp1, p))
	{
	  tp1 = &(cv[(nc->order/2)*stride]);
	  tp3 = &(cv[(nc->length-(nc->order/2))*stride]);
	}
    }

  /* check and correct the points */
  while(!have_good_points && (num_tries < (cvlen/2)))
    {
      have_good_points = AY_TRUE;
      if(AY_V3COMP(tp1, tp2))
	{
	  tp2 += stride;
	  have_good_points = AY_FALSE;
	}
      if(AY_V3COMP(tp2, tp3))
	{
	  tp3 -= stride;
	  have_good_points = AY_FALSE;
	}
      if(AY_V3COMP(tp1, tp3))
	{
	  tp3 -= stride;
	  have_good_points = AY_FALSE;
	}

      if(have_good_points)
	{
	  AY_V3SUB(V1, tp2, tp1);
	  len = AY_V3LEN(V1);
	  AY_V3SCAL(V1,(1.0/len));

	  AY_V3SUB(V2, tp3, tp1);
	  len = AY_V3LEN(V2);
	  AY_V3SCAL(V2, (1.0/len));

	  angle = AY_V3DOT(V1,V2);
	  if(angle <= -1.0)
	    angle = -180.0;
	  else
	    if(angle >= 1.0)
	      angle = 0.0;
	    else
	      angle = AY_R2D(acos(angle));

	  if(angle < AY_EPSILON)
	    {
	      /* V1 and V2 are parallel */
	      have_good_points = AY_FALSE;
	      tp2 += stride;
	    }
	} /* if have_good_points */

      num_tries++;
    } /* while */

  if(!have_good_points)
    return AY_ERROR;

  /* now we may calculate the orientation of the curve */
  AY_V3CROSS(A, V1, V2);
  len = AY_V3LEN(A);
  AY_V3SCAL(A, (1.0/len));

  switch(plane)
    {
    case AY_XY:
      /* A is now perpendicular to the plane in which the curve
	 is defined thus, we calculate angle and rotation axis (B)
	 between A and Z (0,0,1) */
      angle = AY_V3DOT(A, Z);
      if(angle <= -1.0)
	angle = -180.0;
      else
	if(angle >= 1.0)
	  angle = 0.0;
	else
	  angle = AY_R2D(acos(angle));
      if((fabs(angle) < AY_EPSILON))
	{
	  /* Nothing to do, as curve ist properly aligned with
	     XY plane already...*/
	  return AY_OK;
	}
      AY_V3CROSS(B, A, Z);
      break;
    case AY_XZ:
      /* A is now perpendicular to the plane in which the curve
	 is defined thus, we calculate angle and rotation axis (B)
	 between A and Y (0,1,0) */
      angle = AY_V3DOT(A, Y);
      if(angle <= -1.0)
	angle = -180.0;
      else
	if(angle >= 1.0)
	  angle = 0.0;
	else
	  angle = AY_R2D(acos(angle));
      if((fabs(angle) < AY_EPSILON))
	{
	  /* Nothing to do, as curve ist properly aligned with
	     XZ plane already...*/
	  return AY_OK;
	}
      AY_V3CROSS(B, A, Y);
      break;
    case AY_YZ:
      /* A is now perpendicular to the plane in which the curve
	 is defined thus, we calculate angle and rotation axis (B)
	 between A and X (1,0,0) */
      angle = AY_V3DOT(A, X);
      if(angle <= -1.0)
	angle = -180.0;
      else
	if(angle >= 1.0)
	  angle = 0.0;
	else
	  angle = AY_R2D(acos(angle));
      if((fabs(angle) < AY_EPSILON))
	{
	  /* Nothing to do, as curve ist properly aligned with
	     YZ plane already...*/
	  return AY_OK;
	}
      AY_V3CROSS(B, A, X);
      break;
    default:
      break;
    } /* switch plane */

  if(fabs(fabs(angle) - 180.0) < AY_EPSILON)
    {
      if(allow_flip)
	{
	  switch(c->type)
	    {
	    case AY_IDNCURVE:
	      ay_nct_revert(nc);
	      break;
	    case AY_IDICURVE:
	      ay_ict_revert(ic);
	      break;
	    case AY_IDACURVE:
	      ay_act_revert(ac);
	      break;
	    default:
	      return AY_EWTYPE;
	    } /* switch type */
	}
      return AY_OK;
    }

  len = AY_V3LEN(B);
  AY_V3SCAL(B, (1.0/len));

  /* calculate rotation matrix */
  ay_trafo_identitymatrix(m);

  /*ay_trafo_translatematrix(-c->movx, -c->movy, -c->movz, m);*/
  ay_trafo_rotatematrix(angle, B[0], B[1], B[2], m);
  /*ay_trafo_translatematrix(c->movx, c->movy, c->movz, m);*/
  ay_trafo_scalematrix(c->scalx, c->scaly, c->scalz, m);

  /* apply rotation matrix to all points */
  p = cv;
  for(i = 0; i < cvlen; i++)
    {
      ay_trafo_apply3(p, m);
      p += stride;
    } /* for */

  /* calculate new transformation attributes */
  ay_quat_axistoquat(B, AY_D2R(angle), quat);
  ay_quat_add(c->quat, quat, c->quat);
  ay_quat_toeuler(c->quat, euler);
  c->rotx = AY_R2D(euler[2]);
  c->roty = AY_R2D(euler[1]);
  c->rotz = AY_R2D(euler[0]);

  c->scalx = 1.0;
  c->scaly = 1.0;
  c->scalz = 1.0;

 return ay_status;
} /* ay_nct_toplane */


/** ay_nct_toxytcmd:
 *  Rotate selected NURBS curves to XY, YZ, or XZ plane.
 *  Implements the \a toXYC scripting interface command.
 *  Implements the \a toYZC scripting interface command.
 *  Implements the \a toXZC scripting interface command.
 *  See also the corresponding section in the \ayd{sctoxync}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_toxytcmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
{
 int ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL;
 int notify_parent = AY_FALSE;
 int plane = AY_XY;

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  /* distinguish between "toXYC", "toYZC", and "toXZC" */
  if(argv[0][2] == 'Y')
    plane = AY_YZ;
  else
    if(argv[0][3] == 'Z')
      plane = AY_XZ;

  while(sel)
    {
      o = sel->object;
      if((o->type != AY_IDNCURVE) && (o->type != AY_IDACURVE) &&
	 (o->type != AY_IDICURVE))
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	}
      else
	{
	  ay_status = ay_nct_toplane(plane, /*allow_flip=*/AY_FALSE, o);
	  if(ay_status)
	    {
	      ay_error(ay_status, argv[0], "Could not align object to plane.");
	    }
	  else
	    {
	      /* re-create tesselation of curve */
	      (void)ay_notify_object(o);
	      o->modified = AY_TRUE;
	      notify_parent = AY_TRUE;
	    } /* if */
	} /* if */

      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_toxytcmd */


/** ay_nct_iscomptcmd:
 *  Check selected NURBS curves for compatibility.
 *  Implements the \a isCompNC scripting interface command.
 *  See also the corresponding section in the \ayd{sciscompnc}.
 *
 *  \returns 1 if the selected curves are compatible.
 */
int
ay_nct_iscomptcmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[])
{
 int ay_status = AY_OK, tcl_status = TCL_OK;
 ay_object *o = NULL, *curves = NULL;
 ay_list_object *sel = NULL;
 int comp = AY_FALSE, i = 0, level = 2;

  if(!ay_selection)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  if(argc > 1)
    {
      if((argv[1][0] == '-') && (argv[1][1] == 'l'))
	{
	  tcl_status = Tcl_GetInt(interp, argv[2], &level);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	}
    }

  sel = ay_selection;
  while(sel)
    {
      o = sel->object;
      if(o->type == AY_IDNCURVE)
	i++;
      sel = sel->next;
    }

  if(i > 1)
    {
      curves = malloc(i*sizeof(ay_object));
      if(!curves)
	{
	  ay_error(AY_EOMEM, argv[0], NULL);
	  return TCL_OK;
	}
      i = 0;
      sel = ay_selection;
      while(sel)
	{
	  o = sel->object;
	  if(o->type == AY_IDNCURVE)
	    {
	      (curves[i]).refine = o->refine;
	      (curves[i]).next = &(curves[i+1]);
	      i++;
	    }
	  sel = sel->next;
	}
      (curves[i-1]).next = NULL;

      ay_status = ay_nct_iscompatible(curves, level, &comp);
      if(ay_status)
	{
	  ay_error(ay_status, argv[0], "Could not check the curves.");
	}
      else
	{
	  if(comp)
	    {
	      Tcl_SetResult(interp, "1", TCL_VOLATILE);
	    }
	  else
	    {
	      Tcl_SetResult(interp, "0", TCL_VOLATILE);
	    }
	}

      free(curves);
    }
  else
    {
      ay_error(AY_ERROR, argv[0], "Not enough curves selected.");
    } /* if */

 return TCL_OK;
} /* ay_nct_iscomptcmd */


/** ay_nct_makecomptcmd:
 *  Make selected NURBS curves compatible.
 *  Implements the \a makeCompNC scripting interface command.
 *  See also the corresponding section in the \ayd{scmakecompnc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_makecomptcmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 int i = 1, is_comp = AY_FALSE, force = AY_FALSE, level = 2;
 ay_list_object *sel = ay_selection;
 ay_nurbcurve_object *nc = NULL;
 ay_object *o = NULL, *p = NULL, *src = NULL, **nxt = NULL;

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  while(i < argc)
    {
      if((argv[i][0] == '-') && (argv[i][1] == 'f'))
	{
	  force = AY_TRUE;
	}
      else
	if((argv[i][0] == '-') && (argv[i][1] == 'l'))
	  {
	    tcl_status = Tcl_GetInt(interp, argv[i+1], &level);
	    AY_CHTCLERRRET(tcl_status, argv[0], interp);
	    i++;
	  }
      i++;
    }

  /* make copies of all curves */
  nxt = &(src);
  while(sel)
    {
      o = sel->object;
      if(o->type != AY_IDNCURVE)
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	}
      else
	{
	  ay_status = ay_object_copy(o, nxt);
	  if(ay_status)
	    {
	      ay_error(AY_ERROR, argv[0], "Could not copy object.");
	      goto cleanup;
	    }
	  nxt = &((*nxt)->next);
	} /* if */

      sel = sel->next;
    } /* while */

  if(!src || !src->next)
    {
      ay_error(AY_ERROR, argv[0], "Please select at least two NURBS curves.");
      goto cleanup;
    } /* if */

  if(!force)
    {
      ay_status = ay_nct_iscompatible(src, level, &is_comp);
      if(ay_status)
	{
	  ay_error(ay_status, argv[0],
		   "Could not check the curves, assuming incompatibility.");
	  is_comp = AY_FALSE;
	}
      if(is_comp)
	goto cleanup;
    } /* if */

  /* try to make the copies compatible */
  ay_status = ay_nct_makecompatible(src, level);
  if(ay_status)
    {
      ay_error(AY_ERROR, argv[0],
	       "Failed to make selected curves compatible.");
      goto cleanup;
    }

  /* now exchange the nurbcurve objects */
  p = src;
  sel = ay_selection;
  while(sel)
    {
      o = sel->object;
      if(o->type == AY_IDNCURVE)
	{
	  nc = (ay_nurbcurve_object*)o->refine;
	  if(!p)
	    goto cleanup;
	  o->refine = p->refine;
	  p->refine = nc;
	  /* update pointers to controlv;
	     re-create tesselation of the curve */
	  if(o->selp)
	    ay_selp_clear(o);

	  o->modified = AY_TRUE;
	  (void)ay_notify_object(o);

	  p = p->next;
	} /* if */
      sel = sel->next;
    } /* while */

  (void)ay_notify_parent();

cleanup:

  if(src)
    (void)ay_object_deletemulti(src, AY_FALSE);

 return TCL_OK;
} /* ay_nct_makecomptcmd */


/** ay_nct_israt:
 *  Check whether any control point of a NURBS curve
 *  uses a weight value (!= 1.0).
 *
 * \param[in] curve  NURBS curve to check
 *
 * \returns AY_TRUE if curve is rational, AY_FALSE else.
 */
int
ay_nct_israt(ay_nurbcurve_object *curve)
{
 double *p;
 int i;

  if(!curve)
    return AY_FALSE;

  p = &(curve->controlv[3]);
  for(i = 0; i < curve->length; i++)
    {
      if((fabs(*p) < (1.0-AY_EPSILON)) || (fabs(*p) > (1.0+AY_EPSILON)))
	return AY_TRUE;
      p += 4;
    } /* for */

 return AY_FALSE;
} /* ay_nct_israt */


/** ay_nct_coarsen:
 *  Reduces the resolution of a NURBS curve.
 *  Periodic and closed curves will be handled properly.
 *
 *  If a selected region is defined, it should be cleaned via
 *  ay_selp_reducetominmax() beforehand, because this function
 *  will set the index of the second selected point to the new
 *  matching end index of the region.
 *
 * \param[in,out] curve  NURBS curve object to be processed
 * \param[in,out] selp  region of the curve to process, may be NULL
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_coarsen(ay_nurbcurve_object *curve, ay_point *selp)
{
 int ay_status = AY_OK;
 char fname[] = "nct_coarsen";
 ay_point pmin = {0}, pmax = {0}, *tselp = NULL;
 double *newcontrolv = NULL, *newknotv = NULL;
 int newlength = 0, p, t, smin, smax;

  if(!curve)
    return AY_ENULL;

  if(curve->type == AY_CTPERIODIC)
    {
      if(curve->length <= ((curve->order-1)*2))
	return AY_OK;
    }
  else
    {
      if(curve->length <= curve->order)
	return AY_OK;
    }

  smin = curve->length;
  smax = 0;

  tselp = selp;
  while(tselp)
    {
      if(smin > (int)tselp->index)
	smin = tselp->index;

      if(smax < (int)tselp->index)
	smax = tselp->index;

      tselp = tselp->next;
    }

  if(smin > smax)
    {
      t = smin;
      smin = smax;
      smax = t;
    }

  if(curve->type == AY_CTPERIODIC)
    {
      /* special case: curves marked periodic;
       * we keep the p multiple points at the ends
       * and remove points just from the other sections
       */
      p = curve->order-1;

      if(smin < (p-1))
	smin = p-1;
      if(smax > curve->length - p)
	smax = curve->length - p;
    }

  if((smax - smin) < 2)
    {
      /* ay_error(AY_EWARN, fname, "No range."); */
      return AY_OK;
    }

  pmin.index = smin;
  pmin.next = &(pmax);
  pmax.index = smax;

  ay_status = ay_nct_coarsenarray(curve->controlv, curve->length, 4,
				  &pmin, &newcontrolv, &newlength);

  if(ay_status || !newcontrolv || newlength == 0)
    {
      if(newcontrolv)
	free(newcontrolv);
      ay_error(AY_ERROR, fname, "Coarsen failed.");
      return ay_status;
    }

  if(selp && selp->next)
    {
      selp->next->index = pmax.index;
    }

  /* coarsen (custom-) knots */
  if(curve->knot_type == AY_KTCUSTOM)
    {
      ay_status = ay_knots_coarsen(curve->order,
				   curve->length+curve->order,
				   curve->knotv, curve->length-newlength,
				   &newknotv);
      if(ay_status)
	{
	  free(newcontrolv);
	  ay_error(AY_ERROR, fname, "Could not coarsen knots.");
	  return ay_status;
	}
    }

  free(curve->controlv);
  curve->controlv = newcontrolv;
  curve->length = newlength;

  if(newknotv)
    {
      free(curve->knotv);
      curve->knotv = newknotv;
    }
  else
    {
      free(curve->knotv);
      curve->knotv = NULL;

      ay_status = ay_knots_createnc(curve);
      if(ay_status)
	ay_error(AY_ERROR, fname, "Could not create knots.");
    }

  if(curve->type == AY_CTCLOSED)
    {
      ay_status = ay_nct_close(curve);
      if(ay_status)
	ay_error(AY_ERROR, fname, "Could not close curve.");
    }

  ay_nct_recreatemp(curve);

 return ay_status;
} /* ay_nct_coarsen */


/** ay_nct_coarsenarray:
 *  Helper to coarsen an 1D array of control points.
 *
 * \param[in] Pw  array to coarsen
 * \param[in] len  number of elements in Pw
 * \param[in] stride  size of an element in Pw
 * \param[in,out] selp  region to be coarsened, may be NULL
 * \param[in,out] Qw  new coarsened array
 * \param[in,out] Qwlen  length of new array
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_coarsenarray(double *Pw, int len, int stride, ay_point *selp,
		    double **Qw, int *Qwlen)
{
 char fname[] = "coarsenarray";
 double *Q;
 int count;
 int i, j, start = 0, end = len;
 ay_point *endpnt = NULL;

  if(!Qw)
    return AY_ENULL;

  if(selp)
    {
      start = len;
      end = 0;
      while(selp)
	{
	  if(start > (int)selp->index)
	    start = selp->index;
	  if(end < (int)selp->index)
	    {
	      end = selp->index;
	      endpnt = selp;
	    }

	  selp = selp->next;
	}
    }

  if(end >= len)
    end = len-1;

  count = (end-start)/2;

  if(count > 0)
    {
      /* allocate new control vector */
      if(!(Q = malloc((len - count)*stride*sizeof(double))))
	{
	  ay_error(AY_EOMEM, fname, NULL);
	  return AY_ERROR;
	}

      /* copy first points */
      if(start > 0)
	memcpy(Q, Pw, start*stride*sizeof(double));

      /* copy old & create new points */
      i = start;
      j = start;
      while(i < end)
	{
	  memcpy(&(Q[j*stride]), &(Pw[i*stride]), stride*sizeof(double));

	  i += 2;
	  j++;
	} /* while */

      /* copy last point(s) */
      memcpy(&(Q[j*stride]), &(Pw[end*stride]),
	     (len-end)*stride*sizeof(double));

      if(endpnt)
	{
	  endpnt->index -= count;
	}

      /* return result */
      *Qw = Q;
      *Qwlen = len - count;
    } /* if count */

 return AY_OK;
} /* ay_nct_coarsenarray */


/** ay_nct_removesuperfluousknots:
 * Remove all knots from the curve that do not contribute to its shape.
 *
 * \param[in,out] nc  NURBS curve object to process
 * \param[in] tol  tolerance value (0.0-Inf, unchecked)
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_removesuperfluousknots(ay_nurbcurve_object *nc, double tol)
{
 int ay_status = AY_OK, was_rat = AY_FALSE, removed = AY_FALSE;
 char fname[] = "nct_removesuperfluousknots";
 int i, s;
 double *newcontrolv = NULL, *newknotv = NULL;

  do
    {
      removed = AY_FALSE;

      for(i = nc->order; i < nc->length; i++)
	{
	  /* calculate knot multiplicity */
	  s = 1;
	  while(((i+s) < (nc->length+nc->order)) &&
		(fabs(nc->knotv[i] - nc->knotv[i+s]) < AY_EPSILON))
	    {
	      s++;
	    }

	  /* we can (or should) not lower the nc order */
	  if((nc->length - 1) < nc->order)
	    {
	      continue;
	    }

	  if(!newcontrolv)
	    {
	      if(!(newcontrolv = malloc(nc->length*4*sizeof(double))))
		{
		  ay_error(AY_EOMEM, fname, NULL);
		  return AY_EOMEM;
		}
	    }
	  if(!newknotv)
	    {
	      if(!(newknotv = malloc((nc->length+nc->order)*sizeof(double))))
		{
		  free(newcontrolv);
		  ay_error(AY_EOMEM, fname, NULL);
		  return AY_EOMEM;
		}
	    }

	  if(nc->is_rat && !was_rat)
	    {
	      was_rat = AY_TRUE;
	      (void)ay_nct_euctohom(nc);
	    }
	  /*printf("Trying to remove knot %d with mult %d\n",i,s);*/
  	  /* try to remove the knot */
	  ay_status = ay_nb_RemoveKnotCurve4D(nc->length-1, nc->order-1,
					      nc->knotv, nc->controlv,
					      tol, i, s, 1,
					      newknotv, newcontrolv);

	  if(ay_status)
	    {
	      continue;
	    }
	  else
	    {
	      removed = AY_TRUE;
	      /* save results */
	      nc->length--;
	      memcpy(nc->controlv, newcontrolv, nc->length*4*sizeof(double));
	      memcpy(nc->knotv, newknotv,
		     (nc->length+nc->order)*sizeof(double));
	      free(newcontrolv);
	      newcontrolv = NULL;
	      free(newknotv);
	      newknotv = NULL;
	      break;
	    }
	} /* for */
    }
  while(removed);

  if(was_rat)
    (void)ay_nct_homtoeuc(nc);

  if(newcontrolv)
    free(newcontrolv);

  if(newknotv)
    free(newknotv);

 return AY_OK;
} /* ay_nct_removesuperfluousknots */


/** ay_nct_removekntcmd:
 *  Remove knot from selected NURBS curves.
 *  Implements the \a remknNC scripting interface command.
 *  Also implements the \a remsuknNC scripting interface command.
 *  See also the corresponding section in the \ayd{scremknnc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_removekntcmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 int have_index = AY_FALSE, i = 1, j = 0, s = 0, r = 0;
 int notify_parent = AY_FALSE;
 double tol = DBL_MAX;
 double u = 0.0, *newknotv = NULL, *newcontrolv = NULL;
 ay_nurbcurve_object *curve;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL;

  if(argv[0][3] == 's')
    {
      /* detected remsuknNC command */
      if(!sel)
	{
	  ay_error(AY_ENOSEL, argv[0], NULL);
	  return TCL_OK;
	}

      tol = 0.0;

      if(argc > 1)
	{
	  tcl_status = Tcl_GetDouble(interp, argv[1], &tol);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  if(tol != tol)
	    {
	      ay_error_reportnan(argv[0], "tol");
	      return TCL_OK;
	    }
	}

      while(sel)
	{
	  o = sel->object;
	  if(o->type != AY_IDNCURVE)
	    {
	      ay_error(AY_EWARN, argv[0], ay_error_igntype);
	    }
	  else
	    {
	      curve = (ay_nurbcurve_object *)o->refine;

	      ay_status = ay_nct_removesuperfluousknots(curve, tol);

	      if(ay_status)
		{
		  ay_error(AY_ERROR, argv[0], "Knot removal failed.");
		  break;
		}

	      notify_parent = AY_TRUE;

	      ay_nct_recreatemp(curve);

	      /* remove all selected points */
	      if(o->selp)
		ay_selp_clear(o);

	      o->modified = AY_TRUE;

	      /* re-create tesselation of curve */
	      (void)ay_notify_object(o);
	    } /* if */

	  sel = sel->next;
	} /* while */

      if(notify_parent)
	(void)ay_notify_parent();

      return TCL_OK;
    } /* if remsuknNC command */

  if((argc < 3) ||
     ((argv[1][0] == '-') && (argv[1][1] == 'i') && (argc < 4)))
    {
      ay_error(AY_EARGS, argv[0], "(u | -i ind) r [tol]");
      return TCL_OK;
    }

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  if((argv[1][0] == '-') && (argv[1][1] == 'i'))
    {
      tcl_status = Tcl_GetInt(interp, argv[2], &j);
      AY_CHTCLERRRET(tcl_status, argv[0], interp);
      if(j < 0)
	{
	  ay_error(AY_ERROR, argv[0], "Index must be > 0.");
	  return TCL_OK;
	}
      have_index = AY_TRUE;
      i++;
    }
  else
    {
      tcl_status = Tcl_GetDouble(interp, argv[1], &u);
      AY_CHTCLERRRET(tcl_status, argv[0], interp);
      if(u != u)
	{
	  ay_error_reportnan(argv[0], "u");
	  return TCL_OK;
	}
    }

  i++;
  tcl_status = Tcl_GetInt(interp, argv[i], &r);
  AY_CHTCLERRRET(tcl_status, argv[0], interp);
  i++;

  if(r <= 0)
    {
      ay_error(AY_ERROR, argv[0], "Parameter r must be > 0.");
      return TCL_OK;
    }

  if(argc > 3+have_index)
    {
      tcl_status = Tcl_GetDouble(interp, argv[i], &tol);
      AY_CHTCLERRRET(tcl_status, argv[0], interp);
      if(tol != tol)
	{
	  ay_error_reportnan(argv[0], "tol");
	  return TCL_OK;
	}
    }

  while(sel)
    {
      o = sel->object;
      if(o->type != AY_IDNCURVE)
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	}
      else
	{
	  curve = (ay_nurbcurve_object *)o->refine;

	  /* find knot to remove */
	  if(have_index)
	    {
	      if(j >= (curve->length+curve->order))
		{
		  (void)ay_error_reportirange(argv[0], "\"index\"", 0,
					      curve->length+curve->order-1);
		  break;
		}
	      u = curve->knotv[j];
	    }

	  /* even if we have an index already, this makes sure we get to
	     know the first of the possibly multiple knots (to correctly
	     compute the current multiplicity) */
	  i = 0;
	  while((i < (curve->length+curve->order)) &&
		(fabs(curve->knotv[i]-u) > AY_EPSILON))
	    {
	      i++;
	    }

	  if(fabs(curve->knotv[i]-u) >= AY_EPSILON)
	    {
	      ay_error(AY_ERROR, argv[0], "Could not find knot to remove.");
	      break;
	    }

	  /* calculate knot multiplicity */
	  s = 1;
	  while(((i+s) < (curve->length+curve->order)) &&
		(fabs(curve->knotv[i] - curve->knotv[i+s]) < AY_EPSILON))
	    {
	      s++;
	    }

	  /* we can not remove knots more often than they appear */
	  if(r > s)
	    {
	      r = s;
	    }

	  /* we can (or should) also not lower the curve order */
	  if((curve->length - r) < curve->order)
	    {
	      ay_error(AY_ERROR, argv[0], "Can not lower curve order.");
	      break;
	    }

	  if(!(newcontrolv = malloc(curve->length*4*sizeof(double))))
	    {
	      ay_error(AY_EOMEM, argv[0], NULL);
	      return TCL_OK;
	    }
	  if(!(newknotv = malloc((curve->length+curve->order)*sizeof(double))))
	    {
	      free(newcontrolv);
	      ay_error(AY_EOMEM, argv[0], NULL);
	      return TCL_OK;
	    }

	  if(curve->is_rat)
	    (void)ay_nct_euctohom(curve);

	  /* remove the knot */
	  ay_status = ay_nb_RemoveKnotCurve4D(curve->length-1,
					      curve->order-1,
					      curve->knotv,
					      curve->controlv,
					      tol, i, s, r,
					      newknotv, newcontrolv);

	  if(ay_status)
	    {
	      ay_error(AY_ERROR, argv[0], "Knot removal failed.");
	      free(newcontrolv);
	      newcontrolv = NULL;
	      free(newknotv);
	      newknotv = NULL;

	      if(curve->is_rat)
		(void)ay_nct_homtoeuc(curve);

	      break;
	    }

	  /* save results */
	  curve->length -= r;
	  memcpy(curve->controlv, newcontrolv, curve->length*4*sizeof(double));
	  memcpy(curve->knotv, newknotv,
		 (curve->length+curve->order)*sizeof(double));

	  free(newcontrolv);
	  newcontrolv = NULL;
	  free(newknotv);
	  newknotv = NULL;

	  if(curve->is_rat)
	    (void)ay_nct_homtoeuc(curve);

	  curve->knot_type = ay_knots_classify(curve->order, curve->knotv,
					       curve->order+curve->length,
					       AY_EPSILON);

	  ay_nct_recreatemp(curve);

	  /* remove all selected points */
	  if(o->selp)
	    ay_selp_clear(o);

	  o->modified = AY_TRUE;

	  /* re-create tesselation of curve */
	  (void)ay_notify_object(sel->object);
	  notify_parent = AY_TRUE;
	} /* if */

      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_removekntcmd */


/** ay_nct_trim:
 *  trim NURBS curve (cut off pieces at start and/or end)
 *
 * \param[in,out] curve  NURBS curve to trim
 * \param[in] umin  new minimum knot value
 * \param[in] umax  new maximum knot value
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_trim(ay_nurbcurve_object **curve, double umin, double umax)
{
 int ay_status;
 ay_object t1 = {0}, *t2 = NULL;
 void *t;
 double knotmin, knotmax;

  if(!curve || !*curve)
    return AY_ENULL;

  if(umax <= umin)
    return AY_ERROR;

  knotmin = (*curve)->knotv[(*curve)->order-1];
  knotmax = (*curve)->knotv[(*curve)->length];

  /* prepare t1 for splitting (XXXX do we need ay_object_defaults()?) */
  t1.type = AY_IDNCURVE;
  t1.refine = *curve;

  if(umin > knotmin)
    {
      ay_status = ay_nct_split(&t1, umin, AY_FALSE, &t2);

      if(ay_status || !t2)
	return AY_ERROR;

      /* remove superfluous curve and save result in t1 */
      t = t2->refine;
      t2->refine = t1.refine;
      t1.refine = t;
      (void)ay_object_delete(t2);
      t2 = NULL;
    } /* if */

  if(umax < knotmax)
    {
      ay_status = ay_nct_split(&t1, umax, AY_FALSE, &t2);

      if(ay_status)
	return AY_ERROR;

      /* remove superfluous curve */
      (void)ay_object_delete(t2);
      t2 = NULL;
    } /* if */

  /* return result */
  *curve = t1.refine;

 return AY_OK;
} /* ay_nct_trim */


/** ay_nct_trimtcmd:
 *  Trim selected NURBS curves.
 *  Implements the \a trimNC scripting interface command.
 *  See also the corresponding section in the \ayd{sctrimnc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_trimtcmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 double umin = 0.0, umax = 0.0;
 int notify_parent = AY_FALSE, argi = 1, relative = AY_FALSE;
 ay_list_object *sel = ay_selection;
 ay_nurbcurve_object *nc;
 ay_object *o = NULL;

  if(argc < 3)
    {
      ay_error(AY_EARGS, argv[0], "[-relative] umin umax");
      return TCL_OK;
    }

  if(argc > 3)
    {
      if(argv[1][0] == '-' && argv[1][1] == 'r')
	{
	  relative = AY_TRUE;
	  argi++;
	}
    }

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  tcl_status = Tcl_GetDouble(interp, argv[argi], &umin);
  AY_CHTCLERRRET(tcl_status, argv[0], interp);
  if(umin != umin)
    {
      ay_error_reportnan(argv[0], "umin");
      return TCL_OK;
    }
  tcl_status = Tcl_GetDouble(interp, argv[argi+1], &umax);
  AY_CHTCLERRRET(tcl_status, argv[0], interp);
  if(umax != umax)
    {
      ay_error_reportnan(argv[0], "umax");
      return TCL_OK;
    }

  if(umax <= umin)
    {
      ay_error(AY_ERROR, argv[0], "Parameter umin must be smaller than umax.");
      return TCL_OK;
    }

  while(sel)
    {
      o = sel->object;
      if(o->type != AY_IDNCURVE)
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	}
      else
	{
	  nc = (ay_nurbcurve_object *)o->refine;

	  if(relative)
	    {
	      if(umin == 0.0)
		umin = nc->knotv[nc->order-1];
	      else
		if(umin == 1.0)
		  umin = nc->knotv[nc->length];
		else
		  umin = nc->knotv[nc->order-1] + (nc->knotv[nc->length] -
					 nc->knotv[nc->order-1]) * umin;

	      if(umax == 0.0)
		umax = nc->knotv[nc->order-1];
	      else
		if(umax == 1.0)
		  umax = nc->knotv[nc->length];
		else
		  umax = nc->knotv[nc->order-1] + (nc->knotv[nc->length] -
					 nc->knotv[nc->order-1]) * umax;
	    }

	  ay_status = ay_nct_trim((ay_nurbcurve_object**)(void*)&(o->refine),
				  umin, umax);

	  if(ay_status)
	    {
	      ay_error(AY_ERROR, argv[0], "Trim operation failed.");
	      break;
	    }

	  ay_nct_recreatemp(o->refine);

	  /* remove all selected points */
	  if(o->selp)
	    ay_selp_clear(o);

	  o->modified = AY_TRUE;

	  /* re-create tesselation of curve */
	  (void)ay_notify_object(sel->object);
	  notify_parent = AY_TRUE;
	} /* if */

      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_trimtcmd */


/** ay_nct_isdegen:
 *  Check curve for degeneracy (all points equal).
 *
 *  Deliberately not checking the weights, as curves with
 *  equal coordinates but different weights also collapse
 *  to a point.
 *
 * \param[in] curve  NURBS curve to check
 *
 * \returns AY_TRUE if curve is degenerate, AY_FALSE else.
 */
int
ay_nct_isdegen(ay_nurbcurve_object *curve)
{
 int i, stride = 4;
 double *p1, *p2;

  if(!curve)
    return AY_FALSE;

  p1 = curve->controlv;
  p2 = p1+stride;

  for(i = 0; i < curve->length-1; i++)
    {
      if(!AY_V3COMP(p1, p2))
	return AY_FALSE;
      p1 += stride;
      p2 += stride;
    } /* for */

 return AY_TRUE;
} /* ay_nct_isdegen */


/** ay_nct_offset:
 *  Create offset curve.
 *
 * \param[in] o  NURBS curve object to offset
 * \param[in] mode  offset mode:
 *            - 0: point mode (offset points along normals derived from
 *                 surrounding control points)
 *            - 1: section mode (calculate offset points from the intersection
 *                 of the surrounding sections)
 *            - 2: hybrid mode (mix result of point and section mode)
 *            - 3: 3D PV N mode (offset points according to normals
 *                 delivered as primitive variable)
 * \param[in] offset  desired offset distance
 * \param[in,out] result  where to store the offset curve
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_offset(ay_object *o, int mode, double offset,
	      ay_nurbcurve_object **result)
{
 int ay_status = AY_OK;
 int i, stride = 4;
 double tangent[3] = {0}, normal[3] = {0}, *newcv = NULL, *newkv = NULL;
 double zaxis[3] = {0.0,0.0,1.0};
 ay_nurbcurve_object *curve = NULL;
 ay_nurbcurve_object *offcurve1 = NULL;
 ay_nurbcurve_object *offcurve2 = NULL;
 ay_nurbcurve_object *nc = NULL;
 ay_tag *tag = NULL;
 double *p1, *p2, *tangents = NULL, *vt;
 double t1[2], n[2];
 char *nname = ay_prefs.normalname;
 unsigned int vnlen = 0;
 double *vn = NULL, vlen, have_mn = AY_FALSE;
 int vnstride = 3, free_vn = AY_FALSE;

  /* sanity check */
  if(!o || !result)
    return AY_ENULL;

  curve = (ay_nurbcurve_object*)o->refine;

  if(!(newcv = calloc(curve->length*stride, sizeof(double))))
    return AY_EOMEM;

  /* special case for simple lines */
  if((curve->length == 2) && (mode != 3))
    {
      p1 = &(curve->controlv[0]);
      p2 = &(curve->controlv[stride]);

      /* calc tangent */
      t1[0] = p2[0] - p1[0];
      t1[1] = p2[1] - p1[1];

      /* calc normal */
      n[0] =  t1[1];
      n[1] = -t1[0];

      /* scale normal to be offset length */
      vlen = AY_V2LEN(n);
      AY_V2SCAL(n, offset/vlen);

      /* offset the line */
      newcv[0] = p1[0] + n[0];
      newcv[1] = p1[1] + n[1];

      newcv[stride]   = p2[0] + n[0];
      newcv[stride+1] = p2[1] + n[1];
    }
  else
    {
      switch(mode)
	{
	case 0:
	  /*
	    "Point" mode:
	    offset corner points according to normals derived
	    from surrounding control points
	  */
	  for(i = 0; i < curve->length; i++)
	    {
	      ay_npt_gettangentfromcontrol2D(curve->type, curve->length,
					     curve->order-1, 4,
					     curve->controlv, i,
					     tangent);

	      AY_V3CROSS(normal, tangent, zaxis);
	      AY_V3SCAL(normal, offset);

	      newcv[i*stride]   = curve->controlv[i*stride]   + normal[0];
	      newcv[i*stride+1] = curve->controlv[i*stride+1] + normal[1];
	      newcv[i*stride+2] = curve->controlv[i*stride+2];
	      newcv[i*stride+3] = curve->controlv[i*stride+3];
	    } /* for */
	  break;
	case 1:
	  /*
	    "Section" mode:
	    offset control polygon sections
	  */
	  free(newcv);
	  ay_status = ay_nct_offsetsection(o, offset, &nc);
	  if(!ay_status && nc)
	    *result = nc;
	  return ay_status;
	case 2:
	  /*
	    "Hybrid" mode:
	    use a combination of both, point and section mode
	  */
	  ay_status = ay_nct_offset(o, 0, offset, &offcurve1);
	  if(ay_status)
	    { goto cleanup; }

	  ay_status = ay_nct_offset(o, 1, offset, &offcurve2);
	  if(ay_status)
	    { goto cleanup; }

	  for(i = 0; i < curve->length; i++)
	    {
	      p1 = &(offcurve1->controlv[i*stride]);
	      p2 = &(offcurve2->controlv[i*stride]);
	      n[0] = (p1[0]+p2[0])/2.0;
	      n[1] = (p1[1]+p2[1])/2.0;

	      newcv[i*stride]   = n[0];
	      newcv[i*stride+1] = n[1];
	      newcv[i*stride+2] = curve->controlv[i*stride+2];
	      newcv[i*stride+3] = curve->controlv[i*stride+3];
	    } /* for */
	  break;
	case 3:
	case 4:
	  /*
	    "3DPVN" mode:
	    offset points according to normals delivered as primitive variable
	    "3DPVNB" mode:
	    offset points according to binormals
	  */

	  /* first check for PV tag of type "n" */
	  tag = o->tags;
	  while(tag)
	    {
	      if(ay_pv_checkndt(tag, nname, "varying", "n"))
		{
		  ay_pv_convert(tag, 0, &vnlen, (void**)(void*)&vn);
		  free_vn = AY_TRUE;
		  break;
		}
	      tag = tag->next;
	    }

	  /* now check for a NT-tag */
	  if(!vn)
	    {
	      tag = o->tags;
	      while(tag)
		{
		  if(tag->type == ay_nt_tagtype)
		    {
		      vn = ((ay_btval*)(tag->val))->payload;
		      vnlen = curve->length;
		      vnstride = 9;
		      break;
		    }
		  tag = tag->next;
		}
	    }

	  /* also, check for a MN-tag */
	  if(!vn)
	    {
	      tag = o->tags;
	      while(tag)
		{
		  if(tag->type == ay_mn_tagtype)
		    {
		      vnlen = sscanf(tag->val, "%lg,%lg,%lg",
			       &(tangent[0]), &(tangent[1]), &(tangent[2]));
		      if(vnlen != 3)
			{
			  ay_status = AY_ERROR;
			  goto cleanup;
			}
		      else
			{
			  have_mn = AY_TRUE;
			}
		      break;
		    }
		  tag = tag->next;
		}
	    }

	  if(!have_mn && (!vn || (vnlen != (unsigned int)curve->length)))
	    {
	      ay_status = AY_ERROR;
	      goto cleanup;
	    }
	  p1 = normal;
	  if(mode == 3)
	    {
	      /* 3DPVN mode: */
	      for(i = 0; i < curve->length; i++)
		{
		  if(have_mn)
		    memcpy(normal, tangent, 3*sizeof(double));
		  else
		    memcpy(normal, &(vn[i*vnstride]), 3*sizeof(double));
		  AY_V3SCAL(normal, offset);
		  newcv[i*stride]   = curve->controlv[i*stride]   + p1[0];
		  newcv[i*stride+1] = curve->controlv[i*stride+1] + p1[1];
		  newcv[i*stride+2] = curve->controlv[i*stride+2] + p1[2];
		  newcv[i*stride+3] = curve->controlv[i*stride+3];
		} /* for */
	    }
	  else
	    {
	      /* 3DPVNB (binormal) mode: */

	      /* XXXX use tangents from PV/NT tag instead? */
	      ay_status = ay_nct_getcvtangents(curve, &tangents);
	      if(ay_status)
		goto cleanup;

	      for(i = 0; i < curve->length; i++)
		{
		  vt = &(tangents[i*3]);
		  if(have_mn)
		    {
		      AY_V3CROSS(normal, tangent, vt);
		    }
		  else
		    {
		      p2 = &(vn[i*vnstride]);
		      AY_V3CROSS(normal, p2, vt);
		    }
		  AY_V3SCAL(normal, offset);
		  newcv[i*stride]   = curve->controlv[i*stride]   + p1[0];
		  newcv[i*stride+1] = curve->controlv[i*stride+1] + p1[1];
		  newcv[i*stride+2] = curve->controlv[i*stride+2] + p1[2];
		  newcv[i*stride+3] = curve->controlv[i*stride+3];
		} /* for */
	    }
	  break;
	default:
	  break;
	} /* switch */
    } /* if */

  if(curve->knot_type == AY_KTCUSTOM)
    {
      if(!(newkv = malloc((curve->length+curve->order)*sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}
      memcpy(newkv, curve->knotv, (curve->length+curve->order)*sizeof(double));
    }

  ay_status = ay_nct_create(curve->order, curve->length, curve->knot_type,
			    newcv, newkv, &nc);

  if(!ay_status)
    *result = nc;

cleanup:

  if(ay_status || !nc)
    {
      if(newcv)
	free(newcv);
      if(newkv)
	free(newkv);
    }

  if(free_vn && vn)
    free(vn);

  if(offcurve1)
    ay_nct_destroy(offcurve1);

  if(offcurve2)
    ay_nct_destroy(offcurve2);

  if(tangents)
    free(tangents);

 return ay_status;
} /* ay_nct_offset */


/** ay_nct_offsetsection:
 *  create offset curve of open/periodic curve in section mode
 *  (calculate offset points from the intersection of the
 *  surrounding sections)
 *
 * \param[in] o  NURBS curve object to offset
 * \param[in] offset  offset distance
 * \param[in,out] nc  offset curve
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_offsetsection(ay_object *o, double offset,
		     ay_nurbcurve_object **nc)
{
 int ay_status = AY_OK;
 int i, j, k, stride = 4;
 double *newcv = NULL, *newkv = NULL;
 ay_nurbcurve_object *curve = NULL;
 int p1len, p2len, p3len = 0;
 double *p1, *p2, *p3, *pt, *po, p1s1[2], p2s1[2], p1s2[2], p2s2[2];
 double t1[2], t2[2], n[2];
 double vlen;

  /* sanity check */
  if(!o || !nc)
    return AY_ENULL;

  curve = (ay_nurbcurve_object*)o->refine;

  if(!(newcv = calloc(curve->length*stride, sizeof(double))))
    return AY_EOMEM;

  p1 = &(curve->controlv[0]);
  po = &(curve->controlv[curve->length*stride]);
  /* get length of p1 (count multiple points) */
  pt = p1+stride;
  p1len = 1;
  while((pt != po) && (AY_V2COMP(p1, pt)))
    {
      p1len++;
      pt += stride;
    }

  if(pt == po)
    {
      /* this curve, apparently, has no sections
	 (is degenerated to one point) */
      free(newcv);
      return AY_ERROR;
    }

  p2 = pt;
  pt = p2+stride;
  p2len = 1;
  while((pt != po) && (AY_V2COMP(p2, pt)))
    {
      p2len++;
      pt += stride;
    }

  /* calc tangent of first original control polygon segment */
  t1[0] = p2[0] - p1[0];
  t1[1] = p2[1] - p1[1];

  /* calc normal of first original control polygon segment */
  n[0] =  t1[1];
  n[1] = -t1[0];

  /* scale normal to be offset length */
  vlen = AY_V2LEN(n);
  AY_V2SCAL(n, offset/vlen);

  /* offset the first control polygon segment */
  p1s1[0] = p1[0] + n[0];
  p1s1[1] = p1[1] + n[1];

  p2s1[0] = p2[0] + n[0];
  p2s1[1] = p2[1] + n[1];

  /* first point of offset curves control polygon */
  for(i = 0; i < p1len; i++)
    {
      newcv[i*stride]   = p1s1[0];
      newcv[i*stride+1] = p1s1[1];
    }

  /* special case: curve has one section and multiple
     points at the beginning _and_ at the end */
  if((p1len+p2len) == curve->length)
    {
      for(i = p1len; i < curve->length; i++)
	{
	  newcv[i*stride]   = p2s1[0];
	  newcv[i*stride+1] = p2s1[1];
	}
    }
  else
    if(curve->type == AY_CTCLOSED)
      {
	/* find and offset the last control polygon segment */
	p3 = &(curve->controlv[(curve->length-1)*stride]);
	pt = p3-stride;
	p3len = 1;

	while((pt != p2) && (AY_V2COMP(p3, pt)))
	  {
	    p3len++;
	    pt -= stride;
	  }

	/* calc tangent of last original control polygon segment */
	t2[0] = p3[0]-pt[0];
	t2[1] = p3[1]-pt[1];

	/* calc normal of last original control polygon segment */
	n[0] =  t2[1];
	n[1] = -t2[0];

	/* scale normal to be offset length */
	vlen = AY_V2LEN(n);
	AY_V2SCAL(n, offset/vlen);

	/* offset the last control polygon segment */
	p1s2[0] = p1[0] + n[0];
	p1s2[1] = p1[1] + n[1];

	p2s2[0] = p3[0] + n[0];
	p2s2[1] = p3[1] + n[1];

	/* intersect last and first offset segments, intersection is new cv */
	if(!ay_geom_intersectlines2D(p1s1, t1, p1s2, t2, newcv))
	  {
	    /*
	     * the intersection failed (e.g. due to collinear
	     * segments) => we simply pick one of the inner
	     * offset segment points
	     */

	    /* first point(s) of offset curves control polygon */
	    for(i = 0; i < p1len; i++)
	      {
		newcv[i*stride]   = p1s1[0];
		newcv[i*stride+1] = p1s1[1];
	      }
	    /* last point(s) of offset curves control polygon */
	    j = curve->length-1;
	    for(i = 0; i < p3len; i++)
	      {
		newcv[j*stride]   = p1s1[0];
		newcv[j*stride+1] = p1s1[1];
		j--;
	      }
	  }
	else
	  {
	    j = 0;
	    for(k = 1; k < p1len; k++)
	      {
		memcpy(&(newcv[(j+k)*stride]), newcv, 2*sizeof(double));
	      }
	    j = curve->length-1;
	    for(k = 0; k < p3len; k++)
	      {
		memcpy(&(newcv[j*stride]), newcv, 2*sizeof(double));
		j--;
	      }
	  } /* if has intersection */
      }
    else
      if(curve->type == AY_CTPERIODIC)
	{
	  /* find and offset the previous control polygon segment */
	  p3 = &(curve->controlv[(curve->length-curve->order)*stride]);
	  while((p3 != &(curve->controlv[0])) && (AY_V2COMP(p3, p1)))
	    {
	      p3 -= stride;
	    }

	  /* calc tangent of previous original control polygon segment */
	  t2[0] = p1[0]-p3[0];
	  t2[1] = p1[1]-p3[1];

	  /* calc normal of previous original control polygon segment */
	  n[0] =  t2[1];
	  n[1] = -t2[0];

	  /* scale normal to be offset length */
	  vlen = AY_V2LEN(n);
	  AY_V2SCAL(n, offset/vlen);

	  /* offset the control polygon segment */
	  p1s2[0] = p3[0] + n[0];
	  p1s2[1] = p3[1] + n[1];

	  p2s2[0] = p1[0] + n[0];
	  p2s2[1] = p1[1] + n[1];

	  /* intersect two offset segments, intersection is new cv */
	  if(!ay_geom_intersectlines2D(p1s1, t1, p1s2, t2, newcv))
	    {
	      /*
	       * the intersection failed (e.g. due to collinear
	       * segments) => we simply pick one of the inner
	       * offset segment points
	       */
	      for(k = 0; k < p1len; k++)
		{
		  memcpy(&(newcv[k*stride]), p2s2, 2*sizeof(double));
		}
	    }
	  else
	    {
	      /* replicate result if p1 is a multiple point */
	      if(p1len > 1)
		{
		  for(k = 1; k < p1len; k++)
		    {
		      memcpy(&(newcv[k*stride]), newcv, 2*sizeof(double));
		    }
		}
	    } /* if has intersection */
	} /* if periodic */

  if((p1len+p2len) < curve->length)
    {
      j = p1len;
      while((j+p2len) < curve->length)
	{
	  p3 = &(curve->controlv[(j+p2len)*stride]);
	  p3len = 1;
	  if((j+p2len) < (curve->length-1))
	    {
	      pt = p3+stride;
	      while((pt != po) && (AY_V2COMP(p3, pt)))
		{
		  p3len++;
		  pt += stride;
		}
	    }

	  /* calc tangent of next original control polygon segment */
	  t2[0] = p3[0]-p2[0];
	  t2[1] = p3[1]-p2[1];

	  /* calc normal of next original control polygon segment */
	  n[0] =  t2[1];
	  n[1] = -t2[0];

	  /* scale normal to be offset length */
	  vlen = AY_V2LEN(n);
	  AY_V2SCAL(n, offset/vlen);

	  /* offset the control polygon segment */
	  p1s2[0] = p2[0] + n[0];
	  p1s2[1] = p2[1] + n[1];

	  p2s2[0] = p3[0] + n[0];
	  p2s2[1] = p3[1] + n[1];

	  /* intersect two offset segments, intersection is new cv */
	  if(!ay_geom_intersectlines2D(p1s1, t1, p1s2, t2,
				       &(newcv[j*stride])))
	    {
	      /*
	       * the intersection failed (e.g. due to collinear
	       * segments) => we simply pick one of the inner
	       * offset segment points
	       */
	      for(k = 0; k < p2len; k++)
		{
		  memcpy(&(newcv[(j+k)*stride]), p2s1, 2*sizeof(double));
		}
	    }
	  else
	    {
	      if(p2len > 1)
		{
		  for(k = 1; k < p2len; k++)
		    {
		      memcpy(&(newcv[(j+k)*stride]), &(newcv[j*stride]),
			     2*sizeof(double));
		    }
		}
	    } /* if has intersection */

	  /* prepare next iteration */
	  p2 = p3;

	  memcpy(t1, t2, 2*sizeof(double));
	  memcpy(p1s1, p1s2, 2*sizeof(double));
	  memcpy(p2s1, p2s2, 2*sizeof(double));

	  j += p2len;
	  p2len = p3len;
	} /* while */

      /* last point of offset curves control polygon */
      for(i = 0; i < p3len; i++)
	{
	  newcv[(j+i)*stride]   = p2s2[0];
	  newcv[(j+i)*stride+1] = p2s2[1];
	}
    } /* if */

  /* set z and weights */
  po = curve->controlv;
  if(curve->is_rat)
    {
      for(j = 0; j < curve->length; j++)
	{
	  newcv[j*stride+2] = po[j*stride+2];
	  newcv[j*stride+3] = po[j*stride+3];
	}
    }
  else
    {
      for(j = 0; j < curve->length; j++)
	{
	  newcv[j*stride+2] = po[j*stride+2];
	  newcv[j*stride+3] = 1.0;
	}
    } /* if */

  if((curve->type == AY_CTPERIODIC) && (curve->order > 2))
    {
      j = (curve->length-(curve->order-1))*stride;
      memcpy(&(newcv[j]), newcv, (curve->order-1)*stride*sizeof(double));
    }

  if(curve->type == AY_CTCLOSED ||
     ((curve->type == AY_CTPERIODIC) && (curve->order == 2)))
    {
      j = (curve->length-1)*stride;
      memcpy(&(newcv[j]), newcv, stride*sizeof(double));
    }

  if(curve->knot_type == AY_KTCUSTOM)
    {
      if(!(newkv = malloc((curve->length+curve->order)*sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}
      memcpy(newkv, curve->knotv, (curve->length+curve->order)*sizeof(double));
    }

  ay_status = ay_nct_create(curve->order, curve->length, curve->knot_type,
			    newcv, newkv, nc);

cleanup:

  if(ay_status || !nc)
    {
      if(newcv)
	free(newcv);
      if(newkv)
	free(newkv);
    }

 return ay_status;
} /* ay_nct_offsetsection */


/** ay_nct_cmppnt:
 *  compare two points (helper for qsort)
 *
 * \param[in] p1  point 1
 * \param[in] p2  point 2
 *
 * \returns
 *   - 0  the points are equal
 *   - -1 p1 is smaller than p2
 *   - 1 p1 is bigger than p2
 */
int
ay_nct_cmppnt(const void *p1, const void *p2)
{

  if((((double*)p1)[0] < ((double*)p2)[0]) &&
     (fabs(((double*)p2)[0] - ((double*)p1)[0]) > AY_EPSILON))
    return -1;
  if((((double*)p1)[0] > ((double*)p2)[0]) &&
     (fabs(((double*)p1)[0] - ((double*)p2)[0]) > AY_EPSILON))
    return 1;

  if((((double*)p1)[1] < ((double*)p2)[1]) &&
     (fabs(((double*)p2)[1] - ((double*)p1)[1]) > AY_EPSILON))
    return -1;
  if((((double*)p1)[1] > ((double*)p2)[1]) &&
     (fabs(((double*)p1)[1] - ((double*)p2)[1]) > AY_EPSILON))
    return 1;

  if((((double*)p1)[2] < ((double*)p2)[2]) &&
     (fabs(((double*)p2)[2] - ((double*)p1)[2]) > AY_EPSILON))
    return -1;
  if((((double*)p1)[2] > ((double*)p2)[2]) &&
     (fabs(((double*)p1)[2] - ((double*)p2)[2]) > AY_EPSILON))
    return 1;

 return 0;
} /* ay_nct_cmppnt */


/** ay_nct_cmppntp:
 *  compare two points given as pointers (helper for qsort)
 * \param[in] p1  pointer to point 1
 * \param[in] p2  pointer to point 2
 *
 * \returns
 *   - 0  the points are equal
 *   - -1 p1 is smaller than p2
 *   - 1 p1 is bigger than p2
 */
int
ay_nct_cmppntp(const void *p1, const void *p2)
{

  if(((*(double**)p1)[0] < (*(double**)p2)[0]) &&
     (fabs((*(double**)p2)[0] - (*(double**)p1)[0]) > AY_EPSILON))
    return -1;
  if(((*(double**)p1)[0] > (*(double**)p2)[0]) &&
     (fabs((*(double**)p1)[0] - (*(double**)p2)[0]) > AY_EPSILON))
    return 1;

  if(((*(double**)p1)[1] < (*(double**)p2)[1]) &&
     (fabs((*(double**)p2)[1] - (*(double**)p1)[1]) > AY_EPSILON))
    return -1;
  if(((*(double**)p1)[1] > (*(double**)p2)[1]) &&
     (fabs((*(double**)p1)[1] - (*(double**)p2)[1]) > AY_EPSILON))
    return 1;

  if(((*(double**)p1)[2] < (*(double**)p2)[2]) &&
     (fabs((*(double**)p2)[2] - (*(double**)p1)[2]) > AY_EPSILON))
    return -1;
  if(((*(double**)p1)[2] > (*(double**)p2)[2]) &&
     (fabs((*(double**)p1)[2] - (*(double**)p2)[2]) > AY_EPSILON))
    return 1;

 return 0;
} /* ay_nct_cmppntp */


/** ay_nct_estlen:
 *  Estimate length of a NURBS curve.
 *
 * \param[in] nc  NURBS curve object
 * \param[in,out] len  where to store the estimated length
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_estlen(ay_nurbcurve_object *nc, double *len)
{
 int ay_status = AY_OK;
 double v[3], slen, tlen, *Qw = NULL;
 double *oldQw = NULL, *oldU = NULL;
 int a, i, j, nb = 0;
 int stride = 4, freeQw = AY_FALSE;

  if(!nc || !len)
    return AY_ENULL;

  *len = 0.0;

  /* special case for polygons */
  if(nc->order == 2)
    {
      a = 0;
      Qw = nc->controlv;
      for(j = 0; j < (nc->length-1); j++)
	{
	  v[0] = Qw[a+stride] - Qw[a];
	  v[1] = Qw[a+stride+1] - Qw[a+1];
	  v[2] = Qw[a+stride+2] - Qw[a+2];

	  if(fabs(v[0]) > AY_EPSILON ||
	     fabs(v[1]) > AY_EPSILON ||
	     fabs(v[2]) > AY_EPSILON)
	    {
	      *len += AY_V3LEN(v);
	    }
	  a += stride;
	} /* for */

      return ay_status;
    } /* if */

  j = ay_knots_isclamped(/*side=*/0, nc->order, nc->knotv,
			 nc->length+nc->order, AY_EPSILON);

  if(!j || nc->is_rat)
    {
      oldQw = nc->controlv;

      if(!(nc->controlv = malloc(nc->length*stride*sizeof(double))))
	{
	  nc->controlv = oldQw;
	  return AY_EOMEM;
	}

      memcpy(nc->controlv, oldQw, nc->length*stride*sizeof(double));

      if(nc->is_rat)
	{
	  Qw = nc->controlv;
	  a = 0;
	  for(i = 0; i < nc->length; i++)
	    {
	      Qw[a]   *= Qw[a+3];
	      Qw[a+1] *= Qw[a+3];
	      Qw[a+2] *= Qw[a+3];
	      a += stride;
	    }
	} /* if is rat */

      if(!j)
	{
	  oldU = nc->knotv;

	  if(!(nc->knotv = malloc((nc->length+nc->order)*sizeof(double))))
	    {
	      nc->knotv = oldU;
	      free(nc->controlv);
	      nc->controlv = oldQw;
	      return AY_EOMEM;
	    }
	  memcpy(nc->knotv, oldU, (nc->length+nc->order)*sizeof(double));

	  ay_status = ay_nct_clamp(nc, /*side=*/0);
	  if(ay_status)
	    goto cleanup;
	}
    } /* if */

  if(nc->length != nc->order)
    {
      if(!(Qw = malloc(nc->order*stride*sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      freeQw = AY_TRUE;

      ay_status = ay_nb_DecomposeCurve(stride, nc->length-1, nc->order-1,
				       nc->knotv, nc->controlv, &nb, &Qw);
      if(ay_status)
	goto cleanup;
    }
  else
    {
      nb = 1;
      Qw = nc->controlv;
    }

  a = 0;
  for(i = 0; i < nb; i++)
    {
      v[0] = Qw[a+((nc->order-1)*stride)] - Qw[a];
      v[1] = Qw[a+((nc->order-1)*stride)+1] - Qw[a+1];
      v[2] = Qw[a+((nc->order-1)*stride)+2] - Qw[a+2];

      if(fabs(v[0]) > AY_EPSILON ||
	 fabs(v[1]) > AY_EPSILON ||
	 fabs(v[2]) > AY_EPSILON)
	{
	  tlen = AY_V3LEN(v);
	}
      else
	{
	  tlen = 0.0;
	}

      slen = 0.0;
      for(j = 0; j < (nc->order-1); j++)
	{
	  v[0] = Qw[a+stride] - Qw[a];
	  v[1] = Qw[a+stride+1] - Qw[a+1];
	  v[2] = Qw[a+stride+2] - Qw[a+2];

	  if(fabs(v[0]) > AY_EPSILON ||
	     fabs(v[1]) > AY_EPSILON ||
	     fabs(v[2]) > AY_EPSILON)
	    {
	      slen += AY_V3LEN(v);
	    }

	  a += stride;
	} /* for */

      if(tlen > AY_EPSILON && fabs(slen-tlen) > AY_EPSILON)
	{
	  *len += tlen+((slen-tlen)/2.0);
	}
      else
	{
	  *len += slen;
	}

      /* next segment */
      a += stride;
    } /* for */

cleanup:

  if(freeQw)
    {
      free(Qw);
    }

  if(oldU)
    {
      if(nc->knotv)
	free(nc->knotv);
      nc->knotv = oldU;
    }

  if(oldQw)
    {
      if(nc->controlv)
	free(nc->controlv);
      nc->controlv = oldQw;
    }

 return ay_status;
} /* ay_nct_estlen */


/** ay_nct_estlentcmd:
 *  Estimate length of selected NURBS curves.
 *  Implements the \a estlenNC scripting interface command.
 *  See also the corresponding section in the \ayd{scestlennc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_estlentcmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[])
{
 int ay_status = AY_OK, tcl_status = TCL_OK;
 ay_list_object *sel = ay_selection;
 ay_nurbcurve_object *curve;
 ay_object *o, *po = NULL;
 double len;
 int have_vname = AY_FALSE, apply_trafo = 0, i = 1, r = 0;
 Tcl_Obj *to = NULL;
 char *vname;

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  /* parse args */
  while(i < argc)
    {
      if((argv[i][0] == '-') && (argv[i][1] == 't'))
	{
	  apply_trafo = 1;
	}
      else
	if((argv[i][0] == '-') && (argv[i][1] == 'r'))
	  {
	    /* -refine */
	    if(argc < i+1)
	      {
		ay_error(AY_EARGS, argv[0], "[-t | -r n] [vname]");
		return TCL_OK;
	      }
	    tcl_status = Tcl_GetInt(interp, argv[i+1], &r);
	    AY_CHTCLERRRET(tcl_status, argv[0], interp);
	    if(r < 0)
	      r = 0;
	    if(r > 5)
	      r = 5;
	    i++;
	  } /* if have -refine */
	else
	  {
	    vname = argv[i];
	    have_vname = AY_TRUE;
	  }

#if 0
      else
	if((argv[i][0] == '-') && (argv[i][1] == 'w'))
	  {
	    /* -world */
	    apply_trafo = 2;
	    i++;
	  }
#endif
      i++;
    } /* while */

  while(sel)
    {
      /* get curve to work on */
      o = sel->object;
      if(o->type != AY_IDNCURVE)
	{
	  ay_status = ay_provide_object(sel->object, AY_IDNCURVE, &po);
	  if(ay_status || !po)
	    {
	      ay_error(AY_ERROR, argv[0], "Provide failed.");
	      goto cleanup;
	    }

	  o = po;
	  while(o)
	    {
	      if(apply_trafo)
		{
		  ay_nct_applytrafo(o);
		}
	      curve = (ay_nurbcurve_object *)o->refine;

	      while(r)
		{
		  ay_status = ay_nct_refinekn(curve, AY_FALSE, NULL, 0);
		  if(ay_status)
		    {
		      ay_error(AY_ERROR, argv[0], "Refine failed.");
		      goto cleanup;
		    }
		  r--;
		} /* while refine */

	      /* get length */
	      ay_status = ay_nct_estlen(curve, &len);

	      if(ay_status)
		goto cleanup;

	      /* put result into Tcl context */
	      to = Tcl_NewDoubleObj(len);
	      if(have_vname)
		{
		  Tcl_SetVar2Ex(interp, vname, NULL, to,
		    TCL_LEAVE_ERR_MSG | TCL_APPEND_VALUE | TCL_LIST_ELEMENT);
		}
	      else
		{
		  if(po->next || ay_selection->next)
		    Tcl_AppendElement(interp, Tcl_GetStringFromObj(to, NULL));
		  else
		    Tcl_SetObjResult(interp, to);
		}

	      o = o->next;
	    } /* while */
	}
      else
	{
	  /* o is NCurve */
	  curve = (ay_nurbcurve_object *)o->refine;

	  if(apply_trafo)
	    {
	      ay_status = ay_object_copy(o, &po);
	      if(ay_status || !po)
		goto cleanup;
	      ay_nct_applytrafo(po);
	      curve = (ay_nurbcurve_object *)po->refine;
	    }

	  if(r > 0 && curve->order > 2)
	    {
	      if(!po)
		{
		  ay_status = ay_object_copy(o, &po);
		  if(ay_status || !po)
		    goto cleanup;
		  curve = (ay_nurbcurve_object *)po->refine;
		}
	      while(r)
		{
		  ay_status = ay_nct_refinekn(curve, AY_FALSE, NULL, 0);
		  if(ay_status)
		    {
		      ay_error(AY_ERROR, argv[0], "Refine failed.");
		      goto cleanup;
		    }
		  r--;
		}
	    } /* if refine */

	  /* get length */
	  ay_status = ay_nct_estlen(curve, &len);

	  if(ay_status)
	    goto cleanup;

	  /* put result into Tcl context */
	  to = Tcl_NewDoubleObj(len);
	  if(have_vname)
	    {
	      Tcl_SetVar2Ex(interp, vname, NULL, to,
		    TCL_LEAVE_ERR_MSG | TCL_APPEND_VALUE | TCL_LIST_ELEMENT);
	    }
	  else
	    {
	      if(ay_selection->next)
		Tcl_AppendElement(interp, Tcl_GetStringFromObj(to, NULL));
	      else
		Tcl_SetObjResult(interp, to);
	    }
	} /* if is ncurve */

      if(po)
	{
	  (void)ay_object_deletemulti(po, AY_FALSE);
	  po = NULL;
	}

      sel = sel->next;
    } /* while */

  po = NULL;

  /* cleanup */
cleanup:

  if(po)
    (void)ay_object_deletemulti(po, AY_FALSE);

 return TCL_OK;
} /* ay_nct_estlentcmd */


/** ay_nct_reparamtcmd:
 *  Reparameterise selected NURBS curves.
 *  Implements the \a reparamNC scripting interface command.
 *  See also the corresponding section in the \ayd{screparamnc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_reparamtcmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_nurbcurve_object *curve;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL;
 int type = 0, tesslen;
 int notify_parent = AY_FALSE;
 double *tess = NULL, *controlv = NULL, *knotv = NULL;

  /* parse args */
  if(argc < 2)
    {
      ay_error(AY_EARGS, argv[0], "t");
      return TCL_OK;
    }

  tcl_status = Tcl_GetInt(interp, argv[1], &type);
  AY_CHTCLERRRET(tcl_status, argv[0], interp);

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  while(sel)
    {
      o = sel->object;
      if(o->type != AY_IDNCURVE)
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	}
      else
	{
	  curve = (ay_nurbcurve_object *)o->refine;
	  if(tess)
	    {
	      free(tess);
	      tess = NULL;
	    }
	  if(knotv)
	    {
	      free(knotv);
	      knotv = NULL;
	    }
	  if(controlv)
	    {
	      free(controlv);
	      controlv = NULL;
	    }

	  ay_status = ay_tess_ncurve(curve,
				     GLU_OBJECT_PARAMETRIC_ERROR,
				     0.1,
				     &tess, &tesslen);
	  if(ay_status)
	    {
	      ay_error(AY_ERROR, argv[0], "Tesselation failed.");
	      goto cleanup;
	    }

	  ay_status = ay_act_leastSquares(tess, tesslen,
					  curve->length,
					  curve->order-1, type,
					  &knotv, &controlv);
	  if(ay_status)
	    {
	      ay_error(AY_ERROR, argv[0], "Approximation failed.");
	      goto cleanup;
	    }

	  free(curve->knotv);
	  curve->knotv = knotv;
	  free(curve->controlv);
	  curve->controlv = controlv;
#if 0
	  memcpy(curve->knotv, knotv,
		 (curve->length+curve->order)*sizeof(double));
	  memcpy(curve->controlv, controlv,
		 curve->length*4*sizeof(double));
#endif
	  curve->is_rat = AY_FALSE;

	  ay_nct_recreatemp(curve);
	  o->modified = AY_TRUE;

	  /* re-create tesselation of curve */
	  (void)ay_notify_object(sel->object);
	  notify_parent = AY_TRUE;
	} /* if is ncurve */

      sel = sel->next;
    } /* while */

  /* prevent cleanup code from doing something harmful */
  knotv = NULL;
  controlv = NULL;

cleanup:

  if(tess)
    free(tess);

  if(knotv)
    free(knotv);

  if(controlv)
    free(controlv);

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_reparamtcmd */


/** ay_nct_isplanar:
 *  Check a curve for planarity by rotating a copy of
 *  the curve to the XY-plane and testing if any
 *  Z-coordinates are different (to each other).
 *
 * \param[in] c  NURBS curve/ICurve/ACurve to check
 * \param[in] allow_flip  allow curve reversal
 * \param[in,out] cp  pointer where to store the curve rotated to XY,
                      may be NULL
 * \param[in,out] is_planar  pointer where to store the result (AY_TRUE if the
 *                           curve is planar, AY_FALSE, else);
 *                           in case of an error and for
 *                           straight/linear/degenerate
 *                           curves, is_planar will _not_ be changed
 */
void
ay_nct_isplanar(ay_object *c, int allow_flip, ay_object **cp, int *is_planar)
{
 int ay_status;
 ay_object *tmp = NULL;
 ay_nurbcurve_object *nc;
 ay_icurve_object *ic;
 ay_acurve_object *ac;
 double *cv;
 int i, len = 0, stride = 4;

  if(!c || !is_planar)
    return;

  ay_status = ay_object_copy(c, &tmp);

  if(ay_status || !tmp)
    return;

  ay_status = ay_nct_toplane(AY_XY, allow_flip, tmp);

  /* if ay_nct_toplane() failed, maybe it is a linear/degenerate curve... */

  if(!ay_status)
    {
      *is_planar = AY_TRUE;
      switch(tmp->type)
	{
	case AY_IDNCURVE:
	  nc = (ay_nurbcurve_object *)tmp->refine;
	  cv = nc->controlv;
	  len = nc->length;
	  break;
	case AY_IDICURVE:
	  ic = (ay_icurve_object *)tmp->refine;
	  cv = ic->controlv;
	  stride = 3;
	  len = ic->length;
	  break;
	case AY_IDACURVE:
	  ac = (ay_acurve_object *)tmp->refine;
	  cv = ac->controlv;
	  stride = 3;
	  len = ac->length;
	  break;
	default:
	  break;
	} /* switch */

      for(i = 0; i < len-1; i++)
	{
	  if(fabs(cv[i*stride+2]-cv[(i+1)*stride+2]) > AY_EPSILON*2)
	    {
	      *is_planar = AY_FALSE;
	      break;
	    }
	}
    } /* if toplane() succeeded */

  if(cp)
    *cp = tmp;
  else
    (void)ay_object_delete(tmp);

 return;
} /* ay_nct_isplanar */


/** ay_nct_unclamptcmd:
 *  Unclamp the selected NURBS curves.
 *  Implements the \a unclampNC scripting interface command.
 *  See also the corresponding section in the \ayd{scunclampnc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_unclamptcmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
 int ay_status = AY_OK, tcl_status = TCL_OK, side = 0;
 ay_nurbcurve_object *curve;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL;
 int update_selp = AY_FALSE;
 int notify_parent = AY_FALSE;

  /* parse args */
  if(argc > 1)
   {
     if((argv[1][0] == '-') && (argv[1][1] == 's'))
       side = 1;
     else
       if((argv[1][0] == '-') && (argv[1][1] == 'e'))
	 side = 2;
       else
	 {
	   tcl_status = Tcl_GetInt(interp, argv[1], &side);
	   AY_CHTCLERRRET(tcl_status, argv[0], interp);
	 }
   }

  /* check selection */
  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  while(sel)
    {
      o = sel->object;
      if(o->type != AY_IDNCURVE)
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	}
      else
	{
	  curve = (ay_nurbcurve_object *)o->refine;

	  if(!ay_knots_isclamped(side, curve->order, curve->knotv,
				 curve->length+curve->order, AY_EPSILON))
	    {
	      ay_status = ay_nct_clamp(curve, side);
	      if(ay_status)
		break;
	      update_selp = AY_TRUE;
	    }

	  ay_nb_UnclampCurve(curve->is_rat,
			     curve->length-1, curve->order-1, side,
			     curve->knotv, curve->controlv,
			     /*updateknots=*/AY_TRUE);

	  curve->knot_type = ay_knots_classify(curve->order, curve->knotv,
					       curve->order+curve->length,
					       AY_EPSILON);

	  ay_nct_settype(curve);

	  if(update_selp)
	    {
	      /* update selected points pointers to controlv */
	      if(o->selp)
		{
		  (void)ay_pact_getpoint(3, o, NULL, NULL);
		}
	    }

	  /* clean up */
	  ay_nct_recreatemp(curve);

	  /* show ay_notify_parent() the changed objects */
	  o->modified = AY_TRUE;

	  /* re-create tesselation of curve */
	  (void)ay_notify_object(sel->object);
	  notify_parent = AY_TRUE;
	} /* if */

      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_unclamptcmd */


/** ay_nct_extend:
 *  extend a NURBS curve to a given point, not changing the
 *  current shape of the curve
 *
 * \param[in,out] curve  NURBS curve object to extend
 * \param[in] p  point to extend the curve to (must be different
 *  from the last control point in \a curve)
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_extend(ay_nurbcurve_object *curve, double *p)
{
 int ay_status = AY_OK;
 int i, a, stride = 4;
 double tl = 0.0, l = 0.0, *newcv = NULL, *newkv = NULL;
 double u, v[3];

  /* sanity check */
  if(!curve || !p)
    return AY_ENULL;

  /* can only unclamp completely clamped end... */
  if(!ay_knots_isclamped(/*side=*/2, curve->order, curve->knotv,
			 curve->length+curve->order, AY_EPSILON))
    {
      ay_status = ay_nct_clamp(curve, /*side=*/2);
      if(ay_status)
	return ay_status;
    }

  /* rescale knots to proper range (0-1) */
  if((curve->knotv[0] != 0.0) ||
     (curve->knotv[curve->length+curve->order-1] != 1.0))
    {
      ay_status = ay_knots_rescaletorange(curve->length+curve->order,
					  curve->knotv, 0.0, 1.0);
      if(ay_status)
	return ay_status;
    }

  if(!(newcv = malloc((curve->length+1)*stride*sizeof(double))))
    return AY_EOMEM;

  memcpy(newcv, curve->controlv, curve->length*stride*sizeof(double));

  memcpy(&(newcv[curve->length*stride]), p, stride*sizeof(double));

  /* estimate total length of current curve */
  a = 0;
  for(i = 0; i < curve->length-1; i++)
    {
      v[0] = newcv[a+stride]   - newcv[a];
      v[1] = newcv[a+stride+1] - newcv[a+1];
      v[2] = newcv[a+stride+2] - newcv[a+2];
      if(fabs(v[0]) > AY_EPSILON ||
	 fabs(v[1]) > AY_EPSILON ||
	 fabs(v[2]) > AY_EPSILON)
	tl += AY_V3LEN(v);
      a += stride;
    }

  if(tl < AY_EPSILON)
    {
      free(newcv);
      return AY_ERROR;
    }

  /* get length of new last segment */
  a = (curve->length-1)*stride;
  v[0] = p[0] - newcv[a];
  v[1] = p[1] - newcv[a+1];
  v[2] = p[2] - newcv[a+2];
  if(fabs(v[0]) > AY_EPSILON ||
     fabs(v[1]) > AY_EPSILON ||
     fabs(v[2]) > AY_EPSILON)
    {
      l = AY_V3LEN(v);
    }
  else
    {
      free(newcv);
      return AY_ERROR;
    }

  /* calculate new knot */
  u = 1.0 + (l/tl);

  if(!(newkv = malloc((curve->length+1+curve->order)*sizeof(double))))
    {
      free(newcv);
      return AY_EOMEM;
    }

  memcpy(newkv, curve->knotv, (curve->length+1)*sizeof(double));

  for(i = curve->length+1; i < curve->length+curve->order; i++)
    newkv[i] = u;

  ay_nb_UnclampCurve(curve->is_rat, curve->length-1, curve->order-1, 2,
		     newkv, newcv, /* updateknots=*/AY_FALSE);

  curve->length++;
  curve->knot_type = AY_KTCUSTOM;

  for(i = 0; i < curve->length; i++)
    newkv[i] /= u;

  for(i = curve->length; i < curve->length+curve->order; i++)
    newkv[i] = 1.0;

  free(curve->controlv);
  curve->controlv = newcv;

  free(curve->knotv);
  curve->knotv = newkv;

  /* the type of the curve likely changed, recompute... */
  ay_nct_settype(curve);

 return AY_OK;
} /* ay_nct_extend */


/** ay_nct_extendtcmd:
 *  Extend the selected NURBS curves to a point.
 *  Implements the \a extendNC scripting interface command.
 *  See also the corresponding section in the \ayd{scextendnc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_extendtcmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_nurbcurve_object *curve;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL;
 double p[4] = {0}, m[3], *c = NULL;
 int to_mark = AY_FALSE, i = 0;
 int notify_parent = AY_FALSE;

  /* parse args */
  if(argc > 1)
    {
      p[3] = 1.0;
      if(argv[1][0] == '-' && argv[1][1] == 'm')
	{
	  to_mark = AY_TRUE;
	  ay_viewt_getglobalmark(&c);
	  if(!c)
	    {
	      ay_error(AY_ERROR, argv[0], "Could not get global mark.");
	      return TCL_OK;
	    }
	  memcpy(m, c, 3*sizeof(double));
	}
      else
      if(argv[1][0] == '-' && argv[1][1] == 'v')
	{
	  tcl_status = ay_tcmd_convdlist(interp, argv[2], &i, &c);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  if(i > 4)
	    i = 4;
	  memcpy(p, c, i*sizeof(double));
	}
      else
	{
	  for(i = 0; i < argc-1; i++)
	    {
	      tcl_status = Tcl_GetDouble(interp, argv[i+1], &p[i]);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      if(i == 3)
		break;
	    } /* for */
	}
    }
  else
    {
      ay_error(AY_EARGS, argv[0], "(x y z (w)|-vn varname|-m)");
      return TCL_OK;
    } /* if */

  /* check selection */
  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  while(sel)
    {
      o = sel->object;
      if(o->type != AY_IDNCURVE)
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	}
      else
	{
	  if(to_mark)
	    {
	      memcpy(p, m, 3*sizeof(double));
	      /* convert world coordinates to object space */
	      ay_trafo_applyalli(ay_currentlevel->next, o, p);
	    }

	  curve = (ay_nurbcurve_object *)o->refine;

	  ay_status = ay_nct_extend(curve, p);

	  if(ay_status)
	    {
	      ay_error(AY_ERROR, argv[0], "Extend operation failed.");
	      break;
	    }

	  /* clean up */
	  if(o->selp)
	    {
	      (void)ay_pact_getpoint(3, o, NULL, NULL);
	    }

	  ay_nct_recreatemp(curve);

	  /* show ay_notify_parent() the changed objects */
	  o->modified = AY_TRUE;

	  /* re-create tesselation of curve */
	  (void)ay_notify_object(o);
	  notify_parent = AY_TRUE;
	} /* if */

      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_extendtcmd */


/** ay_nct_meandist:
 * Compute mean distance of two 1D control point arrays.
 *
 * \param[in] cvlen  number of points in \a cva _and_ \a cvb
 * \param[in] cvastride  size of a point in \a cva (>=3, unchecked)
 * \param[in] cva  first coordinate array
 * \param[in] cvbstride  size of a point in \a cvb (>=3, unchecked)
 * \param[in] cvb  second coordinate array
 *
 * \returns mean distance
 */
double
ay_nct_meandist(int cvlen, int cvastride, double *cva,
		int cvbstride, double *cvb)
{
 int a = 0, b = 0, t = 0, i;
 double v[3] = {0}, len = 0.0, tlen = 0.0;

  for(i = 0; i < cvlen; i++)
    {
      v[0] = cva[a]-cvb[b];
      v[1] = cva[a+1]-cvb[b+1];
      v[2] = cva[a+2]-cvb[b+2];

      if((fabs(v[0]) > AY_EPSILON) ||
	 (fabs(v[1]) > AY_EPSILON) ||
	 (fabs(v[2]) > AY_EPSILON))
	{
	  len = AY_V3LEN(v);
	  tlen += len;
	  t++;
	}
      a += cvastride;
      b += cvbstride;
    }

  if(t > 0)
    tlen /= t;

 return tlen;
} /* ay_nct_meandist */


/** ay_nct_shifttominmeandist:
 * Shift 1D control point array to minimum mean distance to a second array.
 *
 * \param[in] cvlen  number of points in \a cva _and_ \a cvb
 * \param[in] cvstride  size of a point in \a cva _and_ \a cvb (>=3, unchecked)
 * \param[in] cva  first coordinate array
 * \param[in,out] cvb  second coordinate array (may be shifted)
 *
 * \returns AY_OK on success, error code otherwise
 */
int
ay_nct_shifttominmeandist(int cvlen, int cvstride, double *cva, double *cvb)
{
 int t = 0, mint = 0, b = 0, i;
 double len, minlen = DBL_MAX;
 double *cvb2 = NULL;

  if(!(cvb2 = malloc(2*cvlen*cvstride*sizeof(double))))
    return AY_EOMEM;

  memcpy(cvb2, cvb, cvlen*cvstride*sizeof(double));
  memcpy(&(cvb2[cvlen*cvstride]), cvb, cvlen*cvstride*sizeof(double));

  for(i = 0; i < cvlen; i++)
    {
      len = ay_nct_meandist(cvlen, cvstride, cva, cvstride, &(cvb2[b]));
      if(len < minlen)
	{
	  minlen = len;
	  mint = t;
	}
      t++;
      b += cvstride;
    }

  memcpy(cvb, &(cvb2[mint*cvstride]), cvlen*cvstride*sizeof(double));

  free(cvb2);

 return AY_OK;
} /* ay_nct_shifttominmeandist */


/** ay_nct_rotatetominmeandist:
 * Rotate 1D control point array to minimum mean distance to a second array.
 * Todo: make this really computing a mean value and not just test the
 * first control point of each array.
 *
 * \param[in] cvlen  number of points in \a cva _and_ \a cvb
 * \param[in] cvstride  size of a point in \a cva _and_ \a cvb (>=3, unchecked)
 * \param[in] cva  first coordinate array
 * \param[in,out] cvb  second coordinate array (may be rotated)
 *
 * \returns AY_OK on success, error code otherwise
 */
int
ay_nct_rotatetominmeandist(int cvlen, int cvstride, double *cva, double *cvb)
{
 double minangle = 0.0, angle = 0.0, n[3], m[3], rm[16];
 double cvbt[3], vd[3];
 double len, minlen = DBL_MAX;
 int ay_status = AY_OK, i;

  ay_status = ay_geom_extractmiddlepoint(/*mode=*/0, cvb, cvlen, cvstride,
					 NULL, m);

  ay_status += ay_geom_extractmeannormal(cvb, cvlen, cvstride, m, n);

  if(!ay_status)
    {
      memcpy(cvbt, cvb, 3*sizeof(double));

      for(i = 0; i < 360; i++)
	{
	  AY_V3SUB(vd, cva, cvbt);
	  len = AY_V3LEN(vd);
	  if(len < minlen)
	    {
	      minlen = len;
	      minangle = angle;
	    }

	  angle += 1.0;
	  ay_trafo_identitymatrix(rm);
	  ay_trafo_translatematrix(m[0], m[1], m[2], rm);
	  ay_trafo_rotatematrix(angle, n[0], n[1], n[2], rm);
	  ay_trafo_translatematrix(-m[0], -m[1], -m[2], rm);
	  AY_APTRAN3(cvbt, cvb, rm);
	}

      if(minangle != 0.0)
	{
	  ay_trafo_translatematrix(m[0], m[1], m[2], rm);
	  ay_trafo_rotatematrix(minangle, n[0], n[1], n[2], rm);
	  ay_trafo_translatematrix(-m[0], -m[1], -m[2], rm);
	  ay_trafo_apply3v(cvb, cvlen, cvstride, rm);
	}
    }

 return ay_status;
} /* ay_nct_rotatetominmeandist */


/** ay_nct_getcvtangents:
 * Compute curve tangent vectors from control points.
 *
 * \param[in] nc  NURBS curve to process
 * \param[in,out] result  pointer where to store the tangents
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_getcvtangents(ay_nurbcurve_object *nc, double **result)
{
 int ay_status = AY_OK;
 ay_nct_gndcb *gndcb = ay_nct_gnd;
 double *tt, *p, *pp, *pn, v[3];
 int i;

  if(nc->type == AY_CTCLOSED)
    gndcb = ay_nct_gndc;
  else
    if(nc->type == AY_CTPERIODIC)
      gndcb = ay_nct_gndp;

  if(!(tt = malloc(nc->length*3*sizeof(double))))
    { ay_status = AY_EOMEM; goto cleanup; }

  p = nc->controlv;
  for(i = 0; i < nc->length; i++)
    {
      gndcb(1, nc, p, &pp);
      gndcb(0, nc, p, &pn);
      if(!pp || !pn)
	{
	  memset(&(tt[i*3]), 0, 3*sizeof(double));
	}
      else
	{
	  v[0] = pn[0]-pp[0];
	  v[1] = pn[1]-pp[1];
	  v[2] = pn[2]-pp[2];
	  memcpy(&(tt[i*3]), v, 3*sizeof(double));
	}
      p += 4;
    } /* for */

  *result = tt;

cleanup:
 return ay_status;
} /* ay_nct_getcvtangents */


/* ay_nct_gnd:
 * get address of next different control point in direction <dir>
 * in a open NURBS curve <nc> where the current points address is <p>;
 * <dir> must be one of:
 * 0 - Forward
 * 1 - Backward
 * returns result (new address) in <dp>
 * <dp> is set to NULL if there is no different point in the designated
 * direction (e.g. if the curve is degenerated)
 */
void
ay_nct_gnd(char dir, ay_nurbcurve_object *nc, double *p,
	   double **dp)
{
 int stride = 4;
 double *p2 = NULL;

  if(dir == AY_FORWARD)
    {
      if(p == &(nc->controlv[(nc->length-1)*stride]))
	{
	  *dp = NULL;
	  return;
	}
      p2 = p + stride;
      /* apply offset to p2 until p2 points to
	 a different control point than p
	 (in terms of their coordinate values)
	 or we run off the end of the curve */
      while(AY_V4COMP(p, p2))
	{
	  p2 += stride;

	  if(p2 == &(nc->controlv[(nc->length-1)*stride]))
	    {
	      *dp = NULL;
	      return;
	    }
	} /* while */
    }
  else
    {
      /* dir == AY_BACKWARD */
      if(p == nc->controlv)
	{
	  *dp = NULL;
	  return;
	}
      p2 = p - stride;
      /* apply offset to p2 until p2 points to
	 a different control point than p
	 (in terms of their coordinate values)
	 or we run off the start of the curve */
      while(AY_V4COMP(p, p2))
	{
	  p2 -= stride;
	  if(p2 == nc->controlv)
	    {
	      *dp = NULL;
	      return;
	    }
	} /* while */
    } /* if */

  *dp = p2;

 return;
} /* ay_nct_gnd */


/* ay_nct_gndc:
 * get address of next different control point in direction <dir>
 * in a closed NURBS curve <nc> where the current points address is <p>;
 * <dir> must be one of:
 * 0 - Forward
 * 1 - Backward
 * returns result (new address) in <dp>
 * <dp> is set to NULL if there is no different point in the designated
 * direction (e.g. if the curve is degenerated)
 */
void
ay_nct_gndc(char dir, ay_nurbcurve_object *nc, double *p,
	    double **dp)
{
 int stride = 4;
 double *p2 = NULL;

  if(dir == AY_FORWARD)
    {
      if(p == &(nc->controlv[(nc->length-1)*stride]))
	{
	  /* wrap around */
	  p2 = &(nc->controlv[stride]);
	}
      else
	{
	  p2 = p + stride;
	}

      /* apply offset to p2 until p2 points to
	 a different control point than p
	 (in terms of their coordinate values)
	 or we get to p again or we run off the end of the curve */
      while(AY_V4COMP(p, p2))
	{
	  p2 += stride;
	  /* check degeneracy or 2nd wrap */
	  if(p == p2 || p2 == &(nc->controlv[(nc->length-2)*stride]))
	    {
	      *dp = NULL;
	      return;
	    }
	} /* while */
    }
  else
    {
      /* dir == AY_BACKWARD */
      if(p == nc->controlv)
	{
	  /* wrap around */
	  p2 = &(nc->controlv[(nc->length-1)*stride]);
	}
      else
	{
	  p2 = p - stride;
	}

      /* apply offset to p2 until p2 points to
	 a different control point than p
	 (in terms of their coordinate values)
	 or we get to p again or we run off the start of the curve */
      while(AY_V4COMP(p, p2))
	{
	  p2 -= stride;
	  /* check degeneracy or 2nd wrap */
	  if(p == p2 || p2 == nc->controlv)
	    {
	      *dp = NULL;
	      return;
	    }
	} /* while */
    } /* if */

  *dp = p2;

 return;
} /* ay_nct_gndc */


/* ay_nct_gndp:
 * get address of next different control point in direction <dir>
 * in a periodic NURBS curve <nc> where the current points address is <p>;
 * <dir> must be one of:
 * 0 - Forward
 * 1 - Backward
 * returns result (new address) in <dp>
 * <dp> is set to NULL if there is no different point in the designated
 * direction (e.g. if the curve is degenerated)
 */
void
ay_nct_gndp(char dir, ay_nurbcurve_object *nc, double *p,
	    double **dp)
{
 int stride = 4;
 double *p2 = NULL;

  if(dir == AY_FORWARD)
    {
      if(p == &(nc->controlv[(nc->length-1)*stride]))
	{
	  /* wrap around */
	  p2 = &(nc->controlv[nc->order*stride]);
	}
      else
	{
	  p2 = p + stride;
	}
      /* apply offset to p2 until p2 points to
	 a different control point than p
	 (in terms of their coordinate values)
	 or we get to p again or we run off the end of the curve */
      while(AY_V4COMP(p, p2))
	{
	  p2 += stride;
	  /* check degeneracy or 2nd wrap */
	  if(p == p2 || p == &(nc->controlv[(nc->length-1)*stride]))
	    {
	      *dp = NULL;
	      return;
	    }
	} /* while */
    }
  else
    {
      /* dir == AY_BACKWARD */
      if(p == nc->controlv)
	{
	  /* wrap around */
	  p2 = &(nc->controlv[(nc->length-nc->order-1)*stride]);
	}
      else
	{
	  p2 = p - stride;
	}
      /* apply offset to p2 until p2 points to
	 a different control point than p
	 (in terms of their coordinate values)
	 or we get to p again or we run off the start of the curve */
      while(AY_V4COMP(p, p2))
	{
	  p2 -= stride;
	  /* check degeneracy or 2nd wrap */
	  if(p == p2 || p2 == nc->controlv)
	    {
	      *dp = NULL;
	      return;
	    }
	} /* while */
    } /* if */

  *dp = p2;

 return;
} /* ay_nct_gndp */


/** ay_nct_computebreakpoints:
 * Calculate the 3D positions on the curve that correspond to all
 * distinct knot values (the break points) and put these together
 * with the respective knot values into a special array.
 *
 * \param[in,out] ncurve  NURBS curve to process
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_computebreakpoints(ay_nurbcurve_object *ncurve)
{
 size_t breakvlen;
 int i, usememopt = AY_FALSE;
 double *p, u, lastu;

  if(ncurve->order < 6)
    usememopt = AY_TRUE;

  if(ncurve->breakv)
    free(ncurve->breakv);

  /* calculate size of breakpoint array */
  if(usememopt)
    {
      breakvlen = ((ncurve->length + ncurve->order) * 4 + ncurve->order * 3) *
	sizeof(double);
    }
  else
    {
      breakvlen = (ncurve->length + ncurve->order) * 4 * sizeof(double);
    }
  /* add space for one double (size slot) */
  breakvlen += sizeof(double);

  if(!(ncurve->breakv = malloc(breakvlen)))
    return AY_EOMEM;

  p = &(ncurve->breakv[1]);
  lastu = ncurve->knotv[ncurve->order-1]-1.0;
  for(i = ncurve->order-1; i <= ncurve->length; i++)
    {
      u = ncurve->knotv[i];
      if(fabs(u - lastu) > AY_EPSILON)
	{
	  lastu = u;
	  if(usememopt)
	    if(ncurve->is_rat)
	      ay_nb_CurvePoint4DM(ncurve->length-1, ncurve->order-1,
				  ncurve->knotv, ncurve->controlv, u, p);
	    else
	      ay_nb_CurvePoint3DM(ncurve->length-1, ncurve->order-1,
				  ncurve->knotv, ncurve->controlv, u, p);
	  else
	    if(ncurve->is_rat)
	      ay_nb_CurvePoint4D(ncurve->length-1, ncurve->order-1,
				 ncurve->knotv, ncurve->controlv, u, p);
	    else
	      ay_nb_CurvePoint3D(ncurve->length-1, ncurve->order-1,
				 ncurve->knotv, ncurve->controlv, u, p);
	  p[3] = u;
	  p += 4;
	} /* if u is distinct */
    } /* for all knots */

  p[3] = ncurve->knotv[ncurve->length]+1;

  /* set size slot */
  ncurve->breakv[0] = (p - &(ncurve->breakv[1]))/4;
  /*printf("bps: %d\n",(int)ncurve->breakv[0]);*/
 return AY_OK;
} /* ay_nct_computebreakpoints */


/** ay_nct_drawbreakpoints:
 * draw the break points (distinct knots) of the curve;
 * if there are no break points currently stored for the curve, they
 * will be computed via ay_nct_computebreakpoints() above
 *
 * \param[in,out] o  NURBS curve object to draw
 */
void
ay_nct_drawbreakpoints(ay_nurbcurve_object *ncurve)
{
 double *cv, c[3], dx[3], dy[3];
 GLdouble mvm[16], pm[16], s;
 GLint vp[4];

  if(!ncurve->breakv)
    (void)ay_nct_computebreakpoints(ncurve);

  if(ncurve->breakv)
    {
      cv = &((ncurve->breakv)[1]);
      s = ay_prefs.handle_size*0.75;

      glGetDoublev(GL_MODELVIEW_MATRIX, mvm);
      glGetDoublev(GL_PROJECTION_MATRIX, pm);
      glGetIntegerv(GL_VIEWPORT, vp);

      gluProject(0, 0, 0, mvm, pm, vp, &c[0], &c[1], &c[2]);

      gluUnProject(c[0]+s, c[1], c[2], mvm, pm, vp,
		   &dx[0], &dx[1], &dx[2]);
      gluUnProject(c[0], c[1]+s, c[2], mvm, pm, vp,
		   &dy[0], &dy[1], &dy[2]);

      glBegin(GL_QUADS);
       do
	 {
	   glVertex3d(cv[0]+dx[0], cv[1]+dx[1], cv[2]+dx[2]);
	   glVertex3d(cv[0]+dy[0], cv[1]+dy[1], cv[2]+dy[2]);
	   glVertex3d(cv[0]-dx[0], cv[1]-dx[1], cv[2]-dx[2]);
	   glVertex3d(cv[0]-dy[0], cv[1]-dy[1], cv[2]-dy[2]);
	   cv += 4;
	 }
       while(cv[3] <= ncurve->knotv[ncurve->length]);
      glEnd();
    } /* if */

 return;
} /* ay_nct_drawbreakpoints */


/** ay_nct_colorfromwweight:
 * Set the current OpenGL color from a weight value.
 *
 * \param[in] w  weight value
 */
void
ay_nct_colorfromweight(double w)
{
 float rgb[3] = {1.0f,1.0f,1.0f};

  if(w == 1.0)
    {
      glColor3fv(rgb);
      return;
    }

  if(fabs(w) < AY_EPSILON || (w != w))
    {
      /* zero and NaN weights => black */
      glColor3ub(0,0,0);
    }
  else
    {
      if(w > 0.0)
	{
	  if(w > 1.0)
	    {
	      if(w > 1.1)
		{
		  /* attracting weight => pure red */
		  rgb[0] = AY_MAX(0.25f, (3.0f-(float)w)*0.75f);
		  rgb[1] = 0.0f;
		}
	      else
		{
		  /* small attracting weight (1.0-1.1) */
		  rgb[1] = 0.25f+(1.1f-(float)w)*7.5f;
		}
	      rgb[2] = rgb[1];
	    }
	  else
	    {
	      if(w < 0.9)
		{
		  /* repelling weight => pure blue */
		  rgb[2] = AY_MAX(0.25f, (float)w+0.1f);
		  rgb[0] = 0.0f;
		}
	      else
		{
		  /* small repelling weight (0.9-1.0) */
		  rgb[0] = 0.25f+((float)w-0.9f)*7.5f;
		}
	      rgb[1] = rgb[0];
	    }
	}
      else
	{
	  /* negative weight > green */
	  rgb[0] = 0.0f;
	  rgb[1] = 0.75f;
	  rgb[2] = 0.0f;
	  /*
	  if(w < -1.0)
	    {
	      rgb[1] = 1.0f;
	    }
	  else
	    {
	      rgb[1] =  0.25f+((float)fabs(w)-0.9f)*7.5f;
	    }
	  */
	}

      glColor3fv(rgb);
    } /* weight is healthy */

 return;
} /* ay_nct_colorfromweight */


/** ay_nct_extractnc:
 * Extract a subcurve from a NURBS curve.
 *
 * \param[in] src  curve to extract from
 * \param[in] umin  parametric value of first point of extracted curve
 * \param[in] umax  parametric value of last point of extracted curve
 * \param[in] relative  whether to interpret \a umin and \a umax in
 *                      a relative way (must be in range 0.0-1.0)
 * \param[in,out] result  where to store the extracted subcurve
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_extractnc(ay_object *src, double umin, double umax, int relative,
		 ay_object **result)
{
 int ay_status = AY_OK;
 ay_object *copy = NULL, *nc1 = NULL, *nc2 = NULL;
 ay_nurbcurve_object *curve = NULL;
 char fname[] = "nct_extractnc", split_errmsg[] = "Split failed.";
 double uv, uvmin, uvmax;

  if(!src || !result)
    return AY_ENULL;

  /* check parameters */
  if((umin + AY_EPSILON) > umax)
    {
      ay_error(AY_ERROR, fname, "Parameter min must be smaller than max.");
      return AY_ERROR;
    }

  if(src->type != AY_IDNCURVE)
    {
      ay_error(AY_EWTYPE, fname, ay_nct_ncname);
      return AY_ERROR;
    }
  else
    {
      ay_status = ay_object_copy(src, &copy);
      if(ay_status || !copy)
	{
	  return AY_ERROR;
	}
      curve = (ay_nurbcurve_object*)copy->refine;

      if(relative)
	{
	  uvmin = curve->knotv[curve->order-1];
	  uvmax = curve->knotv[curve->length];
	  uv = uvmin + ((uvmax - uvmin) * umin);
	  umin = uv;
	  uv = uvmin + ((uvmax - uvmin) * umax);
	  umax = uv;
	}

      /* check parameters */
      if((umin < curve->knotv[curve->order-1]) ||
	 (umax > curve->knotv[curve->length]))
	{
	  (void)ay_error_reportdrange(fname, "\"umin/umax\"",
				      curve->knotv[curve->order-1],
				      curve->knotv[curve->length]);
	  ay_status = AY_ERROR;
	  goto cleanup;
	}

      /* split off intervals outside umin/umax */
      /* note that this approach supports e.g. umin to be exactly
	 knotv[curve->order-1]
	 and in this case does not execute the (unneeded) split */
      if(umin > curve->knotv[curve->order-1])
	{
	  ay_status = ay_nct_split(copy, umin, AY_FALSE, &nc1);
	  if(ay_status)
	    {
	      ay_error(AY_ERROR, fname, split_errmsg);
	      goto cleanup;
	    }
	  /* <nc1> is the sub curve we want
	     => remove <copy>; then move <nc1> to <copy> */
	  if(nc1)
	    {
	      nc2 = copy;
	      copy = nc1;
	      (void)ay_object_delete(nc2);
	      nc1 = NULL;
	      curve = (ay_nurbcurve_object*)copy->refine;
	    }
	}
      if(umax < curve->knotv[curve->length])
	{
	  ay_status = ay_nct_split(copy, umax, AY_FALSE, &nc1);
	  if(ay_status)
	    {
	      ay_error(AY_ERROR, fname, split_errmsg);
	      goto cleanup;
	    }
	  /* <copy> is the sub curve we want
	     => remove <nc1> */
	  if(nc1)
	    {
	      (void)ay_object_delete(nc1);
	      nc1 = NULL;
	    }
	}

      /* return result */
      *result = copy;
      copy = NULL;
    } /* if is NCurve */

cleanup:

  if(copy)
    {
      ay_object_delete(copy);
    }

 return ay_status;
} /* ay_nct_extractnc */


/** ay_nct_extractnctcmd:
 *  Extract a sub curve from the selected NURBS curves.
 *  Implements the \a extrNC scripting interface command.
 *  See also the corresponding section in the \ayd{scextrnc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_extractnctcmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_object *o, *new = NULL, *pobject = NULL;
 double umin = 0.0, umax = 0.0;
 int i = 1, relative = AY_FALSE;
 char fargs[] = "[-relative] umin umax";

  if(argc < 3)
    {
      ay_error(AY_EARGS, argv[0], fargs);
      return TCL_OK;
    }

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  if(argv[i][0] == '-' && argv[i][1] == 'r')
    {
      relative = AY_TRUE;
      i++;
    }

  if(i+1 >= argc)
    {
      ay_error(AY_EARGS, argv[0], fargs);
      return TCL_OK;
    }

  tcl_status = Tcl_GetDouble(interp, argv[i], &umin);
  AY_CHTCLERRRET(tcl_status, argv[0], interp);
  if(umin != umin)
    {
      ay_error_reportnan(argv[0], "umin");
      return TCL_OK;
    }
  tcl_status = Tcl_GetDouble(interp, argv[i+1], &umax);
  AY_CHTCLERRRET(tcl_status, argv[0], interp);
  if(umax != umax)
    {
      ay_error_reportnan(argv[0], "umax");
      return TCL_OK;
    }

  while(sel)
    {
      o = sel->object;
      if(o->type != AY_IDNCURVE)
	{
	  pobject = ay_peek_singleobject(o, AY_IDNCURVE);

	  if(!pobject)
	    {
	      ay_error(AY_EWARN, argv[0], ay_error_igntype);
	      o = NULL;
	    }
	  else
	    {
	      o = pobject;
	    } /* if */
	} /* if */

      if(o)
	{
	  new = NULL;

	  ay_status = ay_nct_extractnc(o, umin, umax, relative, &new);

	  if(ay_status)
	    {
	      ay_error(ay_status, argv[0], NULL);
	      goto cleanup;
	    }

	  ay_object_link(new);
	} /* if */

      sel = sel->next;
    } /* while */

  (void)ay_notify_parent();

cleanup:

  if(pobject)
    (void)ay_object_deletemulti(pobject, AY_FALSE);

 return TCL_OK;
} /* ay_nct_extractnctcmd */


/** ay_nct_fair:
 *  make the shape of a curve more pleasant:
 *  change the control points of a NURBS curve so that the curvature
 *  is more evenly distributed
 *
 *  Does not maintain close state or multiple points!
 *
 * \param[in,out] curve  NURBS curve object to process
 * \param[in] selp  selected points (may be NULL)
 * \param[in] tol  maximum distance a control point moves
 * \param[in] fair_worst  if AY_TRUE, only the control point that would move
 *                        the farthest distance will be moved
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_nct_fair(ay_nurbcurve_object *curve, ay_point *selp, double tol,
	    int fair_worst)
{
 int ay_status = AY_OK;
 unsigned int i, j, stride = 4, found;
 double *Pw, *U, len, v[3], l[3], r[3], q[4] = {0,0,0,1};
 double *pll, *pl, *p, *pr, *prr, worstlen = 0.0;
 ay_point *pnt, worstpnt = {0};
 ay_nurbcurve_object nc = {0};

  /* sanity check */
  if(!curve)
    return AY_ENULL;

  if(curve->length < 3)
    return AY_OK;

  if(curve->order < 3)
    return AY_OK;

  if(!(Pw = malloc((curve->length+4)*stride*sizeof(double))))
    return AY_EOMEM;

  memcpy(&(Pw[2*stride]), curve->controlv, curve->length*stride*sizeof(double));

  switch(curve->type)
    {
    case AY_CTOPEN:
      pll = curve->controlv;
      pl = pll+stride;
      AY_V3SUB(l, pl, pll);
      AY_V3SCAL(l, 0.5);
      AY_V3SUB(q, pll, l);
      memcpy(&(Pw[stride]), q, stride*sizeof(double));
      pll += stride;
      pl += stride;
      AY_V3SUB(l, pl, pll);
      AY_V3SCAL(l, 0.5);
      AY_V3SUB(q, q, l);
      memcpy(&(Pw[0]), q, stride*sizeof(double));

      prr = &(curve->controlv[(curve->length-1)*stride]);
      pr = prr-stride;
      AY_V3SUB(r, pr, prr);
      AY_V3SCAL(r, 0.5);
      AY_V3SUB(q, prr, r);
      memcpy(&(Pw[(curve->length+2)*stride]), q, stride*sizeof(double));
      prr -= stride;
      pr -= stride;
      AY_V3SUB(r, pr, prr);
      AY_V3SCAL(r, 0.5);
      AY_V3SUB(q, q, r);
      memcpy(&(Pw[(curve->length+3)*stride]), q, stride*sizeof(double));
      break;
    case AY_CTCLOSED:
      memcpy(Pw,
	     &(curve->controlv[(curve->length-3)*stride]),
	     stride*sizeof(double));
      memcpy(&(Pw[stride]),
	     &(curve->controlv[(curve->length-2)*stride]),
	     stride*sizeof(double));

      memcpy(&(Pw[(curve->length+2)*stride]),
	     &(curve->controlv[stride]),
	     stride*sizeof(double));
      memcpy(&(Pw[(curve->length+3)*stride]),
	     &(curve->controlv[2*stride]),
	     stride*sizeof(double));
      break;
    case AY_CTPERIODIC:
      memcpy(Pw,
	     &(curve->controlv[(curve->length-(curve->order+1))*stride]),
	     stride*sizeof(double));
      memcpy(&(Pw[stride]),
	     &(curve->controlv[(curve->length-(curve->order))*stride]),
	     stride*sizeof(double));

      memcpy(&(Pw[(curve->length+2)*stride]),
	     &(curve->controlv[(curve->order-1)*stride]),
	     stride*sizeof(double));
      memcpy(&(Pw[(curve->length+3)*stride]),
	     &(curve->controlv[curve->order*stride]),
	     stride*sizeof(double));
      break;
    default:
      break;
    } /* switch type */

  if(curve->knot_type != AY_KTCUSTOM)
    {
      nc.length = curve->length+4;
      nc.order = curve->order;
      nc.type = curve->type;
      nc.knot_type = curve->knot_type;
      ay_status = ay_knots_createnc(&nc);
      if(ay_status)
	{
	  free(Pw);
	  return AY_ERROR;
	}
      U = nc.knotv;
    }
  else
    {
      if(!(U = malloc((curve->length+curve->order+4)*sizeof(double))))
	{ free(Pw); return AY_EOMEM; }

      memcpy(&(U[2]), curve->knotv,
	     (curve->length+curve->order)*sizeof(double));

      i = curve->order;
      U[i-1] = U[i]-(U[i+1]-U[i])*0.5;
      U[i-2] = U[i+1]-(U[i+2]-U[i+1])*0.5;

      i = curve->length;
      U[i+1] = U[i]+(U[i]-U[i-1])*0.5;
      U[i+2] = U[i-1]+(U[i-1]-U[i-2])*0.5;

      U[curve->length+curve->order] = U[curve->length+curve->order-1];
    }

  pll = Pw;
  pl = pll+stride;
  p = pl+stride;
  pr = p+stride;
  prr = pr+stride;
  j = curve->order;

  for(i = 2; i < (unsigned int)curve->length+2; i++)
    {
      if(selp)
	{
	  found = AY_FALSE;
	  pnt = selp;
	  while(pnt)
	    {
	      if(pnt->index+2 == i)
		{
		  found = AY_TRUE;
		  break;
		}
	      pnt = pnt->next;
	    }
	}

      if(!selp || found)
	{
	  l[0] = ((U[j+1]-U[j-3])*pl[0] - (U[j+1]-U[j])*pll[0]) / (U[j]-U[j-3]);
	  l[1] = ((U[j+1]-U[j-3])*pl[1] - (U[j+1]-U[j])*pll[1]) / (U[j]-U[j-3]);
	  l[2] = ((U[j+1]-U[j-3])*pl[2] - (U[j+1]-U[j])*pll[2]) / (U[j]-U[j-3]);

	  r[0] = ((U[j+3]-U[j-1])*pr[0] - (U[j]-U[j-1])*prr[0]) / (U[j+3]-U[j]);
	  r[1] = ((U[j+3]-U[j-1])*pr[1] - (U[j]-U[j-1])*prr[1]) / (U[j+3]-U[j]);
	  r[2] = ((U[j+3]-U[j-1])*pr[2] - (U[j]-U[j-1])*prr[2]) / (U[j+3]-U[j]);

	  q[0] = ((U[j+2]-U[j])*l[0] + (U[j]-U[j-2])*r[0]) / (U[j+2]-U[j-2]);
	  q[1] = ((U[j+2]-U[j])*l[1] + (U[j]-U[j-2])*r[1]) / (U[j+2]-U[j-2]);
	  q[2] = ((U[j+2]-U[j])*l[2] + (U[j]-U[j-2])*r[2]) / (U[j+2]-U[j-2]);

	  AY_V3SUB(v, p, q);
	  len = AY_V3LEN(v);
	  if(len > AY_EPSILON)
	    {
	      if(fair_worst)
		{
		  if(len > worstlen)
		    {
		      worstlen = len;
		      worstpnt.index = i-2;
		    }
		}
	      else
		{
		  if(len > tol)
		    {
		      AY_V3SCAL(v, tol);
		      AY_V3ADD(p, p, v);
		    }
		  else
		    {
		      memcpy(p, q, 3*sizeof(double));
		    }
		} /* if worst */
	    } /* if have len */
	} /* if */

      pll += stride;
      pl += stride;
      p += stride;
      pr += stride;
      prr += stride;
      j++;
    } /* for */

  if(fair_worst)
    {
      ay_status = ay_nct_fair(curve, &worstpnt, tol, /*fair_worst=*/AY_FALSE);
    }
  else
    {
      memcpy(curve->controlv, &(Pw[2*stride]),
	     curve->length*stride*sizeof(double));
    }

  free(U);
  free(Pw);

 return ay_status;
} /* ay_nct_fair */


/** ay_nct_fairnctcmd:
 *  make the shape of a curve more pleasant:
 *  change the control points of a NURBS curve so that the curvature
 *  is more evenly distributed
 *  Implements the \a fairNC scripting interface command.
 *  See also the corresponding section in the \ayd{scfairnc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_nct_fairnctcmd(ClientData clientData, Tcl_Interp *interp,
		  int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_object *o;
 ay_nurbcurve_object *nc;
 double tol = DBL_MAX;
 int i = 1;
 int notify_parent = AY_FALSE;
 int fair_worst = AY_FALSE;

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  if(argc > 1)
    {
      if(argv[1][0] == '-' && argv[1][1] == 'w')
	{
	  fair_worst = AY_TRUE;
	  i++;
	}

      tcl_status = Tcl_GetDouble(interp, argv[i], &tol);
      AY_CHTCLERRRET(tcl_status, argv[0], interp);
      if(tol != tol)
	{
	  ay_error_reportnan(argv[0], "tol");
	  return TCL_OK;
	}
      if(tol <= 0)
	{
	  ay_error(AY_ERROR, argv[0], "Argument tol must be > 0.");
	  return TCL_OK;
	}
    }

  while(sel)
    {
      o = sel->object;
      if(o->type == AY_IDNCURVE)
	{
	  nc = (ay_nurbcurve_object *)o->refine;

	  ay_status = ay_nct_fair(nc, o->selp, tol, fair_worst);

	  if(ay_status)
	    {
	      ay_error(ay_status, argv[0], "Fairing failed.");
	      return TCL_OK;
	    }

	  if(nc->type != AY_CTOPEN)
	    {
	      ay_status = ay_nct_close(nc);

	      if(ay_status)
		ay_error(AY_ERROR, argv[0], "Could not close curve.");
	    }

	  if(nc->mpoints)
	    ay_nct_recreatemp(nc);

	  (void)ay_notify_object(o);
	  notify_parent = AY_TRUE;
	}
      else
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	}

      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_fairnctcmd */

#if 0
int
ay_nct_expanddisc(double **p, int *plen)
{
 int stride = 4, i, j, *d = NULL, newlen;
 double *p1, *p2, *newp;

  if(!(d = malloc((*plen)/2*sizeof(int))))
    return AY_EOMEM;

  p1 = *p;
  p2 = *p+stride;
  j = 0;
  newlen = *plen;
  for(i = 0; i < (*plen)-1; i++)
   {
     if(AY_V3COMP(p1, p2))
       {
	 if(j)
	   d[j] = i-d[j-1];
	 else
	   d[j] = i;
	 newlen++;
	 j++;
       }
     p1 = p2;
     p2 += stride;
   }

  if(newlen > *plen)
    {
      if(!(newp = calloc(newlen, stride*sizeof(int))))
	{ free(d); return AY_EOMEM; }
      p1 = newp;
      p2 = *p;
      for(i = 0; i < (*plen-newlen); i++)
	{
	  memcpy(p1, p2, d[i]*stride*sizeof(double));
	  p1 += d[i]*stride;
	  p2 += d[i]*stride;
	  memcpy(p1, p2, stride*sizeof(double));
	  p1 += stride;
	}

      free(*p);
      *p = newp;
      *plen = newlen;
    }

 return AY_OK;
} /* ay_nct_expanddisc */
#endif

/* templates */
#if 0

/* Tcl command */

/* ay_nct_xxxxtcmd:
 *
 */
int
ay_nct_xxxxtcmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_nurbcurve_object *curve;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL;
 int notify_parent = AY_FALSE;

  /* parse args */
  if(argc < 3)
    {
      ay_error(AY_EARGS, argv[0], "u r");
      return TCL_OK;
    }

  tcl_status = Tcl_GetDouble(interp, argv[1], &u);
  AY_CHTCLERRRET(tcl_status, argv[0], interp);
  tcl_status = Tcl_GetInt(interp, argv[2], &r);
  AY_CHTCLERRRET(tcl_status, argv[0], interp);

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  while(sel)
    {
      o = sel->object;
      if(o->type != AY_IDNCURVE)
	{
	  ay_error(AY_EWARN, argv[0], ay_error_igntype);
	}
      else
	{
	  curve = (ay_nurbcurve_object *)o->refine;

	  /* do magic */

	  /* clean up */
	  ay_nct_recreatemp(curve);

	  if(o->selp)
	    {
	      (void)ay_pact_getpoint(3, o, NULL, NULL);
	    }
	  /* or */
	  ay_selp_clear(o);

	  /* show ay_notify_parent() the changed objects */
	  o->modified = AY_TRUE;

	  /* re-create tesselation of curve */
	  (void)ay_notify_object(o);

	  notify_parent = AY_TRUE;
	} /* if */

      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_nct_xxxxtcmd */

#endif
