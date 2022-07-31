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

/* tess.c NURBS (and PolyMesh) tesselation tools */

/* types local to this module */

typedef struct ay_tess_listobject_s
{
  struct ay_tess_listobject_s *next;
  void *data;
} ay_tess_listobject;

typedef struct ay_tess_tri_s {
  struct ay_tess_tri_s *next;
  int is_quad;

  double p1[3];
  double p2[3];
  double p3[3];

  double n1[3];
  double n2[3];
  double n3[3];

  double c1[4];
  double c2[4];
  double c3[4];

  double t1[2];
  double t2[2];
  double t3[2];

} ay_tess_tri;

typedef struct ay_tess_object_s {
  int has_vn;
  int has_vc;
  int has_tc;
  int tesselate_polymesh;

  GLenum type;

  int count;
  int startup;
  int toggle;

  double *p1;
  double *p2;
  double *p3;
  double *p4;

  double *n1;
  double *n2;
  double *n3;
  double *n4;

  double *c1;
  double *c2;
  double *c3;
  double *c4;

  double *t1;
  double *t2;
  double *t3;
  double *t4;

  double **nextpd;
  double **nextnd;
  double **nextcd;
  double **nexttd;

  struct ay_tess_tri_s *tris;
} ay_tess_object;

typedef struct ay_curvetess_s {
  int is_rat;
  double *verts;
  int vertslen;
  int vindex;
} ay_curvetess;

/* prototypes of functions local to this module */


int ay_tess_checkquad(double *p1, double *p2, double *p3, double *p4,
		      int check_convexity);

void ay_tess_createtri(ay_tess_object *to);

void ay_tess_createtrirev(ay_tess_object *to);

void ay_tess_begindata(GLenum type, void *userData);

void ay_tess_vertexdata(GLfloat *vertex, void *userData);

void ay_tess_normaldata(GLfloat *normal, void *userData);

void ay_tess_colordata(GLfloat *color, void *userData);

void ay_tess_texcoorddata(GLfloat *texcoord, void *userData);

void ay_tess_enddata(void *userData);

void ay_tess_combinedata(GLdouble c[3], void *d[4], GLfloat w[4], void **out,
			 void *userData);

void ay_tess_managecombined(void *userData);

int ay_tess_addtag(ay_object *o, char *val);

int ay_tess_tristoquad(double **t1, double **t2, double quad_eps, int *q);

int ay_tess_tristoquadpomesh(ay_tess_tri *tris,
			     int has_vn, int has_vc, int has_tc,
			     char *myst, char *myvc, double quad_eps,
			     ay_object **result);

int ay_tess_tristomixedpomesh(ay_tess_tri *tris,
			      int has_vn, int has_vc, int has_tc,
			      char *myst, char *myvc, double quad_eps,
			      ay_object **result);

int ay_tess_tristopomesh(ay_tess_tri *tris,
			 int has_vn, int has_vc, int has_tc,
			 char *myst, char *myvc, ay_object **result);

int ay_tess_isdegenatv1(ay_nurbpatch_object *np);


void ay_tess_surfacepoint(ay_nurbpatch_object *np, double u, double v,
			  double *t, double *p);

int ay_tess_tristouv(ay_tess_object *to, unsigned int ntris, double **result);

void ay_tess_checknormal(double *N1, double *N2, double *NN);


/* functions */

/* ay_tess_checktri:
 *  check triangle built from p1,p2,p3 for degeneracy;
 *  returns AY_FALSE if triangle is degenerated,
 *  otherwise returns AY_TRUE
 */
int
ay_tess_checktri(double *p1, double *p2, double *p3)
{

  /* check p1 vs. p2 */
  if((fabs(p1[0] - p2[0]) < AY_EPSILON) &&
     (fabs(p1[1] - p2[1]) < AY_EPSILON) &&
     (fabs(p1[2] - p2[2]) < AY_EPSILON))
      return AY_FALSE;

  /* check p2 vs. p3 */
  if((fabs(p2[0] - p3[0]) < AY_EPSILON) &&
     (fabs(p2[1] - p3[1]) < AY_EPSILON) &&
     (fabs(p2[2] - p3[2]) < AY_EPSILON))
      return AY_FALSE;

  /* check p3 vs. p1 */
  if((fabs(p3[0] - p1[0]) < AY_EPSILON) &&
     (fabs(p3[1] - p1[1]) < AY_EPSILON) &&
     (fabs(p3[2] - p1[2]) < AY_EPSILON))
      return AY_FALSE;

 return AY_TRUE;
} /* ay_tess_checktri */


/* ay_tess_checkquad:
 *  check quad built from p1,p2,p3,p4 for degeneracy (line shape, convexity);
 *  returns AY_FALSE if quad is degenerated,
 *  otherwise returns AY_TRUE
 */
int
ay_tess_checkquad(double *p1, double *p2, double *p3, double *p4,
		  int check_convexity)
{
 double V1[3], V2[3], N[16], *n, angle, len;
 int i, j, cnt = 4;

  AY_V3SUB(V1, p2, p1);
  len = AY_V3LEN(V1);
  if(len > AY_EPSILON)
    AY_V3SCAL(V1, 1.0/len)
  AY_V3SUB(V2, p4, p1);
  len = AY_V3LEN(V2);
  if(len > AY_EPSILON)
    AY_V3SCAL(V2, 1.0/len)
  angle = acos(AY_V3DOT(V1, V2));
  if(angle != angle || fabs(angle) < AY_EPSILON ||
     fabs(angle) > AY_PI-AY_EPSILON)
    cnt--;

  n = N;
  AY_V3CROSS(n, V1, V2);

  AY_V3SUB(V1, p3, p2);
  len = AY_V3LEN(V1);
  if(len > AY_EPSILON)
    AY_V3SCAL(V1, 1.0/len)
  AY_V3SUB(V2, p1, p2);
  len = AY_V3LEN(V2);
  if(len > AY_EPSILON)
    AY_V3SCAL(V2, 1.0/len)
  angle = acos(AY_V3DOT(V1, V2));
  if(angle != angle || fabs(angle) < AY_EPSILON ||
     fabs(angle) > AY_PI-AY_EPSILON)
    cnt--;

  n = &(N[3]);
  AY_V3CROSS(n, V1, V2);

  AY_V3SUB(V1, p4, p3);
  len = AY_V3LEN(V1);
  if(len > AY_EPSILON)
    AY_V3SCAL(V1, 1.0/len)
  AY_V3SUB(V2, p2, p3);
  len = AY_V3LEN(V2);
  if(len > AY_EPSILON)
    AY_V3SCAL(V2, 1.0/len)
  angle = acos(AY_V3DOT(V1, V2));
  if(angle != angle || fabs(angle) < AY_EPSILON ||
     fabs(angle) > AY_PI-AY_EPSILON)
    cnt--;

  n = &(N[6]);
  AY_V3CROSS(n, V1, V2);

  AY_V3SUB(V1, p1, p4);
  len = AY_V3LEN(V1);
  if(len > AY_EPSILON)
    AY_V3SCAL(V1, 1.0/len)
  AY_V3SUB(V2, p3, p4);
  len = AY_V3LEN(V2);
  if(len > AY_EPSILON)
    AY_V3SCAL(V2, 1.0/len)
  angle = acos(AY_V3DOT(V1, V2));
  if(angle != angle || fabs(angle) < AY_EPSILON ||
     fabs(angle) > AY_PI-AY_EPSILON)
    cnt--;

  n = &(N[9]);
  AY_V3CROSS(n, V1, V2);

  if(cnt < 3)
    {
      return AY_FALSE;
    }

  /* check signs of normals */
  if(check_convexity)
    {
      memcpy(&(N[12]), N, 3*sizeof(double));
      for(i = 0; i < 3; i++)
	{
	  for(j = 0; j < 3; j++)
	    {
	      if((fabs(N[i*3+j]) > AY_EPSILON) &&
		 (fabs(N[(i+1)*3+j]) > AY_EPSILON))
		{
		  if(((N[i*3+j] < 0) && (N[(i+1)*3+j] > 0)) ||
		     ((N[i*3+j] > 0) && (N[(i+1)*3+j] < 0)))
		    {
		      return AY_FALSE;
		    }
		}
	    }
	}
    }

 return AY_TRUE;
} /* ay_tess_checkquad */


/* ay_tess_createtri:
 *  create triangle from to->p1, to->p2, to->p3 and link it to <to>
 */
void
ay_tess_createtri(ay_tess_object *to)
{
 ay_tess_tri *tri = NULL;

  if(!(tri = malloc(sizeof(ay_tess_tri))))
    return;

  tri->next = to->tris;
  to->tris = tri;

  tri->is_quad = AY_FALSE;

  memcpy(tri->p1, to->p1, 3*sizeof(double));
  memcpy(tri->p2, to->p2, 3*sizeof(double));
  memcpy(tri->p3, to->p3, 3*sizeof(double));

  if(to->has_vn)
    {
      memcpy(tri->n1, to->n1, 3*sizeof(double));
      memcpy(tri->n2, to->n2, 3*sizeof(double));
      memcpy(tri->n3, to->n3, 3*sizeof(double));
    }

  if(to->has_vc)
    {
      memcpy(tri->c1, to->c1, 4*sizeof(double));
      memcpy(tri->c2, to->c2, 4*sizeof(double));
      memcpy(tri->c3, to->c3, 4*sizeof(double));
    }

  if(to->has_tc)
    {
      memcpy(tri->t1, to->t1, 2*sizeof(double));
      memcpy(tri->t2, to->t2, 2*sizeof(double));
      memcpy(tri->t3, to->t3, 2*sizeof(double));
    }

 return;
} /* ay_tess_createtri */


/* ay_tess_createtrirev:
 *  create triangle from to->p3, to->p2, to->p1 and link it to <to>
 */
void
ay_tess_createtrirev(ay_tess_object *to)
{
 ay_tess_tri *tri = NULL;

  if(!(tri = malloc(sizeof(ay_tess_tri))))
    return;

  tri->next = to->tris;
  to->tris = tri;

  tri->is_quad = AY_FALSE;

  memcpy(tri->p1, to->p3, 3*sizeof(double));
  memcpy(tri->p2, to->p2, 3*sizeof(double));
  memcpy(tri->p3, to->p1, 3*sizeof(double));

  if(to->has_vn)
    {
      memcpy(tri->n1, to->n3, 3*sizeof(double));
      memcpy(tri->n2, to->n2, 3*sizeof(double));
      memcpy(tri->n3, to->n1, 3*sizeof(double));
    }

  if(to->has_vc)
    {
      memcpy(tri->c1, to->c3, 4*sizeof(double));
      memcpy(tri->c2, to->c2, 4*sizeof(double));
      memcpy(tri->c3, to->c1, 4*sizeof(double));
    }

  if(to->has_tc)
    {
      memcpy(tri->t1, to->t3, 2*sizeof(double));
      memcpy(tri->t2, to->t2, 2*sizeof(double));
      memcpy(tri->t3, to->t1, 2*sizeof(double));
    }

 return;
} /* ay_tess_createtrirev */


/* ay_tess_begindata:
 *  tesselation callback
 *
 */
void
ay_tess_begindata(GLenum type, void *userData)
{
 ay_tess_object *to;

  to = (ay_tess_object *)userData;

  to->type = type;

  switch(type)
    {
    case GL_TRIANGLE_FAN:
      to->count = 3;
      to->startup = 3;
      break;
    case GL_TRIANGLE_STRIP:
      to->count = 3;
      to->startup = 3;
      to->toggle = 0;
      break;
    case GL_TRIANGLES:
      to->count = 3;
      to->startup = 3;
      break;
    case GL_QUAD_STRIP:
      to->count = 4;
      to->startup = 4;
      break;
    } /* switch */

 return;
} /* ay_tess_begindata */


/* ay_tess_vertexdata:
 *  tesselation callback
 *  this callback is used by both NURBS and PolyMesh tesselation
 *  and thus needs to check, whether <vertex> points to floats (NURBS case)
 *  or doubles (PolyMesh case)
 */
void
ay_tess_vertexdata(GLfloat *vertex, void *userData)
{
 ay_tess_object *to;
 double *t = NULL;
 double *nextpd;
 ay_tess_tri *tri = NULL;
 int check_quad;

  to = (ay_tess_object *)userData;

  to->count--;

  if(to->startup > 0)
    {
      to->startup--;
    }

  nextpd = *to->nextpd;

  if(to->tesselate_polymesh)
    {
      t = (double*)vertex;
      nextpd[0] = t[0];
      nextpd[1] = t[1];
      nextpd[2] = t[2];

      /* if tesselating PolyMeshes, we may also get vertex normals,
	 which we can store in the tesselation object by calling
	 the normal data callback by ourselves */
      if(to->has_vn)
	{
	  ay_tess_normaldata((GLfloat*)&(t[3]), userData);
	}

      t = NULL;
    }
  else
    {
      nextpd[0] = (double)vertex[0];
      nextpd[1] = (double)vertex[1];
      nextpd[2] = (double)vertex[2];
    }

  if((*(to->nextpd)) == to->p1)
    {
      to->nextpd = &(to->p2);
    }
  else
  if((*(to->nextpd)) == to->p2)
    {
      to->nextpd = &(to->p3);
    }
  else
  if((*(to->nextpd)) == to->p3)
    {
      to->nextpd = &(to->p4);
    }
  else
  if((*(to->nextpd)) == to->p4)
    {
      to->nextpd = &(to->p1);
    }

  if(to->count == 0)
    {
      switch(to->type)
	{
	case GL_TRIANGLE_FAN:
	  to->count = 1;
	  /*printf("Fan\n");*/
	  if(to->startup == 0)
	    {
	      /* create new triangle */
	      if(ay_tess_checktri(to->p1, to->p2, to->p3))
		{
		  ay_tess_createtri(to);
		} /* if */

	      /* shift vertex/normal/color/texcoord data */
	      t = to->p2;
	      to->p2 = to->p3;
	      to->p3 = t;

	      t = to->n2;
	      to->n2 = to->n3;
	      to->n3 = t;

	      t = to->c2;
	      to->c2 = to->c3;
	      to->c3 = t;

	      t = to->t2;
	      to->t2 = to->t3;
	      to->t3 = t;

	      to->nextpd = &(to->p3);
	      to->nextnd = &(to->n3);
	      to->nextcd = &(to->c3);
	      to->nexttd = &(to->t3);
	    } /* if */
	  break;
	case GL_TRIANGLE_STRIP:
	  to->count = 1;
	  /*printf("Strip\n");*/
	  if(to->startup == 0)
	    {
	      /* create new triangle */
	      if(ay_tess_checktri(to->p1, to->p2, to->p3))
		{
		  if(to->toggle)
		    {
		      ay_tess_createtrirev(to);
		      to->toggle = 0;
		    }
		  else
		    {
		      ay_tess_createtri(to);
		      to->toggle = 1;
		    }
		} else {
		  if(to->toggle)
		    {
		      to->toggle = 0;
		    }
		  else
		    {
		      to->toggle = 1;
		    }
	      } /* if */

	      /* shift vertex/normal/color/texcoord data */
	      t = to->p1;
	      to->p1 = to->p2;
	      to->p2 = to->p3;
	      to->p3 = to->p4;
	      to->p4 = t;

	      t = to->n1;
	      to->n1 = to->n2;
	      to->n2 = to->n3;
	      to->n3 = to->n4;
	      to->n4 = t;

	      t = to->c1;
	      to->c1 = to->c2;
	      to->c2 = to->c3;
	      to->c3 = to->c4;
	      to->c4 = t;

	      t = to->t1;
	      to->t1 = to->t2;
	      to->t2 = to->t3;
	      to->t3 = to->t4;
	      to->t4 = t;

	      to->nextpd = &(to->p3);
	      to->nextnd = &(to->n3);
	      to->nextcd = &(to->c3);
	      to->nexttd = &(to->t3);
	    } /* if */
	  break;
	case GL_TRIANGLES:
	  to->count = 3;
	  /*printf("Tri\n");*/
	  if(to->startup == 0)
	    {
	      /* create new triangle */
	      if(ay_tess_checktri(to->p1, to->p2, to->p3))
		{
		  ay_tess_createtri(to);
		} /* if */

	      to->nextpd = &(to->p1);
	      to->nextnd = &(to->n1);
	      to->nextcd = &(to->c1);
	      to->nexttd = &(to->t1);
	    } /* if */
	  break;
	case GL_QUAD_STRIP:
	  to->count = 2;
	  /*printf("QuadStrip\n");*/
	  if(to->startup == 0)
	    {
	      check_quad = AY_FALSE;
	      /* create two new triangles */
	      if(ay_tess_checktri(to->p1, to->p2, to->p3))
		{
		  ay_tess_createtri(to);
		  to->tris->is_quad = AY_TRUE;
		  check_quad = AY_TRUE;
		} /* if */

	      if(ay_tess_checktri(to->p2, to->p4, to->p3))
		{
		  if(!(tri = malloc(sizeof(ay_tess_tri))))
		    return;
		  tri->next = to->tris;
		  to->tris = tri;
		  if(check_quad)
		    tri->is_quad = AY_TRUE;
		  else
		    tri->is_quad = AY_FALSE;
		  memcpy(tri->p1, to->p2, 3*sizeof(double));
		  memcpy(tri->p2, to->p4, 3*sizeof(double));
		  memcpy(tri->p3, to->p3, 3*sizeof(double));
		  if(to->has_vn)
		    {
		      memcpy(tri->n1, to->n2, 3*sizeof(double));
		      memcpy(tri->n2, to->n4, 3*sizeof(double));
		      memcpy(tri->n3, to->n3, 3*sizeof(double));
		    }
		  if(to->has_vc)
		    {
		      memcpy(tri->c1, to->c2, 4*sizeof(double));
		      memcpy(tri->c2, to->c4, 4*sizeof(double));
		      memcpy(tri->c3, to->c3, 4*sizeof(double));
		    }
		  if(to->has_tc)
		    {
		      memcpy(tri->t1, to->t2, 2*sizeof(double));
		      memcpy(tri->t2, to->t4, 2*sizeof(double));
		      memcpy(tri->t3, to->t3, 2*sizeof(double));
		    }
		}
	      else
		{
		  if(check_quad)
		    to->tris->is_quad = AY_FALSE;
		}
	      /* shift vertex/normal/color/texcoord data */
	      t = to->p1;
	      to->p1 = to->p3;
	      to->p3 = t;
	      t = to->p2;
	      to->p2 = to->p4;
	      to->p4 = t;

	      t = to->n1;
	      to->n1 = to->n3;
	      to->n3 = t;
	      t = to->n2;
	      to->n2 = to->n4;
	      to->n4 = t;

	      t = to->c1;
	      to->c1 = to->c3;
	      to->c3 = t;
	      t = to->c2;
	      to->c2 = to->c4;
	      to->c4 = t;

	      t = to->t1;
	      to->t1 = to->t3;
	      to->t3 = t;
	      t = to->t2;
	      to->t2 = to->t4;
	      to->t4 = t;

	      to->nextpd = &(to->p3);
	      to->nextnd = &(to->n3);
	      to->nextcd = &(to->c3);
	      to->nexttd = &(to->t3);
	    } /* if */
	  break;
	} /* switch */
    } /* if */

 return;
} /* ay_tess_vertexdata */


