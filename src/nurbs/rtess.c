/*
 * Ayam, a free 3D modeler for the RenderMan interface.
 *
 * Ayam is copyrighted 1998-2007 by Randolf Schultz
 * (randolf.schultz@gmail.com) and others.
 *
 * All rights reserved.
 *
 * See the file License for details.
 *
 */

#include "ayam.h"

/* rtess.c simple recursive NURB tesselators */

/*
  The tessellator is based on idea and example code from
  A. J. Chung and A. J. Field
  "A Simple Recursive Tessellator for Adaptive Surface Triangulation"
  in Journal of Graphics Tools Vol. 5, Iss. 3, 2000,
  (https://sourceforge.net/projects/emvise/).
*/

#define AY_SETTRI(tri,a1,a2,b1,b2,c1,c2) \
  {tri[0]=a1;tri[1]=a2;tri[2]=b1;tri[3]=b2;tri[4]=c1;tri[5]=c2;}

#define AY_POW 10e8

/*
#define RTESSDEBUG
*/

typedef struct rtess_div_s
{
  struct rtess_div_s *next;
  double div;
} rtess_div;

typedef struct rtess_htentry_s
{
  struct rtess_htentry_s *next;
  unsigned int index;
  double u, v;
  double P[6];
} rtess_htentry;

typedef struct rtess_hash_s
{
  unsigned int size;
  rtess_htentry **table;
} rtess_hash;

struct rtp_s;

typedef void (*ay_computesurfacefp)(struct rtp_s *tp, double *uv, int output,
				    double *res);

typedef struct rtp_s {
  double edge_thresh;
  double skew_thresh;
  double split_bias;

  ay_computesurfacefp computesurface;
  void *surface;
  ay_nurbpatch_object **npatch;

  int use_objectspace;
  double edge_thresh_u;
  double edge_thresh_v;
  int is_rat;
  double N[3];

  unsigned int outindex;

  rtess_hash surfaceHash;

  rtess_hash curveHash;


  double *coords;
  unsigned int coordslen;
  unsigned int coordspos;

  double *texcoords;
  unsigned int texcoordslen;
  unsigned int texcoordspos;

  unsigned int *indices;
  unsigned int indiceslen;
  unsigned int indicespos;
  unsigned int numtris;
} rtp;

void ay_rtess_output(rtp *tp, rtess_htentry *entry);

void ay_rtess_outputwithtex(rtp *tp, rtess_htentry *entry);

void ay_rtess_renderfinal(rtp *tp, double *tri);

void ay_rtess_cleartp(rtp *tp);


int
ay_rtess_inithash(rtess_hash *hash)
{
  hash->table = calloc(hash->size, sizeof(rtess_htentry *));
  if(hash->table)
    return AY_OK;
  else
    return AY_EOMEM;
} /* ay_rtess_inithash */


void
ay_rtess_clearhash(rtess_hash *hash)
{
 rtess_htentry **table, *entry;
 unsigned int i;

 if(hash->table)
   {
     table = hash->table;
     for(i = 0; i < hash->size; i++)
       {
	 entry = table[i];
	 while(entry)
	   {
	     table[i] = entry->next;
	     free(entry);
	     entry = table[i];
	   }
       }
     free(table);
     hash->table = NULL;
   }
 return;
} /* ay_rtess_clearhash */


void
ay_rtess_addtohash(rtess_hash *hash, unsigned int key, rtess_htentry *entry)
{
 rtess_htentry *chain;
#ifdef RTESSDEBUG
  if(key >= hash->size)
    {
      printf("addtohash with key %u for hash size %u ignored\n",key,hash->size);
      return;
    }
#endif
  chain = hash->table[key];

  if(chain)
    entry->next = chain;

  hash->table[key] = entry;

 return;
} /* ay_rtess_addtohash */


void
ay_rtess_printhash(rtess_hash *hash)
{
 rtess_htentry *entry = NULL;
 unsigned int i, j;

  if(!hash)
    return;

  for(i = 0; i < hash->size; i++)
    {
      entry = hash->table[i];
      if(entry)
	{
	  j = 0;
	  while(entry)
	    {
	      j++;
	      entry = entry->next;
	    }
	  printf("%u : %u\n",i,j);
	}
    }

 return;
} /* ay_rtess_printhash */


