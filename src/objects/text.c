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
#include "tti.h"

/* text.c - text object */

static char *ay_text_name = "Text";


/* prototypes of functions local to this module: */

int ay_text_getpntcb(int mode, ay_object *o, double *p, ay_pointedit *pe);

int ay_text_notifycb(ay_object *o);


/* functions: */

/* ay_text_createcb:
 *  create callback function of text object
 */
int
ay_text_createcb(int argc, char *argv[], ay_object *o)
{
 int ay_status = AY_OK;
 int tcl_status = TCL_OK;
 char fname[] = "crttext";
 char option_handled = AY_FALSE;
 int optnum = 0, i = 2, j, k, numchars;
 double height = 0.5;
 char *font = NULL, *utfptr;
 Tcl_UniChar *unistring = NULL, *uniptr;
 ay_text_object *text = NULL;

  if(!o)
    return AY_ENULL;

  /* parse args */
  while(i+1 < argc)
    {
      if(i+1 >= argc)
	{
	  ay_error(AY_EOPT, fname, argv[i]);
	  ay_status = AY_ERROR;
	  goto cleanup;
	}

      tcl_status = TCL_OK;
      option_handled = AY_FALSE;
      optnum = i;
      if(argv[i] && argv[i][0] != '\0')
	{
	  switch(argv[i][1])
	    {
	    case 'f':
	      /* -font */
	      if(font)
		free(font);
	      if(!(font = calloc(strlen(argv[i+1])+1, sizeof(char))))
		{
		  ay_error(AY_EOMEM, fname, NULL);
		  ay_status = AY_ERROR;
		  goto cleanup;
		}
	      strcpy(font, argv[i+1]);
	      option_handled = AY_TRUE;
	      break;
	    case 'h':
	      /* -height */
	      tcl_status = Tcl_GetDouble(ay_interp, argv[i+1], &height);
	      option_handled = AY_TRUE;
	      break;
	    case 't':
	      /* -text */
	      numchars = Tcl_NumUtfChars(argv[i+1], -1);
	      if(unistring)
		free(unistring);
	      if(!(unistring = calloc(numchars+1, sizeof(Tcl_UniChar))))
		{
		  ay_error(AY_EOMEM, fname, NULL);
		  ay_status = AY_ERROR;
		  goto cleanup;
		}
	      utfptr = argv[i+1];
	      uniptr = unistring;
	      for(j = 0; j < numchars; j++)
		{
		  k = Tcl_UtfToUniChar(utfptr, uniptr);
		  utfptr += k;
		  uniptr++;
		}
	      option_handled = AY_TRUE;
	      break;
	    default:
	      break;
	    } /* switch */

	  if(option_handled && (tcl_status != TCL_OK))
	    {
	      ay_error(AY_EOPT, fname, argv[i]);
	      ay_status = AY_ERROR;
	      goto cleanup;
	    }

	  i += 2;
	}
      else
	{
	  i++;
	} /* if */

      if(!option_handled)
	{
	  ay_error(AY_EUOPT, fname, argv[optnum]);
	  ay_status = AY_ERROR;
	  goto cleanup;
	}

    } /* while */

  if(!(text = calloc(1, sizeof(ay_text_object))))
    {
      ay_error(AY_EOMEM, fname, NULL);
      goto cleanup;
    }

  text->height = height;
  text->fontname = font;
  text->unistring = unistring;

  o->refine = text;

  /* prevent cleanup code from doing something harmful */
  font = NULL;
  unistring = NULL;

cleanup:

  if(font)
    free(font);

  if(unistring)
    free(unistring);

 return ay_status;
} /* ay_text_createcb */


/* ay_text_deletecb:
 *  delete callback function of text object
 */
int
ay_text_deletecb(void *c)
{
 ay_text_object *text = NULL;

  if(!c)
    return AY_ENULL;

  text = (ay_text_object *)(c);

  if(text->npatch)
    (void)ay_object_deletemulti(text->npatch, AY_FALSE);

  if(text->caps_and_bevels)
    (void)ay_object_deletemulti(text->caps_and_bevels, AY_FALSE);

  if(text->fontname)
    free(text->fontname);

  if(text->unistring)
    free(text->unistring);

  /* free read only points */
  if(text->pnts)
    free(text->pnts);

  free(text);

 return AY_OK;
} /* ay_text_deletecb */


