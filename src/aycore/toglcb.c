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

/* toglcb.c - standard togl callbacks */


/* ay_toglcb_create:
 *  Togl Callback, called on creation of view togl;
 *  creates and links the associated view object
 */
void
ay_toglcb_create(struct Togl *togl)
{
 /*int ay_status = AY_OK;*/
 char fname[] = "toglcb_create";
 ay_view_object *view = NULL, *dview;
 ay_object *o = NULL, *d = NULL, **l = NULL;
 static int id = 0;

  /* create associated view object */
  if(!(view = calloc(1, sizeof(ay_view_object))))
    {
      ay_error(AY_EOMEM, fname, NULL);
      return;
    }

  if(!(o = calloc(1, sizeof(ay_object))))
    {
      free(view);
      ay_error(AY_EOMEM, fname, NULL);
      return;
    }
  ay_object_defaults(o);

  o->refine = view;
  o->type = AY_IDVIEW;
  o->hide = AY_TRUE;
  o->parent = AY_TRUE;
  o->hide_children = AY_TRUE;

  o->down = ay_endlevel;

  /* link object (manually, not via ay_object_link())
     as child to ay_root */
  d = ay_root->down;
  l = &(ay_root->down);
  while(d->next)
    {
      l = &(d->next);
      d = d->next;
    }
  o->next = *l;
  *l = o;

  /* correctly (re)set ay_currentlevel */
  if(ay_currentlevel->next->object == ay_root)
    {
      ay_clevel_del();
      ay_clevel_add(ay_root->down);
    }

  /* add view object pointer to togl object,        */
  /* now togl callbacks have access to the view object */
  Togl_SetClientData(togl, (ClientData)view);

  /* fill view object */
  view->togl = togl;

  view->drawhandles = AY_FALSE;
  view->redraw = AY_TRUE;

  view->from[2] = 10.0;
  view->up[1] = 1.0;
  view->zoom = 3.0;
  view->type = AY_VTFRONT;
  view->enable_undo = AY_TRUE;

  view->id = id;
  id++;

  if(ay_prefs.globalmark)
    {
      /* set the mark (if there is at least one other view) */
      d = ay_root->down;
      while(d->next)
	{
	  if(d != o && d->type == AY_IDVIEW)
	    {
	      dview = (ay_view_object*)d->refine;
	      view->drawmark = dview->drawmark;
	      memcpy(view->markworld, dview->markworld, 3*sizeof(double));
	      break;
	    }
	  d = d->next;
	}
    }

  /* vital OpenGL defaults */
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_AUTO_NORMAL);
  /* works only on OpenGL1.2+: glEnable(GL_RESCALE_NORMAL);*/
  glEnable(GL_NORMALIZE);
  glDisable(GL_DITHER);

  glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, (GLfloat)1.0);
  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
  glEnable(GL_LIGHT0);

  /* glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color); */

#ifndef AYLOCALGLUQUADOBJ
  if(!ay_gluquadobj)
    {
      if(!(ay_gluquadobj = gluNewQuadric()))
	{
	  /* XXXX error handling? */
	  return;
	}
    }
#endif /* AYLOCALGLUQUADOBJ */

 return;
} /* ay_toglcb_create */


/* ay_toglcb_destroy:
 *  Togl callback, called on destruction of view togl;
 *  unlinks and deletes the associated view object
 */
void
ay_toglcb_destroy(struct Togl *togl)
{
 char fname[] = "toglcb_destroy";
 ay_object *root = ay_root, *o, *ov = NULL;
 ay_list_object *clevel;
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 int recreate_clevel = AY_FALSE;

#ifdef AYENABLEPPREV
  if(view)
    {
      if(view->ppreview)
	{
	  ay_wrib_pprevclose();
	}
    }
#endif

  if(ay_currentlevel->object == root->down)
    recreate_clevel = AY_TRUE;

  /* unlink and delete view object */
  if((ay_view_object *)root->down->refine == view)
    {
      ov = root->down;
      root->down = root->down->next;
      if(ov->down && ov->down != ay_endlevel)
	{
	  if(ay_object_candelete(ov->down, ov->down) == AY_OK)
	    {
	      ay_object_deletemulti(ov->down, 1);
	    }
	  else
	    {
	      ay_clipb_prepend(ov->down, fname);
	    }
	}
      ov->down = NULL;
      ov->refine = NULL;
      (void)ay_object_delete(ov);
    }
  else
    {
      o = root->down;
      while(o)
	{
	  if(o->next)
	    {
	      if(o->next->refine == view)
		{
		  ov = o->next;
		  o->next = o->next->next;

		  if(ov->down && ov->down != ay_endlevel)
		    {
		      if(ay_object_candelete(ov->down, ov->down) == AY_OK)
			{
			  ay_object_deletemulti(ov->down, 1);
			}
		      else
			{
			  ay_clipb_prepend(ov->down, fname);
			}
		    }
		  ov->down = NULL;
		  ov->refine = NULL;
		  (void)ay_object_delete(ov);

		  o = NULL;
		}
	      else
		{
		  o = o->next;
		} /* if */
	    }
	  else
	    {
	      /* view object not found in level beneath root! */
	      o = NULL;
	      ay_error(AY_ERROR, fname,
	  "Could not find view object for removal, scene may be corrupt now!");
	    } /* if */
	} /* while */
    } /* if */

  if(view)
    {
      if(view->bgimage)
	free(view->bgimage);

      if(view->bgknotv)
	free(view->bgknotv);

      if(view->bgcv)
	free(view->bgcv);

      free(view);
    }

  if(ay_currentview == view)
    {
      ay_currentview = NULL;
    }

  if(ay_prefs.createview == view)
    {
      ay_prefs.createview = NULL;
    }

  ay_sel_clean();

  clevel = ay_currentlevel;
  while(clevel)
    {
      if(clevel->object == ov)
	{
	  ay_clevel_delall();
	  break;
	}
      clevel = clevel->next;
    }

  if(ay_currentlevel)
    {
      if(recreate_clevel && ay_currentlevel->object != ay_root)
	{
	  ay_currentlevel->object = root->down;
	}

      o = ay_currentlevel->object;
      while(o->next)
	{
	  ay_next = &(o->next);
	  o = o->next;
	}
    }

 return;
} /* ay_toglcb_destroy */


