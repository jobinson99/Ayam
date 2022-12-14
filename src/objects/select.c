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
#include <ctype.h>

/* select.c - select object */

static char *ay_select_name = "Select";

int ay_select_parseindices(ay_select_object *sel, int lastindex);


/* functions: */

/* ay_select_createcb:
 *  create callback function of select object
 */
int
ay_select_createcb(int argc, char *argv[], ay_object *o)
{
 ay_select_object *select = NULL;
 char fname[] = "crtselect";

  if(!o)
    return AY_ENULL;

  if(!(select = calloc(1, sizeof(ay_select_object))))
    {
      ay_error(AY_EOMEM, fname, NULL);
      return AY_ERROR;
    }

  o->inherit_trafos = AY_TRUE;
  o->parent = AY_TRUE;
  o->refine = select;
  /*
  if(argc > 2)
    select->index = atoi(argv[2]);
  */

 return AY_OK;
} /* ay_select_createcb */


/* ay_select_deletecb:
 *  delete callback function of select object
 */
int
ay_select_deletecb(void *c)
{
 ay_select_object *select = NULL;

  if(!c)
    return AY_ENULL;

  select = (ay_select_object *)(c);

  if(select->indices)
    free(select->indices);

  if(select->seli)
    free(select->seli);

  free(select);

 return AY_OK;
} /* ay_select_deletecb */


/* ay_select_copycb:
 *  copy callback function of select object
 */
int
ay_select_copycb(void *src, void **dst)
{
 ay_select_object *select = NULL, *selectsrc = NULL;

  if(!src || !dst)
    return AY_ENULL;

  selectsrc = (ay_select_object *)src;

  if(!(select = malloc(sizeof(ay_select_object))))
    return AY_EOMEM;

  memcpy(select, src, sizeof(ay_select_object));

  if(selectsrc->indices)
    {
      if(!(select->indices = malloc((strlen(selectsrc->indices)+1) *
				    sizeof(char))))
	{ free(select); return AY_EOMEM; }

      strcpy(select->indices, selectsrc->indices);
    }

  select->seli = NULL;

  *dst = select;

 return AY_OK;
} /* ay_select_copycb */


/* ay_select_drawcb:
 *  draw (display in an Ayam view window) callback function of select object
 */
int
ay_select_drawcb(struct Togl *togl, ay_object *o)
{
  if(!o)
    return AY_ENULL;

 return AY_OK;
} /* ay_select_drawcb */


/* ay_select_drawhcb:
 *  draw handles (in an Ayam view window) callback function of select object
 */
int
ay_select_drawhcb(struct Togl *togl, ay_object *o)
{
  if(!o)
    return AY_ENULL;

 return AY_OK;
} /* ay_select_drawhcb */


/* ay_select_shadecb:
 *  shade (display in an Ayam view window) callback function of select object
 */
int
ay_select_shadecb(struct Togl *togl, ay_object *o)
{
  if(!o)
    return AY_ENULL;

 return AY_OK;
} /* ay_select_shadecb */


/* ay_select_getpntcb:
 *  get point (editing and selection) callback function of select object
 */
int
ay_select_getpntcb(int mode, ay_object *o, double *p, ay_pointedit *pe)
{
  if(!o)
    return AY_ENULL;

 return AY_OK;
} /* ay_select_getpntcb */


/* ay_select_setpropcb:
 *  set property (from Tcl to C context) callback function of select object
 */
