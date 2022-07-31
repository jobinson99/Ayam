/*
 * Ayam, a free 3D modeler for the RenderMan interface.
 *
 * Ayam is copyrighted 1998-2002 by Randolf Schultz
 * (randolf.schultz@gmail.com) and others.
 *
 * All rights reserved.
 *
 * See the file License for details.
 *
 */

#include "ayam.h"

#include <ctype.h>

/* tcmd.c - various simple Tcl commands and support functions */

/* global variables for this module: */

/** all registered revert callbacks */
static ay_ftable ay_tcmd_revertcbt;

/** all registered open callbacks */
static ay_ftable ay_tcmd_opencbt;

/** all registered close callbacks */
static ay_ftable ay_tcmd_closecbt;

/** all registered refine callbacks */
static ay_ftable ay_tcmd_refinecbt;

/** all registered coarsen callbacks */
static ay_ftable ay_tcmd_coarsencbt;


/* prototypes of functions local to this module: */

int ay_tcmd_setallpoints(Tcl_Interp *interp, char *fname, char *vn,
			 int from_world);

int ay_tcmd_getselpoints(Tcl_Interp *interp, char *fname, char *vn,
			 int get_all, int apply_trafo);

int ay_tcmd_crtextrnc(ay_object *o, ay_object *p, int oid, int bid,
		      ay_object **e);


/* functions: */

/** ay_tcmd_convdlist:
 *  convert a Tcl list of doubles to a C array of doubles
 *
 * \param[in] interp  Tcl interpreter to use
 * \param[in] vname  name of a Tcl variable that contains the list of values
 *                   to convert
 * \param[in,out] dllen  where to store the length of the array
 * \param[in,out] dl  where to store the array of doubles
 *
 * \returns TCL_OK upon successful completion, TCL_ERROR else.
 */
int
ay_tcmd_convdlist(Tcl_Interp *interp, char *vname, int *dllen, double **dl)
{
 int tcl_status = TCL_OK;
 Tcl_Obj *listPtr = NULL, **elemvPtr = NULL;
 int i;

  if(!vname || !dllen || !dl)
    {
      return TCL_ERROR;
    }

  listPtr = Tcl_GetVar2Ex(interp, vname, NULL, TCL_LEAVE_ERR_MSG);

  if(!listPtr)
    {
      return TCL_ERROR;
    }

  tcl_status = Tcl_ListObjGetElements(interp, listPtr, dllen, &elemvPtr);

  if((tcl_status != TCL_OK) || !elemvPtr)
    {
      return TCL_ERROR;
    }

  if(!(*dl = malloc(*dllen*sizeof(double))))
    {
      return TCL_ERROR;
    }

  for(i = 0; i < *dllen; i++)
    {
      tcl_status = Tcl_GetDoubleFromObj(interp, elemvPtr[i], &((*dl)[i]));
      if(tcl_status != TCL_OK)
	{
	  free(*dl);
	  *dl = NULL;
	  return TCL_ERROR;
	}
    }

 return TCL_OK;
} /* ay_tcmd_convdlist */


/** ay_tcmd_reverttcmd:
 *  revert selected curves
 *  Implements the \a revertC scripting interface command.
 *  See also the corresponding section in the \ayd{screvertc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_tcmd_reverttcmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
 int ay_status = AY_OK;
 int notify_parent = AY_FALSE;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL;
 ay_acurve_object *acurve = NULL;
 ay_icurve_object *icurve = NULL;
 ay_nurbcurve_object *ncurve = NULL;
 ay_concatnc_object *cc = NULL;
 ay_voidfp *arr = NULL;
 ay_genericcb *cb = NULL;

  while(sel)
    {
      o = sel->object;

      switch(o->type)
	{
	case AY_IDACURVE:
	  acurve = (ay_acurve_object*)o->refine;

	  ay_status = ay_act_revert(acurve);
	  if(ay_status)
	    {
	      ay_error(ay_status, argv[0], "Could not revert ACurve!");
	    }
	  else
	    {
	      ay_notify_object(o);
	      o->modified = AY_TRUE;
	      notify_parent = AY_TRUE;
	    }
	  break;
	case AY_IDICURVE:
	  icurve = (ay_icurve_object*)o->refine;

	  ay_status = ay_ict_revert(icurve);
	  if(ay_status)
	    {
	      ay_error(ay_status, argv[0], "Could not revert ICurve!");
	    }
	  else
	    {
	      ay_notify_object(o);
	      o->modified = AY_TRUE;
	      notify_parent = AY_TRUE;
	    }
	  break;
	case AY_IDNCURVE:
	  ncurve = (ay_nurbcurve_object*)o->refine;

	  ay_status = ay_nct_revert(ncurve);
	  if(ay_status)
	    {
	      ay_error(ay_status, argv[0], "Could not revert NCurve!");
	    }
	  else
	    {
	      ay_notify_object(o);
	      o->modified = AY_TRUE;
	      notify_parent = AY_TRUE;
	    }
	  break;
	case AY_IDCONCATNC:
	  cc = (ay_concatnc_object *)o->refine;
	  cc->revert = !cc->revert;
	  ay_notify_object(o);
	  o->modified = AY_TRUE;
	  notify_parent = AY_TRUE;
	  break;
	default:
	  arr = ay_tcmd_revertcbt.arr;
	  cb = (ay_genericcb *)(arr[o->type]);
	  if(cb)
	    {
	      ay_status = cb(o, AY_OPREVERT);
	      if(!ay_status)
		{
		  ay_notify_object(o);
		  o->modified = AY_TRUE;
		  notify_parent = AY_TRUE;
		}
	    }
	  else
	    {
	      ay_error(AY_EWARN, argv[0], ay_error_igntype);
	    }
	  break;
	} /* switch */

      sel = sel->next;
    } /* while */

  if(notify_parent)
    ay_notify_parent();

 return TCL_OK;
} /* ay_tcmd_reverttcmd */


/** ay_tcmd_showhideall:
 *  _recursively_ set the hidden flag of all children of object
 *  \a o and \a o to \a val (showing/hiding the objects)
 *  if \a val is -1, the hide state of the objects is toggled
 *
 * \param[in,out] o  object(s) to process
 * \param[in] val  if -1 the hide flag is toggled;
 *                 if  0 the flag is cleared;
 *                 if  1 the flag is set
 */
void
ay_tcmd_showhideall(ay_object *o, int val)
{
 ay_object *down;

  if(!o)
    return;

  if(o->down && o->down->next)
    {
      down = o->down;
      while(down->next)
	{
	  ay_tcmd_showhideall(down, val);
	  down = down->next;
	}
    }

  if(val == -1)
    o->hide = !o->hide;
  else
    o->hide = val;

 return;
} /* ay_tcmd_showhideall */


/** ay_tcmd_showhidetcmd:
 *  show/hide selected (or all) objects
 *  Implements the \a showOb scripting interface command.
 *  Implements the \a hideOb scripting interface command.
 *  See also the corresponding section in the \ayd{schideob}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_tcmd_showhidetcmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
  /*
 int ay_status = AY_OK;
  */
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL;
 int i = 1, all = AY_FALSE, val = AY_TRUE;

  if(argv[0][0] == 's')
    val = AY_FALSE;

  if(argc > 1)
    {
      while(i < argc)
	{
	  /* -toggle */
	  if(argv[i][0] == '-' && argv[i][1] == 't')
	    {
	      val = -1;
	    }
	  /* -all */
	  if(argv[i][0] == '-' && argv[i][1] == 'a')
	    {
	      all = AY_TRUE;
	    }
	  i++;
	} /* while */
    } /* if */

  if(all)
    {
      o = ay_root->next;
      while(o->next)
	{
	  ay_tcmd_showhideall(o, val);
	  o = o->next;
	}
    }
  else
    {
      while(sel)
	{
	  o = sel->object;

	  if(val == -1)
	    o->hide = !o->hide;
	  else
	    o->hide = val;

	  sel = sel->next;
	} /* while */
    } /* if */

 return TCL_OK;
} /* ay_tcmd_showhidetcmd */


/** ay_tcmd_getversionstcmd:
 *  get all version information from the current OpenGL/GLU
 *  and put it in the global array "ay"
 *  Implements the \a getVersion scripting interface command.
 *  See also the corresponding section in the \ayd{scgetversion}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_tcmd_getversionstcmd(ClientData clientData, Tcl_Interp *interp,
			int argc, char *argv[])
{
 char *glver = NULL, *glven = NULL, *glren = NULL, *gluver = NULL,
   *gluext = NULL, *glext = NULL;
 char arr[] = "ay";

  glver = (char *)glGetString(GL_VERSION);
  if(glver)
    Tcl_SetVar2(interp, arr, "gl_ver", glver, TCL_GLOBAL_ONLY |
		TCL_LEAVE_ERR_MSG);

  glven = (char *)glGetString(GL_VENDOR);
  if(glven)
    Tcl_SetVar2(interp, arr, "gl_ven", glven, TCL_GLOBAL_ONLY |
		TCL_LEAVE_ERR_MSG);

  glren = (char *)glGetString(GL_RENDERER);
  if(glren)
    Tcl_SetVar2(interp, arr, "gl_ren", glren, TCL_GLOBAL_ONLY |
		TCL_LEAVE_ERR_MSG);

  glext = (char *)glGetString(GL_EXTENSIONS);
  if(glext)
    Tcl_SetVar2(interp, arr, "gl_ext", glext, TCL_GLOBAL_ONLY |
		TCL_LEAVE_ERR_MSG);

  gluver = (char *)gluGetString(GLU_VERSION);
  if(gluver)
    Tcl_SetVar2(interp, arr, "glu_ver", gluver, TCL_GLOBAL_ONLY |
		TCL_LEAVE_ERR_MSG);

  gluext = (char *)gluGetString(GLU_EXTENSIONS);
  if(gluext)
    Tcl_SetVar2(interp, arr, "glu_ext", gluext, TCL_GLOBAL_ONLY |
		TCL_LEAVE_ERR_MSG);

 return TCL_OK;
} /* ay_tcmd_getversionstcmd */


/** ay_tcmd_getbppntfromindex:
 *  get bilinear patch coordinate address from index
 *  (performing bounds checking and reporting errors via ay_error())
 *
 * \param[in] patch  bilinear patch to process
 * \param[in] index  point index (0-3)
 *
 * \param[in,out] p  where to store the coordinate address
 *
 * \return AY_OK on success, error code otherwise
 */
int
ay_tcmd_getbppntfromindex(ay_bpatch_object *patch, int index,
			  double **p)
{
 char fname[] = "getbppntfromindex";

  if(!patch || !p)
    return AY_ENULL;

  if(index >= 4 || index < 0)
    {
      ay_error(AY_ERANGE, fname, "[0, 3]");
      return AY_ERROR;
    }

  switch(index)
    {
    case 0:
      *p = patch->p1;
      break;
    case 1:
      *p = patch->p2;
      break;
    case 2:
      *p = patch->p3;
      break;
    case 3:
      *p = patch->p4;
      break;
    default:
      *p = patch->p1;
      break;
    } /* switch */

 return AY_OK;
} /* ay_tcmd_getbppntfromindex */


/* ay_tcmd_getselpoints:
 *  helper for ay_tcmd_getpointtcmd() below
 *  get selected or all points of selected objects
 */
