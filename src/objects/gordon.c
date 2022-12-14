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

/* gordon.c - gordon surface object */

static char *ay_gordon_name = "Gordon";

static char *ay_dskin_name = "DSkin";


/* prototypes of functions local to this module: */

int ay_gordon_getpntcb(int mode, ay_object *o, double *p, ay_pointedit *pe);


/* functions: */

/* ay_gordon_createcb:
 *  create callback function of gordon object
 */
int
ay_gordon_createcb(int argc, char *argv[], ay_object *o)
{
 char fname[] = "crtgordon";
 ay_gordon_object *new = NULL;

  if(!o)
    return AY_ENULL;

  if(!(new = calloc(1, sizeof(ay_gordon_object))))
    {
      ay_error(AY_EOMEM, fname, NULL);
      return AY_ERROR;
    }

  if((argc > 1) && !strcmp(argv[1], ay_dskin_name))
    {
      o->type = AY_IDDSKIN;
    }

  new->watchcorners = AY_FALSE;
  new->uorder = 4;
  new->vorder = 4;

  o->parent = AY_TRUE;
  o->refine = new;

 return AY_OK;
} /* ay_gordon_createcb */


/* ay_gordon_deletecb:
 *  delete callback function of gordon object
 */
int
ay_gordon_deletecb(void *c)
{
 ay_gordon_object *gordon = NULL;

  if(!c)
    return AY_ENULL;

  gordon = (ay_gordon_object *)(c);

  if(gordon->npatch)
    (void)ay_object_delete(gordon->npatch);

  if(gordon->pomesh)
    (void)ay_object_delete(gordon->pomesh);

  if(gordon->caps_and_bevels)
    (void)ay_object_deletemulti(gordon->caps_and_bevels, AY_FALSE);

  free(gordon);

 return AY_OK;
} /* ay_gordon_deletecb */


/* ay_gordon_copycb:
 *  copy callback function of gordon object
 */
int
ay_gordon_copycb(void *src, void **dst)
{
 ay_gordon_object *gordon = NULL;

  if(!src || !dst)
    return AY_ENULL;

  if(!(gordon = malloc(sizeof(ay_gordon_object))))
    return AY_EOMEM;

  memcpy(gordon, src, sizeof(ay_gordon_object));

  gordon->npatch = NULL;
  gordon->caps_and_bevels = NULL;
  gordon->pomesh = NULL;
  gordon->edgethresh = 0.0;

  *dst = (void *)gordon;

 return AY_OK;
} /* ay_gordon_copycb */


/** ay_gordon_managetritess:
 * Compare the GLU sampling tolerance from the preferences with
 * the tolerance used for the tesselation of the triangular Gordon
 * surface and re-tesselate if necessary.
 *
 * \param[in,out] gordon  triangular Gordon surface
 */
void
ay_gordon_managetritess(ay_gordon_object *gordon)
{
 int ay_status = AY_OK;
 double tolerance, edgethresh;

  if(gordon->npatch)
    {
      tolerance = gordon->glu_sampling_tolerance;
      if(tolerance == 0.0)
	{
	  tolerance = ay_prefs.glu_sampling_tolerance;
	}
      edgethresh = ay_rtess_getedgethresh(tolerance);

      if(edgethresh != gordon->edgethresh)
	{
	  if(gordon->pomesh)
	    {
	      if(gordon->pomesh)
		(void)ay_object_delete(gordon->pomesh);
	      gordon->pomesh = NULL;
	    }
	  ay_status = ay_rtess_tessnp(gordon->npatch->refine, AY_TRUE,
				      tolerance, &(gordon->pomesh));
	  if(!ay_status && gordon->pomesh)
	    gordon->edgethresh = edgethresh;
	}
    } /* if */

 return;
} /* ay_gordon_managetritess */


/* ay_gordon_drawcb:
 *  draw (display in an Ayam view window) callback function of gordon object
 */
