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

/* riinc.c - riinc (RIB-Include) object */

static char *ay_riinc_name = "RiInc";

/* functions: */

/* ay_riinc_createcb:
 *  create callback function of riinc object
 */
int
ay_riinc_createcb(int argc, char *argv[], ay_object *o)
{
 ay_riinc_object *riinc = NULL;
 char fname[] = "crtriinc";

  if(!o)
    return AY_ENULL;

  if(!(riinc = calloc(1, sizeof(ay_riinc_object))))
    {
      ay_error(AY_EOMEM, fname, NULL);
      return AY_ERROR;
    }

  riinc->width  = 1.0;
  riinc->length = 1.0;
  riinc->height = 1.0;

  o->refine = riinc;
  o->parent = AY_TRUE;
  o->hide_children = AY_TRUE;

 return AY_OK;
} /* ay_riinc_createcb */


/* ay_riinc_deletecb:
 *  delete callback function of riinc object
 */
int
ay_riinc_deletecb(void *c)
{
 ay_riinc_object *riinc = NULL;

  if(!c)
    return AY_ENULL;

  riinc = (ay_riinc_object *)(c);

  if(riinc->file)
    free(riinc->file);

  free(riinc);

 return AY_OK;
} /* ay_riinc_deletecb */


/* ay_riinc_copycb:
 *  copy callback function of riinc object
 */
int
ay_riinc_copycb(void *src, void **dst)
{
 ay_riinc_object *srcr = NULL, *riinc = NULL;

  if(!src || !dst)
    return AY_ENULL;

  srcr = (ay_riinc_object*) src;

  if(!(riinc = malloc(sizeof(ay_riinc_object))))
    return AY_EOMEM;

  memcpy(riinc, src, sizeof(ay_riinc_object));
  riinc->file = NULL;

  /* copy file name */
  if(srcr->file)
    {
      if(!(riinc->file = malloc((strlen(srcr->file)+1) * sizeof(char))))
	{
	  free(riinc);
	  return AY_EOMEM;
	}

      strcpy(riinc->file, srcr->file);
    }

  *dst = (void *)riinc;

 return AY_OK;
} /* ay_riinc_copycb */


/* ay_riinc_drawcb:
 *  draw (display in an Ayam view window) callback function of riinc object
 */
int
ay_riinc_drawcb(struct Togl *togl, ay_object *o)
{
 ay_object *d;
 ay_riinc_object *riinc = NULL;
 GLdouble wh, hh, lh;

  if(!o)
    return AY_ENULL;

  riinc = (ay_riinc_object *)o->refine;

  if(!riinc)
    return AY_ENULL;

  glEnable(GL_LINE_STIPPLE);
  glLineStipple((GLint)3, (GLushort)0x5555);

  if(o->down && o->down->next)
    {
      d = o->down;
      while(d->next)
	{
	  ay_draw_object(togl, d, AY_FALSE);
	  d = d->next;
	}
    }
  else
    {
      wh = (GLdouble)(riinc->width  * 0.5);
      lh = (GLdouble)(riinc->length * 0.5);
      hh = (GLdouble)(riinc->height * 0.5);

      /* draw */
      glBegin(GL_LINE_STRIP);
       glVertex3d( wh, hh, lh);
       glVertex3d( wh,-hh, lh);
       glVertex3d(-wh,-hh, lh);
       glVertex3d(-wh, hh, lh);
       glVertex3d( wh, hh, lh);
       glVertex3d( wh, hh,-lh);
       glVertex3d( wh,-hh,-lh);
       glVertex3d(-wh,-hh,-lh);
       glVertex3d(-wh, hh,-lh);
       glVertex3d( wh, hh,-lh);
      glEnd();

      glBegin(GL_LINES);
       glVertex3d( wh,-hh, lh);
       glVertex3d( wh,-hh,-lh);
       glVertex3d(-wh,-hh, lh);
       glVertex3d(-wh,-hh,-lh);
       glVertex3d(-wh, hh, lh);
       glVertex3d(-wh, hh,-lh);
      glEnd();
    }
  glDisable(GL_LINE_STIPPLE);

 return AY_OK;
} /* ay_riinc_drawcb */