int
ay_tcmd_getselpoints(Tcl_Interp *interp, char *fname, char *vn,
		     int get_all, int apply_trafo)
{
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL;
 ay_pointedit pe = {0};
 ay_point *pnt;
 unsigned int i = 0, j = 0;
 double p[4] = {0};
 double pm[16], m[16];
 int flags = TCL_APPEND_VALUE | TCL_LIST_ELEMENT | TCL_LEAVE_ERR_MSG;
 int return_result = AY_FALSE;
 Tcl_Obj *to = NULL, *res = NULL;

  if(!vn)
    return_result = AY_TRUE;

  if(apply_trafo)
    {
      ay_trafo_identitymatrix(pm);
    }

  if(apply_trafo == 2)
    {
      if(ay_currentlevel->object != ay_root)
	{
	  ay_trafo_getparent(ay_currentlevel->next, pm);
	}
    }

  while(sel)
    {
      o = sel->object;

      if(apply_trafo == 1)
	{
	  memcpy(m, pm, 16*sizeof(double));
	  ay_trafo_getall(NULL, o, m);
	} /* if */

      if(get_all)
	{
	  ay_pact_getpoint(0, o, p, &pe);

	  for(i = 0; i < pe.num; i++)
	    {
	      memcpy(p, pe.coords[i],
		     ((pe.type == AY_PTRAT)?4:3)*sizeof(double));
	      if(apply_trafo)
		ay_trafo_apply3(p, m);

	      if(!return_result)
		{
		  /* store result in variable */
		  for(j = 0;
		      j < (unsigned int)((pe.type == AY_PTRAT)?4:3); j++)
		    {
		      to = Tcl_NewDoubleObj(p[j]);
		      Tcl_SetVar2Ex(interp, vn, NULL, to, flags);
		    }
		}
	      else
		{
		  if(!res)
		    res = Tcl_NewListObj(0, NULL);
		  if(res)
		    {
		      for(j = 0;
			  j < (unsigned int)((pe.type == AY_PTRAT)?4:3); j++)
			{
			  to = Tcl_NewDoubleObj(p[j]);
			  Tcl_ListObjAppendElement(interp, res, to);
			}
		    }
		} /* if return_result */
	    } /* for */

	  ay_pact_clearpointedit(&pe);
	}
      else
	{
	  pnt = o->selp;
	  while(pnt)
	    {
	      memcpy(p, pnt->point,
		     ((pnt->type == AY_PTRAT)?4:3)*sizeof(double));

	      if(apply_trafo)
		ay_trafo_apply3(p, m);

	      if(!return_result)
		{
		  /* store result in variable */
		  for(j = 0;
		      j < (unsigned int)((pnt->type == AY_PTRAT)?4:3); j++)
		    {
		      to = Tcl_NewDoubleObj(p[j]);
		      Tcl_SetVar2Ex(interp, vn, NULL, to, flags);
		    }
		}
	      else
		{
		  if(!res)
		    res = Tcl_NewListObj(0, NULL);
		  if(res)
		    {
		      for(j = 0;
			  j < (unsigned int)((pnt->type == AY_PTRAT)?4:3); j++)
			{
			  to = Tcl_NewDoubleObj(p[j]);
			  Tcl_ListObjAppendElement(interp, res, to);
			}
		    }
		} /* if return_result */
	      pnt = pnt->next;
	    } /* while */
	} /* if all|sel */

      sel = sel->next;
    } /* while */

  if(res)
    {
      Tcl_SetObjResult(interp, res);
    }

 return TCL_OK;
} /* ay_tcmd_getselpoints */


/** ay_tcmd_evalcurve:
 * Helper for ay_tcmd_getpointtcmd/"getPnt" below.
 * Evaluate a NURBS curve.
 *
 * \param[in] fname  a name for error reporting purposes
 * \param[in] nc  the curve to evaluate
 * \param[in] u  parametric value where to evaluate the curve
 * \param[in] relative  interpret \a u in a relative way?
 *
 * \param[in,out] p  where to store the evaluated point coordinates
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_tcmd_evalcurve(char *fname, ay_nurbcurve_object *nc, double u, int relative,
		  double *p)
{
 int ay_status = AY_OK;
 double ru;
 char *fpname[2] = {fname, "u"};

  ay_status = ay_knots_evalparam(fname?fpname:NULL, u, relative,
				 nc->length, nc->order,
				 nc->knotv, &ru);

  if(ay_status)
    return ay_status;

  /* evaluate the curve */
  if(nc->is_rat)
    {
      ay_status = ay_nb_CurvePoint4D(nc->length-1, nc->order-1,
				     nc->knotv, nc->controlv,
				     ru, p);
    }
  else
    {
      ay_status = ay_nb_CurvePoint3D(nc->length-1, nc->order-1,
				     nc->knotv, nc->controlv,
				     ru, p);
    }

  if(ay_status)
    {
      ay_error(AY_ERROR, fname, "Evaluation failed.");
    }

 return ay_status;
} /* ay_tcmd_evalcurve */


/** ay_tcmd_evalcurvetangent:
 * Helper for ay_tcmd_gettangenttcmd/"getPnt" below.
 * Evaluate a NURBS curve tangent.
 *
 * \param[in] fname  a name for error reporting purposes
 * \param[in] nc  the curve to evaluate
 * \param[in] u  parametric value where to evaluate the curve
 * \param[in] relative  interpret \a u in a relative way?
 *
 * \param[in,out] t  where to store the evaluated tangent
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_tcmd_evalcurvetangent(char *fname, ay_nurbcurve_object *nc,
			 double u, int relative, double *t)
{
 int ay_status = AY_OK;
 double ru, T[6];
 char *fpname[2] = {fname, "u"};

  ay_status = ay_knots_evalparam(fname?fpname:NULL, u, relative,
				 nc->length, nc->order,
				 nc->knotv, &ru);

  if(ay_status)
    return ay_status;

  /* evaluate the curve */
  if(nc->is_rat)
    {
      ay_nb_FirstDer4D(nc->length-1, nc->order-1,
		       nc->knotv, nc->controlv,
		       ru, T);
    }
  else
    {
      ay_nb_FirstDer3D(nc->length-1, nc->order-1,
		       nc->knotv, nc->controlv,
		       ru, T);
    }

  memcpy(t, &(T[3]), 3*sizeof(double));

 return AY_OK;
} /* ay_tcmd_evalcurvetangent */


/** ay_tcmd_evalsurface:
 * Helper for ay_tcmd_getpointtcmd/"getPnt" below.
 * Evaluate a NURBS surface.
 *
 * \param[in] fname  a name for error reporting purposes
 * \param[in] np  the surface to evaluate
 * \param[in] u  parametric value where to evaluate the surface
 * \param[in] v  parametric value where to evaluate the surface
 * \param[in] relative  interpret \a u and \a v in a relative way?
 *
 * \param[in,out] p  where to store the evaluated point coordinates
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_tcmd_evalsurface(char *fname, ay_nurbpatch_object *np, double u, double v,
		    int relative, double *p)
{
 int ay_status = AY_OK;
 double ru, rv;
 char *fpname[2] = {fname, "u"};

 ay_status = ay_knots_evalparam(fname?fpname:NULL, u, relative,
				np->width, np->uorder,
				np->uknotv, &ru);

  if(ay_status)
    return ay_status;

  fpname[1] = "v";

  ay_status = ay_knots_evalparam(fname?fpname:NULL, v, relative,
				 np->height, np->vorder,
				 np->vknotv, &rv);

  if(ay_status)
    return ay_status;

  /* evaluate the patch */
  if(np->is_rat)
    {
      ay_status = ay_nb_SurfacePoint4D(np->width-1, np->height-1,
				       np->uorder-1, np->vorder-1,
				       np->uknotv, np->vknotv,
				       np->controlv, ru, rv, p);
    }
  else
    {
      ay_status = ay_nb_SurfacePoint3D(np->width-1, np->height-1,
				       np->uorder-1, np->vorder-1,
				       np->uknotv, np->vknotv,
				       np->controlv, ru, rv, p);
    }

  if(ay_status)
    {
      ay_error(AY_ERROR, fname, "Evaluation failed.");
    }

 return ay_status;
} /* ay_tcmd_evalsurface */


/** ay_tcmd_evalsurfacenormtan:
 * Helper for ay_tcmd_getnormaltcmd/"getNormal" and
 * ay_tcmd_gettangenttcmd/"getTangent" below.
 * Evaluate a NURBS surface normal or tangents.
 *
 * \param[in] fname  a name for error reporting purposes
 * \param[in] np  the surface to evaluate
 * \param[in] normtan  normal (0) or tangents (1)
 * \param[in] u  parametric value where to evaluate the surface
 * \param[in] v  parametric value where to evaluate the surface
 * \param[in] relative  interpret \a u and \a v in a relative way?
 *
 * \param[in,out] n  where to store the evaluated normal or tangents
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_tcmd_evalsurfacenormtan(char *fname, ay_nurbpatch_object *np,
			   int normtan, double u, double v,
			   int relative, double *n)
{
 int ay_status = AY_OK;
 double ru, rv;
 double *fd1, *fd2, l, fder[16] = {0};
 char *fpname[2] = {fname, "u"};

 ay_status = ay_knots_evalparam(fname?fpname:NULL, u, relative,
				np->width, np->uorder,
				np->uknotv, &ru);

  if(ay_status)
    return ay_status;

  fpname[1] = "v";

  ay_status = ay_knots_evalparam(fname?fpname:NULL, v, relative,
				 np->height, np->vorder,
				 np->vknotv, &rv);

  if(ay_status)
    return ay_status;

  /* evaluate the patch */
  if(np->is_rat)
    {
      ay_nb_FirstDerSurf4D(np->width-1, np->height-1,
			   np->uorder-1, np->vorder-1,
			   np->uknotv, np->vknotv, np->controlv,
			   ru, rv,
			   fder);
    }
  else
    {
      ay_nb_FirstDerSurf3D(np->width-1, np->height-1,
			   np->uorder-1, np->vorder-1,
			   np->uknotv, np->vknotv, np->controlv,
			   ru, rv,
			   fder);
    }

  if(normtan)
    {
      memcpy(n, &(fder[3]), 6*sizeof(double));
    }
  else
    {
      fd1 = &(fder[3]);
      fd2 = &(fder[6]);

      AY_V3CROSS(n, fd2, fd1);
      l = AY_V3LEN(n);
      if(fabs(l) > AY_EPSILON)
	AY_V3SCAL(n, 1.0/l);
    }

 return ay_status;
} /* ay_tcmd_evalsurfacenormtan */


