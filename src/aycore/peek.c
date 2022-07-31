/*
 * Ayam, a free 3D modeler for the RenderMan interface.
 *
 * Ayam is copyrighted 1998-2021 by Randolf Schultz
 * (randolf.schultz@gmail.com) and others.
 *
 * All rights reserved.
 *
 * See the file License for details.
 *
 */

#include "ayam.h"

/* peek.c - functions for peek mechanism */

static double peek_trafos[32];

/** ay_peek_register:
 * register a peek callback
 *
 * \param[in] peekcb  peek callback
 * \param[in] type_id object  type for which to register the callback (AY_ID*)
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_peek_register(ay_peekcb *peekcb, unsigned int type_id)
{
 int ay_status = AY_OK;

  /* register peek callback */
  ay_status = ay_table_addcallback(&ay_peekcbt, (ay_voidfp)peekcb, type_id);

 return ay_status;
} /* ay_peek_register */


/** ay_peek_singleobject:
 * Convenience function for peeking a single object without transformations.
 *
 * \param[in] o  object to query
 * \param[in] type  target object type
 *
 * \returns reference to internal object of \a o or NULL upon failure
 */
ay_object *
ay_peek_singleobject(ay_object *o, unsigned int type)
{
 int ay_status = AY_OK;
 ay_voidfp *arr = NULL;
 ay_peekcb *cb = NULL;
 ay_providecb *provcb = NULL;
 ay_object *pobjects[2] = { NULL, ay_endlevel }, **singlepeek;

  singlepeek = pobjects;

  if(!o)
    return NULL;

  arr = ay_peekcbt.arr;
  cb = (ay_peekcb *)(arr[o->type]);

  if(cb)
    {
      ay_status = cb(o, type, singlepeek, &(peek_trafos[0]));

      if(pobjects[0])
	return pobjects[0];
    }
  else
    {
      arr = ay_providecbt.arr;
      provcb = (ay_providecb *)(arr[o->type]);
      if(provcb)
	{

	  ay_status = provcb(o, UINT_MAX-type, singlepeek);

	  if(ay_status == AY_OK)
	    return pobjects[0];
	}
    } /* if */

 return NULL;
} /* ay_peek_singleobject */


/** ay_peek_object:
 * call registered peek callback of an object
 * to obtain multiple object references of given type.
 * The result will be a freshly allocated array of references to the
 * internal data structure of the inquired object (or one of its children)
 * unless a pre-allocated array is provided.
 * The referenced data should be considered immutable.
 * The array will be terminated by a reference to \a ay_endlevel.
 * In addition, the transformations of the provided objects in relation to
 * the inquired objects can be recorded.
 *
 * Suggested/example usage:
 *
 * int i = 0;
 * ay_object *o, **peekobjs = NULL, *peekobj;
 * ay_status = ay_peek_object(o, AY_IDNPATCH, &peekobjs, NULL);
 * if(peekobjs)
 *   while(peekobjs[i] != ay_endlevel)
 *     {
 *       peekobj = peekobjs[i];
 *       // do something with peekobj
 *       i++;
 *     }
 * // when done, release the array
 * if(peekobjs)
 *   free(peekobjs);
 *
 * When just requesting a single object \a result can also be a
 * pre-allocated array that nevertheless needs to be terminated by
 * a reference to \a ay_endlevel, see the following example:
 *
 * ay_object *pobjects[2] = { NULL, ay_endlevel }, **singlepeek;
 * singlepeek = pobjects;
 * ay_status = ay_peek_object(o, AY_IDNPATCH, &singlepeek, NULL);
 *
 * Parameters:
 * \param[in] o  object to process
 * \param[in] type  desired type of object(s) to be provided by \a o (AY_ID*)
 * \param[in,out] objrefs  where to store the resulting array of references
 * \param[in,out] transforms  where to store the resulting array of
 *   transformation matrices (may be NULL)
 *
 * \returns AY_OK on success, error code otherwise. If \a objrefs is NULL,
 * the number of objects that a peek would deliver is returned.
 */
