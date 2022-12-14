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

/* draw.c - functions for drawing a scene using OpenGL */

void ay_draw_annos(struct Togl *togl, int draw_offset);


/* ay_draw_object:
 *  draw a single object o (and children) in view togl
 *  o if selected is AY_FALSE, selected objects
 *    will not be drawn!
 *  o if selected is 2, a name will be pushed onto OpenGLs
 *    name stack additionally.
 */
void
ay_draw_object(struct Togl *togl, ay_object *o, int selected)
{
 int ay_status = AY_OK;
 char fname[] = "draw_object";
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 ay_voidfp *arr = NULL;
 ay_drawcb *cb = NULL;
 ay_object *down;
 double m[16];

  if(selected == AY_FALSE)
    if(o->selected)
      return;

  if(o->hide)
    {
      o->glname = 0;
      return;
    }

  glPushMatrix();

   glTranslated((GLdouble)o->movx, (GLdouble)o->movy, (GLdouble)o->movz);
   ay_quat_torotmatrix(o->quat, m);
   glMultMatrixd((GLdouble*)m);
   glScaled((GLdouble)o->scalx, (GLdouble)o->scaly, (GLdouble)o->scalz);

   if(selected == AY_TRUE)
     {
       if(view->drawobjectcs)
	 {
	   ay_draw_cs(togl, 0);
	 }
     }

   arr = ay_drawcbt.arr;
   cb = (ay_drawcb *)(arr[o->type]);

   if(selected == 2)
     {
       o->glname = ++ay_glname;
       glPushName(o->glname);
     }

   if(cb)
     ay_status = cb(togl, o);

   if(ay_status)
     {
       ay_error(AY_ERROR, fname, "draw callback failed");

       glPopMatrix();
       return;
     }

   if(!o->inherit_trafos)
     glLoadIdentity();

   if(!o->hide_children && o->down)
     {
       down = o->down;
       while(down->next)
	 {
	   ay_draw_object(togl, down, selected);
	   down = down->next;
	 } /* while */
     }

  if(selected == 2)
    {
      glPopName();
    }

  glPopMatrix();

 return;
} /* ay_draw_object */


/* ay_draw_view:
 *  draw a view
 */
int
ay_draw_view(struct Togl *togl, int draw_offset)
{
 int ay_status = AY_OK;
 char fname[] = "draw_view";
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 ay_list_object *sel = ay_selection;
 ay_object *o = ay_root;
 ay_voidfp *arr = NULL;
 ay_drawcb *cb = NULL;
 ay_point *point = NULL;
 double m[16];

  glDisable(GL_LIGHTING);
  glMatrixMode(GL_MODELVIEW);

  if(!draw_offset)
    {
      if(view->dirty)
	{
	  ay_toglcb_reshape(togl);
	  view->dirty = AY_FALSE;
	}

      if(view->drawbgimage)
	{
	  ay_draw_bgimage(togl);
	}

      /* draw grid */
      if(view->drawgrid)
	{
	  ay_draw_grid(togl);
	}
    }

  /* draw bounds of parametric space of current NURBS patch */
  if(view->type == AY_VTTRIM)
    {
      ay_draw_trimview();
    }

  /* enable anti-aliasing for lines */
  if(view->antialiaslines)
    {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_LINE_SMOOTH);
      glDepthMask(GL_FALSE);
      if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	glLineWidth((GLfloat)(ay_prefs.aalinewidth * ay_prefs.guiscale));
      else
	glLineWidth((GLfloat)ay_prefs.aalinewidth);
    }
  else
    {
      if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	glLineWidth((GLfloat)(ay_prefs.linewidth * ay_prefs.guiscale));
      else
	glLineWidth((GLfloat)ay_prefs.linewidth);
    }

  if(view->drawlevel || view->type == AY_VTTRIM)
    {
      o = ay_currentlevel->object;
      if(view->type == AY_VTTRIM && ay_currentlevel->next &&
	 ay_currentlevel->next->object &&
	 ay_currentlevel->next->object->type == AY_IDTRIM)
	{
	  o = o->next;
	}
      glPushMatrix();
      if(ay_currentlevel->object != ay_root && view->type != AY_VTTRIM)
	{
	  ay_trafo_concatparent(ay_currentlevel->next);
	}
      if(view->type == AY_VTTRIM)
	{
	  glLoadIdentity();
	}
    }

  /* set color for deselected objects */
  glColor3f((GLfloat)ay_prefs.obr,
	    (GLfloat)ay_prefs.obg,
	    (GLfloat)ay_prefs.obb);

  /* draw unselected objects */
  if(!view->drawsel)
    {
      while(o && o->next)
	{
	  ay_draw_object(togl, o, AY_FALSE);
	  o = o->next;
	}
    }

  /* draw selected objects */
  if(sel)
    {
      /* set color for selected objects */
      glColor3f((GLfloat)ay_prefs.ser,
		(GLfloat)ay_prefs.seg,
		(GLfloat)ay_prefs.seb);

      if(view->antialiaslines)
	{
	  if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	    glLineWidth((GLfloat)(ay_prefs.aasellinewidth * ay_prefs.guiscale));
	  else
	    glLineWidth((GLfloat)ay_prefs.aasellinewidth);
	}
      else
	{
	  if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	    glLineWidth((GLfloat)(ay_prefs.sellinewidth * ay_prefs.guiscale));
	  else
	    glLineWidth((GLfloat)ay_prefs.sellinewidth);
	}

      glPushMatrix();
      if(!view->drawlevel && view->type != AY_VTTRIM)
	{
	  if(ay_currentlevel->object != ay_root)
	    {
	      ay_trafo_concatparent(ay_currentlevel->next);
	    }
	}

      /* let selected objects appear "on top" of current drawing */
      if(!draw_offset)
	{
	  glDisable(GL_DEPTH_TEST);
	}
      else
	{
	  glDepthRange(0.0, 0.99999);
	  glDepthFunc(GL_LEQUAL);
	}

       while(sel)
	 {
	   ay_draw_object(togl, sel->object, AY_TRUE);
	   sel = sel->next;
	 }

      if(!draw_offset)
	{
	  glEnable(GL_DEPTH_TEST);
	}
      else
	{
	  glDepthRange(0.0, 1.0);
	  glDepthFunc(GL_LESS);
	}

      /* draw handles of selected objects */
      if(view->drawhandles && view->drawhandles < 4)
	{
	  /* let all handles appear "on top" of current drawing;     */
	  /* we cannot use the glDisable(GL_DEPTH_TEST);-method here */
	  /* because we need the Z-values for vertice picking...     */
	  if(view->type != AY_VTPERSP && view->drawmode != AY_DMWIREHIDDEN)
	    {
	      glClear(GL_DEPTH_BUFFER_BIT);
	    }

	  if(view->antialiaslines)
	    {
	      glDepthMask(GL_TRUE);
	    }

	  /* set "size" of points */
	  if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	    glPointSize((GLfloat)(ay_prefs.handle_size * ay_prefs.guiscale));
	  else
	    glPointSize((GLfloat)ay_prefs.handle_size);

	  arr = ay_drawhcbt.arr;
	  sel = ay_selection;

	  while(sel)
	    {
	      o = sel->object;

	      if(!o->hide)
		{
		  glPushMatrix();

		   glTranslated((GLdouble)o->movx,
				(GLdouble)o->movy,
				(GLdouble)o->movz);
		   ay_quat_torotmatrix(o->quat, m);
		   glMultMatrixd((GLdouble*)m);
		   glScaled((GLdouble)o->scalx,
			    (GLdouble)o->scaly,
			    (GLdouble)o->scalz);

		   cb = (ay_drawcb *)(arr[o->type]);

		   if(cb)
		     {
		       ay_status = cb(togl, o);
		       if(ay_status)
			 {
			   ay_error(ay_status, fname,
				    "draw handle callback failed");
			 }
		     }

		  glPopMatrix();
		} /* if */
	      sel = sel->next;
	    } /* while */

	  /* draw selected points */

	  /* let those be "on top" of all points */
	  if(view->type != AY_VTPERSP)
	    {
	      glDisable(GL_DEPTH_TEST);
	    }
	  else
	    {
	      glDepthRange(0.0, 0.99998);
	      glDepthFunc(GL_LEQUAL);
	    }

	  /* temporarily disable drawing of selected points in
	     "edit weight" and "find u" actions */
	  if(view->drawhandles < 2)
	    {
	  /* set color for selected points */
	  glColor3f((GLfloat)ay_prefs.tpr,
		    (GLfloat)ay_prefs.tpg,
		    (GLfloat)ay_prefs.tpb);

	  if(ay_prefs.scopemanagement)
	    view->transform_points = AY_FALSE;

	  sel = ay_selection;
	  while(sel)
	    {
	      o = sel->object;

	      if(!o->hide && o->selp)
		{
		  if(ay_prefs.scopemanagement)
		    {
		      if(!o->selp->readonly)
			view->transform_points = AY_TRUE;
		    }

		  glPushMatrix();
		   glTranslated((GLdouble)o->movx,
				(GLdouble)o->movy,
				(GLdouble)o->movz);
		   ay_quat_torotmatrix(o->quat, m);
		   glMultMatrixd((GLdouble*)m);
		   glScaled((GLdouble)o->scalx,
			    (GLdouble)o->scaly,
			    (GLdouble)o->scalz);
		   point = o->selp;
		   glBegin(GL_POINTS);
		    while(point)
		      {
			if(point->type < AY_PTKNOT)
			  {
			    if(point->type == AY_PTRAT &&
			       ay_prefs.rationalpoints)
			  glVertex3d((GLdouble)point->point[0]*point->point[3],
				     (GLdouble)point->point[1]*point->point[3],
				     (GLdouble)point->point[2]*point->point[3]);
			    else
			      glVertex3d((GLdouble)point->point[0],
					 (GLdouble)point->point[1],
					 (GLdouble)point->point[2]);
			  }
			/*
			  else
			  draw break points?
			*/
			point = point->next;
		      } /* while */
		   glEnd();
		  glPopMatrix();
		}
	      sel = sel->next;
	    } /* while */

	    } /* if */

	  /* set color for selected objects */
	  glColor3f((GLfloat)ay_prefs.ser, (GLfloat)ay_prefs.seg,
		    (GLfloat)ay_prefs.seb);

	  if(view->type != AY_VTPERSP)
	    {
	      glEnable(GL_DEPTH_TEST);
	    }
	  else
	    {
	      glDepthRange(0.0, 1.0);
	      glDepthFunc(GL_LESS);
	    }
	} /* if drawhandles */
      glPopMatrix();
    } /* if sel */

  if(view->antialiaslines)
    {
      if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	glLineWidth((GLfloat)(ay_prefs.aalinewidth * ay_prefs.guiscale));
      else
	glLineWidth((GLfloat)ay_prefs.aalinewidth);
    }
  else
    {
      if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	glLineWidth((GLfloat)(ay_prefs.linewidth * ay_prefs.guiscale));
      else
	glLineWidth((GLfloat)ay_prefs.linewidth);
    }

  ay_draw_annos(togl, draw_offset);

  if(draw_offset)
    {
      glDepthRange(0.0, 1.0);
    }

  if(view->drawlevel || view->type == AY_VTTRIM)
    {
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
    }

  if(view->antialiaslines)
    {
      glDisable(GL_BLEND);
      glDisable(GL_LINE_SMOOTH);
      glDepthMask(GL_TRUE);
    }

  if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
    glLineWidth((GLfloat)(1.0 * ay_prefs.guiscale));
  else
    glLineWidth((GLfloat)1.0f);

 return AY_OK;
} /* ay_draw_view */