/** ay_tcmd_getpointtcmd:
 *  get points of selected objects
 *  Implements the \a getPnt scripting interface command.
 *  See also the corresponding section in the \ayd{scgetpnt}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_tcmd_getpointtcmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_point *old_selp = NULL, *selp = NULL;
 ay_nurbcurve_object *nc = NULL;
 ay_nurbpatch_object *np = NULL;
 ay_object *o = NULL, *po = NULL;
 int indexu = 0, indexv = 0, i = 1, j = 1, argc2 = argc;
 int rational = AY_FALSE, apply_trafo = AY_FALSE, relative = AY_FALSE;
 int to_world = AY_FALSE, eval = AY_FALSE, vn = AY_FALSE;
 int return_result = AY_FALSE, handled = AY_FALSE;
 double *p = NULL, *tp = NULL, tmp[4] = {0}, utmp[4] = {0};
 double m[16], u = 0.0, v = 0.0;
 char fargs[] = "[-trafo|-world|-eval|-relative] (index | indexu indexv | u | u v ([varx vary varz [varw]] | -vn [varname]) | -all [varname] | -sel [varname])";
 Tcl_Obj *to = NULL, *res = NULL;
 int lflags = TCL_LEAVE_ERR_MSG | TCL_APPEND_VALUE | TCL_LIST_ELEMENT;
 ay_voidfp *arr = NULL;
 ay_getpntcb *cb = NULL;

  if(argc <= 1)
    {
      ay_error(AY_EARGS, argv[0], fargs);
      return TCL_OK;
    }

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  while((j < 4) && (j < argc))
    {
      if(argv[j][0] == '-' && argv[j][1] == 't')
	{
	  /* -trafo */
	  apply_trafo = AY_TRUE;
	  i++;
	  argc2--;
	}
      if(argv[j][0] == '-' && argv[j][1] == 'w')
	{
	  /* -world */
	  apply_trafo = AY_TRUE;
	  to_world = AY_TRUE;
	  i++;
	  argc2--;
	}
      if(argv[j][0] == '-' && argv[j][1] == 'e')
	{
	  /* -eval */
	  eval = AY_TRUE;
	  i++;
	  argc2--;
	}
      if(argv[j][0] == '-' && argv[j][1] == 'r')
	{
	  /* -relative */
	  relative = AY_TRUE;
	  i++;
	  argc2--;
	}
      /* provided for backwards compatibility... */
      if(argv[j][0] == '-' && argv[j][1] == 'p')
	{
	  /* -param */
	  eval = AY_TRUE;
	  i++;
	  argc2--;
	}
      j++;
    } /* while */

  if((i < argc) && (argv[i][0] == '-') &&
     ((argv[i][1] == 'a') || (argv[i][1] == 's')))
    {
      /* -all | -sel */
      j = 0;
      if(to_world)
	j = 2;
      else
	if(apply_trafo)
	  j = 1;
      if(argc2 <= 2)
	return ay_tcmd_getselpoints(interp, argv[0], NULL,
				    (argv[i][1] == 'a')?1:0, j);
      else
	return ay_tcmd_getselpoints(interp, argv[0], argv[argc-1],
				    (argv[i][1] == 'a')?1:0, j);
    }

  if((argc2 > 0) && (argv[argc-2][0] == '-') && (argv[argc-2][1] == 'v'))
    {
      /* -vn */
      if(argc2 < 2)
	{
	  ay_error(AY_EARGS, argv[0], fargs);
	  goto cleanup;
	}
      vn = AY_TRUE;
    }

  /* check for presence of atleast one index/parameter for _all_
     supported object types */
  if(i >= argc)
    {
      ay_error(AY_EARGS, argv[0], fargs);
      goto cleanup;
    }

  j = 0;
  while(sel)
    {
      o = sel->object;
      p = NULL;
      rational = AY_FALSE;
      handled = AY_FALSE;

      switch(o->type)
	{
	case AY_IDNCURVE:
	  if(!vn && (argc2+eval < 6))
	    {
	      return_result = AY_TRUE;
	    }
	  if(!eval)
	    {
	      tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      ay_nct_getpntfromindex((ay_nurbcurve_object*)(o->refine),
				     indexu, &p);
	      rational = AY_TRUE;
	    }
	  else
	    {
	      tcl_status = Tcl_GetDouble(interp, argv[i], &u);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      p = utmp;
	      nc = (ay_nurbcurve_object *)(o->refine);
	      ay_status = ay_tcmd_evalcurve(argv[0], nc, u, relative, p);
	      if(ay_status)
		goto cleanup;
	      rational = AY_FALSE;
	    } /* if */
	  j = i+1;
	  break;

	case AY_IDACURVE:
	  if(!vn && (argc2 < 5))
	    {
	      return_result = AY_TRUE;
	    }
	  if(eval)
	    goto eval_provided_curve;
	  tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  ay_act_getpntfromindex((ay_acurve_object*)(o->refine),
				 indexu, &p);
	  rational = AY_FALSE;
	  j = i+1;
	  break;

	case AY_IDICURVE:
	  if(!vn && (argc2 < 5))
	    {
	      return_result = AY_TRUE;
	    }
	  if(eval)
	    goto eval_provided_curve;
	  tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  ay_ict_getpntfromindex((ay_icurve_object*)(o->refine),
				 indexu, &p);
	  rational = AY_FALSE;
	  j = i+1;
	  break;

	case AY_IDNPATCH:
	  if(!vn && (argc2+eval < 7))
	    {
	      return_result = AY_TRUE;
	    }
	  if(i+1 >= argc)
	    {
	      /* XXXX rewrite 1D index to indexu/indexv pair? */
	      ay_error(AY_EARGS, argv[0], fargs);
	      goto cleanup;
	    }
	  if(!eval)
	    {
	      tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      tcl_status = Tcl_GetInt(interp, argv[i+1], &indexv);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      ay_npt_getpntfromindex((ay_nurbpatch_object*)(o->refine),
				     indexu, indexv, &p);
	      rational = AY_TRUE;
	    }
	  else
	    {
	      tcl_status = Tcl_GetDouble(interp, argv[i], &u);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      tcl_status = Tcl_GetDouble(interp, argv[i+1], &v);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      p = utmp;
	      np = (ay_nurbpatch_object *)(o->refine);
	      ay_status = ay_tcmd_evalsurface(argv[0], np, u, v, relative, p);
	      if(ay_status)
		goto cleanup;
	      rational = AY_FALSE;
	    } /* if */
	  j = i+2;
	  break;

	case AY_IDIPATCH:
	  if(!vn && (argc2 < 6))
	    {
	      return_result = AY_TRUE;
	    }
	  if(i+1 >= argc)
	    {
	      /* XXXX rewrite 1D index to indexu/indexv pair? */
	      ay_error(AY_EARGS, argv[0], fargs);
	      goto cleanup;
	    }
	  if(eval)
	    goto eval_provided_surface;
	  tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  tcl_status = Tcl_GetInt(interp, argv[i+1], &indexv);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  ay_ipt_getpntfromindex((ay_ipatch_object*)(o->refine),
				 indexu, indexv, &p);
	  rational = AY_FALSE;
	  j = i+2;
	  break;

	case AY_IDAPATCH:
	  if(!vn && (argc2 < 6))
	    {
	      return_result = AY_TRUE;
	    }
	  if(i+1 >= argc)
	    {
	      /* XXXX rewrite 1D index to indexu/indexv pair? */
	      ay_error(AY_EARGS, argv[0], fargs);
	      goto cleanup;
	    }
	  if(eval)
	    goto eval_provided_surface;
	  tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  tcl_status = Tcl_GetInt(interp, argv[i+1], &indexv);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  ay_apt_getpntfromindex((ay_apatch_object*)(o->refine),
				 indexu, indexv, &p);
	  rational = AY_FALSE;
	  j = i+2;
	  break;

	case AY_IDBPATCH:
	  if(!vn && (argc2+eval < 5))
	    {
	      return_result = AY_TRUE;
	    }
	  if(eval)
	    goto eval_provided_surface;
	  tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  ay_tcmd_getbppntfromindex((ay_bpatch_object*)(o->refine),
				    indexu, &p);
	  rational = AY_FALSE;
	  j = i+1;
	  break;

	case AY_IDPAMESH:
	  if(!vn && (argc2+eval < 7))
	    {
	      return_result = AY_TRUE;
	    }
	  if(i+1 >= argc)
	    {
	      /* XXXX rewrite 1D index to indexu/indexv pair? */
	      ay_error(AY_EARGS, argv[0], fargs);
	      goto cleanup;
	    }
	  if(eval)
	    goto eval_provided_surface;
	  tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  tcl_status = Tcl_GetInt(interp, argv[i+1], &indexv);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  ay_pmt_getpntfromindex((ay_pamesh_object*)(o->refine),
				 indexu, indexv, &p);
	  rational = AY_TRUE;
	  j = i+2;
	  break;

	default:

	  if(ay_provide_object(o, AY_IDNCURVE, NULL) == AY_OK
	     /*(!vn && argc2+eval < 7) || (vn && argc2+eval < 5)*/)
	    {
eval_provided_curve:
	      po = ay_peek_singleobject(o, AY_IDNCURVE);
	      if(po)
		{
		  if(!vn && (argc2+eval < 6))
		    {
		      return_result = AY_TRUE;
		    }
		  if(!eval)
		    {
		      tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
		      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
		      ay_status = ay_nct_getpntfromindex(
					  (ay_nurbcurve_object*) (po->refine),
					  indexu, &p);
		      if(ay_status)
			goto cleanup;
		      rational = AY_TRUE;
		    }
		  else
		    {
		      tcl_status = Tcl_GetDouble(interp, argv[i], &u);
		      AY_CHTCLERRGOT(tcl_status, argv[0], interp);

		      p = utmp;
		      nc = (ay_nurbcurve_object *)(po->refine);
		      ay_status = ay_tcmd_evalcurve(argv[0], nc, u,
						    relative, p);
		      if(ay_status)
			goto cleanup;
		      rational = AY_FALSE;
		    } /* if */
		  j = i+1;
		  handled = AY_TRUE;
		} /* if have provided object */
	    }
	  else
	    {
eval_provided_surface:
	      po = ay_peek_singleobject(o, AY_IDNPATCH);
	      if(po)
		{
		  if(!vn && (argc2+eval < 7))
		    {
		      return_result = AY_TRUE;
		    }
		  if(i+1 >= argc)
		    {
		      ay_error(AY_EARGS, argv[0], fargs);
		      goto cleanup;
		    }
		  if(!eval)
		    {
		      tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
		      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
		      tcl_status = Tcl_GetInt(interp, argv[i+1], &indexv);
		      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
		      ay_status = ay_npt_getpntfromindex(
					  (ay_nurbpatch_object*) (po->refine),
					  indexu, indexv, &p);
		      if(ay_status)
			goto cleanup;
		      rational = AY_TRUE;
		    }
		  else
		    {
		      tcl_status = Tcl_GetDouble(interp, argv[i], &u);
		      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
		      tcl_status = Tcl_GetDouble(interp, argv[i+1], &v);
		      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
		      p = utmp;
		      np = (ay_nurbpatch_object *)(po->refine);
		      ay_status = ay_tcmd_evalsurface(argv[0], np, u, v,
						      relative, p);
		      if(ay_status)
			goto cleanup;
		      rational = AY_FALSE;
		    } /* if */
		  j = i+2;
		  handled = AY_TRUE;
		} /* if have provided object */
	    } /* if curve or surface */

	  if(!handled && !eval)
	    {
	      arr = ay_getpntcbt.arr;
	      cb = (ay_getpntcb *)(arr[o->type]);
	      if(cb)
		{
		  if(!(selp = calloc(1, sizeof(ay_point))))
		    {
		      ay_error(AY_EOMEM, argv[0], NULL);
		      goto cleanup;
		    }
		  old_selp = o->selp;
		  o->selp = selp;
		  ay_status = ay_tcmd_getuint(argv[i], &selp->index);
		  if(ay_status)
		    {
		      o->selp = old_selp;
		      free(selp);
		      selp = NULL;
		      goto cleanup;
		    }
		  ay_status = cb(3, o, NULL, NULL);
		  if(ay_status || (!o->selp))
		    {
		      o->selp = old_selp;
		      free(selp);
		      selp = NULL;
		      ay_error(AY_ERROR, argv[0], "getpntcb failed");
		      goto cleanup;
		    }
		  p = selp->point;
		  rational = selp->type;
		  free(selp);
		  o->selp = old_selp;
		  handled = AY_TRUE;
		  j = i+1;
		} /* if */
	    } /* if */

	  if(!handled)
	    {
	      ay_error(AY_EWARN, argv[0],
		       "do not know how to get point from this object");
	      /* XXXX specify object? */
	    }
	  else
	    {
	      if(j >= argc)
		return_result = AY_TRUE;
	    }
	  break;
	} /* switch */

      if(p)
	{
	  /* apply trafos? */
	  if(apply_trafo)
	    { /* Yes! */
	      if(to_world)
		{
		  ay_trafo_identitymatrix(m);
		  ay_trafo_getall(ay_currentlevel, o, m);
		}
	      else
		{
		  ay_trafo_creatematrix(o, m);
		}
	      memcpy(tmp, p, 3*sizeof(double));
	      ay_trafo_apply3(tmp, m);
	      if(rational)
		{
		  tmp[3] = p[3];
		}

	      tp = tmp;
	    }
	  else
	    { /* No! */
	      tp = p;
	    }

	  if(!return_result)
	    {
	      if(vn)
		{
		  /* -vn */
		  if(!argv[argc-1])
		    goto cleanup;
		  to = Tcl_NewDoubleObj(tp[0]);
		  Tcl_SetVar2Ex(interp, argv[argc-1], NULL, to, lflags);
		  to = Tcl_NewDoubleObj(tp[1]);
		  Tcl_SetVar2Ex(interp, argv[argc-1], NULL, to, lflags);
		  to = Tcl_NewDoubleObj(tp[2]);
		  Tcl_SetVar2Ex(interp, argv[argc-1], NULL, to, lflags);
		  if(rational)
		    {
		      to = Tcl_NewDoubleObj(tp[3]);
		      Tcl_SetVar2Ex(interp, argv[argc-1], NULL, to, lflags);
		    }
		}
	      else
		{
		  to = Tcl_NewDoubleObj(tp[0]);
		  if(argv[j])
		    Tcl_SetVar2Ex(interp, argv[j], NULL, to,
				  TCL_LEAVE_ERR_MSG);

		  to = Tcl_NewDoubleObj(tp[1]);
		  if(argv[j+1])
		    Tcl_SetVar2Ex(interp, argv[j+1], NULL, to,
				  TCL_LEAVE_ERR_MSG);

		  to = Tcl_NewDoubleObj(tp[2]);
		  if(argv[j+2])
		    Tcl_SetVar2Ex(interp, argv[j+2], NULL, to,
				  TCL_LEAVE_ERR_MSG);

		  if(rational)
		    {
		      to = Tcl_NewDoubleObj(tp[3]);
		      if(argv[j+3])
			Tcl_SetVar2Ex(interp, argv[j+3], NULL, to,
				      TCL_LEAVE_ERR_MSG);
		    }
		} /* if */
	    }
	  else
	    {
	      /* return result */
	      if(!res)
		res = Tcl_NewListObj(0, NULL);
	      if(res)
		{
		  to = Tcl_NewDoubleObj(tp[0]);
		  Tcl_ListObjAppendElement(interp, res, to);
		  to = Tcl_NewDoubleObj(tp[1]);
		  Tcl_ListObjAppendElement(interp, res, to);
		  to = Tcl_NewDoubleObj(tp[2]);
		  Tcl_ListObjAppendElement(interp, res, to);
		  if(rational)
		    {
		      to = Tcl_NewDoubleObj(tp[3]);
		      Tcl_ListObjAppendElement(interp, res, to);
		    }
		}
	    } /* if return result */
	} /* if have point to output */

      sel = sel->next;
    } /* while */

cleanup:

  if(res)
    {
      Tcl_SetObjResult(interp, res);
    }

 return TCL_OK;
} /* ay_tcmd_getpointtcmd */


