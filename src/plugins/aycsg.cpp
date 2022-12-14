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

// aycsg.cpp - Ayam CSG preview plugin

#include <GL/glew.h>
#include "ayam.h"

#include "opencsg.h"
#include "ayCSGPrimitive.h"

#ifdef AYCSGDBG
#include "aycore/ppoh.h"
#endif

// local preprocessor definitions
#define CSGTYPE modified

// global variables
std::vector<OpenCSG::Primitive*> aycsg_primitives;

ay_object *aycsg_root; // the root of the local copy of the object tree

int aycsg_calcbbs = AY_FALSE; // control bounding box optimization, unused!

// TM tags are used to store transformation attributes delegated from
// parents (CSG operation objects) to their children (e.g. primitives);
// note that the TM tags are used here in a quite unusual way:
// <tag->name> is used to denote coordinate system flips caused by an
// odd number of negative scale factors (it is not used as a pointer!) and
// <tag->val> points to an array of doubles (the transformation matrix
// itself) instead to a string;
// we can do this safely, because no TM tag will escape this module ever
unsigned int aycsg_tm_tagtype;
char aycsg_tm_tagname[] = "TM";

typedef struct aycsg_taglistelem_s {
  struct aycsg_taglistelem_s *next;
  ay_tag *tag;
} aycsg_taglistelem;

// all TM tags created by aycsg are managed using this list
aycsg_taglistelem *aycsg_tmtags;

// DC tags are used to store the depth complexity of a primitive
unsigned int aycsg_dc_tagtype;
char aycsg_dc_tagname[] = "DC";

char aycsg_version_ma[] = AY_VERSIONSTR;
char aycsg_version_mi[] = AY_VERSIONSTRMI;

// prototypes of functions local to this module
int aycsg_rendertcb(struct Togl *togl, int argc, char *argv[]);

void aycsg_drawtoplevelprim(Togl *togl);

void aycsg_getNDCBB(ay_object *t, struct Togl *togl,
		    double *minx, double *miny, double *minz,
		    double *maxx, double *maxy, double *maxz);

int aycsg_flatten(ay_object *t, struct Togl *togl, int parent_csgtype,
		  int calc_bbs);

void aycsg_clearprimitives();

int aycsg_applyrule1(ay_object *t);
int aycsg_applyrule2(ay_object *t);
int aycsg_applyrule3(ay_object *t);
int aycsg_applyrule4(ay_object *t);
int aycsg_applyrule5(ay_object *t);
int aycsg_applyrule6(ay_object *t);
int aycsg_applyrule7(ay_object *t);
int aycsg_applyrule8(ay_object *t);
int aycsg_applyrule9(ay_object *t);

int aycsg_normalize(ay_object *t);

int aycsg_removetlu(ay_object *o, ay_object **t);

int aycsg_delegatetrafo(ay_object *o);

int aycsg_delegateall(ay_object *t);

int aycsg_binarify(ay_object *parent, ay_object *left, ay_object **target);

int aycsg_copytree(int sel_only, ay_object *t, int *is_csg,
		   ay_object **target);

void aycsg_cleartree(ay_object *t);

void aycsg_cleartmtags();

extern "C" {

void aycsg_display(struct Togl *togl);

int aycsg_toggletcb(struct Togl *togl, int argc, char *argv[]);

int aycsg_setopttcmd(ClientData clientData, Tcl_Interp *interp,
		     int argc, char *argv[]);

#ifdef WIN32
  __declspec (dllexport)
#endif // WIN32
int Aycsg_Init(Tcl_Interp *interp);

}

// functions

#ifdef AYCSGDBG
// aycsg_printppohcb:
//  ppoh callback to print the CSG type (for debugging purposes only)
int
aycsg_printppohcb(ay_object *o, FILE *fileptr, char *prefix)
{
 int ay_status = AY_OK;

  if(o->tags && (o->tags->type == aycsg_tm_tagtype))
    {
      if(prefix)
	{
	  fprintf(fileptr, "%s", prefix);
	}
      fprintf(fileptr, "Has TM tag\n");
    }

  if(o->type == AY_IDLEVEL)
    {

      if(prefix)
	{
	  fprintf(fileptr, "%s", prefix);
	}

      fprintf(fileptr, "CSG-Type: ");

      switch(o->modified)
	{
	case AY_LTEND:
	  fprintf(fileptr, "EndLevel");
	  break;
	case AY_LTLEVEL:
	  fprintf(fileptr, "Level");
	  break;
	case AY_LTUNION:
	  fprintf(fileptr, "Union");
	  break;
	case AY_LTDIFF:
	  fprintf(fileptr, "Difference");
	  break;
	case AY_LTINT:
	  fprintf(fileptr, "Intersection");
	  break;
	case AY_LTPRIM:
	  fprintf(fileptr, "Primitive");
	  break;
	default:
	  break;
	} // switch
      fprintf(fileptr, "\n");
    } // if

 return ay_status;
} // aycsg_printppohcb
#endif