void
ay_rtess_cleartp(rtp *tp)
{
  if(!tp)
    return;

  ay_rtess_clearhash(&(tp->surfaceHash));

  ay_rtess_clearhash(&(tp->curveHash));

  if(tp->coords)
    free(tp->coords);
  tp->coords = NULL;

  if(tp->texcoords)
    free(tp->texcoords);
  tp->texcoords = NULL;

  if(tp->indices)
    free(tp->indices);
  tp->indices = NULL;

 return;
}



/* Compute a point on the NURBS surface. */
void
ay_rtess_computesurface(rtp *tp, double *uv, int output, double *res)
{
 double PD[12], *fdu, *fdv, *N;
 ay_nurbpatch_object *np;
 rtess_htentry *entry = NULL;
 unsigned int indu = (unsigned int)(fabs(uv[0])*AY_POW);
 unsigned int indv = (unsigned int)(fabs(uv[1])*AY_POW);
 unsigned int key;

  key = (unsigned int)((3 * indu + 5 * indv) % tp->surfaceHash.size);

  entry = (tp->surfaceHash.table)[key];
  while(entry)
    {
      if(entry->u == uv[0] && entry->v == uv[1])
	{
	  if(res)
	    {
	      memcpy(res, entry->P, 3*sizeof(double));
	    }
	  break;
	}
      entry = entry->next;
    }
  if(!entry)
    {
      if(!(entry = calloc(1, sizeof(rtess_htentry))))
	return;
      entry->u = uv[0];
      entry->v = uv[1];

      np = tp->npatch[0];
      if(np->is_rat)
	{
	  (void)ay_nb_FirstDerSurf4D(np->width-1, np->height-1,
				     np->uorder-1, np->vorder-1,
				     np->uknotv, np->vknotv,
				     np->controlv, uv[0], uv[1],
				     PD);
	}
      else
	{
	  (void)ay_nb_FirstDerSurf3D(np->width-1, np->height-1,
				     np->uorder-1, np->vorder-1,
				     np->uknotv, np->vknotv,
				     np->controlv, uv[0], uv[1],
				     PD);
	}
      memcpy(entry->P, PD, 3*sizeof(double));
      fdu = &(PD[3]);
      fdv = &(PD[6]);
      N = &(entry->P[3]);
      AY_V3CROSS(N, fdu, fdv);

      if(res)
	memcpy(res, PD, 3*sizeof(double));

      /* memoize pnt */
      ay_rtess_addtohash(&(tp->surfaceHash), key, entry);
    } /* if have memoized point */

  if(!output)
    return;

  /* output this point */
  if(!entry->index)
    ay_rtess_output(tp, entry);

 return;
} /* ay_rtess_computesurface */


