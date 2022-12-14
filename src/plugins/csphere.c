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

/* csphere.c - csphere custom object */

static char *csphere_name = "CSphere";

static unsigned int csphere_id;

typedef struct csphere_object_s
{
  char closed;
  char is_simple;
  double radius;
  double zmin, zmax;
  double thetamax;
} csphere_object;


static char csphere_version_ma[] = AY_VERSIONSTR;
static char csphere_version_mi[] = AY_VERSIONSTRMI;

#ifdef WIN32
  __declspec (dllexport)
#endif /* WIN32 */
int Csphere_Init(Tcl_Interp *interp);


/* csphere_createcb:
 *  create callback function of csphere object
 */
int
csphere_createcb(int argc, char *argv[], ay_object *o)
{
 csphere_object *csphere = NULL;
 char fname[] = "crtcsphere";

  if(!o)
    return AY_ENULL;

  if(!(csphere = calloc(1, sizeof(csphere_object))))
    {
      ay_error(AY_EOMEM, fname, NULL);
      return AY_ERROR;
    }

  csphere->closed = AY_TRUE;
  csphere->is_simple = AY_TRUE;
  csphere->radius = 1.0;
  csphere->zmin = -1.0;
  csphere->zmax = 1.0;
  csphere->thetamax = 360.0;

  o->refine = csphere;

 return AY_OK;
} /* csphere_createcb */


/* csphere_deletecb:
 *  delete callback function of csphere object
 */
int
csphere_deletecb(void *c)
{
 csphere_object *csphere = NULL;

  if(!c)
    return AY_ENULL;

  csphere = (csphere_object *)(c);

  free(csphere);

 return AY_OK;
} /* csphere_deletecb */


/* csphere_copycb:
 *  copy callback function of csphere object
 */
int
csphere_copycb(void *src, void **dst)
{
 csphere_object *csphere = NULL;

  if(!src || !dst)
    return AY_ENULL;

  if(!(csphere = calloc(1, sizeof(csphere_object))))
    return AY_EOMEM;

  memcpy(csphere, src, sizeof(csphere_object));

  *dst = (void *)csphere;

 return AY_OK;
} /* csphere_copycb */


/* csphere_drawcb:
 *  draw (display in an Ayam view window) callback function of csphere object
 */