// aycsg_rendertcb:
//  Togl callback that renders CSG in the view pointed to by <togl>
int
aycsg_rendertcb(struct Togl *togl, int argc, char *argv[])
{
 int ay_status = AY_OK;
 // Tcl_Interp *interp = ay_interp;
 ay_object *o = NULL;
 ay_view_object *view = NULL;
 int orig_use_materialcolor = ay_prefs.use_materialcolor;
 GLfloat color[4] = {0.0f,0.0f,0.0f,0.0f};
 double tm[16];
 int is_csg;
 Togl_Callback *oldaltdispcb = NULL;
#ifdef AYCSGDBG
 ay_printcb *cbv[4];

  cbv[0] = &ay_ppoh_prtype;
  cbv[1] = &ay_ppoh_prtrafos;
  cbv[2] = &aycsg_printppohcb/*ay_ppoh_prflags*/;
  cbv[3] = NULL;
#endif

  view = (ay_view_object *)Togl_GetClientData(togl);

  // tell shade callbacks we are rendering CSG
  oldaltdispcb = view->altdispcb;
  view->altdispcb = aycsg_display;

  aycsg_clearprimitives();
  aycsg_root = NULL;

  ay_status = aycsg_copytree(view->drawsel,
			     (view->drawsel||view->drawlevel)?
			     ay_currentlevel->object:ay_root->next,
			     &is_csg, &aycsg_root);

  ay_status = aycsg_delegateall(aycsg_root);

  ay_status = aycsg_removetlu(aycsg_root, &aycsg_root);

  o = aycsg_root;
  while(o)
    {
      ay_status = aycsg_normalize(o);
      o = o->next;
    }

  ay_status = aycsg_removetlu(aycsg_root, &aycsg_root);

#ifdef AYCSGDBG
  ay_ppoh_print(aycsg_root, stdout, 0, cbv);
  /*  aycsg_cleartree(aycsg_root);
  aycsg_root = NULL;
  aycsg_cleartmtags();
  aycsg_tmtags = NULL;
  return TCL_OK;*/
#endif

  // draw CSG
  glClearDepth((GLfloat)1.0);
  glClearColor((GLfloat)ay_prefs.bgr, (GLfloat)ay_prefs.bgg,
	       (GLfloat)ay_prefs.bgb, (GLfloat)0.0);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  if(view->drawbgimage)
    {
      ay_draw_bgimage(togl);
    }

  if((view->drawsel || view->drawlevel) && ay_currentlevel->object != ay_root)
    {
      ay_trafo_getparent(ay_currentlevel->next, tm);
      glMultMatrixd((GLdouble*)tm);
    }

  o = aycsg_root;
  while(o)
    {
      if(o->CSGTYPE != AY_LTPRIM)
	{
	  // do not use glColor()/glMaterial() while resolving CSG,
	  // it is needed by OpenCSG...
	  ay_prefs.use_materialcolor = AY_FALSE;

	  ay_status = aycsg_flatten(o, togl, AY_LTUNION, aycsg_calcbbs);

	  // XXXX do we need this?
	  glClear(GL_STENCIL_BUFFER_BIT);

	  // fill depth buffer (resolve CSG operations)
	  glDisable(GL_LIGHTING);

	  OpenCSG::setContext(view->id);

	  OpenCSG::render(aycsg_primitives);

	  // now draw again using existing depth buffer bits and
	  // possibly with colors
	  glEnable(GL_DITHER);
	  glEnable(GL_LIGHTING);
	  glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, (GLfloat)1.0);
	  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

	  color[0] = (GLfloat)ay_prefs.shr;
	  color[1] = (GLfloat)ay_prefs.shg;
	  color[2] = (GLfloat)ay_prefs.shb;
	  color[3] = (GLfloat)1.0;

	  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
	  glMatrixMode(GL_MODELVIEW);

	  ay_prefs.use_materialcolor = orig_use_materialcolor;
	  glDepthFunc(GL_EQUAL);
	  for(std::vector<OpenCSG::Primitive*>::const_iterator i =
		aycsg_primitives.begin(); i != aycsg_primitives.end(); ++i) {
	    (*i)->render();
	  }
	  glDepthFunc(GL_LESS);
	  aycsg_clearprimitives();
	} // if
      o = o->next;
    } // while

  // now draw non-CSG top level primitives
  aycsg_drawtoplevelprim(togl);

  // swap buffers
  Togl_SwapBuffers(togl);

  // clear local copy of object tree
  aycsg_cleartree(aycsg_root);
  aycsg_root = NULL;

  // clear all TM tags
  aycsg_cleartmtags();
  aycsg_tmtags = NULL;

  // restore alternative display callback of view window
  view->altdispcb = oldaltdispcb;

 return TCL_OK;
} // aycsg_rendertcb


// aycsg_drawtoplevelprim:
//  draw non-CSG primitives in the top level of the scene
void
aycsg_drawtoplevelprim(Togl *togl)
{
 ay_object *t = aycsg_root;
 int has_tm = AY_FALSE;

  while(t)
    {
      // is t a primitive?
      if(t->CSGTYPE == AY_LTPRIM)
	{
	  has_tm = AY_FALSE;
	  if(t->tags && (t->tags->type == aycsg_tm_tagtype))
	    {
	      has_tm = AY_TRUE;
	      glPushMatrix();
	      glMultMatrixd((GLdouble*)(t->tags->val));
	      if(t->tags->name)
		{
		  glFrontFace(GL_CW);
		}
	    } // if
	  ay_shade_object(togl, t, AY_FALSE);
	  if(has_tm)
	    {
	      if(t->tags->name)
		{
		  glFrontFace(GL_CCW);
		}
	      glPopMatrix();
	    } // if
	} // if
      t = t->next;
    } // while

 return;
} // aycsg_drawtoplevelprim


// aycsg_getNDCBB:
//  get bounding box of object t in normalised device coordinates
void aycsg_getNDCBB(ay_object *t, struct Togl *togl,
		    double *minx, double *miny, double *minz,
		    double *maxx, double *maxy, double *maxz)
{
 int ay_status;
 int i;
 double bb[24] = {0}, m[16], mm[16], pm[16], vp[4];
 ay_view_object *view = NULL;
 int width = Togl_Width(togl);
 int height = Togl_Height(togl);
 double aspect = ((double)width) / ((double)height);

  if(!t || !togl)
    return;

  view = (ay_view_object *)Togl_GetClientData(togl);

  ay_status = ay_bbc_get(t, bb);

  *minx = DBL_MAX; *maxx = DBL_MIN;
  *miny = DBL_MAX; *maxy = DBL_MIN;
  *minz = DBL_MAX; *maxz = DBL_MIN;

  // transform BB to NDC (normalized device coordinates)

  if(t->tags && (t->tags->type == aycsg_tm_tagtype))
    {
      memcpy(m, t->tags->val, 16*sizeof(double));
    }
  else
    {
      ay_trafo_identitymatrix(m);
    }

  glGetDoublev(GL_MODELVIEW_MATRIX, mm);
  glGetDoublev(GL_PROJECTION_MATRIX, pm);
  glGetDoublev(GL_VIEWPORT, vp);

  for(i = 0; i < 24; i += 3)
    {
      ay_trafo_apply3(&(bb[i]), m);
      ay_trafo_apply3(&(bb[i]), mm);
      ay_trafo_apply3(&(bb[i]), pm);

      if(bb[i] < *minx)
	{
	  *minx = bb[i];
	}
      if(bb[i] > *maxx)
	{
	  *maxx = bb[i];
	}
      if(bb[i+1] < *miny)
	{
	  *miny = bb[i+1];
	}
      if(bb[i+1] > *maxy)
	{
	  *maxy = bb[i+1];
	}
      if(bb[i+2] < *minz)
	{
	  *minz = bb[i+2];
	}
      if(bb[i+2] > *maxz)
	{
	  *maxz = bb[i+2];
	}
    } // for

  if(view->type == AY_VTPERSP)
    {
      *minx *= aspect*view->zoom;
      *maxx *= aspect*view->zoom;
      *miny *= view->zoom;
      *maxy *= view->zoom;
    }

#ifdef AYCSGDBG
  printf("BB: %g %g %g %g %g %g\n", *minx, *miny, *minz, *maxx, *maxy, *maxz);
#endif

 return;
} // aycsg_getNDCBB