/* ay_draw_annos:
 *  draw the annotations (coordinate systems, handles, and the mark)
 */
void
ay_draw_annos(struct Togl *togl, int draw_offset)
{
 int ay_status = AY_OK;
 char fname[] = "draw_annos";
 ay_voidfp *arr;
 ay_drawcb *cb;
 ay_object *o;
 ay_list_object *sel;
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 int draw_root = AY_TRUE;
 double m[16];

  arr = ay_drawacbt.arr;

  if(ay_root->hide || view->drawsel)
    draw_root = AY_FALSE;
  else
    if(view->drawlevel && ay_currentlevel->object != ay_root)
      draw_root = AY_FALSE;

  /* draw the root annotations (coordinate system) */
  if(draw_root)
    {
      cb = (ay_drawcb *)(arr[ay_root->type]);

      if(cb)
	{
	  if(ay_root->selected)
	    glColor3f((GLfloat)ay_prefs.ser,
		      (GLfloat)ay_prefs.seg,
		      (GLfloat)ay_prefs.seb);
	  else
	    glColor3f((GLfloat)ay_prefs.obr,
		      (GLfloat)ay_prefs.obg,
		      (GLfloat)ay_prefs.obb);

	  ay_status = cb(togl, ay_root);
	  if(ay_status)
	    {
	      ay_error(ay_status, fname, "draw annotation callback failed");
	    }
	} /* if cb */
    } /* if */

  /* draw the annotations of all selected objects */
  if(view->drawhandles)
    {
      /* set color for selected objects */
      glColor3f((GLfloat)ay_prefs.ser,
		(GLfloat)ay_prefs.seg,
		(GLfloat)ay_prefs.seb);

      glMatrixMode(GL_MODELVIEW);

      if(ay_currentlevel->object != ay_root)
	{
	  glPushMatrix();
	  glLoadIdentity();
	  ay_trafo_concatparent(ay_currentlevel->next);
	}
      if(view->type == AY_VTTRIM)
	{
	  glLoadIdentity();
	}
      sel = ay_selection;
      while(sel)
	{
	  o = sel->object;

	  if(!o->hide)
	    {
	      cb = (ay_drawcb *)(arr[o->type]);

	      if(!cb && (ay_peek_object(o, AY_IDNPATCH, NULL, NULL) > 0))
		cb = ay_npt_drawboundaries;
	      if(cb)
		{
		  glPushMatrix();
		   if(AY_ISTRAFO(o))
		     {
		       glTranslated((GLdouble)o->movx,
				    (GLdouble)o->movy,
				    (GLdouble)o->movz);
		       ay_quat_torotmatrix(o->quat, m);
		       glMultMatrixd((GLdouble*)m);
		       glScaled((GLdouble)o->scalx,
				(GLdouble)o->scaly,
				(GLdouble)o->scalz);
		     }
		   ay_status = cb(togl, o);
		   if(ay_status)
		     {
		       ay_error(ay_status, fname,
				"draw annotation callback failed");
		     }
		  glPopMatrix();
		} /* if cb */
	    } /* if !hidden */
	  sel = sel->next;
	} /* while */

      if(ay_currentlevel->object != ay_root)
	{
	  glPopMatrix();
	}
    } /* if drawhandles */

  /* draw marked point in space */
  if(!draw_offset)
    {
      if(view->drawmark)
	{
	  ay_draw_mark(togl);
	} /* if */
    } /* if */

 return;
} /* ay_draw_annos */