int
csphere_drawcb(struct Togl *togl, ay_object *o)
{
 csphere_object *csphere = NULL;
 double phimax, phimin, phi, rmax, rmin, thetamax, zmin, zmax, radius;
 double phidiff, thetadiff, angle;
 int i, j, k;
 double P1[15] = {0}, P2[135] = {0}, R[5];
 GLdouble w = 0.0;

  if(!o)
    return AY_ENULL;

  csphere = (csphere_object *)o->refine;

  if(!csphere)
    return AY_ENULL;

  radius = csphere->radius;

  /* check, if csphere is simple */
  if(csphere->is_simple)
    {
      /* yes, it is */

      w = (GLdouble)(sqrt(2.0)*0.5);

      /* draw */
      /* xy-plane-ring */
      glBegin(GL_LINE_LOOP);
      glVertex3d(radius,0.0,0.0);
      glVertex3d(radius*w,-radius*w,0.0);
      glVertex3d(0.0,-radius,0.0);
      glVertex3d(-radius*w,-radius*w,0.0);
      glVertex3d(-radius,0.0,0.0);
      glVertex3d(-radius*w,radius*w,0.0);
      glVertex3d(0.0,radius,0.0);
      glVertex3d(radius*w,radius*w,0.0);
      glEnd();
      /* xz-plane-ring */
      glBegin(GL_LINE_LOOP);
      glVertex3d(radius,0.0,0.0);
      glVertex3d(radius*w,0.0,-radius*w);
      glVertex3d(0.0,0.0,-radius);
      glVertex3d(-radius*w,0.0,-radius*w);
      glVertex3d(-radius,0.0,0.0);
      glVertex3d(-radius*w,0.0,radius*w);
      glVertex3d(0.0,0.0,radius);
      glVertex3d(radius*w,0.0,radius*w);
      glEnd();
      /* yz-plane-ring */
      glBegin(GL_LINE_LOOP);
      glVertex3d(0.0,radius,0.0);
      glVertex3d(0.0,radius*w,-radius*w);
      glVertex3d(0.0,0.0,-radius);
      glVertex3d(0.0,-radius*w,-radius*w);
      glVertex3d(0.0,-radius,0.0);
      glVertex3d(0.0,-radius*w,radius*w);
      glVertex3d(0.0,0.0,radius);
      glVertex3d(0.0,radius*w,radius*w);
      glEnd();

      return AY_OK; /* XXXX early exit! */
    } /* if */


  zmin = csphere->zmin;
  zmax = csphere->zmax;
  thetamax = csphere->thetamax;

  if(fabs(zmax) < radius)
    {
      rmax = sqrt(radius*radius-zmax*zmax);
      phimax = asin(zmax/radius);
    }
  else
    {
      rmax = 0.0;
      phimax = AY_HALFPI;
    }

  if(fabs(zmin) < radius)
    {
      rmin = sqrt(radius*radius-zmin*zmin);
      phimin = asin(zmin/radius);
    }
  else
    {
      rmin = 0.0;
      phimin = -AY_HALFPI;
    }

  phi = fabs(phimax) + fabs(phimin);

  phidiff = phi/4;
  thetadiff = AY_D2R(thetamax/8);

  angle = phimin;
  for(j = 0; j <= 4; j++)
    {
      P1[j*3] = cos(angle)*radius;
      P1[j*3+2] = sin(angle)*radius;
      R[j] = sqrt(radius*radius-(sin(angle)*radius)*(sin(angle)*radius));
      angle += phidiff;
    }

  angle = 0;
  for(i = 0; i <= 8; i++)
    {
      memcpy(&(P2[i*3*5]), P1, 5*3*sizeof(double));
      k = i*3*5;

      for(j = 0; j < 5; j++)
	{
	  P2[k+(j*3)] *= cos(angle);
	  P2[k+(j*3)+1] = sin(angle) * R[j];
	}
      angle += thetadiff;
    }


  /* draw */
  for(i = 0; i < 9; i++)
    {
      k = i*3*5;
      glBegin(GL_LINE_STRIP);
      for(j = 0; j < 5; j++)
	{
	  glVertex3dv(&(P2[k+(3*j)]));
	}
      glEnd();
    }

  for(j = 0; j < 5; j++)
    {
      k = j*3;
      glBegin(GL_LINE_STRIP);
      for(i = 0; i < 9; i++)
	{
	  glVertex3dv(&(P2[k+(i*3*5)]));
	}
      glEnd();
    }

  if(csphere->closed)
    {
      if(fabs(thetamax) != 360.0)
	{
	  glBegin(GL_LINE_STRIP);
	   glVertex3d(rmin, 0.0, zmin);
	   glVertex3d(0.0,  0.0, zmin);
	   glVertex3d(0.0,  0.0, zmax);
	   glVertex3d(rmax, 0.0, zmax);
	  glEnd();
	  glRotated(thetamax, 0.0, 0.0, 1.0);
	  glBegin(GL_LINES);
	   glVertex3d(rmin, 0.0, zmin);
	   glVertex3d(0.0,  0.0, zmin);
	   glVertex3d(rmax, 0.0, zmax);
	   glVertex3d(0.0,  0.0, zmax);
	  glEnd();
	}
    }

 return AY_OK;
} /* csphere_drawcb */


/* csphere_shadecb:
 *  shade (display in an Ayam view window) callback function of csphere object
 */