// aycsg_flatten:
//  _recursively_ convert tree <t> to primitive array for resolving CSG
//  and rendering, <t> should be normalized using aycsg_normalize()
int
aycsg_flatten(ay_object *t, struct Togl *togl, int parent_csgtype,
	      int calc_bbs)
{
 int ay_status = AY_OK;
 ay_tag *tag = NULL;
 int dc = 1;

  // is t a primitive?
  if((t->CSGTYPE == AY_LTPRIM) && (parent_csgtype != AY_LTUNION))
    {
      // yes

      // get depth complexity
      dc = 1;
      if(t->tags)
	{
	  tag = t->tags;
	  while(tag)
	    {
	      if(tag->type == aycsg_dc_tagtype)
		{
		  if(tag->val)
		    {
		      sscanf((char*)tag->val, "%d", &dc);
		    } // if
		} // if
	      tag = tag->next;
	    } // while
	} // if

      // is this the lowest and leftmost primitive in the tree?
      // the simple check (t->next) is possible, because the
      // tree is binary _and_ normalized
      if(t->next)
	{
	  // yes, this is always an intersecting primitive
	  aycsg_primitives.push_back(new OpenCSG::ayCSGPrimitive(t, togl,
						 OpenCSG::Intersection, dc));
	}
      else
	{
	  // no, use parent_csgtype to determine CSG operation
	  if(parent_csgtype == AY_LTDIFF)
	    {
	      aycsg_primitives.push_back(new OpenCSG::ayCSGPrimitive(t, togl,
						  OpenCSG::Subtraction, dc));
	    }
	  else
	    {
	      aycsg_primitives.push_back(new OpenCSG::ayCSGPrimitive(t, togl,
						  OpenCSG::Intersection, dc));
	    } // if
	} // if

      if(calc_bbs)
	{
	  double minx, miny, minz, maxx, maxy, maxz;
	  OpenCSG::Primitive *p = aycsg_primitives.back();
	  aycsg_getNDCBB(t, togl, &minx, &miny, &minz, &maxx, &maxy, &maxz);
	  p->setBoundingBox((float)minx, (float)miny, (float)minz,
			    (float)maxx, (float)maxy, (float)maxz);
	} // if

    }
  else
    {
      // no
      if((t->type == AY_IDLEVEL) && (t->down) && (t->down->next))
	{
	  ay_status = aycsg_flatten(t->down, togl, t->CSGTYPE, calc_bbs);
	  ay_status = aycsg_flatten(t->down->next, togl, t->CSGTYPE, calc_bbs);
	}
    } // if

 return ay_status;
} // aycsg_flatten


// aycsg_clearprimitives:
//  clear the array of CSG primitives created by aycsg_flatten()
void
aycsg_clearprimitives()
{

  for(std::vector<OpenCSG::Primitive*>::const_iterator i =
	aycsg_primitives.begin(); i != aycsg_primitives.end(); ++i)
    {
      OpenCSG::ayCSGPrimitive* p =
	static_cast<OpenCSG::ayCSGPrimitive*>(*i);
      delete p;
    }

  aycsg_primitives.clear();

 return;
} // aycsg_clearprimitives


// The following nine functions implement rules that transform
// a binary tree to disjunctive normal form (sum of products).

// Rule1: X - ( Y U Z ) => ( X - Y ) - Z
//          t     s            s     t
int
aycsg_applyrule1(ay_object *t)
{
 ay_object *X, *Y, *Z, *s;

  // 1. check applicability of rule
  if(!t || !t->down || !t->down->next)
    return AY_ERROR;

  if(t->CSGTYPE != AY_LTDIFF)
    return AY_ERROR;

  X = t->down;

  s = t->down->next;

  if(s->CSGTYPE != AY_LTUNION)
    return AY_ERROR;

  if(!s->down)
    return AY_ERROR;

  Y = s->down;
  Z = Y->next;

  // 2. transform hierarchy
  s->CSGTYPE = AY_LTDIFF;
  t->down = s;
  s->down = X;
  X->next = Y;
  Y->next = NULL;
  s->next = Z;
#ifdef AYCSGDBG
  printf("applied rule1\n");
#endif
 return AY_OK;
} // aycsg_applyrule1


// Rule2: X O ( Y U Z ) => ( X O Y ) U ( X O Z )
//          t     r            r     t     s
int
aycsg_applyrule2(ay_object *t)
{
 ay_object *X, *Xnew, *Y, *Z, *r, *s;

  // 1. check applicability of rule
  if(!t || !t->down || !t->down->next)
    return AY_ERROR;

  if(t->CSGTYPE != AY_LTINT)
    return AY_ERROR;

  X = t->down;
  r = t->down->next;

  if(r->CSGTYPE != AY_LTUNION)
    return AY_ERROR;

  if(!r->down)
    return AY_ERROR;

  Y = r->down;
  Z = r->down->next;

  // 2. transform hierarchy
  if(!(Xnew = (ay_object*)calloc(1, sizeof(ay_object))))
    return AY_EOMEM;
  memcpy(Xnew, X, sizeof(ay_object));
  if(!(s = (ay_object*)calloc(1, sizeof(ay_object))))
    {free(Xnew); return AY_EOMEM;}
  memcpy(s, t, sizeof(ay_object));

  t->CSGTYPE = AY_LTUNION;
  r->CSGTYPE = AY_LTINT;
  s->CSGTYPE = AY_LTINT;
  t->down = r;
  r->next = s;
  r->down = X;
  X->next = Y;
  Y->next = NULL;
  s->down = Xnew;
  Xnew->next = Z;
#ifdef AYCSGDBG
  printf("applied rule2\n");
#endif
 return AY_OK;
} // aycsg_applyrule2