/** ay_tcmd_getnormaltcmd:
 *  get normal of selected objects
 *  Implements the \a getNormal scripting interface command.
 *  See also the corresponding section in the \ayd{scgetnormal}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_tcmd_getnormaltcmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_nurbpatch_object *np = NULL;
 ay_pomesh_object *pm = NULL;
 ay_object *o = NULL, *po = NULL;
 int indexu = 0, indexv = 0, i = 1, j = 1, k, remargc = argc;
 unsigned int index;
 int relative = AY_FALSE, eval = AY_TRUE, vn = AY_FALSE;
 int return_result = AY_FALSE, handled = AY_FALSE;
 double *n = NULL, N[3];
 double u = 0.0, v = 0.0;
 char fargs[] = "[-cv | -relative] (index | indexu indexv | u v) [(varx vary varz | -vn varname)]";
 Tcl_Obj *to = NULL, *res = NULL;
 int lflags = TCL_LEAVE_ERR_MSG | TCL_APPEND_VALUE | TCL_LIST_ELEMENT;

  if(argc <= 1)
    {
      ay_error(AY_EARGS, argv[0], fargs);
      return TCL_OK;
    }

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  while((j < 4) && (j < argc))
    {
      if(argv[j][0] == '-' && argv[j][1] == 'c')
	{
	  /* -cv */
	  eval = AY_FALSE;
	  i++;
	  remargc--;
	}
      if(argv[j][0] == '-' && argv[j][1] == 'r')
	{
	  /* -relative */
	  relative = AY_TRUE;
	  i++;
	  remargc--;
	}
      j++;
    } /* while */

  if((remargc > 0) && (argv[argc-2][0] == '-') && (argv[argc-2][1] == 'v'))
    {
      /* -vn */
      if(remargc < 2)
	{
	  ay_error(AY_EARGS, argv[0], fargs);
	  goto cleanup;
	}
      vn = AY_TRUE;
    }

  /* check for presence of atleast one index/parameter for _all_
     supported object types */
  if(i >= argc)
    {
      ay_error(AY_EARGS, argv[0], fargs);
      goto cleanup;
    }

  j = 0;
  while(sel)
    {
      o = sel->object;
      n = NULL;

      switch(o->type)
	{
	case AY_IDPOMESH:
	  if(!vn && (remargc < 5))
	    return_result = AY_TRUE;
	  pm = (ay_pomesh_object *)o->refine;
	  if(pm->has_normals)
	    {
	      index = 0;
	      ay_status = ay_tcmd_getuint(argv[i], &index);
	      if(ay_status)
		goto cleanup;
	      if(index <= pm->ncontrols)
		n = &(pm->controlv[index*6+3]);
	      else
		ay_error(AY_ERROR, argv[0], "index out of range");
	    }
	  else
	    {
	      ay_error(AY_EWARN, argv[0], "polymesh has no vertex normals");
	    }
	  break;
	case AY_IDNPATCH:
	  if(!vn && (remargc < 6))
	    return_result = AY_TRUE;
	  if(i+1 >= argc)
	    {
	      /* XXXX rewrite 1D index to indexu/indexv pair? */
	      ay_error(AY_EARGS, argv[0], fargs);
	      goto cleanup;
	    }

	  np = (ay_nurbpatch_object *)(o->refine);
	  if(!eval)
	    {
	      tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      tcl_status = Tcl_GetInt(interp, argv[i+1], &indexv);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);

	      ay_npt_getcvnormal(np, indexu, indexv, n);
	    }
	  else
	    {
	      tcl_status = Tcl_GetDouble(interp, argv[i], &u);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      tcl_status = Tcl_GetDouble(interp, argv[i+1], &v);
	      AY_CHTCLERRRET(tcl_status, argv[0], interp);
	      memset(N, 0, 3*sizeof(double));
	      ay_status = ay_tcmd_evalsurfacenormtan(argv[0], np, 0, u, v,
						     relative, N);
	      if(ay_status)
		goto cleanup;
	      n = N;
	    } /* if */
	  j = i+2;
	  break;

	default:
	  handled = AY_FALSE;

	  po = ay_peek_singleobject(o, AY_IDNPATCH);
	  if(po)
	    {
	      if(!vn && (remargc < 6))
		{
		  return_result = AY_TRUE;
		}
	      if(i+1 >= argc)
		{
		  ay_error(AY_EARGS, argv[0], fargs);
		  goto cleanup;
		}
	      np = (ay_nurbpatch_object *)(po->refine);
	      if(!eval)
		{
		  tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
		  AY_CHTCLERRGOT(tcl_status, argv[0], interp);
		  tcl_status = Tcl_GetInt(interp, argv[i+1], &indexv);
		  AY_CHTCLERRGOT(tcl_status, argv[0], interp);
		  ay_npt_getcvnormal(np, indexu, indexv, n);
		}
	      else
		{
		  tcl_status = Tcl_GetDouble(interp, argv[i], &u);
		  AY_CHTCLERRGOT(tcl_status, argv[0], interp);
		  tcl_status = Tcl_GetDouble(interp, argv[i+1], &v);
		  AY_CHTCLERRGOT(tcl_status, argv[0], interp);
		  memset(N, 0, 3*sizeof(double));
		  ay_status = ay_tcmd_evalsurfacenormtan(argv[0], np, 0, u, v,
							 relative, N);
		  if(ay_status)
		    goto cleanup;
		  n = N;
		} /* if */
	      j = i+2;
	      handled = AY_TRUE;
	    } /* if have provided object */

	  if(!handled)
	    {
	      po = ay_peek_singleobject(o, AY_IDPOMESH);
	      if(po)
		{
		  if(!vn && (remargc < 5))
		    {
		      return_result = AY_TRUE;
		    }
		  pm = (ay_pomesh_object *)po->refine;
		  if(pm->has_normals)
		    {
		      index = 0;
		      ay_status = ay_tcmd_getuint(argv[i], &index);
		      if(ay_status)
			goto cleanup;
		      if(index <= pm->ncontrols)
			n = &(pm->controlv[index*6+3]);
		      else
			ay_error(AY_ERROR, argv[0], "index out of range");
		    }
		  else
		    {
		      ay_error(AY_EWARN, argv[0],
			       "polymesh has no vertex normals");
		    }
		  handled = AY_TRUE;
		}
	    }

	  if(!handled)
	    {
	      ay_error(AY_EWARN, argv[0],
		       "do not know how to get normal from this object");
	    }
	  else
	    {
	      if(j >= argc)
		return_result = AY_TRUE;
	    }
	  break;
	} /* switch */

      if(n)
	{
	  if(!return_result)
	    {
	      if(vn)
		{
		  /* -vn */
		  if(!argv[argc-1])
		    goto cleanup;
		  for(k = 0; k < 3; k++)
		    {
		      to = Tcl_NewDoubleObj(n[k]);
		      Tcl_SetVar2Ex(interp, argv[argc-1], NULL, to, lflags);
		    }
		}
	      else
		{
		  for(k = 0; k < 3; k++)
		    {
		      to = Tcl_NewDoubleObj(n[k]);
		      if(argv[j+k])
			Tcl_SetVar2Ex(interp, argv[j+k], NULL, to,
				      TCL_LEAVE_ERR_MSG);
		    }
		} /* if */
	    }
	  else
	    {
	      /* return result */
	      if(!res)
		{
		  res = Tcl_NewListObj(0, NULL);
		}
	      if(res)
		{
		  for(k = 0; k < 3; k++)
		    {
		      to = Tcl_NewDoubleObj(n[k]);
		      Tcl_ListObjAppendElement(interp, res, to);
		    }
		}
	    } /* if return result */
	} /* if have something to output */

      sel = sel->next;
    } /* while */

cleanup:

  if(res)
    {
      Tcl_SetObjResult(interp, res);
    }

 return TCL_OK;
} /* ay_tcmd_getnormaltcmd */


/** ay_tcmd_gettangenttcmd:
 *  get tangent of selected objects
 *  Implements the \a getTangent scripting interface command.
 *  See also the corresponding section in the \ayd{scgettangent}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_tcmd_gettangenttcmd(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_nurbpatch_object *np = NULL;
 ay_nurbcurve_object *nc = NULL;
 ay_object *o = NULL, *po = NULL;
 int i = 1, j = 1, k, l, remargc = argc;
 int relative = AY_FALSE, vn = AY_FALSE;
 int return_result = AY_FALSE, handled = AY_FALSE;
 double *t = NULL, T[6];
 double u = 0.0, v = 0.0;
 char fargs[] = "[-relative] (u | u v) [(varx vary varz [varu varv varw] | -vn varname)]";
 Tcl_Obj *to = NULL, *res = NULL;
 int lflags = TCL_LEAVE_ERR_MSG | TCL_APPEND_VALUE | TCL_LIST_ELEMENT;

  if(argc <= 1)
    {
      ay_error(AY_EARGS, argv[0], fargs);
      return TCL_OK;
    }

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  while((j < 4) && (j < argc))
    {
      if(argv[j][0] == '-' && argv[j][1] == 'r')
	{
	  /* -relative */
	  relative = AY_TRUE;
	  i++;
	  remargc--;
	}
      j++;
    } /* while */

  if((remargc > 0) && (argv[argc-2][0] == '-') && (argv[argc-2][1] == 'v'))
    {
      /* -vn */
      if(remargc < 2)
	{
	  ay_error(AY_EARGS, argv[0], fargs);
	  goto cleanup;
	}
      vn = AY_TRUE;
    }

  /* check for presence of atleast one parameter for _all_
     supported object types */
  if(i >= argc)
    {
      ay_error(AY_EARGS, argv[0], fargs);
      goto cleanup;
    }

  j = 0;
  while(sel)
    {
      o = sel->object;
      t = NULL;

      switch(o->type)
	{
	case AY_IDNCURVE:
	  if(!vn && (remargc < 5))
	    return_result = AY_TRUE;
	  nc = (ay_nurbcurve_object *)(o->refine);

	  tcl_status = Tcl_GetDouble(interp, argv[i], &u);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  memset(T, 0, 3*sizeof(double));
	  ay_status = ay_tcmd_evalcurvetangent(argv[0], nc, u, relative, T);
	  t = T;
	  l = 3;
	  j = i+1;
	  break;
	case AY_IDNPATCH:
	  if(!vn && (remargc < 9))
	    return_result = AY_TRUE;
	  if(i+1 >= argc)
	    {
	      /* XXXX rewrite 1D index to indexu/indexv pair? */
	      ay_error(AY_EARGS, argv[0], fargs);
	      goto cleanup;
	    }

	  np = (ay_nurbpatch_object *)(o->refine);

	  tcl_status = Tcl_GetDouble(interp, argv[i], &u);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  tcl_status = Tcl_GetDouble(interp, argv[i+1], &v);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	  memset(T, 0, 6*sizeof(double));
	  ay_status = ay_tcmd_evalsurfacenormtan(argv[0], np, 1, u, v,
						 relative, T);
	  if(ay_status)
	    goto cleanup;
	  t = T;
	  l = 6;
	  j = i+2;
	  break;

	default:
	  handled = AY_FALSE;
	  po = ay_peek_singleobject(o, AY_IDNCURVE);
	  if(po)
	    {
	      if(!vn && (remargc < 5))
		return_result = AY_TRUE;
	      if(i+1 >= argc)
		{
		  ay_error(AY_EARGS, argv[0], fargs);
		  goto cleanup;
		}
	      nc = (ay_nurbcurve_object *)(po->refine);

	      tcl_status = Tcl_GetDouble(interp, argv[i], &u);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);

	      memset(T, 0, 3*sizeof(double));
	      ay_status = ay_tcmd_evalcurvetangent(argv[0], nc, u,
						   relative, T);
	      if(ay_status)
		goto cleanup;
	      t = T;
	      l = 3;
	      j = i+1;
	      handled = AY_TRUE;
	    } /* if have provided object */

	  if(!handled)
	    {
	      po = ay_peek_singleobject(o, AY_IDNPATCH);
	      if(po)
		{
		  if(!vn && (remargc < 9))
		    return_result = AY_TRUE;
		  if(i+1 >= argc)
		    {
		      ay_error(AY_EARGS, argv[0], fargs);
		      goto cleanup;
		    }
		  np = (ay_nurbpatch_object *)(po->refine);

		  tcl_status = Tcl_GetDouble(interp, argv[i], &u);
		  AY_CHTCLERRGOT(tcl_status, argv[0], interp);
		  tcl_status = Tcl_GetDouble(interp, argv[i+1], &v);
		  AY_CHTCLERRGOT(tcl_status, argv[0], interp);
		  memset(T, 0, 6*sizeof(double));
		  ay_status = ay_tcmd_evalsurfacenormtan(argv[0], np, 1, u, v,
							 relative, T);
		  if(ay_status)
		    goto cleanup;
		  t = T;
		  l = 6;
		  j = i+2;
		  handled = AY_TRUE;
		} /* if have provided object */
	    } /* if !handled */

	  if(!handled)
	    {
	      ay_error(AY_EWARN, argv[0],
		       "do not know how to get tangent from this object");
	    }
	  break;
	} /* switch */

      if(t)
	{
	  if(!return_result)
	    {
	      if(vn)
		{
		  /* -vn */
		  if(!argv[argc-1])
		    goto cleanup;
		  for(k = 0; k < l; k++)
		    {
		      to = Tcl_NewDoubleObj(t[k]);
		      Tcl_SetVar2Ex(interp, argv[argc-1], NULL, to, lflags);
		    }
		}
	      else
		{
		  for(k = 0; k < l; k++)
		    {
		      to = Tcl_NewDoubleObj(t[k]);
		      if(argv[j+k])
			Tcl_SetVar2Ex(interp, argv[j+k], NULL, to,
				      TCL_LEAVE_ERR_MSG);
		    }
		} /* if */
	    }
	  else
	    {
	      /* return result */
	      if(!res)
		{
		  res = Tcl_NewListObj(0, NULL);
		}
	      if(res)
		{
		  for(k = 0; k < l; k++)
		    {
		      to = Tcl_NewDoubleObj(t[k]);
		      Tcl_ListObjAppendElement(interp, res, to);
		    }
		}
	    } /* if return result */
	} /* if have something to output */

      sel = sel->next;
    } /* while */