int
csphere_shadecb(struct Togl *togl, ay_object *o)
{
 csphere_object *csphere = NULL;
 GLUquadricObj *qobj = NULL;
 double phimax, phimin, phi, rmax, rmin, thetamax, zmin, zmax, radius;
 double phidiff, thetadiff, angle;
 int i, j, k;
 double P1[15] = {0}, P2[135] = {0}, R[5];

  if(!o)
    return AY_ENULL;

  csphere = (csphere_object *)o->refine;

  if(!csphere)
    return AY_ENULL;

  radius = csphere->radius;

  /* check, if csphere is simple */
  if(csphere->is_simple)
    {
      /* yes, it is */

      if(!(qobj = gluNewQuadric()))
	return AY_EOMEM;

      /* draw */
      gluSphere(qobj,radius,10,10);

      gluDeleteQuadric(qobj);

      return AY_OK;
    }

  zmin = csphere->zmin;
  zmax = csphere->zmax;
  thetamax = csphere->thetamax;

  if(fabs(zmax) < radius)
    {
      rmax = sqrt(radius*radius-zmax*zmax);
      phimax = asin(zmax/radius);
    }
  else
    {
      rmax = 0.0;
      phimax = AY_HALFPI;
    }

  if(fabs(zmin) < radius)
    {
      rmin = sqrt(radius*radius-zmin*zmin);
      phimin = asin(zmin/radius);
    }
  else
    {
      rmin = 0.0;
      phimin = -AY_HALFPI;
    }

  phi = fabs(phimax) + fabs(phimin);
  phidiff = phi/4;
  thetadiff = thetamax/8;

  angle = phimin;
  for(j = 0; j <= 4; j++)
    {
      P1[j*3] = cos(angle)*radius;
      P1[j*3+2] = sin(angle)*radius;
      R[j] = sqrt(radius*radius-(sin(angle)*radius)*(sin(angle)*radius));
      angle += phidiff;
    }

  angle = 0;
  for(i = 0; i <= 8; i++)
    {
      memcpy(&(P2[i*3*5]), P1, 5*3*sizeof(double));
      k = i*3*5;

      for(j = 0; j <= 4; j++)
	{

	  P2[k+(j*3)] *= cos(angle);
	  P2[k+(j*3)+1] = sin(angle) * R[j];
	}

      angle += AY_D2R(thetadiff);
    }

  /* draw */
  for(i = 0; i < 8; i++)
    {
      k = i*3*5;
      glBegin(GL_QUAD_STRIP);
      for(j = 0; j < 5; j++)
	{
	  glNormal3dv(&(P2[k+(3*j)]));
	  glVertex3dv(&(P2[k+(3*j)]));
	  glNormal3dv(&(P2[k+(3*5)+(3*j)]));
	  glVertex3dv(&(P2[k+(3*5)+(3*j)]));
	}
      glEnd();
    }

  if(csphere->closed)
    {
      if(fabs(thetamax) != 360.0)
	{
	  glPushMatrix();
	  glNormal3d(0.0,-1.0,0.0);
	  glBegin(GL_TRIANGLE_FAN);
	   glVertex3d(0.0,  0.0, zmin);
	   for(i = 0; i < 5; i++)
	     {
	       glVertex3dv(&(P1[i*3]));
	     }
	  glEnd();
	  glBegin(GL_TRIANGLES);
	   glVertex3d(0.0,  0.0, zmin);
	   glVertex3d(rmax, 0.0, zmax);
	   glVertex3d(0.0,  0.0, zmax);
	  glEnd();


	  glRotated(thetamax, 0.0, 0.0, 1.0);

	  glBegin(GL_TRIANGLE_FAN);
	   glVertex3d(0.0,  0.0, zmin);
	   for(i = 0; i < 5; i++)
	     {
	       glVertex3dv(&(P1[i*3]));
	     }
	  glEnd();
	  glBegin(GL_TRIANGLES);
	   glVertex3d(0.0,  0.0, zmin);
	   glVertex3d(rmax, 0.0, zmax);
	   glVertex3d(0.0,  0.0, zmax);
	  glEnd();

	  glPopMatrix();

	}

      /* draw caps */
      if(rmin != 0.0)
	{
	  glPushMatrix();

	  qobj = NULL;
	  if(!(qobj = gluNewQuadric()))
	    return AY_EOMEM;
	  glTranslated(0.0,0.0,zmin);
	  if(fabs(thetamax) != 360.0)
	    {
	      glRotated(thetamax-90.0, 0.0, 0.0, 1.0);
	      gluPartialDisk(qobj, 0.0, rmin, 8, 1, 0.0, thetamax);
	    }
	  else
	    {
	      gluDisk(qobj, 0.0, rmin, 8, 1);
	    }
	  gluDeleteQuadric(qobj);

	  glPopMatrix();
	}

      if(rmax != 0.0)
	{
	  glPushMatrix();

	  qobj = NULL;
	  if(!(qobj = gluNewQuadric()))
	    return AY_EOMEM;
	  glTranslated(0.0, 0.0, zmax);
	  if(fabs(thetamax) != 360.0)
	    {
	      glRotated(thetamax-90.0, 0.0, 0.0, 1.0);
	      gluPartialDisk(qobj, 0.0, rmax, 8, 1, 0.0, thetamax);
	    }
	  else
	    {
	      gluDisk(qobj, 0.0, rmax, 8, 1);
	    }
	  gluDeleteQuadric(qobj);

	  glPopMatrix();
	}
    }

 return AY_OK;
} /* csphere_shadecb */