// Rule3: X - ( Y O Z ) => ( X - Y ) U ( X - Z )
//          t     r            r     t     s
int
aycsg_applyrule3(ay_object *t)
{
 ay_object *X, *Xnew, *Y, *Z, *r, *s;

  // 1. check applicability of rule
  if(!t || !t->down || !t->down->next)
    return AY_ERROR;

  if(t->CSGTYPE != AY_LTDIFF)
    return AY_ERROR;

  X = t->down;
  r = t->down->next;

  if(r->CSGTYPE != AY_LTINT)
    return AY_ERROR;

  if(!r->down)
    return AY_ERROR;

  Y = r->down;
  Z = r->down->next;

  // 2. transform hierarchy
  if(!(Xnew = (ay_object*)calloc(1, sizeof(ay_object))))
    return AY_EOMEM;
  memcpy(Xnew, X, sizeof(ay_object));
  if(!(s = (ay_object*)calloc(1, sizeof(ay_object))))
    {free(Xnew); return AY_EOMEM;}
  memcpy(s, t, sizeof(ay_object));

  t->CSGTYPE = AY_LTUNION;
  r->CSGTYPE = AY_LTDIFF;
  s->CSGTYPE = AY_LTDIFF;
  t->down = r;
  r->next = s;
  r->down = X;
  X->next = Y;
  Y->next = NULL;
  s->down = Xnew;
  Xnew->next = Z;
#ifdef AYCSGDBG
  printf("applied rule3\n");
#endif
 return AY_OK;
} // aycsg_applyrule3


// Rule4: X O ( Y O Z ) => ( X O Y ) O Z
//          t     s            s     t
int
aycsg_applyrule4(ay_object *t)
{
 ay_object *X, *Y, *Z, *s;

  // 1. check applicability of rule
  if(!t || !t->down || !t->down->next)
    return AY_ERROR;

  if(t->CSGTYPE != AY_LTDIFF)
    return AY_ERROR;

  X = t->down;
  s = t->down->next;

  if(s->CSGTYPE != AY_LTINT)
    return AY_ERROR;

  if(!s->down)
    return AY_ERROR;
  Y = s->down;
  Z = s->down->next;

  // 2. transform hierarchy
  t->down = s;
  s->next = Z;
  s->down = X;
  X->next = Y;
  Y->next = NULL;
#ifdef AYCSGDBG
  printf("applied rule4\n");
#endif
 return AY_OK;
} // aycsg_applyrule4


// Rule5: X - ( Y - Z ) => ( X - Y ) U ( X O Z )
//          t     r            r     t     s
int
aycsg_applyrule5(ay_object *t)
{
 ay_object *X, *Xnew, *Y, *Z, *r, *s;

  // 1. check applicability of rule
  if(!t || !t->down || !t->down->next)
    return AY_ERROR;

  if(t->CSGTYPE != AY_LTDIFF)
    return AY_ERROR;

  X = t->down;
  r = t->down->next;

  if(r->CSGTYPE != AY_LTDIFF)
    return AY_ERROR;

  if(!r->down)
    return AY_ERROR;

  Y = r->down;
  Z = r->down->next;

  // 2. transform hierarchy
  if(!(Xnew = (ay_object*)calloc(1, sizeof(ay_object))))
    return AY_EOMEM;
  memcpy(Xnew, X, sizeof(ay_object));
  if(!(s = (ay_object*)calloc(1, sizeof(ay_object))))
    {free(Xnew); return AY_EOMEM;}
  memcpy(s, t, sizeof(ay_object));

  t->CSGTYPE = AY_LTUNION;
  r->CSGTYPE = AY_LTDIFF;
  s->CSGTYPE = AY_LTINT;
  t->down = r;
  r->next = s;
  r->down = X;
  X->next = Y;
  Y->next = NULL;
  s->down = Xnew;
  Xnew->next = Z;
#ifdef AYCSGDBG
  printf("applied rule5\n");
#endif
 return AY_OK;
} // aycsg_applyrule5


// Rule6: X O ( Y - Z ) => ( X O Y ) - Z
//          t     s            s     t
int
aycsg_applyrule6(ay_object *t)
{
 ay_object *X, *Y, *Z, *s;

  // 1. check applicability of rule
  if(!t || !t->down || !t->down->next)
    return AY_ERROR;

  if(t->CSGTYPE != AY_LTINT)
    return AY_ERROR;

  X = t->down;

  s = t->down->next;

  if(s->CSGTYPE != AY_LTDIFF)
    return AY_ERROR;

  if(!s->down)
    return AY_ERROR;

  Y = s->down;
  Z = Y->next;

  // 2. transform hierarchy
  t->CSGTYPE = AY_LTDIFF;
  t->down = s;
  s->CSGTYPE = AY_LTINT;
  s->down = X;
  X->next = Y;
  s->next = Z;
  Y->next = NULL;
#ifdef AYCSGDBG
  printf("applied rule6\n");
#endif
 return AY_OK;
} // aycsg_applyrule6


// Rule7: ( X - Y ) O Z => ( X O Z ) - Y
//            r     t          r     t
int
aycsg_applyrule7(ay_object *t)
{
 ay_object *X, *Y, *Z, *r;

  // 1. check applicability of rule
  if(!t || !t->down || !t->down->next)
    return AY_ERROR;

  if(t->CSGTYPE != AY_LTINT)
    return AY_ERROR;

  r = t->down;
  Z = t->down->next;

  if(r->CSGTYPE != AY_LTDIFF)
    return AY_ERROR;

  if(!r->down)
    return AY_ERROR;

  X = r->down;
  Y = r->down->next;

  // 2. transform hierarchy
  t->CSGTYPE = AY_LTDIFF;
  r->CSGTYPE = AY_LTINT;
  t->down = r;
  r->next = Y;
  r->down = X;
  X->next = Z;

#ifdef AYCSGDBG
  printf("applied rule7\n");
#endif
 return AY_OK;
} // aycsg_applyrule7


// Rule8: ( X U Y ) O Z => ( X O Z ) U ( Y O Z )
//            r     t          r     t     s
int
aycsg_applyrule8(ay_object *t)
{
 ay_object *X, *Y, *Z, *Znew, *r, *s;

  // 1. check applicability of rule
  if(!t || !t->down || !t->down->next)
    return AY_ERROR;

  if(t->CSGTYPE != AY_LTINT)
    return AY_ERROR;

  r = t->down;
  Z = t->down->next;

  if(r->CSGTYPE != AY_LTUNION)
    return AY_ERROR;

  if(!r->down)
    return AY_ERROR;

  X = r->down;
  Y = r->down->next;

  // 2. transform hierarchy
  if(!(Znew = (ay_object*)calloc(1, sizeof(ay_object))))
    return AY_EOMEM;
  memcpy(Znew, Z, sizeof(ay_object));
  if(!(s = (ay_object*)calloc(1, sizeof(ay_object))))
    {free(Znew); return AY_EOMEM;}
  memcpy(s, t, sizeof(ay_object));

  t->CSGTYPE = AY_LTUNION;
  r->CSGTYPE = AY_LTINT;
  s->CSGTYPE = AY_LTINT;
  t->down = r;
  r->next = s;
  r->down = X;
  X->next = Z;
  s->down = Y;
  Y->next = Znew;
#ifdef AYCSGDBG
  printf("applied rule8\n");
#endif
 return AY_OK;
} // aycsg_applyrule8