int
ay_select_setpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
 /*int ay_status = AY_OK;*/
 char fname[] = "setprop";
 char *arr = "SelectAttrData";
 Tcl_Obj *to = NULL;
 ay_select_object *select = NULL;
 char *string = NULL;
 int stringlen;

  if(!o)
    return AY_ENULL;

  select = (ay_select_object *)o->refine;

  if(!select)
    return AY_ENULL;

  if((to = Tcl_GetVar2Ex(interp, arr, "Indices",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    {
      string = Tcl_GetStringFromObj(to, &stringlen);
      if(!string)
	{
	  ay_error(AY_ENULL, fname, NULL);
	  goto cleanup;
	}

      if(select->indices)
	{
	  free(select->indices);
	  select->indices = NULL;
	  o->modified = AY_TRUE;
	}

      if(stringlen > 0)
	{
	  if(!(select->indices = calloc(stringlen+1, sizeof(char))))
	    {
	      ay_error(AY_EOMEM, fname, NULL);
	      goto cleanup;
	    }
	  strcpy(select->indices, string);

	  if(select->seli)
	    free(select->seli);
	  select->seli = NULL;
	  o->modified = AY_TRUE;
	}
    } /* if */

cleanup:

  ay_notify_parent();

 return AY_OK;
} /* ay_select_setpropcb */


/* ay_select_getpropcb:
 *  get property (from C to Tcl context) callback function of select object
 */
int
ay_select_getpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
 char *arr = "SelectAttrData";
 Tcl_Obj *to = NULL;
 ay_select_object *select = NULL;

  if(!o)
    return AY_ENULL;

  select = (ay_select_object *)o->refine;

  if(!select)
    return AY_ENULL;

  to = Tcl_NewStringObj(select->indices, -1);

  Tcl_SetVar2Ex(interp, arr, "Indices", to,
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

 return AY_OK;
} /* ay_select_getpropcb */


/* ay_select_readcb:
 *  read (from scene file) callback function of select object
 */
int
ay_select_readcb(FILE *fileptr, ay_object *o)
{
 int ay_status = AY_OK;
 ay_select_object *select = NULL;
 int tmpi;
 char *buf = NULL;

  if(!o)
    return AY_ENULL;

  if(!(select = calloc(1, sizeof(ay_select_object))))
    return AY_EOMEM;

  if(ay_read_version >= 13)
    {
      /* since 1.16 */
      fscanf(fileptr, "%d", &tmpi);
      fgetc(fileptr);

      ay_status = ay_read_string(fileptr, &(select->indices));
      if(ay_status)
	{
	  free(select);
	  return ay_status;
	}
    }
  else
    {
      fscanf(fileptr, "%d\n", &tmpi);

      if(!(buf = calloc(64, sizeof(char))))
	{ free(select); return AY_EOMEM; }
      sprintf(buf,"%d",tmpi);
      select->indices = buf;
    }

  o->refine = select;

 return AY_OK;
} /* ay_select_readcb */


/* ay_select_writecb:
 *  write (to scene file) callback function of select object
 */
int
ay_select_writecb(FILE *fileptr, ay_object *o)
{
 ay_select_object *select = NULL;

  if(!o)
    return AY_ENULL;

  select = (ay_select_object *)(o->refine);

  if(!select)
    return AY_ENULL;

  if(select->indices)
    fprintf(fileptr, "0\n%s\n", select->indices);
  else
    fprintf(fileptr, "0\n\n");

 return AY_OK;
} /* ay_select_writecb */


/* ay_select_wribcb:
 *  RIB export callback function of select object
 */
int
ay_select_wribcb(char *file, ay_object *o)
{
  /*
  ay_select_object *select = NULL;

  if(!o)
   return AY_ENULL;

  select = (ay_select_object*)o->refine;
  */
 return AY_OK;
} /* ay_select_wribcb */


/* ay_select_bbccb:
 *  bounding box calculation callback function of select object
 */
int
ay_select_bbccb(ay_object *o, double *bbox, int *flags)
{

  if(!o || !bbox || !flags)
    return AY_ENULL;

  /* we have no own bounding box, all that counts are the children */

  /* XXXX since we represent one or more of the childrens provided object(s)
     we should rather calculate the bounding box of _those_ instead */

  *flags = 2;

 return AY_OK;
} /* ay_select_bbccb */


/* ay_select_notifycb:
 *  notify callback function of select object
 */
int
ay_select_notifycb(ay_object *o)
{
 ay_select_object *select = NULL;

  if(!o)
    return AY_ENULL;

  select = (ay_select_object *)o->refine;

  if(!select)
    return AY_ENULL;

  /* if something changed below, the cached indices may be invalid now */
  if(select->seli)
    {
      free(select->seli);
      select->seli = NULL;
    }

 return AY_OK;
} /* ay_select_notifycb */


/* ay_select_parseindices:
 * helper to parse the indices string into an array of unsigned ints
 * stored in the select object
 */
int
ay_select_parseindices(ay_select_object *sel, int lastindex)
{
 int ay_status = AY_OK;
 int i = 0, j = 0, *tmp = NULL;
 int index, startindex, endindex;
 char *buf = NULL, *tok, *min;
 const char delim = ',';

  if(!sel)
    return AY_ENULL;

  if(!sel->indices)
    return AY_OK;

  if(!(buf = calloc(strlen(sel->indices)+1, sizeof(char))))
    { ay_status = AY_EOMEM; goto cleanup; }

  strcpy(buf, sel->indices);

  if(sel->seli)
    free(sel->seli);

  if(!(sel->seli = calloc(64, sizeof(int))))
    { ay_status = AY_EOMEM; goto cleanup; }

  tok = strtok(buf, &delim);
  while(tok)
    {
      /* is there a '-' character in the token? */
      min = strchr(tok, '-');
      if(min)
	{
	  /* parse range */

	  /* get start index of range */
	  /* jump over leading white-space */
	  while(isspace((unsigned char)*tok))
	    {
	      tok++;
	    }
	  /* sanity check for half empty ranges: ",-end,..." or ", -3,...",
	     empty ranges "1,-,..." _and_
	     premature end of string "1,2-" ... */
	  if((*tok == '\0') || (*tok == delim) || (*tok == '-'))
	    {
	      /* ignore this token, process next token */
	      tok = strtok(NULL, &delim);
	    }
	  if(*tok == 'e')
	    {
	      startindex = lastindex;
	    }
	  else
	    {
	      startindex = atoi(tok);
	    }

	  /* get end index of range */
	  /* jump over the '-' */
	  min++;
	  /* jump over leading white-space */
	  while(isspace((unsigned char)*min))
	    {
	      min++;
	    }
	  /* sanity check for half empty ranges: ",1-,..." and
	     premature end of string "1,2-"... */
	  if((*min == '\0') || (*min == delim))
	    {
	      /* ignore this token, process next token */
	      tok = strtok(NULL, &delim);
	      continue;
	    }
	  if(*min == 'e')
	    {
	      endindex = lastindex;
	    }
	  else
	    {
	      endindex = atoi(min);
	    }

	  /* add range to index array */
	  tmp = NULL;
	  if(!(tmp = realloc(sel->seli,
			     (i+(abs(endindex-startindex)))*sizeof(int))))
	    { ay_status = AY_EOMEM; goto cleanup; }
	  sel->seli = tmp;
	  /* increasing range */
	  if(endindex > startindex)
	    {
	      for(j = startindex; j < endindex; j++)
		{
		  sel->seli[i] = j;
		  i++;
		}
	    }
	  /* decreasing range */
	  if(endindex < startindex)
	    {
	      for(j = startindex-1; j >= endindex; j--)
		{
		  sel->seli[i] = j;
		  i++;
		}
	    }
	  /* no(n)sense range */
	  if(endindex == startindex)
	    {
	      tmp = NULL;
	      if(!(tmp = realloc(sel->seli, (i+1)*sizeof(int))))
		{ ay_status = AY_EOMEM; goto cleanup; }
	      sel->seli = tmp;
	      sel->seli[i] = startindex;
	      i++;
	    }
	}
      else
	{
	  /* parse single index */

	  /* jump over leading white-space */
	  while(isspace((unsigned char)*tok))
	    {
	      tok++;
	    }
	  /* sanity check for empty ranges: "1,,2,..." and
	     premature end of string "1,"... */
	  if((*tok == '\0') || (*tok == delim))
	    {
	      /* ignore this token, process next token */
	      tok = strtok(NULL, &delim);
	      continue;
	    }
	  if(*tok == 'e')
	    {
	      index = lastindex;
	    }
	  else
	    {
	      index = atoi(tok);
	    }

	  /* add single index to index array */
	  tmp = NULL;
	  if(!(tmp = realloc(sel->seli, (i+1)*sizeof(int))))
	    { ay_status = AY_EOMEM; goto cleanup; }
	  sel->seli = tmp;
	  sel->seli[i] = index;
	  i++;
	} /* if */

      /* process next token */
      tok = strtok(NULL, &delim);
    } /* while */

  sel->length = i;

cleanup:

  if(buf)
    free(buf);

 return ay_status;
} /* ay_select_parseindices */


/* ay_select_providecb:
 *  provide callback function of select object
 */
int
ay_select_providecb(ay_object *o, unsigned int type, ay_object **result)
{
 int ay_status = AY_OK;
 int i = 0, j = 0;
 int index, lastindex;
 ay_object *down, *allprovided = NULL, **next;
 ay_object *selected = NULL;
 ay_select_object *sel = NULL;

  if(!o)
    return AY_ENULL;

  down = o->down;

  if(!result)
    {
      /* check, whether we can provide at least one object
	 of the wanted type */
      while(down && down->next)
	{
	  if(down->type == type)
	    return AY_OK;
	  ay_status = ay_provide_object(down, type, NULL);
	  if(ay_status == AY_OK)
	    return AY_OK;
	  down = down->next;
	}

      return AY_ERROR;
    } /* if */

  sel = (ay_select_object *)o->refine;

  if(!sel)
    return AY_ENULL;

  if(!sel->indices)
    return AY_OK;

  if(strlen(sel->indices) == 0)
    return AY_OK;

  /* get and count all provided objects first;
     we need to do this first, to know what 'e' means
     when interpreting the indices string
  */
  if(!o->down || !o->down->next)
    return AY_OK;

  next = &(allprovided);
  lastindex = 0;
  down = o->down;
  while(down && down->next)
    {
      if(down->type == type)
	{
	  ay_status = ay_object_copy(down, next);
	}
      else
	{
	  ay_status = ay_provide_object(down, type, next);
	}

      if(ay_status)
	goto cleanup;

      while(*next)
	{
	  lastindex++;
	  next = &((*next)->next);
	}

      down = down->next;
    }

  /* no provided objects => no need to parse the indices string at all */
  if(lastindex == 0)
    return AY_OK;

  /* parse indices string only, if there is no cached index array */
  if(!sel->seli)
    {
      /* (re-)parse indices */
      ay_status = ay_select_parseindices(sel, lastindex);
    } /* if */

  next = &selected;
  lastindex = 0;
  down = allprovided;
  for(i = 0; i < sel->length; i++)
    {
      index = sel->seli[i];
      if(index >= lastindex)
	{
	  for(j = 0; j < index-lastindex; j++)
	    {
	      down = down->next;
	      /* sanity check, index too high? */
	      if(!down)
		break;
	    }
	}
      else
	{
	  down = allprovided;
	  for(j = 0; j < index; j++)
	    {
	      down = down->next;
	      /* sanity check, index too high? */
	      if(!down)
		break;
	    }
	} /* if */

      if(down)
	{
	  /* XXXX make this smarter by not copying _every_ object */
	  ay_status = ay_object_copy(down, next);
	  next = &((*next)->next);
	  lastindex = index;
	}
      else
	{
	  /* we run over the end of the list of provided objects for
	     this index, maybe we have more luck with the next one... */
	  down = allprovided;
	  lastindex = 0;
	}
    } /* for */

  *result = selected;

cleanup:

  if(allprovided)
    (void)ay_object_deletemulti(allprovided, AY_FALSE);

 return ay_status;
} /* ay_select_providecb */


/* ay_select_peekcb:
 *  peek callback function of clone object
 */
int
ay_select_peekcb(ay_object *o, unsigned int type, ay_object **result,
		 double *transform)
{
 int ay_status = AY_OK;
 int j;
 unsigned int i = 0, count = 0;
 ay_select_object *sel = NULL;
 ay_object *down = NULL, **results, **subresults;
 double *subtrafos, tm[16];
 ay_object **tmpo = NULL;

  if(!o)
    return AY_ENULL;

  sel = (ay_select_object *) o->refine;

  if(!sel)
    return AY_ENULL;

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

  if(!sel->seli)
    {
      /* (re-)parse indices */
      ay_select_parseindices(sel, count-1);
    }

  if(!result)
    {
      return sel->length;
    }

  /* dow we have any child objects? */
  down = o->down;
  if(!down || !down->next || !sel->length)
    return AY_OK;

  results = result;

  /* compile temporary array containing all candidate objects */
  count = 0;
  while(down && down->next)
    {
      count++;
      down = down->next;
    } /* while */

  if(!(tmpo = calloc(count, sizeof(ay_object*))))
    return AY_EOMEM;

  down = o->down;
  i = 0;
  while(down && down->next)
    {
      tmpo[i] = down;
      i++;
      down = down->next;
    } /* while */

  /* compile output array by picking from the candidate objects */
  i = 0;
  for(j = 0; j < sel->length; j++)
    {
      down = tmpo[sel->seli[j]];
      if(down->type == type)
	{
	  results[i] = down;
	  if(transform && AY_ISTRAFO(down))
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
	      ay_status = ay_peek_object(down, type, &subresults, &subtrafos);
	      /* error handling? */
	      while(results[i] && (results[i] != ay_endlevel))
		{
		  i++;
		}
	    }
	} /* if */
    } /* for */

  free(tmpo);

 return ay_status;
} /* ay_select_peekcb */


/* ay_select_init:
 *  initialize the select object module
 */
int
ay_select_init(Tcl_Interp *interp)
{
 int ay_status = AY_OK;

  ay_status = ay_otype_registercore(ay_select_name,
				    ay_select_createcb,
				    ay_select_deletecb,
				    ay_select_copycb,
				    NULL, /* no drawing! */
				    NULL, /* no handles! */
				    NULL, /* no shading! */
				    ay_select_setpropcb,
				    ay_select_getpropcb,
				    NULL, /* no picking! */
				    ay_select_readcb,
				    ay_select_writecb,
				    ay_select_wribcb,
				    ay_select_bbccb,
				    AY_IDSELECT);

  ay_status += ay_provide_register(ay_select_providecb, AY_IDSELECT);

  ay_status += ay_peek_register(ay_select_peekcb, AY_IDSELECT);

  ay_status += ay_notify_register(ay_select_notifycb, AY_IDSELECT);

 return ay_status;
} /* ay_select_init */