/* ay_text_copycb:
 *  copy callback function of text object
 */
int
ay_text_copycb(void *src, void **dst)
{
 ay_text_object *tdst = NULL, *tsrc = NULL;

  tsrc = (ay_text_object *)src;

  if(!(tdst = malloc(sizeof(ay_text_object))))
    return AY_EOMEM;

  memcpy(tdst, tsrc, sizeof(ay_text_object));

  tdst->npatch = NULL;
  tdst->caps_and_bevels = NULL;
  tdst->fontname = NULL;
  tdst->unistring = NULL;

  tdst->pnts = NULL;
  tdst->pntslen = 0;

  if(tsrc->fontname)
    {
      if(!(tdst->fontname = malloc((strlen(tsrc->fontname)+1) * sizeof(char))))
	{
	  free(tdst);
	  return AY_EOMEM;
	}
      strcpy(tdst->fontname, tsrc->fontname);
    }

  if(tsrc->unistring)
    {
      if(!(tdst->unistring = malloc((Tcl_UniCharLen(tsrc->unistring)+1) *
				    sizeof(Tcl_UniChar))))
	{
	  if(tdst->fontname)
	    free(tdst->fontname);
	  free(tdst);
	  return AY_EOMEM;
	}

      memcpy(tdst->unistring, tsrc->unistring,
	     Tcl_UniCharLen(tsrc->unistring)*sizeof(Tcl_UniChar));

      memset(&(tdst->unistring[Tcl_UniCharLen(tsrc->unistring)]),0,
			       sizeof(Tcl_UniChar));
    }

  *dst = tdst;

 return AY_OK;
} /* ay_text_copycb */


/* ay_text_drawcb:
 *  draw (display in an Ayam view window) callback function of text object
 */
int
ay_text_drawcb(struct Togl *togl, ay_object *o)
{
 ay_text_object *text = NULL;
 ay_object *npatch;

  if(!o)
    return AY_ENULL;

  text = (ay_text_object *)(o->refine);

  if(!text)
    return AY_ENULL;

  npatch = text->npatch;

  while(npatch)
    {
      ay_draw_object(togl, npatch, AY_TRUE);
      npatch = npatch->next;
    }

  npatch = text->caps_and_bevels;
  while(npatch)
    {
      ay_draw_object(togl, npatch, AY_TRUE);
      npatch = npatch->next;
    }

 return AY_OK;
} /* ay_text_drawcb */


/* ay_text_shadecb:
 *  shade (display in an Ayam view window) callback function of text object
 */
int
ay_text_shadecb(struct Togl *togl, ay_object *o)
{
 ay_text_object *text = NULL;
 ay_object *npatch;

  if(!o)
    return AY_ENULL;

  text = (ay_text_object *)(o->refine);

  if(!text)
    return AY_ENULL;

  npatch = text->npatch;

  while(npatch)
    {
      ay_shade_object(togl, npatch, AY_FALSE);
      npatch = npatch->next;
    }

  npatch = text->caps_and_bevels;

  while(npatch)
    {
      ay_shade_object(togl, npatch, AY_FALSE);
      npatch = npatch->next;
    }

 return AY_OK;
} /* ay_text_shadecb */


/* ay_text_drawhcb:
 *  draw handles (in an Ayam view window) callback function of text object
 */
int
ay_text_drawhcb(struct Togl *togl, ay_object *o)
{
 unsigned int i;
 double *pnts;
 ay_text_object *text;

  if(!o)
    return AY_ENULL;

  text = (ay_text_object *)o->refine;

  if(!text)
    return AY_ENULL;

  if(!text->pnts)
    {
      text->pntslen = 1;
      ay_text_notifycb(o);
    }

  if(text->pnts)
    {
      pnts = text->pnts;

      glColor3f((GLfloat)ay_prefs.obr, (GLfloat)ay_prefs.obg,
		(GLfloat)ay_prefs.obb);

      glBegin(GL_POINTS);
       for(i = 0; i < text->pntslen; i++)
	 {
	   glVertex3dv((GLdouble *)pnts);
	   pnts += 4;
	 }
      glEnd();

      glColor3f((GLfloat)ay_prefs.ser, (GLfloat)ay_prefs.seg,
		(GLfloat)ay_prefs.seb);
    }

 return AY_OK;
} /* ay_text_drawhcb */