// Rule9: ( X U Y ) - Z => ( X - Z ) U ( Y - Z )
//            r     t          r     t     s
int
aycsg_applyrule9(ay_object *t)
{
 ay_object *X, *Y, *Z, *Znew, *r, *s;

  // 1. check applicability of rule
  if(!t || !t->down || !t->down->next)
    return AY_ERROR;

  if(t->CSGTYPE != AY_LTDIFF)
    return AY_ERROR;

  r = t->down;
  Z = t->down->next;

  if(r->CSGTYPE != AY_LTUNION)
    return AY_ERROR;

  if(!r->down)
    return AY_ERROR;

  X = r->down;
  Y = r->down->next;

  // 2. transform hierarchy
  if(!(Znew = (ay_object*)calloc(1, sizeof(ay_object))))
    return AY_EOMEM;
  memcpy(Znew, Z, sizeof(ay_object));
  if(!(s = (ay_object*)calloc(1, sizeof(ay_object))))
    {free(Znew); return AY_EOMEM;}
  memcpy(s, t, sizeof(ay_object));

  t->CSGTYPE = AY_LTUNION;
  r->CSGTYPE = AY_LTDIFF;
  s->CSGTYPE = AY_LTDIFF;
  t->down = r;
  r->next = s;
  r->down = X;
  X->next = Z;
  s->down = Y;
  Y->next = Znew;
#ifdef AYCSGDBG
  printf("applied rule9\n");
#endif
 return AY_OK;
} // aycsg_applyrule9


// aycsg_normalize:
//  convert CSG tree <t> to disjunctive normal (sum of products) form
//  by _recursively_ applying the transformation rules above
int
aycsg_normalize(ay_object *t)
{
 int ay_status = AY_OK;
 int done, stop;

  if(!t)
    return AY_ENULL;

  if((t->CSGTYPE != AY_LTPRIM) && t->down && t->down->next)
    {
      do
	{
	  done = AY_FALSE;
	  while(!done)
	    {
	      done = AY_TRUE;
	      if(!aycsg_applyrule1(t))
		{
		  done = AY_FALSE;
		  continue;
		}
	      if(!aycsg_applyrule2(t))
		{
		  done = AY_FALSE;
		  continue;
		}
	      if(!aycsg_applyrule3(t))
		{
		  done = AY_FALSE;
		  continue;
		}
	      if(!aycsg_applyrule4(t))
		{
		  done = AY_FALSE;
		  continue;
		}
	      if(!aycsg_applyrule5(t))
		{
		  done = AY_FALSE;
		  continue;
		}
	      if(!aycsg_applyrule6(t))
		{
		  done = AY_FALSE;
		  continue;
		}
	      if(!aycsg_applyrule7(t))
		{
		  done = AY_FALSE;
		  continue;
		}
	      if(!aycsg_applyrule8(t))
		{
		  done = AY_FALSE;
		  continue;
		}
	      if(!aycsg_applyrule9(t))
		{
		  done = AY_FALSE;
		  continue;
		}
	    } // while ! done

	  if(t->down)
	    {
	      ay_status = aycsg_normalize(t->down);
	    }

	  if((t->CSGTYPE == AY_LTUNION) || (t->down &&
		(t->down->CSGTYPE != AY_LTUNION) &&
		(t->down->next && (t->down->next->CSGTYPE == AY_LTPRIM))))
	    {
	      stop = AY_TRUE;
	    }
	  else
	    {
	      stop = AY_FALSE;
	    } // if

	}
      while(!stop);

      if(t->down && t->down->next)
	{
	  ay_status = aycsg_normalize(t->down->next);
	}

    } // if t is not prim

 return ay_status;
} // aycsg_normalize


// aycsg_removetlu:
//  remove normal level and union objects in the toplevel of the
//  hierarchy (above all other CSG operations)
int
aycsg_removetlu(ay_object *o, ay_object **t)
{
 int ay_status = AY_OK;
 ay_object *last = NULL;
 //ay_level_object *l = NULL;

  // eliminate unions and levels above CSG trees
  while(o)
    {
      if(o->type == AY_IDLEVEL)
	{
	  //l = (ay_level_object *)(o->refine);
	  //if((l->type == AY_LTLEVEL) || (l->type == AY_LTLUNION))
	  if((o->CSGTYPE == AY_LTUNION) || (o->CSGTYPE == AY_LTPRIM))
	    {
	      // delegate transformation attributes
	      aycsg_delegatetrafo(o);

	      // replace o with its child(ren)
	      if(o->down)
		{
		  last = o->down;
		  while(last->next)
		    {
		      last = last->next;
		    }
		  last->next = o->next;
		}
	      else
		{
		  return AY_ENULL;
		} // if
	      *t = o->down;

	      free(o);
	      o = *t;
	    }
	  else
	    {
	      t = &(o->next);
	      o = o->next;
	    } // if
	}
      else
	{
	  t = &(o->next);
	  o = o->next;
	} // if
    } // while

 return ay_status;
} // aycsg_removetlu