int
ay_gordon_drawcb(struct Togl *togl, ay_object *o)
{
 int display_mode = ay_prefs.np_display_mode;
 ay_gordon_object *gordon = NULL;
 ay_object *b;
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);

  if(!o)
    return AY_ENULL;

  gordon = (ay_gordon_object *)o->refine;

  if(!gordon)
    return AY_ENULL;

  if(gordon->npatch)
    {
      if((gordon->display_mode != 0) && !view->action_state)
	{
	  display_mode = gordon->display_mode-1;
	}

      if(gordon->is_triangular)
	ay_gordon_managetritess(gordon);

      if(gordon->is_triangular && gordon->pomesh &&
	 (display_mode == /* OutlinePolygon (GLU) */1))
	{
	  ay_draw_object(togl, gordon->pomesh, AY_TRUE);
	}
      else
	{
	  ay_draw_object(togl, gordon->npatch, AY_TRUE);
	}
    }

  if(gordon->caps_and_bevels)
    {
      b = gordon->caps_and_bevels;
      while(b)
	{
	  ay_draw_object(togl, b, AY_TRUE);
	  b = b->next;
	}
    }

 return AY_OK;
} /* ay_gordon_drawcb */


/* ay_gordon_shadecb:
 *  shade (display in an Ayam view window) callback function of gordon object
 */
int
ay_gordon_shadecb(struct Togl *togl, ay_object *o)
{
 ay_gordon_object *gordon = NULL;
 ay_object *b;

  if(!o)
    return AY_ENULL;

  gordon = (ay_gordon_object *)o->refine;

  if(!gordon)
    return AY_ENULL;

  if(gordon->npatch)
    {
      if(gordon->is_triangular)
	{
	  ay_gordon_managetritess(gordon);

	  if(gordon->pomesh)
	    ay_shade_object(togl, gordon->pomesh, AY_FALSE);
	}
      else
	{
	  ay_shade_object(togl, gordon->npatch, AY_FALSE);
	}
    }

  if(gordon->caps_and_bevels)
    {
      b = gordon->caps_and_bevels;
      while(b)
	{
	  ay_shade_object(togl, b, AY_FALSE);
	  b = b->next;
	}
    }

 return AY_OK;
} /* ay_gordon_shadecb */


/* ay_gordon_drawhcb:
 *  draw handles (in an Ayam view window) callback function of gordon object
 */
int
ay_gordon_drawhcb(struct Togl *togl, ay_object *o)
{
 ay_gordon_object *gordon;

  if(!o)
    return AY_ENULL;

  gordon = (ay_gordon_object *) o->refine;

  if(!gordon)
    return AY_ENULL;

  if(!gordon->npatch)
    return AY_OK;

  ay_npt_drawhandles(togl, (ay_nurbpatch_object *)gordon->npatch->refine);

 return AY_OK;
} /* ay_gordon_drawhcb */


/* ay_gordon_getpntcb:
 *  get point (editing and selection) callback function of gordon object
 */
int
ay_gordon_getpntcb(int mode, ay_object *o, double *p, ay_pointedit *pe)
{
 ay_nurbpatch_object *patch = NULL;
 ay_gordon_object *gordon = NULL;

  if(!o)
    return AY_ENULL;

  gordon = (ay_gordon_object *)o->refine;

  if(!gordon)
    return AY_ENULL;

  if(gordon->npatch)
    {
      patch = (ay_nurbpatch_object *)gordon->npatch->refine;
      return ay_selp_getpnts(mode, o, p, pe, 1, patch->width*patch->height, 4,
			     ay_prefs.rationalpoints, patch->controlv);
    }

 return AY_ERROR;
} /* ay_gordon_getpntcb */


/* ay_gordon_setpropcb:
 *  set property (from Tcl to C context) callback function of gordon object
 */