/* ay_toglcb_reshape:
 *  Togl callback, called when view togl is
 *  exposed or reshaped
 */
void
ay_toglcb_reshape(struct Togl *togl)
{
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 double aspect = 0.0, width = (double)Togl_Width(togl),
   height = (double)Togl_Height(togl);

  ay_viewt_setupprojection(togl);

  aspect = width/height;

  view->conv_x = (aspect * 2.0 / width) * view->zoom;
  view->conv_y = 2.0/height * view->zoom;

  ay_viewt_updatemark(togl, /* local=*/ AY_TRUE);

  /*  glMatrixMode (GL_MODELVIEW);*/

  if(ay_prefs.cullfaces)
    {
      glEnable(GL_CULL_FACE);
    }
  else
    {
      glDisable(GL_CULL_FACE);
    }

 return;
} /* ay_toglcb_reshape */


/* ay_toglcb_display:
 *  Togl callback, draw view togl
 *  apply changes to this function also to ay_viewt_redrawtcb()!
 */
void
ay_toglcb_display(struct Togl *togl)
{
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 int npdm, ncdm;
 double tol, stqf;

  if(!view->redraw)
    {
      return;
    }

#ifdef AYLOCALGLUQUADOBJ
  if(!(ay_gluquadobj = gluNewQuadric()))
    return;
#endif /* AYLOCALGLUQUADOBJ */

  if(view->altdispcb)
    {
      (view->altdispcb)(togl);
    }
  else
    {
      glClearColor((GLfloat)ay_prefs.bgr, (GLfloat)ay_prefs.bgg,
		   (GLfloat)ay_prefs.bgb, (GLfloat)1.0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      if(view->antialiaslines)
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

      if(view->action_state)
	{
	  /* in action => swap in the second set of draw/display
	     parameters */
	  npdm = ay_prefs.np_display_mode;
	  ncdm = ay_prefs.nc_display_mode;
	  tol = ay_prefs.glu_sampling_tolerance;
	  stqf = ay_prefs.stess_qf;
	  if(ay_prefs.glu_sampling_tolerance_a != 0.0)
	    {
	      if(ay_prefs.glu_sampling_tolerance_a < 0.0)
		ay_prefs.glu_sampling_tolerance *=
		  -ay_prefs.glu_sampling_tolerance_a;
	      else
		ay_prefs.glu_sampling_tolerance =
		  ay_prefs.glu_sampling_tolerance_a;
	      ay_prefs.stess_qf =
		ay_stess_GetQF(ay_prefs.glu_sampling_tolerance);
	    }
	  if(ay_prefs.np_display_mode_a > 0)
	    ay_prefs.np_display_mode = ay_prefs.np_display_mode_a-1;
	  if(ay_prefs.nc_display_mode_a > 0)
	    ay_prefs.nc_display_mode = ay_prefs.nc_display_mode_a-1;
	}

      /* draw */
      if(view->drawmode)
	ay_shade_view(togl);
      else
	ay_draw_view(togl, /*draw_offset=*/AY_FALSE);

      /* XXXX is this really necessary? */
      /*glFlush();*/

      Togl_SwapBuffers(togl);

      /* improve mouse input lag */
      glFinish();

      if(view->action_state)
	{
	  /* in action => restore draw/display parameters */
	  ay_prefs.np_display_mode = npdm;
	  ay_prefs.nc_display_mode = ncdm;
	  ay_prefs.glu_sampling_tolerance = tol;
	  ay_prefs.stess_qf = stqf;
	  /*view->display_list_active = AY_FALSE;*/
	}

      /* front buffer can not contain flashed points anymore now */
      ay_pact_flashpoint(AY_FALSE, AY_FALSE, NULL, NULL);
    } /* if !altdisp */

#ifdef AYENABLEPPREV
  /* redraw permanent preview window? */
  if(view->ppreview)
    ay_wrib_pprevdraw(view);
#endif

#ifdef AYLOCALGLUQUADOBJ
  gluDeleteQuadric(ay_gluquadobj);
#endif /* AYLOCALGLUQUADOBJ */

 return;
} /* ay_toglcb_display */