#define AY_PI4 AY_PI*0.25

/* ay_draw_grid:
 *  draw the current grid
 */
void
ay_draw_grid(struct Togl *togl)
{
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 ay_object *o;
 int width = Togl_Width(togl);
 int height = Togl_Height(togl);
 double aspect = (double)width / (double)height;
 double grid = view->grid;
 double a, j, h, dx, dy, dz;
 double minwinx = 0.0, minwiny = 0.0, maxwinx = 0.0, maxwiny = 0.0;
 double m[16];
 GLdouble mp[16], mm[16], owinx, owiny, owinz, gwinx, gwiny, gwinz;
 GLint vp[4];

  if(view->local && view->type != AY_VTPERSP)
    {
      /* local view */
      glGetIntegerv(GL_VIEWPORT, vp);
      glGetDoublev(GL_PROJECTION_MATRIX, mp);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
       if(ay_currentlevel->object != ay_root)
	 {
	   ay_trafo_concatparent(ay_currentlevel->next);
	 }
       if((view->local == 2) && view->aligned && ay_selection)
	 {
	   o = ay_selection->object;
	   glTranslated((GLdouble)o->movx,
			(GLdouble)o->movy,
			(GLdouble)o->movz);
	   ay_quat_torotmatrix(o->quat, m);
	   glMultMatrixd((GLdouble*)m);

	   glScaled((GLdouble)o->scalx,
		    (GLdouble)o->scaly,
		    (GLdouble)o->scalz);
	 } /* if */
       glGetDoublev(GL_MODELVIEW_MATRIX, mm);
      glPopMatrix();

      /* get origin in window space */
      if(GL_FALSE == gluProject(0.0, 0.0, 0.0, mm, mp, vp,
				&owinx, &owiny, &owinz))
	{
	  return;
	}

      switch(view->type)
	{
	case AY_VTFRONT:
	case AY_VTTRIM:
	  if(GL_FALSE == gluProject(view->grid, view->grid, 0.0, mm, mp, vp,
				    &gwinx, &gwiny, &gwinz))
	    {
	      return;
	    }
	  break;
	case AY_VTSIDE:
	  if(GL_FALSE == gluProject(0.0, view->grid, view->grid, mm, mp, vp,
				    &gwinx, &gwiny, &gwinz))
	    {
	      return;
	    }
	  break;
	case AY_VTTOP:
	  if(GL_FALSE == gluProject(view->grid, 0.0, view->grid, mm, mp, vp,
				    &gwinx, &gwiny, &gwinz))
	    {
	      return;
	    }
	  break;
	default:
	  gwinx = 0.0;
	  gwiny = 0.0;
	  gwinz = 0.0;
	  break;
	}

      dx = fabs(gwinx-owinx);
      dy = fabs(gwiny-owiny);

      /*
	protect from drawing more lines than the view has pixels;
	more precisely, we already stop drawing the grid if we do not
	have more than two blank pixels between two grid lines
      */
      if((dx < 3.0) || (dy < 3.0))
	return;

      glColor3f((GLfloat)ay_prefs.grr,
		(GLfloat)ay_prefs.grg,
		(GLfloat)ay_prefs.grb);

      glDisable(GL_DEPTH_TEST);

      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
       glLoadIdentity();
       glOrtho(0, width, 0, height, -100.0, 100.0);
       glMatrixMode(GL_MODELVIEW);
       glPushMatrix();
        glLoadIdentity();
	glBegin(GL_LINES);
	for(j = owinx; j < width; j += dx)
	  {
	    glVertex3d(j,    0.0, 0.0);
	    glVertex3d(j, height, 0.0);
	  }
	for(j = owinx; j > 0.0; j -= dx)
	  {
	    glVertex3d(j,    0.0, 0.0);
	    glVertex3d(j, height, 0.0);
	  }
	for(j = owiny; j < height; j += dy)
	  {
	    glVertex3d(  0.0, j, 0.0);
	    glVertex3d(width, j, 0.0);
	  }
	for(j = owiny; j > 0.0; j -= dy)
	  {
	    glVertex3d(  0.0, j, 0.0);
	    glVertex3d(width, j, 0.0);
	  }
	glEnd();
       glPopMatrix();
       glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);

      glEnable(GL_DEPTH_TEST);
    }
  else
    {
      /* global (or perspective) view */

      /*
	protect from drawing more lines than the view has pixels;
	more precisely, we already stop drawing the grid if we do not
	have more than two blank pixels between two grid lines
      */
      if((grid/view->conv_x < 3.0) || (grid/view->conv_y < 3.0))
	return;

      switch(view->type)
	{
	case AY_VTFRONT:
	case AY_VTTRIM:
	  minwinx = ((-aspect * view->zoom) + view->from[0]) - grid;
	  minwiny = ((-view->zoom) + view->from[1]) - grid;
	  break;
	case AY_VTSIDE:
	  minwinx = ((-aspect * view->zoom) + view->from[2]) - grid;
	  minwiny = ((-view->zoom) + view->from[1]) - grid;
	  break;
	case AY_VTTOP:
	  minwinx = ((-aspect * view->zoom) + view->from[0]) - grid;
	  minwiny = ((-view->zoom) + view->from[2]) - grid;
	  break;
	default:
	  break;
	}

      maxwinx = minwinx + 2*((aspect * view->zoom)) + grid;
      maxwiny = minwiny + 2*(view->zoom) + grid;

      dx = fabs(view->from[0]-view->to[0]);
      dy = fabs(view->from[1]-view->to[1]);
      dz = fabs(view->from[2]-view->to[2]);
      j = 2500;

      glColor3f((GLfloat)ay_prefs.grr,
		(GLfloat)ay_prefs.grg,
		(GLfloat)ay_prefs.grb);

      glDisable(GL_DEPTH_TEST);

      glBegin(GL_LINES);

      switch(view->type)
	{
	case AY_VTFRONT:
	case AY_VTTRIM:
	  if(dx > AY_EPSILON)
	    {
	      h = sqrt(dx*dx+dz*dz);
	      a = asin(dx/h);
	      if(fabs(a) > AY_EPSILON)
		{
		  minwinx -= a/AY_PI4*j;
		  maxwinx += a/AY_PI4*j;
		}
	    }
	  if(dy > AY_EPSILON)
	    {
	      h = sqrt(dy*dy+dz*dz);
	      a = asin(dy/h);
	      if(fabs(a) > AY_EPSILON)
		{
		  minwiny -= a/AY_PI4*j;
		  maxwiny += a/AY_PI4*j;
		}
	    }

	  /* vertical lines */
	  for(j = 0.0; j < maxwinx; j += view->grid)
	    {
	      glVertex3d((GLdouble)j, (GLdouble)minwiny, (GLdouble)0.0);
	      glVertex3d((GLdouble)j, (GLdouble)maxwiny, (GLdouble)0.0);
	    }
	  for(j = (0.0-view->grid); j > minwinx; j -= view->grid)
	    {
	      glVertex3d((GLdouble)j, (GLdouble)minwiny, (GLdouble)0.0);
	      glVertex3d((GLdouble)j, (GLdouble)maxwiny, (GLdouble)0.0);
	    }

	  /* horizontal lines */
	  for(j = 0.0; j < maxwiny; j += view->grid)
	    {
	      glVertex3d((GLdouble)minwinx, (GLdouble)j, (GLdouble)0.0);
	      glVertex3d((GLdouble)maxwinx, (GLdouble)j, (GLdouble)0.0);
	    }
	  for(j = (0.0-view->grid); j > minwiny; j -= view->grid)
	    {
	      glVertex3d((GLdouble)minwinx, (GLdouble)j, (GLdouble)0.0);
	      glVertex3d((GLdouble)maxwinx, (GLdouble)j, (GLdouble)0.0);
	    }
	  break;
	case AY_VTSIDE:
	  if(dz > AY_EPSILON)
	    {
	      h = sqrt(dz*dz+dx*dx);
	      a = asin(dz/h);
	      if(fabs(a) > AY_EPSILON)
		{
		  minwinx -= a/AY_PI4*j;
		  maxwinx += a/AY_PI4*j;
		}
	    }
	  if(dy > AY_EPSILON)
	    {
	      h = sqrt(dy*dy+dx*dx);
	      a = asin(dy/h);
	      if(fabs(a) > AY_EPSILON)
		{
		  minwiny -= a/AY_PI4*j;
		  maxwiny += a/AY_PI4*j;
		}
	    }

	  /* vertical lines */
	  for(j = 0.0; j < maxwinx; j += view->grid)
	    {
	      glVertex3d((GLdouble)0.0, (GLdouble)minwiny, (GLdouble)j);
	      glVertex3d((GLdouble)0.0, (GLdouble)maxwiny, (GLdouble)j);
	    }
	  for(j = (0.0-view->grid); j > minwinx; j -= view->grid)
	    {
	      glVertex3d((GLdouble)0.0, (GLdouble)minwiny, (GLdouble)j);
	      glVertex3d((GLdouble)0.0, (GLdouble)maxwiny, (GLdouble)j);
	    }

	  /* horizontal lines */
	  for(j = 0.0; j < maxwiny; j += view->grid)
	    {
	      glVertex3d((GLdouble)0.0, (GLdouble)j, (GLdouble)minwinx);
	      glVertex3d((GLdouble)0.0, (GLdouble)j, (GLdouble)maxwinx);
	    }
	  for(j = (0.0-view->grid); j > minwiny; j -= view->grid)
	    {
	      glVertex3d((GLdouble)0.0, (GLdouble)j, (GLdouble)minwinx);
	      glVertex3d((GLdouble)0.0, (GLdouble)j, (GLdouble)maxwinx);
	    }
	  break;
	case AY_VTTOP:
	  if(dx > AY_EPSILON)
	    {
	      h = sqrt(dx*dx+dy*dy);
	      a = asin(dx/h);
	      if(fabs(a) > AY_EPSILON)
		{
		  minwinx -= a/AY_PI4*j;
		  maxwinx += a/AY_PI4*j;
		}
	    }
	  if(dz > AY_EPSILON)
	    {
	      h = sqrt(dz*dz+dy*dy);
	      a = asin(dz/h);
	      if(fabs(a) > AY_EPSILON)
		{
		  minwiny -= a/AY_PI4*j;
		  maxwiny += a/AY_PI4*j;
		}
	    }
	  /* vertical lines */
	  for(j = 0.0; j < maxwinx; j += view->grid)
	    {
	      glVertex3d((GLdouble)j, (GLdouble)0.0, (GLdouble)minwiny);
	      glVertex3d((GLdouble)j, (GLdouble)0.0, (GLdouble)maxwiny);
	    }
	  for(j = (0.0-view->grid); j > minwinx; j -= view->grid)
	    {
	      glVertex3d((GLdouble)j, (GLdouble)0.0, (GLdouble)minwiny);
	      glVertex3d((GLdouble)j, (GLdouble)0.0, (GLdouble)maxwiny);
	    }

	  /* horizontal lines */
	  for(j = 0.0; j < maxwiny; j += view->grid)
	    {
	      glVertex3d((GLdouble)minwinx, (GLdouble)0.0, (GLdouble)j);
	      glVertex3d((GLdouble)maxwinx, (GLdouble)0.0, (GLdouble)j);
	    }
	  for(j = (0.0-view->grid); j > minwiny; j -= view->grid)
	    {
	      glVertex3d((GLdouble)minwinx, (GLdouble)0.0, (GLdouble)j);
	      glVertex3d((GLdouble)maxwinx, (GLdouble)0.0, (GLdouble)j);
	    }
	  break;
	case AY_VTPERSP:
	  minwinx = -(25.0 * view->grid);
	  minwiny = -(25.0 * view->grid);
	  maxwinx = (25.0 * view->grid);
	  maxwiny = (25.0 * view->grid);
	  /* draw lines */
	  for(j = -25.0 * view->grid; j <= maxwinx; j += view->grid)
	    {
	      /* vertical */
	      glVertex3d((GLdouble)j, (GLdouble)0.0, (GLdouble)minwiny);
	      glVertex3d((GLdouble)j, (GLdouble)0.0, (GLdouble)maxwiny);
	      /* horizontal */
	      glVertex3d((GLdouble)minwinx, (GLdouble)0.0, (GLdouble)j);
	      glVertex3d((GLdouble)maxwinx, (GLdouble)0.0, (GLdouble)j);
	    }
	  break;
	} /* switch */

      glEnd();

      glEnable(GL_DEPTH_TEST);
    } /* if */

 return;
} /* ay_draw_grid */