/* ay_text_getpntcb:
 *  get point (editing and selection) callback function of text object
 */
int
ay_text_getpntcb(int mode, ay_object *o, double *p, ay_pointedit *pe)
{
 ay_text_object *text = NULL;

  if(!o)
    return AY_ENULL;

  text = (ay_text_object *)o->refine;

  if(!text)
    return AY_ENULL;

  if(!text->pnts)
    {
      text->pntslen = 1;
      ay_text_notifycb(o);
    }

  if(text->pntslen)
    return ay_selp_getpnts(mode, o, p, pe, 1, text->pntslen, 4,
			   ay_prefs.rationalpoints, text->pnts);
  else
    return AY_OK;
} /* ay_text_getpntcb */


/* ay_text_setpropcb:
 *  set property (from Tcl to C context) callback function of text object
 */
int
ay_text_setpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
 /*int ay_status = AY_OK;*/
 char fname[] = "setProp";
 char *arr = "TextAttrData";
 const char *newfont;
 Tcl_Obj *to = NULL;
 ay_text_object *text = NULL;
 Tcl_UniChar *unistr = NULL;

  if(!interp || !o)
    return AY_ENULL;

  text = (ay_text_object *)o->refine;

  if(!text)
    return AY_ENULL;

  if(text->fontname)
    {
      free(text->fontname);
      text->fontname = NULL;
    }

  newfont = Tcl_GetVar2(interp, arr, "FontName",
			TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  if(!(text->fontname = calloc(strlen(newfont)+1, sizeof(char))))
    {
      ay_error(AY_EOMEM, fname, NULL);
      return AY_ERROR;
    }
  strcpy(text->fontname, newfont);

  to = Tcl_GetVar2Ex(interp, arr, "Height",
		     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
  Tcl_GetDoubleFromObj(interp, to, &(text->height));

  to = Tcl_GetVar2Ex(interp, arr, "String",
		     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
  unistr = Tcl_GetUnicode(to);

  if(text->unistring)
    {
      free(text->unistring);
      text->unistring = NULL;
    }

  if(!(text->unistring = calloc(Tcl_UniCharLen(unistr)+1,
				sizeof(Tcl_UniChar))))
    {
      ay_error(AY_EOMEM, fname, NULL);
      return AY_ERROR;
    }
  memcpy(text->unistring, unistr, Tcl_UniCharLen(unistr)*sizeof(Tcl_UniChar));


  to = Tcl_GetVar2Ex(interp, arr, "Revert",
		     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
  Tcl_GetIntFromObj(interp, to, &(text->revert));

  to = Tcl_GetVar2Ex(interp, arr, "UpperCap",
		     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
  Tcl_GetIntFromObj(interp, to, &(text->has_upper_cap));

  to = Tcl_GetVar2Ex(interp, arr, "LowerCap",
		     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
  Tcl_GetIntFromObj(interp, to, &(text->has_lower_cap));

  to = Tcl_GetVar2Ex(interp, arr, "Tolerance",
		     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
  Tcl_GetDoubleFromObj(interp, to, &(text->glu_sampling_tolerance));

  to = Tcl_GetVar2Ex(interp, arr, "DisplayMode",
		     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
  Tcl_GetIntFromObj(interp, to, &(text->display_mode));

  ay_notify_object(o);

  o->modified = AY_TRUE;

  ay_notify_parent();

 return AY_OK;
} /* ay_text_setpropcb */


/* ay_text_getpropcb:
 *  get property (from C to Tcl context) callback function of text object
 */
int
ay_text_getpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
  /*int ay_status = AY_OK;*/
 char *arr = "TextAttrData";
 char emptystring[] = "";
 ay_text_object *text = NULL;
 Tcl_Obj *to = NULL;

  if(!interp || !o)
    return AY_ENULL;

  text = (ay_text_object *)o->refine;

  if(!text)
    return AY_ENULL;

  Tcl_SetVar2Ex(interp, arr, "FontName",
		Tcl_NewStringObj(text->fontname, -1),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  if(text->unistring && (text->unistring[0] != 0))
    to = Tcl_NewUnicodeObj(text->unistring, Tcl_UniCharLen(text->unistring));
  else
    to = Tcl_NewStringObj(emptystring, -1);
  Tcl_SetVar2Ex(interp, arr, "String", to,
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);


  Tcl_SetVar2Ex(interp, arr, "Height",
		Tcl_NewDoubleObj(text->height),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Revert",
		Tcl_NewIntObj(text->revert),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "UpperCap",
		Tcl_NewIntObj(text->has_upper_cap),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "LowerCap",
		Tcl_NewIntObj(text->has_lower_cap),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Tolerance",
		Tcl_NewDoubleObj(text->glu_sampling_tolerance),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "DisplayMode",
		Tcl_NewIntObj(text->display_mode),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

 return AY_OK;
} /* ay_text_getpropcb */


/* ay_text_readcb:
 *  read (from scene file) callback function of text object
 */
int
ay_text_readcb(FILE *fileptr, ay_object *o)
{
 ay_text_object *text = NULL;
 int ay_status = AY_OK;
 ay_tag tag = {0}, *stag = NULL, *etag = NULL;
 char vbuf[128], nbuf[3] = "BP";
 int has_startb = AY_FALSE, has_startb2 = AY_FALSE;
 int has_endb = AY_FALSE, has_endb2 = AY_FALSE, startb_revert2;
 int startb_type, startb_type2, endb_type, startb_sense, endb_sense;
 double startb_radius, startb_radius2, endb_radius;

  if(!fileptr || !o)
    return AY_ENULL;

  if(!(text = calloc(1, sizeof(ay_text_object))))
    return AY_EOMEM;

  fscanf(fileptr, "%lg", &text->height);
  (void)fgetc(fileptr);

  ay_status = ay_read_string(fileptr, &(text->fontname));
  if(ay_status)
    {
      free(text);
      return ay_status;
    }
  ay_read_unistring(fileptr, &(text->unistring));

  fscanf(fileptr, "%d\n", &text->revert);
  fscanf(fileptr, "%d\n", &text->has_upper_cap);
  fscanf(fileptr, "%d\n", &text->has_lower_cap);
  fscanf(fileptr, "%d\n", &has_endb2);
  fscanf(fileptr, "%d\n", &has_startb2);
  fscanf(fileptr, "%d\n", &startb_type2);
  fscanf(fileptr, "%lg\n", &startb_radius2);
  fscanf(fileptr, "%d\n", &startb_revert2);
  fscanf(fileptr, "%d\n", &text->display_mode);
  fscanf(fileptr, "%lg\n", &text->glu_sampling_tolerance);

  /* get bevel parameters from potentially present BP tags */
  ay_npt_getbeveltags(o, 0, &has_startb, &startb_type, &startb_radius,
		      &startb_sense);
  ay_npt_getbeveltags(o, 1, &has_endb, &endb_type, &endb_radius,
		      &endb_sense);

  tag.name = nbuf;
  tag.type = ay_bp_tagtype;
  tag.val = vbuf;
  if(!has_startb && has_startb2)
    {
      sprintf(vbuf, "0,%d,%g,0", startb_type2, startb_radius2);
      ay_tags_copy(&tag, &stag);
      ay_tags_append(o, stag);
    }

  if(!has_endb && has_endb2)
    {
      sprintf(vbuf, "1,%d,%g,0", startb_type2, startb_radius2);
      ay_tags_copy(&tag, &etag);
      ay_tags_append(o, etag);
    }

  o->refine = text;

 return AY_OK;
} /* ay_text_readcb */


/* ay_text_writecb:
 *  write (to scene file) callback function of text object
 */
int
ay_text_writecb(FILE *fileptr, ay_object *o)
{
 ay_text_object *text = NULL;
 Tcl_UniChar *uc = NULL;
 int has_startb = AY_FALSE, has_endb = AY_FALSE;
 int startb_type = 0, endb_type = 0, startb_sense = 0, endb_sense = 0;
 double startb_radius = 0.0, endb_radius = 0.0;

  if(!fileptr || !o)
    return AY_ENULL;

  text = (ay_text_object *)(o->refine);

  if(!text)
    return AY_ENULL;

  /* get bevel parameters */
  ay_npt_getbeveltags(o, 0, &has_startb, &startb_type, &startb_radius,
		      &startb_sense);
  ay_npt_getbeveltags(o, 1, &has_endb, &endb_type, &endb_radius,
		      &endb_sense);

  fprintf(fileptr, "%g\n", text->height);

  if(!text->fontname || (text->fontname[0] == '\0'))
    fprintf(fileptr, "\n");
  else
    fprintf(fileptr, "%s\n", text->fontname);

  if(!text->unistring || (text->unistring[0] == 0))
    {
      fprintf(fileptr, "\n");
    }
  else
    {
      uc = text->unistring;
      while(*uc != 0)
	{
	  fprintf(fileptr, "%d ", (int)*uc);
	  uc++;
	}
      fprintf(fileptr, "\n");
    }

  fprintf(fileptr, "%d\n", text->revert);
  fprintf(fileptr, "%d\n", text->has_upper_cap);
  fprintf(fileptr, "%d\n", text->has_lower_cap);

  fprintf(fileptr, "%d\n", has_endb);
  fprintf(fileptr, "%d\n", has_startb);
  fprintf(fileptr, "%d\n", startb_type);
  fprintf(fileptr, "%g\n", startb_radius);
  fprintf(fileptr, "%d\n", startb_sense);

  fprintf(fileptr, "%d\n", text->display_mode);
  fprintf(fileptr, "%g\n", text->glu_sampling_tolerance);

 return AY_OK;
} /* ay_text_writecb */


/* ay_text_wribcb:
 *  RIB export callback function of text object
 */
int
ay_text_wribcb(char *file, ay_object *o)
{
 ay_text_object *text = NULL;
 ay_object *p = NULL;

  if(!o)
   return AY_ENULL;

  text = (ay_text_object*)o->refine;

  if(!text)
    return AY_ENULL;

  p = text->npatch;
  while(p)
    {
      ay_wrib_toolobject(file, p, o);
      p = p->next;
    }

  p = text->caps_and_bevels;
  while(p)
    {
      ay_wrib_toolobject(file, p, o);
      p = p->next;
    }

 return AY_OK;
} /* ay_text_wribcb */


/* ay_text_bbccb:
 *  bounding box calculation callback function of text object
 */
int
ay_text_bbccb(ay_object *o, double *bbox, int *flags)
{
 ay_text_object *text = NULL;
 ay_object *p = NULL;
 double bbt[24] = {0};
 double xmin = DBL_MAX, xmax = -DBL_MAX, ymin = DBL_MAX;
 double ymax = -DBL_MAX, zmin = DBL_MAX, zmax = -DBL_MAX;

  if(!o || !bbox || !flags)
    return AY_ENULL;

  text = (ay_text_object *)o->refine;

  if(!text)
    return AY_ENULL;

  if(text->npatch)
    {
      p = text->npatch;

      while(p->next)
	{
	  ay_bbc_get(p, bbt);

	  if(bbt[0] < xmin)
	    xmin = bbt[0];
	  if(bbt[6] > xmax)
	    xmax = bbt[6];
	  if(bbt[13] < ymin)
	    ymin = bbt[13];
	  if(bbt[1] > ymax)
	    ymax = bbt[1];
	  if(bbt[5] < zmin)
	    zmin = bbt[5];
	  if(bbt[2] > zmax)
	    zmax = bbt[2];

	  p = p->next;
	} /* while */

      /* fill in results */
      /* P1 */
      bbox[0] = xmin; bbox[1] = ymax; bbox[2] = zmax;
      /* P2 */
      bbox[3] = xmin; bbox[4] = ymax; bbox[5] = zmin;
      /* P3 */
      bbox[6] = xmax; bbox[7] = ymax; bbox[8] = zmin;
      /* P4 */
      bbox[9] = xmax; bbox[10] = ymax; bbox[11] = zmax;

      /* P5 */
      bbox[12] = xmin; bbox[13] = ymin; bbox[14] = zmax;
      /* P6 */
      bbox[15] = xmin; bbox[16] = ymin; bbox[17] = zmin;
      /* P7 */
      bbox[18] = xmax; bbox[19] = ymin; bbox[20] = zmin;
      /* P8 */
      bbox[21] = xmax; bbox[22] = ymin; bbox[23] = zmax;

      *flags = 1;
    }
  else
    {
      /* invalid/nonexisting bbox */
      *flags = 2;
    }

 return AY_OK;
} /* ay_text_bbccb */


/* ay_text_notifycb:
 *  notification callback function of text object
 */
int
ay_text_notifycb(ay_object *o)
{
 int ay_status = AY_OK;
 char fname[] = "text_notifycb";
 int tti_status = 0;
 ay_text_object *text = NULL;
 ay_tti_letter letter = {0};
 ay_tti_outline *outline;
 ay_object *curve = NULL, *newcurve = NULL;
 ay_object *holes = NULL, **nexthole = NULL;
 ay_object *patch, **nextnpatch, **nextcb;
 ay_object ext = {0}, endlevel = {0};
 ay_extrude_object extrude = {0};
 Tcl_UniChar *uc;
 int i;
 double xoffset = 0.0, yoffset = 0.0;

  if(!o)
    return AY_ENULL;

  text = (ay_text_object *)o->refine;

  if(!text)
    return AY_ENULL;

  if(text->npatch)
    ay_status = ay_object_deletemulti(text->npatch, AY_FALSE);
  text->npatch = NULL;
  nextnpatch = &(text->npatch);

  if(text->caps_and_bevels)
    ay_status = ay_object_deletemulti(text->caps_and_bevels, AY_FALSE);
  text->caps_and_bevels = NULL;
  nextcb = &(text->caps_and_bevels);

  /* always clear the old read only points */
  if(text->pnts)
    {
      free(text->pnts);
      text->pnts = NULL;
    }

  if(!text->fontname || (text->fontname[0] == '\0') ||
     !text->unistring || (text->unistring[0] == 0))
    goto cleanup;

  ext.type = AY_IDEXTRUDE;

  ext.tags = o->tags;

  ext.refine = &extrude;

  extrude.height = text->height;
  extrude.has_lower_cap = text->has_lower_cap;
  extrude.has_upper_cap = text->has_upper_cap;

  extrude.display_mode = text->display_mode;
  extrude.glu_sampling_tolerance = text->glu_sampling_tolerance;

  uc = text->unistring;
  while(*uc != 0)
    {
      /*0x3071*/
      tti_status = ay_tti_getcurves(text->fontname, *uc, &letter);

      if(tti_status)
	{
	  ay_error(AY_ERROR, fname, "could not get curves from font:");
	  switch(tti_status)
	    {
	    case AY_TTI_NOMEM:
	      ay_error(AY_EOMEM, fname, NULL);
	      break;
	    case AY_TTI_NOTFOUND:
	      ay_error(AY_EOPENFILE, fname, text->fontname);
	      break;
	    case AY_TTI_BADFONT:
	      ay_error(AY_ERROR, fname, "bad font file, need TTF");
	      break;
	    default:
	      break;
	    } /* switch */
	  ay_status = AY_ERROR;
	  goto cleanup;
	} /* if */

      if(letter.numoutlines > 0)
	{
	  nexthole = &holes;
	  curve = NULL;
	  holes = NULL;
	  for(i = 0; i < letter.numoutlines; i++)
	    {
	      outline = &((letter.outlines)[i]);

	      newcurve = NULL;
	      ay_status = ay_tti_outlinetoncurve(outline, &newcurve);
	      if(ay_status || !newcurve)
		{
		  ay_error(AY_ERROR, fname, "failed to convert outline:");
		  ay_error(ay_status, fname, NULL);
		  continue;
		}

	      newcurve->movx = xoffset;
	      newcurve->movy = yoffset;

	      if((text->revert?(outline->filled):(!outline->filled)))
		{
		  /* this outline is a true outline */

		  /* if there is already an outline in <curve>
		     and possibly holes in <holes>
		     we need to create patches from them now */
		  if(curve)
		    {
		      if(holes)
			{
			  /* link the holes to the curve */
			  curve->next = holes;
			  /* terminate hierarchy */
			  *nexthole = &endlevel;
			}
		      else
			{
			  /* terminate hierarchy */
			  curve->next = &endlevel;
			}
		      ext.down = curve;
		      extrude.npatch = NULL;
		      extrude.caps_and_bevels = NULL;
		      ay_notify_object(&ext);

		      if(extrude.npatch)
			{
			  *nextnpatch = extrude.npatch;
			  patch = extrude.npatch;
			  while(patch)
			    {
			      nextnpatch = &(patch->next);
			      patch = patch->next;
			    } /* while */
			} /* if */

		      if(extrude.caps_and_bevels)
			{
			  *nextcb = extrude.caps_and_bevels;
			  patch = extrude.caps_and_bevels;
			  while(patch)
			    {
			      nextcb = &(patch->next);
			      patch = patch->next;
			    } /* while */
			} /* if */

		      /* free curve objects */
		      if(holes)
			{
			  curve->next = NULL;
			  *nexthole = NULL;
			  (void)ay_object_deletemulti(holes, AY_FALSE);
			  holes = NULL;
			  nexthole = &(holes);
			}
		      (void)ay_object_delete(curve);
		    } /* if(curve */

		  curve = newcurve;
		}
	      else
		{
		  /* this "outline" is a hole, which we just append to the
		     list of holes in <holes> */
		  *nexthole = newcurve;
		  nexthole = &(newcurve->next);
		} /* if */

	      if((i == letter.numoutlines-1) && (curve))
		{
		  /* end of loop reached, but there is still an unconverted
		     outline in <curve> => convert it to patches now */
		  if(holes)
		    {
		      /* link the holes to the curve */
		      curve->next = holes;
		      /* terminate hierarchy */
		      *nexthole = &endlevel;
		    }
		  else
		    {
		      /* terminate hierarchy */
		      curve->next = &endlevel;
		    }
		  ext.down = curve;
		  extrude.npatch = NULL;
		  extrude.caps_and_bevels = NULL;
		  ay_notify_object(&ext);

		  if(extrude.npatch)
		    {
		      *nextnpatch = extrude.npatch;
		      patch = extrude.npatch;
		      while(patch)
			{
			  nextnpatch = &(patch->next);
			  patch = patch->next;
			} /* while */
		    } /* if */

		  if(extrude.caps_and_bevels)
		    {
		      *nextcb = extrude.caps_and_bevels;
		      patch = extrude.caps_and_bevels;
		      while(patch)
			{
			  nextcb = &(patch->next);
			  patch = patch->next;
			} /* while */
		    } /* if */

		  /* free curve objects */
		  if(holes)
		    {
		      curve->next = NULL;
		      *nexthole = NULL;
		      (void)ay_object_deletemulti(holes, AY_FALSE);
		      holes = NULL;
		      nexthole = &(holes);
		    }

		  (void)ay_object_delete(curve);
		  curve = NULL;
		} /* if */

	      if((i == letter.numoutlines-1) && (holes))
		{
		  /* end of loop reached, but there are unconverted holes;
		     this is probably caused by broken orientation detection;
		     we need to free all the curves in <holes> (and inform the
		     user?) */
		  ay_error(AY_EWARN, fname,
		     "Could not convert all outlines, please try Revert.");
		  (void)ay_object_deletemulti(holes, AY_FALSE);
		  holes = NULL;
		  nexthole = &(holes);
		}

	    } /* for */
	} /* if */

      xoffset = letter.xoffset;
      /*yoffset = letter.yoffset;*/

      for(i = 0; i < letter.numoutlines; i++)
	{
	  outline = &((letter.outlines)[i]);
	  if(outline->points)
	    free(outline->points);
	}
      if(letter.outlines)
	free(letter.outlines);
      letter.outlines = NULL;

      uc++;
    } /* while */

  if(text->pntslen && text->npatch)
    {
      ay_selp_managelist(text->npatch, &text->pntslen, &text->pnts);
    }

cleanup:

  /* free (temporary) font structure */
  (void)ay_tti_getcurves(NULL, 0, NULL);

  /* correct any inconsistent values of pnts and pntslen */
  if(text->pntslen && !text->pnts)
    {
      text->pntslen = 0;
    }

  /* recover selected points */
  if(o->selp)
    {
      if(text->npatch)
	ay_text_getpntcb(3, o, NULL, NULL);
      else
	ay_selp_clear(o);
    }

 return ay_status;
} /* ay_text_notifycb */


/* ay_text_convertcb:
 *  convert callback function of text object
 */
int
ay_text_convertcb(ay_object *o, int in_place)
{
 ay_text_object *text = NULL;

  if(!o)
    return AY_ENULL;

  text = (ay_text_object *) o->refine;

  if(!text)
    return AY_ENULL;

 return ay_convert_nptoolobj(o, text->npatch, text->caps_and_bevels,
			     in_place);
} /* ay_text_convertcb */


/* ay_text_providecb:
 *  provide callback function of text object
 */
int
ay_text_providecb(ay_object *o, unsigned int type, ay_object **result)
{
 ay_text_object *text = NULL;

  if(!o)
    return AY_ENULL;

  text = (ay_text_object *) o->refine;

  if(!text)
    return AY_ENULL;

 return ay_provide_nptoolobj(o, type, text->npatch, text->caps_and_bevels,
			     result);
} /* ay_text_providecb */


/* ay_text_peekcb:
 *  peek callback function of text object
 */
int
ay_text_peekcb(ay_object *o, unsigned int type, ay_object **result,
	       double *transform)
{
 int ay_status = AY_OK;
 unsigned int i = 0, count = 0;
 ay_text_object *text = NULL;
 ay_object *npatch = NULL, **results;
 double tm[16];

  if(!o)
    return AY_ENULL;

  text = (ay_text_object *) o->refine;

  if(!text)
    return AY_ENULL;

  if(!result)
    {
      npatch = text->npatch;
      while(npatch)
	{
	  if(npatch->type == type)
	    {
	      count++;
	    }
	  npatch = npatch->next;
	} /* while */

      return count;
    } /* if */

  if(!text->npatch)
    return AY_OK;

  results = result;
  npatch = text->npatch;
  while(npatch && (results[i] != ay_endlevel))
    {
      if(npatch->type == type)
	{
	  results[i] = npatch;
	  if(transform && AY_ISTRAFO(npatch))
	    {
	      ay_trafo_creatematrix(npatch, tm);
	      ay_trafo_multmatrix(&(transform[i*16]), tm);
	    }
	  i++;
	}

      npatch = npatch->next;
    } /* while */

 return ay_status;
} /* ay_text_peekcb */


/* ay_text_init:
 *  initialize the text object module
 */
int
ay_text_init(Tcl_Interp *interp)
{
 int ay_status = AY_OK;

  ay_status = ay_otype_registercore(ay_text_name,
				    ay_text_createcb,
				    ay_text_deletecb,
				    ay_text_copycb,
				    ay_text_drawcb,
				    ay_text_drawhcb,
				    ay_text_shadecb,
				    ay_text_setpropcb,
				    ay_text_getpropcb,
				    ay_text_getpntcb,
				    ay_text_readcb,
				    ay_text_writecb,
				    ay_text_wribcb,
				    ay_text_bbccb,
				    AY_IDTEXT);


  ay_status += ay_notify_register(ay_text_notifycb, AY_IDTEXT);

  ay_status += ay_convert_register(ay_text_convertcb, AY_IDTEXT);

  ay_status += ay_provide_register(ay_text_providecb, AY_IDTEXT);

  ay_status += ay_peek_register(ay_text_peekcb, AY_IDTEXT);

 return ay_status;
} /* ay_text_init */