cleanup:

  if(res)
    {
      Tcl_SetObjResult(interp, res);
    }

 return TCL_OK;
} /* ay_tcmd_gettangenttcmd */


/* ay_tcmd_setallpoints:
 *  helper for ay_tcmd_setpointtcmd() below
 *  set all points of selected objects
 */
int
ay_tcmd_setallpoints(Tcl_Interp *interp, char *fname, char *vn,
		     int from_world)
{
 int tcl_status = TCL_OK;
 int notify_parent = AY_FALSE;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL;
 ay_pointedit pe = {0};
 int vlen = 0;
 unsigned int i = 0, j = 0;
 double *p = NULL, *v = NULL;
 double pm[16], m[16], mi[16];

  tcl_status = ay_tcmd_convdlist(interp, vn, &vlen, &v);
  AY_CHTCLERRRET(tcl_status, fname, interp);

  if(vlen < 2)
    {
      if(v)
	free(v);
      ay_error(AY_ERROR, fname, "Not enough coordinates provided.");
      return TCL_OK;
    }

  if(from_world)
    {
      ay_trafo_identitymatrix(pm);
      if(ay_currentlevel->object != ay_root)
	{
	  ay_trafo_getparent(ay_currentlevel->next, pm);
	}
    }

  while(sel)
    {
      o = sel->object;

      if(from_world)
	{
	  memcpy(m, pm, 16*sizeof(double));
	  ay_trafo_getall(NULL, o, m);
	  ay_trafo_invmatrix(m, mi);
	} /* if */

      p = m;
      ay_pact_getpoint(0, o, p, &pe);
      if(pe.num && !pe.readonly)
	{
	  if(!(pe.type == AY_PTRAT))
	    {
	      for(i = 0; i < pe.num; i++)
		{
		  p = pe.coords[i];
		  memcpy(p, &(v[j]), 3*sizeof(double));
		  if(from_world)
		    ay_trafo_apply3(p, mi);
		  j += 3;
		  if(j > (unsigned int)vlen)
		    break;
		}
	    }
	  else
	    {
	      /* rational */
	      for(i = 0; i < pe.num; i++)
		{
		  p = pe.coords[i];
		  memcpy(p, &(v[j]), 4*sizeof(double));
		  if(from_world)
		    ay_trafo_apply3(p, mi);
		  j += 4;
		  if(j > (unsigned int)vlen)
		    break;
		}
	    } /* if */

	  (void)ay_notify_object(o);
	  o->modified = AY_TRUE;
	  notify_parent = AY_TRUE;
	} /* if */

      ay_pact_clearpointedit(&pe);

      sel = sel->next;
    } /* while */

  if(notify_parent)
    ay_notify_parent();

  if(v)
    {
      free(v);
    }

 return TCL_OK;
} /* ay_tcmd_setallpoints */


/** ay_tcmd_setpointtcmd:
 *  set points of selected objects
 *  Implements the \a setPnt scripting interface command.
 *  See also the corresponding section in the \ayd{scsetpnt}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_tcmd_setpointtcmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL;
 ay_point *old_selp = NULL, *selp = NULL;
 double dtemp = 0.0;
 int remargc = argc, indexu = 0, indexv = 0, i = 1, j, rational = AY_FALSE;
 int from_world = AY_FALSE, clear_selp = AY_FALSE, handled = AY_FALSE;
 int from_var = AY_FALSE, vlen = 0, notify_parent = AY_FALSE;
 int set_selp = AY_FALSE;
 double *p = NULL, *v = NULL;
 ay_voidfp *arr = NULL;
 ay_getpntcb *cb = NULL;
 double pm[16], m[16], mi[16], t[4];
 char args[] =
   "[-world] (index [indexv] (x y z [w] | -vn varname) | (-all|-sel) varname )";

  if(argc <= 1)
    {
      ay_error(AY_EARGS, argv[0], args);
      return TCL_OK;
    }

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  if(argv[1][0] == '-' && argv[1][1] == 'w')
    {
      /* -world */
      from_world = AY_TRUE;
      i++;
      remargc--;
    }

  if(argv[i][0] == '-' && argv[i][1] == 'a')
    {
      /* -all */
      if(argc < i+1)
	{
	  ay_error(AY_EARGS, argv[0], args);
	  return TCL_OK;
	}
      return ay_tcmd_setallpoints(interp, argv[0], argv[argc-1], from_world);
    }
  else
    {
      if(argv[i][0] == '-' && argv[i][1] == 's')
	{
	  set_selp = AY_TRUE;
	  i++;
	  remargc--;
	}
      /* check for -vn argument */
      if(remargc < 3)
	{
	  ay_error(AY_EARGS, argv[0], args);
	  return TCL_OK;
	}

      if((argc-2 > 1) && (argv[argc-2][0] == '-' && argv[argc-2][1] == 'v'))
	{
	  from_var = AY_TRUE;
	  tcl_status = ay_tcmd_convdlist(interp, argv[argc-1], &vlen, &v);
	  AY_CHTCLERRRET(tcl_status, argv[0], interp);
	}
    }

  if(from_world)
    {
      ay_trafo_identitymatrix(pm);
      if(ay_currentlevel->object != ay_root)
	{
	  ay_trafo_getparent(ay_currentlevel->next, pm);
	}
    }

  while(sel)
    {
      o = sel->object;
      p = NULL;
      rational = AY_FALSE;
      clear_selp = AY_FALSE;

      if(!set_selp)
	{
	  switch(o->type)
	    {
	    case AY_IDNCURVE:
	      remargc--;
	      tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	      ay_nct_getpntfromindex((ay_nurbcurve_object*)(o->refine),
				     indexu, &p);
	      rational = AY_TRUE;
	      i++;
	      break;
	    case AY_IDACURVE:
	      remargc--;
	      tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	      ay_act_getpntfromindex((ay_acurve_object*)(o->refine),
				     indexu, &p);
	      i++;
	      break;
	    case AY_IDICURVE:
	      remargc--;
	      tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	      ay_ict_getpntfromindex((ay_icurve_object*)(o->refine),
				     indexu, &p);
	      i++;
	      break;
	    case AY_IDNPATCH:
	      remargc -= 2;
	      tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	      tcl_status = Tcl_GetInt(interp, argv[i+1], &indexv);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	      ay_npt_getpntfromindex((ay_nurbpatch_object*)(o->refine),
				     indexu, indexv, &p);
	      rational = AY_TRUE;
	      i += 2;
	      break;
	    case AY_IDIPATCH:
	      remargc -= 2;
	      tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	      tcl_status = Tcl_GetInt(interp, argv[i+1], &indexv);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	      ay_ipt_getpntfromindex((ay_ipatch_object*)(o->refine),
				     indexu, indexv, &p);
	      i += 2;
	      break;
	    case AY_IDAPATCH:
	      remargc -= 2;
	      tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	      tcl_status = Tcl_GetInt(interp, argv[i+1], &indexv);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	      ay_apt_getpntfromindex((ay_apatch_object*)(o->refine),
				     indexu, indexv, &p);
	      i += 2;
	      break;
	    case AY_IDBPATCH:
	      remargc--;
	      tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	      ay_tcmd_getbppntfromindex((ay_bpatch_object*)(o->refine),
					indexu, &p);
	      i++;
	      break;
	    case AY_IDPAMESH:
	      remargc -= 2;
	      tcl_status = Tcl_GetInt(interp, argv[i], &indexu);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	      tcl_status = Tcl_GetInt(interp, argv[i+1], &indexv);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	      ay_pmt_getpntfromindex((ay_pamesh_object*)(o->refine),
				     indexu, indexv, &p);
	      rational = AY_TRUE;
	      i += 2;
	      break;
	    default:
	      handled = AY_FALSE;
	      arr = ay_getpntcbt.arr;
	      cb = (ay_getpntcb *)(arr[o->type]);
	      if(cb)
		{
		  if(!(selp = calloc(1, sizeof(ay_point))))
		    {
		      ay_error(AY_EOMEM, argv[0], NULL);
		      goto cleanup;
		    }
		  old_selp = o->selp;
		  o->selp = selp;
		  clear_selp = AY_TRUE;
		  ay_status = ay_tcmd_getuint(argv[i], &selp->index);
		  if(ay_status)
		    {
		      goto cleanup;
		    }
		  ay_status = cb(3, o, NULL, NULL);
		  if(ay_status || (!o->selp))
		    {
		      ay_error(AY_ERROR, argv[0], "getpntcb failed");
		      goto cleanup;
		    }
		  if(o->selp->readonly)
		    {
		      ay_error(AY_ERROR, argv[0], "points are readonly");
		      goto cleanup;
		    }
		  p = selp->point;
		  rational = selp->type;
		  handled = AY_TRUE;
		  i++;
		}
	      if(!handled)
		{
		  ay_error(AY_EWARN, argv[0],
			   "do not know how to set point of this object");
		}
	      break;
	    } /* switch */
	}
      else
	{
	  if(o->selp && o->selp->readonly)
	    {
	      ay_error(AY_ERROR, argv[0], "points are readonly");
	      goto cleanup;
	    }

	  if(o->selp && o->selp->type == AY_PTRAT)
	    {
	      rational = AY_TRUE;
	    }
	  p = t;
	} /* if set_selp */

      if(p)
	{
	  if(!from_var)
	    {
	      if(remargc < 4)
		{
		  ay_error(AY_EARGS, argv[0], args);
		  goto cleanup;
		}
	      tcl_status = Tcl_GetDouble(interp, argv[i], &dtemp);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	      p[0] = dtemp;

	      tcl_status = Tcl_GetDouble(interp, argv[i+1], &dtemp);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	      p[1] = dtemp;

	      tcl_status = Tcl_GetDouble(interp, argv[i+2], &dtemp);
	      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
	      p[2] = dtemp;

	      if(rational)
		{
		  if(remargc > 4)
		    {
		      tcl_status = Tcl_GetDouble(interp, argv[i+3],
						 &dtemp);
		      AY_CHTCLERRGOT(tcl_status, argv[0], interp);
		      p[3] = dtemp;
		    }
		  else
		    {
		      p[3] = 1.0;
		    }
		} /* if */
	    }
	  else
	    {
	      /* from_var */
	      if(!rational)
		{
		  memset(p, 0, 3*sizeof(double));
		  memcpy(p, v, (vlen<3?vlen:3)*sizeof(double));
		}
	      else
		{
		  memset(p, 0, 3*sizeof(double));
		  p[3] = 1.0;
		  memcpy(p, v, (vlen<4?vlen:4)*sizeof(double));
		}
	    } /* if */

	  if(from_world)
	    {
	      memcpy(m, pm, 16*sizeof(double));
	      ay_trafo_getall(NULL, o, m);
	      ay_trafo_invmatrix(m, mi);
	      ay_trafo_apply3(p, mi);
	    } /* if */

	  if(set_selp)
	    {
	      selp = o->selp;

	      j = 0;
	      while(selp)
		{
		  switch(selp->type)
		    {
		    case AY_PT3D:
		      memcpy(selp->point, p, 3*sizeof(double));
		      j += 3;
		      rational = AY_FALSE;
		      break;
		    case AY_PTRAT:
		      memcpy(selp->point, p, 4*sizeof(double));
		      j += 4;
		      rational = AY_TRUE;
		      break;
		    default:
		      ay_error(AY_ERROR, argv[0], "unsupported point type");
		      goto cleanup;
		    }

		  /* fetch next point data from var */
		  if(from_var)
		    {
		      if(j+3+rational < vlen)
			{
			  if(!rational)
			    {
			      memset(p, 0, 3*sizeof(double));
			      memcpy(p, &(v[j]),
				     (vlen<3?vlen:3)*sizeof(double));
			    }
			  else
			    {
			      memset(p, 0, 3*sizeof(double));
			      p[3] = 1.0;
			      memcpy(p, &(v[j]),
				     (vlen<4?vlen:4)*sizeof(double));
			    }

			  if(from_world)
			    {
			      memcpy(m, pm, 16*sizeof(double));
			      ay_trafo_getall(NULL, o, m);
			      ay_trafo_invmatrix(m, mi);
			      ay_trafo_apply3(p, mi);
			    } /* if from world */
			} /* if have enough data */
		    } /* if from var */
		  selp = selp->next;
		} /* while */
	    } /* if set selp */

	  (void)ay_notify_object(o);
	  o->modified = AY_TRUE;
	  notify_parent = AY_TRUE;

	  if(clear_selp)
	    {
	      free(selp);
	      o->selp = old_selp;
	      clear_selp = AY_FALSE;
	    }
	} /* if p */

      sel = sel->next;
    } /* while */

  if(notify_parent)
    ay_notify_parent();

