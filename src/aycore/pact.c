/*
 * Ayam, a free 3D modeler for the RenderMan interface.
 *
 * Ayam is copyrighted 1998-2005 by Randolf Schultz
 * (randolf.schultz@gmail.com) and others.
 *
 * All rights reserved.
 *
 * See the file License for details.
 *
 */

#include "ayam.h"

/* pact.c - single point related interactive actions */


/* global variables for this module: */

/* selected points and indices */
static ay_pointedit pact_pe = {0};

/* number of selected points per object */
static int *pact_numpo;

/* type of selected points per object */
static int *pact_typepo;

/* objects that have selected points */
static ay_object **pact_objects;

/* number of objects that have selected points */
static int pact_objectslen;

/** all registered insert callbacks */
static ay_ftable ay_pact_insertcbt;

/** all registered delete callbacks */
static ay_ftable ay_pact_deletecbt;


/* prototypes of functions local to this module: */

void ay_pact_findpoint(int len, int stride, double *cv,
		       double *objXYZ, int *index);

int ay_pact_insertic(ay_icurve_object *curve, int *index,
		     double *objXYZ, int edit);

int ay_pact_deleteic(ay_icurve_object *icurve, int *index,
		     double *objXYZ);

int ay_pact_notify(ay_object *o, int j, int k);

int ay_pact_multincnc(ay_object *o);

int ay_pact_multdecnc(ay_object *o);


/* functions: */

/* ay_pact_clearpointedit:
 *   clear/reset a ay_pointedit structure
 *   (does not attempt to free <pe> itself!)
 */
void
ay_pact_clearpointedit(ay_pointedit *pe)
{
  if(!pe)
    return;

  if(pe->coords)
    free(pe->coords);
  pe->coords = NULL;

  if(pe->indices)
    free(pe->indices);
  pe->indices = NULL;

  pe->num = 0;
  pe->type = AY_PT3D;
  pe->readonly = AY_FALSE;
  pe->multiple = AY_FALSE;

 return;
} /* ay_pact_clearpointedit */


/* ay_pact_getpoint:
 *   calls object type specific getpoint
 *   functions to let the object decide
 *   which point should be edited in single
 *   point editing mode or for which point
 *   the weight should be changed
*/
int
ay_pact_getpoint(int mode, ay_object *o, double *obj, ay_pointedit *pe)
{
 ay_voidfp *arr = NULL;
 ay_getpntcb *cb = NULL;

  if(!o || ((mode != 3) && (!obj || !pe)))
    return AY_ENULL;

  arr = ay_getpntcbt.arr;
  cb = (ay_getpntcb *)(arr[o->type]);
  if(!cb)
    {
      return AY_OK;
    }

 return cb(mode, o, obj, pe);
} /* ay_pact_getpoint */


/* ay_pact_pickpoint:
 *  like getpoint() above, but additionally modifies ay_pickepsilon
 *  to improve picking accuracy for large/small objects
*/
int
ay_pact_pickpoint(ay_object *o, ay_view_object *view,
		  double minlevelscale, double *obj, ay_pointedit *pe)
{
 int ay_status = AY_OK;
 ay_voidfp *arr = NULL;
 ay_getpntcb *cb = NULL;
 double oldpickepsilon, minobjscale;

  if(!o || !obj || !pe || !view)
    return AY_ENULL;

  oldpickepsilon = ay_prefs.pick_epsilon;

  /* adapt pick epsilon to view zoom, level and object scale */
  minobjscale = fabs(o->scalx);

  if(fabs(o->scaly) < minobjscale)
    minobjscale = fabs(o->scaly);

  if(fabs(o->scalz) < minobjscale)
    minobjscale = fabs(o->scalz);

  if(minlevelscale < 0)
    minlevelscale = 1.0;

  ay_prefs.pick_epsilon = oldpickepsilon / minlevelscale / minobjscale *
    250 * view->conv_x;

  if(view->type == AY_VTPERSP)
    {
      ay_prefs.pick_epsilon *= 4;
    }

  /* now try to pick points on object o */
  arr = ay_getpntcbt.arr;
  cb = (ay_getpntcb *)(arr[o->type]);
  if(cb)
    {
      ay_status = cb(1, o, obj, pe);
    }

  ay_prefs.pick_epsilon = oldpickepsilon;

 return ay_status;
} /* ay_pact_pickpoint */


/* ay_pact_getminlevelscale:
 *  helper to calculate the minimum scale of the current level
 *  may be expensive for deeply nested levels, so only call this
 *  once for a level
 *  Also due to its potential cost, this is not integrated in pickpoint()
 *  above.
 */
double
ay_pact_getminlevelscale()
{
 GLdouble m[16];
 double minlevelscale = 1.0;

  ay_trafo_identitymatrix(m);
  if(ay_currentlevel->object != ay_root)
    {
      ay_trafo_getsomeparent(ay_currentlevel->next, AY_SCA, m);
    }

   minlevelscale = fabs(m[0]);
   if(fabs(m[5]) < minlevelscale)
     minlevelscale = fabs(m[5]);
   if(fabs(m[10]) < minlevelscale)
     minlevelscale = fabs(m[10]);

 return minlevelscale;
} /* ay_pact_getminlevelscale */


/* ay_pact_selboundtcb:
 *  select (tag) all points that form a boundary of an object
 */
int
ay_pact_selboundtcb(struct Togl *togl, int argc, char *argv[])
{
 int ay_status = AY_OK;
 char fname[] = "selbac";
 Tcl_Interp *interp = Togl_Interp(togl);
 ay_point *newp = NULL;
 ay_pointedit pe = {0};
 ay_list_object *sel = ay_selection;
 ay_view_object *view = Togl_GetClientData(togl);
 ay_object *o = NULL;
 double winX = 0.0, winY = 0.0;
 double minlevelscale = 1.0, obj[3] = {0};

  if(!ay_selection)
    {
      ay_error(AY_ENOSEL, fname, NULL);
      return TCL_OK;
    }

  Tcl_GetDouble(interp, argv[2], &winX);
  Tcl_GetDouble(interp, argv[3], &winY);

  minlevelscale = ay_pact_getminlevelscale();

  while(sel)
    {
      o = sel->object;

      /* so that we may use continue */
      sel = sel->next;

      if(o->type != AY_IDPOMESH)
	continue;

      ay_viewt_wintoobj(togl, o, winX, winY, &(obj[0]), &(obj[1]), &(obj[2]));

      ay_status = ay_pact_pickpoint(o, view, minlevelscale, obj, &pe);

      if(!ay_status && pe.coords)
	{
	  ay_selp_clear(o);

	  /* create new point object */
	  if(!(newp = malloc(sizeof(ay_point))))
	    {
	      ay_error(AY_EOMEM, fname, NULL);
	      return TCL_OK;
	    }

	  newp->next = o->selp;
	  o->selp = newp;
	  newp->point = pe.coords[0];

	  if(pe.indices)
	    {
	      newp->index = pe.indices[0];
	    }
	  else
	    {
	      newp->index = 0;
	    }

	  newp->type = pe.type;
	  newp->readonly = pe.readonly;

	  ay_status = ay_pomesht_selectbound(o->refine, o->selp);
	  if(ay_status)
	    {
	      ay_error(ay_status, argv[0], "Select boundary failed.");
	    }
	} /* if */

      ay_pact_clearpointedit(&pe);
    } /* while */

 return TCL_OK;
} /* ay_pact_selboundtcb */


/* ay_pact_seltcb:
 *  select (tag) single points of an object
 */
int
ay_pact_seltcb(struct Togl *togl, int argc, char *argv[])
{
 int ay_status = AY_OK;
 char fname[] = "selpac";
 Tcl_Interp *interp = Togl_Interp(togl);
 ay_point *newp = NULL, *point = NULL, *last = NULL;
 ay_pointedit pe = {0};
 ay_list_object *sel = ay_selection;
 ay_view_object *view = Togl_GetClientData(togl);
 ay_object *o = NULL;
 int have_it = AY_FALSE, multiple = AY_FALSE, multipledel = AY_FALSE;
 unsigned int i = 0;
 double winX = 0.0, winY = 0.0, winX2 = 0.0, winY2 = 0.0, dtemp = 0.0;
 double minlevelscale = 1.0, obj[24] = {0}, pl[16] = {0};

  if(!ay_selection)
    {
      ay_error(AY_ENOSEL, fname, NULL);
      return TCL_OK;
    }

  Tcl_GetDouble(interp, argv[2], &winX);
  Tcl_GetDouble(interp, argv[3], &winY);

  if(argc > 4)
    {
      multiple = AY_TRUE;
      Tcl_GetDouble(interp, argv[4], &winX2);
      Tcl_GetDouble(interp, argv[5], &winY2);
      if(argc > 6)
	{
	  multipledel = AY_TRUE;
	}
    }
  else
    {
      winX2 = winX;
      winY2 = winY;
    }

  if(winX2 < winX)
    {
      dtemp = winX2;
      winX2 = winX;
      winX = dtemp;
    }

  if(winY2 < winY)
    {
      dtemp = winY2;
      winY2 = winY;
      winY = dtemp;
    }

  if(!multiple)
    {
      minlevelscale = ay_pact_getminlevelscale();
    }

  while(sel)
    {
      o = sel->object;
      o->modified = 0;

      if(!multiple)
	{
	  ay_viewt_wintoobj(togl, o, winX, winY,
			    &(obj[0]), &(obj[1]), &(obj[2]));

	  ay_status = ay_pact_pickpoint(o, view, minlevelscale, obj, &pe);
	}
      else
	{
	  ay_viewt_winrecttoobj(togl, o, winX, winY, winX2, winY2, obj);

	  ay_viewt_objrecttoplanes(obj, pl);

	  ay_status = ay_pact_getpoint(2, o, pl, &pe);
	} /* if */

      if(!ay_status && pe.coords)
	{
	  for(i = 0; i < pe.num; i++)
	    {
	      last = NULL;
	      have_it = AY_FALSE;
	      point = o->selp;
	      while(point)
		{
		  /* do we have that point selected already? */
		  if(point->point == pe.coords[i])
		    {
		      have_it = AY_TRUE;
		    }

		  if(have_it)
		    {
		      /* we have that point already, so we remove
			 it from the selection if we are not in
			 multiple selection mode; we also remove
			 it if we are in multiple deletion mode */
		      if(!multiple || multipledel)
			{
			  if(last)
			    last->next = point->next;
			  else
			    o->selp = point->next;

			  free(point);
			  o->modified = 2;
			} /* if */
		      break;
		    } /* if */

		  last = point;

		  point = point->next;
		} /* while */

	      /* add point to selection (but not if we are in
		 multiple deletion mode, where we only remove
		 points from the selection) */
	      if(!have_it && !multipledel)
		{
		  /* create new point object */
		  if(!(newp = malloc(sizeof(ay_point))))
		    {
		      ay_error(AY_EOMEM, fname, NULL);
		      return TCL_OK;
		    }

		  newp->next = o->selp;
		  o->selp = newp;
		  newp->point = pe.coords[i];

		  if(pe.indices)
		    {
		      newp->index = pe.indices[i];
		    }
		  else
		    {
		      newp->index = 0;
		    }

		  newp->type = pe.type;
		  newp->readonly = pe.readonly;
		  o->modified = 2;
		} /* if */
	    } /* for */
	} /* if */

      ay_pact_clearpointedit(&pe);

      if(o->modified == 2)
	(void)ay_notify_object(o);

      sel = sel->next;
    } /* while */

 return TCL_OK;
} /* ay_pact_seltcb */


/* ay_pact_flashpoint:
 *  flash single points if the mouse pointer hovers over them
 *  by drawing with XOR directly into the front buffer
 *  Note: This function needs at least OpenGL V1.1 to work.
 */