/* ay_draw_arrow:
 *  draw an arrow
 */
void
ay_draw_arrow(struct Togl *togl, double *from, double *to)
{
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 GLdouble mvm[16], pm[16], win1x, win1y, win1z, win2x, win2y, win2z;
 GLdouble p1x, p1y, p2x, p2y, alpha;
 GLint vp[4];
 double l = ay_prefs.handle_size*0.75;

  if(l < 7)
    {
      l = 7.1;
    }

  if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
    l *= ay_prefs.guiscale;

  glGetDoublev(GL_MODELVIEW_MATRIX, mvm);
  glGetDoublev(GL_PROJECTION_MATRIX, pm);
  glGetIntegerv(GL_VIEWPORT, vp);

  if(GL_FALSE == gluProject((GLdouble)to[0], (GLdouble)to[1],
			    (GLdouble)to[2], mvm, pm, vp,
			    &win1x, &win1y, &win1z))
    {
      return;
    }

  if(GL_FALSE == gluProject((GLdouble)from[0], (GLdouble)from[1],
			    (GLdouble)from[2], mvm, pm, vp,
			    &win2x, &win2y, &win2z))
    {
      return;
    }

  p1x = win1x+l;
  p1y = win1y+l;

  p2x = win1x+l;
  p2y = win1y-l;

  if(!view->antialiaslines)
    {
      p1x -= 0.735;
      p1y -= 0.735;
    }

  if(win2x-win1x != 0.0)
    {
      alpha = AY_R2D(atan((win2y-win1y)/(win2x-win1x)));

      if(win2x < win1x)
	alpha = 180.0+alpha;
    }
  else
    {
      if(win2y > win1y)
	alpha = 90.0;
      else
	alpha = -90.0;
    }

  glDisable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
   glLoadIdentity();
   glOrtho(0, Togl_Width(togl), 0, Togl_Height(togl), -100.0, 100.0);

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
    glLoadIdentity();
    glTranslated(win1x,win1y,0.0);
    glRotated(alpha,0.0,0.0,1.0);
    glTranslated(-win1x,-win1y,0.0);
    if(!view->antialiaslines)
      glTranslated(0.375,0.375,0.0);
    if(view->antialiaslines)
      {
	if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	  glLineWidth((GLfloat)(ay_prefs.aasellinewidth * 2.0f *
				ay_prefs.guiscale));
	else
	  glLineWidth((GLfloat)ay_prefs.aasellinewidth*2.0f);
      }
    else
      {
	if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	  glLineWidth((GLfloat)(ay_prefs.sellinewidth * 4.0f *
				ay_prefs.guiscale));
	else
	  glLineWidth((GLfloat)ay_prefs.sellinewidth*4.0f);
      }
    glColor3f((GLfloat)ay_prefs.bgr,
	      (GLfloat)ay_prefs.bgg,
	      (GLfloat)ay_prefs.bgb);
    glBegin(GL_LINE_STRIP);
     glVertex3d(p1x,p1y,win1z);
     glVertex3d(win1x,win1y,win1z);
     glVertex3d(p2x,p2y,win1z);
    glEnd();

    if(view->antialiaslines)
      {
	if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	  glLineWidth((GLfloat)(ay_prefs.aasellinewidth*ay_prefs.guiscale));
	else
	  glLineWidth((GLfloat)ay_prefs.aasellinewidth);
      }
    else
      {
	if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	  glLineWidth((GLfloat)(ay_prefs.sellinewidth * 2.0f *
				ay_prefs.guiscale));
	else
	  glLineWidth((GLfloat)ay_prefs.sellinewidth*2.0f);
      }
    glColor3f((GLfloat)ay_prefs.ser,
	      (GLfloat)ay_prefs.seg,
	      (GLfloat)ay_prefs.seb);
    glBegin(GL_LINE_STRIP);
     glVertex3d(p1x,p1y,win1z);
     glVertex3d(win1x,win1y,win1z);
     glVertex3d(p2x,p2y,win1z);
    glEnd();

    glPopMatrix();
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  glEnable(GL_DEPTH_TEST);

 return;
} /* ay_draw_arrow */