/* ay_tess_normaldata:
 *  tesselation callback
 *  this callback is used by both NURBS and PolyMesh tesselation
 *  and thus needs to check, whether <normal> points to floats (NURBS case)
 *  or doubles (PolyMesh case)
 */
void
ay_tess_normaldata(GLfloat *normal, void *userData)
{
 ay_tess_object *to;
 double *nextnd, *t;

  to = (ay_tess_object *)userData;

  nextnd = *to->nextnd;

  if(to->tesselate_polymesh)
    {
      t = (double*)normal;
      nextnd[0] = t[0];
      nextnd[1] = t[1];
      nextnd[2] = t[2];
    }
  else
    {
      nextnd[0] = (double)normal[0];
      nextnd[1] = (double)normal[1];
      nextnd[2] = (double)normal[2];
    }

  if((*(to->nextnd)) == to->n1)
    {
      to->nextnd = &(to->n2);
    }
  else
  if((*(to->nextnd)) == to->n2)
    {
      to->nextnd = &(to->n3);
    }
  else
  if((*(to->nextnd)) == to->n3)
    {
      to->nextnd = &(to->n4);
    }
  else
  if((*(to->nextnd)) == to->n4)
    {
      to->nextnd = &(to->n1);
    }

 return;
} /* ay_tess_normaldata */


/* ay_tess_colordata:
 *  tesselation callback for color data
 *  this callback is used by both NURBS and PolyMesh tesselation
 *  and thus needs to check, whether <color> points to floats (NURBS case)
 *  or doubles (PolyMesh case)
 */
void
ay_tess_colordata(GLfloat *color, void *userData)
{
 ay_tess_object *to;
 double *nextcd, *t;

  to = (ay_tess_object *)userData;

  nextcd = *to->nextcd;

  if(to->tesselate_polymesh)
    {
      t = (double*)color;
      nextcd[0] = t[0];
      nextcd[1] = t[1];
      nextcd[2] = t[2];
    }
  else
    {
      nextcd[0] = (double)color[0];
      nextcd[1] = (double)color[1];
      nextcd[2] = (double)color[2];
    }

  if((*(to->nextcd)) == to->c1)
    {
      to->nextcd = &(to->c2);
    }
  else
  if((*(to->nextcd)) == to->c2)
    {
      to->nextcd = &(to->c3);
    }
  else
  if((*(to->nextcd)) == to->c3)
    {
      to->nextcd = &(to->c4);
    }
  else
  if((*(to->nextcd)) == to->c4)
    {
      to->nextcd = &(to->c1);
    }

 return;
} /* ay_tess_colordata */


/* ay_tess_texcoorddata:
 *  tesselation callback
 *  this callback is used by both NURBS and PolyMesh tesselation
 *  and thus needs to check, whether <texcoord> points to floats (NURBS case)
 *  or doubles (PolyMesh case)
 */
void
ay_tess_texcoorddata(GLfloat *texcoord, void *userData)
{
 ay_tess_object *to;
 double *nexttd, *t;

  to = (ay_tess_object *)userData;

  nexttd = *to->nexttd;

  if(to->tesselate_polymesh)
    {
      t = (double*)texcoord;
      nexttd[0] = t[0];
      nexttd[1] = t[1];
    }
  else
    {
      nexttd[0] = (double)texcoord[0];
      nexttd[1] = (double)texcoord[1];
    }

  if((*(to->nexttd)) == to->t1)
    {
      to->nexttd = &(to->t2);
    }
  else
  if((*(to->nexttd)) == to->t2)
    {
      to->nexttd = &(to->t3);
    }
  else
  if((*(to->nexttd)) == to->t3)
    {
      to->nexttd = &(to->t4);
    }
  else
  if((*(to->nexttd)) == to->t4)
    {
      to->nexttd = &(to->t1);
    }

 return;
} /* ay_tess_texcoorddata */


/* ay_tess_enddata:
 *  tesselation callback
 *
 */
void
ay_tess_enddata(void *userData)
{
 ay_tess_object *to;

  to = (ay_tess_object *)userData;

  to->nextpd = &(to->p1);
  to->nextnd = &(to->n1);
  to->nextcd = &(to->c1);
  to->nexttd = &(to->t1);

 return;
} /* ay_tess_enddata */


/* ay_tess_combinedata:
 *  tesselation callback (XXXX only in use when tesselating PolyMeshes?)
 *
 */
void
ay_tess_combinedata(GLdouble c[3], void *d[4], GLfloat w[4], void **out,
		    void *userData)
{
 GLdouble *nv = NULL;
 ay_tess_object *to;
 int stride, i;

  to = (ay_tess_object *)userData;

  stride = 3;

  if(to->has_vn)
    stride += 3;

  if(to->has_vc)
    stride += 4;

  if(to->has_tc)
    stride += 2;

  if(!(nv = (GLdouble *) malloc(sizeof(GLdouble)*stride)))
    return;

  nv[0] = c[0];
  nv[1] = c[1];
  nv[2] = c[2];

  i = 3;

  if(to->has_vn)
    {
      nv[i] = w[0]*((double*)d[0])[i] + w[1]*((double*)d[1])[i] +
	w[2]*((double*)d[2])[i] + w[3]*((double*)d[3])[i];

      nv[i+1] = w[0]*((double*)d[0])[i+1] + w[1]*((double*)d[1])[i+1] +
	w[2]*((double*)d[2])[i+1] + w[3]*((double*)d[3])[i+1];

      nv[i+2] = w[0]*((double*)d[0])[i+2] + w[1]*((double*)d[1])[i+2] +
	w[2]*((double*)d[2])[i+2] + w[3]*((double*)d[3])[i+2];
      i += 3;
    }

  if(to->has_vc)
    {
      nv[i] = w[0]*((double*)d[0])[i] + w[1]*((double*)d[1])[i] +
	w[2]*((double*)d[2])[i] + w[3]*((double*)d[3])[i];

      nv[i+1] = w[0]*((double*)d[0])[i+1] + w[1]*((double*)d[1])[i+1] +
	w[2]*((double*)d[2])[i+1] + w[3]*((double*)d[3])[i+1];

      nv[i+2] = w[0]*((double*)d[0])[i+2] + w[1]*((double*)d[1])[i+2] +
	w[2]*((double*)d[2])[i+2] + w[3]*((double*)d[3])[i+2];

      nv[i+3] = w[0]*((double*)d[0])[i+3] + w[1]*((double*)d[1])[i+3] +
	w[2]*((double*)d[2])[i+3] + w[3]*((double*)d[3])[i+3];
      i += 4;
    }

  if(to->has_tc)
    {
      nv[i] = w[0]*((double*)d[0])[i] + w[1]*((double*)d[1])[i] +
	w[2]*((double*)d[2])[i] + w[3]*((double*)d[3])[i];

      nv[i+1] = w[0]*((double*)d[0])[i+1] + w[1]*((double*)d[1])[i+1] +
	w[2]*((double*)d[2])[i+1] + w[3]*((double*)d[3])[i+1];
    }

  /* remember pointer to free it later */
  ay_tess_managecombined((void*)nv);

  *out = nv;

 return;
} /* ay_tess_combinedata */


/* ay_tess_managecombined:
 *  manage data created by the combinedata tesselation callback
 *  if <data> is NULL, all data remembered is cleared
 */
void
ay_tess_managecombined(void *data)
{
 static ay_tess_listobject *list = NULL;
 ay_tess_listobject *new = NULL;

  if(data)
    { /* add new pointer to the list */
      if(!(new = malloc(sizeof(ay_tess_listobject))))
	return;

      new->data = data;
      new->next = list;
      list = new;
    }
  else
    { /* data == NULL => free the list */
      while(list)
	{
	  new = list;
	  list = new->next;
	  free(new->data);
	  free(new);
	} /* while */
    } /* if */

 return;
} /* ay_tess_managecombined */


/** ay_tess_addtag:
 * Add a PV tag for texture coordinates or vertex colors to an object.
 *
 * \param[in,out] o object to manipulate
 * \param[in] val tag value string
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_tess_addtag(ay_object *o, char *val)
{
 ay_tag *tag;

  if(!o || !val)
    return AY_ENULL;

  if(!(tag = calloc(1, sizeof(ay_tag))))
    {
      return AY_EOMEM;
    }

  tag->type = ay_pv_tagtype;

  if(!(tag->name = malloc(3*sizeof(char))))
    {
      free(tag);
      return AY_EOMEM;
    }
  strcpy(tag->name, "PV");

  tag->val = val;

  tag->next = o->tags;
  o->tags = tag;

 return AY_OK;
} /* ay_tess_addtag */


/** ay_tess_tristoquad:
 * Compare two triangles, check for two common points, check normals
 * against maximum deviation, if all checks were positive: calculate
 * four indices to form a quad (three indices from triangle 1, one
 * index from triangle 2).
 *
 * \param[in] t1 array of pointers to coordinates, normals etc. of triangle 1
 * \param[in] t2 array of pointers to coordinates, normals etc. of triangle 2
 * \param[in] quad_eps maximum angle (in degrees) the normals of t1 and t2 may
 *  differentiate in order to be considered compatible
 * \param[in,out] q array where the four quad indices are stored (may be
 *  NULL, in which case only the compatibility check(s) will be done)
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_tess_tristoquad(double **t1, double **t2, double quad_eps, int *q)
{
 int i, j, have_point1 = AY_FALSE, have_point2 = AY_FALSE;
 int cp1t1 = 0, cp1t2 = 0, cp2t1 = 0, cp2t2 = 0, t;
 double angle = 0.0, eps = DBL_MAX, N1[3] = {0}, N2[3] = {0};

  /* find two common points */
  for(i = 0; i < 3; i++)
    {
      for(j = 0; j < 3; j++)
	{
	  if(have_point1)
	    {
	      if(AY_V3COMP(t1[i],t2[j]))
		{
		  cp2t1 = i;
		  cp2t2 = j;
		  have_point2 = AY_TRUE;
		  break;
		}
	    }
	  else
	    {
	      if(AY_V3COMP(t1[i],t2[j]))
		{
		  cp1t1 = i;
		  cp1t2 = j;
		  have_point1 = AY_TRUE;
		}
	    }
	} /* for */
    } /* for */

  if(!(have_point1 && have_point2))
    return AY_ERROR;

  if(quad_eps != DBL_MAX)
    {
      /* employ flatness check */
      ay_geom_normalfrom3pnts(t1[0], t1[1], t1[2], N1);
      ay_geom_normalfrom3pnts(t2[0], t2[1], t2[2], N2);

      if(!AY_V3COMP(N1,N2))
	{
	  angle = AY_R2D(acos(AY_V3DOT(N1,N2)));
	  /* XXXX split? */
	  if(fabs(angle) > quad_eps)
	    {
	      return AY_ERROR;
	    }
	}
    }

  if(!q)
    return AY_OK;

  if(fabs(angle) < eps)
    {
      /* emit one quad */
      t = 0;
      if(t == cp1t1 || t == cp2t1)
	{
	  t++;
	  if(t == cp1t1 || t == cp2t1)
	    {
	      t++;
	    }
	}
      q[1] = t;
      q[2] = q[1]+1;
      if(q[2] > 2)
	q[2] = 0;
      q[0] = q[1]-1;
      if(q[0] < 0)
	q[0] = 2;

      if(cp1t1 > cp2t1)
	{
	  t = q[0];
	  q[0] = q[2];
	  q[2] = t;
	}

      t = 0;
      if(t == cp1t2 || t == cp2t2)
	{
	  t++;
	  if(t == cp1t2 || t == cp2t2)
	    {
	      t++;
	    }
	}
      q[3] = t;
    }
  else
    {
      /* split along edge into two quads */
      q[0] = -1;
    } /* if */

 return AY_OK;
} /* ay_tess_tristoquad */


/* ay_tess_tristoquadpomesh:
 *  convert a bunch of tesselated triangles to a PolyMesh object
 *  containing only quads (albeit some of them may be degenerate)
 *  according to <has_vc> and <has_tc> PV tags will be created that
 *  carry the vertex colors and texture coordinates respectively
 */