void
ay_rtess_computesurfacedegen(rtp *tp, double *uv, int output, double *res)
{
 double PD[12], nuv[2], *fdu, *fdv, *N;
 ay_nurbpatch_object *np;
 rtess_htentry *entry = NULL;
 unsigned int indu = (unsigned int)(fabs(uv[0])*AY_POW);
 unsigned int indv = (unsigned int)(fabs(uv[1])*AY_POW);
 unsigned int key;
 double t;

  key = (unsigned int)((3 * indu + 5 * indv) % tp->surfaceHash.size);

  entry = (tp->surfaceHash.table)[key];
  while(entry)
    {
      if(entry->u == uv[0] && entry->v == uv[1])
	{
	  if(res)
	    {
	      memcpy(res, entry->P, 3*sizeof(double));
	    }
	  break;
	}
      entry = entry->next;
    }
  if(!entry)
    {
      if(!(entry = calloc(1, sizeof(rtess_htentry))))
	return;

      entry->u = uv[0];
      entry->v = uv[1];

      if((uv[1] > 0.9+AY_EPSILON) && (uv[1] < 1.0-AY_EPSILON))
	{
	  t = (1.0-0.9)/(1.0-uv[1]);

	  if(uv[0] > 0.5)
	    {
	      nuv[0] = 0.5+(uv[0]-0.5)*t;
	      /*
	      printf("t:%lg, %lg,%lg,%lg\n",t,uv[0],nuv[0],uv[1]);
	      */
	      if(nuv[0] > 1.0)
		{
		  nuv[0] = 1.0;
		}
	    }
	  else
	    {
	      if(uv[0] < 0.5)
		{
		  nuv[0] = 0.5-(0.5-uv[0])*t;
		  /*
		  printf("t:%lg, %lg,%lg,%lg\n",t,uv[0],nuv[0],uv[1]);
		  */
		  if(nuv[0] < 0.0)
		    {
		      nuv[0] = 0.0;
		    }
		}
	      else
		{
		  nuv[0] = uv[0];
		}
	    }
	}
      else
	{
	  nuv[0] = uv[0];
	}

      nuv[1] = uv[1];

      np = tp->npatch[0];
      if(np->is_rat)
	{
	  (void)ay_nb_FirstDerSurf4D(np->width-1, np->height-1,
				     np->uorder-1, np->vorder-1,
				     np->uknotv, np->vknotv,
				     np->controlv, nuv[0], nuv[1],
				     PD);
	}
      else
	{
	  (void)ay_nb_FirstDerSurf3D(np->width-1, np->height-1,
				     np->uorder-1, np->vorder-1,
				     np->uknotv, np->vknotv,
				     np->controlv, nuv[0], nuv[1],
				     PD);
	}

      memcpy(entry->P, PD, 3*sizeof(double));
      fdu = &(PD[3]);
      fdv = &(PD[6]);
      N = &(entry->P[3]);
      AY_V3CROSS(N, fdu, fdv);

      if(res)
	memcpy(res, PD, 3*sizeof(double));

      /* memoize pnt */
      ay_rtess_addtohash(&(tp->surfaceHash), key, entry);
    } /* if have memoized point */

  if(!output)
    return;

  /* output this point */
  if(!entry->index)
    ay_rtess_output(tp, entry);

 return;
} /* ay_rtess_computesurfacedegen */


void
ay_rtess_output(rtp *tp, rtess_htentry *entry)
{
 double *tmp, *c;

  tp->outindex++;
  entry->index = tp->outindex;

  if(tp->coordspos+6 >= tp->coordslen)
    {
      if(!(tmp = realloc(tp->coords, tp->coordslen * 2 * sizeof(double))))
	return;
      tp->coords = tmp;
      tp->coordslen *= 2;
    }
  c = &(tp->coords[tp->coordspos]);
  memcpy(c, entry->P, 6*sizeof(double));
  tp->coordspos += 6;

 return;
}


void
ay_rtess_outputwithtex(rtp *tp, rtess_htentry *entry)
{
 double *tmp, *c;

  /* manage indices */
  tp->outindex++;
  entry->index = tp->outindex;

  /* manage point coordinates */
  if(tp->coordspos+6 >= tp->coordslen)
    {
      if(!(tmp = realloc(tp->coords, tp->coordslen * 2 * sizeof(double))))
	return;
      tp->coords = tmp;
      tp->coordslen *= 2;
    }
  c = &(tp->coords[tp->coordspos]);
  memcpy(c, entry->P, 6*sizeof(double));
  tp->coordspos += 6;

  /* manage texture coordinates */
  if(tp->texcoords)
    {
      if(tp->texcoordspos+2 >= tp->texcoordslen)
	{
	  if(!(tmp = realloc(tp->texcoords, tp->texcoordslen * 2 *
			     sizeof(double))))
	    return;
	  tp->texcoords = tmp;
	  tp->texcoordslen *= 2;
	}
      tp->texcoords[tp->texcoordspos] = entry->u;
      tp->texcoordspos++;
      tp->texcoords[tp->texcoordspos] = entry->v;
      tp->texcoordspos++;
    }

 return;
}


/* Decide if an edge should be split;
   must be commutative to avoid cracks. */