// aycsg_delegatetrafo:
//  delegate transformation attributes from object <o> to its child(ren);
//  in contrast to ay_trafo_delegate(), this function also works properly
//  if the children already have arbitrary transformation attributes by
//  using TM tags to store additional transformations
int
aycsg_delegatetrafo(ay_object *o)
{
 int ay_status = AY_OK;
 ay_object *down = NULL;
 ay_tag *tag = NULL;
 aycsg_taglistelem *tle = NULL;
 double tm[16] = {0};

  if(!o)
    return AY_ENULL;

  if(!o->down)
    return AY_ERROR;

  down = o->down;

  if((o->movx != 0.0) || (o->movy != 0.0) || (o->movz != 0.0) ||
     (o->rotx != 0.0) || (o->roty != 0.0) || (o->rotz != 0.0) ||
     (o->scalx != 1.0) || (o->scaly != 1.0) || (o->scalz != 1.0) ||
     (o->quat[0] != 0.0) || (o->quat[1] != 0.0) || (o->quat[2] != 0.0) ||
     (o->quat[3] != 1.0) || (o->tags && (o->tags->type == aycsg_tm_tagtype)))
    {

      while(down)
	{

	  // has <o> a TM tag?
	  if(o->tags && (o->tags->type == aycsg_tm_tagtype))
	    {
	      // yes, copy it first (before the normal transformation
	      // attributes) to <down>

	      // has <down> already a TM tag?
	      if(down->tags && (down->tags->type == aycsg_tm_tagtype))
		{
		  // yes, just multiply new transformations into it
		  /*
		  memcpy(tm, o->tags->val, 16*sizeof(double));
		  ay_trafo_multmatrix(tm, (double*)down->tags->val);
		  memcpy(down->tags->val, tm, 16*sizeof(double));
		  */
		  ay_trafo_multmatrix((double*)down->tags->val,
				      (double*)o->tags->val);
		  if(o->tags->name)
		    {
		      if(down->tags->name)
			down->tags->name = (char*)AY_FALSE;
		      else
			down->tags->name = (char*)AY_TRUE;
		    }
		}
	      else
		{
		  // no, create, fill, and link a new tag
		  if(!(tag = (ay_tag*)calloc(1, sizeof(ay_tag))))
		    return AY_EOMEM;
		  tag->type = aycsg_tm_tagtype;
		  if(!(tag->val = (char*)calloc(16, sizeof(double))))
		    {
		      free(tag); return AY_EOMEM;
		    }
		  memcpy(tag->val, o->tags->val, 16*sizeof(double));
		  tag->name = (char*)o->tags->name;

		  if(!(tle = (aycsg_taglistelem*)calloc(1,
			       sizeof(aycsg_taglistelem))))
		    {
		      free(tag->val); free(tag); return AY_EOMEM;
		    }
		  tle->tag = tag;
		  tle->next = aycsg_tmtags;
		  aycsg_tmtags = tle;
		  tag->next = down->tags;
		  down->tags = tag;
		} // if
	    } // if

	  // has <down> already a TM tag?
	  if(down->tags && (down->tags->type == aycsg_tm_tagtype))
	    {
	      // yes, just multiply new transformations into it
	      /*
	      ay_trafo_creatematrix(o, tm);
	      ay_trafo_multmatrix(tm, (double*)down->tags->val);
	      memcpy(down->tags->val, tm, 16*sizeof(double));
	      */
	      ay_trafo_creatematrix(o, tm);
	      if((o->scalx*o->scaly*o->scalz)<0.0)
		{
		  if(down->tags->name)
		    down->tags->name = (char*)AY_FALSE;
		  else
		    down->tags->name = (char*)AY_TRUE;
		}
	      ay_trafo_multmatrix((double*)down->tags->val, tm);
	    }
	  else
	    {
	      // no, create, fill, and link a new tag
	      if(!(tag = (ay_tag*)calloc(1, sizeof(ay_tag))))
		return AY_EOMEM;
	      tag->type = aycsg_tm_tagtype;
	      if(!(tag->val = (char*)calloc(16, sizeof(double))))
		{
		  free(tag); return AY_EOMEM;
		}
	      ay_trafo_creatematrix(o, (double*)tag->val);
	      if((o->scalx*o->scaly*o->scalz)<0.0)
		{
		 tag->name = (char*)AY_TRUE;
		}

	      if(!(tle = (aycsg_taglistelem*)calloc(1,
			   sizeof(aycsg_taglistelem))))
		{
		  free(tag->val); free(tag); return AY_EOMEM;
		}
	      tle->tag = tag;
	      tle->next = aycsg_tmtags;
	      aycsg_tmtags = tle;
	      tag->next = down->tags;
	      down->tags = tag;
	    } // if

	  // copy material pointer
	  if(!down->mat)
	    {
	      if(o->mat)
		{
		  down->mat = o->mat;
		} // if
	    } // if

	  down = down->next;
	} // while

      // reset to default trafos
      o->movx = 0.0;
      o->movy = 0.0;
      o->movz = 0.0;

      o->scalx = 1.0;
      o->scaly = 1.0;
      o->scalz = 1.0;

      o->rotx = 0.0;
      o->roty = 0.0;
      o->rotz = 0.0;

      o->quat[0] = 0.0;
      o->quat[1] = 0.0;
      o->quat[2] = 0.0;
      o->quat[3] = 1.0;

      if(o->tags && (o->tags->type == aycsg_tm_tagtype))
	{
	  // no worries, we keep a pointer to the tm tag in aycsg_tmtags...
	  o->tags = o->tags->next;
	} // if
    } // if

 return ay_status;
} // aycsg_delegatetrafo


// aycsg_delegateall:
//  _recursively_ delegate transformation attributes from all CSG-operation
//  level objects to their children for subtree <t>
int
aycsg_delegateall(ay_object *t)
{
 int ay_status = AY_OK;
 ay_object *o = t;

  if(!t)
    return AY_ENULL;

  while(o)
    {

      if(o->down && (o->type == AY_IDLEVEL) && (o->CSGTYPE != AY_LTPRIM))
	{
	  ay_status = aycsg_delegatetrafo(o);
	} // if

      if(o->down && (o->CSGTYPE != AY_LTPRIM))
	{
	  ay_status = aycsg_delegateall(o->down);
	} // if

      o = o->next;
    } // while

 return ay_status;
} // aycsg_delegateall


// aycsg_binarify:
//  _recursively_ convert n-ary subtree below <parent> to binary form,
//  creating copies of <parent> that are inserted in the existing list
//  of children
//  <left>: first new child of the new level
//  <target>: pointer to which new level should be chained
int
aycsg_binarify(ay_object *parent, ay_object *left, ay_object **target)
{
 int ay_status = AY_OK;
 ay_object *tmp = NULL;

  if((!parent) || (!left) || (!target))
    return AY_ENULL;

  if(!left->next)
    { // nothing to be done? probably called wrongly...
      return AY_OK;
    }

  // create a new intermediate level object
  if(!(tmp = (ay_object*)calloc(1, sizeof(ay_object))))
    return AY_EOMEM;

  memcpy(tmp, parent, sizeof(ay_object));

  // make sure, that the transformation attributes remain at the
  // original top level object
  ay_trafo_defaults(tmp);

  if(tmp->tags && (tmp->tags->type == aycsg_tm_tagtype))
    {
      tmp->tags = tmp->tags->next;
    }

  // differences translate to a difference of unions!
  if(parent->CSGTYPE == AY_LTDIFF)
    tmp->CSGTYPE = AY_LTUNION;

  // connect children to new level object
  tmp->next = NULL;
  tmp->down = left;

  // finished?
  if(left->next->next)
    { // no, there are (still) more than two children
      // => wee need another intermediate level object
      ay_status = aycsg_binarify(parent, left->next, &(left->next));
    }

  // link the new level object to the hierarchy
  *target = tmp;

 return AY_OK;
} // aycsg_binarify


