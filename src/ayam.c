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

/* ayam.c - the main Ayam module - initialization and global symbols */

/* global symbols */

Tcl_Interp *ay_interp;
Tcl_Interp *ay_safeinterp;

ay_preferences ay_prefs = {0};

/* pointer to the root object */
ay_object *ay_root;

/* pointer to the endlevel object */
ay_object *ay_endlevel;

/* pointer to slot where the next object will be linked to */
ay_object **ay_next;

ay_view_object *ay_currentview;

ay_list_object *ay_selection;

ay_list_object *ay_currentlevel;

unsigned int ay_glname = 0;

/* object clipboard */
ay_object *ay_clipboard;

GLUquadric *ay_gluquadobj = NULL;

/* registered object types */
Tcl_HashTable ay_otypesht;

/* registered object type names */
ay_otable ay_typenamest;

/* function pointer tables (object callbacks) */
ay_ftable ay_createcbt;

ay_ftable ay_deletecbt;

ay_ftable ay_copycbt;

ay_ftable ay_drawcbt;

ay_ftable ay_drawhcbt;

ay_ftable ay_drawacbt;

ay_ftable ay_shadecbt;

ay_ftable ay_getpropcbt;

ay_ftable ay_setpropcbt;

ay_ftable ay_getpntcbt;

ay_ftable ay_wribcbt;

ay_ftable ay_readcbt;

ay_ftable ay_writecbt;

ay_ftable ay_notifycbt;

ay_ftable ay_treedropcbt;

ay_ftable ay_convertcbt;

ay_ftable ay_providecbt;

ay_ftable ay_bbccbt;

ay_ftable ay_sevalcbt;

ay_ftable ay_peekcbt;

/* registered languages */
Tcl_HashTable ay_languagesht;

/* registered tag types */
Tcl_HashTable ay_tagtypesht;

int ay_errno;

int ay_read_version;

int ay_read_viewnum;

char *ay_version_ma = AY_VERSIONSTR;
char *ay_version_mi = AY_VERSIONSTRMI;

/* internal tags */
unsigned int ay_oi_tagtype;

char *ay_oi_tagname = "OI";

unsigned int ay_riattr_tagtype;

char *ay_riattr_tagname = "RiAttribute";

unsigned int ay_riopt_tagtype;

char *ay_riopt_tagname = "RiOption";

unsigned int ay_tc_tagtype;

char *ay_tc_tagname = "TC";

unsigned int ay_pv_tagtype;

char *ay_pv_tagname = "PV";

unsigned int ay_ridisp_tagtype;

char *ay_ridisp_tagname = "RiDisplay";

unsigned int ay_rihider_tagtype;

char *ay_rihider_tagname = "RiHider";

unsigned int ay_noexport_tagtype;

char *ay_noexport_tagname = "NoExport";

unsigned int ay_savegeom_tagtype;

char *ay_savegeom_tagname = "SaveMainGeom";

unsigned int ay_savelayout_tagtype;

char *ay_savelayout_tagname = "SavePaneLayout";

unsigned int ay_np_tagtype;

char *ay_np_tagname = "NP";

unsigned int ay_rp_tagtype;

char *ay_rp_tagname = "RP";

unsigned int ay_tp_tagtype;

char *ay_tp_tagname = "TP";

unsigned int ay_bns_tagtype;

char *ay_bns_tagname = "BNS";

unsigned int ay_ans_tagtype;

char *ay_ans_tagname = "ANS";

unsigned int ay_dbns_tagtype;

char *ay_dbns_tagname = "DBNS";

unsigned int ay_dans_tagtype;

char *ay_dans_tagname = "DANS";

unsigned int ay_umm_tagtype;

char *ay_umm_tagname = "UMM";

unsigned int ay_vmm_tagtype;

char *ay_vmm_tagname = "VMM";

unsigned int ay_bp_tagtype;

char *ay_bp_tagname = "BP";

unsigned int ay_cp_tagtype;

char *ay_cp_tagname = "CP";

unsigned int ay_hc_tagtype;

char *ay_hc_tagname = "HC";

unsigned int ay_no_tagtype;

char *ay_no_tagname = "NO";

unsigned int ay_nm_tagtype;

char *ay_nm_tagname = "NM";

unsigned int ay_nt_tagtype;

char *ay_nt_tagname = "NT";

unsigned int ay_aswire_tagtype;

char *ay_aswire_tagname = "AsWire";

unsigned int ay_mn_tagtype;

char *ay_mn_tagname = "MN";

unsigned int ay_mp_tagtype;

char *ay_mp_tagname = "MP";

unsigned int ay_sb_tagtype;

char *ay_sb_tagname = "SB";

unsigned int ay_sbc_tagtype;

char *ay_sbc_tagname = "SBC";

unsigned int ay_peek_tagtype;

char *ay_peek_tagname = "PE";

unsigned int ay_da_tagtype;

char *ay_da_tagname = "DA";

/* default logging directory and file */
static char *ay_log = "/tmp/ay.log";

/* bitmaps for the object tree */
Pixmap minus_bitmap;
Pixmap plus_bitmap;
#include "tcl/BWidget-1.2.1/images/minus.xbm"
#include "tcl/BWidget-1.2.1/images/plus.xbm"
Pixmap minus22_bitmap;
#include "tcl/BWidget-1.2.1/images/minus22.xbm"
Pixmap plus22_bitmap;
#include "tcl/BWidget-1.2.1/images/plus22.xbm"

