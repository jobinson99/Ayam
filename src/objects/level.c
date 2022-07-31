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

/* level.c - level object */

static char *ay_level_name = "Level";

/* functions: */

/* ay_level_createcb:
 *  create callback function of level object
 */
int
ay_level_createcb(int argc, char *argv[], ay_object *o)
{
 ay_level_object *level = NULL;
 char fname[] = "crtlevel";

 if(!o)
    return AY_ENULL;

  if(!(level = calloc(1, sizeof(ay_level_object))))
    {
      ay_error(AY_EOMEM, fname, NULL);
      return AY_ERROR;
    }

  o->inherit_trafos = AY_TRUE;
  o->parent = AY_TRUE;
  o->refine = level;

  if(argc > 2)
    {
      if(!argv)
	return AY_ENULL;

      level->type = (unsigned int) atoi(argv[2]);
    }

  if(level->type <= 0)
    level->type = 1;

  if(level->type > 5)
    level->type = 1;

 return AY_OK;
} /* ay_level_createcb */


/* ay_level_deletecb:
 *  delete callback function of level object
 */
int
ay_level_deletecb(void *c)
{
 ay_level_object *level = NULL;

  if(!c)
    return AY_ENULL;

  level = (ay_level_object *)(c);

  free(level);

 return AY_OK;
} /* ay_level_deletecb */


/* ay_level_copycb:
 *  copy callback function of level object
 */
int
ay_level_copycb(void *src, void **dst)
{
 ay_level_object *level = NULL;

  if(!(level = malloc(sizeof(ay_level_object))))
    return AY_EOMEM;

  memcpy(level, src, sizeof(ay_level_object));

  *dst = level;

 return AY_OK;
} /* ay_level_copycb */


/* ay_level_drawcb:
 *  draw (display in an Ayam view window) callback function of level object
 */
int
ay_level_drawcb(struct Togl *togl, ay_object *o)
{
  if(!o)
    return AY_ENULL;

 return AY_OK;
} /* ay_level_drawcb */


/* ay_level_drawhcb:
 *  draw handles (in an Ayam view window) callback function of level object
 */
int
ay_level_drawhcb(struct Togl *togl, ay_object *o)
{
  if(!o)
    return AY_ENULL;

 return AY_OK;
} /* ay_level_drawhcb */


/* ay_level_shadecb:
 *  shade (display in an Ayam view window) callback function of level object
 */
int
ay_level_shadecb(struct Togl *togl, ay_object *o)
{
  if(!o)
    return AY_ENULL;

 return AY_OK;
} /* ay_level_shadecb */


/* ay_level_getpntcb:
 *  get point (editing and selection) callback function of level object
 */
int
ay_level_getpntcb(int mode, ay_object *o, double *p, ay_pointedit *pe)
{
  if(!o)
    return AY_ENULL;

 return AY_OK;
} /* ay_level_getpntcb */


/* ay_level_setpropcb:
 *  set property (from Tcl to C context) callback function of level object
 */