void
ay_pact_flashpoint(int ignore_old, int multiple, double *pnt, ay_object *o)
{
 int old_is_new = AY_FALSE;
 static double old_pnt[3] = {0};
 static ay_object *old_o = NULL;
 static int old_multiple = AY_FALSE;
 double m[16];
 double point_size = ay_prefs.handle_size;

#ifdef GL_VERSION_1_1

  /* when called to clear old point, but there is no old point => just exit */
  if(!ignore_old && !pnt && !o && !old_o)
    {
      /*printf("there is no old point to clear\n");*/
      return;
    }

  if(old_o && pnt)
    {
      if((fabs(pnt[0] - old_pnt[0]) < AY_EPSILON) &&
	 (fabs(pnt[1] - old_pnt[1]) < AY_EPSILON) &&
	 (fabs(pnt[2] - old_pnt[2]) < AY_EPSILON) &&
	 (old_o == o))
	{
	  old_is_new = AY_TRUE;
	}
    }

  if(!old_is_new || ignore_old)
    {
      glDrawBuffer(GL_FRONT);
      glEnable(GL_COLOR_LOGIC_OP);
      glLogicOp(GL_XOR);
      glColor3f((GLfloat)ay_prefs.sxr, (GLfloat)ay_prefs.sxg,
		(GLfloat)ay_prefs.sxb);
      glDisable(GL_DEPTH_TEST);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
       glLoadIdentity();
       if(ay_currentlevel->object != ay_root)
	 {
	   ay_trafo_concatparent(ay_currentlevel->next);
	 }

       /* clear old point? */
       if(old_o && !ignore_old)
	 {
	   /*
	   printf("clearing %lg %lg %lg\n",old_pnt[0],old_pnt[1],old_pnt[2]);
	   */
	   glPushMatrix();
	    glTranslated(old_o->movx, old_o->movy, old_o->movz);
	    ay_quat_torotmatrix(old_o->quat, m);
	    glMultMatrixd((GLdouble*)m);
	    glScaled(old_o->scalx, old_o->scaly, old_o->scalz);
	    if(old_multiple == 2)
	      {
		ay_draw_rhombus(old_pnt);
	      }
	    else
	      {
		if(old_multiple)
		  glPointSize((GLfloat)(point_size*1.4));
		glBegin(GL_POINTS);
		 glVertex3dv(old_pnt);
		glEnd();
		if(old_multiple)
		  glPointSize((GLfloat)(point_size));
	      }
	   glPopMatrix();
	   old_o = NULL;
	 } /* if */

       /*glFlush();*/

       /* draw new point? */
       if(pnt && o)
	 {
	   /*
	   printf("flashing %lg %lg %lg\n",pnt[0],pnt[1],pnt[2]);
	   */
	   glPushMatrix();
	    glTranslated(o->movx, o->movy, o->movz);
	    ay_quat_torotmatrix(o->quat, m);
	    glMultMatrixd((GLdouble*)m);
	    glScaled(o->scalx, o->scaly, o->scalz);
	    if(multiple == 2)
	      {
		ay_draw_rhombus(pnt);
	      }
	    else
	      {
		if(multiple)
		  glPointSize((GLfloat)(point_size*1.4));
		glBegin(GL_POINTS);
		 glVertex3dv(pnt);
		glEnd();
		if(multiple)
		  glPointSize((GLfloat)(point_size));
	      }
	   glPopMatrix();
	   memcpy(old_pnt, pnt, 3*sizeof(double));
	   old_o = o;
	   old_multiple = multiple;
	 }
       else
	 {
	   old_o = NULL;
	 }
      glPopMatrix();
      glEnable(GL_DEPTH_TEST);
      /* the following line fixes problems with Intel
	 onboard graphics (i915) */
      glLogicOp(GL_COPY);
      glDisable(GL_COLOR_LOGIC_OP);
      glFlush();
      glDrawBuffer(GL_BACK);
    } /* if */

#endif /* GL_VERSION_1_1 */

 return;
} /* ay_pact_flashpoint */


/* ay_pact_startpetcb:
 *  prepares everything for the single point editing modes
 *  (it is bound to moving the mouse pointer, and it is flashing
 *   potentially affected points on the go)
 */
int
ay_pact_startpetcb(struct Togl *togl, int argc, char *argv[])
{
 int ay_status = AY_OK;
 char fname[] = "edit_point";
 Tcl_Interp *interp = Togl_Interp(togl);
 ay_list_object *sel = ay_selection;
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 ay_object **tmpo = NULL, *o = NULL;
 double winX = 0.0, winY = 0.0;
 double minlevelscale, obj[3] = {0}, w;
 double **pecoords = NULL, **tmp = NULL;
 int allowreadonly = AY_FALSE, flash = AY_FALSE, ignoreold = AY_FALSE;
 int i, penumber = 0, *tmpi, multiple = AY_FALSE;
 unsigned int *peindices = NULL, *tmpu;

  Togl_MakeCurrent(togl);

  if(pact_numpo)
    free(pact_numpo);
  pact_numpo = NULL;

  if(pact_typepo)
    free(pact_typepo);
  pact_typepo = NULL;

  if(pact_objects)
    free(pact_objects);
  pact_objects = NULL;

  pact_objectslen = 0;

  ay_pact_clearpointedit(&pact_pe);

  if(view->drawhandles == 2)
    {
      pact_pe.type = AY_PTKNOT;
    }

  Tcl_GetDouble(interp, argv[2], &winX);
  Tcl_GetDouble(interp, argv[3], &winY);
  i = 4;
  while(i < argc)
    {
      if(argv[i][0] == '-')
	{
	  switch(argv[i][1])
	    {
	    case 'r':
	      allowreadonly = AY_TRUE;
	      break;
	    case 'f':
	      flash = AY_TRUE;
	      break;
	    case 'i':
	      ignoreold = AY_TRUE;
	      break;
	    default:
	      break;
	    }
	}
      i++;
    }

  minlevelscale = ay_pact_getminlevelscale();

  while(sel)
    {
      o = sel->object;

      ay_viewt_wintoobj(togl, o, winX, winY,
			&(obj[0]), &(obj[1]), &(obj[2]));

      /* now try to pick points on object o */
      ay_status = ay_pact_pickpoint(o, view, minlevelscale, obj, &pact_pe);

      /* see what have we got */
      if(!ay_status && pact_pe.coords && (!pact_pe.readonly || allowreadonly))
	{
	  /* pick succeeded ...*/
	  /* ...save coords */
	  if(!(tmp = realloc(pecoords,
			     (pact_pe.num + penumber)*sizeof(double*))))
	    {
	      ay_status = AY_EOMEM; goto cleanup;
	    }
	  else
	    {
	      pecoords = tmp;
	      memcpy(&(pecoords[penumber]), pact_pe.coords,
		     pact_pe.num*sizeof(double*));
	    }

	  /* ...save indices */
	  if(!(tmpu = realloc(peindices,
			     (pact_pe.num + penumber)*sizeof(unsigned int))))
	    {
	      ay_status = AY_EOMEM; goto cleanup;
	    }
	  else
	    {
	      peindices = tmpu;
	      if(pact_pe.indices)
		{
		  memcpy(&(peindices[penumber]), pact_pe.indices,
			 pact_pe.num*sizeof(unsigned int));
		}
	    }

	  /* ...up the picked points count */
	  penumber += pact_pe.num;

	  /* ...remember number of picked points of current object */
	  if(!(tmpi = realloc(pact_numpo, (pact_objectslen+1)*
			      sizeof(int))))
	    {
	      ay_status = AY_EOMEM; goto cleanup;
	    }
	  else
	    {
	      pact_numpo = tmpi;
	      pact_numpo[pact_objectslen] = pact_pe.num;
	    }

	  /* ...remember rational state of current object */
	  if(!(tmpi = realloc(pact_typepo, (pact_objectslen+1)*
			      sizeof(int))))
	    {
	      ay_status = AY_EOMEM; goto cleanup;
	    }
	  else
	    {
	      pact_typepo = tmpi;
	      pact_typepo[pact_objectslen] = pact_pe.type;
	    }

	  /* ...remember pointer to current object */
	  if(!(tmpo = realloc(pact_objects, (pact_objectslen+1)*
			      sizeof(ay_object*))))
	    {
	      ay_status = AY_EOMEM; goto cleanup;
	    }
	  else
	    {
	      pact_objects = tmpo;
	      pact_objects[pact_objectslen] = sel->object;
	    }

	  if(pact_pe.multiple)
	    multiple = AY_TRUE;

	  pact_objectslen++;
	} /* if pick ok */
      else
	{
	  /* failed picks are of no concern, better avoid clogging
	     the error log */
	  ay_status = AY_OK;
	}

      ay_pact_clearpointedit(&pact_pe);

      if(view->drawhandles == 2)
	{
	  pact_pe.type = AY_PTKNOT;
	}

      sel = sel->next;
    } /* while */

  /* fill pact_pe with data from all selected points
     from all selected objects */
  pact_pe.num = penumber;
  pact_pe.coords = pecoords;
  pact_pe.indices = peindices;

  /* flash option given? */
  if(ay_selection && flash)
    {
      if(pact_objectslen > 0)
	o = pact_objects[0];
      else
	o = NULL;

      if(pecoords && *pact_typepo == AY_PTKNOT)
	{
	  /* flash knot */
	  ay_pact_flashpoint(ignoreold, 2, pecoords?*pecoords:NULL, o);
	}
      else
	{
	  /* flash ordinary point */
	  if(pecoords && (*pact_typepo == AY_PTRAT) && ay_prefs.rationalpoints)
	    {
	      w = pecoords[0][3];
	      obj[0] = pecoords[0][0]*w;
	      obj[1] = pecoords[0][1]*w;
	      obj[2] = pecoords[0][2]*w;
	      ay_pact_flashpoint(ignoreold, multiple, obj, o);
	    }
	  else
	    {
	      ay_pact_flashpoint(ignoreold, multiple,
				 pecoords?*pecoords:NULL, o);
	    } /* if */
	} /* if */
    } /* if */

  /* prevent cleanup code from doing something harmful */
  pecoords = NULL;
  peindices = NULL;

cleanup:

  if(ay_status)
    ay_error(ay_status, fname, NULL);

  if(pecoords)
    free(pecoords);
  if(peindices)
    free(peindices);

 return TCL_OK;
} /* ay_pact_startpetcb */


/* ay_pact_pentcb:
 *  single point numeric edit callback
 *
 */