double
ay_rtess_splitedge(rtp *tp, double *a, double *b)
{
 double pa[3], pb[3], pm[3];
 double m[2], lab, lamb;

  tp->computesurface(tp, a, AY_TRUE, pa);
  tp->computesurface(tp, b, AY_TRUE, pb);

  if((fabs(pa[0]-pb[0]) > AY_EPSILON) ||
     (fabs(pa[1]-pb[1]) > AY_EPSILON) ||
     (fabs(pa[2]-pb[2]) > AY_EPSILON))
    {
      lab = sqrt((pa[0]-pb[0])*(pa[0]-pb[0]) +
		 (pa[1]-pb[1])*(pa[1]-pb[1]) +
		 (pa[2]-pb[2])*(pa[2]-pb[2]));
      if(lab < tp->edge_thresh)
	{
	  /* check for curvy region by computing the middle between
	     a and b and see how the distances a-b and a-m-b compare */
	  m[0] = (a[0]+b[0])*0.5;
	  m[1] = (a[1]+b[1])*0.5;

	  tp->computesurface(tp, m, AY_FALSE, pm);

	  lamb = 0.0;
	  if((fabs(pa[0]-pm[0]) > AY_EPSILON) ||
	     (fabs(pa[1]-pm[1]) > AY_EPSILON) ||
	     (fabs(pa[2]-pm[2]) > AY_EPSILON))
	    {
	      lamb += sqrt((pa[0]-pm[0])*(pa[0]-pm[0])+
			   (pa[1]-pm[1])*(pa[1]-pm[1])+
			   (pa[2]-pm[2])*(pa[2]-pm[2]));
	    }
	  if((fabs(pb[0]-pm[0]) > AY_EPSILON) ||
	     (fabs(pb[1]-pm[1]) > AY_EPSILON) ||
	     (fabs(pb[2]-pm[2]) > AY_EPSILON))
	    {
	      lamb += sqrt((pb[0]-pm[0])*(pb[0]-pm[0])+
			   (pb[1]-pm[1])*(pb[1]-pm[1])+
			   (pb[2]-pm[2])*(pb[2]-pm[2]));
	    }
	  if(lamb > lab*1.01)
	    lab += lamb;
	}
      return lab;
    }
  else
    {
      return 0.0;
    }
} /* ay_rtess_splitedge */


/* To avoid missing high frequency detail (e.g. spikes in functions)
   should all three edges fall below the split threshold, this function
   is called to determine if the triangle should be split by adding
   a vertex to its center. An interval bound on the function over the
   domain of the triangle is one possible implementation. */
int
ay_rtess_splitcenter(rtp *tp, double *tri)
{
 int i;
 double ed = 0.0, cv[2];

  cv[0] = (tri[0] + tri[2] + tri[4])/3.0;
  cv[1] = (tri[1] + tri[3] + tri[5])/3.0;

  for(i = 0; i < 3; i++)
    ed += ay_rtess_splitedge(tp, &(tri[i*2]), cv);

  if(ed*0.5 > tp->edge_thresh)
    {
      tp->computesurface(tp, cv, AY_TRUE, NULL);
      return 1;
    }

 return 0;
} /* ay_rtess_splitcenter */


/* Refine a triangle around a middle point. */
void
ay_rtess_dicetri(rtp *tp, double *tri)
{
 double a[2], b[2], c[2], d[2], slice[6];
 double beg, end;
 int divslen = 0, ed, e1;
 rtess_div *divs = NULL, *tmp;

  /* pick a central point */
  c[0] = (tri[0] + tri[2] + tri[4])/3.0;
  c[1] = (tri[1] + tri[3] + tri[5])/3.0;

  tp->computesurface(tp, c, AY_TRUE, NULL);

  for(ed = 0; ed < 3; ed++)
    {
      while(divs)
	{
	  tmp = divs->next;
	  free(divs);
	  divs = tmp;
	}
      if(!(divs = calloc(1, sizeof(rtess_div))))
	goto cleanup;

      divslen = 1;

      e1 = (ed+1) % 3;

      d[0] = tri[e1*2] - tri[ed*2];
      d[1] = tri[e1*2+1] - tri[ed*2+1];

      divs->div = 1.0;
      beg = 0.0;
      while(divslen)
	{
	  end = divs->div;
	  a[0] = tri[ed*2]   + d[0]*beg;
	  a[1] = tri[ed*2+1] + d[1]*beg;
	  b[0] = tri[ed*2]   + d[0]*end;
	  b[1] = tri[ed*2+1] + d[1]*end;

	  if(ay_rtess_splitedge(tp, a, b) > tp->edge_thresh)
	    {
	      /* split edge */
	      if(!(tmp = calloc(1, sizeof(rtess_div))))
		goto cleanup;
	      tmp->div = 0.5*(beg+end);
	      tmp->next = divs;
	      divs = tmp;
	      divslen++;
	    }
	  else
	    {
	      /* render it */
	      tp->computesurface(tp, a, AY_TRUE, NULL);
	      tp->computesurface(tp, b, AY_TRUE, NULL);

	      AY_SETTRI(slice, c[0],c[1],  a[0],a[1],  b[0],b[1]);

	      ay_rtess_renderfinal(tp, slice);

	      tmp = divs->next;
	      free(divs);
	      divs = tmp;
	      divslen--;

	      beg = end;
	    }
	} /* while */
    } /* for */

cleanup:

  while(divs)
    {
      tmp = divs->next;
      free(divs);
      divs = tmp;
    }

 return;
} /* ay_rtess_dicetri */


