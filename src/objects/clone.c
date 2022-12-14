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

/* clone.c - clone object */

static char *ay_clone_name = "Clone";

static char *ay_mirror_name = "Mirror";


/* prototypes of functions local to this module: */

int ay_clone_getpntcb(int mode, ay_object *o, double *p, ay_pointedit *pe);

int ay_clone_notifycb(ay_object *o);


/* functions: */

/* ay_clone_createcb:
 *  create callback function of clone object
 */
int
ay_clone_createcb(int argc, char *argv[], ay_object *o)
{
 char fname[] = "crtclone";
 ay_clone_object *new = NULL;
 int mirror_mode = 1;

  if(!argv || !o)
    return AY_ENULL;

  if(!(new = calloc(1, sizeof(ay_clone_object))))
    {
      ay_error(AY_EOMEM, fname, NULL);
      return AY_ERROR;
    }

  if(argc > 2)
    {
      /* provided for script backwards compatibility */
      if(!strcmp(argv[2], "-mirror"))
	{
	  o->type = AY_IDMIRROR;
	  mirror_mode = atoi(argv[3]);
	}
    }

  if(o->type == AY_IDMIRROR)
    {
      new->mirror = mirror_mode;
    }
  else
    {
      new->scalx = 1.0;
      new->scaly = 1.0;
      new->scalz = 1.0;

      new->quat[3] = 1.0;

      new->numclones = 1;
    }

  o->parent = AY_TRUE;

  o->refine = new;

 return AY_OK;
} /* ay_clone_createcb */


/* ay_clone_deletecb:
 *  delete callback function of clone object
 */
int
ay_clone_deletecb(void *c)
{
 ay_clone_object *clone = NULL;
 ay_object *t = NULL;

  if(!c)
    return AY_ENULL;

  clone = (ay_clone_object *)(c);

  if(clone->pnts)
    free(clone->pnts);

  while(clone->clones)
    {
      t = clone->clones;
      clone->clones = t->next;
      free(t);
    }

  free(clone);

 return AY_OK;
} /* ay_clone_deletecb */


/* ay_clone_copycb:
 *  copy callback function of clone object
 */
int
ay_clone_copycb(void *src, void **dst)
{
 ay_clone_object *clone;

  if(!src || !dst)
    return AY_ENULL;

  if(!(clone = malloc(sizeof(ay_clone_object))))
    return AY_EOMEM;

  memcpy(clone, src, sizeof(ay_clone_object));

  clone->pnts = NULL;
  clone->pntslen = 0;
  clone->clones = NULL;

  *dst = (void *)clone;

 return AY_OK;
} /* ay_clone_copycb */


/* ay_clone_drawcb:
 *  draw (display in an Ayam view window) callback function of clone object
 */
int
ay_clone_drawcb(struct Togl *togl, ay_object *o)
{
 ay_clone_object *clone = NULL;
 ay_object *c = NULL;

  if(!o)
    return AY_ENULL;

  clone = (ay_clone_object *)o->refine;

  if(!clone)
    return AY_ENULL;

  c = clone->clones;
  while(c)
    {
      ay_draw_object(togl, c, AY_TRUE);
      c = c->next;
    }

 return AY_OK;
} /* ay_clone_drawcb */


/* ay_clone_shadecb:
 *  shade (display in an Ayam view window) callback function of clone object
 */
int
ay_clone_shadecb(struct Togl *togl, ay_object *o)
{
 ay_clone_object *clone = NULL;
 ay_object *c = NULL;

  if(!o)
    return AY_ENULL;

  clone = (ay_clone_object *)o->refine;

  if(!clone)
    return AY_ENULL;

  c = clone->clones;
  while(c)
    {
      ay_shade_object(togl, c, AY_FALSE);
      c = c->next;
    }

 return AY_OK;
} /* ay_clone_shadecb */


/* ay_clone_drawhcb:
 *  draw handles (in an Ayam view window) callback function of clone object
 */
int
ay_clone_drawhcb(struct Togl *togl, ay_object *o)
{
 unsigned int i;
 double *pnts;
 double tm[16];
 ay_clone_object *clone = NULL;
 ay_object *c;
 ay_voidfp *arr;
 ay_drawcb *cb;
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);

  if(!o)
    return AY_ENULL;

  clone = (ay_clone_object *)o->refine;

  if(!clone)
    return AY_ENULL;

  if(view->drawhandles == 2)
    {
      arr = ay_drawhcbt.arr;
      cb = (ay_drawcb *)(arr[AY_IDINSTANCE]);
      glPushMatrix();
      c = clone->clones;
      while(c)
	{
	  glLoadIdentity();
	  glTranslated((GLdouble)c->movx, (GLdouble)c->movy, (GLdouble)c->movz);
	  ay_quat_torotmatrix(c->quat, tm);
	  glMultMatrixd((GLdouble*)tm);
	  glScaled((GLdouble)c->scalx, (GLdouble)c->scaly, (GLdouble)c->scalz);
	  (void) cb(togl, c);
	  c = c->next;
	}
      glPopMatrix();
      return AY_OK;
    }

  if(!clone->pnts)
    {
      clone->pntslen = 1;
      ay_clone_notifycb(o);
    }

  if(clone->pnts)
    {
      pnts = clone->pnts;
      glColor3f((GLfloat)ay_prefs.obr, (GLfloat)ay_prefs.obg,
		(GLfloat)ay_prefs.obb);

      glBegin(GL_POINTS);
       if(clone->pntsrat && ay_prefs.rationalpoints)
	 {
	   for(i = 0; i < clone->pntslen; i++)
	     {
	       glVertex3d((GLdouble)pnts[0]*pnts[3],
			  (GLdouble)pnts[1]*pnts[3],
			  (GLdouble)pnts[2]*pnts[3]);
	       pnts += 4;
	     }
	 }
       else
	 {
	   for(i = 0; i < clone->pntslen; i++)
	     {
	       glVertex3dv((GLdouble *)pnts);
	       pnts += 4;
	     }
	 }
      glEnd();

      glColor3f((GLfloat)ay_prefs.ser, (GLfloat)ay_prefs.seg,
		(GLfloat)ay_prefs.seb);
    }

 return AY_OK;
} /* ay_clone_drawhcb */


/* ay_clone_getpntcb:
 *  get point (editing and selection) callback function of clone object
 */