/* ay_draw_bgimage:
 *  draw background image (if there is one)
 */
void
ay_draw_bgimage(struct Togl *togl)
{
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 GLint w = Togl_Width(togl), h = Togl_Height(togl);
 GLfloat color[4] = {0.0f,0.0f,0.0f,0.0f};
 GLUnurbsObj *no;
 GLdouble sampling_tolerance = ay_prefs.glu_sampling_tolerance;
 GLint uknot_count, vknot_count;

  if(view->bgimage)
    {
      glDisable(GL_DEPTH_TEST);
      glEnable(GL_TEXTURE_2D);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

      if(view->bgknotv && view->bgcv)
	{
	  /* map the image onto a NURBS patch */
	  glColor3f((GLfloat)ay_prefs.bgr, (GLfloat)ay_prefs.bgg,
		    (GLfloat)ay_prefs.bgb);

	  color[0] = (GLfloat)ay_prefs.bgr;
	  color[1] = (GLfloat)ay_prefs.bgg;
	  color[2] = (GLfloat)ay_prefs.bgb;
	  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);

	  no = gluNewNurbsRenderer();

	  gluNurbsCallback(no, GLU_ERROR, AYGLUCBTYPE ay_error_glucb);

	  gluBeginSurface(no);

	   gluNurbsProperty(no, GLU_SAMPLING_TOLERANCE,
			    (GLfloat)sampling_tolerance);

	   gluNurbsProperty(no, GLU_DISPLAY_MODE, GLU_FILL);
	   gluNurbsProperty(no, GLU_CULLING, GL_TRUE);

	   uknot_count = view->bgwidth+view->bguorder;
	   vknot_count = view->bgheight+view->bgvorder;

	   /* specify the texture coordinates */
	   gluNurbsSurface(no, uknot_count, view->bgknotv,
			   vknot_count, &(view->bgknotv[uknot_count]),
			   (GLint)view->bgheight*6, (GLint)6, &(view->bgcv[4]),
			   (GLint)view->bguorder, (GLint)view->bgvorder,
			   GL_MAP2_TEXTURE_COORD_2);

	   /* specify the surface */
	   gluNurbsSurface(no, uknot_count, view->bgknotv,
			   vknot_count, &(view->bgknotv[uknot_count]),
			   (GLint)view->bgheight*6, (GLint)6, view->bgcv,
			   (GLint)view->bguorder, (GLint)view->bgvorder,
			   GL_MAP2_VERTEX_4);

	  gluEndSurface(no);

	  gluDeleteNurbsRenderer(no);
	}
      else
	{
	  /* fill the complete view window */
	  glMatrixMode(GL_PROJECTION);
	  glPushMatrix();
	   glLoadIdentity();
	   glOrtho(0.0, (GLdouble) w, 0.0, (GLdouble) h, -1.0, 1.0);

	   glMatrixMode(GL_MODELVIEW);
	   glPushMatrix();
	    glLoadIdentity();

	    glColor3f((GLfloat)ay_prefs.bgr,
		      (GLfloat)ay_prefs.bgg,
		      (GLfloat)ay_prefs.bgb);

	    color[0] = (GLfloat)ay_prefs.bgr;
	    color[1] = (GLfloat)ay_prefs.bgg;
	    color[2] = (GLfloat)ay_prefs.bgb;
	    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);

	    glBegin(GL_QUADS);
	     glTexCoord2i(0, 0);
	     glVertex3i(0, 0, 0);

	     glTexCoord2i(0, 1);
	     glVertex3i(0, h, 0);

	     glTexCoord2i(1, 1);
	     glVertex3i(w, h, 0);

	     glTexCoord2i(1, 0);
	     glVertex3i(w, 0, 0);
	    glEnd();

	   glPopMatrix();
	   glMatrixMode(GL_PROJECTION);
	  glPopMatrix();
	  glMatrixMode(GL_MODELVIEW);
	} /* if */

      glDisable(GL_BLEND);
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_TEXTURE_2D);
    } /* if */

 return;
} /* ay_draw_bgimage */