/* ay_riinc_shadecb:
 *  shade (display in an Ayam view window) callback function of riinc object
 */
int
ay_riinc_shadecb(struct Togl *togl, ay_object *o)
{
 ay_riinc_object *riinc = NULL;

  if(!o)
    return AY_ENULL;

  riinc = (ay_riinc_object *)o->refine;

  if(!riinc)
    return AY_ENULL;

 return AY_OK;
} /* ay_riinc_shadecb */


/* ay_riinc_setpropcb:
 *  set property (from Tcl to C context) callback function of riinc object
 */
int
ay_riinc_setpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
 /*int ay_status = AY_OK;*/
 char fname[] = "riinc_setprop";
 char *arr = "RiIncAttrData";
 Tcl_Obj *to = NULL;
 ay_riinc_object *riinc = NULL;
 char *result = NULL;
 int reslen = 0;

  if(!interp || !o)
    return AY_ENULL;

  riinc = (ay_riinc_object *)o->refine;

  if(!riinc)
    return AY_ENULL;

  if(riinc->file)
    {
      free(riinc->file);
      riinc->file = NULL;
    }

  /* get filename */
  if((to = Tcl_GetVar2Ex(interp, arr, "File", TCL_LEAVE_ERR_MSG |
			 TCL_GLOBAL_ONLY)))
    {
      result = Tcl_GetStringFromObj(to, &reslen);
      if(!(riinc->file = calloc(reslen+1, sizeof(char))))
	{
	  ay_error(AY_EOMEM, fname, NULL);
	  return AY_ERROR;
	}
      
      strcpy(riinc->file, result);
    }

  if((to = Tcl_GetVar2Ex(interp, arr, "Width",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetDoubleFromObj(interp, to, &(riinc->width));

  if((to = Tcl_GetVar2Ex(interp, arr, "Length",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetDoubleFromObj(interp, to, &(riinc->length));

  if((to = Tcl_GetVar2Ex(interp, arr, "Height",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetDoubleFromObj(interp, to, &(riinc->height));

  o->modified = AY_TRUE;
  ay_notify_parent();

 return AY_OK;
} /* ay_riinc_setpropcb */


/* ay_riinc_getpropcb:
 *  get property (from C to Tcl context) callback function of riinc object
 */
int
ay_riinc_getpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
 char *arr = "RiIncAttrData";
 ay_riinc_object *riinc = NULL;

  if(!interp || !o)
    return AY_ENULL;

  riinc = (ay_riinc_object *)(o->refine);

  if(!riinc)
    return AY_ENULL;

  Tcl_SetVar2(interp, arr, "File", "", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
  if(riinc->file)
    Tcl_SetVar2(interp, arr, "File", riinc->file, TCL_LEAVE_ERR_MSG |
		TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Width",
		Tcl_NewDoubleObj(riinc->width),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Length",
		Tcl_NewDoubleObj(riinc->length),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Height",
		Tcl_NewDoubleObj(riinc->height),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

 return AY_OK;
} /* ay_riinc_getpropcb */


/* ay_riinc_readcb:
 *  read (from scene file) callback function of riinc object
 */
int
ay_riinc_readcb(FILE *fileptr, ay_object *o)
{
 int ay_status = AY_OK;
 ay_riinc_object *riinc = NULL;

  if(!fileptr || !o)
    return AY_ENULL;

  if(!(riinc = calloc(1, sizeof(ay_riinc_object))))
    return AY_EOMEM;

  fscanf(fileptr, "%lg\n", &riinc->width);
  fscanf(fileptr, "%lg\n", &riinc->length);
  fscanf(fileptr, "%lg", &riinc->height);
  (void)fgetc(fileptr);

  ay_status = ay_read_string(fileptr, &(riinc->file));
  if(ay_status)
    {
      free(riinc);
      return ay_status;
    }
  o->refine = riinc;

 return AY_OK;
} /* ay_riinc_readcb */


/* ay_riinc_writecb:
 *  write (to scene file) callback function of riinc object
 */
int
ay_riinc_writecb(FILE *fileptr, ay_object *o)
{
 ay_riinc_object *riinc = NULL;

  if(!fileptr || !o)
    return AY_ENULL;

  riinc = (ay_riinc_object *)(o->refine);

  if(!riinc)
    return AY_ENULL;

  fprintf(fileptr, "%g\n", riinc->width);
  fprintf(fileptr, "%g\n", riinc->length);
  fprintf(fileptr, "%g\n", riinc->height);
  fprintf(fileptr, "%s\n", riinc->file);

 return AY_OK;
} /* ay_riinc_writecb */


/* ay_riinc_wribcb:
 *  RIB export callback function of riinc object
 */
int
ay_riinc_wribcb(char *file, ay_object *o)
{
 ay_riinc_object *riinc = NULL;

  if(!o)
    return AY_ENULL;

  riinc = (ay_riinc_object *)(o->refine);

  if(!riinc)
    return AY_ENULL;

  if(riinc->file)
    RiReadArchive(riinc->file,(RtVoid*)RI_NULL,RI_NULL);

 return AY_OK;
} /* ay_riinc_wribcb */


/* ay_riinc_bbccb:
 *  bounding box calculation callback function of riinc object
 */
int
ay_riinc_bbccb(ay_object *o, double *bbox, int *flags)
{
 double wh = 0.0, lh = 0.0, hh = 0.0;
 ay_riinc_object *riinc = NULL;

  if(!o || !bbox || !flags)
    return AY_ENULL;

  riinc = (ay_riinc_object *)o->refine;

  if(!riinc)
    return AY_ENULL;

  wh = riinc->width  * 0.5;
  lh = riinc->length * 0.5;
  hh = riinc->height * 0.5;

  /* P1 */
  bbox[0] = -wh; bbox[1] = hh; bbox[2] = lh;
  /* P2 */
  bbox[3] = -wh; bbox[4] = hh; bbox[5] = -lh;
  /* P3 */
  bbox[6] = wh; bbox[7] = hh; bbox[8] = -lh;
  /* P4 */
  bbox[9] = wh; bbox[10] = hh; bbox[11] = lh;

  /* P5 */
  bbox[12] = -wh; bbox[13] = -hh; bbox[14] = lh;
  /* P6 */
  bbox[15] = -wh; bbox[16] = -hh; bbox[17] = -lh;
  /* P7 */
  bbox[18] = wh; bbox[19] = -hh; bbox[20] = -lh;
  /* P8 */
  bbox[21] = wh; bbox[22] = -hh; bbox[23] = lh;

 return AY_OK;
} /* ay_riinc_bbccb */


/* ay_riinc_init:
 *  initialize the riinc object module
 */
int
ay_riinc_init(Tcl_Interp *interp)
{
 int ay_status = AY_OK;

  ay_status = ay_otype_registercore(ay_riinc_name,
				    ay_riinc_createcb,
				    ay_riinc_deletecb,
				    ay_riinc_copycb,
				    ay_riinc_drawcb,
				    NULL, /* no points to edit */
				    ay_riinc_shadecb,
				    ay_riinc_setpropcb,
				    ay_riinc_getpropcb,
				    NULL, /* No Picking! */
				    ay_riinc_readcb,
				    ay_riinc_writecb,
				    ay_riinc_wribcb,
				    ay_riinc_bbccb,
				    AY_IDRIINC);

 return ay_status;
} /* ay_riinc_init */