cleanup:

  if(clear_selp)
    {
      free(selp);
      o->selp = old_selp;
    }

  if(v)
    {
      free(v);
    }

 return TCL_OK;
} /* ay_tcmd_setpointtcmd */


/** ay_tcmd_setnormaltcmd:
 *  set normal of selected objects
 *  Implements the \a setNormal scripting interface command.
 *  See also the corresponding section in the \ayd{scsetnormal}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_tcmd_setnormaltcmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_pomesh_object *pm;
 ay_object *o = NULL;
 double dtemp = 0.0;
 int i = 1, j;
 int from_var = AY_FALSE, vlen = 0, notify_parent = AY_FALSE;
 unsigned int index;
 double *n = NULL, *v = NULL;
 char args[] = "index (x y z | -vn varname)";

  if(argc <= 1)
    {
      ay_error(AY_EARGS, argv[0], args);
      return TCL_OK;
    }

  if(!sel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  if(argv[argc-2][0] == '-' && argv[argc-2][1] == 'v')
    {
      from_var = AY_TRUE;
      tcl_status = ay_tcmd_convdlist(interp, argv[argc-1], &vlen, &v);
      AY_CHTCLERRRET(tcl_status, argv[0], interp);
    }

  while(sel)
    {
      o = sel->object;
      n = NULL;

      switch(o->type)
	{
	case AY_IDPOMESH:
	  pm = (ay_pomesh_object *)o->refine;
	  if(pm->has_normals)
	    {
	      index = 0;
	      ay_status = ay_tcmd_getuint(argv[i], &index);
	      if(ay_status)
		{
		  goto cleanup;
		}
	      if(index <= pm->ncontrols)
		n = &(pm->controlv[index*6+3]);
	      else
		ay_error(AY_ERROR, argv[0], "index out of range");
	    }
	  else
	    {
	      ay_error(AY_EWARN, argv[0], "polymesh has no vertex normals");
	    }

	  i++;
	  break;
	default:
	  break;
	} /* switch */

      if(n)
	{
	  if(!from_var)
	    {
	      /* from_args */
	      if(argc < 4)
		{
		  ay_error(AY_EARGS, argv[0], args);
		  goto cleanup;
		}
	      for(j = 0; j < 3; j++)
		{
		  tcl_status = Tcl_GetDouble(interp, argv[i+j], &dtemp);
		  AY_CHTCLERRGOT(tcl_status, argv[0], interp);
		  n[j] = dtemp;
		}
	    }
	  else
	    {
	      /* from_var */
	      memcpy(n, v, 3*sizeof(double));
	    } /* if */

	  (void)ay_notify_object(o);
	  o->modified = AY_TRUE;
	  notify_parent = AY_TRUE;
	} /* if n */

      sel = sel->next;
    } /* while */

  if(notify_parent)
    ay_notify_parent();

cleanup:

  if(v)
    {
      free(v);
    }

 return TCL_OK;
} /* ay_tcmd_setnormaltcmd */


#ifdef AYENABLEWAIT
#include <sys/types.h>
#include <sys/wait.h>
/* ay_tcmd_waitpidtcmd:
 *  this command waits for a spawned process in order to avoid zombies
 *  (code taken from tclUnixPipe.c/Tcl_WaitPid())
 */
int
ay_tcmd_waitpidtcmd(ClientData clientData, Tcl_Interp *interp,
		    int argc, char *argv[])
{
 int pid;
 pid_t real_pid;
 int result;

  if(argc <= 1)
    {
      ay_error(AY_EARGS, argv[0], "pid");
      return TCL_OK;
    }

  Tcl_GetInt(interp, argv[1], &pid);

  real_pid = (pid_t) pid;
  while(1)
    {
      result = (int) waitpid(real_pid, NULL, WUNTRACED);
      if((result != -1) || (errno != EINTR))
	{
	  return TCL_OK;
	}
    } /* while */

 return TCL_OK;
} /* ay_tcmd_waitpidtcmd */

#endif /* AYENABLEWAIT */


#ifdef AYENABLEFEXIT
/** ay_tcmd_fastexittcmd:
 *  this command exits the application (without trying to clean up properly);
 *  it seems that this is the only way to quit Ayam on Mac OS X (at least
 *  using Tcl/Tk8.2.2...)
 */
int
ay_tcmd_fastexittcmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[])
{
  exit(0);
} /* ay_tcmd_fastexittcmd */
#endif /* AYENABLEFEXIT */


/** ay_tcmd_withobtcmd:
 *  execute command(s) for one of the selected objects;
 *  this command modifies the selection before executing
 *  another command of the Ayam scripting interface
 *
 *  Implements the \a withOb scripting interface command.
 *  See also the corresponding section in the \ayd{scwithob}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_tcmd_withobtcmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
 int tcl_status = TCL_OK, ay_status = AY_OK;
 ay_list_object *oldsel = ay_selection, *l = NULL;
 int i = 0, index = 0, commandindex = 3;
 char args[] = "index [do] command";

  if(!oldsel)
    {
      ay_error(AY_ENOSEL, argv[0], NULL);
      return TCL_OK;
    }

  if(argc < 3)
    {
      ay_error(AY_EARGS, argv[0], args);
      return TCL_OK;
    }

  if(strcmp(argv[2], "do"))
    commandindex--;

  if(commandindex >= argc)
    {
      ay_error(AY_EARGS, argv[0], args);
      return TCL_OK;
    }

  tcl_status = Tcl_GetInt(interp, argv[1], &index);
  AY_CHTCLERRRET(tcl_status, argv[0], interp);

  if(index < 0)
    {
      ay_error(AY_ERROR, argv[0], "Index must be positive.");
      return TCL_OK;
    }

  l = oldsel;
  while(l)
    {
      if(i == index)
	{
	  /* found the object with right index =>
	     fake a single object selection */
	  ay_selection = NULL;
	  ay_status = ay_sel_add(l->object, AY_FALSE);
	  if(!ay_status)
	    {
	      /* execute the command */
	      if(argv[commandindex])
		{
		  Tcl_Eval(interp, argv[commandindex]);
		}
	    }
	  /* restore original selection */
	  ay_sel_free(AY_FALSE);
	  ay_selection = oldsel;
	  /* remember, that we found the object */
	  index = -1;
	  /* break the while loop */
	  break;
	} /* if */
      i++;
      l = l->next;
    } /* while */

  /* if index is here not -1, we did not find the object */
  if(index >= 0)
    {
      ay_error(AY_ERROR, argv[0], "Object not found in selection.");
    }
  else
    {
      ay_sel_clean();
    }

 return TCL_OK;
} /* ay_tcmd_withobtcmd */


/* ay_tcmd_getstring:
 *  get a copy of a string from a Tcl variable
 *
 * \param[in] interp Tcl interpreter to use, may be NULL to designate usage
 *  of the main interpreter
 * \param[in] arr array part of variable name
 * \param[in] var name part of variable name
 * \param[in,out] result pointer where to store the new string
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_tcmd_getstring(Tcl_Interp *interp, char *arr, char *var, char **result)
{
 Tcl_Obj *to;
 char *str;
 int len = 0;

  if(!interp)
    interp = ay_interp;

  if(!arr || !var || !result)
    return AY_ENULL;

  to = Tcl_GetVar2Ex(interp, arr, var, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
  if(to)
    {
      str = Tcl_GetStringFromObj(to, &len);
      if(str)
	{
	  if(*result)
	    {
	      if(!strcmp(*result, str))
		{
		  return AY_OK;
		}
	      free(*result);
	      *result = NULL;
	    }
	  if(!(*result = malloc((len+1)*sizeof(char))))
	    {
	      return AY_EOMEM;
	    }
	  memcpy(*result, str, (len+1)*sizeof(char));
	} /* if str */
    } /* if to */

 return AY_OK;
} /* ay_tcmd_getstring */


/** ay_tcmd_getuint:
 *  convert string to unsigned int
 *  conversion errors will be reported to the user via ay_error()
 *
 * \param[in] str string to convert
 * \param[in,out] uint pointer where to store the converted result
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_tcmd_getuint(char *str, unsigned int *uint)
{
 unsigned long int ret;
 char fname[] = "getuint", *p;
 Tcl_DString ds;

  if(!str || !uint)
    return AY_ENULL;

  p = str;
  while(*p)
    {
      if(isdigit(*p))
	break;
      else
	if(*p == '-')
	  goto outputerror;
      p++;
    }

  errno = 0;

  ret = strtoul(str, &p, 0);

  if(p == str)
    {
outputerror:
      Tcl_DStringInit(&ds);
      Tcl_DStringAppend(&ds, "Expected unsigned integer value but got: ", -1);
      Tcl_DStringAppend(&ds, str, -1);
      ay_error(AY_ERROR, fname,  Tcl_DStringValue(&ds));
      return AY_ERROR;
    }

  if(errno != 0)
    {
#ifdef EINVAL
      if(errno == EINVAL)
	{
	  ay_error(AY_ERROR, fname, "Conversion failed.");
	  return AY_ERROR;
	}
#endif
      if(errno == ERANGE && ret == ULONG_MAX)
	{
	  ay_error(AY_ERROR, fname, "Conversion overflow.");
	  return AY_ERROR;
	}

      if(ret == 0)
	{
	  ay_error(AY_ERROR, fname, "Conversion failed.");
	  return AY_ERROR;
	}
    } /* if */

  if(p && (*p != '\0'))
    {
      ay_error(AY_EWARN, fname, "Ignoring trailing characters.");
    }

  *uint = (unsigned int)ret;

 return AY_OK;
} /* ay_tcmd_getuint */


/** ay_tcmd_registerlang:
 *  register a new scripting language
 *
 * \param[in] name name of scripting language
 * \param[in,out] result pointer where to store the corresponding index
 *  in the script evaluation callback table (using ay_table_addcallback())
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_tcmd_registerlang(char *name, unsigned int *result)
{
 int new_item = 0;
 Tcl_HashEntry *entry = NULL;
 static unsigned int langcounter = 0;
 char fname[] = "registerlang";

  /* check, if language is already registered */
  if(Tcl_FindHashEntry(&ay_languagesht, name))
    {
      ay_error(AY_ERROR, fname, "Language is already registered.");
      return AY_ERROR;
    }

  langcounter++;

  entry = Tcl_CreateHashEntry(&ay_languagesht, name, &new_item);
  Tcl_SetHashValue(entry, ay_otype_getpointer(langcounter));

  *result = langcounter;

 return AY_OK;
} /* ay_tcmd_registerlang */


/** ay_tcmd_menustatetcmd:
 *  set action state from menu state; bound to <Map>/<Unmap>
 *  of any menu to switch to faster drawing of views during
 *  menu interactions
 *
 *  Implements the \a menuState scripting interface command.
 *  \returns TCL_OK in any case.
 */
int
ay_tcmd_menustatetcmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
 ay_object *o = ay_root->down;
 ay_view_object *view = NULL;
 int offset = 1;

  if(argc > 1)
    offset = -1;

  while(o)
    {
      if(o->type == AY_IDVIEW)
	{
	  view = (ay_view_object*)o->refine;
	  view->action_state += offset;
	}
      o = o->next;
    } /* while */

 return TCL_OK;
} /* ay_tcmd_menustatetcmd */