/* ay_draw_needredraw:
 *  For two lists of objects in the scene, decide whether a redraw
 *  is necessary when the objects in oldsel are deselected and the
 *  objects in newsel are selected (if e.g. oldsel contains a material
 *  and newsel a hidden object, no redraw is necessary, because nothing
 *  would change in any view anyway).
 *  Both lists may be empty.
 */
void
ay_draw_needredraw(ay_list_object *oldsel, ay_list_object *newsel,
		   int *result)
{
 int oldseldrawn = AY_FALSE, newseldrawn = AY_FALSE;
 ay_object *o = NULL;
 ay_list_object *s = NULL, *os = NULL, *ns = NULL;

  *result = AY_TRUE;

  /* check, whether any objects in old selection were actually drawn */
  s = oldsel;
  while(s)
    {
      o = s->object;
      if(o->type != AY_IDMATERIAL)
	{
	  if(!o->hide)
	    {
	      oldseldrawn = AY_TRUE;
	    }
	}
      s = s->next;
    }

  /* check, whether any objects in new selection will actually be drawn */
  s = newsel;
  while(s)
    {
      o = s->object;
      if(o->type != AY_IDMATERIAL)
	{
	  if(!o->hide)
	    {
	      newseldrawn = AY_TRUE;
	    }
	}
      s = s->next;
    }

  /* if oldsel was invisible and newsel too => we need no redraw */
  if(!oldseldrawn && !newseldrawn)
    {
      *result = AY_FALSE;
      return;
    }

  /* check for identical selections */
  if(oldsel && newsel)
    {
      if(oldsel->next || newsel->next)
	{
	  /* check multiple object selections */
	  os = oldsel;
	  ns = newsel;
	  /* XXXX we would rather need to find matching objects
	     for all objects of <os> in <ns> and check for objects
	     without matches here, but this simple check should
	     be sufficient this time */
	  while(os && ns)
	    {
	      /* compare objects */
	      if(os->object != ns->object)
		{
		  return;
		}
	      os = os->next;
	      ns = ns->next;
	      /* check for premature end of one of both lists */
	      if(((!os) && ns) || (os && (!ns)))
		{
		  return;
		}
	    } /* while */
	  *result = AY_FALSE;
	}
      else
	{
	  /* check single object selections */
	  if(oldsel->object == newsel->object)
	    {
	      *result = AY_FALSE;
	      return;
	    }
	} /* if */
    } /* if */

 return;
} /* ay_draw_needredraw */


/* ay_draw_trimview:
 *  draw a view of type Trim by drawing the
 *  bounds of parametric space of current NURBS patch as a
 *  stippled rectangle
 */
void
ay_draw_trimview(void)
{
 int have_bounds;
 double bounds[4] = {0};

  have_bounds = ay_clevel_getparentbounds(bounds);

  /* draw the bounds of the parametric space of the patch as rectangle */
  if(have_bounds)
    {
      glEnable(GL_LINE_STIPPLE);

      glColor3f((GLfloat)ay_prefs.obr, (GLfloat)ay_prefs.obg,
		(GLfloat)ay_prefs.obb);

      glLineStipple((GLint)3, (GLushort)0x5555);

      glBegin(GL_LINE_LOOP);
       glVertex2f((GLfloat)bounds[0], (GLfloat)bounds[1]);
       glVertex2f((GLfloat)bounds[2], (GLfloat)bounds[1]);
       glVertex2f((GLfloat)bounds[2], (GLfloat)bounds[3]);
       glVertex2f((GLfloat)bounds[0], (GLfloat)bounds[3]);
      glEnd();

      glDisable(GL_LINE_STIPPLE);
    } /* if */

 return;
} /* ay_draw_trimview */


/* ay_draw_rectangle:
 *
 */
void
ay_draw_rectangle(int winwidth, int winheight,
		  double xmin, double ymin, double xmax, double ymax)
{

  glDisable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
   glLoadIdentity();
   glOrtho(0, winwidth, 0, winheight, -100.0, 100.0);
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
    glLoadIdentity();
    glBegin(GL_LINE_LOOP);
     glVertex3d(xmin, winheight-ymin, 0.0);
     glVertex3d(xmax, winheight-ymin, 0.0);
     glVertex3d(xmax, winheight-ymax, 0.0);
     glVertex3d(xmin, winheight-ymax, 0.0);
    glEnd();
   glPopMatrix();
   glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glEnable(GL_DEPTH_TEST);

 return;
} /* ay_draw_rectangle */


/* ay_draw_cs:
 *  draw an object coordinate system consisting of three color
 *  coded and labelled axes
 */