void
ay_rtess_refinetri(rtp *tp, double *tri, double *pieces, int *pieceslen)
{
 int i, j, j0, j1, co;
 double area;
 double a[2];
 double eds[3], m[3], mv[6];
 double max_ed;
 double *p;

#if 0
  /* 0 cull entire triangle? */
  if(this.tloops && this.inOut(tri) < 0)
   return [];
#endif

  /* 1 measure triangle degeneracy */

  area = tri[0]*tri[3] - tri[2]*tri[1] +
         tri[2]*tri[5] - tri[4]*tri[3] +
         tri[4]*tri[1] - tri[0]*tri[5];

  if(area < 0)
    area = -area;

  /* calc sum of squares of edge lengths */
  a[0] = tri[0] - tri[2];
  a[1] = tri[1] - tri[3];
  max_ed = a[0]*a[0] + a[1]*a[1];
  a[0] = tri[2] - tri[4];
  a[1] = tri[3] - tri[5];
  max_ed += a[0]*a[0] + a[1]*a[1];
  a[0] = tri[4] - tri[0];
  a[1] = tri[5] - tri[1];
  max_ed += a[0]*a[0] + a[1]*a[1];

  area /= max_ed;

  if(area <= tp->skew_thresh)
    {
      /* triangle is skewed => dice rather than split edges */
      ay_rtess_dicetri(tp, tri);
      return;
    }

  /* 2 split edges */

  max_ed = 0.0;
  for(i = 0; i < 3; i++)
    {
      eds[i] = ay_rtess_splitedge(tp, &(tri[((i+1)*2) % 6]),
			       &(tri[((i+2)*2) % 6]));
      if(eds[i] > max_ed)
	max_ed = eds[i];
    }

  max_ed *= tp->split_bias;

  co = 0;
  for(i = 0; i < 3; i++)
    {
      j0 = (i+1) % 3;
      j1 = (i+2) % 3;

      if((eds[i] > tp->edge_thresh) && (eds[i] >= max_ed))
	{
	  co++;
	  mv[i*2] = 0.5*(tri[j0*2] + tri[j1*2]);
	  mv[i*2+1] = 0.5*(tri[j0*2+1] + tri[j1*2+1]);
	  m[i] = i - 3;
	}
      else
	{
	  /* move midpt to vertex closer to center */
	  if(eds[j0] > eds[j1])
	    {
	      mv[i*2] = tri[j0*2];
	      mv[i*2+1] = tri[j0*2+1];
	      m[i] = j0;
	    }
	  else
	    {
	      mv[i*2] = tri[j1*2];
	      mv[i*2+1] = tri[j1*2+1];
	      m[i] = j1;
	    }
	}
    } /* for */

  if(co)
    {
      co++;
      j = co;
      p = pieces;

      /* corner tiles */
      for(i = 0; i < 3; i++)
	{
	  j0 = (i+1) % 3;
	  j1 = (i+2) % 3;
	  if((m[j1] != i) && (m[j0] != i))
	    {
	      memcpy(p, &(tri[i*2]), 2*sizeof(double));
	      memcpy(p+2, &(mv[j1*2]), 2*sizeof(double));
	      memcpy(p+4, &(mv[j0*2]), 2*sizeof(double));
	      *pieceslen = *pieceslen+1;
	      p += 6;
	      j--;
	    }
	} /* for */

      /* center tile */
      if(j)
	{
	  if((m[0] == m[1]) || (m[1] == m[2]) || (m[2] == m[0]))
	    {
	      /* avoid degenerate center tile */
	      *pieceslen = 0;
	    }
	  else
	    {
	      memcpy(p, mv, 6*sizeof(double));
	      *pieceslen = *pieceslen+1;
	    }
	}
      return;
    } /* if co */

  /* check disabled, because the curvature check in splitEdge()
     already does enough */
#if 0
    else if (this.splitCenter( mv )) {
    /* no edges split; add vertex to center and do a simple dice? */
    var c = [];
    c[0] = (tri[0][0] + tri[1][0] + tri[2][0])/3.0;
    c[1] = (tri[0][1] + tri[1][1] + tri[2][1])/3.0;

    for(var i = 0; i < 3; i++) {
    res[i] = [tri[i], tri[(i+1)%3], c];
    }

    return res;
    }
#endif

  ay_rtess_renderfinal(tp, tri);

 return;
} /* ay_rtess_refinetri */