/** ay_tcmd_getplanenormaltcmd:
 *  Get plane normal of selected objects (e.g. of a planar curve).
 *  Implements the \a getPlaneNormal scripting interface command.
 *  See also the corresponding section in the \ayd{scgetplanenormal}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_tcmd_getplanenormaltcmd(ClientData clientData, Tcl_Interp *interp,
			   int argc, char *argv[])
{
 int ay_status = AY_OK;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL, *po = NULL;
 ay_nurbcurve_object *nc;
 ay_icurve_object *ic;
 ay_acurve_object *ac;
 ay_pointedit pe = {0};
 double *cv = NULL, *pnt, p[4], normal[3], m[16], len;
 int applytrafo = AY_FALSE, clearcv = AY_FALSE, cvlen, stride = 3;
 unsigned int i;
 int j = 1, isCurve = AY_FALSE;
 Tcl_Obj *to = NULL, *res = NULL;

  while(j < argc)
    {
      if(argv[j][0] == '-' && argv[j][1] == 't')
	{
	  /* -trafo */
	  applytrafo = AY_TRUE;
	}
      j++;
    }

  while(sel)
    {
      o = sel->object;
      isCurve = AY_FALSE;
      po = NULL;

      if((o->type == AY_IDNCURVE) ||
	 (ay_provide_object(o, AY_IDNCURVE, NULL) == AY_OK))
	{
	  isCurve = AY_TRUE;
	  switch(o->type)
	    {
	    case AY_IDNCURVE:
	      nc = (ay_nurbcurve_object*)o->refine;
	      cv = nc->controlv;
	      cvlen = nc->length;
	      if(nc->type == AY_CTCLOSED)
		cvlen--;
	      if(nc->type == AY_CTPERIODIC)
		cvlen -= (nc->order-1);
	      stride = 4;
	      break;
	    case AY_IDICURVE:
	      ic = (ay_icurve_object*)o->refine;
	      cv = ic->controlv;
	      cvlen = ic->length;
	      break;
	    case AY_IDACURVE:
	      ac = (ay_acurve_object*)o->refine;
	      cv = ac->controlv;
	      cvlen = ac->length;
	      break;
	    default:
	      ay_status = ay_pact_getpoint(0, o, p, &pe);
	      if(ay_status)
		return TCL_OK;
	      cvlen = pe.num;
	      if(!(cv = malloc(pe.num*3*sizeof(double))))
		{
		  ay_error(AY_EOMEM, argv[0], NULL);
		  ay_pact_clearpointedit(&pe);
		  return TCL_OK;
		}
	      pnt = cv;
	      for(i = 0; i < pe.num; i++)
		{
		  memcpy(pnt, pe.coords[i], 3*sizeof(double));
		  pnt += 3;
		}
	      clearcv = AY_TRUE;
	      ay_pact_clearpointedit(&pe);
	      break;
	    } /* switch */
	}
      else
	{
	  if(o->type == AY_IDNPATCH)
	    {
	      po = o;
	    }
	  else
	    {
	      po = ay_peek_singleobject(o, AY_IDNPATCH);
	    }
	}

      if((isCurve && cv) || (po && (po->type == AY_IDNPATCH)))
	{
	  /* compute result */
	  if(isCurve)
	    {
	      ay_status = ay_geom_extractmeannormal(cv, cvlen, stride,
						    /*center=*/NULL, normal);
	    }
	  else
	    {
	      if(po && (po->type == AY_IDNPATCH))
		{
		  if((ay_npt_isplanar((ay_nurbpatch_object*)po->refine,
				      normal)) != AY_TRUE)
		    {
		      ay_error(AY_ERROR, argv[0], "Surface is not planar.");
		      ay_status = AY_ERROR;
		      goto cleanup;
		    }
		}
	    } /* if curve or surface */

	  if(ay_status)
	    {
	      ay_error(ay_status, argv[0], NULL);
	      goto cleanup;
	    }

	  if(applytrafo)
	    {
	      if(AY_ISTRAFO(o)/*AY_ISROT(o)*/)
		{
		  /* ay_trafo_creatematrix(o, m); */
		  ay_quat_torotmatrix(o->quat, m);
		  ay_trafo_apply3(normal, m);
		}
	    }

	  len = AY_V3LEN(normal);
	  if(len > AY_EPSILON)
	    AY_V3SCAL(normal, 1.0/len);

	  /* compile result */
	  if(!res)
	    res = Tcl_NewListObj(0, NULL);
	  to = Tcl_NewDoubleObj(normal[0]);
	  Tcl_ListObjAppendElement(interp, res, to);
	  to = Tcl_NewDoubleObj(normal[1]);
	  Tcl_ListObjAppendElement(interp, res, to);
	  to = Tcl_NewDoubleObj(normal[2]);
	  Tcl_ListObjAppendElement(interp, res, to);
	} /* if have cv or po */

cleanup:
      if(clearcv && cv)
	{
	  free(cv);
	  cv = NULL;
	}

      if(ay_status)
	break;

      sel = sel->next;
    } /* while */

  /* return result */
  if(res)
    Tcl_SetObjResult(interp, res);

 return TCL_OK;
} /* ay_tcmd_getplanenormaltcmd */


/** ay_tcmd_openclosetcmd:
 *  Open/close the selected curve objects.
 *  Implements the \a openC scripting interface command.
 *  Also implements the \a closeC scripting interface command.
 *  See also the corresponding section in the \ayd{scopenc}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_tcmd_openclosetcmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
 int ay_status = AY_OK;
 int close = AY_FALSE, notify_parent = AY_FALSE;
 ay_list_object *sel = ay_selection;
 ay_nurbcurve_object *nc = NULL;
 ay_icurve_object *ic = NULL;
 ay_acurve_object *ac = NULL;
 ay_concatnc_object *cc = NULL;
 ay_voidfp *arr = NULL;
 ay_genericcb *cb = NULL;

  if(argv[0][0] == 'c')
    close = AY_TRUE;

  while(sel)
    {
      switch(sel->object->type)
	{
	case AY_IDNCURVE:
	  nc = (ay_nurbcurve_object *)sel->object->refine;
	  if(close)
	    {
	      nc->type = AY_CTCLOSED;
	      ay_status = ay_nct_close(nc);
	    }
	  else
	    {
	      ay_status = ay_nct_open(nc);
	    }
	  if(ay_status)
	    {
	      goto cleanup;
	    }
	  else
	    {
	      if(sel->object->selp)
		ay_selp_clear(sel->object);

	      ay_nct_recreatemp(nc);

	      sel->object->modified = AY_TRUE;

	      /* re-create tesselation of curve */
	      (void)ay_notify_object(sel->object);
	      notify_parent = AY_TRUE;
	    }
	  break;
	case AY_IDICURVE:
	  ic = (ay_icurve_object *)sel->object->refine;
	  if(close)
	    {
	      if(ic->type == AY_CTOPEN)
		{
		  ic->type = AY_CTCLOSED;
		  sel->object->modified = AY_TRUE;

		  /* re-run interpolation */
		  (void)ay_notify_object(sel->object);
		  notify_parent = AY_TRUE;
		}
	    }
	  else
	    {
	      if(ic->type == AY_CTCLOSED)
		{
		  ic->type = AY_CTOPEN;
		  sel->object->modified = AY_TRUE;

		  /* re-run interpolation */
		  (void)ay_notify_object(sel->object);
		  notify_parent = AY_TRUE;
		}
	    }
	  break;
	case AY_IDACURVE:
	  ac = (ay_acurve_object *)sel->object->refine;
	  if(close)
	    {
	      if(!ac->closed)
		{
		  ac->closed = AY_TRUE;
		  sel->object->modified = AY_TRUE;

		  /* re-run approximation */
		  (void)ay_notify_object(sel->object);
		  notify_parent = AY_TRUE;
		}
	    }
	  else
	    {
	      if(ac->closed)
		{
		  ac->closed = AY_FALSE;
		  sel->object->modified = AY_TRUE;

		  /* re-run approximation */
		  (void)ay_notify_object(sel->object);
		  notify_parent = AY_TRUE;
		}
	    }
	  break;
	case AY_IDCONCATNC:
	  cc = (ay_concatnc_object *)sel->object->refine;
	  if(close)
	    {
	      if(!cc->closed)
		{
		  cc->closed = AY_TRUE;
		  sel->object->modified = AY_TRUE;

		  /* re-run concatenation */
		  (void)ay_notify_object(sel->object);
		  notify_parent = AY_TRUE;
		}
	    }
	  else
	    {
	      if(cc->closed)
		{
		  cc->closed = AY_FALSE;
		  sel->object->modified = AY_TRUE;

		  /* re-run concatenation */
		  (void)ay_notify_object(sel->object);
		  notify_parent = AY_TRUE;
		}
	    }
	  break;
	default:
	  if(close)
	    arr = ay_tcmd_closecbt.arr;
	  else
	    arr = ay_tcmd_opencbt.arr;
	  cb = (ay_genericcb *)(arr[sel->object->type]);
	  if(cb)
	    {
	      if(close)
		ay_status = cb(sel->object, AY_OPCLOSE);
	      else
		ay_status = cb(sel->object, AY_OPOPEN);
	      if(!ay_status)
		{
		  ay_notify_object(sel->object);
		  sel->object->modified = AY_TRUE;
		  notify_parent = AY_TRUE;
		}
	      else
		{
		  goto cleanup;
		}
	    }
	  else
	    {
	      ay_error(AY_EWARN, argv[0], ay_error_igntype);
	    }
	  break;
	} /* switch */

      if(ay_status)
	break;

      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

cleanup:

  if(ay_status)
    {
      if(close)
	ay_error(AY_ERROR, argv[0], "Error closing object.");
      else
	ay_error(AY_ERROR, argv[0], "Error opening object.");
    }

 return TCL_OK;
} /* ay_tcmd_openclosetcmd */


/** ay_tcmd_refinecoarsentcmd:
 *  Refine/coarsen selected curves.
 *  Implements the \a refineC scripting interface command.
 *  Also implements the \a coarsenC scripting interface command.
 *  See also the corresponding section in the \ayd{screfinec}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_tcmd_refinecoarsentcmd(ClientData clientData, Tcl_Interp *interp,
			  int argc, char *argv[])
{
 int ay_status = AY_OK;
 ay_voidfp *arr = NULL;
 ay_genericcb *cb = NULL;
 ay_list_object *sel = ay_selection;
 ay_object *o;
 ay_nurbcurve_object *nc;
 ay_icurve_object *ic;
 ay_acurve_object *ac;
 double *Qw;
 int notify_parent = AY_FALSE, coarsen = AY_FALSE, Qwlen;

  /* distinguish between
     refineC and coarsenC */
  if(argv[0][0] == 'c')
    coarsen = AY_TRUE;

  while(sel)
    {
      o = sel->object;
      o->modified = AY_FALSE;

      if(o->selp && o->selp->next)
	{
	  (void)ay_selp_reducetominmax(o);
	}

      switch(o->type)
	{
	case AY_IDNCURVE:
	  nc = (ay_nurbcurve_object *)o->refine;
	  if(coarsen)
	    ay_status = ay_nct_coarsen(nc, o->selp);
	  else
	    ay_status = ay_nct_refinecv(nc, o->selp);
	  if(ay_status)
	    {
	      goto cleanup;
	    }
	  else
	    {
	      o->modified = AY_TRUE;
	    }
	  break;
	case AY_IDICURVE:
	  ic = (ay_icurve_object*)o->refine;
	  Qw = NULL;
	  if(coarsen)
	    ay_status = ay_nct_coarsenarray(ic->controlv, ic->length, 3,
					    o->selp, &Qw, &Qwlen);
	  else
	    ay_status = ay_nct_refinearray(ic->controlv, ic->length, 3,
					   o->selp, &Qw, &Qwlen);
	  if(!ay_status && Qw)
	    {
	      free(ic->controlv);
	      ic->controlv = Qw;
	      ic->length = Qwlen;
	      o->modified = AY_TRUE;
	    }
	  else
	    {
	      goto cleanup;
	    }
	  break;
	case AY_IDACURVE:
	  ac = (ay_acurve_object*)o->refine;
	  Qw = NULL;
	  if(coarsen)
	    ay_status = ay_nct_coarsenarray(ac->controlv, ac->length, 3,
					    o->selp, &Qw, &Qwlen);
	  else
	    ay_status = ay_nct_refinearray(ac->controlv, ac->length, 3,
					   o->selp, &Qw, &Qwlen);
	  if(!ay_status && Qw)
	    {
	      free(ac->controlv);
	      ac->controlv = Qw;
	      ac->length = Qwlen;
	      o->modified = AY_TRUE;
	    }
	  else
	    {
	      goto cleanup;
	    }
	  break;
	default:
	  if(coarsen)
	    arr = ay_tcmd_coarsencbt.arr;
	  else
	    arr = ay_tcmd_refinecbt.arr;
	  cb = (ay_genericcb *)(arr[o->type]);
	  if(cb)
	    {
	      if(coarsen)
		ay_status = cb(o, AY_OPCOARSEN);
	      else
		ay_status = cb(o, AY_OPREFINE);
	      if(!ay_status)
		{
		  ay_notify_object(o);
		  o->modified = AY_TRUE;
		  notify_parent = AY_TRUE;
		}
	      else
		{
		  if(ay_status != AY_EDONOTLINK)
		    goto cleanup;
		  else
		    ay_status = AY_OK;
		}
	    }
	  else
	    {
	      ay_error(AY_EWARN, argv[0], ay_error_igntype);
	    }
	  break;
	} /* switch */

      if(o->modified)
	{
	  if(o->selp)
	    {
	      if(!o->selp->next)
		{
		  ay_selp_clear(o);
		}
	      else
		{
		  ay_status = ay_pact_getpoint(3, o, NULL, NULL);
		}
	    } /* if selp */
	  (void)ay_notify_object(o);
	  notify_parent = AY_TRUE;
	} /* if modified */

      sel = sel->next;
    } /* while */