void
ay_draw_cs(struct Togl *togl, int mode)
{
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 GLdouble mvm[16], pm[16];
 GLdouble win1x, win1y, win1z, win2x, win2y, win2z, win3x, win3y, win3z;
 GLfloat s = 6.0f, color[4] = {0.0f,0.0f,0.0f,0.0f};
 GLint vp[4];
 int t = 3, e = 4;

  if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
    {
      s *= ay_prefs.guiscale;
      t *= ay_prefs.guiscale;
      e *= ay_prefs.guiscale;
    }

 if(mode == 0)
   {
     glGetFloatv(GL_CURRENT_COLOR, color);

     /* draw */
     glBegin(GL_LINES);
      glColor3f((GLfloat)1.0f, (GLfloat)0.0f, (GLfloat)0.0f);
      glVertex3f((GLfloat)0.0f, (GLfloat)0.0f, (GLfloat)0.0f);
      glVertex3f((GLfloat)1.0f, (GLfloat)0.0f, (GLfloat)0.0f);
      glColor3f((GLfloat)0.0f, (GLfloat)1.0f, (GLfloat)0.0f);
      glVertex3f((GLfloat)0.0f, (GLfloat)0.0f, (GLfloat)0.0f);
      glVertex3f((GLfloat)0.0f, (GLfloat)1.0f, (GLfloat)0.0f);
      glColor3f((GLfloat)0.0f, (GLfloat)0.0f, (GLfloat)1.0f);
      glVertex3f((GLfloat)0.0f, (GLfloat)0.0f, (GLfloat)0.0f);
      glVertex3f((GLfloat)0.0f, (GLfloat)0.0f, (GLfloat)1.0f);
     glEnd();

     glColor4fv(color);
   }
 else
   {
     if(view->antialiaslines)
       {
	 e -= ay_prefs.aafudge;
       }

     glGetDoublev(GL_MODELVIEW_MATRIX, mvm);
     glGetDoublev(GL_PROJECTION_MATRIX, pm);
     glGetIntegerv(GL_VIEWPORT, vp);
     if(GL_FALSE == gluProject((GLdouble)1.0, (GLdouble)0.0,
			       (GLdouble)0.0, mvm, pm, vp,
			       &win1x, &win1y, &win1z))
       {
	 return;
       }
     if(GL_FALSE == gluProject((GLdouble)0.0, (GLdouble)1.0,
			       (GLdouble)0.0, mvm, pm, vp,
			       &win2x, &win2y, &win2z))
       {
	 return;
       }
     if(GL_FALSE == gluProject((GLdouble)0.0, (GLdouble)0.0,
			       (GLdouble)1.0, mvm, pm, vp,
			       &win3x, &win3y, &win3z))
       {
	 return;
       }

     glMatrixMode(GL_PROJECTION);
     glPushMatrix();
      glLoadIdentity();
      glOrtho(0, Togl_Width(togl), 0, Togl_Height(togl), -100.0, 100.0);

      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
       glLoadIdentity();
       glTranslatef(((int)win1x)+s+0.375f, ((int)win1y)+0.375f, 0.0f);
       /* draw X */
       glBegin(GL_LINES);
        glVertex2i(-t, -t);
	glVertex2i(e, e);
	glVertex2i(-t, t);
	glVertex2i(e, -e);
       glEnd();
       glLoadIdentity();
       glTranslatef(((int)win2x)+s+0.375f, ((int)win2y)+0.375f, 0.0f);
       /* draw Y */
       glBegin(GL_LINES);
        glVertex2i(-t, t);
	glVertex2i(0, 0);
       glEnd();
       glBegin(GL_LINE_STRIP);
        glVertex2i(t, t);
	glVertex2i(0, 0);
	glVertex2i(0, -e);
       glEnd();
       glLoadIdentity();
       glTranslatef(((int)win3x)+s+0.375f, ((int)win3y)+0.375f, 0.0f);
       /* draw Z */
       glBegin(GL_LINE_STRIP);
        glVertex2i(-t, t);
	glVertex2i(t, t);
	glVertex2i(-t, -t);
	glVertex2i(e, -t);
       glEnd();
       glPopMatrix();
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
     glMatrixMode(GL_MODELVIEW);
   } /* if */

 return;
} /* ay_draw_cs */


/* ay_draw_mark:
 *  draw marked point in space as simple red '+' symbol
 */
void
ay_draw_mark(struct Togl *togl)
{
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 int s = (int)(ay_prefs.handle_size*1.75/2.0);

  if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
    s *= ay_prefs.guiscale;

  glColor3f((GLfloat)ay_prefs.tpr,
	    (GLfloat)ay_prefs.tpg,
	    (GLfloat)ay_prefs.tpb);
  if(view->antialiaslines)
    {
      if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	glLineWidth((GLfloat)(ay_prefs.aasellinewidth * ay_prefs.guiscale));
      else
	glLineWidth((GLfloat)ay_prefs.aasellinewidth);
    }
  else
    {
      if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	glLineWidth((GLfloat)(ay_prefs.sellinewidth * 2.0f *
			      ay_prefs.guiscale));
      else
	glLineWidth((GLfloat)(ay_prefs.sellinewidth * 2.0f));
    }

  glDisable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
   glLoadIdentity();
   glOrtho(0, Togl_Width(togl), 0, Togl_Height(togl), -100.0, 100.0);
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
    glLoadIdentity();

    if(view->antialiaslines)
      glTranslatef((float)(floor(view->markx+0.5)),
		   (float)(floor(Togl_Height(togl)-view->marky+0.5)),
		   0.0f);
    else
      glTranslatef((float)(floor(view->markx+0.5)-0.375f),
		   (float)(floor(Togl_Height(togl)-view->marky+0.5)-0.375f),
		   0.0f);

    glBegin(GL_LINES);
     if(view->antialiaslines)
       {
	 glVertex2f((GLfloat)-s, 0.0f);
	 glVertex2f((GLfloat)s/*-ay_prefs.aafudge*/, 0.0f);
	 glVertex2f(0.0f, (GLfloat)-s);
	 glVertex2f(0.0f, (GLfloat)s/*-ay_prefs.aafudge*/);
       }
     else
       {
	 /* XXXX employ ay_prefs.useguiscale */
	 if(ay_prefs.sellinewidth*2.0 >= 2.0)
	   glVertex2i(-(s-2), 0);
	 else
	   glVertex2i(-(s-1), 0);
	 glVertex2i(s, 0);
	 if(ay_prefs.sellinewidth*2.0 >= 2.0)
	   glVertex2i(0, -(s-2));
	 else
	   glVertex2i(0, -(s-1));
	 glVertex2i(0, s);
       }
    glEnd();
   glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glEnable(GL_DEPTH_TEST);

 return;
} /* ay_draw_mark */


/* ay_draw_registerdacb:
 *  register the draw annotation callback dacb for
 *  objects of type type_id
 */
int
ay_draw_registerdacb(ay_drawcb *dacb, unsigned int type_id)
{
 int ay_status = AY_OK;

  /* register draw annotation callback */
  ay_status = ay_table_addcallback(&ay_drawacbt, (ay_voidfp)dacb, type_id);

 return ay_status;
} /* ay_draw_registerdacb */


/** ay_draw_silhouettes:
 * Draw the silhouettes of some objects, that are delivered in a
 * silhouette image as mostly transparent quad.
 *
 * \param[in] togl  view to draw
 * \param[in] silimg  silhouette image, should be of views size
 */
void
ay_draw_silhouettes(struct Togl *togl, unsigned char *silimg)
{
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 int i, w, h;
 float ex, ey;
 GLuint texid;

  w = Togl_Width(togl);
  h = Togl_Height(togl);

  /* draw silhouette texture */
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  if(view->antialiaslines)
    {
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
  else
    {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
  glGenTextures(1, &texid);
  glBindTexture(GL_TEXTURE_2D, texid);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
	       silimg);

  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0, (GLdouble) w, 0.0, (GLdouble) h, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
   glLoadIdentity();
   glBegin(GL_QUADS);
    if(view->antialiaslines)
      {
	for(i = 0; i < 4; i++)
	  {
	    switch(i)
	      {
	      case 0:
		ex = -0.1f;
		ey = 0.1f;
		break;
	      case 1:
		ex = 0.1f;
		ey = 0.1f;
		break;
	      case 2:
		ex = 0.1f;
		ey = -0.1f;
		break;
	      case 3:
		ex = -0.1f;
		ey = -0.1f;
		break;
	      }
	    ex /= w;
	    ey /= h;
	    glTexCoord2f(0+ex, 0+ey);
	    glVertex3i(0, 0, 0);

	    glTexCoord2f(0+ex, 1+ey);
	    glVertex3i(0, h, 0);

	    glTexCoord2f(1+ex, 1+ey);
	    glVertex3i(w, h, 0);

	    glTexCoord2f(1+ex, 0+ey);
	    glVertex3i(w, 0, 0);
	  } /* for each pass */
      }
    else
      {
	glTexCoord2i(0, 0);
	glVertex3i(0, 0, 0);

	glTexCoord2i(0, 1);
	glVertex3i(0, h, 0);

	glTexCoord2i(1, 1);
	glVertex3i(w, h, 0);

	glTexCoord2i(1, 0);
	glVertex3i(w, 0, 0);
      } /* if antialias */
   glEnd();
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_TEXTURE_2D);

  glDeleteTextures(1, &texid);

 return;
} /* ay_draw_silhouettes */


