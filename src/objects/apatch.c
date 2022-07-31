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

/* apatch.c -  approximating patch object */

static char *ay_apatch_name = "APatch";

void ay_apatch_drawcp(ay_apatch_object *apatch);

int ay_apatch_notifycb(ay_object *o);

/* functions: */

/* ay_apatch_getkt:
 * Helper function to translate the knot type to AY_KT* constants.
 */
int
ay_apatch_getkt(int akt)
{
 int kt = 0;

  switch(akt)
    {
    case 0:
      kt = AY_KTCHORDAL;
      break;
    case 1:
      kt = AY_KTCENTRI;
      break;
    case 2:
      kt = AY_KTBEZIER;
      break;
    case 3:
      kt = AY_KTBSPLINE;
      break;
    case 4:
      kt = AY_KTNURB;
      break;
    default:
      /* output error? */
      break;
    }

 return kt;
} /* ay_apatch_getkt */


/* ay_apatch_createcb:
 *  create callback function of apatch object
 */
int
ay_apatch_createcb(int argc, char *argv[], ay_object *o)
{
 int ay_status = AY_OK;
 int tcl_status = TCL_OK;
 char fname[] = "crtapatch";
 char option_handled = AY_FALSE;
 int mode = 0, center = AY_FALSE, createmp = -1;
 int stride = 3, uorder = 3, vorder = 3, width = 4, height = 4;
 int awidth = 3, aheight = 3;
 int uclosed = AY_FALSE, vclosed = AY_FALSE;
 int ukt = 0, vkt = 0, optnum = 0, i = 2, j = 0, k = 0;
 int acvlen = 0;
 char **acv = NULL;
 double *cv = NULL;
 double udx = 0.25, udy = 0.0, udz = 0.0;
 double vdx = 0.0, vdy = 0.25, vdz = 0.0;
 double ext = 0.0, s[3] = {0};
 ay_apatch_object *ap = NULL;

  if(!argv || !o)
    return AY_ENULL;

  /* parse args */
  while(i < argc)
    {
      if(i+1 >= argc)
	{
	  ay_error(AY_EOPT, fname, argv[i]);
	  ay_status = AY_ERROR;
	  goto cleanup;
	}

      tcl_status = TCL_OK;
      option_handled = AY_FALSE;
      optnum = i;
      if(argv[i] && argv[i][0] != '\0')
	{
	  switch(argv[i][1])
	    {
	    case 'a':
	      switch(argv[i][2])
		{
		case 'w':
		  /* -awidth */
		  tcl_status = Tcl_GetInt(ay_interp, argv[i+1], &awidth);
		  option_handled = AY_TRUE;
		  break;
		case 'h':
		  /* -aheight */
		  tcl_status = Tcl_GetInt(ay_interp, argv[i+1], &aheight);
		  option_handled = AY_TRUE;
		  break;
		default:
		  break;
		}
	      break;
	    case 'w':
	      /* -width */
	      tcl_status = Tcl_GetInt(ay_interp, argv[i+1], &width);
	      option_handled = AY_TRUE;
	      break;
	    case 'h':
	      /* -height */
	      tcl_status = Tcl_GetInt(ay_interp, argv[i+1], &height);
	      option_handled = AY_TRUE;
	      break;
	    case 'u':
	      switch(argv[i][2])
		{
		case 'c':
		  /* -uclosed */
		  tcl_status = Tcl_GetInt(ay_interp, argv[i+1], &uclosed);
		  option_handled = AY_TRUE;
		  break;
		case 'd':
		  switch(argv[i][3])
		    {
		    case 'x':
		      /* -udx */
		      tcl_status = Tcl_GetDouble(ay_interp, argv[i+1], &udx);
		      option_handled = AY_TRUE;
		      break;
		    case 'y':
		      /* -udy */
		      tcl_status = Tcl_GetDouble(ay_interp, argv[i+1], &udy);
		      option_handled = AY_TRUE;
		      break;
		    case 'z':
		      /* -udz */
		      tcl_status = Tcl_GetDouble(ay_interp, argv[i+1], &udz);
		      option_handled = AY_TRUE;
		      break;
		    default:
		      break;
		    } /* switch */
		  break;
		case 'o':
		  /* -uorder */
		  tcl_status = Tcl_GetInt(ay_interp, argv[i+1], &uorder);
		  option_handled = AY_TRUE;
		  break;
		case 'k':
		  /* -uktype */
		  tcl_status = Tcl_GetInt(ay_interp, argv[i+1], &ukt);
		  option_handled = AY_TRUE;
		  break;
		default:
		  break;
		} /* switch */
	      break;
	    case 'v':
	      switch(argv[i][2])
		{
		case 'c':
		  /* -vclosed */
		  tcl_status = Tcl_GetInt(ay_interp, argv[i+1], &vclosed);
		  option_handled = AY_TRUE;
		  break;
		case 'd':
		  switch(argv[i][3])
		    {
		    case 'x':
		      /* -vdx */
		      tcl_status = Tcl_GetDouble(ay_interp, argv[i+1], &vdx);
		      option_handled = AY_TRUE;
		      break;
		    case 'y':
		      /* -vdy */
		      tcl_status = Tcl_GetDouble(ay_interp, argv[i+1], &vdy);
		      option_handled = AY_TRUE;
		      break;
		    case 'z':
		      /* -vdz */
		      tcl_status = Tcl_GetDouble(ay_interp, argv[i+1], &vdz);
		      option_handled = AY_TRUE;
		      break;
		    default:
		      break;
		    } /* switch */
		  break;
		case 'o':
		  /* -vorder */
		  tcl_status = Tcl_GetInt(ay_interp, argv[i+1], &vorder);
		  option_handled = AY_TRUE;
		  break;
		case 'k':
		  /* -vktype */
		  tcl_status = Tcl_GetInt(ay_interp, argv[i+1], &vkt);
		  option_handled = AY_TRUE;
		  break;
		default:
		  break;
		} /* switch */
	      break;
	    case 'c':
	      switch(argv[i][2])
		{
		case 'n':
		  /* -cn */
		  if(cv)
		    {
		      free(cv);
		      cv = NULL;
		    }
		  tcl_status = ay_tcmd_convdlist(ay_interp, argv[i+1],
						 &acvlen, &cv);
		  option_handled = AY_TRUE;
		  break;
		case 'v':
		  /* -cv */
		  if(Tcl_SplitList(ay_interp, argv[i+1], &acvlen, &acv) ==
		     TCL_OK)
		    {
		      if(cv)
			{
			  free(cv);
			}
		      if(!(cv = calloc(acvlen, sizeof(double))))
			{
			  Tcl_Free((char *) acv);
			  ay_status = AY_EOMEM;
			  goto cleanup;
			}
		      for(j = 0; j < acvlen; j++)
			{
			  tcl_status = Tcl_GetDouble(ay_interp,
						     acv[j], &cv[j]);
			  if(tcl_status != TCL_OK)
			    {
			      break;
			    }
			} /* for */
		      Tcl_Free((char *) acv);
		    }
		  option_handled = AY_TRUE;
		  break;
		case 'e':
		  /* -center */
		  tcl_status = Tcl_GetInt(ay_interp, argv[i+1], &center);
		  option_handled = AY_TRUE;
		  break;
		case 'r':
		  /* -createmp */
		  tcl_status = Tcl_GetBoolean(ay_interp, argv[i+1], &createmp);
		  option_handled = AY_TRUE;
		  break;
		default:
		  break;
		} /* switch */
	      break;
	    case 'm':
		  tcl_status = Tcl_GetInt(ay_interp, argv[i+1], &mode);
		  option_handled = AY_TRUE;
	      break;
	    default:
	      break;
	    } /* switch */

	  if(option_handled && (tcl_status != TCL_OK))
	    {
	      ay_error(AY_EOPT, fname, argv[i]);
	      ay_status = AY_ERROR;
	      goto cleanup;
	    }

	  i += 2;
	}
      else
	{
	  i++;
	} /* if */

      if(!option_handled)
	{
	  ay_error(AY_EUOPT, fname, argv[optnum]);
	  ay_status = AY_ERROR;
	  goto cleanup;
	}

    } /* while */

  /* check and correct awidth/aheight/width/height/uorder/vorder */
  if(width <= 1)
    width = 4;

  if(awidth < 2)
    awidth = 2;

  if(awidth > width)
    awidth = width;

  if(uorder < 2)
    uorder = 2;

  if(uorder > awidth)
    uorder = (awidth < 4)?awidth:4;

  if(height <= 1)
    height = 4;

  if(aheight < 2)
    aheight = 2;

  if(aheight > height)
    aheight = height;

  if(vorder < 2)
    vorder = 2;

  if(vorder > aheight)
    vorder = (aheight < 4)?aheight:4;

  if(cv)
    {
      /* check length of user provided control point array */
      if(acvlen/stride < width*height)
	{
	  if(acvlen > 0)
	    s[0] = cv[0];
	  if(acvlen > 1)
	    s[1] = cv[1];
	  if(acvlen > 2)
	    s[2] = cv[2];

	  free(cv);
	  cv = NULL;
	  center = AY_FALSE;
	}
    } /* if */


  /* now create the apatch */
  if(!(ap = calloc(1, sizeof(ay_apatch_object))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  ap->mode = mode;
  ap->awidth = awidth;
  ap->aheight = aheight;
  ap->width = width;
  ap->height = height;
  ap->order_u = uorder;
  ap->order_v = vorder;
  ap->close_u = uclosed;
  ap->close_v = vclosed;

  ap->ktype_u = ay_apatch_getkt(ukt);
  ap->ktype_v = ay_apatch_getkt(vkt);

  if(!cv)
    {
      if(!(cv = calloc(width*height*stride, sizeof(double))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      if(center)
	{
	  if(fabs(udx) > AY_EPSILON)
	    ext = (width-1)*udx;
	  if(fabs(vdx) > AY_EPSILON)
	    ext += (height-1)*vdx;
	  s[0] = -(ext/2.0);
	  ext = 0.0;
	  if(fabs(udy) > AY_EPSILON)
	    ext = (width-1)*udy;
	  if(fabs(vdy) > AY_EPSILON)
	    ext += (height-1)*vdy;
	  s[1] = -(ext/2.0);
	  ext = 0.0;
	  if(fabs(udz) > AY_EPSILON)
	    ext = (width-1)*udz;
	  if(fabs(vdz) > AY_EPSILON)
	    ext += (height-1)*vdz;
	  s[2] = -(ext/2.0);
	}

      k = 0;
      for(i = 0; i < width; i++)
	{
	  for(j = 0; j < height; j++)
	    {
	      cv[k]   = s[0] + (double)j*vdx;
	      cv[k+1] = s[1] + (double)j*vdy;
	      cv[k+2] = s[2] + (double)j*vdz;
	      k += stride;
	    }
	  s[0] += udx;
	  s[1] += udy;
	  s[2] += udz;
	}
    } /* if */

  ap->controlv = cv;

  o->refine = (void *)ap;

  ay_status = ay_apatch_notifycb(o);

  if(ay_status)
    goto cleanup;

  /* prevent cleanup code from doing something harmful */
  ap = NULL;
  cv = NULL;

cleanup:

  if(ap)
    free(ap);

  if(cv)
    free(cv);

 return ay_status;
} /* ay_apatch_createcb */


/* ay_apatch_deletecb:
 *  delete callback function of apatch object
 */
int
ay_apatch_deletecb(void *c)
{
 ay_apatch_object *apatch = NULL;

  if(!c)
    return AY_ENULL;

  apatch = (ay_apatch_object *)(c);

  /* free controlv */
  if(apatch->controlv)
    free(apatch->controlv);

  /* free NURBS patch(es) */
  if(apatch->npatch)
    (void)ay_object_deletemulti(apatch->npatch, AY_FALSE);

  if(apatch->caps_and_bevels)
    (void)ay_object_deletemulti(apatch->caps_and_bevels, AY_FALSE);

  free(apatch);

 return AY_OK;
} /* ay_apatch_deletecb */


/* ay_apatch_copycb:
 *  copy callback function of apatch object
 */
int
ay_apatch_copycb(void *src, void **dst)
{
 int ay_status = AY_OK;
 ay_apatch_object *apatch = NULL, *apatchsrc = NULL;

  if(!src || !dst)
    return AY_ENULL;

  apatchsrc = (ay_apatch_object *)src;

  if(!(apatch = malloc(sizeof(ay_apatch_object))))
    return AY_EOMEM;

  memcpy(apatch, src, sizeof(ay_apatch_object));

  apatch->controlv = NULL;

  /* copy controlv */
  if(!(apatch->controlv = malloc(3 * apatch->width * apatch->height *
				 sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  memcpy(apatch->controlv, apatchsrc->controlv,
	 3 * apatch->width * apatch->height * sizeof(double));

  apatch->npatch = NULL;
  apatch->caps_and_bevels = NULL;

  *dst = (void *)apatch;

  /* prevent cleanup code from doing something harmful */
  apatch = NULL;

cleanup:

  if(apatch)
    {
      if(apatch->controlv)
	free(apatch->controlv);

      free(apatch);
    }

 return ay_status;
} /* ay_apatch_copycb */


/* ay_apatch_drawcp:
 *  internal helper function
 *  draw the control polygon
 */
void
ay_apatch_drawcp(ay_apatch_object *apatch)
{
 double *ver = NULL;
 int i, j, a, width, height;

  if(!apatch)
    return;

  width = apatch->width;
  height = apatch->height;

  ver = apatch->controlv;

  a = 0;
  if(apatch->close_v)
    {
      for(i = 0; i < width; i++)
	{
	  glBegin(GL_LINE_LOOP);
	   for(j = 0; j < height; j++)
	     {
	       glVertex3dv((GLdouble *)&ver[a]);
	       a += 3;
	     }
	  glEnd();
	}
    }
  else
    {
      for(i = 0; i < width; i++)
	{
	  glBegin(GL_LINE_STRIP);
	   for(j = 0; j < height; j++)
	     {
	       glVertex3dv((GLdouble *)&ver[a]);
	       a += 3;
	     } /* for */
	  glEnd();
	} /* for */
    } /* if closev */

  if(apatch->close_u)
    {
      for(j = 0; j < height; j++)
	{
	  a = j * 3;
	  glBegin(GL_LINE_LOOP);
	   for(i = 0; i < width; i++)
	     {
	       glVertex3dv((GLdouble *)&ver[a]);

	       a += (3 * height);
	     }
	  glEnd();
	}
    }
  else
    {
      for(j = 0; j < height; j++)
	{
	  a = j * 3;
	  glBegin(GL_LINE_STRIP);
	   for(i = 0; i < width; i++)
	     {
	       glVertex3dv((GLdouble *)&ver[a]);

	       a += (3 * height);
	     } /* for */
	  glEnd();
	} /* for */
    } /* if closeu */

 return;
} /* ay_apatch_drawcp */


/* ay_apatch_drawcb:
 *  draw (display in an Ayam view window) callback function of apatch object
 */
int
ay_apatch_drawcb(struct Togl *togl, ay_object *o)
{
 int display_mode = ay_prefs.np_display_mode;
 ay_apatch_object *apatch = NULL;
 ay_object *p = NULL;

  if(!o)
    return AY_ENULL;

  apatch = (ay_apatch_object *)o->refine;

  if(!apatch)
    return AY_ENULL;

  if(apatch->display_mode != 0)
    {
      display_mode = apatch->display_mode-1;
    }

  /* draw just the control polygon? */
  if(display_mode == 0)
    {
      /* Yes */
      ay_apatch_drawcp(apatch);
    }
  else
    {
      /* No, draw the NURBS patch(es), if present */
      p = apatch->npatch;
      while(p)
	{
	  ay_draw_object(togl, p, AY_FALSE);
	  p = p->next;
	} /* while */
    } /* if */

  if(apatch->caps_and_bevels)
    {
      p = apatch->caps_and_bevels;
      while(p)
	{
	  ay_draw_object(togl, p, AY_TRUE);
	  p = p->next;
	}
    }

 return AY_OK;
} /* ay_apatch_drawcb */


/* ay_apatch_shadecb:
 *  shade (display in an Ayam view window) callback function of apatch object
 */
int
ay_apatch_shadecb(struct Togl *togl, ay_object *o)
{
 ay_apatch_object *apatch = NULL;
 ay_object *p = NULL;

  if(!o)
    return AY_ENULL;

  apatch = (ay_apatch_object *)o->refine;

  if(!apatch)
    return AY_ENULL;

  p = apatch->npatch;
  while(p)
    {
      ay_shade_object(togl, p, AY_FALSE);
      p = p->next;
    } /* while */

  if(apatch->caps_and_bevels)
    {
      p = apatch->caps_and_bevels;
      while(p)
	{
	  ay_shade_object(togl, p, AY_FALSE);
	  p = p->next;
	}
    }

 return AY_OK;
} /* ay_apatch_shadecb */


/* ay_apatch_drawacb:
 *  draw annotations (in an Ayam view window) callback function of apatch object
 */
int
ay_apatch_drawacb(struct Togl *togl, ay_object *o)
{
 int width = 0, height = 0;
 ay_apatch_object *apatch;
 GLdouble *ver = NULL;
 /*double point_size = ay_prefs.handle_size;*/

  if(!o)
    return AY_ENULL;

  apatch = (ay_apatch_object *) o->refine;

  if(!apatch)
    return AY_ENULL;

  width = apatch->width;
  height = apatch->height;

  ver = apatch->controlv;

  /* draw direction arrow */
  ay_draw_arrow(togl, &(ver[width*height*3-6]), &(ver[width*height*3-3]));

  if(ay_tags_hastag(o, ay_sb_tagtype))
    (void)ay_npt_drawboundaries(togl, o);

 return AY_OK;
} /* ay_apatch_drawacb */


/* ay_apatch_drawhcb:
 *  draw handles (in an Ayam view window) callback function of apatch object
 */
int
ay_apatch_drawhcb(struct Togl *togl, ay_object *o)
{
 int i;
 double *pnts;
 ay_apatch_object *apatch;

  if(!o)
    return AY_ENULL;

  apatch = (ay_apatch_object *) o->refine;

  if(!apatch)
    return AY_ENULL;

  pnts = apatch->controlv;

  /* draw points */
  glBegin(GL_POINTS);
   for(i = 0; i < (apatch->width*apatch->height); i++)
     {
       glVertex3dv((GLdouble *)pnts);
       pnts += 3;
     }
  glEnd();

 return AY_OK;
} /* ay_apatch_drawhcb */


/* ay_apatch_getpntcb:
 *  get point (editing and selection) callback function of apatch object
 */
int
ay_apatch_getpntcb(int mode, ay_object *o, double *p, ay_pointedit *pe)
{
 int ay_status = AY_OK;
 ay_apatch_object *apatch = NULL;
 ay_point *pnt = NULL, **lastpnt = NULL;
 double min_dist = ay_prefs.pick_epsilon, dist = 0.0;
 double *pecoord = NULL, **pecoords = NULL, **pecoordstmp;
 double *control = NULL, *c;
 int i = 0, j = 0, k = 0, a = 0, found = AY_FALSE;
 unsigned int *itmp, peindex = 0, *peindices = NULL;

  if(!o || ((mode != 3) && (!p || !pe)))
    return AY_ENULL;

  apatch = (ay_apatch_object *)(o->refine);

  if(!apatch)
    return AY_ENULL;

  if(min_dist == 0.0)
    min_dist = DBL_MAX;

  switch(mode)
    {
    case 0:
      /* select all points */
      pe->num = apatch->width * apatch->height;

      if(!(pe->coords = calloc(pe->num, sizeof(double*))))
	return AY_EOMEM;

      if(!(pe->indices = calloc(pe->num, sizeof(unsigned int))))
	return AY_EOMEM;

      for(i = 0; i < (apatch->width*apatch->height); i++)
	{
	  pe->coords[j] = &(apatch->controlv[a]);
	  pe->indices[j] = j;
	  j++;
	  a += 3;
	}
      break;
    case 1:
      /* selection based on a single point */
      control = apatch->controlv;

      for(i = 0; i < (apatch->width * apatch->height); i++)
	{
	  dist = AY_VLEN((p[0] - control[j]),
			 (p[1] - control[j+1]),
			 (p[2] - control[j+2]));

	  if(dist < min_dist)
	    {
	      pecoord = &(control[j]);
	      peindex = i;
	      min_dist = dist;
	    }
	  k++;
	  j += 3;
	}

      if(!pecoord)
	return AY_OK; /* XXXX should this return a 'AY_EPICK' ? */

      if(!found)
	{
	  if(!(pe->coords = calloc(1, sizeof(double*))))
	    return AY_EOMEM;
	  if(!(pe->indices = calloc(1, sizeof(unsigned int))))
	    return AY_EOMEM;

	  pe->coords[0] = pecoord;
	  pe->indices[0] = peindex;
	  pe->num = 1;
	}
      break;
    case 2:
      /* selection based on planes */
      control = apatch->controlv;
      j = 0;
      a = 0;
      for(i = 0; i < apatch->width * apatch->height; i++)
	{
	  c = &(control[j]);

	  /* test point c against the four planes in p */
	  if(((p[0]*c[0] + p[1]*c[1] + p[2]*c[2] + p[3]) < 0.0) &&
	     ((p[4]*c[0] + p[5]*c[1] + p[6]*c[2] + p[7]) < 0.0) &&
	     ((p[8]*c[0] + p[9]*c[1] + p[10]*c[2] + p[11]) < 0.0) &&
	     ((p[12]*c[0] + p[13]*c[1] + p[14]*c[2] + p[15]) < 0.0))
	    {
	      if(!(pecoordstmp = realloc(pecoords, (a+1)*sizeof(double *))))
		{
		  ay_status = AY_EOMEM;
		  goto cleanup;
		}
	      pecoords = pecoordstmp;
	      if(!(itmp = realloc(peindices, (a+1)*sizeof(unsigned int))))
		{
		  ay_status = AY_EOMEM;
		  goto cleanup;
		}
	      peindices = itmp;
	      pecoords[a] = c;
	      peindices[a] = i;
	      a++;
	    } /* if */
	  k++;
	  j += 3;
	} /* for */

      if(!pecoords)
	return AY_OK; /* XXXX should this return a 'AY_EPICK' ? */

      pe->coords = pecoords;
      pe->indices = peindices;
      pe->num = a;

      /* prevent cleanup code from doing something harmful */
      pecoords = NULL;
      peindices = NULL;
      break;
    case 3:
      /* rebuild from o->selp */
      pnt = o->selp;
      lastpnt = &o->selp;
      while(pnt)
	{
	  if(pnt->index < (unsigned int)(apatch->width * apatch->height))
	    {
	      pnt->point = &(apatch->controlv[pnt->index*3]);
	      lastpnt = &(pnt->next);
	      pnt = pnt->next;
	    }
	  else
	    {
	      *lastpnt = pnt->next;
	      free(pnt);
	      pnt = *lastpnt;
	    }
	} /* while */
      break;
    default:
      break;
    } /* switch */

cleanup:

  if(pecoords)
    free(pecoords);

  if(peindices)
    free(peindices);

 return ay_status;
} /* ay_apatch_getpntcb */


/* ay_apatch_setpropcb:
 *  set property (from Tcl to C context) callback function of apatch object
 */
int
ay_apatch_setpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
 int ay_status = AY_OK;
 char *arr = "APatchAttrData";
 char fname[] = "apatch_setpropcb";
 char rszerr[] = "Could not resize the patch.";
 Tcl_Obj *to = NULL;
 ay_apatch_object *apatch = NULL;
 int new_ktype_u = -1, new_close_u = -1, new_order_u = -1;
 int new_width = -1, new_awidth = -1;
 int new_ktype_v = -1, new_close_v = -1, new_order_v = -1;
 int new_height = -1, new_aheight = -1;
 int new_mode = 0, i, update = AY_FALSE;

  if(!interp || !o)
    return AY_ENULL;

  apatch = (ay_apatch_object *)o->refine;

  if(!apatch)
    return AY_ENULL;

  if((to = Tcl_GetVar2Ex(interp, arr, "Mode",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(new_mode));

  if((to = Tcl_GetVar2Ex(interp, arr, "AWidth",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(new_awidth));

  if((to = Tcl_GetVar2Ex(interp, arr, "AHeight",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(new_aheight));

  if((to = Tcl_GetVar2Ex(interp, arr, "Width",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(new_width));

  if((to = Tcl_GetVar2Ex(interp, arr, "Height",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(new_height));

  if((to = Tcl_GetVar2Ex(interp, arr, "Close_U",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(new_close_u));

  if((to = Tcl_GetVar2Ex(interp, arr, "Close_V",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(new_close_v));

  if((to = Tcl_GetVar2Ex(interp, arr, "Order_U",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(new_order_u));

  if((to = Tcl_GetVar2Ex(interp, arr, "Order_V",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(new_order_v));

  if((to = Tcl_GetVar2Ex(interp, arr, "Knot-Type_U",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(new_ktype_u));

  if((to = Tcl_GetVar2Ex(interp, arr, "Knot-Type_V",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(new_ktype_v));

  if((new_mode != apatch->mode)||
     (new_awidth != apatch->awidth)||
     (new_aheight != apatch->aheight)||
     (new_order_u != apatch->order_u)||
     (new_order_v != apatch->order_v)||
     (new_close_u != apatch->close_u)||
     (new_close_v != apatch->close_v)||
     (new_ktype_u != apatch->ktype_u)||
     (new_ktype_v != apatch->ktype_v))
    update = AY_TRUE;

  if(new_mode >= 0)
    apatch->mode = new_mode;

  switch(new_ktype_u)
    {
    case 0:
      apatch->ktype_u = AY_KTCHORDAL;
      break;
    case 1:
      apatch->ktype_u = AY_KTCENTRI;
      break;
    case 2:
      apatch->ktype_u = AY_KTBEZIER;
      break;
    case 3:
      apatch->ktype_u = AY_KTBSPLINE;
      break;
    case 4:
      apatch->ktype_u = AY_KTNURB;
      break;
    default:
      /* output error? */
      break;
    }

  switch(new_ktype_v)
    {
    case 0:
      apatch->ktype_v = AY_KTCHORDAL;
      break;
    case 1:
      apatch->ktype_v = AY_KTCENTRI;
      break;
    case 2:
      apatch->ktype_v = AY_KTBEZIER;
      break;
    case 3:
      apatch->ktype_v = AY_KTBSPLINE;
      break;
    case 4:
      apatch->ktype_v = AY_KTNURB;
      break;
    default:
      /* output error? */
      break;
    }

  apatch->close_u = new_close_u;
  apatch->close_v = new_close_v;

  if((to = Tcl_GetVar2Ex(interp, arr, "Tolerance",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetDoubleFromObj(interp, to, &(apatch->glu_sampling_tolerance));

  if((to = Tcl_GetVar2Ex(interp, arr, "DisplayMode",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(apatch->display_mode));

  if((to = Tcl_GetVar2Ex(interp, arr, "BevelsChanged",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    {
      Tcl_GetIntFromObj(interp, to, &i);
      if(i)
	{
	  update = AY_TRUE;
	  Tcl_SetVar2Ex(interp, arr, "BevelsChanged",
			Tcl_NewIntObj(0),
			TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	}
    }


  if((to = Tcl_GetVar2Ex(interp, arr, "CapsChanged",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    {
      Tcl_GetIntFromObj(interp, to, &i);
      if(i)
	{
	  update = AY_TRUE;
	  Tcl_SetVar2Ex(interp, arr, "CapsChanged",
			Tcl_NewIntObj(0),
			TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	}
    }

  /* apply changed values to patch */

  /* resize patch */
  if(new_width != apatch->width && (new_width > 1))
    {
      if(o->selp)
	{
	  ay_selp_clear(o);
	}

      ay_status = ay_npt_resizearrayw(&(apatch->controlv), 3,
				      apatch->width, apatch->height,
				      new_width);

      if(ay_status)
	{
	  ay_error(AY_ERROR, fname, rszerr);
	}
      else
	{
	  apatch->width = new_width;
	  update = AY_TRUE;
	} /* if */
    } /* if */

  if(new_height != apatch->height && (new_height > 1))
    {
      if(o->selp)
	{
	  ay_selp_clear(o);
	}

      ay_status = ay_npt_resizearrayh(&(apatch->controlv), 3,
				      apatch->width, apatch->height,
				      new_height);

      if(ay_status)
	{
	  ay_error(AY_ERROR, fname, rszerr);
	}
      else
	{
	  apatch->height = new_height;
	  update = AY_TRUE;
	} /* if */
    } /* if */

  apatch->awidth = new_awidth;
  if(apatch->awidth > apatch->width)
    apatch->awidth = apatch->width-1;

  apatch->aheight = new_aheight;
  if(apatch->aheight > apatch->height)
    apatch->aheight = apatch->height-1;

  if(apatch->awidth < 2)
    apatch->awidth = 2;

  if(apatch->aheight < 2)
    apatch->aheight = 2;

  apatch->order_u = new_order_u;
  apatch->order_v = new_order_v;

  if(apatch->order_u < 2)
    apatch->order_u = 2;

  if(apatch->order_u > apatch->awidth)
    {
      apatch->order_u = (apatch->awidth < 4) ? apatch->awidth : 4;
    }

  if(apatch->order_v < 2)
    apatch->order_v = 2;

  if(apatch->order_v > apatch->aheight)
    {
      apatch->order_v = (apatch->aheight < 4) ? apatch->aheight : 4;
    }


  if(update)
    {
      (void)ay_notify_object(o);

      o->modified = AY_TRUE;

      (void)ay_notify_parent();
    }

 return AY_OK;
} /* ay_apatch_setpropcb */


/* ay_apatch_getpropcb:
 *  get property (from C to Tcl context) callback function of apatch object
 */
int
ay_apatch_getpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
 char *arr = "APatchAttrData";
 ay_apatch_object *apatch = NULL;
 Tcl_Obj *to = NULL;

  if(!interp || !o)
    return AY_ENULL;

  apatch = (ay_apatch_object *)(o->refine);

  if(!apatch)
    return AY_ENULL;

  Tcl_SetVar2Ex(interp, arr, "Mode",
		Tcl_NewIntObj(apatch->mode),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "AWidth",
		Tcl_NewIntObj(apatch->awidth),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "AHeight",
		Tcl_NewIntObj(apatch->aheight),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Width",
		Tcl_NewIntObj(apatch->width),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Height",
		Tcl_NewIntObj(apatch->height),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Close_U",
		Tcl_NewIntObj(apatch->close_u),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Close_V",
		Tcl_NewIntObj(apatch->close_v),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Order_U",
		Tcl_NewIntObj(apatch->order_u),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Order_V",
		Tcl_NewIntObj(apatch->order_v),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  switch(apatch->ktype_u)
    {
    case AY_KTCHORDAL:
      to = Tcl_NewIntObj(0);
      break;
    case AY_KTCENTRI:
      to = Tcl_NewIntObj(1);
      break;
    case AY_KTBEZIER:
      to = Tcl_NewIntObj(2);
      break;
    case AY_KTBSPLINE:
      to = Tcl_NewIntObj(3);
      break;
    case AY_KTNURB:
      to = Tcl_NewIntObj(4);
      break;
    default:
      break;
    }
  Tcl_SetVar2Ex(interp, arr, "Knot-Type_U", to,
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  switch(apatch->ktype_v)
    {
    case AY_KTCHORDAL:
      to = Tcl_NewIntObj(0);
      break;
    case AY_KTCENTRI:
      to = Tcl_NewIntObj(1);
      break;
    case AY_KTBEZIER:
      to = Tcl_NewIntObj(2);
      break;
    case AY_KTBSPLINE:
      to = Tcl_NewIntObj(3);
      break;
    case AY_KTNURB:
      to = Tcl_NewIntObj(4);
      break;
    default:
      break;
    }
  Tcl_SetVar2Ex(interp, arr, "Knot-Type_V", to,
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Tolerance",
		Tcl_NewDoubleObj(apatch->glu_sampling_tolerance),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "DisplayMode",
		Tcl_NewIntObj(apatch->display_mode),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "BevelsChanged",
		Tcl_NewIntObj(0),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "CapsChanged",
		Tcl_NewIntObj(0),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  (void)ay_prop_getnpinfo(interp, arr, apatch->npatch);

 return AY_OK;
} /* ay_apatch_getpropcb */


/* ay_apatch_readcb:
 *  read (from scene file) callback function of apatch object
 */
int
ay_apatch_readcb(FILE *fileptr, ay_object *o)
{
 int ay_status = AY_OK;
 ay_apatch_object *apatch = NULL;
 int i, a;

  if(!fileptr || !o)
    return AY_ENULL;

  if(!(apatch = calloc(1, sizeof(ay_apatch_object))))
    return AY_EOMEM;

  fscanf(fileptr, "%d\n", &apatch->mode);
  fscanf(fileptr, "%d\n", &apatch->awidth);
  fscanf(fileptr, "%d\n", &apatch->aheight);
  fscanf(fileptr, "%d\n", &apatch->width);
  fscanf(fileptr, "%d\n", &apatch->height);
  fscanf(fileptr, "%d\n", &apatch->close_u);
  fscanf(fileptr, "%d\n", &apatch->close_v);
  fscanf(fileptr, "%d\n", &apatch->order_u);
  fscanf(fileptr, "%d\n", &apatch->order_v);
  fscanf(fileptr, "%d\n", &apatch->ktype_u);
  fscanf(fileptr, "%d\n", &apatch->ktype_v);

  if(!(apatch->controlv = calloc(apatch->width*apatch->height*3,
				 sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  a = 0;
  for(i = 0; i < apatch->width*apatch->height; i++)
    {
      fscanf(fileptr, "%lg %lg %lg\n",
	     &(apatch->controlv[a]),
	     &(apatch->controlv[a+1]),
	     &(apatch->controlv[a+2]));
      a += 3;
    }

  fscanf(fileptr, "%lg\n", &(apatch->glu_sampling_tolerance));
  fscanf(fileptr, "%d\n", &(apatch->display_mode));

  o->refine = apatch;

  /* prevent cleanup code from doing something harmful */
  apatch = NULL;

cleanup:

  if(apatch)
    {
      if(apatch->controlv)
	free(apatch->controlv);

      free(apatch);
    }

 return ay_status;
} /* ay_apatch_readcb */


/* ay_apatch_writecb:
 *  write (to scene file) callback function of apatch object
 */
int
ay_apatch_writecb(FILE *fileptr, ay_object *o)
{
 ay_apatch_object *apatch = NULL;
 int i, a;

  if(!fileptr || !o)
    return AY_ENULL;

  apatch = (ay_apatch_object *)(o->refine);

  if(!apatch)
    return AY_ENULL;

  fprintf(fileptr, "%d\n", apatch->mode);
  fprintf(fileptr, "%d\n", apatch->awidth);
  fprintf(fileptr, "%d\n", apatch->aheight);
  fprintf(fileptr, "%d\n", apatch->width);
  fprintf(fileptr, "%d\n", apatch->height);
  fprintf(fileptr, "%d\n", apatch->close_u);
  fprintf(fileptr, "%d\n", apatch->close_v);
  fprintf(fileptr, "%d\n", apatch->order_u);
  fprintf(fileptr, "%d\n", apatch->order_v);
  fprintf(fileptr, "%d\n", apatch->ktype_u);
  fprintf(fileptr, "%d\n", apatch->ktype_v);

  a = 0;
  for(i = 0; i < apatch->width*apatch->height; i++)
    {
      fprintf(fileptr, "%g %g %g\n", apatch->controlv[a],
	      apatch->controlv[a+1], apatch->controlv[a+2]);
      a += 3;
    }

  fprintf(fileptr, "%g\n", apatch->glu_sampling_tolerance);
  fprintf(fileptr, "%d\n", apatch->display_mode);

 return AY_OK;
} /* ay_apatch_writecb */


/* ay_apatch_wribcb:
 *  RIB export callback function of apatch object
 */
int
ay_apatch_wribcb(char *file, ay_object *o)
{
 int ay_status = AY_OK;
 ay_apatch_object *apatch = NULL;
 ay_object *p;
 unsigned int ci = 1;

  if(!o)
    return AY_ENULL;

  apatch = (ay_apatch_object*)(o->refine);

  if(!apatch)
    return AY_ENULL;

  p = apatch->npatch;
  while(p)
    {
      ay_wrib_object(file, p);
      p = p->next;
    }

  p = apatch->caps_and_bevels;
  while(p)
    {
      ay_wrib_caporbevel(file, o, p, ci);
      ci++;
      p = p->next;
    }

 return ay_status;
} /* ay_apatch_wribcb */


/* ay_apatch_bbccb:
 *  bounding box calculation callback function of apatch object
 */
int
ay_apatch_bbccb(ay_object *o, double *bbox, int *flags)
{
 ay_apatch_object *apatch = NULL;

  if(!o || !bbox || !flags)
    return AY_ENULL;

  apatch = (ay_apatch_object *)o->refine;

  if(!apatch)
    return AY_ENULL;

 return ay_bbc_fromarr(apatch->controlv, apatch->width*apatch->height,
		       3, bbox);
} /* ay_apatch_bbccb */


/* ay_apatch_notifycb:
 *  notification callback function of apatch object
 */
int
ay_apatch_notifycb(ay_object *o)
{
 int ay_status = AY_OK;
 int mode, ktu = 0, ktv = 0;
 ay_object *p = NULL;
 ay_apatch_object *ap = NULL;
 ay_nurbpatch_object *np = NULL;
 ay_object *bevel = NULL, **nextcb;
 ay_bparam bparams = {0};
 ay_cparam cparams = {0};
 double tolerance;

  if(!o)
    return AY_ENULL;

  ap = (ay_apatch_object *)o->refine;

  if(!ap)
    return AY_ENULL;

  mode = ap->display_mode;
  tolerance = ap->glu_sampling_tolerance;

  nextcb = &(ap->caps_and_bevels);

  if(ap->npatch)
    {
      (void)ay_object_deletemulti(ap->npatch, AY_FALSE);
      ap->npatch = NULL;
    }

  if(ap->caps_and_bevels)
    {
      (void)ay_object_deletemulti(ap->caps_and_bevels, AY_FALSE);
      ap->caps_and_bevels = NULL;
    }

  /* create new approximating surface */

  switch(ap->mode)
    {
    case 0:
      /* UV */
      if(ap->ktype_u == AY_KTCENTRI)
	ktu = 1;
      if(ap->ktype_v == AY_KTCENTRI)
	ktv = 1;
      ay_status = ay_apt_approximateuv(ap->controlv, ap->width, ap->height,
				       ap->awidth, ap->aheight,
				       ap->order_u, ap->order_v,
				       ktu, ktv,
				       ap->close_u, ap->close_v,
				       &np);
      break;
    case 1:
      /* VU */
      if(ap->ktype_u == AY_KTCENTRI)
	ktu = 1;
      if(ap->ktype_v == AY_KTCENTRI)
	ktv = 1;
      ay_status = ay_apt_approximatevu(ap->controlv, ap->width, ap->height,
				       ap->awidth, ap->aheight,
				       ap->order_u, ap->order_v,
				       ktu, ktv,
				       ap->close_u, ap->close_v,
				       &np);
      break;
    case 2:
      /* U */
      if(ap->ktype_u == AY_KTCENTRI)
	ktu = 1;
      ktv = ap->ktype_v;
      ay_status = ay_apt_approximateu(ap->controlv, ap->width, ap->height,
				      ap->awidth,
				      ap->order_u, ap->order_v,
				      ktu, ktv,
				      ap->close_u, ap->close_v,
				      &np);
      break;
    case 3:
      /* V */
      ktu = ap->ktype_u;
      if(ap->ktype_v == AY_KTCENTRI)
	ktv = 1;
      ay_status = ay_apt_approximatev(ap->controlv, ap->width, ap->height,
				      ap->aheight,
				      ap->order_u, ap->order_v,
				      ktu, ktv,
				      ap->close_u, ap->close_v,
				      &np);
      break;
    default:
      break;
    } /* switch */

  if(ay_status)
    goto cleanup;

  if((ay_status = ay_npt_createnpatchobject(&p)))
    {
      goto cleanup;
    }
  p->refine = (void*)np;
  np = NULL;

  ap->npatch = p;

  /* copy sampling tolerance/mode attributes to NURBS patch(es) */
  p = ap->npatch;
  while(p)
    {
      if(p->type == AY_IDNPATCH)
	{
	  ((ay_nurbpatch_object *)
	   (p->refine))->glu_sampling_tolerance = tolerance;
	  ((ay_nurbpatch_object *)
	   (p->refine))->display_mode = mode;
	}
      p = p->next;
    }

  /* get bevel and cap parameters */
  if(o->tags)
    {
      ay_bevelt_parsetags(o->tags, &bparams);
      ay_capt_parsetags(o->tags, &cparams);
    }

  /* create/add caps */
  if(cparams.has_caps)
    {
      ay_status = ay_capt_addcaps(&cparams, &bparams, ap->npatch, nextcb);
      if(ay_status)
	goto cleanup;

      while(*nextcb)
	nextcb = &((*nextcb)->next);
    }

  /* create/add bevels */
  if(bparams.has_bevels)
    {
      bparams.dirs[1] = !bparams.dirs[1];
      bparams.dirs[2] = !bparams.dirs[2];
      bparams.radii[2] = -bparams.radii[2];

      ay_status = ay_bevelt_addbevels(&bparams, &cparams, ap->npatch, nextcb);
      if(ay_status)
	goto cleanup;
    }

  bevel = ap->caps_and_bevels;
  while(bevel)
    {
      ((ay_nurbpatch_object *)
       (bevel->refine))->glu_sampling_tolerance = tolerance;
      ((ay_nurbpatch_object *)
       (bevel->refine))->display_mode = mode;
      bevel = bevel->next;
    }

  /* prevent cleanup code from doing something harmful */
  p = NULL;

cleanup:

  if(np)
    ay_npt_destroy(np);

  if(p)
    (void)ay_object_deletemulti(p, AY_FALSE);

 return ay_status;
} /* ay_apatch_notifycb */


/* ay_apatch_providecb:
 *  provide callback function of apatch object
 */
int
ay_apatch_providecb(ay_object *o, unsigned int type, ay_object **result)
{
 ay_apatch_object *ap = NULL;

  if(!o)
    return AY_ENULL;

  ap = (ay_apatch_object *) o->refine;

  if(!ap)
    return AY_ENULL;

 return ay_provide_nptoolobj(o, type, ap->npatch, ap->caps_and_bevels, result);
} /* ay_apatch_providecb */


/* ay_apatch_convertcb:
 *  convert callback function of apatch object
 */
int
ay_apatch_convertcb(ay_object *o, int in_place)
{
 ay_apatch_object *ap = NULL;

  if(!o)
    return AY_ENULL;

  ap = (ay_apatch_object *) o->refine;

  if(!ap)
    return AY_ENULL;

 return ay_convert_nptoolobj(o, ap->npatch, ap->caps_and_bevels, in_place);
} /* ay_apatch_convertcb */


/* ay_apatch_init:
 *  initialize the apatch object module
 */
int
ay_apatch_init(Tcl_Interp *interp)
{
 int ay_status = AY_OK;

  ay_status = ay_otype_registercore(ay_apatch_name,
				    ay_apatch_createcb,
				    ay_apatch_deletecb,
				    ay_apatch_copycb,
				    ay_apatch_drawcb,
				    ay_apatch_drawhcb,
				    ay_apatch_shadecb,
				    ay_apatch_setpropcb,
				    ay_apatch_getpropcb,
				    ay_apatch_getpntcb,
				    ay_apatch_readcb,
				    ay_apatch_writecb,
				    ay_apatch_wribcb,
				    ay_apatch_bbccb,
				    AY_IDAPATCH);

  ay_status += ay_draw_registerdacb(ay_apatch_drawacb, AY_IDAPATCH);

  ay_status += ay_notify_register(ay_apatch_notifycb, AY_IDAPATCH);

  ay_status += ay_convert_register(ay_apatch_convertcb, AY_IDAPATCH);

  ay_status += ay_provide_register(ay_apatch_providecb, AY_IDAPATCH);

 return ay_status;
} /* ay_apatch_init */