cleanup:

  if(ay_status)
    {
      if(coarsen)
	ay_error(AY_ERROR, argv[0], "Coarsen operation failed.");
      else
	ay_error(AY_ERROR, argv[0], "Refine operation failed.");
    }

  if(notify_parent)
    {
      (void)ay_notify_parent();
    }

 return TCL_OK;
} /* ay_tcmd_refinecoarsentcmd */


/** ay_tcmd_crtextrnc:
 * Helper for \a ay_tcmd_crttoolobj() below, create an ExtrNC object.
 *
 * \param[in] o originating object
 * \param[in] p provided object, may be NULL
 * \param[in] oid object id (select patch from multiple provided)
 * \param[in] bid boundary id
 * \param[in,out] e where to store the new object
 *
 * \returns AY_OK on success, error code otherwise
 */
int
ay_tcmd_crtextrnc(ay_object *o, ay_object *p, int oid, int bid, ay_object **e)
{
 int ay_status = AY_OK;
 ay_tag *ptag = NULL, *rptag = NULL;
 ay_object *instobj = NULL, *extobj = NULL;
 ay_extrnc_object *extrnc = NULL;

  if(p)
    {
      ptag = p->tags;
      while(ptag)
	{
	  if(ptag->type == ay_peek_tagtype)
	    break;
	  ptag = ptag->next;
	}
    }

  /* create an instance of the originating object
     of the peeked/provided object */
  if(!(instobj = calloc(1, sizeof(ay_object))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  ay_object_defaults(instobj);
  if(ptag)
    {
      if(!(rptag = calloc(1, sizeof(ay_tag))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}
      if(!(rptag->name = malloc(3*sizeof(char))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}
      memcpy(rptag->name, "RP", 3*sizeof(char));
      if(!(rptag->val = malloc(16*sizeof(char))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}
      memcpy(rptag->val, "Transformations", 16*sizeof(char));
      rptag->type = ay_rp_tagtype;
      instobj->tags = rptag;
    }
  else
    {
      if(o->type != AY_IDNPATCH)
	ay_trafo_copy(o, instobj);
    }
  instobj->type = AY_IDINSTANCE;
  instobj->next = ay_endlevel;
  if(ptag)
    {
      instobj->refine = ((ay_btval*)ptag->val)->payload;
    }
  else
    {
      if(o->type == AY_IDINSTANCE)
	{
	  instobj->refine = o->refine;
	  ((ay_object*)o->refine)->refcount++;
	}
      else
	{
	  instobj->refine = o;
	  o->refcount++;
	}
    }

  /* create a ExtrNC object */
  if(!(extobj = calloc(1, sizeof(ay_object))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  ay_object_defaults(extobj);
  extobj->parent = AY_TRUE;
  extobj->hide_children = AY_TRUE;
  extobj->type = AY_IDEXTRNC;
  extobj->down = instobj;
  extobj->next = ay_endlevel;

  if(!(extrnc = calloc(1, sizeof(ay_extrnc_object))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  if(bid < 4)
    {
      extrnc->relative = AY_TRUE;
    }
  switch(bid)
    {
    case 0:
      extrnc->side = 4;
      extrnc->parameter = 0.0;
      break;
    case 1:
      extrnc->side = 4;
      extrnc->parameter = 1.0;
      break;
    case 2:
      extrnc->side = 5;
      extrnc->parameter = 0.0;
      break;
    case 3:
      extrnc->side = 5;
      extrnc->parameter = 1.0;
      break;
    case 4:
      extrnc->side = 6;
      break;
    default:
      extrnc->side = bid+4;
      break;
    }

  extrnc->pnum = oid;

  extobj->refine = extrnc;

  /* return result */
  *e = extobj;

  rptag = NULL;

cleanup:

  if(ay_status)
    {
      if(rptag)
	{
	  if(rptag->name)
	    free(rptag->name);
	  if(rptag->val)
	    free(rptag->val);
	  free(rptag);
	}
      if(instobj)
	{
	  ((ay_object*)instobj->refine)->refcount--;
	  free(instobj);
	}
      if(extobj)
	free(extobj);
      if(extrnc)
	free(extrnc);
    }

 return ay_status;
} /* ay_tcmd_crtextrnc */


/** ay_tcmd_crttoolobjtcmd:
 *  Create a tool object from selected boundary curves.
 *  Implements the \a crtToolOb scripting interface command.
 *  See also the corresponding section in the \ayd{sccrttoolob}.
 *
 * \returns TCL_OK in any case.
 */
int
ay_tcmd_crttoolobjtcmd(ClientData clientData, Tcl_Interp *interp,
		       int argc, char *argv[])
{
 int ay_status = AY_OK;
 int notify_parent = AY_FALSE;
 unsigned int i;
 ay_list_object *sel = ay_selection;
 ay_tag *tag = NULL;
 ay_object *o, *p, **pobjects = NULL;
 ay_object *exts = NULL, **next;
 double *trafos = NULL;
 unsigned int oid, tagbid, tagoid;
 ay_object *new;
 Tcl_HashEntry *entry = NULL;

  if(argc < 2)
    {
      ay_error(AY_EARGS, argv[0], "type");
      return TCL_OK;
    }

  if(!(entry = Tcl_FindHashEntry(&ay_otypesht, argv[1])))
    {
      ay_error(AY_ENTYPE, argv[0], NULL);
      return TCL_OK;
    }

  oid = *(unsigned int*)Tcl_GetHashValue(entry);

  next = &exts;

  while(sel)
    {
      if(sel->object)
	{
	  o = sel->object;
	  if((o->type == AY_IDNCURVE) ||
	     (ay_provide_object(o, AY_IDNCURVE, NULL) == AY_OK))
	    {
	      ay_status = ay_object_copy(o, next);

	      if(ay_status)
		goto cleanup;

	      next = &((*next)->next);
	    }
	  else
	    {
	      if((o->type == AY_IDNPATCH) ||
		 (ay_provide_object(o, AY_IDNPATCH, NULL) == AY_OK))
		{
		  tag = o->tags;

		  while(tag)
		    {
		      if(tag->type == ay_sb_tagtype)
			{
			  if(tag->val && (((char*)tag->val)[0] != '\0'))
			    {
			      if(((char*)tag->val)[0] != 'o')
				{
				  if(sscanf(tag->val, "%u", &tagbid) != 1)
				    continue;
				  tagoid = 0;
				}
			      else
				{
				  if(sscanf(tag->val, "o:%ub:%u",
					    &tagoid, &tagbid) != 2)
				    continue;
				}

			      /**/
			      if(o->type == AY_IDNPATCH)
				{
				  ay_status = ay_tcmd_crtextrnc(o, NULL, 0,
								tagbid, next);

				  if(ay_status)
				    goto cleanup;

				  next = &((*next)->next);
				}
			      else
				{
				  if(!pobjects)
				    {
				      (void)ay_peek_object(o, AY_IDNPATCH,
							   &pobjects, &trafos);
				    }

				  if(pobjects)
				    {
				      i = 0;
				      while((i != tagoid) &&
					    (pobjects[i] != ay_endlevel))
					i++;

				      if(pobjects[i] &&
					 (pobjects[i] != ay_endlevel))
					{
					  p = pobjects[i];
					  ay_status = ay_tcmd_crtextrnc(o, p,
							tagoid+1, tagbid, next);

					  if(ay_status)
					    goto cleanup;

					  next = &((*next)->next);
					}
				    } /* if have pobjects */
				} /* if is NPatch */
			    } /* if have tag val */
			} /* have sb tag */

		      tag = tag->next;
		    } /* while */
		  if(pobjects)
		    {
		      free(pobjects);
		      pobjects = NULL;
		    }
		  if(trafos)
		    {
		      free(trafos);
		      trafos = NULL;
		    }
		} /* if is/provides NPatch */
	    } /* if curve or surface */
	} /* if */

      sel = sel->next;
    } /* while */

  if(oid == AY_IDEXTRNC)
    {
      if(exts)
	{
	  ay_object_link(exts);
	}
      /* prevent cleanup code from doing something harmful */
      exts = NULL;
    }

  /* XXXX todo sort */


  switch(oid)
    {
    case AY_IDBIRAIL1:
    case AY_IDBIRAIL2:
    case AY_IDCAP:
    case AY_IDBEVEL:
    case AY_IDEXTRUDE:
    case AY_IDGORDON:
    case AY_IDREVOLVE:
    case AY_IDSKIN:
    case AY_IDSWEEP:
    case AY_IDSWING:
    case AY_IDDSKIN:
    case AY_IDCONCATNC:
    case AY_IDSCRIPT:
      if(exts)
	{
	  if(argc > 3)
	    ay_status = ay_object_createargs(oid, argc, argv, &new);
	  else
	    ay_status = ay_object_create(oid, &new);

	  if(new)
	    {
	      new->down = exts;

	      ay_object_link(new);

	      /* prevent cleanup code from doing something harmful */
	      exts = NULL;
	    }
	}
      break;
    default:
      break;
    }

cleanup:

  while(exts)
    {
      o = exts->next;
      ay_object_delete(exts);
      exts = o;
    }

  if(pobjects)
    {
      free(pobjects);
      pobjects = NULL;
    }
  if(trafos)
    {
      free(trafos);
      trafos = NULL;
    }

  if(ay_status)
    {
      ay_error(AY_ERROR, argv[0], "Create operation failed.");
    }

  if(notify_parent)
    {
      (void)ay_notify_parent();
    }

 return TCL_OK;
} /* ay_tcmd_crttoolobjtcmd */


/** ay_tcmd_registergeneric:
 *  register a generic operation callback
 *
 * \param[in] op operation designation (AY_OP*)
 * \param[in] cb generic operation callback
 * \param[in] type_id object type for which to register the callback (AY_ID...)
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_tcmd_registergeneric(int op, ay_genericcb *cb, unsigned int type_id)
{
 int ay_status = AY_OK;

  switch(op)
    {
    case AY_OPREVERT:
      ay_status = ay_table_addcallback(&ay_tcmd_revertcbt, (ay_voidfp)cb,
				       type_id);
      break;
    case AY_OPOPEN:
      ay_status = ay_table_addcallback(&ay_tcmd_opencbt, (ay_voidfp)cb,
				       type_id);
      break;
    case AY_OPCLOSE:
      ay_status = ay_table_addcallback(&ay_tcmd_closecbt, (ay_voidfp)cb,
				       type_id);
      break;
    case AY_OPREFINE:
      ay_status = ay_table_addcallback(&ay_tcmd_refinecbt, (ay_voidfp)cb,
				       type_id);
      break;
    case AY_OPCOARSEN:
      ay_status = ay_table_addcallback(&ay_tcmd_coarsencbt, (ay_voidfp)cb,
				       type_id);
      break;
    default:
      break;
    }

 return ay_status;
} /* ay_tcmd_registergeneric */


/** ay_tcmd_init:
 * Initialize generic operation function tables, so that they
 * may be filled using \a ay_tcmd_registergeneric() above.
 *
 * \param[in] interp Tcl interpreter, currently unused
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_tcmd_init(Tcl_Interp *interp)
{
 int ay_status = AY_OK;
 char fname[] = "tcmd_init";

  if((ay_status = ay_table_initftable(&ay_tcmd_revertcbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_tcmd_opencbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_tcmd_closecbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_tcmd_refinecbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_tcmd_coarsencbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

 return ay_status;
} /* ay_tcmd_init */