int
ay_pact_pentcb(struct Togl *togl, int argc, char *argv[])
{
 int ay_status = AY_OK, tcl_status = TCL_OK;
 char fname[] = "edit_point_num";
 Tcl_Interp *interp = NULL;
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 double winX = 0.0, winY = 0.0;
 double obj[3] = {0};
 int i;
 char arr[] = "editPntArr";
 char *ln[4] = {"lx","ly","lz","lw"}, *wn[4] = {"wx","wy","wz","ww"};
 char *xyzw[4] = {"x","y","z","w"};
 char *str = NULL;
 Tcl_Obj *to = NULL;
 int local = 0/*, need_parentnotify = AY_TRUE*/;
 char set[4] = {AY_FALSE};
 double minlevelscale = 1.0, *coords, wcoords[4], tcoords[4];
 ay_list_object *sel = NULL;
 ay_object *o = NULL;
 ay_pointedit pe = {0};
 ay_point *selp = NULL;

  if(argc < 1)
    {
      ay_error(AY_EARGS, fname, NULL);
      return TCL_OK;
    }

  interp = Togl_Interp(togl);

  if(!strcmp(argv[2], "-start"))
    {
      sel = ay_selection;
      if(!sel)
	{
	  ay_error(AY_ENOSEL, fname, NULL);
	  return TCL_OK;
	}

      Tcl_GetDouble(interp, argv[3], &winX);
      Tcl_GetDouble(interp, argv[4], &winY);

      minlevelscale = ay_pact_getminlevelscale();

      while(sel)
	{
	  o = sel->object;

	  /* pick new point */
	  ay_viewt_wintoobj(togl, o, winX, winY,
			    &(obj[0]), &(obj[1]), &(obj[2]));

	  ay_status = ay_pact_pickpoint(o, view, minlevelscale, obj, &pe);

	  /* update GUI and point selection (only if the pick succeeded) */
	  if(!ay_status && pe.coords)
	    {
	      /* remove old point selection */
	      ay_selp_clear(o);

	      /*
		the new point selection is established in a second
		binding of <B1-Release> to ay_pact_seltcb() above
		which saves us some work here...
	      */

	      /*
		now, transport the coordinate values of the picked point
		to the Tcl context for display and editing in the direct
		point edit dialog
	      */
	      coords = pe.coords[0];

	      if(pe.type == AY_PTRAT)
		{
		  memcpy(wcoords, coords, 4*sizeof(double));
		  if(ay_prefs.rationalpoints)
		    {
		      wcoords[0] *= wcoords[3];
		      wcoords[1] *= wcoords[3];
		      wcoords[2] *= wcoords[3];
		    }
		}
	      else
		{
		  memcpy(wcoords, coords, 3*sizeof(double));
		  wcoords[3] = 1.0;
		}

	      for(i = 0; i < 3; i++)
		{
		  Tcl_SetVar2Ex(interp, arr, ln[i],
				Tcl_NewDoubleObj(wcoords[i]),
				TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
		}

	      if(pe.type == AY_PTRAT)
		{
		  Tcl_SetVar2Ex(interp, arr, ln[3],
				Tcl_NewDoubleObj(coords[3]),
				TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
		}
	      else
		{
		  Tcl_SetVar2Ex(interp, arr, ln[3],
				Tcl_NewDoubleObj(1.0),
				TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
		}

	      /* convert to world coordinates */
	      ay_trafo_applyall(ay_currentlevel->next, o, wcoords);

	      for(i = 0; i < 3; i++)
		{
		  Tcl_SetVar2Ex(interp, arr, wn[i],
				Tcl_NewDoubleObj(wcoords[i]),
				TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
		}

	      if(pe.type == AY_PTRAT)
		{
		  Tcl_SetVar2Ex(interp, arr, wn[3],
				Tcl_NewDoubleObj(coords[3]),
				TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
		}
	      else
		{
		  Tcl_SetVar2Ex(interp, arr, wn[3],
				Tcl_NewDoubleObj(1.0),
				TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
		}

	      Tcl_SetVar2Ex(interp, arr, "valid",
			    Tcl_NewIntObj(1),
			    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

	      ay_pact_clearpointedit(&pe);
	    } /* if have picked point */
	  sel = sel->next;
	} /* while */
      return TCL_OK;
    } /* if start */

  if(!strcmp(argv[2], "-apply"))
    {
      to = Tcl_GetVar2Ex(interp, arr, "local",
		     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
      Tcl_GetIntFromObj(interp, to, &local);

      /* get new coordinates from Tcl GUI */
      for(i = 0; i < 4; i++)
	{
	  to = Tcl_GetVar2Ex(interp, arr, xyzw[i],
			     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	  str = Tcl_GetStringFromObj(to, NULL);
	  /* empty entry field/string => no change for this dimension */
	  if(str[0] != '\0')
	    {
	      tcl_status = Tcl_GetDoubleFromObj(interp, to, &tcoords[i]);
	      AY_CHTCLERRGOT(tcl_status, fname, interp);
	      set[i] = AY_TRUE;
	    }
	}

      /* apply coordinates to all selected objects selected points */
      sel = ay_selection;
      while(sel)
	{
	  o = sel->object;
	  if(o->selp)
	    {
	      if(!local)
		{
		  /* convert world coordinates to object space */
		  ay_trafo_applyalli(ay_currentlevel->next, o, tcoords);
		}

	      selp = o->selp;

	      if(set[0] && set[1] && set[2] && set[3])
		{
		  while(selp)
		    {
		      if(!selp->readonly)
			{
			  if(selp->type == AY_PTRAT)
			    {
			      memcpy(selp->point, tcoords, 4*sizeof(double));
			      if(ay_prefs.rationalpoints)
				{
				  selp->point[0] /= selp->point[3];
				  selp->point[1] /= selp->point[3];
				  selp->point[2] /= selp->point[3];
				}
			    }
			  else
			    {
			      memcpy(selp->point, tcoords, 3*sizeof(double));
			    }
			  o->modified = AY_TRUE;
			}
		      selp = selp->next;
		    } /* while */
		}
	      else
		{
		  while(selp)
		    {
		      if(!selp->readonly)
			{
			  if(set[0])
			    selp->point[0] = tcoords[0];
			  if(set[1])
			    selp->point[1] = tcoords[1];
			  if(set[2])
			    selp->point[2] = tcoords[2];
			  if(selp->type == AY_PTRAT)
			    {
			      if(set[3])
				selp->point[3] = tcoords[3];
			    }
			  o->modified = AY_TRUE;
			}
		      selp = selp->next;
		    } /* while */
		} /* if */

	      if(o->modified)
		{
		  ay_notify_object(o);
		}
	    } /* if have selp */
	  sel = sel->next;
	} /* while */

      (void)ay_notify_parentof(o, AY_FALSE);

      return TCL_OK;
    } /* if apply */

cleanup:

 return TCL_OK;
} /* ay_pact_pentcb */


/* ay_pact_findpoint:
 *  find point in cv array
 */
void
ay_pact_findpoint(int len, int stride, double *cv, double *objXYZ, int *index)
{
 int i, a;
 double min_distance = ay_prefs.pick_epsilon, distance = 0.0;

  if(!cv || !objXYZ || !index)
    return;

  if(min_distance == 0.0)
    min_distance = DBL_MAX;

  a = 0;
  for(i = 0; i < len; i++)
    {
      if((fabs(objXYZ[0] - cv[a])   < AY_EPSILON) &&
	 (fabs(objXYZ[1] - cv[a+1]) < AY_EPSILON) &&
	 (fabs(objXYZ[2] - cv[a+2]) < AY_EPSILON))
	{
	  *index = i;
	  break;
	}
      else
	{
	  distance = AY_VLEN((objXYZ[0] - cv[a]),
			     (objXYZ[1] - cv[a+1]),
			     (objXYZ[2] - cv[a+2]));

	  if(distance < min_distance)
	    {
	      *index = i;
	      min_distance = distance;
	    }
	} /* if */

      a += stride;
    } /* for */

 return;
} /* ay_pact_findpoint */


/** ay_pact_insertnc:
 * Insert a new control point into a NURBS curve (NCurve).
 *
 * \param[in,out] curve  curve object to process
 * \param[in,out] index  where to store the index of the inserted point
 * \param[in] objXYZ  coordinates of point after which to insert
 * \param[in] edit  interactive editing support
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_pact_insertnc(ay_nurbcurve_object *curve, int *index, double *objXYZ,
		 int edit)
{
 int ay_status = AY_OK;
 char fname[] = "insert_pointnc";
 int i = 0, j = 0, k = 0;
 double *newcontrolv = NULL, *oldcontrolv = NULL, *newknotv = NULL;
 int inserted, section;

  if(!curve || !index || !objXYZ)
    return AY_ENULL;

  *index = -1;
  ay_pact_findpoint(curve->length, 4, curve->controlv, objXYZ, index);

  /* no point picked? */
  if(*index == -1)
    {
      return AY_OK;
    }

  if(curve->type == AY_CTPERIODIC)
    {
      /*
       * if the curve is periodic we simply insert the new point
       * in the controlvector between the picked point and the
       * next different
       */
      oldcontrolv = curve->controlv;

      if(*index == curve->length-1)
	*index = *index - 1;

      curve->length++;
      if(!(newcontrolv = calloc(curve->length*4, sizeof(double))))
	{
	  curve->length--;
	  return AY_EOMEM;
	}

      j = 0;
      inserted = AY_FALSE;
      for(i = 0; i < (curve->length-1); i++)
	{
	  if(i >= *index && !inserted)
	    {
	      memcpy(&(newcontrolv[j*4]), &(oldcontrolv[i*4]),
		     4*sizeof(double));

	      if((fabs(oldcontrolv[i*4]-oldcontrolv[(i+1)*4]) > AY_EPSILON) ||
	      (fabs(oldcontrolv[i*4+1]-oldcontrolv[(i+1)*4+1]) > AY_EPSILON) ||
	      (fabs(oldcontrolv[i*4+2]-oldcontrolv[(i+1)*4+2]) > AY_EPSILON))
		{
		  newcontrolv[(j+1)*4] = oldcontrolv[i*4] +
		    ((oldcontrolv[(i+1)*4] - oldcontrolv[i*4])/2.0);
		  newcontrolv[(j+1)*4+1] = oldcontrolv[i*4+1] +
		    ((oldcontrolv[(i+1)*4+1] - oldcontrolv[i*4+1])/2.0);
		  newcontrolv[(j+1)*4+2] = oldcontrolv[i*4+2] +
		    ((oldcontrolv[(i+1)*4+2] - oldcontrolv[i*4+2])/2.0);
		  newcontrolv[(j+1)*4+3] = oldcontrolv[i*4+3] +
		    ((oldcontrolv[(i+1)*4+3] - oldcontrolv[i*4+3])/2.0);

		  if(fabs(newcontrolv[(j+1)*4+3]) < AY_EPSILON)
		    newcontrolv[(j+1)*4+3] = 1.0;

		  j++;
		  inserted = AY_TRUE;
		}
	    }
	  else
	    {
	      memcpy(&(newcontrolv[j*4]), &(oldcontrolv[i*4]),
		     4*sizeof(double));
	    }
	  j++;
	} /* for */
    }
  else
    {
      /* curve is not periodic => employ special handling
	 of the last point in the curve (extend the curve, if picked) */
      curve->length++;
      if(!(newcontrolv = calloc(curve->length*4, sizeof(double))))
	{
	  curve->length--;
	  return AY_EOMEM;
	}
      oldcontrolv = curve->controlv;
      j = 0;
      inserted = AY_FALSE;
      for(i = 0; i < (curve->length-1); i++)
	{
	  memcpy(&(newcontrolv[j*4]), &(oldcontrolv[i*4]),
		 4*sizeof(double));

	  if((i == curve->length-2) && !inserted)
	    {
	      /* the new point is the new last point */
	      k = (curve->length-1)*4;
	      j *= 4;
	      newcontrolv[k] = curve->controlv[j] +
		((curve->controlv[j]-curve->controlv[j-4])/2.0);
	      newcontrolv[k+1] = curve->controlv[j+1]+
		((curve->controlv[j+1]-curve->controlv[j+1-4])/2.0);
	      newcontrolv[k+2] = curve->controlv[j+2]+
		((curve->controlv[j+2]-curve->controlv[j+2-4])/2.0);
	      newcontrolv[k+3] = curve->controlv[j+3]+
		((curve->controlv[j+3]-curve->controlv[j+3-4])/2.0);

	      if(fabs(newcontrolv[k+3]) < AY_EPSILON)
		newcontrolv[k+3] = 1.0;

	      inserted = AY_TRUE;
	    }
	  else
	    {
	      if(i >= *index && !inserted)
		{
		  if((fabs(oldcontrolv[i*4]-oldcontrolv[(i+1)*4]) >
		      AY_EPSILON) ||
		     (fabs(oldcontrolv[i*4+1]-oldcontrolv[(i+1)*4+1]) >
		      AY_EPSILON) ||
		     (fabs(oldcontrolv[i*4+2]-oldcontrolv[(i+1)*4+2]) >
		      AY_EPSILON))
		    {
		      newcontrolv[(j+1)*4] = oldcontrolv[i*4] +
			((oldcontrolv[(i+1)*4] - oldcontrolv[i*4])/2.0);
		      newcontrolv[(j+1)*4+1] = oldcontrolv[i*4+1] +
			((oldcontrolv[(i+1)*4+1] - oldcontrolv[i*4+1])/2.0);
		      newcontrolv[(j+1)*4+2] = oldcontrolv[i*4+2] +
			((oldcontrolv[(i+1)*4+2] - oldcontrolv[i*4+2])/2.0);
		      newcontrolv[(j+1)*4+3] = oldcontrolv[i*4+3] +
			((oldcontrolv[(i+1)*4+3] - oldcontrolv[i*4+3])/2.0);

		      if(fabs(newcontrolv[(j+1)*4+3]) < AY_EPSILON)
			newcontrolv[(j+1)*4+3] = 1.0;

		      j++;
		      inserted = AY_TRUE;
		    } /* if */
		} /* if */
	    } /* if */
	  j++;
	} /* for */
    } /* if */

  if(!inserted)
    {
      free(newcontrolv);
      curve->length--;
      ay_error(AY_ERROR, fname, "Cannot insert point here.");
      return AY_ERROR;
    }
  else
    {
      if(edit)
	memcpy(&(newcontrolv[(*index+1)*4]), &(oldcontrolv[*index*4]),
	       4*sizeof(double));
    }

  free(curve->controlv);
  curve->controlv = newcontrolv;

  if(curve->knot_type != AY_KTCUSTOM)
    {
      /* adjust order for Bezier curves */
      if(curve->knot_type == AY_KTBEZIER)
	curve->order++;

      ay_status = ay_knots_createnc(curve);
      if(ay_status)
	{
	  curve->length--;
	  curve->controlv = oldcontrolv;
	  free(newcontrolv);
	  return ay_status;
	}
    }
  else
    {
      /* create new custom knots */
      if(!(newknotv = calloc(curve->length+curve->order, sizeof(double))))
	{
	  curve->length--;
	  curve->controlv = oldcontrolv;
	  free(newcontrolv);
	  return AY_EOMEM;
	}

      if(*index == (curve->length-2))
	section = (*index);
      else
	section = (*index)+1;

      if(curve->order > 2)
	for(j = section; j < section+(curve->order-1); j++)
	  {
	    if(fabs(curve->knotv[j] - curve->knotv[j+1]) < AY_EPSILON)
	      {
		section++;
	      }
	    else
	      break;
	  }

      /* if section is too large, we probably appended
	 a point to the curve or a serious error occured
	 so we rather try to insert a knot into the last
	 section of the original knot vector */
      if(section >= (curve->length+curve->order-2))
	{
	  for(j = 0; j < curve->length+curve->order-2; j++)
	    {
	      if(fabs(curve->knotv[j] - curve->knotv[j+1]) > AY_EPSILON)
		{
		  section = j;
		}
	    }
	}

      k = 0;
      for(j = 0; j < curve->length+curve->order-1; j++)
	{
	  newknotv[k] = curve->knotv[j];
	  k++;
	  if(j == section)
	    {
	      newknotv[k] = curve->knotv[j]+
		((curve->knotv[j+1]-curve->knotv[j])/2.0);
	      k++;
	    }
	} /* for */

      free(curve->knotv);
      curve->knotv = newknotv;
    } /* if */

  (void) ay_nct_close(curve);

  ay_nct_recreatemp(curve);

 return ay_status;
} /* ay_pact_insertnc */


/** ay_pact_insertic:
 * Insert a new control point into an interpolating curve (ICurve).
 *
 * \param[in,out] icurve  curve object to process
 * \param[in,out] index  where to store the index of the inserted point
 * \param[in] objXYZ  coordinates of point after which to insert
 * \param[in] edit  interactive editing support
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_pact_insertic(ay_icurve_object *icurve, int *index, double *objXYZ,
		 int edit)
{
 int ay_status = AY_OK;
 char fname[] = "insert_pointic";
 int i = 0, j = 0;
 double *newcontrolv = NULL, *oldcontrolv = NULL;
 int inserted;

  if(!icurve || !index || !objXYZ)
    return AY_ENULL;

  *index = -1;
  ay_pact_findpoint(icurve->length, 3, icurve->controlv, objXYZ, index);

  /* no point picked? */
  if(*index == -1)
    {
      return AY_OK;
    }

  /*
   * we simply insert the new point in the controlvector
   * between the picked point and the next different
   */
  oldcontrolv = icurve->controlv;

  if(!icurve->type)
    {
      /* open curve */
      if(*index == icurve->length-1)
	{
	  *index = *index - 1;
	}
    }
  else
    {
      /* closed curve */
      if(*index == icurve->length-1)
	{
	  icurve->length++;
	  if(!(newcontrolv = calloc(icurve->length*3, sizeof(double))))
	    {
	      icurve->length--;
	      return AY_EOMEM;
	    }
	  memcpy(newcontrolv, oldcontrolv,
		 (icurve->length-1)*3*sizeof(double));
	  j = (icurve->length-1)*3;
	  if(edit)
	    {
	      memcpy(&(newcontrolv[j]), &(oldcontrolv[j-3]), 3*sizeof(double));
	    }
	  else
	    {
	      i = (icurve->length-2)*3;
	      newcontrolv[j] = oldcontrolv[i] +
		((oldcontrolv[0] - oldcontrolv[i])/2.0);

	      newcontrolv[j+1] = oldcontrolv[i+1] +
		((oldcontrolv[1] - oldcontrolv[i+1])/2.0);

	      newcontrolv[j+2] = oldcontrolv[i+2] +
		((oldcontrolv[2] - oldcontrolv[i+2])/2.0);
	    }

	  free(icurve->controlv);
	  icurve->controlv = newcontrolv;
	  return ay_status; /* XXXX early exit */
	} /* if */
    } /* if */

  icurve->length++;
  if(!(newcontrolv = calloc(icurve->length*3, sizeof(double))))
    {
      icurve->length--;
      return AY_EOMEM;
    }

  j = 0;
  inserted = AY_FALSE;
  for(i = 0; i < (icurve->length-1); i++)
    {
      if(i >= *index && !inserted)
	{
	  memcpy(&(newcontrolv[j*3]), &(oldcontrolv[i*3]),
		 3*sizeof(double));

	  if((fabs(oldcontrolv[i*3]   - oldcontrolv[(i+1)*3])   > AY_EPSILON) ||
	     (fabs(oldcontrolv[i*3+1] - oldcontrolv[(i+1)*3+1]) > AY_EPSILON) ||
	     (fabs(oldcontrolv[i*3+2] - oldcontrolv[(i+1)*3+2]) > AY_EPSILON))
	    {
	      newcontrolv[(j+1)*3] = oldcontrolv[i*3] +
		((oldcontrolv[(i+1)*3] - oldcontrolv[i*3])/2.0);

	      newcontrolv[(j+1)*3+1] = oldcontrolv[i*3+1] +
		((oldcontrolv[(i+1)*3+1] - oldcontrolv[i*3+1])/2.0);

	      newcontrolv[(j+1)*3+2] = oldcontrolv[i*3+2] +
		((oldcontrolv[(i+1)*3+2] - oldcontrolv[i*3+2])/2.0);

	      j++;
	      inserted = AY_TRUE;
	    }
	}
      else
	{
	  memcpy(&(newcontrolv[j*3]), &(oldcontrolv[i*3]),
		 3*sizeof(double));
	} /* if */
      j++;
    } /* for */

  if((icurve->type) && (*index == icurve->length-2))
    {
      inserted = AY_TRUE;
    }

  if(!inserted)
    {
      free(newcontrolv);
      icurve->length--;
      ay_error(AY_ERROR, fname, "Cannot insert point here.");
      return AY_ERROR;
    }
  else
    {
      if(edit)
	memcpy(&(newcontrolv[(*index+1)*3]), &(oldcontrolv[*index*3]),
	       3*sizeof(double));
    }

  free(icurve->controlv);
  icurve->controlv = newcontrolv;

 return ay_status;
} /* ay_pact_insertic */


/** ay_pact_insertac:
 * Insert a new control point into an approximating curve (ACurve).
 *
 * \param[in,out] acurve  curve object to process
 * \param[in,out] index  where to store the index of the inserted point
 * \param[in] objXYZ  coordinates of point after which to insert
 * \param[in] edit  interactive editing support
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_pact_insertac(ay_acurve_object *acurve, int *index, double *objXYZ, int edit)
{
 int ay_status = AY_OK;
 char fname[] = "insert_pointac";
 int i = 0, j = 0;
 double *newcontrolv = NULL, *oldcontrolv = NULL;
 int inserted;

  if(!acurve || !index || !objXYZ)
    return AY_ENULL;

  *index = -1;
  ay_pact_findpoint(acurve->length, 3, acurve->controlv, objXYZ, index);

  /* no point picked? */
  if(*index == -1)
    {
      return AY_OK;
    }

  /*
   * we simply insert the new point in the controlvector
   * between the picked point and the next different
   */
  oldcontrolv = acurve->controlv;

  if(!acurve->closed)
    {
      if(*index == acurve->length-1)
	{
	  *index = *index - 1;
	}
    }
  else
    {
      /* closed curve */
      if(*index == acurve->length-1)
	{
	  acurve->length++;
	  if(!(newcontrolv = calloc(acurve->length*3, sizeof(double))))
	    {
	      acurve->length--;
	      return AY_EOMEM;
	    }
	  memcpy(newcontrolv, oldcontrolv, (acurve->length-1)*3*sizeof(double));
	  j = (acurve->length-1)*3;
	  if(edit)
	    {
	      memcpy(&(newcontrolv[j]), &(newcontrolv[j-3]), 3*sizeof(double));
	    }
	  else
	    {
	      i = (acurve->length-2)*3;
	      newcontrolv[j] = oldcontrolv[i] +
		((oldcontrolv[0] - oldcontrolv[i])/2.0);

	      newcontrolv[j+1] = oldcontrolv[i+1] +
		((oldcontrolv[1] - oldcontrolv[i+1])/2.0);

	      newcontrolv[j+2] = oldcontrolv[i+2] +
		((oldcontrolv[2] - oldcontrolv[i+2])/2.0);
	    }
	  free(acurve->controlv);
	  acurve->controlv = newcontrolv;
	  return ay_status; /* XXXX early exit */
	} /* if */
    } /* if */

  acurve->length++;
  if(!(newcontrolv = calloc(acurve->length*3, sizeof(double))))
    {
      acurve->length--;
      return AY_EOMEM;
    }

  j = 0;
  inserted = AY_FALSE;
  for(i = 0; i < (acurve->length-1); i++)
    {
      if(i >= *index && !inserted)
	{
	  memcpy(&(newcontrolv[j*3]), &(oldcontrolv[i*3]),
		 3*sizeof(double));

	  if((fabs(oldcontrolv[i*3]   - oldcontrolv[(i+1)*3])   > AY_EPSILON) ||
	     (fabs(oldcontrolv[i*3+1] - oldcontrolv[(i+1)*3+1]) > AY_EPSILON) ||
	     (fabs(oldcontrolv[i*3+2] - oldcontrolv[(i+1)*3+2]) > AY_EPSILON))
	    {
	      newcontrolv[(j+1)*3] = oldcontrolv[i*3] +
		((oldcontrolv[(i+1)*3] - oldcontrolv[i*3])/2.0);

	      newcontrolv[(j+1)*3+1] = oldcontrolv[i*3+1] +
		((oldcontrolv[(i+1)*3+1] - oldcontrolv[i*3+1])/2.0);

	      newcontrolv[(j+1)*3+2] = oldcontrolv[i*3+2] +
		((oldcontrolv[(i+1)*3+2] - oldcontrolv[i*3+2])/2.0);

	      j++;
	      inserted = AY_TRUE;
	    }
	}
      else
	{
	  memcpy(&(newcontrolv[j*3]), &(oldcontrolv[i*3]),
		 3*sizeof(double));
	} /* if */
      j++;
    } /* for */

  if((acurve->closed) && (*index == acurve->length-2))
    {
      inserted = AY_TRUE;
    }

  if(!inserted)
    {
      free(newcontrolv);
      acurve->length--;
      ay_error(AY_ERROR, fname, "Cannot insert point here.");
      return AY_ERROR;
    }
  else
    {
      if(edit)
	memcpy(&(newcontrolv[(*index+1)*3]), &(oldcontrolv[*index*3]),
	       3*sizeof(double));
    }

  free(acurve->controlv);
  acurve->controlv = newcontrolv;

 return ay_status;
} /* ay_pact_insertac */


/* ay_pact_insertptcb:
 *  insert point action callback
 */
int
ay_pact_insertptcb(struct Togl *togl, int argc, char *argv[])
{
 int ay_status = AY_OK;
 char fname[] = "insert_point";
 Tcl_Interp *interp = Togl_Interp(togl);
 double winX = 0.0, winY = 0.0, objXYZ[3] = {0};
 int index, notify_parent = AY_FALSE, edit = AY_FALSE;
 unsigned int uindex;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL;
 ay_voidfp *arr = NULL;
 ay_inspntcb *cb = NULL;

  if(!sel)
    {
      ay_error(AY_ENOSEL, fname, NULL);
    }

  Tcl_GetDouble(interp, argv[2], &winX);
  Tcl_GetDouble(interp, argv[3], &winY);

  if(argc > 4)
    {
      edit = AY_TRUE;
    }

  while(sel)
    {
      o = sel->object;

      ay_viewt_wintoobj(togl, o,
			winX, winY,
			&(objXYZ[0]), &(objXYZ[1]), &(objXYZ[2]));

      index = -1;

      switch(o->type)
	{
	case AY_IDNCURVE:
	  ay_status = ay_pact_insertnc((ay_nurbcurve_object *)(o->refine),
				       &index, objXYZ, edit);
	  break;
	case AY_IDICURVE:
	  ay_status = ay_pact_insertic((ay_icurve_object *)(o->refine),
				       &index, objXYZ, edit);
	  break;
	case AY_IDACURVE:
	  ay_status = ay_pact_insertac((ay_acurve_object *)(o->refine),
				       &index, objXYZ, edit);
	  break;
	default:
	  arr = ay_pact_insertcbt.arr;
	  cb = (ay_inspntcb *)(arr[o->type]);
	  if(cb)
	    {
	      ay_status = cb(o, &index, objXYZ, edit);
	    }
	  else
	    {
	      ay_error(AY_EWARN, fname, ay_error_igntype);
	      ay_status = AY_ERROR;
	    }
	  break;
	} /* switch */

      if(index != -1)
	{
	  if(ay_status)
	    {
	      ay_error(ay_status, fname, "Error inserting point.");
	    }
	  else
	    {
	      notify_parent = AY_TRUE;
	      if(edit)
		{
		  (void)ay_selp_clear(o);
		  uindex = index+1;
		  (void)ay_selp_sel(o, 1, &uindex);
		}
	      else
		{
		  (void)ay_selp_ins(o, (unsigned int)index);
		}
	      (void)ay_notify_object(o);
	      o->modified = AY_TRUE;
	    } /* if */
	} /* if */
      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_pact_insertptcb */


/** ay_pact_deletenc:
 * Remove a control point from a NURBS curve (NCurve).
 *
 * \param[in,out] curve  curve object to process
 * \param[in,out] index  where to store the index of the removed point
 * \param[in] objXYZ  coordinates of point to delete
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_pact_deletenc(ay_nurbcurve_object *curve, int *index, double *objXYZ)
{
 int ay_status = AY_OK;
 char fname[] = "delete_pointnc";
 int i = 0, j = 0, k = 0;
 double *newcontrolv = NULL, *newknotv = NULL;

  if(!curve || !index || !objXYZ)
    return AY_ENULL;

  *index = -1;
  ay_pact_findpoint(curve->length, 4, curve->controlv, objXYZ, index);

  if(*index == -1)
    {
      return AY_OK;
    }

  if(curve->length-1 < 2)
    {
      ay_error(AY_ERROR, fname, "Need at least two points.");
      return AY_ERROR;
    }

  if((curve->type == AY_CTPERIODIC) &&
     (*index > (curve->length-curve->order)))
    {
      *index = curve->order-(curve->length-(*index))-1;
    }

  /* create new control points */
  curve->length--;
  if(!(newcontrolv = calloc(curve->length*4, sizeof(double))))
    {
      curve->length++;
      return AY_EOMEM;
    }

  /* copy controlv */
  j = 0;
  k = 0;
  for(i = 0; i <= curve->length; i++)
    {
      if(i != *index)
	{
	  newcontrolv[k] = curve->controlv[j];
	  newcontrolv[k+1] = curve->controlv[j+1];
	  newcontrolv[k+2] = curve->controlv[j+2];
	  newcontrolv[k+3] = curve->controlv[j+3];
	  k += 4;
	}

      j += 4;
    } /* for */

  free(curve->controlv);
  curve->controlv = newcontrolv;

  /* adjust order for Bezier curves */
  if(curve->knot_type == AY_KTBEZIER)
    curve->order--;

  if(curve->length < curve->order)
    curve->order = curve->length;

  /* generate new knots? */
  if(curve->knot_type == AY_KTCUSTOM)
    {
      if(!(newknotv = calloc(curve->order+curve->length,
			     sizeof(double))))
	{
	  ay_error(ay_status, fname, "Error creating new knots.");
	  return AY_EOMEM;
	}

      /* find the knot that answers most for the removed
	 control point *index */
      k = *index+1;
      if(curve->order > 3)
	k += ((curve->order-1)/2);

      /* copy knots, omitting the kth */
      j = 0;
      for(i = 0; i < curve->length+curve->order+1; i++)
	{
	  if(i != k)
	    {
	      newknotv[j] = curve->knotv[i];
	      j++;
	    }
	}

      free(curve->knotv);
      curve->knotv = newknotv;
    }
  else
    {
      ay_status = ay_knots_createnc(curve);
      if(ay_status)
	{
	  ay_error(ay_status, fname, "Error creating new knots.");
	  goto cleanup;
	}
    }

  if(curve->type)
    {
      ay_status = ay_nct_close(curve);
      if(ay_status)
	{
	  ay_error(ay_status, fname, "Can not close this curve.");
	  ay_status = AY_EWARN;
	}
    }

cleanup:
  ay_nct_recreatemp(curve);

 return ay_status;
} /* ay_pact_deletenc */


/** ay_pact_deleteic:
 * Remove a control point from an interpolating curve (ICurve).
 *
 * \param[in,out] icurve  curve object to process
 * \param[in,out] index  where to store the index of the removed point
 * \param[in] objXYZ  coordinates of point to delete
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_pact_deleteic(ay_icurve_object *icurve, int *index, double *objXYZ)
{
 char fname[] = "delete_pointic";
 int i = 0, j = 0, k = 0;
 double *newcontrolv = NULL;

  if(!icurve || !index || !objXYZ)
    return AY_ENULL;

  *index = -1;
  ay_pact_findpoint(icurve->length, 3, icurve->controlv, objXYZ, index);

  if(*index == -1)
    {
      return AY_OK;
    }

  if(icurve->length-1 < 3)
    {
      ay_error(AY_ERROR, fname, "Need at least three points.");
      return AY_ERROR;
    }

  /* create new icurve */
  icurve->length--;
  if(!(newcontrolv = calloc(icurve->length*3, sizeof(double))))
    {
      icurve->length++;
      return AY_EOMEM;
    }

  /* copy controlv */
  j = 0;
  k = 0;
  for(i = 0; i <= icurve->length; i++)
    {
      if(i != *index)
	{
	  memcpy(&(newcontrolv[k]), &(icurve->controlv[j]), 3*sizeof(double));
	  k += 3;
	}

      j += 3;
    } /* for */

  free(icurve->controlv);
  icurve->controlv = newcontrolv;

 return AY_OK;
} /* ay_pact_deleteic */


/** ay_pact_deleteac:
 * Remove a control point from an approximating curve (ACurve)
 *
 * \param[in,out] acurve  curve object to process
 * \param[in,out] index  where to store the index of the removed point
 * \param[in] objXYZ  coordinates of point to delete
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_pact_deleteac(ay_acurve_object *acurve, int *index, double *objXYZ)
{
 char fname[] = "delete_pointac";
 int i = 0, j = 0, k = 0;
 double *newcontrolv = NULL;

  if(!acurve || !index || !objXYZ)
    return AY_ENULL;

  *index = -1;
  ay_pact_findpoint(acurve->length, 3, acurve->controlv, objXYZ, index);

  if(*index == -1)
    {
      return AY_OK;
    }

  if(acurve->length-1 < 3)
    {
      ay_error(AY_ERROR, fname, "Need at least three points.");
      return AY_ERROR;
    }

  /* create new acurve */
  acurve->length--;
  if(!(newcontrolv = calloc(acurve->length*3, sizeof(double))))
    {
      acurve->length++;
      return AY_EOMEM;
    }

  /* copy controlv */
  j = 0;
  k = 0;
  for(i = 0; i <= acurve->length; i++)
    {
      if(i != *index)
	{
	  memcpy(&(newcontrolv[k]), &(acurve->controlv[j]), 3*sizeof(double));
	  k += 3;
	}

      j += 3;
    } /* for */

  free(acurve->controlv);
  acurve->controlv = newcontrolv;

 return AY_OK;
} /* ay_pact_deleteac */


/* ay_pact_deleteptcb:
 *  delete point action callback
 *
 */
int
ay_pact_deleteptcb(struct Togl *togl, int argc, char *argv[])
{
 int ay_status = AY_OK;
 char fname[] = "delete_point";
 Tcl_Interp *interp = Togl_Interp(togl);
 double winX = 0.0, winY = 0.0, objXYZ[3] = {0};
 int index, notify_parent = AY_FALSE;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL;
 ay_voidfp *arr = NULL;
 ay_delpntcb *cb = NULL;

  if(!sel)
    {
      ay_error(AY_ENOSEL, fname, NULL);
    }

  Tcl_GetDouble(interp, argv[2], &winX);
  Tcl_GetDouble(interp, argv[3], &winY);

  while(sel)
    {
      o = sel->object;

      ay_viewt_wintoobj(togl, o,
			winX, winY,
			&(objXYZ[0]), &(objXYZ[1]), &(objXYZ[2]));

      switch(o->type)
	{
	case AY_IDNCURVE:
	  ay_status = ay_pact_deletenc((ay_nurbcurve_object *)(o->refine),
				       &index, objXYZ);
	  break;
	case AY_IDICURVE:
	  ay_status = ay_pact_deleteic((ay_icurve_object *)(o->refine),
				       &index, objXYZ);
	  break;
	case AY_IDACURVE:
	  ay_status = ay_pact_deleteac((ay_acurve_object *)(o->refine),
				       &index, objXYZ);
	  break;
	default:
	  arr = ay_pact_deletecbt.arr;
	  cb = (ay_delpntcb *)(arr[o->type]);
	  if(cb)
	    {
	      ay_status = cb(o, &index, objXYZ);
	    }
	  else
	    {
	      ay_error(AY_EWARN, fname, ay_error_igntype);
	      ay_status = AY_ERROR;
	    }
	  break;
	} /* switch */

      if(ay_status > AY_EWARN)
	{
	  ay_error(ay_status, fname, "Error deleting point.");
	}
      else
	{
	  notify_parent = AY_TRUE;
	  if(ay_status == AY_EWARN)
	    {
	      ay_selp_clear(o);
	    }
	  else
	    {
	      ay_selp_rem(o, (unsigned int)index);
	    }
	  (void)ay_notify_object(o);
	  o->modified = AY_TRUE;
	} /* if */

      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_pact_deleteptcb */


/* ay_pact_griddify:
 *  helper for ay_pact_petcb()
 *  snap value n to a grid sized by grid
 */
void
ay_pact_griddify(double *n, double grid)
{
 double m;

  m = fmod(*n, grid);

  if(fabs(m) > 1.0e-05)
    {
      if(*n > 0.0)
	{
	  if(m < (grid/2.0))
	    {
	      *n -= m;
	    }
	  else
	    {
	      *n += grid-m;
	    }
	}
      else
	{
	  if(m < -(grid/2.0))
	    {
	      *n -= grid+m;
	    }
	  else
	    {
	      *n -= m;
	    } /* if */
	} /* if */
    } /* if */

 return;
} /* ay_pact_griddify */


/* ay_pact_petcb:
 *  action callback for single point editing
 */
int
ay_pact_petcb(struct Togl *togl, int argc, char *argv[])
{
 /*int ay_status = AY_OK;*/
 char fname[] = "edit_points";
 Tcl_Interp *interp = Togl_Interp(togl);
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 /*ay_list_object *sel = ay_selection;*/
 static double oldwinx = 0.0, oldwiny = 0.0;
 static int warp = AY_FALSE;
 double winx = 0.0, winy = 0.0;
 double movX, movY, movZ, dx = 0.0, dy = 0.0, dz = 0.0, *coords = NULL;
 double quat[4] = {0}, uccoords[3] = {0}, wcoords[3];
 int i = 0, j, k = 0, start = AY_FALSE, redraw = AY_FALSE;
 double m[16] = {0}, rm[16] = {0};
 /*GLdouble mo[16] = {0};*/
 ay_object *o = NULL;

  if(warp)
    {
      /* while in a (self initiated) warp, we do not process any events */
      return TCL_OK;
    }

  if(pact_objectslen == 0)
    {
      return TCL_OK;
    }

  for(j = 0; j < pact_objectslen; j++)
    {
      o = pact_objects[j];

      ay_trafo_identitymatrix(m);
      ay_trafo_scalematrix(1.0/o->scalx, 1.0/o->scaly, 1.0/o->scalz, m);
      if(!view->aligned)
	{
	  memcpy(quat, o->quat, 4*sizeof(double));
	  ay_quat_inv(quat);
	  ay_quat_torotmatrix(quat, rm);
	  ay_trafo_multmatrix(m, rm);
	}
      ay_trafo_translatematrix(-o->movx, -o->movy, -o->movz, m);

      if(!view->local)
	{
	  if(ay_currentlevel->object != ay_root)
	    {
	      ay_trafo_getparentinv(ay_currentlevel->next, m);
	    }
	}
      else
	{
	  if(ay_currentlevel->object != ay_root)
	    {
	      ay_trafo_getsomeparentinv(ay_currentlevel->next, AY_SCA, m);
	    }
	}

      if(argc >= 4)
	{
	  if(!strcmp(argv[2], "-winxy"))
	    {
	      Tcl_GetDouble(interp, argv[3], &winx);
	      Tcl_GetDouble(interp, argv[4], &winy);

	      if(view->usegrid && (view->grid != 0.0))
		{
		  ay_viewt_griddify(togl, &winx, &winy);
		}
	    }
	  else
	    {
	      if(!strcmp(argv[2], "-start"))
		{
		  start = AY_TRUE;
		  Tcl_GetDouble(interp, argv[3], &winx);
		  Tcl_GetDouble(interp, argv[4], &winy);

		  /* snap selected points to grid coordinates */
		  if(pact_pe.coords)
		    {
		      if(view->usegrid && (view->grid != 0.0) &&
			 ay_prefs.edit_snaps_to_grid)
			{
			  for(i = 0; i < pact_numpo[j]; i++)
			    {
			      coords = pact_pe.coords[k];
			      if(!view->local)
				{
				  ay_trafo_applyall(ay_currentlevel->next, o,
						    coords);
				} /* if */
			      if(i == 0)
				{
				  memcpy(uccoords, coords, 3*sizeof(double));
				} /* if */
			      if(ay_prefs.snap3d)
				{
				  ay_pact_griddify(&(coords[0]), view->grid);
				  ay_pact_griddify(&(coords[1]), view->grid);
				  ay_pact_griddify(&(coords[2]), view->grid);
				}
			      else
				{
				  switch(view->type)
				    {
				    case AY_VTFRONT:
				    case AY_VTTRIM:
				      ay_pact_griddify(&(coords[0]),
						       view->grid);
				      ay_pact_griddify(&(coords[1]),
						       view->grid);
				      break;
				    case AY_VTSIDE:
				      ay_pact_griddify(&(coords[1]),
						       view->grid);
				      ay_pact_griddify(&(coords[2]),
						       view->grid);
				      break;
				    case AY_VTTOP:
				      ay_pact_griddify(&(coords[0]),
						       view->grid);
				      ay_pact_griddify(&(coords[2]),
						       view->grid);
				      break;
				    default:
				      break;
				    } /* switch */
				} /* if */
			      if(i == 0)
				{
				  if(memcmp(uccoords, coords,
					    3*sizeof(double)))
				    {
				      o->modified = AY_TRUE;
				      /* warp mouse to new coords */
				      if(ay_prefs.allow_warp)
					{
					  memcpy(wcoords, coords,
						 3*sizeof(double));
					  if(view->local)
					    {
					      ay_trafo_applyall(
					       ay_currentlevel->next, o,
					       wcoords);
					    }
					  warp = AY_TRUE;
					  ay_viewt_warpmouse(togl, wcoords, o,
							     &winx, &winy);
					  warp = AY_FALSE;
					}
				    }
				} /* if */
			      if(!view->local)
				{
				  ay_trafo_applyalli(ay_currentlevel->next, o,
						     coords);
				} /* if */
			      k++;
			    } /* for */

			  /* notify&redraw are only necessary if coords are
			     really changed by the snap to grid operation */
			  if(o->modified)
			    {
			      ay_pact_notify(o, j, k-pact_numpo[j]);
			      redraw = AY_TRUE;
			    } /* if */
			} /* if grid */
		    } /* if pecoords */

		  if(view->usegrid && (view->grid != 0.0))
		    {
		      ay_viewt_griddify(togl, &winx, &winy);
		    }
		} /* if */
	    } /* if */
	}
      else
	{
	  ay_error(AY_EARGS, fname, NULL);
	  return TCL_OK;
	} /* if */

      if(!start)
	{
	  dx = -(oldwinx - winx) * view->conv_x;
	  dy = (oldwiny - winy) * view->conv_y;

	  /* Side or Top view? */
	  if(view->type == AY_VTSIDE)
	    {
	      dz = -dx;
	      dx = 0.0;
	    }

	  if(view->type == AY_VTTOP)
	    {
	      dz = -dy;
	      dy = 0.0;
	    }

	  movX = m[0]*dx+m[4]*dy+m[8]*dz;
	  movY = m[1]*dx+m[5]*dy+m[9]*dz;
	  movZ = m[2]*dx+m[6]*dy+m[10]*dz;

	  if(pact_pe.coords)
	    {
	      for(i = 0; i < pact_numpo[j]; i++)
		{
		  coords = pact_pe.coords[k];
		  k++;
		  coords[0] += movX;
		  coords[1] += movY;
		  coords[2] += movZ;
		} /* for */

	      if((fabs(movX) > AY_EPSILON)||
		 (fabs(movY) > AY_EPSILON)||
		 (fabs(movZ) > AY_EPSILON))
		{
		  ay_pact_notify(o, j, k-pact_numpo[j]);
		  redraw = AY_TRUE;
		}
	    } /* if have coords */
	} /* if !start */
    } /* for all objects with picked points */

  oldwinx = winx;
  oldwiny = winy;

  if(redraw)
    {
      if(!ay_prefs.lazynotify || start)
	{
	  (void)ay_notify_parent();
	}

      ay_toglcb_display(togl);
    }

 return TCL_OK;
} /* ay_pact_petcb */


/* ay_pact_wetcb:
 *  action callback for single point weight editing
 */
int
ay_pact_wetcb(struct Togl *togl, int argc, char *argv[])
{
 /*int ay_status = AY_OK;*/
 char fname[] = "weight_edit";
 Tcl_Interp *interp = Togl_Interp(togl);
 ay_view_object *view = Togl_GetClientData(togl);
 double g = 0.1, dx = 0.0, winx = 0.0, new_weight = 0.0, *coords;
 static double oldwinx = 0.0, olddx = 0.0;
 static int absolute = AY_TRUE;
 int i = 0, j, k = 0, notifyparent = AY_FALSE;
 ay_object *o = NULL;
 ay_nurbcurve_object *nc = NULL;
 ay_nurbpatch_object *np = NULL;
 ay_pamesh_object *pm = NULL;

  if(pact_objectslen == 0)
    return TCL_OK;

  /* parse args */
  if(argc >= 4)
    {
      if(!strcmp(argv[2], "-winx"))
	{
	  Tcl_GetDouble(interp, argv[3], &winx);
	}
      else
	if(!strcmp(argv[2], "-start"))
	  {
	    Tcl_GetDouble(interp, argv[3], &winx);
	    oldwinx = winx;
	    if(argc > 4)
	      Tcl_GetInt(interp, argv[4], &absolute);
	    else
	      absolute = AY_TRUE;
	  }
    }
  else
    {
      ay_error(AY_EARGS, fname, NULL);
      return TCL_OK;
    }

  if(!ay_selection)
    return TCL_OK;

  /* calculate amount of change */
  if(view->usegrid && (view->grid != 0.0))
    {
      g *= view->grid;
    }

  if(absolute)
    {
      /* absolute mode */
      dx = floor((winx-oldwinx)/10.0)*10.0;

      if(fabs(dx*g) < AY_EPSILON)
	{
	  /* always reset the weight to 1.0 when starting */
	  dx = 1.0;
	}
      else
	{
	  new_weight = dx;
	  if(dx > 0.0)
	    {
	      dx = 1.0+(dx*(g/10.0));
	    }
	  else
	    {
	      dx = 1.0-(-dx/(10.0/g));
	    }
	  /* clamp result to small non zero value (AY_EPSILON) */
	  if(fabs(dx) < AY_EPSILON)
	    dx = AY_EPSILON;
	}
    }
  else
    {
      /* relative mode */
      dx = floor((winx-oldwinx)/10.0);

      if(dx > olddx)
	{
	  olddx = dx;
	  dx = g;
	}
      else
	{
	  if(dx < olddx)
	    {
	      olddx = dx;
	      dx = -g;
	    }
	  else
	    {
	      dx = 0.0;
	    }
	}
    } /* if absolute or relative */

  /* apply change to selected point(s) */
  for(j = 0; j < pact_objectslen; j++)
    {
      o = pact_objects[j];

      for(i = 0; i < pact_numpo[j]; i++)
	{
	  coords = pact_pe.coords[k];
	  k++;
	  if(pact_typepo[j])
	    {
	      new_weight = coords[3];

	      if(absolute)
		{
		  new_weight = dx;
		}
	      else
		{
		  if(fabs(dx) > AY_EPSILON)
		    {
		      new_weight += dx;

		      if(fabs(new_weight) < AY_EPSILON)
			new_weight = AY_EPSILON;
		    }
		}

	      coords[3] = new_weight;
	    }
	  else
	    {
	      ay_error(AY_ERROR, fname, "Point is not rational.");
	    } /* if */
	} /* for */

      if(pact_typepo[j])
	{
	  switch(o->type)
	    {
	    case AY_IDNCURVE:
	      nc = (ay_nurbcurve_object *)o->refine;
	      if(fabs(1.0-new_weight) > AY_EPSILON)
		nc->is_rat = AY_TRUE;
	      else
		nc->is_rat = ay_nct_israt(o->refine);
	      break;
	    case AY_IDNPATCH:
	      np = (ay_nurbpatch_object *)o->refine;
	      if(fabs(1.0-new_weight) > AY_EPSILON)
		np->is_rat = AY_TRUE;
	      else
		np->is_rat = ay_npt_israt(o->refine);
	      break;
	    case AY_IDPAMESH:
	      pm = (ay_pamesh_object *)o->refine;
	      if(fabs(1.0-new_weight) > AY_EPSILON)
		pm->is_rat = AY_TRUE;
	      else
		pm->is_rat = ay_pmt_israt(o->refine);
	      break;
	    default:
	      break;
	    }
	  o->modified = AY_TRUE;
	  ay_pact_notify(o, j, k-pact_numpo[j]);
	  notifyparent = AY_TRUE;
	} /* if israt */
    } /* for */

  if(!ay_prefs.lazynotify && notifyparent)
    {
      (void)ay_notify_parent();
    }

 return TCL_OK;
} /* ay_pact_wetcb */


/* ay_pact_wrtcb:
 *  this action callback resets all weights
 */
int
ay_pact_wrtcb(struct Togl *togl, int argc, char *argv[])
{
 int ay_status = AY_OK;
 Tcl_Interp *interp = Togl_Interp(togl);
 char fname[] = "reset_weights";
 double p[3], *coords;
 double winXY[4] = {0}, dt;
 double obj[24] = {0}, pl[16] = {0};
 unsigned int i, j;
 int k, l;
 int notify_object = AY_FALSE, notify_parent = AY_FALSE;
 ay_object *o = NULL;
 ay_list_object *sel = ay_selection;
 ay_pointedit pe = {0};
 int reset_selected = AY_FALSE, dragselect = AY_FALSE;

  if(argc >= 3)
    {
      if(!strcmp(argv[2], "-selected"))
	reset_selected = AY_TRUE;
      if(!strcmp(argv[2], "-rect"))
	{
	  reset_selected = AY_TRUE;
	  dragselect = AY_TRUE;
	}
    }

  if(reset_selected)
    {
      if(dragselect)
	{
	  Tcl_GetDouble(interp, argv[3], &(winXY[0]));
	  Tcl_GetDouble(interp, argv[4], &(winXY[1]));
	  Tcl_GetDouble(interp, argv[5], &(winXY[2]));
	  Tcl_GetDouble(interp, argv[6], &(winXY[3]));

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

	  while(sel)
	    {
	      o = sel->object;
	      notify_object = AY_FALSE;

	      ay_viewt_winrecttoobj(togl, o, winXY[0], winXY[1],
				    winXY[2], winXY[3], obj);

	      ay_viewt_objrecttoplanes(obj, pl);

	      (void)ay_pact_getpoint(2, o, pl, &pe);

	      for(i = 0; i < pe.num; i++)
		{
		  if(pe.type == AY_PTRAT)
		    {
		      pe.coords[i][3] = 1.0;
		      o->modified = AY_TRUE;
		      notify_object = AY_TRUE;
		      notify_parent = AY_TRUE;
		    }
		}

	      if(notify_object)
		(void)ay_notify_object(o);

	      sel = sel->next;
	    } /* while */
	}
      else
	{
	  j = 0;
	  for(k = 0; k < pact_objectslen; k++)
	    {
	      if(pact_typepo[k])
		{
		  for(l = 0; l < pact_numpo[k]; l++)
		    {
		      coords = pact_pe.coords[j+l];
		      coords[3] = 1.0;
		    }
		  pact_objects[k]->modified = AY_TRUE;
		  (void)ay_notify_object(pact_objects[k]);
		  notify_parent = AY_TRUE;
		}
	      j += pact_numpo[k];
	    } /* for */
	} /* if dragselect */
    }
  else
    {
      /* reset all */
      while(sel)
	{
	  o = sel->object;

	  ay_status = ay_pact_getpoint(0, o, p, &pe);

	  if(ay_status)
	    {
	      ay_error(AY_ERROR, fname, NULL);
	    }

	  if(pe.coords && pe.type && (!pe.readonly))
	    {
	      for(i = 0; i < pe.num; i++)
		{
		  coords = pe.coords[i];
		  coords[3] = 1.0;
		} /* for */

	      o->modified = AY_TRUE;
	      (void)ay_notify_object(o);
	      notify_parent = AY_TRUE;
	    } /* if */

	  ay_pact_clearpointedit(&pe);

	  sel = sel->next;
	} /* while */
    } /* if sel/all */

  if(notify_parent)
    {
      (void)ay_notify_parent();
    }
  else
    {
      ay_error(AY_EWARN, fname, "No editable/rational points found.");
    }

 return TCL_OK;
} /* ay_pact_wrtcb */


/* ay_pact_snaptogridcb:
 *  this action callback snaps all selected points to grid coordinates
 *  if no points are selected, all selectable points are snapped
 */
int
ay_pact_snaptogridcb(struct Togl *togl, int argc, char *argv[])
{
 int ay_status = AY_OK;
 char fname[] = "snap_to_grid";
 Tcl_Interp *interp = Togl_Interp(togl);
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 double p[3], *coords;
 int mode = 0, notify_parent = AY_FALSE;
 unsigned int i = 0;
 ay_object *o = NULL;
 ay_list_object *sel = ay_selection;
 ay_point *pnt = NULL;
 ay_pointedit pe = {0};

  if(!sel)
    {
      ay_error(AY_ENOSEL, fname, NULL);
      return TCL_OK;
    }

  if(argc > 2)
    {
      Tcl_GetInt(interp, argv[2], &mode);
    }

  while(sel)
    {
      o = sel->object;
      if(mode > 1)
	{
	  /* snap objects */
	  if(mode == 2)
	    {
	      /* 3D */
	      ay_pact_griddify(&(o->movx), view->grid);
	      ay_pact_griddify(&(o->movy), view->grid);
	      ay_pact_griddify(&(o->movz), view->grid);
	    }
	  else
	    {
	      /* 2D */
	      switch(view->type)
		{
		case AY_VTFRONT:
		case AY_VTTRIM:
		  ay_pact_griddify(&(o->movx), view->grid);
		  ay_pact_griddify(&(o->movy), view->grid);
		  break;
		case AY_VTSIDE:
		  ay_pact_griddify(&(o->movy), view->grid);
		  ay_pact_griddify(&(o->movz), view->grid);
		  break;
		case AY_VTTOP:
		  ay_pact_griddify(&(o->movx), view->grid);
		  ay_pact_griddify(&(o->movz), view->grid);
		  break;
		default:
		  /* XXXX output proper error message */
		  break;
		} /* switch */
	    } /* if 2D/3D */

	  o->modified = AY_TRUE;
	}
      else
	{
	  if(!o->selp)
	    {
	      ay_status = ay_pact_getpoint(0, o, p, &pe);

	      if(ay_status || (!pe.coords) || pe.readonly)
		{
		  ay_error(AY_ERROR, fname, NULL);
		}
	      else
		{
		  for(i = 0; i < pe.num; i++)
		    {
		      coords = pe.coords[i];
		      if(mode == 0)
			{
			  /* 3D */
			  ay_pact_griddify(&(coords[0]), view->grid);
			  ay_pact_griddify(&(coords[1]), view->grid);
			  ay_pact_griddify(&(coords[2]), view->grid);
			}
		      else
			{
			  /* 2D */
			  switch(view->type)
			    {
			    case AY_VTFRONT:
			    case AY_VTTRIM:
			      ay_pact_griddify(&(coords[0]), view->grid);
			      ay_pact_griddify(&(coords[1]), view->grid);
			      break;
			    case AY_VTSIDE:
			      ay_pact_griddify(&(coords[1]), view->grid);
			      ay_pact_griddify(&(coords[2]), view->grid);
			      break;
			    case AY_VTTOP:
			      ay_pact_griddify(&(coords[0]), view->grid);
			      ay_pact_griddify(&(coords[2]), view->grid);
			      break;
			    default:
			      /* XXXX output proper error message */
			      break;
			    } /* switch */
			} /* if 2D/3D */
		    } /* for */
		} /* if have pnts */

	      ay_pact_clearpointedit(&pe);
	    }
	  else
	    {
	      pnt = o->selp;
	      while(pnt)
		{
		  if(!pnt->readonly)
		    {
		      coords = pnt->point;
		      if(mode == 0)
			{
			  /* 3D */
			  ay_pact_griddify(&(coords[0]), view->grid);
			  ay_pact_griddify(&(coords[1]), view->grid);
			  ay_pact_griddify(&(coords[2]), view->grid);
			}
		      else
			{
			  /* 2D */
			  switch(view->type)
			    {
			    case AY_VTFRONT:
			    case AY_VTTRIM:
			      ay_pact_griddify(&(coords[0]), view->grid);
			      ay_pact_griddify(&(coords[1]), view->grid);
			      break;
			    case AY_VTSIDE:
			      ay_pact_griddify(&(coords[1]), view->grid);
			      ay_pact_griddify(&(coords[2]), view->grid);
			      break;
			    case AY_VTTOP:
			      ay_pact_griddify(&(coords[0]), view->grid);
			      ay_pact_griddify(&(coords[2]), view->grid);
			      break;
			    default:
			      /* XXXX output proper error message */
			      break;
			    } /* switch */
			} /* if 2D/3D */

		      o->modified = AY_TRUE;

		    } /* if !readonly */
		  pnt = pnt->next;
		} /* while */
	    } /* if have selp */
	} /* if snap objs/pnts */

      if(o->modified)
	{
	  ay_notify_object(o);
	  notify_parent = AY_TRUE;
	}
      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_pact_snaptogridcb */


/* ay_pact_snaptomarkcb:
 *  this action callback snaps all selected points (mode == 0)
 *  or objects (mode == 1) to the mark
 */
int
ay_pact_snaptomarkcb(struct Togl *togl, int argc, char *argv[])
{
 char fname[] = "snap_to_mark";
 Tcl_Interp *interp = Togl_Interp(togl);
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 ay_object *o = NULL;
 ay_list_object *sel = ay_selection;
 ay_point *pnt = NULL;
 int mode = 0, notify_parent = AY_FALSE;
 double p[4], m[16], mi[16];

  if(!sel)
    {
      ay_error(AY_ENOSEL, fname, NULL);
      return TCL_OK;
    }

  if(argc == -1)
    mode = 1;
  else
    if(argc > 1)
      Tcl_GetInt(interp, argv[2], &mode);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
   glLoadIdentity();
   ay_trafo_concatparent(ay_currentlevel->next);

   if(mode)
     {
       glGetDoublev(GL_MODELVIEW_MATRIX, m);
       ay_trafo_invmatrix(m, mi);
     }

   while(sel)
     {
       o = sel->object;

       if(mode == 0)
	 {
	   /* snap points */
	   if(o->selp)
	     {
	       glPushMatrix();
	        glTranslated(o->movx, o->movy, o->movz);
		ay_quat_torotmatrix(o->quat, m);
		glMultMatrixd(m);
		glScaled(o->scalx, o->scaly, o->scalz);

		glGetDoublev(GL_MODELVIEW_MATRIX, m);
		ay_trafo_invmatrix(m, mi);
	       glPopMatrix();

	       pnt = o->selp;
	       while(pnt)
		 {
		   if(!pnt->readonly)
		     {
		       memcpy(pnt->point, view->markworld, 3*sizeof(double));

		       /* the mark data is non rational, so it makes
			  no sense to use apply4() anyway */
		       ay_trafo_apply3(pnt->point, mi);

		       /* reset weight */
		       if(pnt->type == AY_PTRAT)
			 {
			   pnt->point[3] = 1.0;
			 }

		       o->modified = AY_TRUE;
		     } /* if */
		   pnt = pnt->next;
		 } /* while */

	       if(o->modified)
		 {
		   ay_notify_object(o);
		   notify_parent = AY_TRUE;
		 }
	     }
	   else
	     {
	       /* XXXX output error message? */
	     } /* if have selp */
	 }
       else
	 {
	   /* snap objects */
	   p[0] = view->markworld[0];
	   p[1] = view->markworld[1];
	   p[2] = view->markworld[2];
	   ay_trafo_apply3(p, mi);
	   o->movx = p[0];
	   o->movy = p[1];
	   o->movz = p[2];
	   notify_parent = AY_TRUE;
	 } /* if snap pnts/objs */

       sel = sel->next;
     } /* while */

  glPopMatrix();

  if(notify_parent && (argc != -1))
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_pact_snaptomarkcb */


/* ay_pact_notify:
 *
 */
int
ay_pact_notify(ay_object *o, int j, int k)
{
 ay_point *oldselp = NULL, tmpselp = {0};

  if(!o)
    return AY_ENULL;

  if(pact_numpo[j] == 1)
    {
      tmpselp.point = pact_pe.coords[k];
      tmpselp.index = pact_pe.indices[k];
    }
  else
    {
      tmpselp.point = NULL;
    }

  oldselp = o->selp;
  o->selp = &tmpselp;
  o->modified = 3;

  ay_notify_object(o);

  o->selp = oldselp;

 return AY_OK;
} /* ay_pact_notify */


/* ay_pact_multiptcb:
 *  manipulate multiplicity point action callback
 *
 */
int
ay_pact_multiptcb(struct Togl *togl, int argc, char *argv[])
{
 int ay_status = AY_OK;
 char fname[] = "multiply_point";
 Tcl_Interp *interp = Togl_Interp(togl);
 int decrease = AY_FALSE, notify_parent = AY_FALSE;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL;

  if(!sel)
    {
      ay_error(AY_ENOSEL, fname, NULL);
    }

  if(argc > 1)
    {
      Tcl_GetInt(interp, argv[2], &decrease);
    }

  while(sel)
    {
      o = sel->object;

      if(o->selp)
	{
	  switch(o->type)
	    {
	    case AY_IDNCURVE:
	      if(!decrease)
		ay_status = ay_pact_multincnc(o);
	      else
		ay_status = ay_pact_multdecnc(o);
	      break;
	    case AY_IDNPATCH:
	      /*
	      ay_status = ay_pact_multinp((ay_npatch_object *)
					  (o->refine), decrease);
	      */
	      ay_status = AY_OK;
	      break;

	    default:
	      ay_error(AY_EWTYPE, fname, "NCurve, NPatch");
	      ay_status = AY_ERROR;
	      break;
	    }

	  if(ay_status)
	    {
	      ay_error(ay_status, fname, "Error changing multiplicity.");
	    }
	  else
	    {
	      notify_parent = AY_TRUE;
	      (void)ay_notify_object(o);
	      o->modified = AY_TRUE;
	    } /* if */
	} /* if selp */
      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_pact_multiptcb */


/** ay_pact_multincnc:
 *  increase multiplicity of selected points of a NURBS curve
 *
 * \param[in,out] o  NURBS curve object to process
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_pact_multincnc(ay_object *o)
{
 int ay_status = AY_OK;
 ay_nurbcurve_object *nc = NULL;
 ay_point *pnt1 = NULL, *pnt2 = NULL;
 ay_point *opnt1;
 int i, a, b, m, stride = 4;
 double *cv, *p1, *p2, *newcv = NULL;

  nc = (ay_nurbcurve_object*)o->refine;
  cv = nc->controlv;

  /* copy the list of points to work on */
  opnt1 = o->selp;
  while(opnt1)
    {
      if(!(pnt2 = calloc(1, sizeof(ay_point))))
	{ ay_status = AY_EOMEM; goto cleanup; }

      memcpy(pnt2, opnt1, sizeof(ay_point));

      pnt2->next = pnt1;
      pnt1 = pnt2;

      opnt1 = opnt1->next;
    } /* while */

  while(pnt1)
    {
      p1 = &(cv[pnt1->index*stride]);

      /* remove potential selected points that are pointing
	 to the same coordinates and get current multiplicity of
	 the selected point pnt1 */
      m = 1;
      pnt2 = pnt1->next;
      while(pnt2)
	{
	  p2 = pnt2->point;
	  if(AY_V3COMP(p1, p2))
	    {
	      pnt1->next = pnt2->next;
	      free(pnt2);
	      pnt2 = pnt1->next;

	      m++;
	    }
	  else
	    break;
	} /* while */

      /* if feasable, increase multiplicity of p1 */
      if(m < (nc->order-1))
	{
	  /* allocate new coordinate vector */
	  if(!(newcv = calloc((nc->length+1)*stride, sizeof(double))))
	    { ay_status = AY_EOMEM; goto cleanup; }

	  a = 0;
	  b = 0;
	  for(i = 0; i < nc->length; i++)
	    {
	      memcpy(&(newcv[a]), &(cv[b]), stride*sizeof(double));
	      if((unsigned int)i == pnt1->index)
		{
		  a += stride;
		  memcpy(&(newcv[a]), &(cv[b]), stride*sizeof(double));
		}
	      a += stride;
	      b += stride;
	    }

	  /* adjust order/length/knots */
	  if(nc->knot_type == AY_KTCUSTOM)
	    {
	      ay_status = ay_knots_insert(pnt1->index, nc->order,
					  nc->length, &(nc->knotv));

	      if(ay_status)
		nc->knot_type = AY_KTNURB;
	    }

	  nc->length++;

	  if(nc->knot_type == AY_KTBEZIER)
	    nc->order++;

	  /* rewrite indices of trailing selected points */
	  opnt1 = o->selp;
	  while(opnt1)
	    {
	      if(opnt1->index > pnt1->index)
		opnt1->index++;
	      opnt1 = opnt1->next;
	    }

	  pnt2 = pnt1->next;
	  while(pnt2)
	    {
	      if(pnt2->index > pnt1->index)
		pnt2->index++;

	      pnt2->point = &(newcv[pnt2->index*stride]);

	      pnt2 = pnt2->next;
	    }

	  /* replace control vector */
	  free(nc->controlv);
	  nc->controlv = newcv;
	  cv = newcv;
	  newcv = NULL;
	} /* if m < order-1 */

      /* remove current selected point and
	 advance to the next */
      pnt2 = pnt1->next;
      free(pnt1);
      pnt1 = pnt2;
    } /* while pnt1 */

  /* create new knot vectors */
  if(nc->knot_type != AY_KTCUSTOM)
    {
      ay_status = ay_knots_createnc(nc);
    }

  /* update pointers in selected points */
  (void)ay_pact_getpoint(3, o, NULL, NULL);

  /* re-create multiple points */
  if(nc->createmp)
    ay_nct_recreatemp(nc);

  /* add all multiple points to point selection */
  ay_selp_selectmpnc(o, AY_FALSE/*select_all*/);

  /* correct the curve type? */
  if(nc->type == AY_CTPERIODIC)
    {
      ay_nct_settype(nc);
    }

cleanup:

  while(pnt1)
    {
      pnt2 = pnt1->next;
      free(pnt1);
      pnt1 = pnt2;
    }

 return ay_status;
} /* ay_pact_multincnc */


/** ay_pact_multdecnc:
 *  decrease multiplicity of selected points of a NURBS curve
 *
 * \param[in,out] o  NURBS curve object to process
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_pact_multdecnc(ay_object *o)
{
 int ay_status = AY_OK;
 ay_nurbcurve_object *nc = NULL;
 ay_point *pnt1 = NULL, *pnt2 = NULL;
 ay_point *opnt1 = NULL;
 int i, a, b, m, stride = 4;
 double *cv, *p1, *p2, *newcv = NULL;

  nc = (ay_nurbcurve_object*)o->refine;
  cv = nc->controlv;

  /* copy the list of points to work on */
#if 0
  i = 0;
  opnt1 = o->selp;
  while(opnt1)
    {
      if(!(pnt2 = calloc(1, sizeof(ay_point))))
	{ ay_status = AY_EOMEM; goto cleanup; }

      memcpy(pnt2, opnt1, sizeof(ay_point));

      pnt2->next = pnt1;
      pnt1 = pnt2;

      i++;
      opnt1 = opnt1->next;
    } /* while */

  opnt1 = pnt1;
#endif

  ay_selp_copy(o->selp, &pnt1);
  opnt1 = pnt1;

  /*pnt1 = o->selp;*/
  while(pnt1)
    {
      pnt2 = pnt1->next;
      p1 = &(cv[pnt1->index*stride]);

      /* get current multiplicity of selected point */
      m = 1;
      p2 = p1+stride;
      while(((pnt1->index + m) < (unsigned int)nc->length) &&
	    AY_V3COMP(p1, p2))
	{
	  p2 += stride;
	  m++;
	}

      /* if feasable, decrease multiplicity of p1 */
      if(m > 1)
	{
	  /* allocate new coordinate vector */
	  if(!(newcv = calloc((nc->length-1)*stride, sizeof(double))))
	    { ay_status = AY_EOMEM; goto cleanup; }

	  a = 0;
	  b = 0;
	  for(i = 0; i < nc->length; i++)
	    {
	      if((unsigned int)i != pnt1->index)
		{
		  memcpy(&(newcv[a]), &(cv[b]), stride*sizeof(double));
		  a += stride;
		}
	      b += stride;
	    }

	  /* adjust order/length/knots */
	  if(nc->knot_type == AY_KTCUSTOM)
	    {
	      ay_status = ay_knots_remove(pnt1->index, nc->order,
					  nc->length, &(nc->knotv));

	      if(ay_status)
		nc->knot_type = AY_KTNURB;
	    }

	  nc->length--;

	  if(nc->knot_type == AY_KTBEZIER)
	    nc->order--;

	  if(nc->length < nc->order)
	    {
	      nc->order = nc->length;
	      if(nc->knot_type == AY_KTCUSTOM)
		{
		  nc->knot_type = AY_KTNURB;
		}
	    }

	  /* adjust indices of trailing points and let
	     the points point to the new coordinate vector
	     so that we can continue to work with them */
	  if(pnt2)
	    {
	      while(pnt2)
		{
		  if(pnt2->index > pnt1->index)
		    pnt2->index--;

		  pnt2->point = &(newcv[pnt2->index*stride]);

		  pnt2 = pnt2->next;
		}

	      /* advance to the next different point */
	      pnt2 = pnt1->next;
	      p2 = pnt2->point;
	      while(AY_V3COMP(p1, p2))
		{
		  pnt2 = pnt2->next;
		  if(!pnt2)
		    break;
		  p2 = pnt2->point;
		} /* while */
	    } /* if */

	  /* replace control vector */
	  free(nc->controlv);
	  nc->controlv = newcv;
	  cv = newcv;
	  newcv = NULL;

	  /* remove currently processed selected point
	     and readjust the indices of trailing points */
	  ay_selp_rem(o, pnt1->index);
	} /* if m > 1 */

      pnt1 = pnt2;
    } /* while */

  /* create new knot vectors */
  if(nc->knot_type != AY_KTCUSTOM)
    {
      ay_status = ay_knots_createnc(nc);
    }

  /* re-create multiple points */
  if(nc->createmp)
    ay_nct_recreatemp(nc);

  /* correct the curve type */
  ay_nct_settype(nc);

cleanup:

  /* remove the copy of selected points */
  pnt1 = opnt1;
  while(pnt1)
    {
      pnt2 = pnt1->next;
      free(pnt1);
      pnt1 = pnt2;
    }

 return ay_status;
} /* ay_pact_multdecnc */


/** ay_pact_registerinsert:
 *  register a point insert callback
 *
 * \param[in] inscb  point insert callback
 * \param[in] type_id  object type for which to register the callback (AY_ID...)
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_pact_registerinsert(ay_inspntcb *inscb, unsigned int type_id)
{
 int ay_status = AY_OK;

  /* register insert callback */
  ay_status = ay_table_addcallback(&ay_pact_insertcbt, (ay_voidfp)inscb,
				   type_id);

 return ay_status;
} /* ay_pact_registerinsert */


/** ay_pact_registerdelete:
 *  register a point delete callback
 *
 * \param[in] delcb  point delete callback
 * \param[in] type_id  object type for which to register the callback (AY_ID...)
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_pact_registerdelete(ay_delpntcb *delcb, unsigned int type_id)
{
 int ay_status = AY_OK;

  /* register delete callback */
  ay_status = ay_table_addcallback(&ay_pact_deletecbt, (ay_voidfp)delcb,
				   type_id);

 return ay_status;
} /* ay_pact_registerdelete */


/** ay_pact_init:
 *  initialize point action module
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_pact_init(Tcl_Interp *interp)
{
 int ay_status = AY_OK;
 char fname[] = "pact_init";

  if((ay_status = ay_table_initftable(&ay_pact_insertcbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_pact_deletecbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

 return ay_status;
} /* ay_pact_init */