/* Render the refined tile once all edges match the acceptance criteria. */
void
ay_rtess_renderfinal(rtp *tp, double *tri)
{
 unsigned int key, *tmp;
 unsigned int indu, indv;
 int i;
 double *uv;
 rtess_htentry *entry;

  for(i = 0; i < 3; i++)
    {
      uv = &(tri[i*2]);

      indu = (unsigned int)(fabs(uv[0]*AY_POW));
      indv = (unsigned int)(fabs(uv[1]*AY_POW));
      key = (unsigned int)((3 * indu + 5 * indv) % tp->surfaceHash.size);

      entry = tp->surfaceHash.table[key];
      while(entry)
	{
	  if(entry->u == uv[0] && entry->v == uv[1])
	    {
	      if(tp->indicespos >= tp->indiceslen)
		{
		  if(!(tmp = realloc(tp->indices, 2 * tp->indiceslen *
				     sizeof(unsigned int))))
		    return;
		  tp->indices = tmp;
		  tp->indiceslen *= 2;
		}
	      tp->indices[tp->indicespos] = entry->index-1;
	      tp->indicespos++;
	    }
	  entry = entry->next;
	} /* while */
    } /* for */

  tp->numtris++;

 return;
} /* ay_rtess_renderfinal */


int
ay_rtess_isdegen(double *tri)
{
 double *p1, *p2, px[3], t1[3], t2[3];
 double v[3], vw;
 int degen = AY_FALSE, i;

  for(i = 0; i < 3; i++)
    {
      p1 = &(tri[i%3]);
      p2 = &(tri[(i+1)%3]);
      if(AY_V3COMP(p1, p2))
	{
	  degen = AY_TRUE;
	  break;
	}
    } /* for */

  if(degen)
    return 1;

  p1 = tri;
  p2 = p1+3;
  AY_V3SUB(t1, p1, p2)
    p2 = p2+3;
  AY_V3SUB(t2, p1, p2)
  AY_V3CROSS(px, t1, t2)
  AY_V3CROSS(v, px, t1)
  vw = AY_V3DOT(v, t2);

  if((vw*vw) < AY_EPSILON)
    return 1;

 return 0;
}


int
ay_rtess_tesstri(rtp *tp, double *tri)
{
 double *work = tri, *tmp;
 int worklen = 1;
 double pieces[6*4] = {0};
 int pieceslen = 0;

  if(!(work = malloc(6*sizeof(double))))
    {
      return AY_EOMEM;
    }
  memcpy(work, tri, 6*sizeof(double));
  while(worklen)
    {
      pieceslen = 0;
      ay_rtess_refinetri(tp, &(work[(worklen-1)*6]), pieces, &pieceslen);
      worklen--;
      if(pieceslen)
	{
	  if(!(tmp = realloc(work, (worklen+pieceslen)*6*sizeof(double))))
	    {
	      free(work);
	      return AY_EOMEM;
	    }
	  work = tmp;
	  memcpy(&(work[worklen*6]), pieces, pieceslen*6*sizeof(double));
	  worklen += pieceslen;
	}
    } /* while */

  free(work);

 return AY_OK;
} /* ay_rtess_tesstri */


