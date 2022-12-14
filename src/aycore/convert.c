/*
 * Ayam, a free 3D modeler for the RenderMan interface.
 *
 * Ayam is copyrighted 1998-2004 by Randolf Schultz
 * (randolf.schultz@gmail.com) and others.
 *
 * All rights reserved.
 *
 * See the file License for details.
 *
 */

#include "ayam.h"

/* convert.c - functions for object conversion */

/** ay_convert_register:
 * register a conversion callback
 *
 * \param[in] convcb conversion callback
 * \param[in] type_id object type for which to register the callback (AY_ID...)
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_convert_register(ay_convertcb *convcb, unsigned int type_id)
{
 int ay_status = AY_OK;

  /* register convert callback */
  ay_status = ay_table_addcallback(&ay_convertcbt, (ay_voidfp)convcb, type_id);

 return ay_status;
} /* ay_convert_register */


/** ay_convert_object:
 * call conversion callback of an object
 *
 * \param[in,out]  o  object to convert
 * \param[in]  in_place  designates whether to replace \a o with the
 *   result of the conversion
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_convert_object(ay_object *o, int in_place)
{
 int ay_status = AY_OK;
 char fname[] = "convert";
 ay_voidfp *arr = NULL;
 ay_convertcb *cb = NULL;

  if(!o)
    return AY_ENULL;

  /* call the conversion callback */
  arr = ay_convertcbt.arr;
  cb = (ay_convertcb *)(arr[o->type]);
  if(cb)
    {
      ay_status = cb(o, in_place);

      if(ay_status)
	{
	  ay_error(AY_ERROR, fname, "Convert callback failed!");
	  return AY_ERROR;
	}
    }
  else
    {
     ay_error(AY_ERROR, fname, "No converter available!");
    }

 return AY_OK;
} /* ay_convert_object */


/** ay_convert_objecttcmd:
 * force conversion of selected object(s)
 * Implements the \a convOb scripting interface command.
 * See also the corresponding section in the \ayd{scconvob}.
 * \returns TCL_OK in any case.
 */
int
ay_convert_objecttcmd(ClientData clientData, Tcl_Interp *interp,
		      int argc, char *argv[])
{
 int ay_status = AY_OK, in_place = AY_FALSE, notify_parent = AY_FALSE;
 int argt = 1, check = AY_FALSE;
 unsigned int ttype = 0;
 ay_list_object *sel = ay_selection;
 ay_object *new = NULL;
 Tcl_HashEntry *entry = NULL;
 char *res, no[] = "0", yes[] = "1";

  if(argc > 1)
    {
      if((argv[1][0] == '-') && (argv[1][1] == 'i'))
	{
	  in_place = AY_TRUE;
	  argt++;
	}
      else
      if((argv[1][0] == '-') && (argv[1][1] == 'c'))
	{
	  check = AY_TRUE;
	  if(argc < 3)
	    {
	      ay_error(AY_EARGS, argv[0], "-check type");
	      return TCL_OK;
	    }
	  else
	    {
	      argt++;
	    }
	}
    }

  /* resolve target type */
  if(argt < argc)
    {
      if((entry = Tcl_FindHashEntry(&ay_otypesht, argv[argt])))
	{
	  ttype = *((unsigned int*)Tcl_GetHashValue(entry));
	}
      else
	{
	  ay_error(AY_ENTYPE, argv[0], argv[argt]);
	  return TCL_OK;
	}
    }

  while(sel)
    {
      if(check)
	{
	  /* check */
	  ay_status = ay_provide_object(sel->object, ttype, NULL);
	  if(ay_status == AY_OK)
	    res = yes;
	  else
	    res = no;

	  if(ay_selection->next)
	    Tcl_AppendElement(interp, res);
	  else
	    Tcl_SetResult(interp, res, TCL_VOLATILE);
	}
      else
	{
	  /* convert */
	  if(ttype != 0)
	    {
	      ay_status = ay_provide_object(sel->object, ttype, NULL);
	      if(ay_status == AY_OK)
		{
		  ay_status = ay_provide_object(sel->object, ttype, &new);

		  if(ay_status == AY_OK && new)
		    {
		      if(!in_place)
			{
			  ay_object_link(new);
			}
		      else
			{
			  (void)ay_object_replace(new, sel->object);
			}
		    }
		}
	      else
		{
		  while(sel->object->type != ttype && !ay_status)
		    {
		      ay_status = ay_convert_object(sel->object, in_place);
		      if(!ay_status)
			{
			  sel->object->modified = AY_TRUE;
			  notify_parent = AY_TRUE;
			}
		    }
		}
	    }
	  else
	    {
	      ay_status = ay_convert_object(sel->object, in_place);
	      if(!ay_status)
		{
		  sel->object->modified = AY_TRUE;
		  notify_parent = AY_TRUE;
		}
	    }
	} /* if check or convert */
      sel = sel->next;
    } /* while */

  if(notify_parent)
    {
      (void)ay_notify_parent();
    }

 return TCL_OK;
} /* ay_convert_objecttcmd */