// aycsg_copytree:
//  _recursively_ copies the tree pointed to by <t> into <target>;
//  omits terminating end-level objects!,
//  descends just into level objects, does not copy type specific objects,
//  converts to binary form, informs caller via <is_csg> whether subtree
//  <t> contains CSG operations, removes parents with a single child,
//  if <sel_only> is AY_TRUE: just copies selected objects
int
aycsg_copytree(int sel_only, ay_object *t, int *is_csg, ay_object **target)
{
 int ay_status = AY_OK;
 int lis_csg = AY_FALSE;
 ay_level_object *l = NULL;
 ay_object *tmp = NULL;

  if(!t)
    return AY_ENULL;

  while(t->next)
    {
      if(t->hide || (sel_only && !t->selected))
	{
	  t = t->next;
	  continue;
	}

      if(!(*target = (ay_object*)calloc(1, sizeof(ay_object))))
	return AY_EOMEM;

      memcpy(*target, t, sizeof(ay_object));
      (*target)->next = NULL;
      (*target)->down = NULL;

      ay_status = AY_OK;

      // we just descend into non-empty level objects (and NURBS patches)
      if(((t->type == AY_IDLEVEL) || (t->type == AY_IDNPATCH)) && t->down &&
	 t->down->next)
	{
	  // descend
	  ay_status = aycsg_copytree(AY_FALSE,
				     t->down, &lis_csg, &((*target)->down));

	  // repair hierarchy for NURBS patches with trim curves
	  // (re-create terminating end level objects)
	  if(t->type == AY_IDNPATCH)
	    {
	      tmp = (*target)->down;
	      if(tmp)
		{
		  while(tmp->next)
		    tmp = tmp->next;
		  tmp->next = ay_endlevel;
		} // if
	    } // if
	} // if

      if(ay_status)
	return ay_status;

      // check for (and discard) level objects with just one child
      if((*target)->type == AY_IDLEVEL)
	{
	  if(((*target)->down) && (!(*target)->down->next))
	    {
	      // t has just one child, and may be discarded
	      aycsg_delegatetrafo(*target);
	      tmp = (*target)->down;
	      free(*target);
	      *target = tmp;
	      t = t->next;
	      target = &((*target)->next);
	      continue;
	    } // if
	} // if

      // we use the "modified" (CSGTYPE) flag to remember whether an object is
      // a primitive or a CSG operation and if it is a CSG op. of which type
      // (see ayam.h Level Object SubType Ids)

      if((*target)->type != AY_IDLEVEL)
	{
	  (*target)->CSGTYPE = AY_LTPRIM;
	}
      else
	{
	  l = (ay_level_object *)((*target)->refine);
	  switch(l->type)
	    {
	    case AY_LTUNION:
	      (*target)->CSGTYPE = AY_LTUNION;
	      break;
	    case AY_LTINT:
	      (*target)->CSGTYPE = AY_LTINT;
	      break;
	    case AY_LTDIFF:
	      (*target)->CSGTYPE = AY_LTDIFF;
	      break;
	    case AY_LTPRIM:
	      (*target)->CSGTYPE = AY_LTPRIM;
	      break;
	    default:
	      // CSGTYPE == AY_LTLEVEL
	      if(lis_csg)
		{
		  (*target)->CSGTYPE = AY_LTUNION;
		}
	      else
		{
		  (*target)->CSGTYPE = AY_LTPRIM;
		}
	      break;
	    } // switch

	  if(lis_csg || ((l->type > 1) && (l->type < 5)))
	    {
	      lis_csg = AY_TRUE;

	      if((*target)->down && (*target)->down->next &&
		 (*target)->down->next->next)
		{
		  // *target has more than 2 children
		  // => convert to binary tree
		  ay_status = aycsg_binarify(*target, (*target)->down->next,
					     &((*target)->down->next));


		} // if
	    } // if
	} // if

      if(lis_csg)
	*is_csg = AY_TRUE;

      t = t->next;
      target = &((*target)->next);
    } // while

 return ay_status;
} // aycsg_copytree


// aycsg_cleartree:
//  clear the copy of the scene tree created by aycsg_copytree()
void
aycsg_cleartree(ay_object *t)
{
 ay_object *temp = NULL;

  while(t)
    {
      if(((t->type == AY_IDLEVEL) || (t->type == AY_IDNPATCH)) && t->down
	 && t->down->next)
	{
	  aycsg_cleartree(t->down);
	} // if

      temp = t->next;
      if(t != ay_endlevel)
	free(t);
      t = temp;
    } // while

 return;
} // aycsg_cleartree


// aycsg_cleartmtags:
//
void
aycsg_cleartmtags()
{
 aycsg_taglistelem *tle = NULL;

  while(aycsg_tmtags)
    {
      tle = aycsg_tmtags->next;

      if(aycsg_tmtags->tag)
	{
	  if(aycsg_tmtags->tag->val)
	    free(aycsg_tmtags->tag->val);
	  free(aycsg_tmtags->tag);
	} // if
      free(aycsg_tmtags);

      aycsg_tmtags = tle;
    } // while

 return;
} // aycsg_cleartmtags


