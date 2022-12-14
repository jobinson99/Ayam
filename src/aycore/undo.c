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

/* undo.c - undo/redo related functions */

/*
#define UNDO_DBG 1
*/

/* types local to this module */
typedef struct ay_undo_object_s
{
  char *operation;
  ay_object *objects;
  ay_list_object *references;
  int saved_children;
} ay_undo_object;

/* prototypes of functions local to this module */
void ay_undo_deletemulti(ay_object *o);

void ay_undo_clearuo(ay_undo_object *uo);

int ay_undo_copymat(ay_mat_object *src, ay_mat_object *dst);

int ay_undo_copyview(ay_view_object *src, ay_view_object *dst);

int ay_undo_copyroot(ay_root_object *src, ay_root_object *dst);

int ay_undo_copyselp(ay_object *src, ay_object *dst);

int ay_undo_copytags(ay_object *src, ay_object *dst);

int ay_undo_copy(ay_undo_object *uo);

int ay_undo_redo(void);

int ay_undo_undo(void);

int ay_undo_rewind(int silent);

int ay_undo_savechildren(ay_object *o, ay_undo_object *uo,
			 ay_list_object **lastr,
			 ay_object ***nexto);

int ay_undo_save(int save_children);

int ay_undo_copysave(ay_object *src, ay_object **dst);

/* global variables */
static ay_undo_object *undo_buffer;

static int undo_current;

static int undo_buffer_size;

static int undo_last_op; /* last undo operation: -1: no op,
			    1-4: see mode variable in ay_undo_undotcmd() */

static char *undo_saved_op; /* name of saved modelling operation */

/* functions */

/* ay_undo_init:
 *  initialize the undo system by allocating the undo buffer
 */
int
ay_undo_init(int buffer_size)
{

  if(undo_buffer)
    {
      free(undo_buffer);
    }
  undo_buffer = NULL;

  if(buffer_size <= 0)
    {
      undo_buffer_size = -1;
      return AY_OK;
    }

  buffer_size++;

  if(!(undo_buffer = calloc(buffer_size, sizeof(ay_undo_object))))
    {
      return AY_EOMEM;
    }

  undo_current = -1;
  undo_buffer_size = buffer_size;

  undo_last_op = -1; /* no op */

  if(undo_saved_op)
    {
      free(undo_saved_op);
    }
  undo_saved_op = NULL;

 return AY_OK;
} /* ay_undo_init */


/* ay_undo_deletemulti:
 *  delete some connected objects
 */
void
ay_undo_deletemulti(ay_object *o)
{
 ay_object *next = NULL, *d = NULL;
 ay_view_object *view = NULL;
 ay_mat_object *material = NULL;
 ay_deletecb *dcb = NULL;
 ay_voidfp *arr = NULL;

  if(!o)
    return;

  d = o;
  while(d)
    {
      next = d->next;
      switch(d->type)
	{
	case AY_IDVIEW:
	  view = (ay_view_object *)d->refine;
	  if(view->bgimage)
	    free(view->bgimage);
	  free(view);
	  break;
	case AY_IDMATERIAL:
	  material = (ay_mat_object *)(d->refine);
	  ay_matt_clearshaders(material);
	  free(material);
	  break;
	case AY_IDINSTANCE:
	case AY_IDLAST:
	  break;
	default:
	  arr = ay_deletecbt.arr;
	  dcb = (ay_deletecb*)(arr[d->type]);
	  if(dcb)
	    (void)dcb(d->refine);
	  break;
	} /* switch */

      /* delete selected points */
      if(d->selp)
	ay_selp_clear(d);

      /* delete tags */
      if(d->tags)
	ay_tags_delall(d);

      /* free name */
      if(d->name)
	free(d->name);

      /* free generic object */
      free(d);

      d = next;
    } /* while */

 return;
} /* ay_undo_deletemulti */


/* ay_undo_clearuo:
 *  clear a single undo object
 */
void
ay_undo_clearuo(ay_undo_object *uo)
{
 ay_list_object *lo = NULL;

  while(uo->references)
    {
      lo = uo->references->next;
      free(uo->references);
      uo->references = lo;
    }

  if(uo->objects)
    {
      ay_undo_deletemulti(uo->objects);
    }
  uo->objects = NULL;

  if(uo->operation)
    {
      free(uo->operation);
    }
  uo->operation = NULL;

 return;
} /* ay_undo_clearuo */


/* ay_undo_copymat:
 */
int
ay_undo_copymat(ay_mat_object *src, ay_mat_object *dst)
{
 int ay_status = AY_OK;
 char **onameptr;
 unsigned int *orefcountptr;
 ay_object *oobjptr;
 int oregistered;

  ay_matt_clearshaders(dst);

  oregistered = dst->registered;
  onameptr = dst->nameptr;
  orefcountptr = dst->refcountptr;
  oobjptr = dst->objptr;

  memcpy(dst, src, sizeof(ay_mat_object));

  dst->registered = oregistered;
  dst->nameptr = onameptr;
  dst->refcountptr = orefcountptr;
  dst->objptr = oobjptr;

  if(src->sshader)
    {
      dst->sshader = NULL;
      ay_status += ay_shader_copy(src->sshader, &(dst->sshader));
    }

  if(src->dshader)
    {
      dst->dshader = NULL;
      ay_status += ay_shader_copy(src->dshader, &(dst->dshader));
    }

  if(src->ishader)
    {
      dst->ishader = NULL;
      ay_status += ay_shader_copy(src->ishader, &(dst->ishader));
    }

  if(src->eshader)
    {
      dst->eshader = NULL;
      ay_status += ay_shader_copy(src->eshader, &(dst->eshader));
    }

  if(ay_status)
    {
      ay_status = AY_ERROR;
    }

 return ay_status;
} /* ay_undo_copymat */