/** ay_convert_nptoolobj:
 * Helper for tool objects that convert to a NURBS patch
 * plus caps/bevels.
 *
 * \param[in,out]  o  original object to be converted
 * \param[in]  p  patch object(s) \a o converts to
 * \param[in]  cb  caps and bevels
 * \param[in]  in_place  if AY_TRUE, \a o will be replaced by the
 *  converted object(s), if AY_FALSE, the converted objects will
 *  be linked to the current level of the scene
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_convert_nptoolobj(ay_object *o, ay_object *p, ay_object *cb, int in_place)
{
 int ay_status = AY_OK;
 ay_object *new = NULL, **next = NULL;
 ay_object *b;
 ay_nurbpatch_object *np = NULL;

  if(!o || !p)
    return AY_ENULL;

  if(ay_prefs.conv_keep_caps || ay_prefs.conv_keep_bevels)
    cb = NULL;

  if(p->next || cb)
    {
      /* object converts to multiple patches or there are caps/bevels */
      if(!(new = calloc(1, sizeof(ay_object))))
	{ return AY_EOMEM; }

      ay_object_defaults(new);
      new->type = AY_IDLEVEL;
      new->parent = AY_TRUE;
      new->inherit_trafos = AY_TRUE;
      ay_trafo_copy(o, new);

      if(!(new->refine = calloc(1, sizeof(ay_level_object))))
	{ free(new); return AY_EOMEM; }

      ((ay_level_object *)(new->refine))->type = AY_LTLEVEL;

      next = &(new->down);

      b = p;
      while(b)
	{
	  ay_status = ay_object_copy(b, next);
	  if(ay_status)
	    {
	      (void)ay_object_deletemulti(new, AY_FALSE);
	      return AY_ERROR;
	    }
	  if(*next)
	    {
	      (*next)->hide_children = AY_TRUE;
	      (*next)->parent = AY_TRUE;
	      if(!(*next)->down)
		(*next)->down = ay_endlevel;

	      next = &((*next)->next);
	    }
	  b = b->next;
	} /* while */

      b = cb;
      while(b)
	{
	  ay_status = ay_object_copy(b, next);
	  if(ay_status)
	    {
	      (void)ay_object_deletemulti(new, AY_FALSE);
	      return AY_ERROR;
	    }
	  if(*next)
	    {
	      (*next)->hide_children = AY_TRUE;
	      (*next)->parent = AY_TRUE;
	      if(!(*next)->down)
		(*next)->down = ay_endlevel;

	      next = &((*next)->next);
	    }
	  b = b->next;
	} /* while */

      /* copy some tags */
      (void)ay_tags_copyselected(o, new->down, ay_prefs.converttags,
				 ay_prefs.converttagslen);

      /* terminate the level */
      *next = ay_endlevel;
    }
  else
    {
      /* object converts to a single patch */
      ay_status = ay_object_copy(p, &new);
      if(!ay_status && new)
	{
	  ay_trafo_copy(o, new);
	  new->hide_children = AY_TRUE;
	  new->parent = AY_TRUE;
	  if(!new->down)
	    new->down = ay_endlevel;

	  /* copy some tags */
	  (void)ay_tags_copyselected(o, new, ay_prefs.converttags,
				     ay_prefs.converttagslen);
	} /* if */
    } /* if */

  if(new)
    {
      if(p->next || cb)
	{
	  b = new->down;
	}
      else
	{
	  b = new;
	}
      while(b)
	{
	  /* re-create caps and bevels */
	  b->modified = AY_TRUE;
	  ay_notify_object(b);

	  /* reset display mode and sampling tolerance
	     of new patch to "global"? */
	  if(!in_place && ay_prefs.conv_reset_display)
	    {
	      ay_npt_resetdisplay(b);
	    }

	  /* immediately create and show the multiple points */
	  if(b->type == AY_IDNPATCH)
	    {
	      np = (ay_nurbpatch_object *)b->refine;
	      np->createmp = AY_TRUE;
	      ay_npt_recreatemp(np);
	    }
	  b = b->next;
	} /* while */

      /* link the new object(s) to the scene */
      if(!in_place)
	{
	  ay_object_link(new);
	}
      else
	{
	  (void)ay_object_replace(new, o);
	}
    } /* if */

 return ay_status;
} /* ay_convert_nptoolobj */