int
ay_clone_getpntcb(int mode, ay_object *o, double *p, ay_pointedit *pe)
{
 ay_clone_object *clone = NULL;
 ay_object *c;
 ay_voidfp *arr = NULL;
 ay_getpntcb *cb = NULL;
 double pc[3], mc[16], mcr[16];

  if(!o || ((mode != 3) && (!p || !pe)))
    return AY_ENULL;

  clone = (ay_clone_object *)o->refine;

  if(!clone)
    return AY_ENULL;

  if(pe && pe->type == 2)
    {
      arr = ay_getpntcbt.arr;
      cb = (ay_getpntcb *)(arr[AY_IDINSTANCE]);
      c = clone->clones;
      while(c)
	{
	  ay_trafo_identitymatrix(mc);
	  ay_trafo_translatematrix(c->movx, c->movy, c->movz, mc);
	  ay_quat_torotmatrix(c->quat, mcr);
	  ay_trafo_multmatrix(mc, mcr);
	  ay_trafo_scalematrix(c->scalx, c->scaly, c->scalz, mc);

	  AY_APTRAN3(pc, p, mc)

	  (void) cb(mode, c, pc, pe);
	  if(pe->num)
	    break;
	  c = c->next;
	}
      return AY_OK;
    }

  if(!clone->pnts)
    {
      if(mode != 3)
	{
	  clone->pntslen = 1;
	  ay_clone_notifycb(o);
	}
      else
	{
	  /* mode is 3 (recover selected points) but we have no points =>
	     clear the selected points and bail out */
	  clone->pntslen = 0;
	  ay_selp_clear(o);
	}
    }

  if(clone->pntslen)
    return ay_selp_getpnts(mode, o, p, pe, 1, clone->pntslen, 4,
			   ay_prefs.rationalpoints, clone->pnts);
  else
    return AY_OK;
} /* ay_clone_getpntcb */


/* ay_clone_setpropcb:
 *  set property (from Tcl to C context) callback function of clone object
 */