/* prototypes of functions local to this module: */

int ay_init(Tcl_Interp *interp);

void ay_safeinit(Tcl_Interp *interp);

void ay_printhelp();

/* functions: */

/* ay_init:
 *  create tables, ay_root
 */
int
ay_init(Tcl_Interp *interp)
{
 int ay_status = AY_OK;
 char fname[] = "ay_init";
 ay_level_object *level = NULL;

  ay_interp = interp;

  /* initialize AY tables */
  Tcl_InitHashTable(&ay_otypesht, TCL_STRING_KEYS);

  if((ay_status = ay_table_init(&ay_typenamest)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_createcbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_deletecbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_copycbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_drawcbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_drawhcbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_drawacbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_shadecbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_setpropcbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_getpropcbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_getpntcbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_notifycbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_readcbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_writecbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_wribcbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_bbccbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_treedropcbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_convertcbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_providecbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_sevalcbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_table_initftable(&ay_peekcbt)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  /* Languages */
  Tcl_InitHashTable(&ay_languagesht, TCL_STRING_KEYS);

  /* Tags */
  Tcl_InitHashTable(&ay_tagtypesht, TCL_STRING_KEYS);

  /* initialize object type helper module */
  ay_otype_init();

  /* initialize instancing helper module */
  ay_instt_init(interp);

  /* initialize material object helper module */
  ay_matt_init(interp);

  /* initialize riattribute tag helper module */
  ay_riattr_init(interp);

  /* initialize tc tag helper module */
  ay_tc_init(interp);

  /* initialize pv tag helper module */
  ay_pv_init(interp);

  /* initialize rioption tag helper module */
  ay_riopt_init(interp);

  /* initialize ns tag helper module */
  ay_ns_init(interp);

  /* initialize wrib module */
  ay_wrib_init(interp);

  /* initialize tree module */
  ay_tree_init(interp);

  /* initialize object comparison helper module */
  if((ay_status = ay_comp_init()))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  /* initialize automatic instancing module */
  if((ay_status = ay_ai_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  /* initialize notification module */
  ay_notify_init(interp);

  /* initialize pact module */
  if((ay_status = ay_pact_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  /* initialize tcmd module */
  if((ay_status = ay_tcmd_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  /* fill tables (init object types) */
  if((ay_status = ay_root_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_view_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_acurve_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_bevel_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_birail1_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_birail2_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_box_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_bpatch_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_camera_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_cap_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_clone_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_cone_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_concatnc_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_concatnp_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_cylinder_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_disk_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_extrnc_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_extrnp_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_extrude_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_gordon_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_hyperb_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_icurve_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_instance_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_ipatch_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_level_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_light_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_parab_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_revolve_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_riinc_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_riproc_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_material_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_ncircle_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_ncurve_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_npatch_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_offnc_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_pamesh_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_pomesh_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_script_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_sdmesh_init(interp)))
     { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_select_init(interp)))
     { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_sphere_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_skin_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_sweep_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_swing_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_text_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_torus_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_trim_init(interp)))
     { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_offnp_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  if((ay_status = ay_apatch_init(interp)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  /* initialize undo system */
  if((ay_status = ay_undo_init(10)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  /* initialize Tesselation GUI module */
  ay_tgui_init(interp);

  /* initialize NURBS knots module */
  ay_knots_init(interp);

  /* initialize the patch mesh tools module */
  ay_pmt_init();

  /* register SaveMainGeom tag type */
  (void)ay_tags_register(ay_savegeom_tagname, &ay_savegeom_tagtype);

  /* register SavePaneLayout tag type */
  (void)ay_tags_register(ay_savelayout_tagname, &ay_savelayout_tagtype);

  /* register NP (NewProperty) tag type */
  (void)ay_tags_register(ay_np_tagname, &ay_np_tagtype);

  /* register RP (RemoveProperty) tag type */
  (void)ay_tags_register(ay_rp_tagname, &ay_rp_tagtype);

  /* register BP (Bevel Parameters) tag type */
  (void)ay_tags_register(ay_bp_tagname, &ay_bp_tagtype);

  /* register CP (Cap Parameters) tag type */
  (void)ay_tags_register(ay_cp_tagname, &ay_cp_tagtype);

  /* register HC (Has Child) tag type */
  (void)ay_tags_register(ay_hc_tagname, &ay_hc_tagtype);

  /* register NO (Notify Object) tag type */
  (void)ay_tags_register(ay_no_tagname, &ay_no_tagtype);

  /* register NM (Notify Master) tag type */
  (void)ay_tags_register(ay_nm_tagname, &ay_nm_tagtype);

  /* register NT (Normals&Tangents) tag type */
  (void)ay_tags_register(ay_nt_tagname, &ay_nt_tagtype);

  /* register AsWire tag type */
  (void)ay_tags_register(ay_aswire_tagname, &ay_aswire_tagtype);

  /* register MN (Mean Normal) tag type */
  (void)ay_tags_register(ay_mn_tagname, &ay_mn_tagtype);

  /* register MP (Mean Point) tag type */
  (void)ay_tags_register(ay_mp_tagname, &ay_mp_tagtype);

  /* register SB (Selected Boundary) tag type */
  (void)ay_tags_register(ay_sb_tagname, &ay_sb_tagtype);

  /* register SBC (Selected Boundary Cache) tag type */
  (void)ay_tags_register(ay_sbc_tagname, &ay_sbc_tagtype);

  /* register PE (Peek) tag type */
  (void)ay_tags_register(ay_peek_tagname, &ay_peek_tagtype);

  /* register DA (DataArray) tag type */
  (void)ay_tags_register(ay_da_tagname, &ay_da_tagtype);


  /* create root object */
  if((ay_status = ay_object_create(AY_IDROOT, &ay_root)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }

  /* create terminating level object in current level */
  if((ay_status = ay_object_create(AY_IDLEVEL, &ay_root->next)))
    { ay_error(ay_status, fname, NULL); return AY_ERROR; }
  ay_endlevel = ay_root->next;
  ay_endlevel->parent = AY_FALSE;
  ay_endlevel->down = NULL;

  level = (ay_level_object *)ay_endlevel->refine;
  level->type = AY_LTEND;

  /* create terminating level object in views level */
  ay_root->down = ay_root->next;

  /* next object will be created as sibling of ay_root */
  ay_next = &(ay_root->next);

  /* current level is the root level */
  ay_currentlevel = NULL;
  if((ay_status = ay_clevel_add(NULL)))
    return ay_status;
  if((ay_status = ay_clevel_add(ay_root)))
    return ay_status;

  ay_currentview = NULL;
  ay_selection = NULL;
  ay_clipboard = NULL;

  ay_prefs.guiscale = 1.0;
  ay_prefs.useguiscale = AY_TRUE;

  ay_prefs.handle_size = 6.0;
  ay_prefs.glu_sampling_tolerance = 30.0;
  ay_prefs.pick_epsilon = 0.2;

  ay_prefs.checklights = AY_TRUE;

  ay_prefs.bgr = 0.5;
  ay_prefs.bgg = 0.5;
  ay_prefs.bgb = 0.5;

  ay_prefs.obr = 0.0;
  ay_prefs.obg = 0.1;
  ay_prefs.obb = 0.45;

  ay_prefs.ser = 1.0;
  ay_prefs.seg = 1.0;
  ay_prefs.seb = 1.0;

  ay_prefs.grr = 0.6;
  ay_prefs.grg = 0.6;
  ay_prefs.grb = 0.6;

  ay_prefs.tpr = 0.5;
  ay_prefs.tpg = 0.0;
  ay_prefs.tpb = 0.0;

  ay_prefs.shr = 0.8;
  ay_prefs.shg = 0.75;
  ay_prefs.shb = 0.65;

  ay_prefs.linewidth = 1.0;
  ay_prefs.sellinewidth = 1.0;

  ay_prefs.aalinewidth = 1.3;
  ay_prefs.aasellinewidth = 1.3;
  ay_prefs.aafudge = 1.0;

  ay_prefs.selbndfactor = 1.5;

  ay_prefs.sdmode = 3;

  ay_prefs.use_sm = AY_FALSE;
  ay_prefs.edit_snaps_to_grid = AY_TRUE;

  ay_prefs.list_types = AY_TRUE;
  ay_prefs.mark_hidden = AY_TRUE;

  ay_prefs.smethod = 3;
  ay_prefs.sparamu = 5;
  ay_prefs.sparamv = 5;

  ay_prefs.wutag = AY_TRUE;

  ay_prefs.conv_reset_display = AY_TRUE;

  ay_prefs.errorlevel = 3;

  if(!(ay_prefs.logfile = calloc(strlen(ay_log)+1, sizeof(char))))
    return AY_EOMEM;
  strcpy(ay_prefs.logfile, ay_log);

  if(!(ay_prefs.pprender = calloc(3+1, sizeof(char))))
    return AY_EOMEM;
  strcpy(ay_prefs.pprender, "rgl");


  /* PV tag names */
  if(!(ay_prefs.texcoordname = calloc(2+1, sizeof(char))))
    return AY_EOMEM;
  strcpy(ay_prefs.texcoordname, "st");

  if(!(ay_prefs.normalname = calloc(1+1, sizeof(char))))
    return AY_EOMEM;
  strcpy(ay_prefs.normalname, "N");

  if(!(ay_prefs.colorname = calloc(2+1, sizeof(char))))
    return AY_EOMEM;
  strcpy(ay_prefs.colorname, "Cs");

  if(!(ay_prefs.opacityname = calloc(2+1, sizeof(char))))
    return AY_EOMEM;
  strcpy(ay_prefs.opacityname, "Os");

  ay_prefs.save_rootviews = AY_TRUE;

  ay_prefs.globalmark = AY_TRUE;
  ay_prefs.createat = 1;
  ay_prefs.createin = 1;
  ay_prefs.disablefailedscripts = AY_TRUE;

 return ay_status;
} /* ay_init */


/*
 * Tcl_AppInit || ay_InitStandalone:
 *
 */
#ifdef AYWRAPPED
int ay_InitStandalone(Tcl_Interp *interp);
int
ay_InitStandalone(Tcl_Interp *interp)
#else
int
Tcl_AppInit(Tcl_Interp *interp)
#endif /* AYWRAPPED */
{
 int ay_status = AY_OK;
 /*int tcl_status = TCL_OK;*/
 /*char app_init_script[] = "source [file dirname $argv0]/tcl/ayam.tcl\n";*/

#ifndef AYWRAPPED
  if(Tcl_Init(interp) == TCL_ERROR)
    return TCL_ERROR;

  if(Tk_Init(interp) == TCL_ERROR)
    return TCL_ERROR;
#endif /* AYWRAPPED */

  if(Togl_Init(interp) == TCL_ERROR)
    return TCL_ERROR;

  Togl_CreateFunc(ay_toglcb_create);
  Togl_DestroyFunc(ay_toglcb_destroy);
  Togl_DisplayFunc(ay_toglcb_display);
  Togl_ReshapeFunc(ay_toglcb_reshape);

  /* clear.c */
  Tcl_CreateCommand(interp, "newScene", ay_clear_scenetcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* clevel.c */
  Tcl_CreateCommand(interp, "cl", ay_clevel_cltcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* error.c */
  Tcl_CreateCommand(interp, "ayError", ay_error_tcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "printGLError", ay_error_printglerrortcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* matt.c */
  Tcl_CreateCommand(interp, "getRegMats", ay_matt_getregisteredtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* notify.c */
  /* provided for scripting interface backwards compatibility */
  Tcl_CreateCommand(interp, "forceNot", ay_notify_objecttcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp, "notifyOb", ay_notify_objecttcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* prefs.c */
  Tcl_CreateCommand(interp, "setPrefs", ay_prefs_settcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* read.c */
  Tcl_CreateCommand(interp, "replaceScene", ay_read_replacetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "insertScene", ay_read_inserttcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* sel.c */
  Tcl_CreateCommand(interp, "hSL", ay_sel_hsltcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* shader.c */
  Tcl_CreateCommand(interp, "shaderScanSLC", ay_shader_scanslctcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "shaderScanSLX", ay_shader_scanslxtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* tcmd.c */
#ifdef AYENABLEWAIT
  Tcl_CreateCommand(interp, "waitPid", ay_tcmd_waitpidtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
#endif /* AYENABLEWAIT */

#ifdef AYENABLEFEXIT
  Tcl_CreateCommand(interp, "fastExit", ay_tcmd_fastexittcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
#endif /* AYENABLEFEXIT */

  Tcl_CreateCommand(interp, "menuState", ay_tcmd_menustatetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* tmp.c */
  Tcl_CreateCommand(interp, "tmpGet", ay_tmp_gettcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* oact.c */
  Togl_CreateCommand("moveoac", ay_oact_movetcb);

  Togl_CreateCommand("rotoac", ay_oact_rottcb);

  Togl_CreateCommand("rotoaac", ay_oact_rotatcb);

  Togl_CreateCommand("sc1dxoac", ay_oact_sc1DXcb);

  Togl_CreateCommand("sc1dxaoac", ay_oact_sc1DXAcb);

  Togl_CreateCommand("sc1dyoac", ay_oact_sc1DYcb);

  Togl_CreateCommand("sc1dyaoac", ay_oact_sc1DYAcb);

  Togl_CreateCommand("sc1dzoac", ay_oact_sc1DZcb);

  Togl_CreateCommand("sc1dzaoac", ay_oact_sc1DZAcb);

  Togl_CreateCommand("sc2doac", ay_oact_sc2Dcb);

  Togl_CreateCommand("sc3doac", ay_oact_sc3Dcb);

  Togl_CreateCommand("str2doac", ay_oact_str2Dcb);

  Togl_CreateCommand("sc2daoac", ay_oact_sc2DAcb);

  Togl_CreateCommand("str2daoac", ay_oact_str2DAcb);

  Togl_CreateCommand("sc3daoac", ay_oact_sc3DAcb);

  /* objsel.c */
  Togl_CreateCommand("processObjSel", ay_objsel_processcb);

  Tcl_CreateCommand(interp, "getNameFromNode", ay_objsel_getnmfrmndtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* pact.c */
  Togl_CreateCommand("selpac", ay_pact_seltcb);

  Togl_CreateCommand("selbac", ay_pact_selboundtcb);

  Togl_CreateCommand("insertpac", ay_pact_insertptcb);

  Togl_CreateCommand("deletepac", ay_pact_deleteptcb);

  Togl_CreateCommand("startpepac", ay_pact_startpetcb);

  Togl_CreateCommand("pepac", ay_pact_petcb);

  Togl_CreateCommand("penpac", ay_pact_pentcb);

  Togl_CreateCommand("wepac", ay_pact_wetcb);

  Togl_CreateCommand("wrpac", ay_pact_wrtcb);

  Togl_CreateCommand("snapac", ay_pact_snaptogridcb);

  Togl_CreateCommand("snapmac", ay_pact_snaptomarkcb);

  Togl_CreateCommand("multpac", ay_pact_multiptcb);

  /* undo.c */
  Tcl_CreateCommand(interp, "undo", ay_undo_undotcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* vact.c */
  Togl_CreateCommand("movevac", ay_vact_movetcb);

  Togl_CreateCommand("zoomvac", ay_vact_zoomtcb);

  Togl_CreateCommand("movezvac", ay_vact_moveztcb);

  /* viewt.c */
  Togl_CreateCommand("redraw", ay_viewt_redrawtcb);

  Togl_CreateCommand("reshape", ay_viewt_reshapetcb);

  Togl_CreateCommand("setconf", ay_viewt_setconftcb);

  Togl_CreateCommand("mc", ay_viewt_makecurtcb);

  Togl_CreateCommand("zoomob", ay_viewt_zoomtoobj);

  Togl_CreateCommand("align", ay_viewt_align);

  Togl_CreateCommand("tocam", ay_viewt_tocamtcb);

  Togl_CreateCommand("fromcam", ay_viewt_fromcamtcb);

  Togl_CreateCommand("drop", ay_viewt_droptcb);

  Togl_CreateCommand("saveimg", ay_viewt_saveimgtcb);

  Togl_CreateCommand("rendertoviewport", ay_viewt_rendertoviewportcb);

  /* w32t.c */
#ifdef WIN32
  Tcl_CreateCommand(interp, "w32kill", ay_w32t_w32killtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
#endif

  /* wrib.c */
  Togl_CreateCommand("wrib", ay_wrib_viewtcb);

  Tcl_CreateCommand(interp, "wrib", ay_wrib_tcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /*
    the shortcut "rribWrite" exists solely for the bgconvert script,
    so that it may derive a command name for RIB export automatically
   */
  Tcl_CreateCommand(interp, "rribWrite", ay_wrib_tcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* write.c */
  Tcl_CreateCommand(interp, "saveScene", ay_write_scenetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* contrib/tree.c */
  /*
  Tcl_CreateCommand(interp, "treeInit", ay_tree_inittcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
  */

  /* nurbs/nct.c */
  Togl_CreateCommand("finduac", ay_nct_finducb);

  /* nurbs/npt.c */
  Togl_CreateCommand("finduvac", ay_npt_finduvcb);

  Togl_CreateCommand("selbndac", ay_npt_pickboundcb);

  Togl_CreateCommand("cselbndac", ay_npt_pickboundcb);


  /* create all safe commands in main interpreter */
  ay_safeinit(interp);

/* inform Tcl-context about compile time configuration: */

#ifndef AYWRAPPED
Tcl_SetVar(interp,"AYWRAPPED", "0", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#else
Tcl_SetVar(interp, "AYWRAPPED", "1", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#endif /* AYWRAPPED */

#ifndef AYUSEAFFINE
Tcl_SetVar(interp,"AYUSEAFFINE", "0", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#else
Tcl_SetVar(interp, "AYUSEAFFINE", "1", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#endif /* AYUSEAFFINE */

#ifndef AYUSESLARGS
Tcl_SetVar(interp,"AYUSESLARGS", "0", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#else
Tcl_SetVar(interp, "AYUSESLARGS", "1", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#endif /* AYUSESLARGS */

#ifndef AYUSESLCARGS
Tcl_SetVar(interp,"AYUSESLCARGS", "0", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#else
Tcl_SetVar(interp, "AYUSESLCARGS", "1", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#endif /* AYUSESLCARGS */

#ifndef AYUSESLXARGS
Tcl_SetVar(interp,"AYUSESLXARGS", "0", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#else
Tcl_SetVar(interp, "AYUSESLXARGS", "1", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#endif /* AYUSESLXARGS */

#ifndef AYUSESLOARGS
Tcl_SetVar(interp,"AYUSESLOARGS", "0", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#else
Tcl_SetVar(interp, "AYUSESLOARGS", "1", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#endif /* AYUSESLOARGS */

#ifndef AYUSESOARGS
Tcl_SetVar(interp,"AYUSESOARGS", "0", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#else
Tcl_SetVar(interp, "AYUSESOARGS", "1", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#endif /* AYUSESOARGS */

#ifndef AYIDR
Tcl_SetVar(interp,"AYIDR", "0", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#else
Tcl_SetVar(interp, "AYIDR", "1", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#endif

#ifndef AYENABLEPPREV
Tcl_SetVar(interp,"AYENABLEPPREV", "0", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#else
Tcl_SetVar(interp, "AYENABLEPPREV", "1", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#endif /* AYENABLEPPREV */

#ifndef AYWITHAQUA
Tcl_SetVar(interp,"AYWITHAQUA", "0", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#else
Tcl_SetVar(interp, "AYWITHAQUA", "1", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#endif /* AYWITHAQUA */

#ifndef AYENABLEFEXIT
Tcl_SetVar(interp,"AYENABLEFEXIT", "0", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#else
Tcl_SetVar(interp, "AYENABLEFEXIT", "1", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#endif /* AYENABLEFEXIT */

#ifdef AYNOSAFEINTERP
Tcl_SetVar(interp,"AYNOSAFEINTERP", "1", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#else
Tcl_SetVar(interp, "AYNOSAFEINTERP", "0", TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
#endif /* AYNOSAFEINTERP */

  if((ay_status = ay_init(interp)) != AY_OK)
    {
      Tcl_SetResult (interp,
                     "Ayam: Initialization failed! Bailing out!",
                     TCL_STATIC);
      return TCL_ERROR;
    }

  Tcl_SetVar2(interp, "ay", "ay_version", AY_VERSIONSTR, TCL_GLOBAL_ONLY |
	      TCL_LEAVE_ERR_MSG);

  /* create bitmaps */
  if(Tk_MainWindow(interp))
    {
      Tk_DefineBitmap(interp, "minus", minus_bits, minus_width, minus_height);
      minus_bitmap = Tk_GetBitmap(interp, Tk_MainWindow(interp), "minus");
      Tk_DefineBitmap(interp, "plus", plus_bits, plus_width, plus_height);
      plus_bitmap = Tk_GetBitmap(interp, Tk_MainWindow(interp), "plus");
      Tk_DefineBitmap(interp, "minus22", minus22_bits,
		      minus22_width, minus22_height);
      minus22_bitmap = Tk_GetBitmap(interp, Tk_MainWindow(interp), "minus22");
      Tk_DefineBitmap(interp, "plus22", plus22_bits,
		      plus22_width, plus22_height);
      plus22_bitmap = Tk_GetBitmap(interp, Tk_MainWindow(interp), "plus22");
    }

#if 0
  /* if we run from an .app-Bundle, we know where "ayam.tcl" is and
     may (and have to!) start it now automatically */
#ifdef AYWITHAQUA
  Tcl_Eval(interp, app_init_script);
#endif /* AYWITHAQUA */
#endif

#ifndef AYNOSAFEINTERP
  ay_safeinterp = Tcl_CreateSlave(interp, "aySafeInterp", 1);

  ay_safeinit(ay_safeinterp);

  /*
  Tcl_SetVar(ay_safeinterp, "argv", "-use", TCL_GLOBAL_ONLY |
	     TCL_LEAVE_ERR_MSG);
  sprintf(buf,"%x",Tk_GetWindowID(".");
  Tcl_SetVar(ay_safeinterp, "argv", buf, TCL_GLOBAL_ONLY |
	     TCL_LEAVE_ERR_MSG|TCL_LIST_ELEMENT);
  tcl_status = Tk_SafeInit(ay_safeinterp);
  if(tcl_status != TCL_OK)
    return TCL_ERROR;
  */
#endif

 return TCL_OK;
} /* Tcl_AppInit */


/** ay_safeinit:
 *  create all commands that are considered to be safe
 *
 * \param[in,out] interp  Tcl interpreter in which to create the commands
 *
 * \returns AY_OK on success, error code otherwise.
 */
void
ay_safeinit(Tcl_Interp *interp)
{

  /* bbc.c */
  Tcl_CreateCommand(interp, "getBB", ay_bbc_gettcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* clevel.c */
  Tcl_CreateCommand(interp, "goTop", ay_clevel_gotoptcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "goUp", ay_clevel_gouptcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "goDown", ay_clevel_godowntcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getLevel", ay_clevel_gettcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
  /*
  Tcl_CreateCommand(interp, "cl", ay_clevel_cltcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
  */

  /* clipb.c */
  Tcl_CreateCommand(interp, "clearClip", ay_clipb_cleartcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "copOb", ay_clipb_copytcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "cutOb", ay_clipb_cuttcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "pasOb", ay_clipb_pastetcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "pasmovOb", ay_clipb_pastetcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "repOb", ay_clipb_replacetcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "upOb", ay_clipb_hmovtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "downOb", ay_clipb_hmovtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* conv.c */
  Tcl_CreateCommand(interp, "convOb", ay_convert_objecttcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* instt.c */
  Tcl_CreateCommand(interp, "resolveIn", ay_instt_resolvetcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getMaster", ay_instt_getmastertcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* interpol.c */
  Tcl_CreateCommand(interp, "tweenNC", ay_interpol_curvestcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "tweenNP", ay_interpol_surfacestcmd,
		     (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* notify.c */
  if(interp == ay_safeinterp)
    {
      Tcl_CreateCommand(interp, "notifyOb", ay_notify_objectsafetcmd,
			(ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    }

  /* object.c */
  Tcl_CreateCommand(interp, "crtOb", ay_object_createtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "delOb", ay_object_deletetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "candelOb", ay_object_deletetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "placeOb", ay_object_placetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "nameOb", ay_object_setnametcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "hasChild", ay_object_ishastcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "hasMat", ay_object_ishastcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "hasRefs", ay_object_ishastcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "hasTrafo", ay_object_ishastcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "isCurve", ay_object_ishastcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "isSurface", ay_object_ishastcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "isDegen", ay_object_ishastcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "isHidden", ay_object_ishastcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "isClosed", ay_object_ishastcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "isParent", ay_object_ishastcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "isPlanar", ay_object_ishastcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "isRational", ay_object_ishastcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "isTrimmed", ay_object_ishastcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getType", ay_object_gettypeornametcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getName", ay_object_gettypeornametcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* pomesht.c */
  Tcl_CreateCommand(interp, "mergePo", ay_pomesht_mergetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "optiPo", ay_pomesht_optimizetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "splitPo", ay_pomesht_splittcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "genfnPo", ay_pomesht_gennormtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "gensnPo", ay_pomesht_gennormtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "remsnPo", ay_pomesht_gennormtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "flipPo", ay_pomesht_gennormtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "connectPo", ay_pomesht_connecttcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "quadPo", ay_pomesht_quadrangulatetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);


  /* prop.c */
  Tcl_CreateCommand(interp, "setProp", ay_prop_settcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getProp", ay_prop_gettcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "setTrafo", ay_prop_settrafotcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getTrafo", ay_prop_gettrafotcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "setAttr", ay_prop_setattrtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getAttr", ay_prop_getattrtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "setMat", ay_prop_setmattcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getMat", ay_prop_getmattcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* prefs.c */
  Tcl_CreateCommand(interp, "getPrefs", ay_prefs_gettcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* sel.c */
  Tcl_CreateCommand(interp, "selOb", ay_sel_selobtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getSel", ay_sel_getseltcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  if(interp == ay_safeinterp)
    {
      Tcl_CreateCommand(interp, "sL", ay_sel_hsltcmd,
			(ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
      Tcl_CreateCommand(interp, "sP", ay_sel_hsptcmd,
			(ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    }

  /* shader.c */
  Tcl_CreateCommand(interp, "shaderSet", ay_shader_settcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "shaderGet", ay_shader_gettcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* tags.c */
  Tcl_CreateCommand(interp, "setTags", ay_tags_settcmd,
		      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "addTag", ay_tags_addtcmd,
		      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "setTag", ay_tags_addtcmd,
		      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getTags", ay_tags_gettcmd,
		      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getTag", ay_tags_gettcmd,
		      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "hasTag", ay_tags_hastcmd,
		      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "delTags", ay_tags_deletetcmd,
		      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "registerTag", ay_tags_registertcmd,
		      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* tcmd.c */
  Tcl_CreateCommand(interp, "revertC", ay_tcmd_reverttcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "showOb", ay_tcmd_showhidetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "hideOb", ay_tcmd_showhidetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getVersion", ay_tcmd_getversionstcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getPnt", ay_tcmd_getpointtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "setPnt", ay_tcmd_setpointtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getNormal", ay_tcmd_getnormaltcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getTangent", ay_tcmd_gettangenttcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "setNormal", ay_tcmd_setnormaltcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "withOb", ay_tcmd_withobtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "getPlaneNormal", ay_tcmd_getplanenormaltcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "crtToolOb", ay_tcmd_crttoolobjtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* trafo.c */
  Tcl_CreateCommand(interp, "delegTrafo", ay_trafo_delegatetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "movOb", ay_trafo_movtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "movPnts", ay_trafo_movtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "scalOb", ay_trafo_scaltcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "scalPnts", ay_trafo_scaltcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "rotOb", ay_trafo_rottcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "rotPnts", ay_trafo_rottcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "normTrafos", ay_trafo_normalizetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "normPnts", ay_trafo_normalizetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "normVar", ay_trafo_normalizetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "normVal", ay_trafo_normalizetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* selp.c */
  Tcl_CreateCommand(interp, "selPnts", ay_selp_seltcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "invPnts", ay_selp_inverttcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "applyTrafo", ay_selp_applytrafotcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "centerPnts", ay_selp_centertcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* provided for scripting interface backwards compatibility */
  Tcl_CreateCommand(interp, "centerNC", ay_selp_centertcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* nurbs/nct.c */
  Tcl_CreateCommand(interp, "openC", ay_tcmd_openclosetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "closeC", ay_tcmd_openclosetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "refineC", ay_tcmd_refinecoarsentcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "coarsenC", ay_tcmd_refinecoarsentcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "refineknNC", ay_nct_refinekntcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* provided for scripting interface backwards compatibility */
  Tcl_CreateCommand(interp, "coarsenNC", ay_tcmd_refinecoarsentcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "clampNC", ay_nct_clamptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "elevateNC", ay_nct_elevatetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "reduceNC", ay_nct_elevatetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "insknNC", ay_nct_insertkntcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "remknNC", ay_nct_removekntcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "remsuknNC", ay_nct_removekntcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "collMP", ay_selp_collapsetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "explMP", ay_selp_explodetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "splitNC", ay_nct_splittcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* provided for scripting interface backwards compatibility */
  Tcl_CreateCommand(interp, "concatNC", ay_nct_concatctcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "concatC", ay_nct_concatctcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "crtNCircle", ay_nct_crtncircletcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "crtNRect", ay_nct_crtrecttcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "crtTrimRect", ay_nct_crtrecttcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "crtClosedBS", ay_nct_crtclosedbsptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "rescaleknNC", ay_nct_rescaleknvtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "curvPlot", ay_nct_curvplottcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "shiftC", ay_nct_shiftctcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* provided for scripting interface backwards compatibility */
  Tcl_CreateCommand(interp, "toXYNC", ay_nct_toxytcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "toXYC", ay_nct_toxytcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "toXZC", ay_nct_toxytcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "toYZC", ay_nct_toxytcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "isCompNC", ay_nct_iscomptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "makeCompNC", ay_nct_makecomptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "trimNC", ay_nct_trimtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "estlenNC", ay_nct_estlentcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "reparamNC", ay_nct_reparamtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "unclampNC", ay_nct_unclamptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "extendNC", ay_nct_extendtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "extrNC", ay_nct_extractnctcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "fairNC", ay_nct_fairnctcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "curvatNC", ay_nct_getcurvaturetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "torsionNC", ay_nct_getcurvaturetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* nurbs/npt.c */
  Tcl_CreateCommand(interp, "crtNSphere", ay_npt_crtnspheretcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "crtNSphere2", ay_npt_crtnsphere2tcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "breakNP", ay_npt_breakintocurvestcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "buildNP", ay_npt_buildfromcurvestcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "topoly", ay_tess_npatchtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "elevateuNP", ay_npt_elevateuvtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "elevatevNP", ay_npt_elevateuvtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "reduceuNP", ay_npt_degreereducetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "reducevNP", ay_npt_degreereducetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "refineuNP", ay_npt_refineuvtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "refinevNP", ay_npt_refineuvtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "swapuvS", ay_npt_swapuvtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "revertuS", ay_npt_revertutcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "revertvS", ay_npt_revertvtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "closeuNP", ay_npt_closeutcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "closevNP", ay_npt_closevtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "clampuNP", ay_npt_clampuvtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "clampvNP", ay_npt_clampuvtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "rescaleknNP", ay_npt_rescaleknvnptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "insknuNP", ay_npt_insertknutcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "insknvNP", ay_npt_insertknvtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "splituNP", ay_npt_splituvtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "splitvNP", ay_npt_splituvtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "extrNP", ay_npt_extractnptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "concatS", ay_npt_concatstcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "remknuNP", ay_npt_remknunptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "remknvNP", ay_npt_remknvnptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "remsuknuNP", ay_npt_remsuknnptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "remsuknvNP", ay_npt_remsuknnptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "interpuNP", ay_ipt_interpuvtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "interpvNP", ay_ipt_interpuvtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "unclampuNP", ay_npt_unclamptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "unclampvNP", ay_npt_unclamptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "isCompNP", ay_npt_iscomptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "makeCompNP", ay_npt_makecomptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "curvatNP", ay_npt_getcurvaturetcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "fairNP", ay_npt_fairnptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* nurbs/ict.c */
  Tcl_CreateCommand(interp, "interpNC", ay_ict_interptcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* nurbs/act.c */
  Tcl_CreateCommand(interp, "approxNC", ay_act_approxtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* nurbs/apt.c */
  Tcl_CreateCommand(interp, "approxNP", ay_apt_approxtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* nurbs/pmt.c */
  Tcl_CreateCommand(interp, "tobasisPM", ay_pmt_tobasistcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  /* nurbs/tess.c */
  Tcl_CreateCommand(interp, "tessNP", ay_tess_npatchtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "rtessNP", ay_tess_npatchtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  Tcl_CreateCommand(interp, "stessNP", ay_tess_npatchtcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

 return;
} /* ay_safeinit */


/** ay_checkversion:
 * Compare plugin compilation and Ayam runtime versions.
 * May report incompatibility to the user via ay_error().
 *
 * If this function returns AY_ERROR, plugin loading should be
 * aborted immediately as crashes will likely occur should the
 * plugin attempt to use the (incompatible/changed) Ayam API.
 *
 * \param[in] fname  place parameter for ay_error(), may be NULL
 * \param[in] version_ma  major Ayam version the plugin was compiled with
 * \param[in] version_mi  minor Ayam version the plugin was compiled with
 *
 * \returns AY_OK if the plugin version is compatible,
 *          AY_ERROR if the plugin version is not compatible,
 *          AY_ENULL if any version parameter is NULL
 */
int
ay_checkversion(char *fname, char *version_ma, char *version_mi)
{
 int ay_status = AY_OK;
 char errmsg[] = "Plugin has been compiled for a different Ayam version!";
 char bailmsg[] = "It is unsafe to continue! Bailing out...";
 char contmsg[] = "However, it is probably safe to continue...";

  if(!version_ma || !version_mi)
    return AY_ENULL;

  if(strcmp(ay_version_ma, version_ma))
    {
      if(fname)
	{
	  ay_error(AY_ERROR, fname, errmsg);
	  ay_error(AY_ERROR, fname, bailmsg);
	}
      ay_status = AY_ERROR;
    }
  else
    {
      if(strcmp(ay_version_mi, version_mi) && fname)
	{
	  ay_error(AY_EWARN, fname, errmsg);
	  ay_error(AY_EWARN, fname, contmsg);
	}
    }

 return ay_status;
} /* ay_checkversion */


/** 
 * Helper to print the supported command line arguments to stdout.
 */
void
ay_printhelp()
{
  printf("Ayam supports the following command line arguments:\n");
  printf( " -h:          Display this help.\n");
  printf( " -nosplash:   Do not display splash-image.\n");
  printf( " -failsafe:   Do not load preferences and environment.\n");
  printf( " -noview:     Do not open a view.\n");
  printf( " -guiscale x: Scale GUI by amount x [1.0-3.0].\n");
  printf( " 1.ay 2.ay: Load 1.ay, insert 2.ay.\n");
  printf( "\n Ayam - Reconstruct the World!\n");
 return;
} /* ay_printhelp */


#ifndef AYWRAPPED
/********************************************/
/*main:                                     */
/********************************************/
int
main(int argc, char **argv)
{
  if(argc > 1 && argv[1][0] == '-' && argv[1][1] == 'h')
    {
      ay_printhelp();
      return 0;
    }

  Tk_Main(argc, argv, Tcl_AppInit);
  return 0;
} /* main */
#endif /* AYWRAPPED */