/** ay_rtess_computedegenpeak:
 * Compute the point and normal of the degenerate peak at v1.
 * The result will be stored in the surfaceHash of the given tesselation.
 *
 * \param[in] np  NURBS patch
 * \param[in,out] tp  the tesselation
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_rtess_computedegenpeak(ay_nurbpatch_object *np, rtp *tp)
{
 double P[9];
 double uv[6];
 double offset = 0.1;
 int i;
 rtess_htentry *entry = NULL;
 unsigned int indu, indv, key;

  uv[0] = (np->uknotv[np->uorder-1] + np->uknotv[np->width]) * 0.5;
  uv[1] = np->vknotv[np->height];
  indu = (unsigned int)(fabs(uv[0])*AY_POW);
  indv = (unsigned int)(fabs(uv[1])*AY_POW);
  key = (unsigned int)((3 * indu + 5 * indv) % tp->surfaceHash.size);

  if(!(entry = calloc(1, sizeof(rtess_htentry))))
    return AY_EOMEM;

  entry->u = uv[0];
  entry->v = uv[1];

  uv[2] = np->uknotv[np->uorder-1];
  uv[3] = np->vknotv[np->height] -
    ((np->vknotv[np->height]-np->vknotv[np->height-1]) * offset);

  uv[4] = np->uknotv[np->width];
  uv[5] = uv[3];

  for(i = 0; i < 3; i++)
    {
      if(np->is_rat)
	{
	  (void)ay_nb_SurfacePoint4D(np->width-1, np->height-1,
				     np->uorder-1, np->vorder-1,
				     np->uknotv, np->vknotv,
				     np->controlv, uv[i*2], uv[i*2+1],
				     &(P[i*3]));
	}
      else
	{
	  (void)ay_nb_SurfacePoint3D(np->width-1, np->height-1,
				     np->uorder-1, np->vorder-1,
				     np->uknotv, np->vknotv,
				     np->controlv, uv[i*2], uv[i*2+1],
				     &(P[i*3]));
	}
    }

  ay_geom_normalfrom3pnts(&(P[0]), &(P[6]), &(P[3]), &(entry->P[3]));

  memcpy(entry->P, P, 3*sizeof(double));

  /* memoize pnt */
  ay_rtess_addtohash(&(tp->surfaceHash), key, entry);

 return AY_OK;
} /* ay_rtess_computedegenpeak */


/** ay_rtess_getedgethresh:
 * Compute a RTESS edge threshold value in the range of [AY_EPSILON, 0.5]
 * from a GLU sampling tolerance in the range of [0, 100].
 *
 * \param[in] tolerance  GLU sampling tolerance value
 *
 * \returns RTESS edge threshhold
 */
double
ay_rtess_getedgethresh(double tolerance)
{
 double t;

  if(tolerance > 100)
    return 0.5;

  if((tolerance+AY_EPSILON) > 25.0)
    {
      return 0.1+(tolerance-25.0)/75.0*0.4;
    }
  else
    {
      if((tolerance-AY_EPSILON) < 25.0)
	{
	  t = tolerance*0.004;
	  if(t > AY_EPSILON)
	    return t;
	  else
	    return AY_EPSILON;
	}
    }

 return 0.1;
} /* ay_rtess_getedgethresh */


/** ay_rtess_tessnp:
 * Tesselate a NURBS patch using RTESS.
 *
 * \param[in] np  NURBS patch to tesselate
 * \param[in] degen  if AY_TRUE, the patch is believed to be degenerate at
 *                   v1, as e.g. created by the triangular Gordon surface
 * \param[in] tolerance  GLU sampling tolerance value in the range [0, 100]
 *                       to control the tesselation fidelity
 * \param[in,out] result  where to store the resulting PolyMesh object
 *
 * \returns AY_OK on success, error code otherwise.
 */