/* ay_undo_copyview:
 */
int
ay_undo_copyview(ay_view_object *src, ay_view_object *dst)
{
 struct Togl *otogl;

  otogl = dst->togl;

  memcpy(dst, src, sizeof(ay_view_object));

  dst->togl = otogl;
  dst->dirty = AY_TRUE;
  dst->bgimage = NULL;
  dst->rect_xmin = 0.0;
  dst->rect_xmax = 0.0;
  dst->rect_ymin = 0.0;
  dst->rect_ymax = 0.0;
  dst->drawrect = 0;

  /* copy BGImage */
  if(src->bgimage)
    {
      if(!(dst->bgimage = malloc((strlen(src->bgimage)+1) * sizeof(char))))
	{
	  return AY_EOMEM;
	}
      strcpy(dst->bgimage, src->bgimage);
    }

 return AY_OK;
} /* ay_undo_copyview */


/* ay_undo_copyroot:
 */
int
ay_undo_copyroot(ay_root_object *src, ay_root_object *dst)
{
 int ay_status = AY_OK;
 ay_riopt *srcriopt = NULL, *dstriopt = NULL;

  if(dst->imager)
    {
      ay_shader_free(dst->imager);
      dst->imager = NULL;
    }

  if(src->imager)
    {
      ay_status = ay_shader_copy(src->imager, &(dst->imager));
      if(ay_status)
	return ay_status;
    }

  if(dst->atmosphere)
    {
      ay_shader_free(dst->atmosphere);
      dst->atmosphere = NULL;
    }

  if(src->atmosphere)
    {
      ay_status = ay_shader_copy(src->atmosphere, &(dst->atmosphere));
      if(ay_status)
	return ay_status;
    }

  /* copy RiOptions */
  srcriopt = src->riopt;
  dstriopt = dst->riopt;
  if(dstriopt)
    {
      if(dstriopt->textures)
	{
	  free(dstriopt->textures);
	  dstriopt->textures = NULL;
	}
      if(dstriopt->shaders)
	{
	  free(dstriopt->shaders);
	  dstriopt->shaders = NULL;
	}
      if(dstriopt->archives)
	{
	  free(dstriopt->archives);
	  dstriopt->archives = NULL;
	}
      if(dstriopt->procedurals)
	{
	  free(dstriopt->procedurals);
	  dstriopt->procedurals = NULL;
	}
      free(dst->riopt);
    } /* if */

  if(!(dst->riopt = malloc(sizeof(ay_riopt))))
    return AY_EOMEM;

  memcpy(dst->riopt, src->riopt, sizeof(ay_riopt));
  dstriopt = dst->riopt;
  dstriopt->textures = NULL;
  dstriopt->shaders = NULL;
  dstriopt->archives = NULL;
  dstriopt->procedurals = NULL;

  if(srcriopt->textures)
    {
      if(!(dstriopt->textures = malloc((strlen(srcriopt->textures)+1)*
				       sizeof(char))))
	return AY_EOMEM;
      strcpy(dstriopt->textures, srcriopt->textures);
    }

  if(srcriopt->shaders)
    {
      if(!(dstriopt->shaders = malloc((strlen(srcriopt->shaders)+1)*
				      sizeof(char))))
	return AY_EOMEM;
      strcpy(dstriopt->shaders, srcriopt->shaders);
    }

  if(srcriopt->archives)
    {
      if(!(dstriopt->archives = malloc((strlen(srcriopt->archives)+1)*
				       sizeof(char))))
	return AY_EOMEM;
      strcpy(dstriopt->archives, srcriopt->archives);
    }

  if(srcriopt->procedurals)
    {
      if(!(dstriopt->procedurals = malloc((strlen(srcriopt->procedurals)+1)*
					  sizeof(char))))
	return AY_EOMEM;
      strcpy(dstriopt->procedurals, srcriopt->procedurals);
    }

 return AY_OK;
} /* ay_undo_copyroot */


/* ay_undo_copyselp:
 */
int
ay_undo_copyselp(ay_object *src, ay_object *dst)
{
 ay_point *p = NULL, *n = NULL, **last = NULL;

  if(dst->selp)
    {
      ay_selp_clear(dst);
    }

  p = src->selp;
  last = &(dst->selp);
  while(p)
    {
      if(!(n = malloc(sizeof(ay_point))))
	return AY_EOMEM;

      memcpy(n, p, sizeof(ay_point));
      *last = n;
      last = &(n->next);

      p = p->next;
    } /* while */

 return AY_OK;
} /* ay_undo_copyselp */


/* ay_undo_copytags:
 *  copy all tags from object <src> to object <dst>
 *  including internal and binary tags
 */