int
ay_gordon_setpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
 /*int ay_status = AY_OK;*/
 char *arr = "GordonAttrData";
 /* char fname[] = "gordon_setpropcb";*/
 Tcl_Obj *to = NULL;
 ay_gordon_object *gordon = NULL;
 int new_watchcorners = 0;
 char uarg0[] = "rewind", uarg1[] = "save", uarg2[] = "setProp", uarg3[] = "1";
 char *uargv[4] = {0};

  if(!interp || !o)
    return AY_ENULL;

  gordon = (ay_gordon_object *)o->refine;

  if(!gordon)
    return AY_ENULL;

  if(o->type == AY_IDDSKIN)
    {
      arr = "DSkinAttrData";
    }

  if((to = Tcl_GetVar2Ex(interp, arr, "WatchCorners",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(new_watchcorners));

  if((to = Tcl_GetVar2Ex(interp, arr, "Order_U",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(gordon->uorder));

  if((to = Tcl_GetVar2Ex(interp, arr, "Order_V",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(gordon->vorder));

  if((to = Tcl_GetVar2Ex(interp, arr, "Tolerance",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetDoubleFromObj(interp, to, &(gordon->glu_sampling_tolerance));

  if((to = Tcl_GetVar2Ex(interp, arr, "DisplayMode",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetIntFromObj(interp, to, &(gordon->display_mode));
  
  /* replace topmost undo state with a new one that includes our children? */
  if((argc > 1) &&
     (new_watchcorners != gordon->watchcorners) && new_watchcorners)
    {
      /* undo rewind */
      uargv[1] = uarg0;
      ay_undo_undotcmd(NULL, ay_interp, 2, uargv);
      /* undo save setProp 1 */
      uargv[1] = uarg1;
      uargv[2] = uarg2;
      uargv[3] = uarg3;
      ay_undo_undotcmd(NULL, ay_interp, 4, uargv);
    }
  gordon->watchcorners = new_watchcorners;

  (void)ay_notify_object(o);

  o->modified = AY_TRUE;
  (void)ay_notify_parent();

 return AY_OK;
} /* ay_gordon_setpropcb */


/* ay_gordon_getpropcb:
 *  get property (from C to Tcl context) callback function of gordon object
 */
int
ay_gordon_getpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
 char *arr = "GordonAttrData";
 ay_gordon_object *gordon = NULL;

  if(!interp || !o)
    return AY_ENULL;

  gordon = (ay_gordon_object *)(o->refine);

  if(!gordon)
    return AY_ENULL;

  if(o->type == AY_IDDSKIN)
    {
      arr = "DSkinAttrData";
    }

  Tcl_SetVar2Ex(interp, arr, "WatchCorners",
		Tcl_NewIntObj(gordon->watchcorners),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Order_U",
		Tcl_NewIntObj(gordon->uorder),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Order_V",
		Tcl_NewIntObj(gordon->vorder),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Tolerance",
		Tcl_NewDoubleObj(gordon->glu_sampling_tolerance),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "DisplayMode",
		Tcl_NewIntObj(gordon->display_mode),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  ay_prop_getnpinfo(interp, arr, gordon->npatch);

 return AY_OK;
} /* ay_gordon_getpropcb */


/* ay_gordon_readcb:
 *  read (from scene file) callback function of gordon object
 */
int
ay_gordon_readcb(FILE *fileptr, ay_object *o)
{
 ay_gordon_object *gordon = NULL;

  if(!fileptr || !o)
    return AY_ENULL;

  if(!(gordon = calloc(1, sizeof(ay_gordon_object))))
    return AY_EOMEM;

  fscanf(fileptr, "%d\n", &gordon->watchcorners);
  fscanf(fileptr, "%d\n", &gordon->uorder);
  fscanf(fileptr, "%d\n", &gordon->vorder);
  fscanf(fileptr, "%d\n", &gordon->display_mode);
  fscanf(fileptr, "%lg\n", &gordon->glu_sampling_tolerance);

  o->refine = gordon;

 return AY_OK;
} /* ay_gordon_readcb */


/* ay_gordon_writecb:
 *  write (to scene file) callback function of gordon object
 */
int
ay_gordon_writecb(FILE *fileptr, ay_object *o)
{
 ay_gordon_object *gordon = NULL;

  if(!fileptr || !o)
    return AY_ENULL;

  gordon = (ay_gordon_object *)(o->refine);

  if(!gordon)
    return AY_ENULL;

  fprintf(fileptr, "%d\n", gordon->watchcorners);
  fprintf(fileptr, "%d\n", gordon->uorder);
  fprintf(fileptr, "%d\n", gordon->vorder);
  fprintf(fileptr, "%d\n", gordon->display_mode);
  fprintf(fileptr, "%g\n", gordon->glu_sampling_tolerance);

 return AY_OK;
} /* ay_gordon_writecb */


/* ay_gordon_wribcb:
 *  RIB export callback function of gordon object
 */
int
ay_gordon_wribcb(char *file, ay_object *o)
{
 ay_gordon_object *gordon = NULL;
 ay_object *b;
 unsigned int ci = 1;

  if(!o)
   return AY_ENULL;

  gordon = (ay_gordon_object*)o->refine;

  if(!gordon)
    return AY_ENULL;

  if(gordon->npatch)
    ay_wrib_toolobject(file, gordon->npatch, o);

  if(gordon->caps_and_bevels)
    {
      b = gordon->caps_and_bevels;
      while(b)
	{
	  ay_wrib_caporbevel(file, o, b, ci);
	  ci++;
	  b = b->next;
	}
    }

 return AY_OK;
} /* ay_gordon_wribcb */


/* ay_gordon_bbccb:
 *  bounding box calculation callback function of gordon object
 */
int
ay_gordon_bbccb(ay_object *o, double *bbox, int *flags)
{
 ay_gordon_object *gordon = NULL;

  if(!o || !bbox || !flags)
    return AY_ENULL;

  gordon = (ay_gordon_object *)o->refine;

  if(!gordon)
    return AY_ENULL;

  if(gordon->npatch)
    {
      ay_bbc_get(gordon->npatch, bbox);
      *flags = 1;
    }
  else
    {
      /* invalid/nonexisting bbox */
      *flags = 2;
    }

 return AY_OK;
} /* ay_gordon_bbccb */


/* ay_gordon_notifycb:
 *  notification callback function of gordon object
 */
int
ay_gordon_notifycb(ay_object *o)
{
 int ay_status = AY_OK;
 ay_nurbcurve_object *curve = NULL;
 ay_nurbpatch_object *patch = NULL;
 ay_gordon_object *gordon = NULL;
 ay_object *down = NULL, *c = NULL, *p = NULL, *last = NULL;
 ay_object *hcurves = NULL, *vcurves = NULL, *inpatch = NULL;
 ay_object *tcurves[3] = {0};
 ay_object *npatch = NULL, **nextcb;
 ay_bparam bparams = {0};
 ay_cparam cparams = {0};
 int getvcurves = AY_FALSE, getinpatch = AY_FALSE, hcount = 0, vcount = 0;
 int mode = 0, a, i;
 double tolerance, m[16] = {0};

  if(!o)
    return AY_ENULL;

  gordon = (ay_gordon_object *)(o->refine);

  if(!gordon)
    return AY_ENULL;

  mode = gordon->display_mode;
  tolerance = gordon->glu_sampling_tolerance;

  /* remove old objects */
  if(gordon->npatch)
    (void)ay_object_delete(gordon->npatch);
  gordon->npatch = NULL;

  if(gordon->caps_and_bevels)
    {
      (void)ay_object_deletemulti(gordon->caps_and_bevels, AY_FALSE);
      gordon->caps_and_bevels = NULL;
    }

  nextcb = &(gordon->caps_and_bevels);

  /* get parameter curves */
  if(!o->down || !o->down->next)
    goto cleanup;

  down = o->down;

  i = 0;
  while(down->next)
    {
      if((down->type == AY_IDNCURVE) ||
	 (ay_provide_object(down, AY_IDNCURVE, NULL) == AY_OK))
	{
	  i++;
	}
      else
	{
	  i = 0;
	  break;
	}
      down = down->next;
    }

  if(i == 3)
    {
      /* have three arguments => create triangular Gordon surface */

      if(gordon->watchcorners)
	ay_npt_gordonwct(o);

      i = 0;
      down = o->down;
      while(down->next)
	{
	  if(down->type == AY_IDNCURVE)
	    {
	      tcurves[i] = down;
	      i++;
	    }
	  else
	    {
	      if(ay_provide_object(down, AY_IDNCURVE, NULL) == AY_OK)
		{
		  ay_provide_object(down, AY_IDNCURVE, &(tcurves[i]));
		  i++;
		}
	    }
	  if(i == 3)
	    break;
	  down = down->next;
	}

      if((ay_status = ay_npt_createnpatchobject(&npatch)))
	{
	  goto cleanup;
	}

      if((ay_status = ay_npt_tgordon(tcurves, gordon->uorder, gordon->vorder,
		            (ay_nurbpatch_object **)(void*)&(npatch->refine))))
	{
	  goto cleanup;
	}
      gordon->is_triangular = AY_TRUE;
      /* trigger re-tesselation */
      gordon->edgethresh = 0.0;
      goto save;
    } /* if have three arguments */

  if(gordon->watchcorners)
    ay_npt_gordonwc(o);

  down = o->down;

  while(down->next)
    {
      c = NULL;
      if(down->type == AY_IDLEVEL)
	{
	  if(!getvcurves)
	    {
	      getvcurves = AY_TRUE;
	    }
	  else
	    {
	      getinpatch = AY_TRUE;
	    }
	  last = NULL;
	  down = down->next;
	  continue;
	}
      if(!getinpatch)
	{
	  if(down->type == AY_IDNCURVE)
	    {
	      (void)ay_object_copy(down, &c);
	    }
	  else
	    {
	      (void)ay_provide_object(down, AY_IDNCURVE, &c);
	    } /* if */

	  while(c)
	    {
	      /* immediately apply transformation attributes to
		 control points */
	      if(AY_ISTRAFO(c))
		{
		  ay_trafo_creatematrix(c, m);
		  curve = (ay_nurbcurve_object *)c->refine;
		  a = 0;
		  for(i = 0; i < curve->length; i++)
		    {
		      ay_trafo_apply3(&(curve->controlv[a]), m);
		      a += 4;
		    }
		} /* if */

	      /* link curve to list */
	      if(getvcurves)
		{
		  /* get v curves */
		  vcount++;
		  if(last)
		    last->next = c;
		  else
		    vcurves = c;
		}
	      else
		{
		  /* get h curves */
		  hcount++;
		  if(last)
		    last->next = c;
		  else
		    hcurves = c;
		}

	      last = c;
	      c = c->next;
	    } /* while */
	}
      else
	{
	  if(!inpatch)
	    {
	      if(down->type == AY_IDNPATCH)
		{
		  (void)ay_object_copy(down, &inpatch);
		}
	      else
		{
		  (void)ay_provide_object(down, AY_IDNPATCH, &inpatch);
		} /* if */

	      if(inpatch)
		{
		  p = inpatch;
		  if(AY_ISTRAFO(p))
		    {
		      ay_trafo_creatematrix(p, m);
		      patch = (ay_nurbpatch_object *)p->refine;
		      a = 0;
		      for(i = 0; i < patch->width*patch->height; i++)
			{
			  ay_trafo_apply3(&(patch->controlv[a]), m);
			  a += 4;
			}
		    } /* if */
		} /* if */
	    } /* if */
	} /* if get inpatch */

      down = down->next;
    } /* while */

  /* enough curves to process? */
  if((hcount < 2) || (vcount < 2))
    {
      ay_status = AY_OK;
      goto cleanup;
    }

  /* create Gordon surface */
  if((ay_status = ay_npt_createnpatchobject(&npatch)))
    {
      goto cleanup;
    }

  if(o->type == AY_IDDSKIN)
    ay_status = ay_npt_dualskin(hcurves, vcurves, inpatch,
				gordon->uorder, gordon->vorder,
			    (ay_nurbpatch_object **)(void*)&(npatch->refine));
  else
    ay_status = ay_npt_gordon(hcurves, vcurves, inpatch,
			      gordon->uorder, gordon->vorder,
			    (ay_nurbpatch_object **)(void*)&(npatch->refine));

  if(ay_status)
    {
      free(npatch);
      goto cleanup;
    }

save:
  /* save result */
  gordon->npatch = npatch;

  /* copy sampling tolerance/mode over to new object */
  ((ay_nurbpatch_object *)npatch->refine)->glu_sampling_tolerance =
    tolerance;
  ((ay_nurbpatch_object *)npatch->refine)->display_mode =
    mode;

  /* get bevel and cap parameters */
  if(o->tags)
    {
      ay_bevelt_parsetags(o->tags, &bparams);
      ay_capt_parsetags(o->tags, &cparams);
    }

  /* create/add caps */
  if(cparams.has_caps)
    {
      ay_status = ay_capt_addcaps(&cparams, &bparams, npatch, nextcb);
      if(ay_status)
	goto cleanup;

      while(*nextcb)
	nextcb = &((*nextcb)->next);
    }

  /* create/add bevels */
  if(bparams.has_bevels)
    {
      ay_status = ay_bevelt_addbevels(&bparams, &cparams, npatch, nextcb);
      if(ay_status)
	goto cleanup;
    }

cleanup:
  /* remove temporary curves and intersection patch */
  if(hcurves)
    (void)ay_object_deletemulti(hcurves, AY_FALSE);

  if(vcurves)
    (void)ay_object_deletemulti(vcurves, AY_FALSE);

  if(inpatch)
    (void)ay_object_deletemulti(inpatch, AY_FALSE);

  /* recover selected points */
  if(o->selp)
    {
      if(gordon->npatch)
	ay_gordon_getpntcb(3, o, NULL, NULL);
      else
	ay_selp_clear(o);
    }

  if(tcurves[0])
    {
      i = 0;
      down = o->down;
      while(down->next)
	{
	  if(down->type != AY_IDNCURVE)
	    ay_object_deletemulti(tcurves[i], AY_FALSE);
	  i++;
	  if(i == 3)
	    break;
	  down = down->next;
	}
    }

 return ay_status;
} /* ay_gordon_notifycb */


/* ay_gordon_providecb:
 *  provide callback function of gordon object
 */
int
ay_gordon_providecb(ay_object *o, unsigned int type, ay_object **result)
{
 ay_gordon_object *g = NULL;

  if(!o)
    return AY_ENULL;

  g = (ay_gordon_object *) o->refine;

  if(!g)
    return AY_ENULL;

  if((type == AY_IDPOMESH) && g->is_triangular && g->pomesh)
    {
      if(result)
	return ay_object_copy(g->pomesh, result);
      else
	return AY_OK;
    }

 return ay_provide_nptoolobj(o, type, g->npatch, g->caps_and_bevels, result);
} /* ay_gordon_providecb */


/* ay_gordon_convertcb:
 *  convert callback function of gordon object
 */
int
ay_gordon_convertcb(ay_object *o, int in_place)
{
 ay_gordon_object *g = NULL;

  if(!o)
    return AY_ENULL;

  g = (ay_gordon_object *) o->refine;

  if(!g)
    return AY_ENULL;

 return ay_convert_nptoolobj(o, g->npatch, g->caps_and_bevels, in_place);
} /* ay_gordon_convertcb */


/* ay_gordon_init:
 *  initialize the gordon object module
 */
int
ay_gordon_init(Tcl_Interp *interp)
{
 int ay_status = AY_OK;

  ay_status = ay_otype_registercore(ay_gordon_name,
				    ay_gordon_createcb,
				    ay_gordon_deletecb,
				    ay_gordon_copycb,
				    ay_gordon_drawcb,
				    ay_gordon_drawhcb,
				    ay_gordon_shadecb,
				    ay_gordon_setpropcb,
				    ay_gordon_getpropcb,
				    ay_gordon_getpntcb,
				    ay_gordon_readcb,
				    ay_gordon_writecb,
				    ay_gordon_wribcb,
				    ay_gordon_bbccb,
				    AY_IDGORDON);

  ay_status += ay_notify_register(ay_gordon_notifycb, AY_IDGORDON);

  ay_status += ay_convert_register(ay_gordon_convertcb, AY_IDGORDON);

  ay_status += ay_provide_register(ay_gordon_providecb, AY_IDGORDON);


  /* register the DualSkin object as a Gordon in disguise*/
  ay_status = ay_otype_registercore(ay_dskin_name,
				    ay_gordon_createcb,
				    ay_gordon_deletecb,
				    ay_gordon_copycb,
				    ay_gordon_drawcb,
				    ay_gordon_drawhcb,
				    ay_gordon_shadecb,
				    ay_gordon_setpropcb,
				    ay_gordon_getpropcb,
				    ay_gordon_getpntcb,
				    ay_gordon_readcb,
				    ay_gordon_writecb,
				    ay_gordon_wribcb,
				    ay_gordon_bbccb,
				    AY_IDDSKIN);

  ay_status += ay_notify_register(ay_gordon_notifycb, AY_IDDSKIN);

  ay_status += ay_convert_register(ay_gordon_convertcb, AY_IDDSKIN);

  ay_status += ay_provide_register(ay_gordon_providecb, AY_IDDSKIN);

 return ay_status;
} /* ay_gordon_init */