int
ay_tess_tristoquadpomesh(ay_tess_tri *tris, int has_vn, int has_vc, int has_tc,
			 char *myst, char *myvc, double quad_eps,
			 ay_object **result)
{
 int ay_status = AY_OK;
 ay_object *new = NULL;
 ay_pomesh_object *po = NULL;
 ay_tess_tri *tri, *tri1, *tri2, **dquads = NULL;
 double *pt1[12] = {0}, *pt2[12] = {0}, mid[11] = {0};
 unsigned int numtris = 0, numquads = 0, numdquads = 0, i, j, k, stride;
 unsigned int trik = 0;
 int q[4] = {0};
 char *tctagbuf = NULL, *vctagbuf = NULL;
 char *tctagptr = NULL, *vctagptr = NULL;

  if(!tris || !result || (has_tc && !myst) || (has_vc && !myvc))
    return AY_ENULL;

  stride = 3;

  if(has_vn)
    stride += 3;

  /* XXXX not yet, PolyMeshes currently only support vertices and normals
   * colors and texcoords must go as tags
   */
  /*
  if(has_vc)
    stride += 4;

  if(has_tc)
    stride += 2;
  */

  /* first, count the triangles */
  tri = tris;
  while(tri)
    {
      numtris++;
      tri = tri->next;
    }

  /* create new object (the PolyMesh) */
  if(!(new = calloc(1, sizeof(ay_object))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  ay_object_defaults(new);
  new->type = AY_IDPOMESH;
  if(!(new->refine = calloc(1, sizeof(ay_pomesh_object))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  po = (ay_pomesh_object*)new->refine;

  numquads = numtris;
  po->has_normals = has_vn;

  if(!(dquads = malloc(numquads*sizeof(ay_tess_tri*))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  /* now copy all the vertices and normals */
  if(!(po->controlv = malloc((numquads*4+numquads/2)*stride*sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  if(has_tc)
    {
      if(!(tctagbuf = malloc((strlen(myst) + 64 +
			     ((numquads*4+numquads/2)*2*(TCL_DOUBLE_SPACE+1)))*
			     sizeof(char))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}
      sprintf(tctagbuf, "%s,varying,g,%u", myst, numquads*4);
      tctagptr = tctagbuf+strlen(tctagbuf);
    }

  if(has_vc)
    {
      if(!(vctagbuf = malloc((strlen(myvc) + 64 +
			     ((numquads*4+numquads/2)*3*(TCL_DOUBLE_SPACE+1)))*
			     sizeof(char))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}
      sprintf(vctagbuf, "%s,varying,c,%u", myvc, numquads*4);
      vctagptr = vctagbuf+strlen(vctagbuf);
    }

  numquads = 0;
  i = 0;
  tri1 = tris;
  tri2 = tri1->next;
  while(tri1)
    {
      if(!tri2)
	{
	  ay_status = AY_ERROR;
	}
      else
	{
	  pt1[0] = tri1->p1;
	  pt1[1] = tri1->p2;
	  pt1[2] = tri1->p3;

	  pt2[0] = tri2->p1;
	  pt2[1] = tri2->p2;
	  pt2[2] = tri2->p3;

	  if(has_vn)
	    {
	      pt1[3] = tri1->n1;
	      pt1[4] = tri1->n2;
	      pt1[5] = tri1->n3;

	      pt2[3] = tri2->n1;
	      pt2[4] = tri2->n2;
	      pt2[5] = tri2->n3;
	    }

	  if(has_tc)
	    {
	      pt1[6] = tri1->t1;
	      pt1[7] = tri1->t2;
	      pt1[8] = tri1->t3;

	      pt2[6] = tri2->t1;
	      pt2[7] = tri2->t2;
	      pt2[8] = tri2->t3;
	    }

	  if(has_vc)
	    {
	      pt1[9] = tri1->c1;
	      pt1[10] = tri1->c2;
	      pt1[11] = tri1->c3;

	      pt2[9] = tri2->c1;
	      pt2[10] = tri2->c2;
	      pt2[11] = tri2->c3;
	    }

	  ay_status = ay_tess_tristoquad(pt1, pt2, quad_eps, q);
	}

      if(ay_status)
	{
	  /* triangles are incompatible => create degenerate quad from tri1 */
	  dquads[numdquads] = tri1;
	  numdquads++;
#if 0
	  if(has_vn)
	    {
	      memcpy(&(po->controlv[i]), tri1->p1, 3*sizeof(double));
	      memcpy(&(po->controlv[i+3]), tri1->n1, 3*sizeof(double));
	      memcpy(&(po->controlv[i+6]), tri1->p2, 3*sizeof(double));
	      memcpy(&(po->controlv[i+9]), tri1->n2, 3*sizeof(double));
	      memcpy(&(po->controlv[i+12]), tri1->p3, 3*sizeof(double));
	      memcpy(&(po->controlv[i+15]), tri1->n3, 3*sizeof(double));
	      memcpy(&(po->controlv[i+18]), tri1->p3, 3*sizeof(double));
	      memcpy(&(po->controlv[i+21]), tri1->n3, 3*sizeof(double));
	    }
	  else
	    {
	      memcpy(&(po->controlv[i]), tri1->p1, 3*sizeof(double));
	      memcpy(&(po->controlv[i+3]), tri1->p2, 3*sizeof(double));
	      memcpy(&(po->controlv[i+6]), tri1->p3, 3*sizeof(double));
	      memcpy(&(po->controlv[i+9]), tri1->p3, 3*sizeof(double));
	    }

	  if(has_tc)
	    {
	      tctagptr += sprintf(tctagptr,
				  ",%g,%g,%g,%g,%g,%g,%g,%g",
				  tri1->t1[0], tri1->t1[1],
				  tri1->t2[0], tri1->t2[1],
				  tri1->t3[0], tri1->t3[1],
				  tri1->t3[0], tri1->t3[1]);
	    }

	  if(has_vc)
	    {
	      vctagptr += sprintf(vctagptr,
				  ",%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g",
				  tri1->c1[0], tri1->c1[1], tri1->c1[2],
				  tri1->c2[0], tri1->c2[1], tri1->c2[2],
				  tri1->c3[0], tri1->c3[1], tri1->c3[2],
				  tri1->c3[0], tri1->c3[1], tri1->c3[2]);
	    }

	  i += (4*stride);
#endif
	  tri1 = tri1->next;
	  if(tri2)
	    tri2 = tri2->next;
	  continue;
	}

      if(q[0] != -1)
	{
	  /* one quad => just copy the relevant data */
	  numquads++;
	  if(has_vn)
	    {
	      memcpy(&(po->controlv[i]), pt1[q[0]], 3*sizeof(double));
	      memcpy(&(po->controlv[i+3]), pt1[q[0]+3], 3*sizeof(double));
	      memcpy(&(po->controlv[i+6]), pt1[q[1]], 3*sizeof(double));
	      memcpy(&(po->controlv[i+9]), pt1[q[1]+3], 3*sizeof(double));
	      memcpy(&(po->controlv[i+12]), pt1[q[2]], 3*sizeof(double));
	      memcpy(&(po->controlv[i+15]), pt1[q[2]+3], 3*sizeof(double));
	      memcpy(&(po->controlv[i+18]), pt2[q[3]], 3*sizeof(double));
	      memcpy(&(po->controlv[i+21]), pt2[q[3]+3], 3*sizeof(double));
	    }
	  else
	    {
	      memcpy(&(po->controlv[i]), pt1[q[0]], 3*sizeof(double));
	      memcpy(&(po->controlv[i+3]), pt1[q[1]], 3*sizeof(double));
	      memcpy(&(po->controlv[i+6]), pt1[q[2]], 3*sizeof(double));
	      memcpy(&(po->controlv[i+9]), pt2[q[3]], 3*sizeof(double));
	    }

	  if(has_tc)
	    {
	      tctagptr += sprintf(tctagptr,
				  ",%g,%g,%g,%g,%g,%g,%g,%g",
				  *(pt1[q[0]+6]), *(pt1[q[0]+6]+1),
				  *(pt1[q[1]+6]), *(pt1[q[1]+6]+1),
				  *(pt1[q[2]+6]), *(pt1[q[2]+6]+1),
				  *(pt2[q[3]+6]), *(pt2[q[3]+6]+1));
	    }

	  if(has_vc)
	    {
	      vctagptr += sprintf(vctagptr,
				  ",%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g",
		      *(pt1[q[0]+9]), *(pt1[q[0]+9]+1), *(pt1[q[0]+9]+2),
		      *(pt1[q[1]+9]), *(pt1[q[1]+9]+1), *(pt1[q[1]+9]+2),
		      *(pt1[q[2]+9]), *(pt1[q[2]+9]+1), *(pt1[q[2]+9]+2),
		      *(pt2[q[3]+9]), *(pt2[q[3]+9]+1), *(pt2[q[3]+9]+2));
	    }

	  i += (4*stride);
	}
      else
	{
	  /* emit two quads */

	}

      tri1 = tri2->next;
      if(tri1)
	tri2 = tri1->next;
    } /* while */

  /* search degenerate quads for potential partners */
  for(j = 0; j < numdquads; j++)
    {
      tri1 = dquads[j];
      if(tri1)
	{
	  pt1[0] = tri1->p1;
	  pt1[1] = tri1->p2;
	  pt1[2] = tri1->p3;

	  if(has_vn)
	    {
	      pt1[3] = tri1->n1;
	      pt1[4] = tri1->n2;
	      pt1[5] = tri1->n3;
	    }

	  if(has_tc)
	    {
	      pt1[6] = tri1->t1;
	      pt1[7] = tri1->t2;
	      pt1[8] = tri1->t3;
	    }

	  if(has_vc)
	    {
	      pt1[9] = tri1->c1;
	      pt1[10] = tri1->c2;
	      pt1[11] = tri1->c3;
	    }

	  tri = NULL;

	  for(k = j+1; k < numdquads; k++)
	    {
	      tri2 = dquads[k];
	      if(tri2)
		{
		  pt2[0] = tri2->p1;
		  pt2[1] = tri2->p2;
		  pt2[2] = tri2->p3;

		  if(has_vn)
		    {
		      pt2[3] = tri2->n1;
		      pt2[4] = tri2->n2;
		      pt2[5] = tri2->n3;
		    }

		  if(has_tc)
		    {
		      pt2[6] = tri2->t1;
		      pt2[7] = tri2->t2;
		      pt2[8] = tri2->t3;
		    }

		  if(has_vc)
		    {
		      pt2[9] = tri2->c1;
		      pt2[10] = tri2->c2;
		      pt2[11] = tri2->c3;
		    }

		  ay_status = ay_tess_tristoquad(pt1, pt2, quad_eps, q);

		  if(!ay_status && q[0] != -1)
		    {
		      /* one quad => just copy the relevant data */
		      if(ay_tess_checkquad(pt1[q[0]], pt1[q[1]],
					   pt1[q[2]], pt2[q[3]],
					   /*check_convexity=*/AY_TRUE))
			{
			  numquads++;
			  if(has_vn)
			    {
			      memcpy(&(po->controlv[i]), pt1[q[0]],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+3]), pt1[q[0]+3],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+6]), pt1[q[1]],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+9]), pt1[q[1]+3],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+12]), pt1[q[2]],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+15]), pt1[q[2]+3],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+18]), pt2[q[3]],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+21]), pt2[q[3]+3],
				     3*sizeof(double));
			    }
			  else
			    {
			      memcpy(&(po->controlv[i]), pt1[q[0]],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+3]), pt1[q[1]],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+6]), pt1[q[2]],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+9]), pt2[q[3]],
				     3*sizeof(double));
			    }

			  if(has_tc)
			    {
			      tctagptr += sprintf(tctagptr,
					      ",%g,%g,%g,%g,%g,%g,%g,%g",
					      *(pt1[q[0]+6]), *(pt1[q[0]+6]+1),
					      *(pt1[q[1]+6]), *(pt1[q[1]+6]+1),
					      *(pt1[q[2]+6]), *(pt1[q[2]+6]+1),
					      *(pt2[q[3]+6]), *(pt2[q[3]+6]+1));
			    }

			  if(has_vc)
			    {
			      vctagptr += sprintf(vctagptr,
				      ",%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g",
			  *(pt1[q[0]+9]), *(pt1[q[0]+9]+1), *(pt1[q[0]+9]+2),
			  *(pt1[q[1]+9]), *(pt1[q[1]+9]+1), *(pt1[q[1]+9]+2),
			  *(pt1[q[2]+9]), *(pt1[q[2]+9]+1), *(pt1[q[2]+9]+2),
			  *(pt2[q[3]+9]), *(pt2[q[3]+9]+1), *(pt2[q[3]+9]+2));
			    }

			  i += (4*stride);

			  /* signal successful processing of tri1/tri2 */
			  dquads[j] = NULL;
			  dquads[k] = NULL;
			  break;
			} /* if quad is ok */
		    } /* if tri1/tri2 form a quad */
		} /* if tri2 */
	    } /* for k */
	} /* if tri1 */
    } /* for j */

  /* search degenerate quads for potential partners */
  for(j = 0; j < numdquads; j++)
    {
      tri1 = dquads[j];
      if(tri1)
	{
	  pt1[0] = tri1->p1;
	  pt1[1] = tri1->p2;
	  pt1[2] = tri1->p3;

	  if(has_vn)
	    {
	      pt1[3] = tri1->n1;
	      pt1[4] = tri1->n2;
	      pt1[5] = tri1->n3;
	    }

	  if(has_tc)
	    {
	      pt1[6] = tri1->t1;
	      pt1[7] = tri1->t2;
	      pt1[8] = tri1->t3;
	    }

	  if(has_vc)
	    {
	      pt1[9] = tri1->c1;
	      pt1[10] = tri1->c2;
	      pt1[11] = tri1->c3;
	    }

	  tri = NULL;

	  for(k = j+1; k < numdquads; k++)
	    {
	      tri2 = dquads[k];
	      if(tri2)
		{
		  pt2[0] = tri2->p1;
		  pt2[1] = tri2->p2;
		  pt2[2] = tri2->p3;

		  if(has_vn)
		    {
		      pt2[3] = tri2->n1;
		      pt2[4] = tri2->n2;
		      pt2[5] = tri2->n3;
		    }

		  if(has_tc)
		    {
		      pt2[6] = tri2->t1;
		      pt2[7] = tri2->t2;
		      pt2[8] = tri2->t3;
		    }

		  if(has_vc)
		    {
		      pt2[9] = tri2->c1;
		      pt2[10] = tri2->c2;
		      pt2[11] = tri2->c3;
		    }

		  ay_status = ay_tess_tristoquad(pt1, pt2, quad_eps, q);

		  if(!ay_status && q[0] != -1)
		    {
		      /* one quad => just copy the relevant data */
		      if(ay_tess_checkquad(pt1[q[0]], pt1[q[1]],
					   pt1[q[2]], pt2[q[3]],
					   /*check_convexity=*/AY_TRUE))
			{
			  numquads++;
			  if(has_vn)
			    {
			      memcpy(&(po->controlv[i]), pt1[q[0]],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+3]), pt1[q[0]+3],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+6]), pt1[q[1]],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+9]), pt1[q[1]+3],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+12]), pt1[q[2]],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+15]), pt1[q[2]+3],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+18]), pt2[q[3]],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+21]), pt2[q[3]+3],
				     3*sizeof(double));
			    }
			  else
			    {
			      memcpy(&(po->controlv[i]), pt1[q[0]],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+3]), pt1[q[1]],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+6]), pt1[q[2]],
				     3*sizeof(double));
			      memcpy(&(po->controlv[i+9]), pt2[q[3]],
				     3*sizeof(double));
			    }

			  if(has_tc)
			    {
			      tctagptr += sprintf(tctagptr,
					      ",%g,%g,%g,%g,%g,%g,%g,%g",
					      *(pt1[q[0]+6]), *(pt1[q[0]+6]+1),
					      *(pt1[q[1]+6]), *(pt1[q[1]+6]+1),
					      *(pt1[q[2]+6]), *(pt1[q[2]+6]+1),
					      *(pt2[q[3]+6]), *(pt2[q[3]+6]+1));
			    }

			  if(has_vc)
			    {
			      vctagptr += sprintf(vctagptr,
				      ",%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g",
			  *(pt1[q[0]+9]), *(pt1[q[0]+9]+1), *(pt1[q[0]+9]+2),
			  *(pt1[q[1]+9]), *(pt1[q[1]+9]+1), *(pt1[q[1]+9]+2),
			  *(pt1[q[2]+9]), *(pt1[q[2]+9]+1), *(pt1[q[2]+9]+2),
			  *(pt2[q[3]+9]), *(pt2[q[3]+9]+1), *(pt2[q[3]+9]+2));
			    }

			  i += (4*stride);

			  /* signal successful processing of tri1/tri2 */
			  dquads[j] = NULL;
			  dquads[k] = NULL;

			  break;
			} /* if quad is ok */
		    }
		  else
		    {
		      if(!tri)
			{
			  /* XXXX this simply picks the first compatible
			     (edge sharing) triangle; but maybe it would
			     be better to search for a compatible triangle
			     with a longer sharing edge? */

			  ay_status = ay_tess_tristoquad(pt1, pt2, DBL_MAX, q);
			  if(!ay_status)
			    {
			      trik = k;
			      tri = tri2;
			    }
			}
		  } /* one quad */
		} /* if tri2 */
	    } /* for k */

	  if(dquads[j])
	    {
	      if(tri)
		{
		  /* found edge sharing triangle => create two quads
		     by inserting a new vertex in the middle of the
		     sharing edge */

		  pt2[0] = tri->p1;
		  pt2[1] = tri->p2;
		  pt2[2] = tri->p3;

		  if(has_vn)
		    {
		      pt2[3] = tri->n1;
		      pt2[4] = tri->n2;
		      pt2[5] = tri->n3;
		    }

		  if(has_tc)
		    {
		      pt2[6] = tri->t1;
		      pt2[7] = tri->t2;
		      pt2[8] = tri->t3;
		    }

		  if(has_vc)
		    {
		      pt2[9] = tri->c1;
		      pt2[10] = tri->c2;
		      pt2[11] = tri->c3;
		    }

		  ay_tess_tristoquad(pt1, pt2, DBL_MAX, q);

		  /* sharing edge is q[0]-q[2] from t1 */

		  mid[0] = *(pt1[q[0]]) +
		    (*(pt1[q[2]])   - *(pt1[q[0]]))*0.5;
		  mid[1] = *(pt1[q[0]]+1) +
		    (*(pt1[q[2]]+1) - *(pt1[q[0]]+1))*0.5;
		  mid[2] = *(pt1[q[0]]+2) +
		    (*(pt1[q[2]]+2) - *(pt1[q[0]]+2))*0.5;

		  if(has_vn)
		    {
		      mid[3] = *(pt1[q[0]+3]) +
			(*(pt1[q[2]+3])   - *(pt1[q[0]+3]))*0.5;
		      mid[4] = *(pt1[q[0]+3]+1) +
			(*(pt1[q[2]+3]+1) - *(pt1[q[0]+3]+1))*0.5;
		      mid[5] = *(pt1[q[0]+3]+2) +
			(*(pt1[q[2]+3]+2) - *(pt1[q[0]+3]+2))*0.5;
		    }

		  if(has_tc)
		    {
		      mid[6] = *(pt1[q[0]+6]) +
			(*(pt1[q[2]+6]) - *(pt1[q[0]+6]))*0.5;
		      mid[7] = *(pt1[q[0]+6]+1) +
			(*(pt1[q[2]+6]+1) - *(pt1[q[0]+6]+1))*0.5;
		    }

		  if(has_vc)
		    {
		      mid[8] = *(pt1[q[0]+9]) +
			(*(pt1[q[2]+9]) - *(pt1[q[0]+9]))*0.5;
		      mid[9] = *(pt1[q[0]+9]+1) +
			(*(pt1[q[2]+9]+1) - *(pt1[q[0]+9]+1))*0.5;
		      mid[10] = *(pt1[q[0]+9]+2) +
			(*(pt1[q[2]+9]+2) - *(pt1[q[0]+9]+2))*0.5;
		    }

		  if(ay_tess_checkquad(pt1[q[0]], pt1[q[1]], pt1[q[2]], mid,
				       /*check_convexity=*/AY_FALSE))
		    {
		      numquads++;
		      /* Q1 */
		      if(has_vn)
			{
			  memcpy(&(po->controlv[i]), pt1[q[0]],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+3]), pt1[q[0]+3],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+6]), pt1[q[1]],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+9]), pt1[q[1]+3],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+12]), pt1[q[2]],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+15]), pt1[q[2]+3],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+18]), mid,
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+21]), &(mid[3]),
				 3*sizeof(double));
			}
		      else
			{
			  memcpy(&(po->controlv[i]), pt1[q[0]],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+3]), pt1[q[1]],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+6]), pt1[q[2]],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+9]), mid,
				 3*sizeof(double));
			}

		      if(has_tc)
			{
			  tctagptr += sprintf(tctagptr,
					  ",%g,%g,%g,%g,%g,%g,%g,%g",
					  *(pt1[q[0]+6]), *(pt1[q[0]+6]+1),
					  *(pt1[q[1]+6]), *(pt1[q[1]+6]+1),
					  *(pt1[q[2]+6]), *(pt1[q[2]+6]+1),
					  mid[6], mid[7]);
			}

		      if(has_vc)
			{
			  vctagptr += sprintf(vctagptr,
				       ",%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g",
			   *(pt1[q[0]+9]), *(pt1[q[0]+9]+1), *(pt1[q[0]+9]+2),
			   *(pt1[q[1]+9]), *(pt1[q[1]+9]+1), *(pt1[q[1]+9]+2),
			   *(pt1[q[2]+9]), *(pt1[q[2]+9]+1), *(pt1[q[2]+9]+2),
					  mid[8], mid[9], mid[10]);
			}

		      i += 4*stride;
		    } /* if quad is ok */

		  if(ay_tess_checkquad(pt1[q[2]], pt2[q[3]], pt1[q[0]], mid,
				       /*check_convexity=*/AY_FALSE))
		    {
		      numquads++;
		      /* Q2 */
		      if(has_vn)
			{
			  memcpy(&(po->controlv[i]), pt1[q[2]],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+3]), pt1[q[2]+3],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+6]), pt2[q[3]],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+9]), pt2[q[3]+3],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+12]), pt1[q[0]],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+15]), pt1[q[0]+3],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+18]), mid,
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+21]), &(mid[3]),
				 3*sizeof(double));
			}
		      else
			{
			  memcpy(&(po->controlv[i]), pt1[q[2]],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+3]), pt2[q[3]],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+6]), pt1[q[0]],
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+9]), mid,
				 3*sizeof(double));
			}

		      if(has_tc)
			{
			  tctagptr += sprintf(tctagptr,
					  ",%g,%g,%g,%g,%g,%g,%g,%g",
					  *(pt1[q[2]+6]), *(pt1[q[2]+6]+1),
					  *(pt2[q[3]+6]), *(pt2[q[3]+6]+1),
					  *(pt1[q[0]+6]), *(pt1[q[0]+6]+1),
					  mid[6], mid[7]);
			}

		      if(has_vc)
			{
			  vctagptr += sprintf(vctagptr,
				       ",%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g",
			   *(pt1[q[2]+9]), *(pt1[q[2]+9]+1), *(pt1[q[2]+9]+2),
			   *(pt2[q[3]+9]), *(pt2[q[3]+9]+1), *(pt2[q[3]+9]+2),
			   *(pt1[q[0]+9]), *(pt1[q[0]+9]+1), *(pt1[q[0]+9]+2),
			   mid[8], mid[9], mid[10]);
			}

		      i += 4*stride;
		    }

		  /* signal successful processing of tri1/tri */
		  dquads[j] = NULL;
		  dquads[trik] = NULL;
		}
	      else
		{
		  /* found no matching triangle
		     => create degenerate quad from tri1 */
		  if(ay_tess_checkquad(tri1->p1, tri1->p2, tri1->p3, tri1->p3,
				       /*check_convexity=*/AY_FALSE))
		    {
		      numquads++;

		      if(has_vn)
			{
			  memcpy(&(po->controlv[i]), tri1->p1,
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+3]), tri1->n1,
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+6]), tri1->p2,
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+9]), tri1->n2,
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+12]), tri1->p3,
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+15]), tri1->n3,
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+18]), tri1->p3,
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+21]), tri1->n3,
				 3*sizeof(double));
			}
		      else
			{
			  memcpy(&(po->controlv[i]), tri1->p1,
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+3]), tri1->p2,
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+6]), tri1->p3,
				 3*sizeof(double));
			  memcpy(&(po->controlv[i+9]), tri1->p3,
				 3*sizeof(double));
			}

		      if(has_tc)
			{
			  tctagptr += sprintf(tctagptr,
					      ",%g,%g,%g,%g,%g,%g,%g,%g",
					      tri1->t1[0], tri1->t1[1],
					      tri1->t2[0], tri1->t2[1],
					      tri1->t3[0], tri1->t3[1],
					      tri1->t3[0], tri1->t3[1]);
			}

		      if(has_vc)
			{
			  vctagptr += sprintf(vctagptr,
				     ",%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g",
				      tri1->c1[0], tri1->c1[1], tri1->c1[2],
				      tri1->c2[0], tri1->c2[1], tri1->c2[2],
				      tri1->c3[0], tri1->c3[1], tri1->c3[2],
				      tri1->c3[0], tri1->c3[1], tri1->c3[2]);
			}

		      i += (4*stride);
		    } /* if quad is ok */
		} /* if found no match */
	    } /* if degen */
	} /* if tri1 */
    } /* for j */

  ay_status = AY_OK;
  po->ncontrols = numquads*4;
  po->npolys = numquads;

  /* XXXX what happens, when numtris/numcontrolv gets too big for
     an unsigned int? */

  /* create index structures */
  if(!(po->nloops = malloc(numquads*sizeof(unsigned int))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  for(i = 0; i < numquads; i++)
    {
      /* each polygon has just one loop */
      po->nloops[i] = 1;
    } /* for */

  if(!(po->nverts = malloc(numquads*sizeof(unsigned int))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  for(i = 0; i < numquads; i++)
    {
      /* each loop has just three vertices (is a triangle) */
      po->nverts[i] = 4;
    } /* for */

  if(!(po->verts = malloc(numquads*4*sizeof(unsigned int))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  for(i = 0; i < numquads*4; i++)
    {
      /* vertex indices are simply ordered (user may remove multiply used
	 vertices using PolyMesh optimization later) */
      po->verts[i] = i;
    } /* for */

  if(has_tc)
    {
      ay_status = ay_tess_addtag(new, tctagbuf);
      if(ay_status)
	goto cleanup;
      tctagbuf = NULL;
    } /* if has_tc */

  if(has_vc)
    {
      ay_status = ay_tess_addtag(new, vctagbuf);
      if(ay_status)
	goto cleanup;
      vctagbuf = NULL;
    } /* if has_vc */

  *result = new;

  /* prevent cleanup code from doing something harmful */
  new = NULL;
  po = NULL;

cleanup:
  if(new)
    {
      if(new->tags)
	{
	  ay_tags_delall(new);
	}
      free(new);
    }

  if(po)
    {
      if(po->nloops)
	free(po->nloops);

      if(po->nverts)
	free(po->nverts);

      if(po->verts)
	free(po->verts);

      if(po->controlv)
	free(po->controlv);

      free(po);
    }

  if(tctagbuf)
    free(tctagbuf);

  if(vctagbuf)
    free(vctagbuf);

  if(dquads)
    free(dquads);

 return ay_status;
} /* ay_tess_tristoquadpomesh */


/* ay_tess_tristomixedpomesh:
 *  convert a bunch of tesselated triangles to a PolyMesh object
 *  with triangles and possibly quads
 *  according to <has_vc> and <has_tc> PV tags will be created that
 *  carry the vertex colors and texture coordinates respectively
 */
int
ay_tess_tristomixedpomesh(ay_tess_tri *tris, int has_vn, int has_vc, int has_tc,
			  char *myst, char *myvc, double quad_eps,
			  ay_object **result)
{
 int ay_status = AY_OK;
 ay_object *new = NULL;
 ay_pomesh_object *po = NULL;
 ay_tess_tri *tri, *tri1, *tri2;
 double *pt1[12] = {0}, *pt2[12] = {0};
 unsigned int numtris = 0, numquads = 0, i = 0, stride;
 int q[4] = {0};
 char *isquad = NULL;
 char *tctagbuf = NULL, *vctagbuf = NULL;
 char *tctagptr = NULL, *vctagptr = NULL;

  if(!tris || !result || (has_tc && !myst) || (has_vc && !myvc))
    return AY_ENULL;

  stride = 3;

  if(has_vn)
    stride += 3;

  /* XXXX not yet, PolyMeshes currently only support vertices and normals
   * colors and texcoords must go as tags
   */
  /*
  if(has_vc)
    stride += 4;

  if(has_tc)
    stride += 2;
  */

  tri = tris;
  while(tri)
    {
      numtris++;
      tri = tri->next;
    }

  if(!(isquad = malloc((numtris)*sizeof(char))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  numtris = 0;
  /* first, count the quads and triangles */
  tri = tris;
  while(tri)
    {
      if(tri->is_quad && tri->next && tri->next->is_quad)
	{
	  tri1 = tri;
	  tri2 = tri->next;

	  pt1[0] = tri1->p1;
	  pt1[1] = tri1->p2;
	  pt1[2] = tri1->p3;

	  pt2[0] = tri2->p1;
	  pt2[1] = tri2->p2;
	  pt2[2] = tri2->p3;

	  ay_status = ay_tess_tristoquad(pt1, pt2, quad_eps, NULL);
	  if(ay_status)
	    {
	      numtris++;
	      isquad[i] = 0;
	      i++;
	      tri = tri->next;
	      continue;
	    }

	  numquads++;
	  isquad[i] = 1;
	  i++;

	  tri = tri->next;

	  /* */
	  if(!tri)
	    break;
	}
      else
	{
	  isquad[i] = 0;
	  i++;
	  numtris++;
	}
      tri = tri->next;
    } /* while tri */

  /* create new object (the PolyMesh) */
  if(!(new = calloc(1, sizeof(ay_object))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  ay_object_defaults(new);
  new->type = AY_IDPOMESH;
  if(!(new->refine = calloc(1, sizeof(ay_pomesh_object))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  po = (ay_pomesh_object*)new->refine;

  po->npolys = numquads+numtris;

  /* XXXX what happens, when numpolys/numcontrolv gets too big for
     an unsigned int? */

  /* create index structures */
  if(!(po->nloops = malloc((numquads+numtris)*sizeof(unsigned int))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  for(i = 0; i < numquads+numtris; i++)
    {
      /* each polygon has just one loop */
      po->nloops[i] = 1;
    } /* for */

  if(!(po->nverts = malloc((numquads+numtris)*sizeof(unsigned int))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  for(i = 0; i < numquads+numtris; i++)
    {
      /* each loop has just three/four vertices (depending on
	 whether it is a tri or a quad) */
      if(isquad[i])
	{
	  po->nverts[i] = 4;
	}
      else
	{
	  po->nverts[i] = 3;
	}
    } /* for */

  free(isquad);
  isquad = NULL;

  if(!(po->verts = malloc((numquads*4+numtris*3)*sizeof(unsigned int))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  for(i = 0; i < numquads*4+numtris*3; i++)
    {
      /* vertex indices are simply ordered (user may remove multiply used
	 vertices using PolyMesh optimization later) */
      po->verts[i] = i;
    } /* for */

  po->ncontrols = numquads*4+numtris*3;
  po->has_normals = has_vn;

  /* now copy all the vertices and normals */
  if(!(po->controlv = malloc((numquads*4+numtris*3)*stride*sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  if(has_tc)
    {
      if(!(tctagbuf = malloc((strlen(myst) + 64 +
			      (numquads*4+numtris*3)*2*(TCL_DOUBLE_SPACE+1)) *
			     sizeof(char))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}
      sprintf(tctagbuf, "%s,varying,g,%u", myst, numquads*4+numtris*3);
      tctagptr = tctagbuf+strlen(tctagbuf);
    }

  if(has_vc)
    {
      if(!(vctagbuf = malloc((strlen(myvc) + 64 +
			      (numquads*4+numtris*3)*2*(TCL_DOUBLE_SPACE+1)) *
			     sizeof(char))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}
      sprintf(vctagbuf, "%s,varying,c,%u", myvc, numquads*4+numtris*3);
      vctagptr = vctagbuf+strlen(vctagbuf);
    }

  /* create controlv and tag data */
  i = 0;
  tri = tris;
  while(tri)
    {
      if(tri->is_quad && tri->next && tri->next->is_quad)
	{
	  tri1 = tri;
	  tri2 = tri->next;

	  pt1[0] = tri1->p1;
	  pt1[1] = tri1->p2;
	  pt1[2] = tri1->p3;

	  pt2[0] = tri2->p1;
	  pt2[1] = tri2->p2;
	  pt2[2] = tri2->p3;

	  if(has_vn)
	    {
	      pt1[3] = tri1->n1;
	      pt1[4] = tri1->n2;
	      pt1[5] = tri1->n3;

	      pt2[3] = tri2->n1;
	      pt2[4] = tri2->n2;
	      pt2[5] = tri2->n3;
	    }

	  if(has_tc)
	    {
	      pt1[6] = tri1->t1;
	      pt1[7] = tri1->t2;
	      pt1[8] = tri1->t3;

	      pt2[6] = tri2->t1;
	      pt2[7] = tri2->t2;
	      pt2[8] = tri2->t3;
	    }

	  if(has_vc)
	    {
	      pt1[9] = tri1->c1;
	      pt1[10] = tri1->c2;
	      pt1[11] = tri1->c3;

	      pt2[9] = tri2->c1;
	      pt2[10] = tri2->c2;
	      pt2[11] = tri2->c3;
	    }

	  ay_status = ay_tess_tristoquad(pt1, pt2, quad_eps, q);
	  if(ay_status)
	    {
	      /* emit tri as triangle */
	      if(has_vn)
		{
		  memcpy(&(po->controlv[i]), tri->p1, 3*sizeof(double));
		  memcpy(&(po->controlv[i+3]), tri->n1, 3*sizeof(double));
		  memcpy(&(po->controlv[i+6]), tri->p2, 3*sizeof(double));
		  memcpy(&(po->controlv[i+9]), tri->n2, 3*sizeof(double));
		  memcpy(&(po->controlv[i+12]), tri->p3, 3*sizeof(double));
		  memcpy(&(po->controlv[i+15]), tri->n3, 3*sizeof(double));
		}
	      else
		{
		  memcpy(&(po->controlv[i]), tri->p1, 3*sizeof(double));
		  memcpy(&(po->controlv[i+3]), tri->p2, 3*sizeof(double));
		  memcpy(&(po->controlv[i+6]), tri->p3, 3*sizeof(double));
		}

	      if(has_tc)
		{
		  tctagptr += sprintf(tctagptr,
				      ",%g,%g,%g,%g,%g,%g",
				      tri->t1[0], tri->t1[1],
				      tri->t2[0], tri->t2[1],
				      tri->t3[0], tri->t3[1]);
		}

	      if(has_vc)
		{
		  tctagptr += sprintf(tctagptr,
				      ",%g,%g,%g,%g,%g,%g,%g,%g,%g",
				      tri->c1[0], tri->c1[1], tri->c1[2],
				      tri->c2[0], tri->c2[1], tri->c2[2],
				      tri->c3[0], tri->c3[1], tri->c3[2]);
		}

	      i += (3*stride);

	      tri = tri->next;
	      continue;
	    }

	  /* XXXX TODO call checkquad()? */

	  if(q[0] != -1)
	    {
	      /* one quad => just copy the relevant data */
	      if(has_vn)
		{
		  memcpy(&(po->controlv[i]), pt1[q[0]], 3*sizeof(double));
		  memcpy(&(po->controlv[i+3]), pt1[q[0]+3], 3*sizeof(double));
		  memcpy(&(po->controlv[i+6]), pt1[q[1]], 3*sizeof(double));
		  memcpy(&(po->controlv[i+9]), pt1[q[1]+3], 3*sizeof(double));
		  memcpy(&(po->controlv[i+12]), pt1[q[2]], 3*sizeof(double));
		  memcpy(&(po->controlv[i+15]), pt1[q[2]+3], 3*sizeof(double));
		  memcpy(&(po->controlv[i+18]), pt2[q[3]], 3*sizeof(double));
		  memcpy(&(po->controlv[i+21]), pt2[q[3]+3], 3*sizeof(double));
		}
	      else
		{
		  memcpy(&(po->controlv[i]), pt1[q[0]], 3*sizeof(double));
		  memcpy(&(po->controlv[i+3]), pt1[q[1]], 3*sizeof(double));
		  memcpy(&(po->controlv[i+6]), pt1[q[2]], 3*sizeof(double));
		  memcpy(&(po->controlv[i+9]), pt2[q[3]], 3*sizeof(double));
		}
	      if(has_tc)
		{
		  tctagptr += sprintf(tctagptr,
				     ",%g,%g,%g,%g,%g,%g,%g,%g",
				     *(pt1[q[0]+6]), *(pt1[q[0]+6]+1),
				     *(pt1[q[1]+6]), *(pt1[q[1]+6]+1),
				     *(pt1[q[2]+6]), *(pt1[q[2]+6]+1),
				     *(pt2[q[3]+6]), *(pt2[q[3]+6]+1));
		}

	      if(has_vc)
		{
		  vctagptr += sprintf(vctagptr,
				      ",%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g",
			  *(pt1[q[0]+9]), *(pt1[q[0]+9]+1), *(pt1[q[0]+9]+2),
			  *(pt1[q[1]+9]), *(pt1[q[1]+9]+1), *(pt1[q[1]+9]+2),
			  *(pt1[q[2]+9]), *(pt1[q[2]+9]+1), *(pt1[q[2]+9]+2),
			  *(pt2[q[3]+9]), *(pt2[q[3]+9]+1), *(pt2[q[3]+9]+2));
		}
	    }

	  i += (4*stride);
	  tri = tri->next;
	}
      else
	{
	  /* emit tri as triangle */
	  if(has_vn)
	    {
	      memcpy(&(po->controlv[i]), tri->p1, 3*sizeof(double));
	      memcpy(&(po->controlv[i+3]), tri->n1, 3*sizeof(double));
	      memcpy(&(po->controlv[i+6]), tri->p2, 3*sizeof(double));
	      memcpy(&(po->controlv[i+9]), tri->n2, 3*sizeof(double));
	      memcpy(&(po->controlv[i+12]), tri->p3, 3*sizeof(double));
	      memcpy(&(po->controlv[i+15]), tri->n3, 3*sizeof(double));
	    }
	  else
	    {
	      memcpy(&(po->controlv[i]), tri->p1, 3*sizeof(double));
	      memcpy(&(po->controlv[i+3]), tri->p2, 3*sizeof(double));
	      memcpy(&(po->controlv[i+6]), tri->p3, 3*sizeof(double));
	    }

	  if(has_tc)
	    {
	      tctagptr += sprintf(tctagptr,
				  ",%g,%g,%g,%g,%g,%g",
				  tri->t1[0], tri->t1[1],
				  tri->t2[0], tri->t2[1],
				  tri->t3[0], tri->t3[1]);
	    }

	  if(has_vc)
	    {
	      tctagptr += sprintf(tctagptr,
				  ",%g,%g,%g,%g,%g,%g,%g,%g,%g",
				  tri->c1[0], tri->c1[1], tri->c1[2],
				  tri->c2[0], tri->c2[1], tri->c2[2],
				  tri->c3[0], tri->c3[1], tri->c3[2]);
	    }

	  i += (3*stride);
	} /* if is quad */

      if(tri)
	tri = tri->next;
    } /* while tri */

  if(has_tc)
    {
      ay_status = ay_tess_addtag(new, tctagbuf);
      if(ay_status)
	goto cleanup;
      tctagbuf = NULL;
    } /* if has_tc */

  if(has_vc)
    {
      ay_status = ay_tess_addtag(new, vctagbuf);
      if(ay_status)
	goto cleanup;
      vctagbuf = NULL;
    } /* if has_vc */

  *result = new;
  ay_status = AY_OK;

  /* prevent cleanup code from doing something harmful */
  new = NULL;
  po = NULL;

cleanup:

  if(isquad)
    free(isquad);

  if(new)
    {
      if(new->tags)
	{
	  ay_tags_delall(new);
	}
      free(new);
    }

  if(po)
    {
      if(po->nloops)
	free(po->nloops);

      if(po->nverts)
	free(po->nverts);

      if(po->verts)
	free(po->verts);

      if(po->controlv)
	free(po->controlv);

      free(po);
    }

  if(tctagbuf)
    free(tctagbuf);

  if(vctagbuf)
    free(vctagbuf);

 return ay_status;
} /* ay_tess_tristomixedpomesh */


/* ay_tess_tristopomesh:
 *  convert a bunch of tesselated triangles to a PolyMesh object
 *  according to <has_vc> and <has_tc> PV tags will be created that
 *  carry the vertex colors and texture coordinates respectively
 */
int
ay_tess_tristopomesh(ay_tess_tri *tris, int has_vn, int has_vc, int has_tc,
		     char *myst, char *myvc,
		     ay_object **result)
{
 int ay_status = AY_OK;
 ay_object *new = NULL;
 ay_pomesh_object *po = NULL;
 ay_tess_tri *tri;
 unsigned int numtris = 0, i, stride;
 char *tctagbuf = NULL, *vctagbuf = NULL;
 char *tctagptr = NULL, *vctagptr = NULL;

  if(!tris || !result || (has_tc && !myst) || (has_vc && !myvc))
    return AY_ENULL;

  stride = 3;

  if(has_vn)
    stride += 3;

  /* XXXX not yet, PolyMeshes currently only support vertices and normals
   * colors and texcoords must go as tags
   */
  /*
  if(has_vc)
    stride += 4;

  if(has_tc)
    stride += 2;
  */

  /* create new object (the PolyMesh) */
  if(!(new = calloc(1, sizeof(ay_object))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  ay_object_defaults(new);
  new->type = AY_IDPOMESH;
  if(!(new->refine = calloc(1, sizeof(ay_pomesh_object))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  po = (ay_pomesh_object*)new->refine;

  /* first, count the triangles */
  tri = tris;
  while(tri)
    {
      numtris++;
      tri = tri->next;
    }

  po->npolys = numtris;

  /* XXXX what happens, when numtris/numcontrolv gets too big for
     an unsigned int? */

  /* create index structures */
  if(!(po->nloops = malloc(numtris*sizeof(unsigned int))))
    {
      return AY_EOMEM;
    }
  for(i = 0; i < numtris; i++)
    {
      /* each polygon has just one loop */
      po->nloops[i] = 1;
    } /* for */

  if(!(po->nverts = malloc(numtris*sizeof(unsigned int))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  for(i = 0; i < numtris; i++)
    {
      /* each loop has just three vertices (is a triangle) */
      po->nverts[i] = 3;
    } /* for */

  if(!(po->verts = malloc(numtris*3*sizeof(unsigned int))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  for(i = 0; i < numtris*3; i++)
    {
      /* vertex indices are simply ordered (user may remove multiply used
	 vertices using PolyMesh optimization later) */
      po->verts[i] = i;
    } /* for */

  po->ncontrols = numtris*3;
  po->has_normals = has_vn;

  /* now copy all the vertices and normals */
  if(!(po->controlv = malloc(numtris*3*stride*sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  if(has_tc)
    {
      if(!(tctagbuf = malloc((strlen(myst) + 64 +
			      numtris*3*2*(TCL_DOUBLE_SPACE+1)) *
			     sizeof(char))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}
      sprintf(tctagbuf, "%s,varying,g,%u", myst, numtris*3);
      tctagptr = tctagbuf+strlen(tctagbuf);
    }

  if(has_vc)
    {
      if(!(vctagbuf = malloc((strlen(myvc) + 64 +
			      numtris*3*3*(TCL_DOUBLE_SPACE+1)) *
			     sizeof(char))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}
      sprintf(vctagbuf, "%s,varying,c,%u", myvc, numtris*3);
      vctagptr = vctagbuf+strlen(vctagbuf);
    }

  i = 0;
  tri = tris;
  while(tri)
    {
      if(has_vn)
	{
	  memcpy(&(po->controlv[i]), tri->p1, 3*sizeof(double));
	  memcpy(&(po->controlv[i+3]), tri->n1, 3*sizeof(double));
	  memcpy(&(po->controlv[i+6]), tri->p2, 3*sizeof(double));
	  memcpy(&(po->controlv[i+9]), tri->n2, 3*sizeof(double));
	  memcpy(&(po->controlv[i+12]), tri->p3, 3*sizeof(double));
	  memcpy(&(po->controlv[i+15]), tri->n3, 3*sizeof(double));
	}
      else
	{
	  memcpy(&(po->controlv[i]), tri->p1, 3*sizeof(double));
	  memcpy(&(po->controlv[i+3]), tri->p2, 3*sizeof(double));
	  memcpy(&(po->controlv[i+6]), tri->p3, 3*sizeof(double));
	}

      if(has_tc)
	{
	  tctagptr += sprintf(tctagptr,
			      ",%g,%g,%g,%g,%g,%g",
			      tri->t1[0], tri->t1[1],
			      tri->t2[0], tri->t2[1],
			      tri->t3[0], tri->t3[1]);
	}

      if(has_vc)
	{
	  vctagptr += sprintf(vctagptr,
			      ",%g,%g,%g,%g,%g,%g,%g,%g,%g",
			      tri->c1[0], tri->c1[1], tri->c1[2],
			      tri->c2[0], tri->c2[1], tri->c2[2],
			      tri->c3[0], tri->c3[1], tri->c3[2]);
	}

      i += (3*stride);
      tri = tri->next;
    } /* while */

  if(has_tc)
    {
      ay_status = ay_tess_addtag(new, tctagbuf);
      if(ay_status)
	goto cleanup;
      tctagbuf = NULL;
    } /* if has_tc */

  if(has_vc)
    {
      ay_status = ay_tess_addtag(new, vctagbuf);
      if(ay_status)
	goto cleanup;
      vctagbuf = NULL;
    } /* if has_vc */

  *result = new;

  /* prevent cleanup code from doing something harmful */
  new = NULL;
  po = NULL;

cleanup:
  if(new)
    {
      if(new->tags)
	{
	  ay_tags_delall(new);
	}
      free(new);
    }

  if(po)
    {
      if(po->nloops)
	free(po->nloops);

      if(po->nverts)
	free(po->nverts);

      if(po->verts)
	free(po->verts);

      if(po->controlv)
	free(po->controlv);

      free(po);
    }

  if(tctagbuf)
    free(tctagbuf);

  if(vctagbuf)
    free(vctagbuf);

 return ay_status;
} /* ay_tess_tristopomesh */


/* ay_tess_npatch:
 *  tesselate the NURBS patch object o into a PolyMesh object
 *  using the GLU (V1.3+) tesselation facility
 *  smethod - sampling method:
 *   1: GLU_OBJECT_PARAMETRIC_ERROR
 *   2: GLU_OBJECT_PATH_LENGTH
 *   3: GLU_DOMAIN_DISTANCE
 *   4: AY_NORMALIZED_DOMAIN_DISTANCE
 *   5: AY_ADAPTIVE_DOMAIN_DISTANCE
 *   6: AY_ADAPTIVE_KNOT_DISTANCE
 *  sparamu/sparamv - sampling parameters (sparamv only used by
 *  smethod 3 - 6)
 *  use_tc - use texture coordinates delivered by a PV tag
 *  myst - name of PV tag used for the texture coordinates
 *  use_vc - use vertex colors delivered by a PV tag
 *  mycs - name of PV tag used for the vertex colors
 *  use_vn - use vertex normals delivered by a PV tag
 *  myn - name of PV tag used for the vertex normals
 *  refine_trims - how many times shall the trim curves be refined
 *  primitives - what primitives to emit:
 *   0: Triangles
 *   1: Triangles&Quads
 *   2: Quads
 *   3: QuadrangulatedTriangles
 *  quad_eps: maximum deviation of triangle normals to be combined to a quad
 *  pm - resulting PolyMesh object
 */
int
ay_tess_npatch(ay_object *o,
	       int smethod, double sparamu, double sparamv,
	       int use_tc, char *myst,
	       int use_vc, char *mycs,
	       int use_vn, char *myn,
	       int refine_trims, int primitives, double quad_eps,
	       ay_object **pm)
{
#ifndef GLU_VERSION_1_3
 char fname[] = "tess_npatch";
 ay_error(AY_ERROR, fname, "This function is just available on GLU V1.3+ !");
 return AY_ERROR;
#else
 int ay_status = AY_OK;
 ay_object *new = NULL, *newq = NULL;
 ay_nurbpatch_object *npatch = NULL;
 int uorder = 0, vorder = 0, width = 0, height = 0;
 int j, uknot_scount = 0, vknot_scount = 0;
 unsigned int uknot_count = 0, vknot_count = 0, i = 0, a = 0;
 GLfloat *uknots = NULL, *vknots = NULL, *controls = NULL;
 GLfloat *texcoords = NULL, *vcolors = NULL, *vnormals = NULL;
 GLfloat *vcolors4 = NULL, *tmp = NULL;
 unsigned int texcoordlen, vcolorlen, vnormallen;
 char have_tc = AY_FALSE, have_vc = AY_FALSE, have_vn = AY_FALSE;
 ay_tag *tag = NULL;
 ay_tess_object to = {0};
 double p1[3], p2[3], p3[3], p4[3], n1[3], n2[3], n3[3], n4[3];
 double t1[2], t2[2], t3[2], t4[2];
 double c1[4], c2[4], c3[4], c4[4];
 double knotlen, w, *tc = NULL, *uv = NULL;
 ay_tess_tri *tr1 = NULL, *tr2;

  if(!o || !pm)
    return AY_ENULL;

  if(use_tc && !myst)
    myst = ay_prefs.texcoordname;

  if(use_vc && !mycs)
    mycs = ay_prefs.colorname;

  if(use_vn && !myn)
    myn = ay_prefs.normalname;

  if(o->type != AY_IDNPATCH)
    return AY_ERROR;

  npatch = (ay_nurbpatch_object *)(o->refine);

  /* properly initialize tesselation object */
  to.has_vn = AY_TRUE;
  to.has_tc = use_tc;
  to.p1 = p1;
  to.p2 = p2;
  to.p3 = p3;
  to.p4 = p4;
  to.n1 = n1;
  to.n2 = n2;
  to.n3 = n3;
  to.n4 = n4;

  to.t1 = t1;
  to.t2 = t2;
  to.t3 = t3;
  to.t4 = t4;

  to.c1 = c1;
  to.c2 = c2;
  to.c3 = c3;
  to.c4 = c4;

  to.nextpd = &(to.p1);
  to.nextnd = &(to.n1);
  to.nextcd = &(to.c1);
  to.nexttd = &(to.t1);

  /* convert npatch data from double to float */
  uorder = npatch->uorder;
  vorder = npatch->vorder;
  width = npatch->width;
  height = npatch->height;

  uknot_count = width + uorder;
  vknot_count = height + vorder;

  if(!(uknots = calloc(uknot_count, sizeof(GLfloat))))
    { ay_status = AY_EOMEM; goto cleanup; }
  if(!(vknots = calloc(vknot_count, sizeof(GLfloat))))
    { ay_status = AY_EOMEM; goto cleanup; }
  if(!(controls = calloc(width*height*4, sizeof(GLfloat))))
    { ay_status = AY_EOMEM; goto cleanup; }

  a = 0;
  for(i = 0; i < uknot_count; i++)
    {
      uknots[a] = (GLfloat)npatch->uknotv[a];
      a++;
    }
  a = 0;
  for(i = 0; i < vknot_count; i++)
    {
      vknots[a] = (GLfloat)npatch->vknotv[a];
      a++;
    }
  a = 0;
  for(i = 0; i < (unsigned int)width*height; i++)
    {
      w = npatch->controlv[a+3];
      controls[a] = (GLfloat)(npatch->controlv[a]*w);
      a++;
      controls[a] = (GLfloat)(npatch->controlv[a]*w);
      a++;
      controls[a] = (GLfloat)(npatch->controlv[a]*w);
      a++;
      controls[a] = (GLfloat)npatch->controlv[a];
      a++;
    }

#ifdef AYGLUATTRIBBUG
  glPushAttrib((GLbitfield) GL_EVAL_BIT);
#endif

  if(npatch->no)
    {
      gluDeleteNurbsRenderer(npatch->no);
      npatch->no = NULL;
    }

  npatch->no = gluNewNurbsRenderer();
  if(npatch->no == NULL)
    { ay_status = AY_EOMEM; goto cleanup; }

  /* register error handling callback */
  gluNurbsCallback(npatch->no, GLU_ERROR, AYGLUCBTYPE ay_error_glucb);

  /* set properties */
  gluNurbsProperty(npatch->no, GLU_NURBS_MODE, GLU_NURBS_TESSELLATOR);

  gluNurbsProperty(npatch->no, GLU_DISPLAY_MODE, GLU_FILL);

  /* set sampling method and parameter(s) */
  switch(smethod)
    {
    case 1:
      gluNurbsProperty(npatch->no, GLU_SAMPLING_METHOD,
		       GLU_OBJECT_PARAMETRIC_ERROR);
      gluNurbsProperty(npatch->no, GLU_PARAMETRIC_TOLERANCE,
		       (GLfloat)sparamu);
      break;
    case 2:
      gluNurbsProperty(npatch->no, GLU_SAMPLING_METHOD,
		       GLU_OBJECT_PATH_LENGTH);
      gluNurbsProperty(npatch->no, GLU_SAMPLING_TOLERANCE,
		       (GLfloat)sparamu);
      break;
    case 3:
      /* use sparamu/sparamv unchanged */
      break;
    case 4:
      /* U */
      knotlen = npatch->uknotv[npatch->width] -
	npatch->uknotv[npatch->uorder - 1];
      sparamu = sparamu / knotlen;
      /* V */
      knotlen = npatch->vknotv[npatch->height] -
	npatch->vknotv[npatch->vorder - 1];
      sparamv = sparamv / knotlen;
      break;
    case 5:
      /* U */
      knotlen = npatch->uknotv[npatch->width] -
	npatch->uknotv[npatch->uorder - 1];
      sparamu = ((4 + npatch->width) * sparamu) / knotlen;
      /* V */
      knotlen = npatch->vknotv[npatch->height] -
	npatch->vknotv[npatch->vorder -  1];
      sparamv = ((4 + npatch->height) * sparamv) / knotlen;
      break;
    case 6:
      /* U */
      knotlen = npatch->uknotv[npatch->width] -
	npatch->uknotv[npatch->uorder - 1];
      for(j = npatch->uorder - 1; j <= npatch->width; j++)
	{
	  if(fabs(npatch->uknotv[j] - npatch->uknotv[j+1]) > AY_EPSILON)
	    uknot_scount++;
	}
      sparamu = (uknot_scount * sparamu) / knotlen;
      /* V */
      knotlen = npatch->vknotv[npatch->height] -
	npatch->vknotv[npatch->vorder -  1];
      for(j = npatch->vorder - 1; j <= npatch->height; j++)
	{
	  if(fabs(npatch->vknotv[j] - npatch->vknotv[j+1]) > AY_EPSILON)
	    vknot_scount++;
	}
      sparamv = (vknot_scount * sparamv) / knotlen;
      break;
    default:
      break;
    } /* switch */

  if(smethod > 2)
    {
      gluNurbsProperty(npatch->no, GLU_SAMPLING_METHOD, GLU_DOMAIN_DISTANCE);
      gluNurbsProperty(npatch->no, GLU_U_STEP, (GLfloat)sparamu);
      gluNurbsProperty(npatch->no, GLU_V_STEP, (GLfloat)sparamv);
    }
  /*
  gluNurbsProperty(npatch->no, GLU_AUTO_LOAD_MATRIX, GL_FALSE);
  */

  /* register callbacks to get tesselated data back from GLU */
  gluNurbsCallbackData(npatch->no, (void *)(&to));
  gluNurbsCallback(npatch->no, GLU_NURBS_BEGIN_DATA,
		   AYGLUCBTYPE ay_tess_begindata);
  gluNurbsCallback(npatch->no, GLU_NURBS_VERTEX_DATA,
		   AYGLUCBTYPE ay_tess_vertexdata);
  gluNurbsCallback(npatch->no, GLU_NURBS_NORMAL_DATA,
		   AYGLUCBTYPE ay_tess_normaldata);
  gluNurbsCallback(npatch->no, GLU_NURBS_END_DATA,
		   AYGLUCBTYPE ay_tess_enddata);

  if(use_tc)
    {
      gluNurbsCallback(npatch->no, GLU_NURBS_TEXTURE_COORD_DATA,
		       AYGLUCBTYPE ay_tess_texcoorddata);
    }

  if(use_vc)
    {
      gluNurbsCallback(npatch->no, GLU_NURBS_COLOR_DATA,
		       AYGLUCBTYPE ay_tess_colordata);
    }

  /* tesselate the patch */
  gluBeginSurface(npatch->no);

  /* put surface data */
  gluNurbsSurface(npatch->no, (GLint)uknot_count, uknots,
		  (GLint)vknot_count, vknots,
		  (GLint)height*4, (GLint)4, controls,
		  (GLint)npatch->uorder, (GLint)npatch->vorder,
		  GL_MAP2_VERTEX_4);

  if((use_tc || use_vc || use_vn) && o->tags)
    {
      tag = o->tags;
      while(tag)
	{
	  /* put texture coordinates */
	  if(use_tc && !have_tc)
	    {
	      if(ay_pv_checkndt(tag, myst, "varying", "g"))
		{
		  ay_status = ay_pv_convert(tag, 1, &texcoordlen,
					    (void**)(void*)&texcoords);

		  if(!ay_status && texcoords)
		    {
		      to.has_tc = AY_TRUE;
		      /*
			the use of (GLint)2, (GLint)width*2
			for s_stride, t_stride as opposed to
			(GLint)height*4, (GLint)4, above
			caters for the fact that PV tag data is
			stored in u-major (whereas the (GLU)
			NURBS patch is in v-major order
		      */
		      if(texcoordlen == (unsigned int)(width*height))
			gluNurbsSurface(npatch->no, (GLint)uknot_count, uknots,
			  (GLint)vknot_count, vknots,
			  (GLint)2, (GLint)width*2, texcoords,
			  (GLint)npatch->uorder, (GLint)npatch->vorder,
			  GL_MAP2_TEXTURE_COORD_2);
		    }
		  have_tc = AY_TRUE;
		  tag = tag->next;
		  continue;
		} /* if */
	    } /* if */

	  /* put vertex colors */
	  if(use_vc && !have_vc)
	    {
	      if(ay_pv_checkndt(tag, mycs, "varying", "c") ||
		 ay_pv_checkndt(tag, mycs, "varying", "d"))
		{
		  ay_status = ay_pv_convert(tag, 1, &vcolorlen,
					    (void**)(void*)&vcolors);

		  if(!ay_status && vcolors)
		    {
		      if(!(vcolors4 = calloc(vcolorlen*4, sizeof(GLfloat))))
			{ ay_status = AY_EOMEM; goto cleanup; }
		      a = 0;
		      if(ay_pv_checkndt(tag, mycs, "varying", "c"))
			{
			  for(i = 0; i < vcolorlen; i++)
			    {
			      memcpy(&(vcolors4[a]), &(vcolors[i*3]),
				     3*sizeof(GLfloat));
			      /*vcolors4[a+3] = 1.0;*/
			      a += 4;
			    }
			}
		      else
			{
			  for(i = 0; i < vcolorlen; i++)
			    {
			      memcpy(&(vcolors4[a]), &(vcolors[i*4]),
				     4*sizeof(GLfloat));
			      a += 4;
			    }
			}
		      to.has_vc = AY_TRUE;
		      /*
			the use of (GLint)4, (GLint)width*4
			for s_stride, t_stride as opposed to
			(GLint)height*4, (GLint)4, above
			caters for the fact that PV tag data is
			stored in u-major (whereas the (GLU)
			NURBS patch is in v-major order
		      */
		      if(vcolorlen == (unsigned int)(width*height))
			gluNurbsSurface(npatch->no, (GLint)uknot_count, uknots,
			      (GLint)vknot_count, vknots,
			      (GLint)4, (GLint)width*4, vcolors4,
			      (GLint)npatch->uorder, (GLint)npatch->vorder,
			      GL_MAP2_COLOR_4);
		    } /* if */
		  have_vc = AY_TRUE;
		  tag = tag->next;
		  continue;
		} /* if */
	    } /* if */

	  /* put vertex normals */
	  if(use_vn && !have_vn)
	    {
	      if(ay_pv_checkndt(tag, myn, "varying", "n"))
		{
		  ay_status = ay_pv_convert(tag, 1, &vnormallen,
					    (void**)(void*)&vnormals);

		  if(!ay_status && vnormals)
		    {
		      if(vnormallen < (unsigned int)(width*height))
			{
			  if((tmp = realloc(vnormals, width * height * 3 *
					    sizeof(GLfloat))))
			    {
			      vnormals = tmp;
			      for(i = 0; i < (width * height) - vnormallen; i++)
				{
				  memcpy(&(vnormals[(vnormallen+i)*3]),
					 &(vnormals[(vnormallen-1)*3]),
					 3*sizeof(GLfloat));
				}
			      vnormallen = width * height;
			    }
			}
		      /*
			the use of (GLint)3, (GLint)width*3
			for s_stride, t_stride as opposed to
			(GLint)height*4, (GLint)4, above
			caters for the fact that PV tag data is
			stored in u-major (whereas the (GLU)
			NURBS patch is in v-major order
		      */
		      if(vnormallen == (unsigned int)(width*height))
			gluNurbsSurface(npatch->no, (GLint)uknot_count, uknots,
			      (GLint)vknot_count, vknots,
			      (GLint)3, (GLint)width*3, vnormals,
			      (GLint)npatch->uorder, (GLint)npatch->vorder,
			      GL_MAP2_NORMAL);
		    }
		  have_vn = AY_TRUE;
		  tag = tag->next;
		  continue;
		} /* if */
	    } /* if */

	  tag = tag->next;
	} /* while */
    } /* if */

  /* put synthesized texture coordinates? */
  if(use_tc && !have_tc)
    {
      ay_npt_gentexcoords(npatch, o->tags, &tc);

      if(tc)
	{
	  if((texcoords = malloc(width*height*2*sizeof(float))))
	    {
	      for(i = 0; i < (unsigned int)(width*height*2); i++)
		texcoords[i] = (float)tc[i];

	      to.has_tc = AY_TRUE;

	      gluNurbsSurface(npatch->no, (GLint)uknot_count, uknots,
			      (GLint)vknot_count, vknots,
			      (GLint)height*2, (GLint)2, texcoords,
			      (GLint)npatch->uorder, (GLint)npatch->vorder,
			      GL_MAP2_TEXTURE_COORD_2);
	      have_tc = AY_TRUE;
	    }
	  free(tc);
	}
    }

  /* put trimcurves */
  if(o->down && o->down->next)
    {
      ay_status = ay_npt_drawtrimcurves(o, refine_trims);
    }

  gluEndSurface(npatch->no);

  /* the tess_object should now contain lots of triangles;
     convert them to a PolyMesh object */
  switch(primitives)
    {
    case 0:
      /* Triangles */
      ay_status = ay_tess_tristopomesh(to.tris, AY_TRUE, have_vc, have_tc,
				       myst, mycs, &new);
      break;
    case 1:
      /* Triangles and Quads */
      ay_status = ay_tess_tristomixedpomesh(to.tris, AY_TRUE, have_vc, have_tc,
					    myst, mycs, quad_eps, &new);
      break;
    case 2:
      /* Quads */
      ay_status = ay_tess_tristoquadpomesh(to.tris, AY_TRUE, have_vc, have_tc,
					   myst, mycs, quad_eps, &new);
      break;
    case 3:
      /* Quadrangulated Triangles */
      ay_status = ay_tess_tristopomesh(to.tris, AY_TRUE, have_vc, have_tc,
				       myst, mycs, &new);

      tr1 = to.tris;
      while(tr1)
	{
	  tr2 = tr1;
	  tr1 = tr1->next;
	  free(tr2);
	} /* while */
      to.tris = NULL;

      gluNurbsCallback(npatch->no, GLU_NURBS_TEXTURE_COORD_DATA,
		       AYGLUCBTYPE ay_tess_texcoorddata);

      gluBeginSurface(npatch->no);

       /* put surface data */
       gluNurbsSurface(npatch->no, (GLint)uknot_count, uknots,
		       (GLint)vknot_count, vknots,
		       (GLint)height*4, (GLint)4, controls,
		       (GLint)npatch->uorder, (GLint)npatch->vorder,
		       GL_MAP2_VERTEX_4);

       tc = NULL;
       ay_npt_gentexcoords(npatch, /*tags=*/NULL, &tc);

       if(tc)
	 {
	   if(texcoords)
	     free(texcoords);

	   if((texcoords = malloc(width*height*2*sizeof(float))))
	     {
	       for(i = 0; i < (unsigned int)(width*height*2); i++)
		 texcoords[i] = (float)tc[i];

	       to.has_tc = AY_TRUE;

	       gluNurbsSurface(npatch->no, (GLint)uknot_count, uknots,
			       (GLint)vknot_count, vknots,
			       (GLint)height*2, (GLint)2, texcoords,
			       (GLint)npatch->uorder, (GLint)npatch->vorder,
			       GL_MAP2_TEXTURE_COORD_2);
	       have_tc = AY_TRUE;
	     }
	   free(tc);
	 }

       /* put trimcurves */
       if(o->down && o->down->next)
	 {
	   ay_status = ay_npt_drawtrimcurves(o, refine_trims);
	 }

      gluEndSurface(npatch->no);

      ay_tess_tristouv(&to, ((ay_pomesh_object*)new->refine)->npolys, &uv);

      ay_status = ay_tess_quadrangulate(new, npatch, uv, myst, &newq);

      if(uv)
	free(uv);
      ay_object_delete(new);
      new = newq;

      break;
    default:
      break;
    } /* switch primitives */

  gluDeleteNurbsRenderer(npatch->no);
  npatch->no = NULL;

#ifdef AYGLUATTRIBBUG
  glPopAttrib();
#endif

  if(ay_status || !new)
    goto cleanup;

  ay_pomesht_fliploops((ay_pomesh_object*)new->refine);
  ay_pomesht_flipnormals((ay_pomesh_object*)new->refine);

  /* immediately optimize the polymesh (remove multiply used vertices) */
  if(!primitives && new && !have_tc && !have_vc)
    {
      ay_status = ay_pomesht_optimizecoords((ay_pomesh_object*)new->refine,
					    0.0, NULL, NULL, NULL);

      if(ay_status)
	goto cleanup;
    }

  /* return result */
  *pm = new;

  /* clean up */

cleanup:

  if(uknots)
    free(uknots);

  if(vknots)
    free(vknots);

  if(controls)
    free(controls);

  if(npatch->no)
    {
      gluDeleteNurbsRenderer(npatch->no);
      npatch->no = NULL;
    }

  if(texcoords)
    free(texcoords);

  if(vnormals)
    free(vnormals);

  if(vcolors)
    free(vcolors);

  if(vcolors4)
    free(vcolors4);

  tr1 = to.tris;
  while(tr1)
    {
      tr2 = tr1;
      tr1 = tr1->next;
      free(tr2);
    } /* while */
  to.tris = NULL;

 return ay_status;
#endif
} /* ay_tess_npatch */


/** ay_tess_isdegenatv1
 * Check patch for degeneracy at V1.
 *
 * \param np  NURBS patch to interrogate
 *
 * \returns AY_TRUE if the patch is degenerate, AY_FALSE else
 */
int
ay_tess_isdegenatv1(ay_nurbpatch_object *np)
{
 double *cv, *a, *b;
 int i;

  if(!np)
    return AY_FALSE;

  cv = np->controlv;
  a = cv+(4*(np->height-1));
  b = a+(4*np->height);
  for(i = 0; i < np->width-1; i++)
    {
      if(!AY_V3COMP(a,b))
	return AY_FALSE;
      a = b;
      b += (4*np->height);
    }

 return AY_TRUE;
} /* ay_tess_isdegenatv1 */


/* ay_tess_npatchtcmd:
 *  Tesselate selected NURBS patches (convert to PolyMesh) with GLU.
 *  Implements the \a tessNP scripting interface command.
 *  Also implements the \a rtessNP scripting interface command.
 *  Also implements the \a stessNP scripting interface command.
 *  See also the corresponding section in the \ayd{sctessnp}.
 *  \returns TCL_OK in any case.
 */
int
ay_tess_npatchtcmd(ClientData clientData, Tcl_Interp *interp,
		   int argc, char *argv[])
{
 int ay_status;
 ay_list_object *sel = ay_selection;
 ay_object *o = NULL, *new = NULL;
 ay_stess_patch stesselation = {0};
 double sparamu = ay_prefs.sparamu, sparamv = ay_prefs.sparamv;
 double quad_eps = DBL_MAX;
 int smethod = ay_prefs.smethod+1;
 int use_tc = AY_FALSE, use_vc = AY_FALSE, use_vn = AY_FALSE;
 int refine_trims = 0, primitives = 0, qf = 2;
 int mode = 0;
 int degen = AY_FALSE, notify_parent = AY_FALSE;
 double et = 25.0;

  /* distinguish between
     tessNP and rtessNP */
  if(argv[0][0] == 'r')
    mode = 1;
  /* distinguish between
     tessNP and stessNP */
  if(argv[0][0] == 's')
    mode = 2;

  if(argc > 1)
    {
      switch(mode)
	{
	case 0:
	  /* tessNP */
	  Tcl_GetInt(interp, argv[1], &smethod);

	  if(smethod < 0)
	    smethod = 0;
	  else
	    if(smethod > 6)
	      smethod = 6;

	  switch(smethod)
	    {
	    case 1:
	      sparamu = 0.25;
	      break;
	    case 2:
	      sparamu = 1.5;
	      break;
	    case 3:
	    case 4:
	    case 5:
	      sparamu = 10.0;
	      sparamv = 10.0;
	      break;
	    case 6:
	      sparamu = 3.0;
	      sparamv = 3.0;
	      break;
	    default:
	      break;
	    } /* switch smethod */

	  if(argc > 2)
	    {
	      Tcl_GetDouble(interp, argv[2], &sparamu);
	    }

	  if(argc > 3)
	    {
	      Tcl_GetDouble(interp, argv[3], &sparamv);
	    }

	  if(argc > 4)
	    {
	      Tcl_GetInt(interp, argv[4], &use_tc);
	    }

	  if(argc > 5)
	    {
	      Tcl_GetInt(interp, argv[5], &use_vc);
	    }

	  if(argc > 6)
	    {
	      Tcl_GetInt(interp, argv[6], &use_vn);
	    }

	  if(argc > 7)
	    {
	      Tcl_GetInt(interp, argv[7], &refine_trims);

	      if(refine_trims < 0)
		refine_trims = 0;
	      else
		if(refine_trims > 5)
		  refine_trims = 5;
	    }

	  if(argc > 8)
	    {
	      Tcl_GetInt(interp, argv[8], &primitives);

	      if(primitives < 0)
		primitives = 0;
	      else
		if(primitives > 2)
		  primitives = 2;
	    }

	  if(argc > 9)
	    {
	      Tcl_GetDouble(interp, argv[9], &quad_eps);
	    }
	  break;
	case 1:
	  /* rtessNP */
	  Tcl_GetInt(interp, argv[1], &degen);

	  if(argc > 2)
	    {
	      Tcl_GetDouble(interp, argv[2], &et);
	    }
	  break;
	case 2:
	  /* stessNP */
	  Tcl_GetInt(interp, argv[1], &qf);
	  break;
	default:
	  break;
	} /* switch mode */
    } /* if have args */

  while(sel)
    {
      o = sel->object;

      if(o->type == AY_IDNPATCH)
	{
	  new = NULL;
	  switch(mode)
	    {
	    case 0:
	      ay_status = ay_tess_npatch(o, smethod, sparamu, sparamv,
					 use_tc, NULL,
					 use_vc, NULL,
					 use_vn, NULL,
					 refine_trims,
					 primitives,
					 quad_eps,
					 &new);
	      break;
	    case 1:
	      if(degen)
		{
		  if(!ay_tess_isdegenatv1(o->refine))
		    {
		      ay_error(AY_ERROR, argv[0],
			       "Patch is not degenerate at V1!");
		      return TCL_OK;
		    }
		}

	      ay_status = ay_rtess_tessnp(o->refine, degen, et, &new);
	      break;
	    case 2:
	      ay_status = ay_stess_TessNP(o, qf, &stesselation);
	      if(!ay_status)
		ay_status = ay_stess_topomesh(&stesselation, &new);
	      ay_stess_destroy(&stesselation);
	      break;
	    default:
	      break;
	    } /* switch */

	  if(!ay_status)
	    {
	      ay_trafo_copy(o, new);
	      ay_object_link(new);
	      notify_parent = AY_TRUE;
	    }
	  else
	    {
	      ay_error(AY_ERROR, argv[0], "Could not tesselate object!");
	      return TCL_OK;
	    }
	}
      else
	{
	  ay_error(AY_EWTYPE, argv[0], "NPatch");
	} /* if is NPatch */

      sel = sel->next;
    } /* while */

  if(notify_parent)
    (void)ay_notify_parent();

 return TCL_OK;
} /* ay_tess_npatchtcmd */


/* ay_tess_pomeshf:
 *  tesselate the face <f> of PolyMesh <pomesh> into triangles, removes doubly
 *  used vertices if <optimize> is AY_TRUE,
 *  <m> and <n> have to be set up correctly (pointing in the index arrays
 *  for nloops and nverts of face <f>) outside!,
 *  returns new PolyMesh in <trpomesh>
 */
int
ay_tess_pomeshf(ay_pomesh_object *pomesh,
		unsigned int f, unsigned int m, unsigned int n,
		int optimize,
		ay_pomesh_object **trpomesh)
{
#ifndef GLU_VERSION_1_2
 char fname[] = "tess_pomeshf";
 ay_error(AY_ERROR, fname, "This function is just available on GLU V1.2+ !");
 return AY_ERROR;
#else
 int ay_status = AY_OK;
 unsigned int j = 0, k = 0;
 unsigned int a;
 int stride = 0;
 GLUtesselator *tess = NULL;
 ay_tess_object to = {0};
 double p1[3], p2[3], p3[3], p4[3], n1[3], n2[3], n3[3], n4[3];
 ay_tess_tri *t1 = NULL, *t2;
 ay_object *tmpo = NULL;

  if(pomesh->has_normals)
    stride = 6;
  else
    stride = 3;

  if(!(tess = gluNewTess()))
    return AY_EOMEM;

  /* properly initialize tesselation object */
  to.tesselate_polymesh = AY_TRUE;
  to.has_vn = pomesh->has_normals;
  to.p1 = p1;
  to.p2 = p2;
  to.p3 = p3;
  to.p4 = p4;
  to.n1 = n1;
  to.n2 = n2;
  to.n3 = n3;
  to.n4 = n4;

  to.nextpd = &(to.p1);
  to.nextnd = &(to.n1);

  gluTessCallback(tess, GLU_TESS_ERROR, AYGLUCBTYPE ay_error_glucb);
  gluTessCallback(tess, GLU_TESS_BEGIN_DATA, AYGLUCBTYPE ay_tess_begindata);
  gluTessCallback(tess, GLU_TESS_VERTEX_DATA, AYGLUCBTYPE ay_tess_vertexdata);
  /*  gluTessCallback(tess, GLU_TESS_NORMAL_DATA, AYGLUCBTYPE ay_tess_normaldata);*/
  gluTessCallback(tess, GLU_TESS_END_DATA, AYGLUCBTYPE ay_tess_enddata);
  gluTessCallback(tess, GLU_TESS_COMBINE_DATA, AYGLUCBTYPE ay_tess_combinedata);

  gluTessBeginPolygon(tess, (GLvoid*)(&to));
   for(j = 0; j < pomesh->nloops[f]; j++)
     {
       gluTessBeginContour(tess);
        for(k = 0; k < pomesh->nverts[m]; k++)
	  {
	    a = pomesh->verts[n++];
	    gluTessVertex(tess,
			  (GLdouble*)(&(pomesh->controlv[a*stride])),
			  (GLdouble*)(&(pomesh->controlv[a*stride])));
	  } /* for */
       gluTessEndContour(tess);
       m++;
     } /* for */
  gluTessEndPolygon(tess);

  gluDeleteTess(tess);

  /* free combined vertices */
  ay_tess_managecombined(NULL);

  if(!to.tris)
    {
      ay_status = AY_ERROR; goto cleanup;
    }

  /* the tess_object should now contain lots of triangles;
     copy them to the PolyMesh object */
  ay_status = ay_tess_tristopomesh(to.tris, to.has_vn, AY_FALSE, AY_FALSE,
				   NULL, NULL, &tmpo);

  if(!tmpo)
    {
      ay_status = AY_ERROR; goto cleanup;
    }

  /* immediately optimize the polymesh (remove multiply used vertices) */
  if(optimize)
    ay_status = ay_pomesht_optimizecoords((ay_pomesh_object*)tmpo->refine,
					  0.0, NULL, NULL, NULL);

  /* return result */
  *trpomesh = tmpo->refine;

cleanup:
  if(tmpo)
    free(tmpo);

  /* free triangles */
  t1 = to.tris;
  while(t1)
    {
      t2 = t1;
      t1 = t1->next;
      free(t2);
    } /* while */
  to.tris = NULL;

 return ay_status;
#endif
} /* ay_tess_pomeshf */


/* ay_tess_pomesh:
 *  tesselate the PolyMesh <pomesh> into triangles, removes doubly
 *  used vertices if <optimize> is AY_TRUE, if <normal> is not NULL it
 *  provides a normal that will be set to the tesselator via gluTessNormal(),
 *  returns new PolyMesh in <trpomesh>
 */
int
ay_tess_pomesh(ay_pomesh_object *pomesh, int optimize, double *normal,
	       ay_pomesh_object **trpomesh)
{
#ifndef GLU_VERSION_1_2
 char fname[] = "tess_pomesh";
 ay_error(AY_ERROR, fname, "This function is just available on GLU V1.2+ !");
 return AY_ERROR;
#else
 int ay_status = AY_OK;
 unsigned int i = 0, j = 0, k = 0, l = 0, m = 0, n = 0;
 unsigned int a;
 int stride = 0;
 GLUtesselator *tess = NULL;
 ay_tess_object to = {0};
 double p1[3], p2[3], p3[3], p4[3], n1[3], n2[3], n3[3], n4[3];
 ay_tess_tri *t1 = NULL, *t2;
 ay_object *tmpo = NULL;

  if(!pomesh)
    return AY_ENULL;

  if(pomesh->has_normals)
    stride = 6;
  else
    stride = 3;

  if(!(tess = gluNewTess()))
    return AY_EOMEM;

  /* properly initialize tesselation object */
  to.tesselate_polymesh = AY_TRUE;
  to.has_vn = pomesh->has_normals;
  to.p1 = p1;
  to.p2 = p2;
  to.p3 = p3;
  to.p4 = p4;
  to.n1 = n1;
  to.n2 = n2;
  to.n3 = n3;
  to.n4 = n4;

  to.nextpd = &(to.p1);
  to.nextnd = &(to.n1);

  if(normal)
    gluTessNormal(tess, normal[0], normal[1], normal[2]);

  gluTessCallback(tess, GLU_TESS_ERROR, AYGLUCBTYPE ay_error_glucb);
  gluTessCallback(tess, GLU_TESS_BEGIN_DATA, AYGLUCBTYPE ay_tess_begindata);
  gluTessCallback(tess, GLU_TESS_VERTEX_DATA, AYGLUCBTYPE ay_tess_vertexdata);
  /*  gluTessCallback(tess, GLU_TESS_NORMAL_DATA, AYGLUCBTYPE ay_tess_normaldata);*/
  gluTessCallback(tess, GLU_TESS_END_DATA, AYGLUCBTYPE ay_tess_enddata);
  gluTessCallback(tess, GLU_TESS_COMBINE_DATA, AYGLUCBTYPE ay_tess_combinedata);
  for(i = 0; i < pomesh->npolys; i++)
    {
      gluTessBeginPolygon(tess, (GLvoid*)(&to));
       for(j = 0; j < pomesh->nloops[l]; j++)
	 {
	   gluTessBeginContour(tess);
	    for(k = 0; k < pomesh->nverts[m]; k++)
	      {
		a = pomesh->verts[n++];
		gluTessVertex(tess,
			      (GLdouble*)(&(pomesh->controlv[a*stride])),
			      (GLdouble*)(&(pomesh->controlv[a*stride])));
	      } /* for */
	   gluTessEndContour(tess);
	   m++;
	 } /* for */
      gluTessEndPolygon(tess);
      l++;
    } /* for */

  gluDeleteTess(tess);

  /* free combined vertices */
  ay_tess_managecombined(NULL);

  if(!to.tris)
    {
      ay_status = AY_ERROR; goto cleanup;
    }

  /* the tess_object should now contain lots of triangles;
     copy them to the PolyMesh object */
  ay_status = ay_tess_tristopomesh(to.tris, to.has_vn, AY_FALSE, AY_FALSE,
				   NULL, NULL, &tmpo);

  if(!tmpo)
    {
      ay_status = AY_ERROR; goto cleanup;
    }

  /* immediately optimize the polymesh (remove multiply used vertices) */
  if(optimize)
    ay_status = ay_pomesht_optimizecoords((ay_pomesh_object*)tmpo->refine,
					  0.0, NULL, NULL, NULL);

  /* return result */
  *trpomesh = tmpo->refine;

cleanup:
  if(tmpo)
    free(tmpo);

  /* free triangles */
  t1 = to.tris;
  while(t1)
    {
      t2 = t1;
      t1 = t1->next;
      free(t2);
    } /* while */
  to.tris = NULL;

 return ay_status;
#endif
} /* ay_tess_pomesh */


/** ay_tess_curvevertexcb:
 * Helper for ay_tess_ncurve() below.
 * This vertex data callback accepts a tesselated curve point,
 * and simply stores it into an automatically growing array.
 *
 * \param[in] vertex tesselated curve vertex
 * \param[in,out] userData pointer to a \a ay_curvetess structure
 *  where to store the vertices
 */
void
ay_tess_curvevertexcb(GLfloat *vertex, void *userData)
{
 ay_curvetess *tess;
 double v[3], *t;

  if(vertex && userData)
    {
      tess = (ay_curvetess*)userData;
      if(tess->is_rat)
	{
	  v[0] = (double)vertex[0]/vertex[3];
	  v[1] = (double)vertex[1]/vertex[3];
	  v[2] = (double)vertex[2]/vertex[3];
	}
      else
	{
	  v[0] = (double)vertex[0];
	  v[1] = (double)vertex[1];
	  v[2] = (double)vertex[2];
	}
      if(tess->vindex >= tess->vertslen)
	{
	  tess->vertslen *= 2;
	  if(!(t = realloc(tess->verts, tess->vertslen*3*sizeof(double))))
	    return;

	  tess->verts = t;
	}
      memcpy(&(tess->verts[3*tess->vindex]), v, 3*sizeof(double));
      tess->vindex++;
    }

 return;
} /* ay_tess_curvevertexcb */


/** ay_tess_ncurve:
 * Tesselate a NURBS curve to a polyline.
 *
 * \param[in] ncurve  NURBS curve to tesselate
 * \param[in] tmode  tesselation mode
 * \param[in] tparam  tesselation parameter
 * \param[in,out] verts  where to store the tesselated vertex array
 * \param[in,out] vertslen  where to store the length of \a verts
 *
 * \returns AY_OK on success, error code otherwise
 */
int
ay_tess_ncurve(ay_nurbcurve_object *ncurve, double tmode, double tparam,
	       double **verts, int *vertslen)
{
#ifndef GLU_VERSION_1_3
 char fname[] = "tess_ncurve";
 ay_error(AY_ERROR, fname, "This function is just available on GLU V1.3+ !");
 return AY_ERROR;
#else
 ay_curvetess tess = {0};
 int knot_count;
 GLUnurbsObj *no;

  if(!ncurve->fltcv)
    {
      ay_ncurve_cacheflt(ncurve);
    }

  if(!ncurve->fltcv)
    {
      return AY_ERROR;
    }

  if(!(tess.verts = malloc(64*3*sizeof(double))))
    {
      return AY_EOMEM;
    }

  tess.vertslen = 64;

  tess.is_rat = ncurve->is_rat;

  no = gluNewNurbsRenderer();
  if(no == NULL)
    {
      free(tess.verts);
      return AY_EOMEM;
    }

  knot_count = ncurve->length + ncurve->order;

#ifdef AYGLUATTRIBBUG
  glPushAttrib((GLbitfield) GL_EVAL_BIT);
#endif

  gluNurbsProperty(no, GLU_NURBS_MODE, GLU_NURBS_TESSELLATOR);

  gluNurbsCallback(no, GLU_NURBS_VERTEX_DATA,
		   AYGLUCBTYPE ay_tess_curvevertexcb);

  gluNurbsCallbackData(no, &tess);

  gluBeginCurve(no);
   gluNurbsProperty(no, GLU_SAMPLING_METHOD, tmode);
   gluNurbsProperty(no, GLU_SAMPLING_TOLERANCE,
		   (GLfloat)tparam);
   gluNurbsCurve(no, (GLint)knot_count, (GLfloat*)ncurve->fltcv,
		 (GLint)(ncurve->is_rat?4:3),
		 (GLfloat*)&(ncurve->fltcv[knot_count]),
		 (GLint)ncurve->order,
		 (ncurve->is_rat?GL_MAP1_VERTEX_4:GL_MAP1_VERTEX_3));
  gluEndCurve(no);

  gluNurbsCallback(no, GLU_NURBS_VERTEX_DATA, NULL);

  gluNurbsProperty(no, GLU_NURBS_MODE, GLU_NURBS_RENDERER);

  gluDeleteNurbsRenderer(no);

#ifdef AYGLUATTRIBBUG
  glPopAttrib();
#endif

  /* return result */
  *verts = tess.verts;
  *vertslen = tess.vindex;

 return AY_OK;
#endif
} /* ay_tess_ncurve */


/** ay_tess_tristouv:
 * Convert texture coordinate values stored in a tesselation
 * to a uv parameter array.
 *
 * \param[in] to  tesselation
 * \param[in] ntris  number of triangles in tesselation
 * \param[in,out] result  where to store the uv parameters
 *
 * \returns AY_OK on success, error code otherwise
 */
int
ay_tess_tristouv(ay_tess_object *to, unsigned int ntris, double **result)
{
 double *uv = NULL;
 ay_tess_tri *tri = to->tris;
 int i = 0;

  if(!(uv = malloc(ntris * 3 * 2 * sizeof(double))))
    return AY_EOMEM;

  while(tri)
    {
      memcpy(&(uv[i]), tri->t1, 2*sizeof(double));
      i += 2;
      memcpy(&(uv[i]), tri->t2, 2*sizeof(double));
      i += 2;
      memcpy(&(uv[i]), tri->t3, 2*sizeof(double));
      i += 2;

      tri = tri->next;
    }

  *result = uv;

 return AY_OK;
} /* ay_tess_tristouv */


/** ay_tess_surfacepoint:
 * Helper for ay_tess_quadrangulate() below. Calculate a single
 * surface point and normal.
 *
 * \param[in] np  NURBS surface to sample
 * \param[in] u  parametric value in U (width)
 * \param[in] v  parametric value in V (height)
 * \param[in,out] t  scratch space (of size obtained via
 *                   ay_nb_FirstDerSurf4DMSize()),
 *                   must be zeroed or appropriately filled with
 *                   spans / basis functions for each call
 * \param[in,out] p  where to store the computed point and normal
 */
void
ay_tess_surfacepoint(ay_nurbpatch_object *np, double u, double v,
		     double *t, double *p)
{
 double *fdu, *fdv, *N;
 double len;

  if(np->is_rat)
    ay_nb_FirstDerSurf4DM(np->width-1, np->height-1,
			  np->uorder-1, np->vorder-1,
			  np->uknotv, np->vknotv, np->controlv,
			  u, v, t);
  else
    ay_nb_FirstDerSurf3DM(np->width-1, np->height-1,
			  np->uorder-1, np->vorder-1,
			  np->uknotv, np->vknotv, np->controlv,
			  u, v, t);

  memcpy(p, t, 3*sizeof(double));


  fdu = &(t[3]);
  fdv = &(t[6]);
  N = &(p[3]);

  AY_V3CROSS(N, fdu, fdv);
  len = AY_V3LEN(N);
  if(len > AY_EPSILON)
    AY_V3SCAL(N, -1.0/len);

 return;
} /* ay_tess_surfacepoint */


/** ay_tess_isdegen:
 * Check a point on a NURBS surface for degeneracy, by tesselating
 * the surface at two additional points each offset from the original
 * point to check along u and v by provided amounts.
 * If any of these new points are identical to the point to check,
 * degeneracy is assumed.
 *
 * \param[in] p  3D coordinates of point to check for degeneracy
 * \param[in] np  NURBS surface
 * \param[in] u  parametric coordinate of point \a p in U direction (width)
 * \param[in] v  parametric coordinate of point \a p in V direction (height)
 * \param[in] ud  epsilon in U direction (width)
 * \param[in] vd  epsilon in V direction (height)
 *
 * \returns 0 - point is not degenerate,
 *          1 - point is degenerate in u direction (width),
 *          2 - point is degenerate in v direction (height)
 */
int
ay_tess_isdegen(double *p, ay_nurbpatch_object *np, double u, double v,
		double ud, double vd)
{
 double pd[6];

  if(np->is_rat)
    ay_nb_SurfacePoint4D(np->width-1, np->height-1,
			 np->uorder-1, np->vorder-1,
			 np->uknotv, np->vknotv, np->controlv,
			 u+ud, v, pd);
  else
    ay_nb_SurfacePoint3D(np->width-1, np->height-1,
			 np->uorder-1, np->vorder-1,
			 np->uknotv, np->vknotv, np->controlv,
			 u+ud, v, pd);
  if(AY_V3COMP(p, pd))
    return 1;

  if(np->is_rat)
    ay_nb_SurfacePoint4D(np->width-1, np->height-1,
			 np->uorder-1, np->vorder-1,
			 np->uknotv, np->vknotv, np->controlv,
			 u, v+vd, pd);
  else
    ay_nb_SurfacePoint3D(np->width-1, np->height-1,
			 np->uorder-1, np->vorder-1,
			 np->uknotv, np->vknotv, np->controlv,
			 u, v+vd, pd);
  if(AY_V3COMP(p, pd))
    return 2;

 return 0;
} /* ay_tess_isdegen */


/** ay_tess_checknormal
 * Check (and possibly correct) the normal of a new intermediate point
 * as created by the quadrangulation below.
 * This is necessary, because GLU and our NURBS tesselator may have
 * wildly different opinions on a surface normal of a given point on
 * a NURBS surface, especially in (or in the vicinity of) non-continuous
 * regions. Therefore we must compare the new normal to the other two
 * and if major differences are detected, we just use the mean of the
 * original two normals (N1 and N2).
 *
 * \param[in] N1  first normal
 * \param[in] N2  second normal
 * \param[in,out] NN  new normal
 */
void
ay_tess_checknormal(double *N1, double *N2, double *NN)
{
 double angle;

  angle = AY_V3DOT(N1,NN);
  if(angle <= -1.0)
    angle = -AY_PI;
  else
    if(angle >= 1.0)
      angle = 1.0;
    else
      angle = acos(angle);

  if(fabs(angle) > AY_D2R(45))
    {
      NN[0] = N1[0]+(N2[0]-N1[0])*0.5;
      NN[1] = N1[1]+(N2[1]-N1[1])*0.5;
      NN[2] = N1[2]+(N2[2]-N1[2])*0.5;
      return;
    }

  angle = AY_V3DOT(N2,NN);
  if(angle <= -1.0)
    angle = -AY_PI;
  else
    if(angle >= 1.0)
      angle = 1.0;
    else
      angle = acos(angle);

  if(fabs(angle) > AY_D2R(45))
    {
      NN[0] = N1[0]+(N2[0]-N1[0])*0.5;
      NN[1] = N1[1]+(N2[1]-N1[1])*0.5;
      NN[2] = N1[2]+(N2[2]-N1[2])*0.5;
      return;
    }

 return;
} /* ay_tess_checknormal */


/** ay_tess_quadrangulate:
 * Refine a triangular polymesh, stemming from a NURBS surface tesselation
 * to triangles, by inserting four new control points for each original
 * triangle and creating three quads for each original triangle.
 *
 * \param[in] o  polymesh object (containing only triangles) to quadrangulate
 * \param[in] np  NURBS surface that was tesselated to \a o
 * \param[in] uv  parametric coordinates of the polymesh control points in \a o
 * \param[in] stname  name of PV tag carrying the texture coordinates,
 *                    may be NULL
 * \param[in,out] q  where to store the new polymesh object
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_tess_quadrangulate(ay_object *o, ay_nurbpatch_object *np, double *uv,
		      char *stname, ay_object **q)
{
 int ay_status = AY_OK;
 /*char fname[] = "tess_quadrangulate";*/
 int tsize;
 unsigned int i, j, a, b, tco;
 unsigned int quads[] = {
   0, 1, 6, 5,
   1, 2, 3, 6,
   3, 4, 5, 6
 };
 ay_object *newo = NULL;
 ay_tag *tag;
 ay_pomesh_object *po;
 ay_pomesh_object *npo = NULL;
 double *p1, *p2, *p3;
 double uv12[2], uv23[2], uv31[2], uvbc[2];
 double p12[6], p23[6], p31[6], pbc[6];
 double *t = NULL, umin, umax, vmin, vmax, ud, vd;
 float *tc = NULL, *ntc = NULL;
 int isdegen[3] = {0}, checkdegen = AY_TRUE;
 int checknormal = AY_TRUE;
 int have_tc = AY_FALSE;
 char *myst = ay_prefs.texcoordname;

  if(!o || !np || !uv || !q)
    return AY_ENULL;

  if(o->type != AY_IDPOMESH)
    return AY_EWTYPE;
  po = (ay_pomesh_object*)o->refine;

  if(stname)
    myst = stname;

  tag = o->tags;
  while(tag)
    {
      if(ay_pv_checkndt(tag, myst, "varying", "g"))
	{
	  if(tc)
	    {
	      free(tc);
	      tc = NULL;
	    }
	  ay_status = ay_pv_convert(tag, 1, NULL,
				    (void**)(void*)&tc);
	  /* XXXX TODO check tc length?*/
	  /*if(tclength<)*/

	  if(!ay_status && tc)
	    have_tc = AY_TRUE;
	}

      tag = tag->next;
    }

  tsize = ay_nb_FirstDerSurf4DMSize(np->uorder-1, np->vorder-1);
  if(!(t = malloc(tsize*sizeof(double))))
    return AY_EOMEM;

  if(!(newo = calloc(1, sizeof(ay_object))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  ay_object_defaults(newo);
  newo->type = AY_IDPOMESH;

  if(!(npo = calloc(1, sizeof(ay_pomesh_object))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  npo->npolys = po->npolys*3;

  if(!(npo->nloops = malloc(npo->npolys * sizeof(unsigned int))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  for(i = 0; i < npo->npolys; i++)
    npo->nloops[i] = 1;

  if(!(npo->nverts = malloc(npo->npolys * sizeof(unsigned int))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  for(i = 0; i < npo->npolys; i++)
    npo->nverts[i] = 4;

  if(!(npo->verts = malloc(npo->npolys * 4 * sizeof(unsigned int))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  npo->ncontrols = po->npolys * 7;
  npo->has_normals = AY_TRUE;
  if(!(npo->controlv = malloc(npo->ncontrols * 6 * sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }

  if(have_tc)
    {
      if(!(ntc = malloc(npo->ncontrols * 2 * sizeof(float))))
	{
	  ay_status = AY_EOMEM;
	  goto cleanup;
	}
      tco = 0;
    }

  a = 0;
  b = 0;
  for(i = 0; i < po->npolys; i++)
    {
      p1 = &(uv[po->verts[i*3]*2]);
      p2 = &(uv[po->verts[i*3+1]*2]);
      p3 = &(uv[po->verts[i*3+2]*2]);

      if(checkdegen)
	{
	  umin = DBL_MAX;
	  if(p1[0] < umin)
	    umin = p1[0];
	  if(p2[0] < umin)
	    umin = p2[0];
	  if(p3[0] < umin)
	    umin = p3[0];

	  umax = -DBL_MAX;
	  if(p1[0] > umax)
	    umax = p1[0];
	  if(p2[0] > umax)
	    umax = p2[0];
	  if(p3[0] > umax)
	    umax = p3[0];

	  vmin = DBL_MAX;
	  if(p1[1] < vmin)
	    vmin = p1[1];
	  if(p2[1] < vmin)
	    vmin = p2[1];
	  if(p3[1] < vmin)
	    vmin = p3[1];

	  vmax = -DBL_MAX;
	  if(p1[1] > vmax)
	    vmax = p1[1];
	  if(p2[1] > vmax)
	    vmax = p2[1];
	  if(p3[1] > vmax)
	    vmax = p3[1];

	  ud = umax-umin;
	  vd = vmax-vmin;

	  isdegen[0] = ay_tess_isdegen(&(po->controlv[po->verts[i*3]*6]),
				       np, p1[0], p1[1], ud, vd);

	  isdegen[1] = ay_tess_isdegen(&(po->controlv[po->verts[i*3+1]*6]),
				       np, p2[0], p2[1], ud, vd);

	  isdegen[2] = ay_tess_isdegen(&(po->controlv[po->verts[i*3+2]*6]),
				       np, p3[0], p3[1], ud, vd);
	} /* if */

      /* output p1 */
      memcpy(&(npo->controlv[a]), &(po->controlv[po->verts[i*3]*6]),
	     6*sizeof(double));
      a += 6;
      if(have_tc)
	{
	  memcpy(&(ntc[tco]), &(tc[po->verts[i*3]*2]), 2*sizeof(float));
	  tco += 2;
	}

      /* synthesize and output p12 */
      if(isdegen[0] == 1)
	uv12[0] = p2[0];
      else
	if(isdegen[1] == 1)
	  uv12[0] = p1[0];
	else
	  /*
	  if(fabs(p1[0]-p2[0])<AY_EPSILON)
	    uv12[0] = p1[0];
	  else
	  */
	    uv12[0] = p1[0]+(p2[0]-p1[0])*0.5;
      if(isdegen[0] == 2)
	uv12[1] = p2[1];
      else
	if(isdegen[1] == 2)
	  uv12[1] = p1[1];
	else
	  /*
	  if(fabs(p1[1]-p2[1])<AY_EPSILON)
	    uv12[1] = p1[1];
	  else
	  */
	    uv12[1] = p1[1]+(p2[1]-p1[1])*0.5;
      memset(t, 0, tsize*sizeof(double));
      ay_tess_surfacepoint(np, uv12[0], uv12[1], t, p12);
      if(checknormal)
	ay_tess_checknormal(&(po->controlv[po->verts[i*3]*6+3]),
			    &(po->controlv[po->verts[i*3+1]*6+3]),
			    &(p12[3]));
      memcpy(&(npo->controlv[a]), p12, 6*sizeof(double));
      a += 6;

      if(have_tc)
	{
	  ntc[tco] = tc[po->verts[i*3]*2] +
	    (tc[po->verts[i*3+1]*2] - tc[po->verts[i*3]*2])*0.5;
	  tco++;
	  ntc[tco] = tc[po->verts[i*3]*2+1] +
	    (tc[po->verts[i*3+1]*2+1] - tc[po->verts[i*3]*2+1])*0.5;
	  tco++;
	}
      /* end p12 */

      /* output p2 */
      memcpy(&(npo->controlv[a]), &(po->controlv[po->verts[i*3+1]*6]),
	     6*sizeof(double));
      a += 6;
      if(have_tc)
	{
	  memcpy(&(ntc[tco]), &(tc[po->verts[i*3+1]*2]), 2*sizeof(float));
	  tco += 2;
	}

      /* synthesize and output p23 */
      if(isdegen[1] == 1)
	uv23[0] = p3[0];
      else
	if(isdegen[2] == 1)
	  uv23[0] = p2[0];
	else
	  /*
	  if(fabs(p2[0]-p3[0])<AY_EPSILON)
	    uv23[0] = p2[0];
	  else
	  */
	    uv23[0] = p2[0]+(p3[0]-p2[0])*0.5;
      if(isdegen[1] == 2)
	uv23[1] = p3[1];
      else
	if(isdegen[2] == 2)
	  uv23[1] = p2[1];
	else
	  /*
	  if(fabs(p2[1]-p3[1])<AY_EPSILON)
	    uv23[1] = p2[1];
	  else
	  */
	    uv23[1] = p2[1]+(p3[1]-p2[1])*0.5;
      memset(t, 0, tsize*sizeof(double));
      ay_tess_surfacepoint(np, uv23[0], uv23[1], t, p23);
      if(checknormal)
	ay_tess_checknormal(&(po->controlv[po->verts[i*3+1]*6+3]),
			    &(po->controlv[po->verts[i*3+2]*6+3]),
			    &(p23[3]));
      memcpy(&(npo->controlv[a]), p23, 6*sizeof(double));
      a += 6;
      if(have_tc)
	{
	  ntc[tco] = tc[po->verts[i*3+1]*2]+
	    (tc[po->verts[i*3+2]*2]-tc[po->verts[i*3+1]*2])*0.5;
	  tco++;
	  ntc[tco] = tc[po->verts[i*3+1]*2+1]+
	    (tc[po->verts[i*3+2]*2+1]-tc[po->verts[i*3+1]*2+1])*0.5;
	  tco++;
	}
      /* end p23 */

      /* output p3 */
      memcpy(&(npo->controlv[a]), &(po->controlv[po->verts[i*3+2]*6]),
	     6*sizeof(double));
      a += 6;
      if(have_tc)
	{
	  memcpy(&(ntc[tco]), &(tc[po->verts[i*3+2]*2]), 2*sizeof(float));
	  tco += 2;
	}

      /* synthesize and output p31 */
      if(isdegen[2] == 1)
	uv31[0] = p1[0];
      else
	if(isdegen[0] == 1)
	  uv31[0] = p3[0];
	else
	  /*
	  if(fabs(p1[0]-p3[0])<AY_EPSILON)
	    uv31[0] = p1[0];
	  else
	  */
	    uv31[0] = p1[0]+(p3[0]-p1[0])*0.5;
      if(isdegen[2] == 2)
	uv31[1] = p1[1];
      else
	if(isdegen[0] == 2)
	  uv31[1] = p3[1];
	else
	  /*
	  if(fabs(p1[1]-p3[1])<AY_EPSILON)
	    uv31[1] = p1[1];
	  else
	  */
	    uv31[1] = p1[1]+(p3[1]-p1[1])*0.5;
      memset(t, 0, tsize*sizeof(double));
      ay_tess_surfacepoint(np, uv31[0], uv31[1], t, p31);
      if(checknormal)
	ay_tess_checknormal(&(po->controlv[po->verts[i*3+2]*6+3]),
			    &(po->controlv[po->verts[i*3]*6+3]),
			    &(p31[3]));
      memcpy(&(npo->controlv[a]), p31, 6*sizeof(double));
      a += 6;
      if(have_tc)
	{
	  ntc[tco] = tc[po->verts[i*3+2]*2] +
	    (tc[po->verts[i*3]*2] - tc[po->verts[i*3+2]*2])*0.5;
	  tco++;
	  ntc[tco] = tc[po->verts[i*3+2]*2+1] +
	    (tc[po->verts[i*3]*2+1] - tc[po->verts[i*3+2]*2+1])*0.5;
	  tco++;
	}
      /* end p31 */

      /* synthesize and output middle point */
      uvbc[0] = p1[0]*1.0/3.0 + p2[0]*1.0/3.0 + p3[0]*1.0/3.0;
      uvbc[1] = p1[1]*1.0/3.0 + p2[1]*1.0/3.0 + p3[1]*1.0/3.0;
      memset(t, 0, tsize*sizeof(double));
      ay_tess_surfacepoint(np, uvbc[0], uvbc[1], t, pbc);
      memcpy(&(npo->controlv[a]), pbc, 6*sizeof(double));
      a += 6;
      if(have_tc)
	{
	  ntc[tco] = tc[po->verts[i*3]*2]   * 1.0/3.0 +
	             tc[po->verts[i*3+1]*2] * 1.0/3.0 +
	             tc[po->verts[i*3+2]*2] * 1.0/3.0;
	  tco++;
	  ntc[tco] = tc[po->verts[i*3]*2+1]   * 1.0/3.0 +
	             tc[po->verts[i*3+1]*2+1] * 1.0/3.0 +
	             tc[po->verts[i*3+2]*2+1] * 1.0/3.0;
	  tco++;
	}
      /* end middle point */

      /* emit quad indices */
      memcpy(&(npo->verts[b]), quads, 12*sizeof(unsigned int));
      b += 12;

      /* adjust quad indices for next iteration */
      for(j = 0; j < 12; j++)
	quads[j] += 7;
    } /* for */

  if(have_tc)
    ay_pv_add(newo, myst, "varying", "g", npo->ncontrols, 2, ntc);

  /* return result */
  newo->refine = npo;
  *q = newo;

  /* prevent cleanup from doing something harmful */
  npo = NULL;
  newo = NULL;
  ntc = NULL;

cleanup:

  if(t)
    free(t);

  if(npo)
    ay_pomesht_destroy(npo);

  if(newo)
    free(newo);

  if(tc)
    free(tc);

  if(ntc)
    free(ntc);

 return ay_status;
} /* ay_tess_quadrangulate */