int
ay_clone_setpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
 /*int ay_status = AY_OK;*/
 char *arc = "CloneAttrData";
 char *arm = "MirrorAttrData";
 /*char fname[] = "clone_setpropcb";*/
 Tcl_Obj *to = NULL;
 int i, itemp;
 double dtemp;
 double xaxis[3] = {1.0,0.0,0.0};
 double yaxis[3] = {0.0,1.0,0.0};
 double zaxis[3] = {0.0,0.0,1.0};
 double quat[4], drot;
 ay_clone_object *clone = NULL;
 int pasteProp = AY_FALSE;
 char *q[4] = {"Quat0","Quat1","Quat2","Quat3"};

  if(!interp || !o)
    return AY_ENULL;

  clone = (ay_clone_object *)o->refine;

  if(!clone)
    return AY_ENULL;

  if(o->type == AY_IDMIRROR)
    {
      if((to = Tcl_GetVar2Ex(interp, arm, "Plane",
			     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
	{
	  Tcl_GetIntFromObj(interp, to, &(itemp));
	  clone->mirror = itemp+1;
	}
    }
  else
    {
      if((to = Tcl_GetVar2Ex(interp, arc, "NumClones",
			     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
	Tcl_GetIntFromObj(interp, to, &(clone->numclones));

      if(clone->numclones < 0)
	clone->numclones = 0;

      if((to = Tcl_GetVar2Ex(interp, arc, "Rotate",
			     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
	Tcl_GetIntFromObj(interp, to, &(clone->rotate));

      if((to = Tcl_GetVar2Ex(interp, arc, "Translate_X",
			     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
	Tcl_GetDoubleFromObj(interp, to, &(clone->movx));

      if((to = Tcl_GetVar2Ex(interp, arc, "Translate_Y",
			     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
	Tcl_GetDoubleFromObj(interp, to, &(clone->movy));

      if((to = Tcl_GetVar2Ex(interp, arc, "Translate_Z",
			     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
	Tcl_GetDoubleFromObj(interp, to, &(clone->movz));

      for(i = 0; i < 4; i++)
	{
	  if((to = Tcl_GetVar2Ex(interp, arc, q[i],
				 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
	    Tcl_GetDoubleFromObj(interp, to, &(clone->quat[i]));
	}

      if((to = Tcl_GetVar2Ex(interp, arc, "Rotate_X",
			     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
	{
	  Tcl_GetDoubleFromObj(interp, to, &(dtemp));
	  if(clone->rotx != dtemp)
	    {
	      if(!pasteProp)
		{
		  drot = (clone->rotx - dtemp);
		  ay_quat_axistoquat(xaxis, AY_D2R(drot), quat);
		  ay_quat_add(quat, clone->quat, clone->quat);
		}
	      clone->rotx = dtemp;
	    }
	}

      if((to = Tcl_GetVar2Ex(interp, arc, "Rotate_Y",
			     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
	{
	  Tcl_GetDoubleFromObj(interp, to, &(dtemp));
	  if(clone->roty != dtemp)
	    {
	      if(!pasteProp)
		{
		  drot = (clone->roty - dtemp);
		  ay_quat_axistoquat(yaxis, AY_D2R(drot), quat);
		  ay_quat_add(quat, clone->quat, clone->quat);
		}
	      clone->roty = dtemp;
	    }
	}

      if((to = Tcl_GetVar2Ex(interp, arc, "Rotate_Z",
			     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
	{
	  Tcl_GetDoubleFromObj(interp, to, &(dtemp));
	  if(clone->rotz != dtemp)
	    {
	      if(!pasteProp)
		{
		  drot = (clone->rotz - dtemp);
		  ay_quat_axistoquat(zaxis, AY_D2R(drot), quat);
		  ay_quat_add(quat, clone->quat, clone->quat);
		}
	      clone->rotz = dtemp;
	    }
	}

      if(clone->rotx == 0.0 && clone->roty == 0.0 && clone->rotz == 0.0)
	{
	  clone->quat[0] = 0.0;
	  clone->quat[1] = 0.0;
	  clone->quat[2] = 0.0;
	  clone->quat[3] = 1.0;
	}

      if((to = Tcl_GetVar2Ex(interp, arc, "Scale_X",
			     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
	{
	  Tcl_GetDoubleFromObj(interp, to, &(dtemp));
	  if(dtemp != 0.0)
	    clone->scalx = dtemp;
	}

      if((to = Tcl_GetVar2Ex(interp, arc, "Scale_Y",
			     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
	{
	  Tcl_GetDoubleFromObj(interp, to, &(dtemp));
	  if(dtemp != 0.0)
	    clone->scaly = dtemp;
	}

      if((to = Tcl_GetVar2Ex(interp, arc, "Scale_Z",
			     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
	{
	  Tcl_GetDoubleFromObj(interp, to, &(dtemp));
	  if(dtemp != 0.0)
	    clone->scalz = dtemp;
	}
    } /* if mirror or clone */

  ay_notify_object(o);

  o->modified = AY_TRUE;
  ay_notify_parent();

 return AY_OK;
} /* ay_clone_setpropcb */


/* ay_clone_getpropcb:
 *  get property (from C to Tcl context) callback function of clone object
 */
int
ay_clone_getpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
 char *arc = "CloneAttrData", *quatstr = NULL;
 char *arm = "MirrorAttrData";
 int i = 0;
 ay_clone_object *clone = NULL;
 char *q[4] = {"Quat0","Quat1","Quat2","Quat3"};

  if(!interp || !o)
    return AY_ENULL;

  clone = (ay_clone_object *)(o->refine);

  if(!clone)
    return AY_ENULL;

  if(o->type == AY_IDMIRROR)
    {
      Tcl_SetVar2Ex(interp, arm, "Plane",
		    Tcl_NewIntObj(clone->mirror-1),
		    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
    }
  else
    {
      if(!(quatstr = calloc(TCL_DOUBLE_SPACE*4+10, sizeof(char))))
	return AY_EOMEM;

      Tcl_SetVar2Ex(interp, arc, "NumClones",
		    Tcl_NewIntObj(clone->numclones),
		    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

      Tcl_SetVar2Ex(interp, arc, "Rotate",
		    Tcl_NewIntObj(clone->rotate),
		    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

      Tcl_SetVar2Ex(interp, arc, "Translate_X",
		  Tcl_NewDoubleObj(clone->movx),
		  TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

      Tcl_SetVar2Ex(interp, arc, "Translate_Y",
		  Tcl_NewDoubleObj(clone->movy),
		  TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

      Tcl_SetVar2Ex(interp, arc, "Translate_Z",
		  Tcl_NewDoubleObj(clone->movz),
		  TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

      for(i = 0; i < 4; i++)
	Tcl_SetVar2Ex(interp, arc, q[i],
		      Tcl_NewDoubleObj(clone->quat[i]),
		      TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

      sprintf(quatstr, "[%.2lg, %.2lg, %.2lg, %.2lg]",
	      clone->quat[0], clone->quat[1], clone->quat[2], clone->quat[3]);
      Tcl_SetVar2Ex(interp, arc, "Quaternion", Tcl_NewStringObj(quatstr, -1),
		    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

      sprintf(quatstr, "[%lg, %lg, %lg, %lg]",
	      clone->quat[0], clone->quat[1], clone->quat[2], clone->quat[3]);
      Tcl_SetVar2Ex(interp, arc, "QuaternionBall",
		    Tcl_NewStringObj(quatstr, -1),
		    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

      Tcl_SetVar2Ex(interp, arc, "Rotate_X",
		  Tcl_NewDoubleObj(clone->rotx),
		  TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

      Tcl_SetVar2Ex(interp, arc, "Rotate_Y",
		  Tcl_NewDoubleObj(clone->roty),
		  TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

      Tcl_SetVar2Ex(interp, arc, "Rotate_Z",
		  Tcl_NewDoubleObj(clone->rotz),
		  TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

      Tcl_SetVar2Ex(interp, arc, "Scale_X",
		  Tcl_NewDoubleObj(clone->scalx),
		  TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

      Tcl_SetVar2Ex(interp, arc, "Scale_Y",
		  Tcl_NewDoubleObj(clone->scaly),
		  TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

      Tcl_SetVar2Ex(interp, arc, "Scale_Z",
		  Tcl_NewDoubleObj(clone->scalz),
		  TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

      free(quatstr);
    } /* if */

 return AY_OK;
} /* ay_clone_getpropcb */


/* ay_clone_readcb:
 *  read (from scene file) callback function of clone object
 */
int
ay_clone_readcb(FILE *fileptr, ay_object *o)
{
 ay_clone_object *clone = NULL;

  if(!fileptr || !o)
    return AY_ENULL;

  if(!(clone = calloc(1, sizeof(ay_clone_object))))
    return AY_EOMEM;

  fscanf(fileptr, "%d\n", &clone->numclones);

  fscanf(fileptr, "%lg\n", &clone->movx);
  fscanf(fileptr, "%lg\n", &clone->movy);
  fscanf(fileptr, "%lg\n", &clone->movz);

  fscanf(fileptr, "%lg\n", &clone->rotx);
  fscanf(fileptr, "%lg\n", &clone->roty);
  fscanf(fileptr, "%lg\n", &clone->rotz);

  fscanf(fileptr, "%lg\n", &clone->quat[0]);
  fscanf(fileptr, "%lg\n", &clone->quat[1]);
  fscanf(fileptr, "%lg\n", &clone->quat[2]);
  fscanf(fileptr, "%lg\n", &clone->quat[3]);

  fscanf(fileptr, "%lg\n", &clone->scalx);
  fscanf(fileptr, "%lg\n", &clone->scaly);
  fscanf(fileptr, "%lg\n", &clone->scalz);

  if(ay_read_version > 3)
    {
      /* since 1.4 */
      fscanf(fileptr, "%d\n", &clone->rotate);
    }

  if(ay_read_version > 5)
    {
      /* since 1.7 */
      fscanf(fileptr, "%d\n", &clone->mirror);
    }

  o->refine = clone;

 return AY_OK;
} /* ay_clone_readcb */


/* ay_clone_writecb:
 *  write (to scene file) callback function of clone object
 */
int
ay_clone_writecb(FILE *fileptr, ay_object *o)
{
 ay_clone_object *clone = NULL;

  if(!fileptr || !o)
    return AY_ENULL;

  clone = (ay_clone_object *)(o->refine);

  if(!clone)
    return AY_ENULL;

  fprintf(fileptr, "%d\n", clone->numclones);

  fprintf(fileptr, "%g\n", clone->movx);
  fprintf(fileptr, "%g\n", clone->movy);
  fprintf(fileptr, "%g\n", clone->movz);

  fprintf(fileptr, "%g\n", clone->rotx);
  fprintf(fileptr, "%g\n", clone->roty);
  fprintf(fileptr, "%g\n", clone->rotz);

  fprintf(fileptr, "%g\n", clone->quat[0]);
  fprintf(fileptr, "%g\n", clone->quat[1]);
  fprintf(fileptr, "%g\n", clone->quat[2]);
  fprintf(fileptr, "%g\n", clone->quat[3]);

  fprintf(fileptr, "%g\n", clone->scalx);
  fprintf(fileptr, "%g\n", clone->scaly);
  fprintf(fileptr, "%g\n", clone->scalz);

  fprintf(fileptr, "%d\n", clone->rotate);

  fprintf(fileptr, "%d\n", clone->mirror);

 return AY_OK;
} /* ay_clone_writecb */


/* ay_clone_wribcb:
 *  RIB export callback function of clone object
 */
int
ay_clone_wribcb(char *file, ay_object *o)
{
 ay_clone_object *clone = NULL;
 ay_object *c = NULL;
 int old_resinstances;

  if(!o)
    return AY_ENULL;

  clone = (ay_clone_object *)o->refine;

  if(!clone)
    return AY_ENULL;

  old_resinstances = ay_prefs.resolveinstances;
  ay_prefs.resolveinstances = AY_TRUE;

  c = clone->clones;
  while(c)
    {
      ay_wrib_object(file, c);
      c = c->next;
    }

  ay_prefs.resolveinstances = old_resinstances;

 return AY_OK;
} /* ay_clone_wribcb */


/* ay_clone_bbccb:
 *  bounding box calculation callback function of clone object
 */
int
ay_clone_bbccb(ay_object *o, double *bbox, int *flags)
{
 int ay_status = AY_OK;
 ay_clone_object *clone = NULL;
 ay_object *c = NULL;

  if(!o || !bbox || !flags)
    return AY_ENULL;

  clone = (ay_clone_object *)o->refine;

  if(!clone)
    return AY_ENULL;

  c = clone->clones;
  if(!c)
    {
      /* use the bounding boxes of the child(ren) */
      *flags = 2;
      return AY_OK;
    }

  ay_status = ay_bbc_fromlist(c, AY_FALSE, bbox);

 return ay_status;
} /* ay_clone_bbccb */


/* ay_clone_notifycb:
 *  notification callback function of clone object
 */
int
ay_clone_notifycb(ay_object *o)
{
 int ay_status = AY_OK;
 char fname[] = "clone_notifycb";
 ay_clone_object *clone = NULL;
 ay_object *down = NULL, *newo = NULL, **next = NULL, trafo = {0};
 ay_object *tr;
 int i = 0, use_trajectory = AY_FALSE, tr_iscopy = AY_FALSE;
 double euler[3], quat[4], m[16];
 double xaxis[3] = {1.0,0.0,0.0};
 double yaxis[3] = {0.0,1.0,0.0};
 double zaxis[3] = {0.0,0.0,1.0};
 double *p1 = NULL, *p2 = NULL;
 ay_pointedit pe = {0};
 unsigned int j = 0, a = 0;

  if(!o)
    return AY_ENULL;

  clone = (ay_clone_object *)(o->refine);

  if(!clone)
    return AY_ENULL;

  /* clear the old clones */
  while(clone->clones)
    {
      tr = clone->clones;
      clone->clones = tr->next;
      free(tr);
    }
  tr = NULL;

  /* always clear the old read only points */
  if(clone->pnts)
    {
      free(clone->pnts);
      clone->pnts = NULL;
    }

  /* get (first) child object */
  down = o->down;

  if(!down || !down->next)
    goto cleanup;

  if(
     (down->type != AY_IDMATERIAL) &&
     (down->type != AY_IDLIGHT) &&
     (down->type != AY_IDCAMERA) &&
     (down->type != AY_IDVIEW)
    )
    {
      /* arrange clones along curve? */
      if((!clone->mirror) && down->next && down->next->next &&
	 (down->next->type != AY_IDLEVEL))
	{
	  use_trajectory = AY_TRUE;
	}

      if(use_trajectory)
	{ /* Yes, use trajectory curve! */

	  /* get trajectory curve */
	  if(down->next->type != AY_IDNCURVE)
	    {
	      ay_status = ay_provide_object(down->next, AY_IDNCURVE, &tr);
	      if(ay_status || !tr)
		{
		  /* XXXX issue error message? */
		  goto cleanup;
		}
	      else
		{
		  tr_iscopy = AY_TRUE;
		}
	    }
	  else
	    {
	      tr = down->next;
	    }

	  /* create clones */
	  next = &(clone->clones);
	  for(i = 0; i < clone->numclones; i++)
	    {
	      /* create a new instance object */
	      newo = NULL;
	      if(!(newo = calloc(1, sizeof(ay_object))))
		{
		  if(tr_iscopy)
		    {
		      (void)ay_object_deletemulti(tr, AY_FALSE);
		    }
		  ay_status = AY_EOMEM;
		  goto cleanup;
		}
	      ay_object_defaults(newo);
	      newo->type = AY_IDINSTANCE;
	      if(down->type != AY_IDINSTANCE)
		{
		  newo->refine = down;
		}
	      else
		{
		  newo->refine = down->refine;
		}

	      /* link new instance object */
	      *next = newo;
	      next = &(newo->next);
	    } /* for */

	  ay_status = ay_nct_arrange(clone->clones, tr, clone->rotate);

	  /* apply trafo */
	  if((clone->movx != 0.0) || (clone->movy != 0.0) ||
	     (clone->movz != 0.0) || (clone->rotx != 0.0) ||
	     (clone->roty != 0.0) || (clone->rotz != 0.0) ||
	     (clone->scalx != 1.0) || (clone->scaly != 1.0) ||
	     (clone->scalz != 1.0) || (clone->quat[0] != 0.0) ||
	     (clone->quat[1] != 0.0) || (clone->quat[2] != 0.0) ||
	     (clone->quat[3] != 1.0))
	    {
	      ay_trafo_defaults(&trafo);
	      newo = clone->clones;
	      while(newo)
		{
		  trafo.movx += clone->movx;
		  trafo.movy += clone->movy;
		  trafo.movz += clone->movz;

		  trafo.scalx *= clone->scalx;
		  trafo.scaly *= clone->scaly;
		  trafo.scalz *= clone->scalz;

		  ay_quat_add(trafo.quat, clone->quat, trafo.quat);
		  ay_quat_toeuler(trafo.quat, euler);

		  trafo.rotx = AY_R2D(euler[0]);
		  trafo.roty = AY_R2D(euler[1]);
		  trafo.rotz = AY_R2D(euler[2]);

		  ay_trafo_add(&trafo, newo);

		  newo = newo->next;
		} /* while */
	    } /* if */

	  if(tr_iscopy)
	    {
	      (void)ay_object_deletemulti(tr, AY_FALSE);
	    }
	}
      else
	{ /* No, do not use trajectory curve! */
	  /* Mirror objects to clone? */
	  if(!clone->mirror)
	    { /* No! */
	      ay_trafo_defaults(&trafo);
	      next = &(clone->clones);
	      for(i = 0; i < clone->numclones; i++)
		{
		  /* prepare transformation attributes */
		  trafo.movx += clone->movx;
		  trafo.movy += clone->movy;
		  trafo.movz += clone->movz;

		  trafo.scalx *= clone->scalx;
		  trafo.scaly *= clone->scaly;
		  trafo.scalz *= clone->scalz;

		  ay_quat_add(trafo.quat, clone->quat, trafo.quat);
		  ay_quat_toeuler(trafo.quat, euler);

		  trafo.rotx = AY_R2D(euler[0]);
		  trafo.roty = AY_R2D(euler[1]);
		  trafo.rotz = AY_R2D(euler[2]);

		  /* create a new instance object */
		  newo = NULL;
		  if(!(newo = calloc(1, sizeof(ay_object))))
		    {
		      ay_status = AY_EOMEM;
		      goto cleanup;
		    }

		  ay_object_defaults(newo);
		  newo->type = AY_IDINSTANCE;
		  if(down->type != AY_IDINSTANCE)
		    {
		      newo->refine = down;
		    }
		  else
		    {
		      newo->refine = down->refine;
		    }
		  ay_trafo_copy(down, newo);
		  ay_trafo_add(&trafo, newo);

		  /* link new instance */
		  *next = newo;
		  next = &(newo->next);
		} /* for */
	    }
	  else
	    { /* Yes! Mirror objects. */
	      next = &(clone->clones);
	      while(down->next)
		{
		  /* XXXX add instantiability test here! */

		  /* create a new instance object */
		  newo = NULL;
		  if(!(newo = calloc(1, sizeof(ay_object))))
		    {
		      ay_status = AY_EOMEM;
		      goto cleanup;
		    }

		  ay_object_defaults(newo);
		  newo->type = AY_IDINSTANCE;
		  if(down->type != AY_IDINSTANCE)
		    {
		      newo->refine = down;
		    }
		  else
		    {
		      newo->refine = down->refine;
		    }
		  ay_trafo_copy(down, newo);
		  switch(clone->mirror)
		    {
		    case 1:
		      /* mirror at YZ plane */
		      newo->movx *= -1.0;
		      newo->scalx *= -1.0;
		      ay_quat_toeuler(newo->quat, euler);
		      if((fabs(euler[0]) > AY_EPSILON) &&
			 (fabs(euler[1]) < AY_EPSILON) &&
			 (fabs(euler[2]) < AY_EPSILON))
			{
			  ay_quat_axistoquat(zaxis, -2.0*euler[0], quat);
			  ay_quat_add(quat, newo->quat, newo->quat);
			  newo->rotz = -euler[0];
			}
		      else
			{
			  if((fabs(euler[0]) > AY_EPSILON) ||
			     (fabs(euler[1]) > AY_EPSILON) ||
			     (fabs(euler[2]) > AY_EPSILON))
			    {
			      euler[0] = AY_R2D(euler[0]);
			      euler[1] = AY_R2D(euler[1]);
			      euler[2] = AY_R2D(euler[2]);
			      ay_trafo_creatematrix(down, m);
			      ay_trafo_rotatematrix(euler[0],0.0,0.0,1.0,m);
			      ay_trafo_rotatematrix(euler[1],0.0,1.0,0.0,m);
			      ay_trafo_rotatematrix(euler[2],1.0,0.0,0.0,m);
			      ay_trafo_scalematrix(-1.0, 1.0, 1.0, m);
			      ay_trafo_translatematrix(2.0*down->movx,
						       0.0, 0.0, m);
			      ay_trafo_rotatematrix(-euler[2],1.0,0.0,0.0,m);
			      ay_trafo_rotatematrix(-euler[1],0.0,1.0,0.0,m);
			      ay_trafo_rotatematrix(-euler[0],0.0,0.0,1.0,m);
			      ay_trafo_decomposematrix(m, newo);
			    }
			}
		      break;
		    case 2:
		      /* mirror at XZ plane */
		      newo->movy *= -1.0;
		      newo->scaly *= -1.0;
		      ay_quat_toeuler(newo->quat, euler);
		      if((fabs(euler[0]) < AY_EPSILON) &&
			 (fabs(euler[1]) > AY_EPSILON) &&
			 (fabs(euler[2]) < AY_EPSILON))
			{
			  ay_quat_axistoquat(yaxis, -2.0*euler[1], quat);
			  ay_quat_add(quat, newo->quat, newo->quat);
			  newo->rotz = -euler[1];
			}
		      else
			{
			  if((fabs(euler[0]) > AY_EPSILON) ||
			     (fabs(euler[1]) > AY_EPSILON) ||
			     (fabs(euler[2]) > AY_EPSILON))
			    {
			      euler[0] = AY_R2D(euler[0]);
			      euler[1] = AY_R2D(euler[1]);
			      euler[2] = AY_R2D(euler[2]);
			      ay_trafo_creatematrix(down, m);
			      ay_trafo_rotatematrix(euler[0],0.0,0.0,1.0,m);
			      ay_trafo_rotatematrix(euler[1],0.0,1.0,0.0,m);
			      ay_trafo_rotatematrix(euler[2],1.0,0.0,0.0,m);
			      ay_trafo_scalematrix(1.0, -1.0, 1.0, m);
			      ay_trafo_translatematrix(0.0, 2.0*down->movy,
						       0.0, m);
			      ay_trafo_rotatematrix(-euler[2],1.0,0.0,0.0,m);
			      ay_trafo_rotatematrix(-euler[1],0.0,1.0,0.0,m);
			      ay_trafo_rotatematrix(-euler[0],0.0,0.0,1.0,m);
			      ay_trafo_decomposematrix(m, newo);
			    }
			}
		      break;
		    case 3:
		      /* mirror at XY plane */
		      newo->movz *= -1.0;
		      newo->scalz *= -1.0;
		      ay_quat_toeuler(newo->quat, euler);
		      if((fabs(euler[0]) < AY_EPSILON) &&
			 (fabs(euler[1]) < AY_EPSILON) &&
			 (fabs(euler[2]) > AY_EPSILON))
			{
			  ay_quat_axistoquat(xaxis, -2.0*euler[2], quat);
			  ay_quat_add(quat, newo->quat, newo->quat);
			  newo->rotz = -euler[2];
			}
		      else
			{
			  if((fabs(euler[0]) > AY_EPSILON) ||
			     (fabs(euler[1]) > AY_EPSILON) ||
			     (fabs(euler[2]) > AY_EPSILON))
			    {
			      euler[0] = AY_R2D(euler[0]);
			      euler[1] = AY_R2D(euler[1]);
			      euler[2] = AY_R2D(euler[2]);
			      ay_trafo_creatematrix(down, m);
			      ay_trafo_rotatematrix(euler[0],0.0,0.0,1.0,m);
			      ay_trafo_rotatematrix(euler[1],0.0,1.0,0.0,m);
			      ay_trafo_rotatematrix(euler[2],1.0,0.0,0.0,m);
			      ay_trafo_scalematrix(1.0, 1.0, -1.0, m);
			      ay_trafo_translatematrix(0.0, 0.0,
						       2.0*down->movz, m);
			      ay_trafo_rotatematrix(-euler[2],1.0,0.0,0.0,m);
			      ay_trafo_rotatematrix(-euler[1],0.0,1.0,0.0,m);
			      ay_trafo_rotatematrix(-euler[0],0.0,0.0,1.0,m);
			      ay_trafo_decomposematrix(m, newo);
			    }
			}
		      break;
		    default:
		      break;
		    } /* switch */

		  /* link new instance */
		  *next = newo;
		  next = &(newo->next);
		  down = down->next;
		} /* while */
	    } /* if */
	} /* if */

      /* manage read only points */
      if(clone->pntslen)
	{
	  clone->pntsrat = AY_FALSE;
	  down = o->down;
	  tr = clone->clones;

	  if(!clone->mirror)
	    {
	      /* normal or trajectory mode... */
	      if(clone->numclones && clone->clones)
		{
		  /* get all points of first child */
		  ay_status = ay_pact_getpoint(0, down, xaxis, &pe);

		  if(!ay_status && pe.num)
		    {
		      /* get memory for all clone points */
		      if(!(clone->pnts = calloc(clone->numclones * pe.num,
						4 * sizeof(double))))
			{
			  ay_pact_clearpointedit(&pe);
			  ay_status = AY_EOMEM;
			  goto cleanup;
			}
		      clone->pntslen = clone->numclones * pe.num;

		      /* iterate over all clones and transform/copy
			 the child points according to clone trafos
			 into the big clone points vector */
		      for(i = 0; i < clone->numclones; i++)
			{
			  ay_trafo_creatematrix(tr, m);
			  if(pe.type == AY_PTRAT)
			    {
			      clone->pntsrat = AY_TRUE;
			      for(j = 0; j < pe.num; j++)
				{
				  p1 = &(clone->pnts[a]);
				  p2 = pe.coords[j];
				  AY_APTRAN3(p1, p2, m);
				  p1[3] = pe.coords[j][3];
				  a += 4;
				} /* for */
			    }
			  else
			    {
			      for(j = 0; j < pe.num; j++)
				{
				  p1 = &(clone->pnts[a]);
				  p2 = pe.coords[j];
				  AY_APTRAN3(p1, p2, m);
				  p1[3] = 1.0;
				  a += 4;
				} /* for */
			    } /* if */
			  tr = tr->next;
			} /* for */
		    } /* if */
		  ay_pact_clearpointedit(&pe);
		} /* if have clones*/
	    }
	  else
	    {
	      /* mirror mode... */
	      if(clone->clones)
		{
		  clone->pntslen = 0;
		  /* iterate over all children and transform/copy
		     the respective child points according to childs
		     transformation attributes and then also according
		     to the corresponding mirror clone trafos
		     into a big clone points vector (built
		     up dynamically using realloc()) */
		  while(down && down->next && tr)
		    {
		      ay_status = ay_pact_getpoint(0, down, xaxis, &pe);

		      if(!ay_status && pe.num)
			{
			  clone->pntslen += (2 * pe.num);

			  p1 = realloc(clone->pnts,
				       clone->pntslen*4*sizeof(double));

			  if(p1)
			    {
			      clone->pnts = p1;
			      ay_trafo_creatematrix(tr, m);
			      if(pe.type == AY_PTRAT)
				{
				  clone->pntsrat = AY_TRUE;
				  for(j = 0; j < pe.num; j++)
				    {
				      p1 = &(clone->pnts[a]);
				      p2 = pe.coords[j];
				      AY_APTRAN3(p1, p2, m);
				      p1[3] = pe.coords[j][3];
				      a += 4;
				    } /* for */
				}
			      else
				{
				  for(j = 0; j < pe.num; j++)
				    {
				      p1 = &(clone->pnts[a]);
				      p2 = pe.coords[j];
				      AY_APTRAN3(p1, p2, m);
				      p1[3] = 1.0;
				      a += 4;
				    } /* for */
				} /* if */

			      ay_trafo_creatematrix(down, m);
			      if(pe.type == AY_PTRAT)
				{
				  for(j = 0; j < pe.num; j++)
				    {
				      p1 = &(clone->pnts[a]);
				      p2 = pe.coords[j];
				      AY_APTRAN3(p1, p2, m);
				      p1[3] = pe.coords[j][3];
				      a += 4;
				    } /* for */
				}
			      else
				{
				  for(j = 0; j < pe.num; j++)
				    {
				      p1 = &(clone->pnts[a]);
				      p2 = pe.coords[j];
				      AY_APTRAN3(p1, p2, m);
				      p1[3] = 1.0;
				      a += 4;
				    } /* for */
				} /* if */
			    }
			  else
			    {
			      /* realloc() failed! */
			      ay_pact_clearpointedit(&pe);
			      free(clone->pnts);
			      clone->pnts = NULL;
			      break;
			    } /* if */
			} /* if have points*/

		      ay_pact_clearpointedit(&pe);

		      down = down->next;
		      tr = tr->next;
		    } /* while */
		} /* if have clones */
	    } /* if mirror */
	} /* if manage points */
    }
  else
    {
      ay_error(AY_ERROR, fname, "cannot clone this object");
    } /* if */

cleanup:

  /* correct any inconsistent values of pnts and pntslen */
  if(clone->pntslen && !clone->pnts)
    {
      clone->pntslen = 0;
    }

  /* recover selected points */
  if(o->selp)
    {
      ay_clone_getpntcb(3, o, NULL, NULL);
    }

 return ay_status;
} /* ay_clone_notifycb */


/* ay_clone_convertcb:
 *  convert callback function of clone object
 */
int
ay_clone_convertcb(ay_object *o, int in_place)
{
 int ay_status = AY_OK;
 ay_clone_object *clone = NULL;
 ay_object *c = NULL, *new = NULL, *down = NULL, **next = NULL;
 ay_object *newo = NULL;

  if(!o)
    return AY_ENULL;

  /* dow we have any child objects? */
  down = o->down;
  if(!down || !down->next)
    return AY_OK;

  clone = (ay_clone_object *) o->refine;

  if(!clone)
    return AY_ENULL;

  /* first, create new objects */

  if(!(new = calloc(1, sizeof(ay_object))))
    return AY_EOMEM;

  ay_object_defaults(new);
  new->type = AY_IDLEVEL;
  new->parent = AY_TRUE;
  new->inherit_trafos = AY_TRUE;
  ay_trafo_copy(o, new);

  if(!(new->refine = calloc(1, sizeof(ay_level_object))))
    { free(new); return AY_EOMEM; }

  ((ay_level_object *)(new->refine))->type = AY_LTLEVEL;

  next = &(new->down);

  /* copy parameter object(s) */
  ay_status = ay_object_copy(down, next);
  down = down->next;
  if(*next)
    {
      next = &((*next)->next);
    }
  if(clone->mirror)
    {
      while(down->next)
	{
	  ay_status += ay_object_copy(down, next);
	  down = down->next;
	  if(*next)
	    {
	      next = &((*next)->next);
	    }
	} /* while */
    } /* if */

  /* copy clone(s) */
  down = o->down;
  c = clone->clones;
  while(c)
    {
      newo = NULL;
      ay_status += ay_object_copy(down, &newo);
      if(newo)
	{
	  ay_trafo_copy(c, newo);
	  /* link clones */
	  if(clone->mirror == 0)
	    {
	      /* in order */
	      *next = newo;
	      next = &(newo->next);
	    }
	  else
	    {
	      /* in reverse order for mirrored clones */
	      newo->next = *next;
	      *next = newo;
	    } /* if */
	} /* if */

      if(clone->mirror != 0)
	down = down->next;
      c = c->next;
    } /* while */

  while(*next)
    next = &((*next)->next);

  *next = ay_endlevel;

  /* second, link new objects, or replace old objects with them */

  if(new)
    {
      if(!in_place)
	{
	  ay_object_link(new);
	}
      else
	{
	  ay_object_replace(new, o);
	} /* if */
    } /* if */

 return ay_status;
} /* ay_clone_convertcb */


/* ay_clone_providecb:
 *  provide callback function of clone object
 */
int
ay_clone_providecb(ay_object *o, unsigned int type, ay_object **result)
{
 int ay_status = AY_OK;
 ay_clone_object *clone = NULL;
 ay_object *c = NULL, *down = NULL, *newo = NULL, **next = NULL;
 ay_object *po;

  if(!o)
    return AY_ENULL;

  clone = (ay_clone_object *)o->refine;

  if(!clone)
    return AY_ENULL;

  if(!result)
    {
      /* count */

      /* lest we do something bad for forbidden combinations of child
	 objects... */
      if(!clone->clones)
	return AY_ERROR;

      if(clone->mirror != 0)
	{
	  down = o->down;
	  while(down && down->next)
	    {
	      if(down->type == type)
		{
		  return AY_OK;
		}
	      else
		{
		  ay_status = ay_provide_object(down, type, NULL);
		  if(ay_status == AY_OK)
		    {
		      return AY_OK;
		    }
		  /* it is sufficient if we can provide at least
		     one object of wanted type, thus we do not
		     give up here immediately... */
		}
	      down = down->next;
	    } /* while */
	}
      else
	{
	  if(o->down && o->down->next)
	    return ay_provide_object(o->down, type, NULL);
	}

      return AY_ERROR;
    } /* if */

  /* dow we have any child objects? */
  down = o->down;
  if(!down || !down->next)
    return AY_OK;

  if(!clone->clones)
    return AY_OK;

  next = result;

  while(down)
    {
      newo = NULL;
      if(down->type == type)
	{
	  (void) ay_object_copy(down, &newo);
	}
      else
	{
	  (void) ay_provide_object(down, type, &newo);
	} /* if */

      if(newo)
	{
	  *next = newo;
	  /* add trafo to all provided objects */
	  ay_trafo_add(o, newo);
	  while(newo->next)
	    {
	      newo = newo->next;
	      ay_trafo_add(o, newo);
	    }
	  next = &(newo->next);
	} /* if */

      if(clone->mirror != 0)
	down = down->next;
      else
	break;
    } /* while */

  down = o->down;
  c = clone->clones;
  while(c)
    {
      newo = NULL;

      if(down->type == type)
	{
	  (void) ay_object_copy(down, &newo);
	}
      else
	{
	  (void) ay_provide_object(down, type, &newo);
	} /* if */

      if(newo)
	{
	  po = newo;
	  /* add trafo to all provided objects */
	  while(po)
	    {
	      if(down->type == type)
		ay_trafo_copy(c, po);
	      else
		ay_trafo_add(c, po);
	      ay_trafo_add(o, po);
	      po = po->next;
	    }
	  /* link provided objects to result */
	  if(clone->mirror == 0)
	    {
	      /* in natural order for normal clones */
	      *next = newo;
	      while(newo->next)
		{
		  newo = newo->next;
		}
	      next = &(newo->next);
	    }
	  else
	    {
	      /* in reverse order for mirrored clones */
	      po = newo;
	      while(newo->next)
		{
		  newo = newo->next;
		}
	      newo->next = *next;
	      *next = po;
	    } /* if */
	} /* if */

      if(clone->mirror != 0)
	down = down->next;

      c = c->next;
    } /* while */

 return ay_status;
} /* ay_clone_providecb */


/* ay_clone_peekcb:
 *  peek callback function of clone object
 */
int
ay_clone_peekcb(ay_object *o, unsigned int type, ay_object **result,
		double *transform)
{
 int ay_status = AY_OK;
 unsigned int i = 0, count = 0;
 ay_clone_object *clone = NULL;
 ay_object *down = NULL, **results, **subresults;
 double *subtrafos, tm[16];

  if(!o)
    return AY_ENULL;

  clone = (ay_clone_object *) o->refine;

  if(!clone)
    return AY_ENULL;

  if(!result)
    {
      /* count */

      /* lest we do something bad for forbidden combinations of child
	 objects... */
      if(!clone->clones)
	return 0;

      down = o->down;

      if((!clone->mirror) && down->next && down->next->next &&
	 (down->next->type != AY_IDLEVEL))
	{
	  /* trajectory mode */
	  if(down->type == type)
	    {
	      count = 1;
	    }
	  else
	    {
	      count = ay_peek_object(down, type, NULL, NULL);
	    }
	  count *= clone->numclones;
	}
      else
	{
	  down = o->down;
	  while(down && down->next)
	    {
	      if(down->type == type)
		{
		  count++;
		}
	      else
		{
		  count += ay_peek_object(down, type, NULL, NULL);
		}
	      down = down->next;
	    } /* while */

	  if(clone->mirror != 0)
	    {
	      /* mirror mode */
	      count *= 2;
	    }
	  else
	    {
	      /* clone mode */
	      count *= (clone->numclones+1);
	    }
	} /* if */

      return count;
    } /* if */

  /* dow we have any child objects? */
  down = o->down;
  if(!down || !down->next)
    return AY_OK;

  if(!clone->clones)
    return AY_OK;

  results = result;
  if(!((!clone->mirror) && down->next && down->next->next &&
       (down->next->type != AY_IDLEVEL)))
    {
      /* not trajectory mode => also consider our children */
      while(down && down->next && (results[i] != ay_endlevel))
	{
	  if(down->type == type)
	    {
	      results[i] = down;
	      if(AY_ISTRAFO(down))
		{
		  ay_trafo_creatematrix(down, tm);
		  ay_trafo_multmatrix(&(transform[i*16]), tm);
		}
	      i++;
	    }
	  else
	    {
	      if(ay_peek_object(down, type, NULL, NULL))
		{
		  subresults = &(results[i]);
		  subtrafos = &(transform[i*16]);
		  ay_status = ay_peek_object(down, type,
					     &subresults, &subtrafos);
		  /* error handling? */
		  while(results[i] && (results[i] != ay_endlevel))
		    {
		      i++;
		    }
		}
	    } /* if */
	  down = down->next;
	} /* while */
    } /* if */

  down = clone->clones;
  while(down && (results[i] != ay_endlevel))
    {
      if((count = ay_peek_object(down, type, NULL, NULL)))
	{
	  subresults = &(results[i]);
	  subtrafos = &(transform[i*16]);
	  ay_status = ay_peek_object(down, type, &subresults, &subtrafos);
	  /* error handling? */
	  while(results[i] && (results[i] != ay_endlevel))
	    {
	      i++;
	    }
	} /* if */
      down = down->next;
    } /* while */

 return ay_status;
} /* ay_clone_peekcb */


/* ay_clone_drawacb:
 *  draw annotations (in an Ayam view window) callback function of
 *  clone object
 */
int
ay_clone_drawacb(struct Togl *togl, ay_object *o)
{
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);

  if(!o)
    return AY_ENULL;

  if(view->drawhandles == 4)
    {
      (void)ay_npt_drawboundaries(togl, o);
      return AY_OK;
    }

 return AY_OK;
} /* ay_clone_drawacb */


/* ay_clone_init:
 *  initialize the clone object module
 */
int
ay_clone_init(Tcl_Interp *interp)
{
 int ay_status = AY_OK;

  ay_status = ay_otype_registercore(ay_clone_name,
				    ay_clone_createcb,
				    ay_clone_deletecb,
				    ay_clone_copycb,
				    ay_clone_drawcb,
				    ay_clone_drawhcb,
				    ay_clone_shadecb,
				    ay_clone_setpropcb,
				    ay_clone_getpropcb,
				    ay_clone_getpntcb,
				    ay_clone_readcb,
				    ay_clone_writecb,
				    ay_clone_wribcb,
				    ay_clone_bbccb,
				    AY_IDCLONE);

  ay_status += ay_notify_register(ay_clone_notifycb, AY_IDCLONE);

  ay_status += ay_convert_register(ay_clone_convertcb, AY_IDCLONE);

  ay_status += ay_provide_register(ay_clone_providecb, AY_IDCLONE);

  ay_status += ay_peek_register(ay_clone_peekcb, AY_IDCLONE);

  ay_status += ay_draw_registerdacb(ay_clone_drawacb, AY_IDCLONE);

  /* now register the Mirror object as a Clone in disguise */
  ay_status += ay_otype_registercore(ay_mirror_name,
				     ay_clone_createcb,
				     ay_clone_deletecb,
				     ay_clone_copycb,
				     ay_clone_drawcb,
				     ay_clone_drawhcb,
				     ay_clone_shadecb,
				     ay_clone_setpropcb,
				     ay_clone_getpropcb,
				     ay_clone_getpntcb,
				     ay_clone_readcb,
				     ay_clone_writecb,
				     ay_clone_wribcb,
				     ay_clone_bbccb,
				     AY_IDMIRROR);

  ay_status += ay_notify_register(ay_clone_notifycb, AY_IDMIRROR);

  ay_status += ay_convert_register(ay_clone_convertcb, AY_IDMIRROR);

  ay_status += ay_provide_register(ay_clone_providecb, AY_IDMIRROR);

  ay_status += ay_peek_register(ay_clone_peekcb, AY_IDMIRROR);

  ay_status += ay_draw_registerdacb(ay_clone_drawacb, AY_IDMIRROR);

 return ay_status;
} /* ay_clone_init */