int
ay_peek_object(ay_object *o, unsigned int type, ay_object ***objrefs,
	       double **transforms)
{
 int ay_status = AY_OK;
 char fname[] = "peek";
 ay_voidfp *arr = NULL;
 ay_peekcb *cb = NULL;
 ay_providecb *provcb = NULL;
 int peekcount, freeobjrefs = AY_FALSE, freetrafos = AY_FALSE;
 int i = 0, j = 0, have_trafo = AY_FALSE;
 double tmo[16];

  if(!o)
    return AY_ENULL;

  arr = ay_peekcbt.arr;
  cb = (ay_peekcb *)(arr[o->type]);

  if(cb)
    {
      if(transforms && AY_ISTRAFO(o))
	{
	  have_trafo = AY_TRUE;
	  ay_trafo_creatematrix(o, tmo);
	}

      if(!objrefs || !*objrefs)
	{
	  peekcount = cb(o, type, NULL, NULL);

	  if(!objrefs)
	    return peekcount;

	  if(peekcount == 0)
	    return AY_OK;

	  if(!*objrefs)
	    {
	      if(!(*objrefs = calloc(peekcount+1, sizeof(ay_object *))))
		return AY_EOMEM;
	      (*objrefs)[peekcount] = ay_endlevel;
	      freeobjrefs = AY_TRUE;
	    }
	  if(transforms && !*transforms)
	    {
	      if(!(*transforms = malloc(peekcount*16*sizeof(double))))
		{
		  if(freeobjrefs)
		    {
		      free(*objrefs);
		      *objrefs = NULL;
		    }
		  return AY_EOMEM;
		}

	      for(i = 0; i < peekcount; i++)
		{
		  if(have_trafo)
		    memcpy(&((*transforms)[j]), tmo, 16*sizeof(double));
		  else
		    ay_trafo_identitymatrix(&((*transforms)[j]));
		  j += 16;
		}

	      freetrafos = AY_TRUE;
	    }
	} /* if count or need new arrays */

      if(!freetrafos && have_trafo && *transforms)
	{
	  peekcount = cb(o, type, NULL, NULL);
	  for(i = 0; i < peekcount; i++)
	    {
	      ay_trafo_multmatrix(&((*transforms)[j]), tmo);
	      j += 16;
	    }
	}

      if(transforms)
	ay_status = cb(o, type, *objrefs, *transforms);
      else
	ay_status = cb(o, type, *objrefs, &(peek_trafos[0]));

      if(ay_status)
	{
	  if(freeobjrefs)
	    free(*objrefs);
	  *objrefs = NULL;
	  if(freetrafos)
	    {
	      free(*transforms);
	      *transforms = NULL;
	    }
	  ay_error(AY_ERROR, fname, "peek callback failed");
	  return AY_ERROR;
	}
    }
  else
    {
      arr = ay_providecbt.arr;
      provcb = (ay_providecb *)(arr[o->type]);
      if(provcb)
	{
	  if(!objrefs)
	    {
	      ay_status = provcb(o, UINT_MAX-type, NULL);
	      if(ay_status == AY_OK)
		peekcount = 1;
	      else
		peekcount = 0;

	      return peekcount;
	    }

	  if(!*objrefs)
	    {
	      if(!(*objrefs = calloc(2, sizeof(ay_object *))))
		return AY_EOMEM;
	      (*objrefs)[1] = ay_endlevel;
	      freeobjrefs = AY_TRUE;
	    }
	  if(transforms && !*transforms)
	    {
	      if(!(*transforms = malloc(16*sizeof(double))))
		{
		  if(freeobjrefs)
		    {
		      free(*objrefs);
		      *objrefs = NULL;
		    }
		  return AY_EOMEM;
		}
	    }
	  ay_status = provcb(o, UINT_MAX-type, *objrefs);

	  if(ay_status == AY_OK)
	    {
	      if(transforms)
		{
		  if(*transforms && AY_ISTRAFO(o))
		    {
		      ay_trafo_creatematrix(o, *transforms);
		    }
		  else
		    {
		      ay_trafo_identitymatrix(*transforms);
		    }
		}
	    }
	  else
	    {
	      if(freeobjrefs)
		free(*objrefs);
	      *objrefs = NULL;
	    }
	}
      else
	{
	  if(!objrefs)
	    {
	      /* caller just wants to test ability to peek
		 and this object has no peek and no provide callback... */
	      return 0;
	    }
	} /* if */
    } /* if */

 return AY_OK;
} /* ay_peek_object */


/** ay_peek_settag
 * Set a peek tag.
 *
 * \param[in] o  object the tag shall point to
 * \param[in,out] p  object to receive the tag
 *
 * \returns AY_OK on success, error code otherwise
 */
int
ay_peek_settag(ay_object *o, ay_object *p)
{
 int ay_status = AY_OK;
 ay_tag *tag = NULL;

  if(!o || !p)
    return AY_ENULL;

  tag = p->tags;

  while(tag)
    {
      if(tag->type == ay_peek_tagtype)
	break;
      tag = tag->next;
    }

  if(!tag)
    {
      if(!(tag = calloc(1, sizeof(ay_tag))))
	return AY_EOMEM;

      if(!(tag->name = malloc(strlen(ay_peek_tagname)+1)))
	{
	  free(tag);
	  return AY_EOMEM;
	}
      strcpy(tag->name, ay_peek_tagname);

      if(!(tag->val = calloc(1, sizeof(ay_btval))))
	{
	  free(tag->name);
	  free(tag);
	  return AY_EOMEM;
	}

      tag->is_intern = AY_TRUE;
      tag->is_binary = AY_TRUE;

      tag->next = p->tags;
      p->tags = tag;
    }

  ((ay_btval*)tag->val)->payload = o;

 return ay_status;
} /* ay_peek_settag */