extern "C" {


// aycsg_display:
//  this small wrapper makes aycsg_rendertcb() callable as ToglDisplayFunc
void
aycsg_display(struct Togl *togl)
{

  aycsg_rendertcb(togl, 0, NULL);

 return;
} // aycsg_display


// aycsg_toggletcb:
//  this callback toggles the display callback of view <togl> between
//  standard behaviour and aycsg_rendertcb()
int
aycsg_toggletcb(struct Togl *togl, int argc, char *argv[])
{
 ay_view_object *view = NULL;

  if(!togl)
    return TCL_OK;
  view = (ay_view_object *)Togl_GetClientData(togl);

  if(view->altdispcb)
    {
      view->altdispcb = NULL;
    }
  else
    {
      view->altdispcb = aycsg_display;
    }

 return TCL_OK;
} // aycsg_toggletcb


// aycsg_setopttcmd:
//  this Tcl command transports the AyCSG preferences
//  from the "Special/AyCSG Preferences"-dialog to OpenCSG options
//
int
aycsg_setopttcmd(ClientData clientData, Tcl_Interp *interp,
		 int argc, char *argv[])
{
 OpenCSG::Algorithm algo = OpenCSG::Automatic;
 OpenCSG::DepthComplexityAlgorithm depthalgo =
   OpenCSG::NoDepthComplexitySampling;
 OpenCSG::OffscreenType offscreen = OpenCSG::AutomaticOffscreenType;
 OpenCSG::Optimization opti = OpenCSG::OptimizationDefault;
 int opencsg_algorithm = 0, opencsg_dcsampling = 0;
 int opencsg_offscreen = 0, opencsg_optimization = 0;
 Tcl_Obj *to = NULL;
 char arr[] = "aycsg_options";

  // get preferences data
  to = Tcl_GetVar2Ex(interp, arr, "Algorithm",
		     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
  if(to)
    Tcl_GetIntFromObj(interp, to, &opencsg_algorithm);

  switch(opencsg_algorithm)
    {
    case 0:
      algo = OpenCSG::Automatic;
      break;
    case 1:
      algo = OpenCSG::Goldfeather;
      break;
    case 2:
      algo = OpenCSG::SCS;
      break;
    default:
      algo = OpenCSG::Automatic;
      break;
    } // switch


  to = Tcl_GetVar2Ex(interp, arr, "DCSampling",
		     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
  if(to)
    Tcl_GetIntFromObj(interp, to, &opencsg_dcsampling);

  switch(opencsg_dcsampling)
    {
    case 0:
      depthalgo = OpenCSG::NoDepthComplexitySampling;
      break;
    case 1:
      depthalgo = OpenCSG::OcclusionQuery;
      break;
    case 2:
      depthalgo = OpenCSG::DepthComplexitySampling;
      break;
    default:
      depthalgo = OpenCSG::NoDepthComplexitySampling;
      break;
    } // switch

  to = Tcl_GetVar2Ex(interp, arr, "CalcBBS",
		     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
  if(to)
    Tcl_GetIntFromObj(interp, to, &aycsg_calcbbs);

  to = Tcl_GetVar2Ex(interp, arr, "OffscreenType",
		     TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
  if(to)
    Tcl_GetIntFromObj(interp, to, &opencsg_offscreen);

  switch(opencsg_offscreen)
    {
    case 0:
      offscreen = OpenCSG::AutomaticOffscreenType;
      break;
    case 1:
      offscreen = OpenCSG::FrameBufferObject;
      break;
    case 2:
      offscreen = OpenCSG::PBuffer;
      break;
    case 3:
      offscreen = OpenCSG::FrameBufferObjectARB;
      break;
    case 4:
      offscreen = OpenCSG::FrameBufferObjectEXT;
      break;
    default:
      offscreen = OpenCSG::AutomaticOffscreenType;
      break;
    } // switch

  to = Tcl_GetVar2Ex(interp, arr, "Optimization",
		    TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
  if(to)
    Tcl_GetIntFromObj(interp, to, &opencsg_optimization);

  switch(opencsg_optimization)
    {
    case 0:
      opti = OpenCSG::OptimizationDefault;
      break;
    case 1:
      opti = OpenCSG::OptimizationForceOn;
      break;
    case 2:
      opti = OpenCSG::OptimizationOn;
      break;
    case 3:
      opti = OpenCSG::OptimizationOff;
      break;
    default:
      opti = OpenCSG::OptimizationDefault;
      break;
    } // switch

  // set the options
  OpenCSG::setOption(OpenCSG::AlgorithmSetting, algo);
  OpenCSG::setOption(OpenCSG::DepthComplexitySetting, depthalgo);
  OpenCSG::setOption(OpenCSG::OffscreenSetting, offscreen);
  OpenCSG::setOption(OpenCSG::DepthBoundsOptimization, opti);

 return TCL_OK;
} // aycsg_setopttcmd


// Aycsg_Init | aycsg_inittcmd:
//  initialize aycsg module
//  note: this function _must_ be capitalized exactly this way
//  regardless of the filename of the shared object (see: man n load)!
#ifdef WIN32
  __declspec (dllexport)
#endif // WIN32
int
Aycsg_Init(Tcl_Interp *interp)
{
 char fname[] = "Aycsg_Init";
 int err;
 int ay_status = AY_OK;

#ifdef WIN32
  if(Tcl_InitStubs(interp, "8.2", 0) == NULL)
    {
      return TCL_ERROR;
    }
#endif // WIN32

  // first, check versions
  if(ay_checkversion(fname, aycsg_version_ma, aycsg_version_mi))
    {
      return TCL_ERROR;
    }

  aycsg_root = NULL;

  // register TM tag type
  ay_status = ay_tags_register(aycsg_tm_tagname, &aycsg_tm_tagtype);
  if(ay_status)
    return TCL_OK;

  aycsg_tmtags = NULL;

  // register DC tag type
  ay_status = ay_tags_register(aycsg_dc_tagname, &aycsg_dc_tagtype);
  if(ay_status)
    return TCL_OK;

#ifdef AYCSGDBG
  ay_ppoh_init(interp);
#endif

  Tcl_CreateCommand(interp, "aycsgSetOpt", aycsg_setopttcmd,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  // source aycsg.tcl, it contains Tcl-code for new key bindings etc.
  if((Tcl_EvalFile(interp, "aycsg.tcl")) != TCL_OK)
     {
       ay_error(AY_ERROR, fname, "Error while sourcing \"aycsg.tcl\"!");
       return TCL_OK;
     }

  // initialize GLEW
  err = glewInit();
  if(GLEW_OK != err)
    {
      // problem: glewInit failed, something is seriously wrong
      ay_error(AY_ERROR, fname, "GLEW Initialization failed:");
      ay_error(AY_ERROR, fname, (char*)glewGetErrorString(err));
      return TCL_OK;
    }

  // create new commands for all views (Togl widgets)
  Togl_CreateCommand("rendercsg", aycsg_rendertcb);
  Togl_CreateCommand("togglecsg", aycsg_toggletcb);

  // reconnect potentially present DC tags
  ay_tags_reconnect(ay_root, aycsg_dc_tagtype, aycsg_dc_tagname);

  ay_error(AY_EOUTPUT, fname, "Plugin 'aycsg' successfully loaded.");

 return TCL_OK;
} // Aycsg_Init | aycsg_inittcmd

} // extern "C"

// remove local preprocessor definitions
#undef CSGTYPE