int
ay_rtess_tessnp(ay_nurbpatch_object *np, int degen, double tolerance,
		ay_object **result)
{
 int ay_status = AY_OK;
 rtp tp = {0};
 unsigned int i;
 double u00, u05, u10;
 double v00, v05, v10, v09;
 double tri[6];
 ay_object *new = NULL;
 ay_pomesh_object *po = NULL;

  if(!np)
    return AY_ENULL;

  if(!(tp.coords = malloc(32*sizeof(double))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  tp.coordslen = 32;

  if(!(tp.indices = malloc(32*sizeof(unsigned int))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  tp.indiceslen = 32;
  tp.npatch = &np;
  tp.computesurface = ay_rtess_computesurface;
  tp.edge_thresh = ay_rtess_getedgethresh(tolerance);

  tp.split_bias = 0.5;
  tp.skew_thresh = 0.01;

  tp.surfaceHash.size = 65536;
  ay_status = ay_rtess_inithash(&(tp.surfaceHash));

  u00 = np->uknotv[np->uorder-1];
  u10 = np->uknotv[np->width];
  u05 = (u00+u10)*0.5;
  v00 = np->vknotv[np->vorder-1];
  v10 = np->vknotv[np->height];
  v05 = (v00+v10)*0.5;

  AY_SETTRI(tri, u00,v00, u00,v05, u05,v00);
  ay_status = ay_rtess_tesstri(&tp, tri);
  if(ay_status)
    goto cleanup;
  AY_SETTRI(tri, u00,v05, u05,v05, u05,v00);
  ay_status = ay_rtess_tesstri(&tp, tri);
  if(ay_status)
    goto cleanup;
  AY_SETTRI(tri, u05,v00, u05,v05, u10,v00);
  ay_status = ay_rtess_tesstri(&tp, tri);
  if(ay_status)
    goto cleanup;
  AY_SETTRI(tri, u05,v05, u10,v05,u10,v00);
  ay_status = ay_rtess_tesstri(&tp, tri);
  if(ay_status)
    goto cleanup;

  if(!degen)
    {
      AY_SETTRI(tri, u00,v05, u00,v10, u05,v05);
      ay_status = ay_rtess_tesstri(&tp, tri);
      if(ay_status)
	goto cleanup;
      AY_SETTRI(tri, u00,v10, u05,v10, u05,v05);
      ay_status = ay_rtess_tesstri(&tp, tri);
      if(ay_status)
	goto cleanup;

      AY_SETTRI(tri, u05,v05, u05,v10, u10,v05);
      ay_status = ay_rtess_tesstri(&tp, tri);
      if(ay_status)
	goto cleanup;
      AY_SETTRI(tri, u05,v10, u10,v10, u10,v05);
      ay_status = ay_rtess_tesstri(&tp, tri);
      if(ay_status)
	goto cleanup;
    }
  else
    {
      v09 = v10*0.9;
      AY_SETTRI(tri, u00,v05, u00,v09, u05,v05);
      ay_status = ay_rtess_tesstri(&tp, tri);
      if(ay_status)
	goto cleanup;
      AY_SETTRI(tri, u00,v09, u05,v09, u05,v05);
      ay_status = ay_rtess_tesstri(&tp, tri);
      if(ay_status)
	goto cleanup;

      AY_SETTRI(tri, u05,v05, u05,v09, u10,v05);
      ay_status = ay_rtess_tesstri(&tp, tri);
      if(ay_status)
	goto cleanup;
      AY_SETTRI(tri, u05,v09, u10,v09, u10,v05);
      ay_status = ay_rtess_tesstri(&tp, tri);
      if(ay_status)
	goto cleanup;

      tp.computesurface = ay_rtess_computesurfacedegen;

      ay_status = ay_rtess_computedegenpeak(np, &tp);
      if(ay_status)
	goto cleanup;

      AY_SETTRI(tri, u00,v09, u05,v10, u10,v09);
      ay_status = ay_rtess_tesstri(&tp, tri);
      if(ay_status)
	goto cleanup;
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

  po->npolys = tp.numtris;

  /* create index structures */
  if(!(po->nloops = malloc(tp.numtris*sizeof(unsigned int))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  for(i = 0; i < tp.numtris; i++)
    {
      /* each polygon has just one loop */
      po->nloops[i] = 1;
    } /* for */

  if(!(po->nverts = malloc(tp.numtris*sizeof(unsigned int))))
    {
      ay_status = AY_EOMEM;
      goto cleanup;
    }
  for(i = 0; i < tp.numtris; i++)
    {
      /* each loop has just three vertices (is a triangle) */
      po->nverts[i] = 3;
    } /* for */

  /* check indicespos vs indiceslen for possible realloc */
  po->verts = tp.indices;
  tp.indices = NULL;

  po->ncontrols = tp.outindex;
  po->has_normals = AY_TRUE;

  /* check coordspos vs coordslen for possible realloc */
  po->controlv = tp.coords;
  tp.coords = NULL;

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

  ay_rtess_cleartp(&tp);

 return ay_status;
} /* ay_rtess_tessnp */