int
ay_undo_copytags(ay_object *src, ay_object *dst)
{
 int ay_status = AY_OK;
 ay_tag *tag = NULL, **newtagptr = NULL;

  if(!src || !dst)
    return AY_ENULL;

  tag = src->tags;
  newtagptr = &(dst->tags);
  while(tag)
    {
      ay_status = ay_tags_copy(tag, newtagptr);
      if(ay_status == AY_OK)
	{
	  newtagptr = &((*newtagptr)->next);
	  *newtagptr = NULL;
	}
      else
	{
	  return ay_status;
	}
      tag = tag->next;
    }

 return AY_OK;
} /* ay_undo_copytags */


/* ay_undo_copy:
 *  copy objects from undo object <uo> back to the scene
 *  (in-place) using references information
 */
int
ay_undo_copy(ay_undo_object *uo)
{
 int ay_status = AY_OK;
 ay_object *o = NULL, *c = NULL, *m = NULL;
 ay_list_object *r = NULL;
 ay_voidfp *arr = NULL;
 ay_deletecb *dcb = NULL;
 ay_copycb *ccb = NULL;
 char view_repairtitle_cmd[] = "viewRepairTitle ", buf[64];
 char view_setgridicon_cmd[] = "viewSetGridIcon ";
 char view_setdmodeicon_cmd[] = "viewSetDModeIcon ";
 Tcl_DString ds;
 int find_parent = AY_TRUE, notify_parent = AY_FALSE;
 ay_object *parent = NULL;
 Tk_Window win;
 char *winpath = NULL;

  if(!uo)
    return AY_OK;

  r = uo->references;
  c = uo->objects;
  while(r)
    {
      o = r->object;

      /* skip over objects without references (e.g. AY_IDLAST separator) */
      if(!o)
	{
	  r = r->next;
	  c = c->next;
	  continue;
	}

      /* copy name */
      if(o->type != AY_IDMATERIAL)
	{
	  if(o->name)
	    {
	      free(o->name);
	      o->name = NULL;
	    }
	  if(c->name)
	    {
	      if(!(o->name = malloc((strlen(c->name)+1) * sizeof(char))))
		return AY_EOMEM;
	      strcpy(o->name, c->name);
	    }
	} /* if */

      /* copy tags */
      if(o->tags)
	{
	  ay_tags_delall(o);
	}
      ay_status = ay_undo_copytags(c, o);
      if(ay_status)
	{
	  return ay_status;
	}

      /* copy type specific part */
      switch(c->type)
	{
	case AY_IDVIEW:
	  ay_status = ay_undo_copyview((ay_view_object *)(c->refine),
				       (ay_view_object *)(o->refine));

	  if(ay_status)
	    {
	      return ay_status;
	    }

	  /* repair title of view window */
	  Tcl_DStringInit(&ds);
	  Tcl_DStringAppend(&ds, view_repairtitle_cmd, -1);
	  Tcl_DStringAppend(&ds, o->name, -1);
	  sprintf(buf, " %d", ((ay_view_object *)(o->refine))->type);
	  Tcl_DStringAppend(&ds, buf, -1);
	  Tcl_Eval(ay_interp, Tcl_DStringValue(&ds));
	  Tcl_DStringFree(&ds);

	  /* get path name of view widget associated with the view */
	  win = Togl_TkWin(((ay_view_object *)(o->refine))->togl);
	  /* win == .view1.f3D.togl */
	  win = Tk_Parent(win);
	  /* win == .view1.f3D */
	  win = Tk_Parent(win);
	  /* win == .view1 */
	  winpath = Tk_PathName(win);

	  /* set grid icon of view window */
	  Tcl_DStringInit(&ds);
	  Tcl_DStringAppend(&ds, view_setgridicon_cmd, -1);
	  Tcl_DStringAppend(&ds, winpath, -1);
	  memset(buf, 0, sizeof(buf));
	  sprintf(buf, " %g", ((ay_view_object *)(o->refine))->grid);
	  Tcl_DStringAppend(&ds, buf, -1);
	  Tcl_Eval(ay_interp, Tcl_DStringValue(&ds));
	  Tcl_DStringFree(&ds);

	  /* set drawing mode icon of view window */
	  Tcl_DStringInit(&ds);
	  Tcl_DStringAppend(&ds, view_setdmodeicon_cmd, -1);
	  Tcl_DStringAppend(&ds, winpath, -1);
	  memset(buf, 0, sizeof(buf));
	  sprintf(buf, " %d", ((ay_view_object *)(o->refine))->drawmode);
	  Tcl_DStringAppend(&ds, buf, -1);
	  Tcl_Eval(ay_interp, Tcl_DStringValue(&ds));
	  Tcl_DStringFree(&ds);

	  break;
	case AY_IDROOT:
	  ay_status = ay_undo_copyroot((ay_root_object *)(c->refine),
				       (ay_root_object *)(o->refine));
	  if(ay_status)
	    {
	      return ay_status;
	    }
	  break;
	case AY_IDINSTANCE:
	  o->refine = c->refine;
	  break;
	case AY_IDMATERIAL:
	  if(o->name)
	    {
	      (void)ay_matt_deregister(o->name);
	      free(o->name);
	      o->name = NULL;
	    }
	  if(c->name)
	    {
	      (void)ay_matt_registermaterial(c->name,
				       (ay_mat_object *)(o->refine));
	      if(!(o->name = malloc((strlen(c->name)+1) * sizeof(char))))
		{
		  return AY_EOMEM;
		}
	      strcpy(o->name, c->name);
	    }

	  ay_status = ay_undo_copymat((ay_mat_object *)(c->refine),
				      (ay_mat_object *)(o->refine));
	  if(ay_status)
	    {
	      return ay_status;
	    }
	  break;
	default:
	  arr = ay_deletecbt.arr;
	  dcb = (ay_deletecb*)(arr[o->type]);
	  if(dcb)
	    (void)dcb(o->refine);

	  arr = ay_copycbt.arr;
	  ccb = (ay_copycb*)(arr[c->type]);
	  if(ccb)
	    {
	      ay_status = ccb(c->refine, &(o->refine));
	      if(ay_status)
		{
		  return ay_status;
		}
	    }
	  break;
	} /* switch */

      /* copy trafos */
      ay_trafo_copy(c, o);

      if(o->type != AY_IDMATERIAL)
	{
	  /* copy material */
	  if(!o->mat && c->mat)
	    {
	      m = c->mat->objptr;
	      m->refcount++;
	    }
	  if(o->mat && !c->mat)
	    {
	      m = o->mat->objptr;
	      m->refcount--;
	    }
	  if(o->mat && c->mat)
	    {
	      m = o->mat->objptr;
	      m->refcount--;
	      m = c->mat->objptr;
	      m->refcount++;
	    }

	  o->mat = c->mat;
	} /* if */

      /* copy selected points */
      if(c->selp)
	{
	  (void)ay_undo_copyselp(c, o);
	  (void)ay_pact_getpoint(3, o, NULL, NULL);
	}

      /* copy all other attributes */
      o->type = c->type;
      /*o->selected = c->selected;*/
      o->modified = AY_TRUE; /*c->modified;*/
      o->parent = c->parent;
      o->inherit_trafos = c->inherit_trafos;
      o->hide = c->hide;
      o->hide_children = c->hide_children;

      if((c->type != AY_IDVIEW) && (c->type != AY_IDROOT))
	{
	  (void)ay_notify_object(o);
	  if(find_parent)
	    {
	      notify_parent = AY_TRUE;
	      parent = o;
	      find_parent = AY_FALSE;
	    }
	} /* if */

      c = c->next;
      r = r->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parentof(parent, AY_TRUE);

 return AY_OK;
} /* ay_undo_copy */


/* ay_undo_redo:
 *
 */
int
ay_undo_redo(void)
{
 char fname[] = "redo";
 int ay_status = AY_OK;

  if(undo_current == (undo_buffer_size-1))
    {
      ay_error(AY_ERROR, fname, "No further redo info available!");
      return AY_ERROR;
    }

  if((undo_buffer[undo_current+1]).references == NULL)
    {
      ay_error(AY_ERROR, fname, "No further redo info available!");
      return AY_ERROR;
    }

  if(undo_last_op == 0)
    { /* if last op was an undo, we jump an extra step */
      undo_current++;
    }

  undo_current++;

  ay_status = ay_undo_copy(&(undo_buffer[undo_current]));

  undo_last_op = 1;

 return ay_status;
} /* ay_undo_redo */


/* ay_undo_undo:
 *
 */
int
ay_undo_undo(void)
{
 char fname[] = "undo";
 int ay_status = AY_OK;

  if(undo_current < 0)
    {
      ay_error(AY_ERROR, fname, "No further undo info available!");
      return AY_ERROR;
    }

  if(undo_last_op == 2)
    { /* if last op was a save, we need to save current state too,
         to allow the user to get back to current state with redo */
      ay_status = ay_undo_save((undo_buffer[undo_current]).saved_children);
      if(ay_status)
	ay_error(AY_EWARN, fname,
		 "Undo save failed, the state before 'undo' will be lost!");
      undo_current--;
    }

  if(undo_last_op == 1)
    { /* if last operation was a redo, we jump an extra step */
      undo_current--;
    }

  ay_status = ay_undo_copy(&(undo_buffer[undo_current]));

  undo_current--;

  if(undo_current < 0)
    undo_current = -1;

  undo_last_op = 0;

 return ay_status;
} /* ay_undo_undo */


/* ay_undo_rewind:
 *  undo the last "undo save" operation
 */
int
ay_undo_rewind(int silent)
{
 char fname[] = "rewind";
 ay_undo_object *uo = NULL;

  if(undo_current < 0)
    {
      if(!silent)
	ay_error(AY_ERROR, fname, "Can not perform rewind on empty buffer!");
      return AY_ERROR;
    }

  if(undo_last_op != 2)
    {
      if(!silent)
	ay_error(AY_ERROR, fname, "Can perform rewind only after save!");
      return AY_ERROR;
    }

  uo = &(undo_buffer[undo_current]);
  ay_undo_clearuo(uo);

  undo_current--;

  if(undo_current < 0)
    undo_current = -1;

  undo_last_op = 4;

  if(undo_saved_op)
    {
      free(undo_saved_op);
    }
  undo_saved_op = NULL;

 return AY_OK;
} /* ay_undo_rewind */


/* ay_undo_copysave:
 *  copy object to undo buffer
 */
int
ay_undo_copysave(ay_object *src, ay_object **dst)
{
 int ay_status = AY_OK;
 char fname[] = "undo_copysave";
 ay_object *new = NULL;
 ay_voidfp *arr = NULL;
 ay_copycb *cb = NULL;
 ay_view_object *srcview = NULL, *dstview = NULL;

  if(!src || !dst)
    return AY_ENULL;

  /* copy generic object */
  if(!(new = malloc(sizeof(ay_object))))
    return AY_EOMEM;

  memcpy(new, src, sizeof(ay_object));
  /* danger! links point to original hierarchy */

  new->next = NULL;
  new->down = NULL;
  new->selp = NULL;
  new->tags = NULL;
  /*  new->mat = NULL;*/

  /*if(src->type != AY_IDMATERIAL)*/
  new->refcount = 0;

  /* copy type specific part */
  switch(src->type)
    {
    case AY_IDVIEW:
      srcview = (ay_view_object *)(src->refine);

      if(!(new->refine = malloc(sizeof(ay_view_object))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      dstview = (ay_view_object *)(new->refine);

      memcpy(dstview, srcview, sizeof(ay_view_object));
      dstview->bgimage = NULL;

      /* copy BGImage */
      if(srcview->bgimage)
	{
	  if(!(dstview->bgimage = malloc((strlen(srcview->bgimage)+1)*
					 sizeof(char))))
	    {
	      ay_status = AY_EOMEM;
	      goto cleanup;
	    }

	  strcpy(dstview->bgimage, srcview->bgimage);
	}
      break;
    case AY_IDROOT:
      if(!(new->refine = calloc(1, sizeof(ay_root_object))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      ay_status = ay_undo_copyroot((ay_root_object *)(src->refine),
				   (ay_root_object *)(new->refine));
      if(ay_status)
	{
	  goto cleanup;
	}
      break;
    case AY_IDINSTANCE:
      new->refine = src->refine;
      break;
    default:
      arr = ay_copycbt.arr;
      cb = (ay_copycb*)(arr[src->type]);
      if(cb)
	ay_status = cb(src->refine, &(new->refine));

      if(ay_status)
	{
	  ay_error(AY_ERROR, fname, "copy callback failed");
	  goto cleanup;
	}
      break;
    } /* switch */

  /* copy name */
  if(src->name)
    {
      if(!(new->name = malloc((strlen(src->name)+1) * sizeof(char))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}

      strcpy(new->name, src->name);
    }

  /* copy tags */
  if(src->tags)
    {
      ay_status = ay_undo_copytags(src, new);

      if(ay_status)
	{
	  goto cleanup;
	}
    }

  /* copy selected points */
  if(src->selp)
    {
      ay_status = ay_undo_copyselp(src, new);

      if(ay_status)
	{
	  goto cleanup;
	}
    }

  new->modified = AY_TRUE;

  *dst = new;

  /* prevent cleanup code from doing something harmful */
  new = NULL;

cleanup:

  if(new)
    {
      new->mat = NULL;
      ay_object_delete(new);
    }

 return ay_status;
} /* ay_undo_copysave */


/* ay_undo_savechildren:
 *  _recursively_ save all children of <o> to undo state <uo>
 */
int
ay_undo_savechildren(ay_object *o, ay_undo_object *uo,
		     ay_list_object **lastr,
		     ay_object ***nexto)
{
 int ay_status = AY_OK;
 ay_object *down = NULL;
 ay_list_object *r = NULL;

  if(!(o->down && o->down->next))
    return AY_OK;

  down = o->down;
  while(down->next)
    {
      /* copy reference */
      if(!(r = malloc(sizeof(ay_list_object))))
	{
	  return AY_EOMEM;
	}

      r->next = NULL;
      if(uo->references)
	{
	  (*lastr)->next = r;
	}
      else
	{
	  uo->references = r;
	} /* if */

      *lastr = r;

      r->object = down;

      /* copy object */
      ay_status = ay_undo_copysave(down, *nexto);
      if(ay_status)
	return ay_status;

      *nexto = &((**nexto)->next);

      /* recursively save children */
      if(down->down && down->down->next)
	{
	  ay_status = ay_undo_savechildren(down, uo, lastr, nexto);
	  if(ay_status)
	    return ay_status;
	}

      down = down->next;
    } /* while */

 return AY_OK;
} /* ay_undo_savechildren */


/* ay_undo_save:
 *
 */
int
ay_undo_save(int save_children)
{
 int ay_status = AY_OK;
 ay_undo_object *uo = NULL, *uo2 = NULL;
 ay_list_object *sel = ay_selection, *r = NULL, *lastr = NULL, *lsr = NULL;
 ay_object **nexto = NULL, *view = NULL, *lso = NULL;
 ay_object *markprevsel = NULL, *saved = NULL;
 int i, prevselmarked = AY_FALSE, alreadysaved = AY_FALSE;

  if((!sel) && (!ay_root->down->next))
    return AY_OK;

  /* check, whether we operate on top of undo buffer */
  if(undo_current+1 == undo_buffer_size)
    {
      /* yes, we need to clear the oldest saved state
       * and then shift all states, so that the top
       * of the undo buffer is empty again
       */

      /* clear oldest state */
      uo = &(undo_buffer[0]);
      ay_undo_clearuo(uo);
      /* shift all states down */
      for(i = 0; i < undo_buffer_size-1; i++)
	{
	  uo = &(undo_buffer[i]);
	  uo2 = &(undo_buffer[i+1]);

	  uo->objects = uo2->objects;
	  uo->references = uo2->references;
	  uo->operation = uo2->operation;
	}
      uo = &(undo_buffer[undo_buffer_size-1]);
      uo->objects = NULL;
      uo->references = NULL;
    }
  else
    {
      /* to avoid, that the user gets (after an undo/redo)
       * into an old sequence of changed states using
       * redo after a save, we clear the undo buffer
       * now partially from the current slot to the last
       */
      if((undo_last_op == 0) || (undo_last_op == 1))
	{
	  if(undo_last_op == 0)
	    undo_current++;
	  /* clear tail of undo buffer */
	  for(i = undo_current; i < undo_buffer_size; i++)
	    {
	      uo = &(undo_buffer[i]);
	      ay_undo_clearuo(uo);
	    }
	}
      else
	{
	  undo_current++;
	} /* if */
    } /* if */

  uo = &(undo_buffer[undo_current]);

  if(!uo)
    return AY_ENULL;

  /* link name of saved modelling operation to this undo object */
  uo->operation = undo_saved_op;
  /* avoid alias */
  undo_saved_op = NULL;

  /* check, whether the current undo slot contains saved objects */
  if(uo->objects)
    { /* yes, free them */
      ay_undo_clearuo(uo);
    }

  /* finally, we may copy objects (and save references to the original
   * objects) to the undo buffer */

  /* first we save all currently selected objects */
  sel = ay_selection;
  nexto = &(uo->objects);
  while(sel)
    {
      /* copy reference */
      if(!(r = malloc(sizeof(ay_list_object))))
	{
	  return AY_EOMEM;
	}

      r->next = NULL;
      if(lastr)
	{
	  lastr->next = r;
	}
      else
	{
	  uo->references = r;
	} /* if */

      lastr = r;

      r->object = sel->object;

      /* copy object */
      ay_status = ay_undo_copysave(sel->object, nexto);
      if(ay_status)
	{
	  return ay_status;
	}

      nexto = &((*nexto)->next);

      /* save children */
      if(save_children && sel->object->down && sel->object->down->next)
	{
	  uo->saved_children = AY_TRUE;
	  ay_status = ay_undo_savechildren(sel->object, uo, &lastr, &nexto);
	  if(ay_status)
	    {
	      return ay_status;
	    }
	}
      else
	{
	  uo->saved_children = AY_FALSE;
	}

      sel = sel->next;
    } /* while */

  /* now we save all views */
  view = ay_root->down;
  while(view->next)
    {
      /* copy reference */
      if(!(r = malloc(sizeof(ay_list_object))))
	{
	  return AY_EOMEM;
	}

      r->next = NULL;
      if(lastr)
	{
	  lastr->next = r;
	}
      else
	{
	  uo->references = r;
	}

      lastr = r;

      r->object = view;

      /* copy object */
      ay_status = ay_undo_copysave(view, nexto);
      if(ay_status)
	{
	  return ay_status;
	}

      nexto = &((*nexto)->next);
      view = view->next;
    } /* while */

  /* now, we save certain objects from previous undo state _again_
   * (if not saved in this state already); this is, because the
   * user might have changed the selection after a modelling
   * action and those changed, but now not anymore selected
   * objects, would be omitted otherwise
   */
  if(undo_current > 0)
    {
      uo2 = &(undo_buffer[undo_current-1]);
      lso = uo2->objects;
      lsr = uo2->references;
      while(lso)
	{
	  if(lso->type == AY_IDLAST)
	    {
	      break;
	    }

	  /* check, whether we already saved this object in the
	     current undo slot */
	  alreadysaved = AY_FALSE;
	  saved = uo->objects;
	  while(saved && (saved->type != AY_IDLAST))
	    {
	      if(saved == lso)
		{
		  alreadysaved = AY_TRUE;
		  break;
		}
	      saved = saved->next;
	    } /* while */

	  if(alreadysaved)
	    {
	      lso = lso->next;
	      lsr = lsr->next;
	      continue;
	    }

	  /* mark beginning of extra objects in undo buffer using
	     a special object of type AY_IDLAST with empty reference */
	  if(!prevselmarked)
	    {
	      if(!(r = malloc(sizeof(ay_list_object))))
		{
		  return AY_EOMEM;
		}

	      if(!(markprevsel = calloc(1, sizeof(ay_object))))
		{
		  free(r);
		  return AY_EOMEM;
		}

	      r->next = NULL;
	      if(uo->references)
		{
		  lastr->next = r;
		}
	      else
		{
		  uo->references = r;
		} /* if */

	      lastr = r;

	      /* mark the separator */
	      r->object = NULL;

	      markprevsel->type = AY_IDLAST;
	      *nexto = markprevsel;
	      nexto = &((*nexto)->next);
	    } /* if */

	  /* copy reference */
	  if(!(r = malloc(sizeof(ay_list_object))))
	    {
	      return AY_EOMEM;
	    }

	  r->next = NULL;
	  if(uo->references)
	    {
	      lastr->next = r;
	    }
	  else
	    {
	      uo->references = r;
	    } /* if */

	  lastr = r;

	  r->object = lsr->object;

	  /* copy object */
	  ay_status = ay_undo_copysave(lsr->object, nexto);
	  if(ay_status)
	    {
	      return ay_status;
	    }

	  nexto = &((*nexto)->next);

	  lso = lso->next;
	  lsr = lsr->next;
	} /* while */
    } /* if */

  undo_last_op = 2;

 return AY_OK;
} /* ay_undo_save */


/* ay_undo_clear:
 *  clear all undo information
 */
void
ay_undo_clear(void)
{
 int i = 0;
 ay_undo_object *uo = NULL;

  for(i = 0; i < undo_buffer_size; i++)
    {
      uo = &(undo_buffer[i]);
      ay_undo_clearuo(uo);
    }

  undo_current = -1;
  undo_last_op = -1; /* no op */

  if(undo_saved_op)
    {
      free(undo_saved_op);
    }
  undo_saved_op = NULL;

 return;
} /* ay_undo_clear */


/* ay_undo_clearobj:
 *  clear all undo information of object <o> from undo buffer
 *  also prevent NO tags pointing to <o> from sneaking back
 *  into the scene and causing havoc
 *  for possible future undo/redo operations
 */
void
ay_undo_clearobj(ay_object *o)
{
 int i = 0;
 ay_undo_object *uo = NULL;
 ay_object **lu = NULL, *u = NULL, *down;
 ay_object *no_master = NULL;
 ay_list_object **lr = NULL, *r = NULL;
 ay_tag *tag = NULL, **lasttag;

  tag = o->tags;
  while(tag)
    {
      if(tag->type == ay_nm_tagtype && tag->is_binary)
	{
	  no_master = (ay_object*)(((ay_btval*)tag->val)->payload);
	}
      tag = tag->next;
    }

  for(i = 0; i < undo_buffer_size; i++)
    {
      uo = &(undo_buffer[i]);
      lu = &(uo->objects);
      lr = &(uo->references);
      u = uo->objects;
      r = uo->references;
      while(u)
	{
	  if(r->object == no_master)
	    {
	      tag = u->tags;
	      lasttag = &(u->tags);
	      while(tag)
		{
		  if(tag->type == ay_no_tagtype && tag->is_binary)
		    {
		      if(o == ((ay_btval*)tag->val)->payload)
			{
			  *lasttag = tag->next;
			  ay_tags_free(tag);
			  break;
			}
		    }
		  lasttag = &(tag->next);
		  tag = tag->next;
		}
	    }
	  if(r->object == o)
	    {
	      *lu = u->next;
	      *lr = r->next;
	      u->next = NULL;
	      ay_undo_deletemulti(u);
	      free(r);
	      u = *lu;
	      r = *lr;
	    }
	  else
	    {
	      lu = &(u->next);
	      lr = &(r->next);
	      u = u->next;
	      r = r->next;
	    } /* if */
	} /* while */
    } /* for */

  down = o->down;
  while(down)
    {
      ay_undo_clearobj(down);
      down = down->next;
    }

 return;
} /* ay_undo_clearobj */


/* ay_undo_undotcmd:
 *  manages the undo system
 *  Implements the \a undo scripting interface command.
 *  See also the corresponding section in the \ayd{scundo}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_undo_undotcmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
 int ay_status = AY_OK;
 int mode = 0; /* default mode is "undo" */
 int save_children = AY_FALSE;
 char *a = "ay", *n = "sc", *v = "1";
 char *n2 = "uc", *n3 = "undoo", *n4 = "redoo", v2[64];
 char vnone[] = "None";
#ifdef UNDO_DBG
 char vnull[] = "Null";
#else
 char *vnull = vnone;
#endif

 static int uc = 0;
 static int frozen = 0;


  /* parse args */
  if(argc > 1)
    {
      if(argv[1][0] == 'f')
	{
	  if(argc > 2)
	    Tcl_GetInt(interp, argv[2], &frozen);
	  return TCL_OK;
	}

      if(!strcmp(argv[1], "redo"))
	{
	  mode = 1;
	}
      else
	{
	  if(!strcmp(argv[1], "save"))
	    {
	      mode = 2;
	    }
	  else
	    {
	      if(!strcmp(argv[1], "clear"))
		{
		  mode = 3;
		}
	      else
		{
		  if(!strcmp(argv[1], "rewind"))
		    {
		      mode = 4;
		    }
		  else
		    {
		      ay_error(AY_EARGS, argv[0], "redo|save|clear|rewind");
		      return TCL_OK;
		    } /* if */
		} /* if */
	    } /* if */
	} /* if */
    } /* if */

  if(frozen)
    return TCL_OK;

  /* protect undo code from too small buffers */
  if(undo_buffer_size < 2)
    {
      if(mode == 2)
	{
	  /* set scene changed flag */
	  Tcl_SetVar2(interp, a, n, v, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	}
      return TCL_OK;
    } /* if */

  switch(mode)
    {
    case 0:
#ifdef UNDO_DBG
      printf("undo\n");
#endif
      /* perform undo */
      ay_status = ay_undo_undo();
      if(!ay_status)
	{
	  uc--;
	  /*printf("undo_current:%d ; undo_buffer_size:%d\n",
	    undo_current, undo_buffer_size);*/
	  /* set undo prompt */
	  if(undo_current > -1)
	    {
	      if((undo_buffer[undo_current]).operation)
		Tcl_SetVar2(interp, a, n3,
			    (undo_buffer[undo_current]).operation,
			    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	      else
		Tcl_SetVar2(interp, a, n3, vnull,
			    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	    }
	  else
	    {
	      Tcl_SetVar2(interp, a, n3, vnone,
			  TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	    }
	  if(undo_current+1 < undo_buffer_size)
	    {
	      if((undo_buffer[undo_current+1]).operation)
		Tcl_SetVar2(interp, a, n4,
			    (undo_buffer[undo_current+1]).operation,
			    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	      else
		Tcl_SetVar2(interp, a, n4, vnull,
			    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	    }
	} /* if */
      break;
    case 1:
#ifdef UNDO_DBG
      printf("redo\n");
#endif
      /* perform redo */
      ay_status = ay_undo_redo();
      if(!ay_status)
	{
	  uc++;
	  /* set undo prompt */
	  if(undo_current > 0)
	    {
	      if((undo_buffer[undo_current-1]).operation)
		Tcl_SetVar2(interp, a, n3,
			    (undo_buffer[undo_current-1]).operation,
			    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	      else
		Tcl_SetVar2(interp, a, n3, vnull,
			    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	    }
	  if(undo_current+1 < undo_buffer_size)
	    {
	      if((undo_buffer[undo_current]).operation)
		Tcl_SetVar2(interp, a, n4,
			    (undo_buffer[undo_current]).operation,
			    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	      else
		Tcl_SetVar2(interp, a, n4, vnull,
			    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	    }
	  else
	    {
	      Tcl_SetVar2(interp, a, n4, vnone,
			  TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	    }
	} /* if */
      break;
    case 2:
#ifdef UNDO_DBG
      printf("save\n");
#endif
      /* perform save */
      /* save name of modelling operation in progress */
      if(argc > 2)
	{
	  if(undo_saved_op)
	    free(undo_saved_op);
	  undo_saved_op = NULL;

	  if(!(undo_saved_op = malloc((strlen(argv[2]) + 1) *
				      sizeof(char))))
	    {
	      ay_error(AY_EOMEM, argv[0], NULL);
	      return TCL_OK;
	    }
	  else
	    {
	      strcpy(undo_saved_op, argv[2]);
	    }
	  /* is the save_children argument given? */
	  if(argc > 3)
	    {
	      sscanf(argv[3], "%d", &save_children);
	    } /* if */
	} /* if */
      ay_status = ay_undo_save(save_children);
      if(ay_status)
	{
	  ay_error(AY_ERROR, argv[0], "undo save failed");
	  ay_undo_clearuo(&(undo_buffer[uc]));
	  if(undo_saved_op)
	    {
	      free(undo_saved_op);
	      undo_saved_op = NULL;
	    }
	  return TCL_OK;
	}
      else
	{
	  uc++;
	  /* set undo prompt */
	  if((undo_buffer[undo_current]).operation)
	    Tcl_SetVar2(interp, a, n3, (undo_buffer[undo_current]).operation,
			TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	  Tcl_SetVar2(interp, a, n4, vnone, TCL_LEAVE_ERR_MSG |
		      TCL_GLOBAL_ONLY);
	}
      /* set scene changed flag */
      Tcl_SetVar2(interp, a, n, v, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
      undo_saved_op = NULL;
      break;
    case 3:
#ifdef UNDO_DBG
      printf("clear\n");
#endif
      /* perform clear */
      ay_undo_clear();
      uc = 0;
      /* re-set undo prompt */
      Tcl_SetVar2(interp, a, n3, vnone, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
      Tcl_SetVar2(interp, a, n4, vnone, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
      break;
    case 4:
#ifdef UNDO_DBG
      printf("rewind\n");
#endif
      /* perform rewind (undo a save) */
      ay_status = ay_undo_rewind(argc > 2);
      if(!ay_status)
	{
	  uc--;
	  /*printf("undo_current:%d ; undo_buffer_size:%d\n",
	    undo_current, undo_buffer_size);*/
	  /* set undo prompt */
	  if(undo_current > -1)
	    {
	      if((undo_buffer[undo_current]).operation)
		Tcl_SetVar2(interp, a, n3,
			    (undo_buffer[undo_current]).operation,
			    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	      else
		Tcl_SetVar2(interp, a, n3, vnull,
			    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	    }
	  else
	    {
	      Tcl_SetVar2(interp, a, n3, vnone,
			  TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	    }
	  if(undo_current+1 < undo_buffer_size)
	    {
	      if((undo_buffer[undo_current+1]).operation)
		Tcl_SetVar2(interp, a, n4,
			    (undo_buffer[undo_current+1]).operation,
			    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	      else
		Tcl_SetVar2(interp, a, n4, vnull,
			    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
	    }
	} /* if */
      break;
    default:
      break;
    } /* switch */

  if(uc > undo_buffer_size)
    uc = undo_buffer_size;

  if((uc < 1) && (mode != 3))
    uc = 1;

  /* set (user visible) undo buffer pointer */
  sprintf(v2, "%d", uc);
  Tcl_SetVar2(interp, a, n2, v2, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

 return TCL_OK;
} /* ay_undo_undotcmd */