int
ay_level_setpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
 char *arr = "LevelAttrData";
 Tcl_Obj *to = NULL;
 ay_level_object *level = NULL;

  if(!interp || !o)
    return AY_ENULL;

  level = (ay_level_object *)o->refine;

  if(!level)
    return AY_ENULL;

  if((to = Tcl_GetVar2Ex(interp, arr, "Type",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    {
      Tcl_GetIntFromObj(interp, to, &(level->type));
      level->type++;
    }

 return AY_OK;
} /* ay_level_setpropcb */


/* ay_level_getpropcb:
 *  get property (from C to Tcl context) callback function of level object
 */
int
ay_level_getpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
 char *arr = "LevelAttrData";
 ay_level_object *level = NULL;

  if(!interp || !o)
    return AY_ENULL;

  level = (ay_level_object *)o->refine;

  if(!level)
    return AY_ENULL;

  Tcl_SetVar2Ex(interp, arr, "Type",
		Tcl_NewIntObj(level->type-1),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

 return AY_OK;
} /* ay_level_getpropcb */


/* ay_level_readcb:
 *  read (from scene file) callback function of level object
 */
int
ay_level_readcb(FILE *fileptr, ay_object *o)
{
 ay_level_object *level = NULL;
 int type = 0, result = 0;

  if(!fileptr || !o)
    return AY_ENULL;

  fscanf(fileptr, "%d\n", &type);

  if(type == AY_LTEND)
    {
      /* the level terminator was already created
	 when the parent object was read, thus, all we
	 need to do here is to go up in the hierarchy */
      result = ay_clevel_gouptcmd(NULL, ay_interp, 0, NULL);
      if(result != TCL_OK)
	return AY_ERROR;
      else
	return AY_EDONOTLINK;
    }

  if(!(level = calloc(1, sizeof(ay_level_object))))
    return AY_EOMEM;

  level->type = type;

  o->refine = level;

 return AY_OK;
} /* ay_level_readcb */


/* ay_level_writecb:
 *  write (to scene file) callback function of level object
 */
int
ay_level_writecb(FILE *fileptr, ay_object *o)
{
 ay_level_object *level = NULL;

  if(!fileptr || !o)
    return AY_ENULL;

  level = (ay_level_object *)(o->refine);

  if(!level)
    return AY_ENULL;

  fprintf(fileptr, "%d\n", level->type);

 return AY_OK;
} /* ay_level_writecb */


/* ay_level_wribcb:
 *  RIB export callback function of level object
 *  does nothing, the real work of testing level types
 *  and writing appropriate RiSolids is done in:
 *  aycore/wrib.c/ay_wrib_object()
 *  aycore/instt.c/ay_instt_wribiarchives()
 *  objects/instance.c/ay_instance_wribcb()
 */
int
ay_level_wribcb(char *file, ay_object *o)
{
  /*
  ay_level_object *level = NULL;

  if(!o)
   return AY_ENULL;

  level = (ay_level_object*)o->refine;
  */

 return AY_OK;
} /* ay_level_wribcb */


/* ay_level_bbccb:
 *  bounding box calculation callback function of level object
 */
int
ay_level_bbccb(ay_object *o, double *bbox, int *flags)
{

  if(!o || !bbox || !flags)
    return AY_ENULL;

  /* we have no own bounding box, all that counts are the children */
  *flags = 2;

 return AY_OK;
} /* ay_level_bbccb */


/* ay_level_providecb:
 *  provide callback function of level object
 */
int
ay_level_providecb(ay_object *o, unsigned int type, ay_object **result)
{
 int ay_status = AY_OK;
 ay_object *t = NULL, *d, **last;
 ay_level_object *l = NULL;
 double m1[16], m2[16], m3[16];

  if(!o)
    return AY_ENULL;

  if(!result)
    {
      /* can we deliver at least one object of right type? */
      d = o->down;
      while(d && d->next)
	{
	  if(type == d->type)
	    {
	      return AY_OK;
	    }
	  else
	    {
	      if(AY_OK == ay_provide_object(d, type, NULL))
		return AY_OK;
	    }
	  d = d->next;
	} /* while */
      return AY_ERROR;
    } /* if */

  l = (ay_level_object *)o->refine;

  if(!l)
    return AY_ENULL;

  if(l->type == AY_LTEND)
    return AY_OK;

  /* check for presence of any child objects */
  if(!o->down || (o->down && !o->down->next))
    return AY_ERROR;

  d = o->down;
  last = result;
  while(d->next)
    {
      ay_trafo_creatematrix(d, m1);

      *last = NULL;
      if(type == d->type)
	{
	  ay_status = ay_object_copy(d, last);
	  if(*last)
	    {
	      last = &((*last)->next);
	    }
	}
      else
	{
	  ay_status = ay_provide_object(d, type, last);
	  t = *last;

	  if(d->type == AY_IDLEVEL)
	    {
	      while(t)
		{
		  memcpy(m3, m1, 16*sizeof(double));
		  ay_trafo_creatematrix(t, m2);
		  ay_trafo_multmatrix(m3, m2);
		  ay_trafo_decomposematrix(m3, t);

		  last = &(t->next);
		  t = t->next;
		} /* while */
	    }
	  else
	    {
	      while(t)
		{
		  last = &(t->next);
		  t = t->next;
		} /* while */
	    } /* if */

	} /* if */
      d = d->next;
    } /* while */

 return ay_status;
} /* ay_level_providecb */


/* ay_level_peekb:
 *  provide callback function of level object
 */
int
ay_level_peekcb(ay_object *o, unsigned int type, ay_object **result,
		double *transform)
{
 int ay_status = AY_OK;
 unsigned int count = 0, subcount;
 unsigned int i, j;
 ay_object *d, **results, **subresults;
 double *subtrafos;
 ay_level_object *l = NULL;
 double tm[16];

  if(!o)
    return AY_ENULL;

  l = (ay_level_object *)o->refine;

  if(!l)
    return AY_ENULL;

  if(l->type == AY_LTEND)
    return AY_OK;

  if(!result || transform)
    {
      d = o->down;

      while(d && d->next)
	{
	  if(d->type == type)
	    {
	      count++;
	    }
	  else
	    {
	      subcount = ay_peek_object(d, type, NULL, NULL);
	      count += subcount;
	    } /* if d is target type */

	  d = d->next;
	} /* while */
      if(!result)
	return count;
    } /* if check/count */

  if(transform)
    {
      memcpy(tm, transform, 16*sizeof(double));
      j = 16;
      for(i = 1; i < count; i++)
	{
	  memcpy(&(transform[j]), tm, 16*sizeof(double));
	  j += 16;
	}
    }

  results = result;
  i = 0;
  d = o->down;
  while(d && d->next && (results[i] != ay_endlevel))
    {
      if(d->type == type)
	{
	  results[i] = d;
	  if(transform && AY_ISTRAFO(d))
	    {
	      ay_trafo_creatematrix(d, tm);
	      ay_trafo_multmatrix(&(transform[i*16]), tm);
	    }
	  i++;
	}
      else
	{
	  if(ay_peek_object(d, type, NULL, NULL))
	    {
	      subresults = &(results[i]);
	      subtrafos = &(transform[i*16]);
	      ay_status = ay_peek_object(d, type, &subresults, &subtrafos);
	      /* error handling? */
	      while(results[i] && (results[i] != ay_endlevel))
		{
		  i++;
		}
	    }
	} /* if d is target type */

      d = d->next;
    } /* while */

 return ay_status;
} /* ay_level_peekcb */


/* ay_level_drawacb:
 *  draw annotations (in an Ayam view window) callback function of level object
 */
int
ay_level_drawacb(struct Togl *togl, ay_object *o)
{
 ay_view_object *view = (ay_view_object *)Togl_GetClientData(togl);

  if(!o)
    return AY_ENULL;

  glLoadIdentity();

  if(view->drawhandles == 4)
    {
      (void)ay_npt_drawboundaries(togl, o);
      return AY_OK;
    }

 return AY_OK;
} /* ay_level_drawacb */


/* ay_level_init:
 *  initialize the level object module
 */
int
ay_level_init(Tcl_Interp *interp)
{
 int ay_status = AY_OK;

  ay_status = ay_otype_registercore(ay_level_name,
				    ay_level_createcb,
				    ay_level_deletecb,
				    ay_level_copycb,
				    NULL, /* no drawing! */
				    NULL, /* no handles! */
				    NULL, /* no shading! */
				    ay_level_setpropcb,
				    ay_level_getpropcb,
				    NULL, /* no picking! */
				    ay_level_readcb,
				    ay_level_writecb,
				    ay_level_wribcb,
				    ay_level_bbccb,
				    AY_IDLEVEL);

  ay_status += ay_provide_register(ay_level_providecb, AY_IDLEVEL);

  ay_status += ay_peek_register(ay_level_peekcb, AY_IDLEVEL);

  ay_status += ay_draw_registerdacb(ay_level_drawacb, AY_IDLEVEL);

 return ay_status;
} /* ay_level_init */
