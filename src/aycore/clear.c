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

/* clear.c - clear scenes */

/** ay_clear_scene:
 * Clears the current scene.
 *
 */
int
ay_clear_scene(void)
{
 int ay_status = AY_OK;
 char fname[] = "clear_scene";
 ay_object *o = NULL, *onext = NULL, **p;
 ay_root_object *root = NULL;
 char *ucargs[3] = {0}, ucarg0[] = "undo", ucarg1[] = "clear";

  /* clear undo buffer */
  /*ay_status = ay_undo_clear();*/
  ucargs[0] = ucarg0;
  ucargs[1] = ucarg1;
  ay_undo_undotcmd(NULL, ay_interp, 2, ucargs);

  /* clear instances of masters in the scene from clipboard */
  ay_instt_clearclipboard(ay_root);

  /* remove all instance objects from the scene */
  ay_instt_removeinstances(&ay_root, NULL);

  /* for all material objects in scene:
   * - remove references to them from clipboard
   * - additionally, deregister those materials
   *   (this keeps them from trying to remove references to
   *    them while delete operation is in progress, which
   *    would make crash us badly)
   */
  ay_matt_removecliprefs(ay_root);

  /* remove all references from normal objects to material objects */
  ay_matt_removeallrefs(ay_root);

  /* remove all objects now, no unlink necessary */
  o = ay_root->next;
  while(o->next)
    {
      onext = o->next;

      ay_status = ay_object_delete(o);

      if(ay_status != AY_OK)
	{
	  ay_error(AY_ERROR, fname, "Error removing object! Continuing...\n");
	}
      o = onext;
    }
  ay_root->next = o;

  /* clear the views level */
  o = ay_root->down;
  p = &(ay_root->down);
  while(o->next)
    {
      onext = o->next;
      if(o->type != AY_IDVIEW)
	{
	  /* unlink o */
	  *p = o->next;

	  ay_status = ay_object_delete(o);

	  if(ay_status != AY_OK)
	    {
	      ay_error(AY_ERROR, fname,
		       "Error removing object! Continuing...\n");
	    }
	}
      else
	{
	  p = &(o->next);
	}
      o = onext;
    }

  /* clear selection */
  ay_sel_free(AY_FALSE);

  /* clear current level */
  ay_clevel_gotop();

  /* free shaders from root object */
  root = (ay_root_object *)ay_root->refine;
  if(root->atmosphere)
    {
      ay_shader_free(root->atmosphere);
      root->atmosphere = NULL;
    }

  if(root->imager)
    {
      ay_shader_free(root->imager);
      root->imager = NULL;
    }

  ay_prefs.save_rootviews = AY_TRUE;

 return AY_OK;
} /* ay_clear_scene */


/** ay_clear_scenetcmd:
 * Clears the current scene.
 *
 *  Implements the \a newScene scripting interface command.
 *  See also the corresponding section in the \ayd{scnewscene}.
 *
 *  \returns TCL_OK in any case.
 */
int
ay_clear_scenetcmd(ClientData clientData, Tcl_Interp * interp,
		   int argc, char *argv[])
{
 int ay_status = AY_OK;

  ay_status = ay_clear_scene();
  if(ay_status)
    {
      ay_error(ay_status, argv[0], "Could not clear scene!");
    }

 return TCL_OK;
} /* ay_clear_scenetcmd */