/* csphere_setpropcb:
 *  set property (from Tcl to C context) callback function of csphere object
 */
int
csphere_setpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
 char *arr = "CSphereAttrData";
 Tcl_Obj *to = NULL;
 csphere_object *csphere = NULL;
 int itemp = 0;

  if(!o)
    return AY_ENULL;

  csphere = (csphere_object *)o->refine;

  if(!csphere)
    return AY_ENULL;

  if((to = Tcl_GetVar2Ex(interp, arr, "Closed",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    {
      Tcl_GetIntFromObj(interp, to, &itemp);
      csphere->closed = (char)itemp;
    }

  if((to = Tcl_GetVar2Ex(interp, arr, "Radius",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetDoubleFromObj(interp, to, &csphere->radius);

  if((to = Tcl_GetVar2Ex(interp, arr, "ZMin",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetDoubleFromObj(interp, to, &csphere->zmin);

  if((to = Tcl_GetVar2Ex(interp, arr, "ZMax",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetDoubleFromObj(interp, to, &csphere->zmax);

  if((to = Tcl_GetVar2Ex(interp, arr, "ThetaMax",
			 TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY)))
    Tcl_GetDoubleFromObj(interp, to, &csphere->thetamax);

  if((fabs(csphere->zmin) == csphere->radius) &&
     (fabs(csphere->zmax) == csphere->radius) &&
     (fabs(csphere->thetamax) == 360.0))
    {
      csphere->is_simple = AY_TRUE;
    }
  else
    {
      csphere->is_simple = AY_FALSE;
    }

 return AY_OK;
} /* csphere_setpropcb */


/* csphere_getpropcb:
 *  get property (from C to Tcl context) callback function of csphere object
 */
int
csphere_getpropcb(Tcl_Interp *interp, int argc, char *argv[], ay_object *o)
{
 char *arr = "CSphereAttrData";
 csphere_object *csphere = NULL;

  if(!o)
    return AY_ENULL;

  csphere = (csphere_object *)(o->refine);

  if(!csphere)
    return AY_ENULL;

  Tcl_SetVar2Ex(interp, arr, "Closed",
		Tcl_NewIntObj(csphere->closed),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "Radius",
		Tcl_NewDoubleObj(csphere->radius),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "ZMin",
		Tcl_NewDoubleObj(csphere->zmin),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "ZMax",
		Tcl_NewDoubleObj(csphere->zmax),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

  Tcl_SetVar2Ex(interp, arr, "ThetaMax",
		Tcl_NewDoubleObj(csphere->thetamax),
		TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);

 return AY_OK;
} /* csphere_getpropcb */


/* csphere_readcb:
 *  read (from scene file) callback function of csphere object
 */
int
csphere_readcb(FILE *fileptr, ay_object *o)
{
 csphere_object *csphere = NULL;
 int itemp = 0;

  if(!o)
    return AY_ENULL;

  if(!(csphere = calloc(1, sizeof(csphere_object))))
    return AY_EOMEM;

  fscanf(fileptr, "%d\n", &itemp);
  csphere->closed = (char)itemp;
  fscanf(fileptr, "%lg\n", &csphere->radius);
  fscanf(fileptr, "%lg\n", &csphere->zmin);
  fscanf(fileptr, "%lg\n", &csphere->zmax);
  fscanf(fileptr, "%lg\n", &csphere->thetamax);

  if((fabs(csphere->zmin) == csphere->radius) &&
     (fabs(csphere->zmax) == csphere->radius) &&
     (fabs(csphere->thetamax) == 360.0))
    {
      csphere->is_simple = AY_TRUE;
    }
  else
    {
      csphere->is_simple = AY_FALSE;
    }

  o->refine = csphere;

 return AY_OK;
} /* csphere_readcb */


/* csphere_writecb:
 *  write (to scene file) callback function of csphere object
 */
int
csphere_writecb(FILE *fileptr, ay_object *o)
{
 csphere_object *csphere = NULL;

  if(!o)
    return AY_ENULL;

  csphere = (csphere_object *)(o->refine);

  if(!csphere)
    return AY_ENULL;

  fprintf(fileptr, "%d\n", csphere->closed);
  fprintf(fileptr, "%g\n", csphere->radius);
  fprintf(fileptr, "%g\n", csphere->zmin);
  fprintf(fileptr, "%g\n", csphere->zmax);
  fprintf(fileptr, "%g\n", csphere->thetamax);

 return AY_OK;
} /* csphere_writecb */


/* csphere_wribcb:
 *  RIB export callback function of csphere object
 *  (code taken from Affine)
 */
int
csphere_wribcb(char *file, ay_object *o)
{
 csphere_object *csphere = NULL;
 RtPoint  P1[16];
 RtPoint  P2[16];
 RtFloat rmin, rmax;
 RtFloat xmid, zmid;
 RtFloat angle, circle;
 RtFloat phimax, phimin, phidiff;
 RtFloat radius, zmin, zmax, thetamax;

#define X 0
#define Y 1
#define Z 2
#define UNITCIRCLE (0.5522847)

  if(!o)
    return AY_ENULL;

  csphere = (csphere_object*)o->refine;

  if(!csphere)
    return AY_ENULL;

  if(!csphere->closed)
  {
    RiSphere((RtFloat)csphere->radius,
	     (RtFloat)csphere->zmin,
	     (RtFloat)csphere->zmax,
	     (RtFloat)csphere->thetamax,
	     NULL);
  }
  else
  {
    radius = (RtFloat)csphere->radius;
    zmin = (RtFloat)csphere->zmin;
    zmax = (RtFloat)csphere->zmax;
    thetamax = (RtFloat)csphere->thetamax;


    if(radius == 0.0)
      return AY_OK;

    RiSphere( radius, zmin, zmax, thetamax, NULL );

    /* Top if needed. */
    if ( fabs(zmax) < radius )
      {
	/* Refer to [UPST90]. */
	rmax = (RtFloat)sqrt(radius*radius-zmax*zmax);
	RiDisk( zmax, rmax, thetamax, NULL );

	phimax = (RtFloat)asin(zmax/radius);
      }
    else
      {
	rmax = (RtFloat)0.0;
	phimax = (RtFloat)AY_HALFPI;
      }

    /* Bottom if needed. */
    if ( fabs(zmin) < radius )
      {
	RiAttributeBegin();
	 RiReverseOrientation();
	 /* Refer to [UPST90]. */
	 rmin = (RtFloat)sqrt(radius*radius-zmin*zmin);
	 RiDisk( zmin, rmin, thetamax, NULL);
	RiAttributeEnd();

	phimin = (RtFloat)asin(zmin/radius);
      }
    else
      {
	rmin = (RtFloat)0.0;
	phimin = (RtFloat)-AY_HALFPI;
      }

    /* Don't add patches for wedge shape if they are not needed. */
    if (thetamax != 360.0)
      {
	phidiff = phimax - phimin;
	zmid = (RtFloat)sin( phimin + phidiff/2.0 );
	xmid = (RtFloat)sqrt(radius*radius-zmid*zmid);

	/* The Y coordinates need to be zero, so just clear everything. */
	memset( P1, 0, 16*sizeof(RtPoint) );
	memset( P2, 0, 16*sizeof(RtPoint) );

	/* Calculate the patch from bottom to middle. */
	P1[0][Z] = P1[1][Z] = P1[2][Z] = P1[3][Z] = zmin; /* height */
	P1[0][X] = (RtFloat)0.0;
	P1[3][X] = (RtFloat)rmin; /* radius */
	P1[1][X] = (RtFloat)(P1[3][X]/3.0);
	P1[2][X] = (RtFloat)(2*P1[1][X]);

	P1[12][Z] = P1[13][Z] = P1[14][Z] = P1[15][Z] = zmid; /* middle's
								 height */
	P1[12][X] = (RtFloat)0.0;
	P1[15][X] = xmid;
	P1[13][X] = (RtFloat)(P1[15][X]/3.0);
	P1[14][X] = (RtFloat)(2*P1[13][X]);

	angle = (RtFloat)(phimin + AY_HALFPI);
	circle = (RtFloat)((UNITCIRCLE/AY_PI)*radius*phidiff);

	P1[4][Z] = P1[5][Z] = P1[6][Z] = P1[7][Z] = (RtFloat)(zmin +
						      circle*sin(angle));
	P1[4][X] = (RtFloat)0.0;
	P1[7][X] = (RtFloat)(rmin + circle*cos(angle));
	P1[5][X] = (RtFloat)(P1[7][X]/3.0);
	P1[6][X] = (RtFloat)(2*P1[5][X]);

	angle = (RtFloat)(phimin + phidiff/2.0 - AY_HALFPI);
	P1[8][Z]  = P1[9][Z] = P1[10][Z] = P1[11][Z] = (RtFloat)(zmid +
							 circle*sin(angle));
	P1[8][X]  = (RtFloat)0.0;
	P1[11][X] = (RtFloat)(xmid + circle*cos(angle));
	P1[9][X]  = (RtFloat)(P1[11][X]/3.0);
	P1[10][X] = (RtFloat)(2*P1[9][X]);


	/* Calculate the patch from equator to top. */
	P2[0][Z] = P2[1][Z] = P2[2][Z] = P2[3][Z] = zmid; /* middle's height */
	P2[0][X] = (RtFloat)0.0;
	P2[3][X] = xmid;
	P2[1][X] = (RtFloat)(P2[3][X]/3.0);
	P2[2][X] = (RtFloat)(2*P2[1][X]);

	P2[12][Z] = P2[13][Z] = P2[14][Z] = P2[15][Z] = zmax; /* height */
	P2[12][X] = (RtFloat)0.0;
	P2[15][X] = rmax; /* Radius at zmax. */
	P2[13][X] = (RtFloat)(P2[15][X]/3.0);
	P2[14][X] = (RtFloat)(2*P2[13][X]);

	angle = (RtFloat)(phimin + phidiff/2.0 + AY_HALFPI);
	circle = (RtFloat)((UNITCIRCLE/AY_PI)*radius*phidiff);

	P2[4][Z] = P2[5][Z] = P2[6][Z] = P2[7][Z] = (RtFloat)(zmid +
						      circle*sin(angle));
	P2[4][X] = (RtFloat)0.0;
	P2[7][X] = (RtFloat)(xmid + circle*cos(angle));
	P2[5][X] = (RtFloat)(P2[7][X]/3.0);
	P2[6][X] = (RtFloat)(2*P2[5][X]);

	angle = (RtFloat)(phimin + phidiff - AY_HALFPI);
	P2[8][Z] = P2[9][Z] = P2[10][Z] = P2[11][Z] = (RtFloat)(zmax +
							circle*sin(angle));
	P2[8][X]  = (RtFloat)0.0;
	P2[11][X] = (RtFloat)(rmax + circle*cos(angle));
	P2[9][X]  = (RtFloat)(P2[11][X]/3.0);
	P2[10][X] = (RtFloat)(2*P2[9][X]);

	RiPatch( RI_BICUBIC, RI_P, (RtPointer)P1, NULL );
	RiPatch( RI_BICUBIC, RI_P, (RtPointer)P2, NULL );

	RiAttributeBegin();
	 RiRotate( thetamax, 0, 0, 1 );
	 RiReverseOrientation();
	 RiPatch( RI_BICUBIC, RI_P, (RtPointer)P1, NULL );
	 RiPatch( RI_BICUBIC, RI_P, (RtPointer)P2, NULL );
	RiAttributeEnd();
      }
  }

#undef X
#undef Y
#undef Z
#undef UNITCIRCLE

 return AY_OK;
} /* csphere_wribcb */


/* csphere_bbccb:
 *  bounding box calculation callback function of csphere object
 */
int
csphere_bbccb(ay_object *o, double *bbox, int *flags)
{
 double r = 0.0, zmi = 0.0, zma = 0.0;
 csphere_object *csphere = NULL;

  if(!o || !bbox)
    return AY_ENULL;

  csphere = (csphere_object *)o->refine;

  if(!csphere)
    return AY_ENULL;

  r = csphere->radius;
  zmi = csphere->zmin;
  zma = csphere->zmax;

  /* XXXX does not take into account ThetaMax! */

  /* P1 */
  bbox[0] = -r; bbox[1] = r; bbox[2] = zma;
  /* P2 */
  bbox[3] = -r; bbox[4] = -r; bbox[5] = zma;
  /* P3 */
  bbox[6] = r; bbox[7] = -r; bbox[8] = zma;
  /* P4 */
  bbox[9] = r; bbox[10] = r; bbox[11] = zma;

  /* P5 */
  bbox[12] = -r; bbox[13] = r; bbox[14] = zmi;
  /* P6 */
  bbox[15] = -r; bbox[16] = -r; bbox[17] = zmi;
  /* P7 */
  bbox[18] = r; bbox[19] = -r; bbox[20] = zmi;
  /* P8 */
  bbox[21] = r; bbox[22] = r; bbox[23] = zmi;

 return AY_OK;
} /* csphere_bbccb */


/* Csphere_Init:
 * initializes the csphere module/plugin by registering a new
 * object type (CSphere) and loading the accompanying Tcl script file.
 * Note: This function _must_ be capitalized exactly this way
 * (_C_sphere__I_nit, not csphere_init!)
 * regardless of the filename of the shared object (see: man n load)!
 */
#ifdef WIN32
  __declspec (dllexport)
#endif /* WIN32 */
int
Csphere_Init(Tcl_Interp *interp)
{
 int ay_status = AY_OK;
 char fname[] = "csphere_init";

#ifdef WIN32
  if(Tcl_InitStubs(interp, "8.2", 0) == NULL)
    {
      return TCL_ERROR;
    }
#endif /* WIN32 */

  if(ay_checkversion(fname, csphere_version_ma, csphere_version_mi))
    {
      return TCL_ERROR;
    }

  ay_status = ay_otype_register(csphere_name,
				csphere_createcb,
				csphere_deletecb,
				csphere_copycb,
				csphere_drawcb,
				NULL, /* no points to edit */
				csphere_shadecb,
				csphere_setpropcb,
				csphere_getpropcb,
				NULL, /* No Picking! */
				csphere_readcb,
				csphere_writecb,
				csphere_wribcb,
				csphere_bbccb,
				&csphere_id);

  if(ay_status)
    {
      ay_error(AY_ERROR, fname, "Error registering custom object!");
      return TCL_OK;
    }


  /* source csphere.tcl, it contains Tcl-code to build
     the CSphere-Attributes Property GUI */
  if((Tcl_EvalFile(interp, "csphere.tcl")) != TCL_OK)
     {
       ay_error(AY_ERROR, fname,
		  "Error while sourcing \"csphere.tcl\"!");
       return TCL_OK;
     }

  ay_error(AY_EOUTPUT, fname,
	   "Custom object \"CSphere\" successfully loaded.");

 return TCL_OK;
} /* Csphere_Init */