/** ay_draw_selmp:
 * Draw selected multiple points.
 *
 * \param[in] o  object that has multiple points
 * \param[in] is_rat  are the points rational?
 * \param[in] mp  the multiple points to draw
 */
void
ay_draw_selmp(ay_object *o, int is_rat, ay_mpoint *mp)
{
  /*double m[16];*/
 double w, *pnts;

  /* set color for selected points */
  glColor3f((GLfloat)ay_prefs.tpr, (GLfloat)ay_prefs.tpg,
	    (GLfloat)ay_prefs.tpb);
  glDepthFunc(GL_ALWAYS);

  if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
    glPointSize((GLfloat)(ay_prefs.handle_size * 1.4 * ay_prefs.guiscale));
  else
    glPointSize((GLfloat)(ay_prefs.handle_size * 1.4));
  /*
  glPushMatrix();
   glTranslated((GLdouble)o->movx, (GLdouble)o->movy,
		(GLdouble)o->movz);
   ay_quat_torotmatrix(o->quat, m);
   glMultMatrixd((GLdouble*)m);
   glScaled((GLdouble)o->scalx, (GLdouble)o->scaly,
	    (GLdouble)o->scalz);
  */
  glBegin(GL_POINTS);
   while(mp)
     {
       if(mp->selected)
	 {
	   if(is_rat && ay_prefs.rationalpoints)
	     {
	       pnts = mp->points[0];
	       w = pnts[3];
	       glVertex3d((GLdouble)(pnts[0]*w),
			  (GLdouble)(pnts[1]*w),
			  (GLdouble)(pnts[2]*w));
	     }
	   else
	     glVertex3dv((GLdouble*)mp->points[0]);
	 }
       mp = mp->next;
     }
  glEnd();
  /*
  glPopMatrix();
  */
  if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
    glPointSize((GLfloat)(ay_prefs.handle_size * ay_prefs.guiscale));
  else
    glPointSize((GLfloat)ay_prefs.handle_size);
  glDepthFunc(GL_LESS);

  /* set color for selected objects */
  glColor3f((GLfloat)ay_prefs.ser,
	    (GLfloat)ay_prefs.seg,
	    (GLfloat)ay_prefs.seb);

 return;
} /* ay_draw_selmp */


/** ay_draw_rhombus:
 * Helper function to draw a rhombus (knot, break point).
 *
 * \param[in] pnt  3D coordinates of rhombus
 */
void
ay_draw_rhombus(double *pnt)
{
 double c[3], dx[3], dy[3];
 GLdouble mvm[16], pm[16], s;
 GLint vp[4];

  s = ay_prefs.handle_size*0.75;

  if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
    s *= ay_prefs.guiscale;

  glGetDoublev(GL_MODELVIEW_MATRIX, mvm);
  glGetDoublev(GL_PROJECTION_MATRIX, pm);
  glGetIntegerv(GL_VIEWPORT, vp);

  gluProject(0, 0, 0, mvm, pm, vp, &c[0], &c[1], &c[2]);

  gluUnProject(c[0]+s, c[1], c[2], mvm, pm, vp,
	       &dx[0], &dx[1], &dx[2]);
  gluUnProject(c[0], c[1]+s, c[2], mvm, pm, vp,
	       &dy[0], &dy[1], &dy[2]);

  glBegin(GL_QUADS);
   glVertex3d(pnt[0]+dx[0], pnt[1]+dx[1], pnt[2]+dx[2]);
   glVertex3d(pnt[0]+dy[0], pnt[1]+dy[1], pnt[2]+dy[2]);
   glVertex3d(pnt[0]-dx[0], pnt[1]-dx[1], pnt[2]-dx[2]);
   glVertex3d(pnt[0]-dy[0], pnt[1]-dy[1], pnt[2]-dy[2]);
  glEnd();

 return;
} /* ay_draw_rhombus */


/** ay_draw_linestrip:
 * Helper function to draw a 2D line strip.
 *
 * \param[in] togl  view to draw into
 * \param[in] n  number of points in the strip
 * \param[in] stride  distance between points in \a cv
 * \param[in] cv  control points [(n+1)*stride]
 */
void
ay_draw_linestrip(struct Togl *togl, int n, int stride, double *cv)
{
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);
 int i;
 double *p;

  glDisable(GL_DEPTH_TEST);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
   glLoadIdentity();
   glOrtho(0, Togl_Width(togl), 0, Togl_Height(togl), -100.0, 100.0);

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
    glLoadIdentity();
    if(!view->antialiaslines)
      glTranslated(0.375, 0.375, 0.0);
    if(view->antialiaslines)
      {
	if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	  glLineWidth((GLfloat)(ay_prefs.aasellinewidth * 2.0f *
				ay_prefs.guiscale));
	else
	  glLineWidth((GLfloat)ay_prefs.aasellinewidth * 2.0f);
      }
    else
      {
	if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	  glLineWidth((GLfloat)(ay_prefs.sellinewidth * ay_prefs.guiscale));
	else
	  glLineWidth((GLfloat)ay_prefs.sellinewidth * 4.0f);
      }
    glColor3f((GLfloat)ay_prefs.bgr,
	      (GLfloat)ay_prefs.bgg,
	      (GLfloat)ay_prefs.bgb);
    glBegin(GL_LINE_STRIP);
     p = cv;
     glVertex2dv(p);
     p += stride;
     for(i = 0; i < n; i++)
       {
	 glVertex2dv(p);
	 p += stride;
       }
    glEnd();

    if(view->antialiaslines)
      {
	if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	  glLineWidth((GLfloat)(ay_prefs.aasellinewidth * ay_prefs.guiscale));
	else
	  glLineWidth((GLfloat)ay_prefs.aasellinewidth);
      }
    else
      {
	if(ay_prefs.useguiscale && ay_prefs.guiscale > 1.0)
	  glLineWidth((GLfloat)(ay_prefs.sellinewidth * 2.0f *
				ay_prefs.guiscale));
	else
	  glLineWidth((GLfloat)ay_prefs.sellinewidth * 2.0f);
      }
    glColor3f((GLfloat)ay_prefs.ser,
	      (GLfloat)ay_prefs.seg,
	      (GLfloat)ay_prefs.seb);

    glBegin(GL_LINE_STRIP);
     p = cv;
     glVertex2dv(p);
     p += stride;
     for(i = 0; i < n; i++)
       {
	 glVertex2dv(p);
	 p += stride;
       }
    glEnd();

    glPopMatrix();
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  glEnable(GL_DEPTH_TEST);

 return;
} /* ay_draw_linestrip */
