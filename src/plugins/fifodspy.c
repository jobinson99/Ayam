/*
 * Ayam, a free 3D modeler for the RenderMan interface.
 *
 * Ayam is copyrighted 1998-2020 by Randolf Schultz
 * (randolf.schultz@gmail.com) and others.
 *
 * All rights reserved.
 *
 * See the file License for details.
 *
 */

/* fifodspy.c - a RenderMan Display Driver that writes RGBA-Data to a FIFO */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <ndspy.h>

#ifdef WIN32
#include "mkfifo.h"
#endif

typedef struct fifoimagetype_s
{
  FILE *file;
  int channels;
  int width, height;
#ifdef PIXIEDISPLAY
  float	qmin,qmax,qone,qzero,qamp;
  float	gamma, gain;
  int numSamples;
  unsigned char *cdata;
  size_t cdatalen;
#endif
} fifoimagetype;


PtDspyError
DspyImageOpen(PtDspyImageHandle *imagehandle,
	      const char *drivername,
	      const char *filename,
	      int width, int height, int paramCount,
	      const UserParameter *parameters,
	      int formatCount,
	      PtDspyDevFormat *format,
	      PtFlagStuff *flagstuff)
{
 PtDspyError ret = PkDspyErrorNone;
 int i, err;
 fifoimagetype *image;
 PtDspyDevFormat ourformat[4];
#ifdef WIN32
 size_t lf, lt;
 char *pipe = NULL;
 char *tmpdirname = NULL;
 char *syncfilename = NULL;
#endif

  if(formatCount != 4)
    return PkDspyErrorBadParams;

  if(!(image = malloc(sizeof(fifoimagetype))))
    return PkDspyErrorNoMemory;

#ifdef WIN32
  /* construct pipe name from filename */
  lf = strlen(filename);
  if(!(pipe = malloc((lf+10)*sizeof(char))))
    {
      ret = PkDspyErrorNoMemory;
      goto cleanup;
    }
  memcpy(pipe, "\\\\.\\pipe\\", 9*sizeof(char));
  memcpy(&(pipe[9]), filename, lf*sizeof(char));
  pipe[lf+9] = '\0';

  /* construct sync file name */
  tmpdirname = getenv("TEMP");
  if(!tmpdirname)
    tmpdirname = getenv("TMP");
  if(!tmpdirname)
    tmpdirname = getenv("USERPROFILE");

  if(tmpdirname)
    {
      lt = strlen(tmpdirname);
      if(!(syncfilename = malloc((lf+lt+2)*sizeof(char))))
	{
	  ret = PkDspyErrorNoMemory;
	  goto cleanup;
	}
      memcpy(syncfilename, tmpdirname, lt*sizeof(char));
      syncfilename[lt] = '\\';
      memcpy(&(syncfilename[lt+1]), filename, lf*sizeof(char));

      syncfilename[lf+lt+1] = '\0';
    }

  /* create named pipe */
  if((image->file = mkfifo(pipe, syncfilename)) == NULL)
    {
      ret = PkDspyErrorNoResource;
      goto cleanup;
    }
#else
  if((err = mkfifo(filename, 0666)) != 0)
    {
      ret = PkDspyErrorNoResource;
      goto cleanup;
    }
  image->file = fopen(filename, "wb");
#endif

  if(0 == width)
    width = 640;
  if(0 == height)
    height = 480;

  image->channels = formatCount;
  image->width = width;
  image->height = height;

  for(i = 0; i < formatCount; i++)
    {
      format[i].type = PkDspyUnsigned8;
    }

  for(i = 0; i < formatCount; i++)
    {
      switch(*format[i].name)
	{
	case 'r':
	  ourformat[0] = format[i];
	  break;
	case 'g':
	  ourformat[1] = format[i];
	  break;
	case 'b':
	  ourformat[2] = format[i];
	  break;
	case 'a':
	  ourformat[3] = format[i];
	  break;
	default:
	  break;
	}
    }

  for(i = 0; i < formatCount; i++)
    {
      format[i].name = ourformat[i].name;
    }

  flagstuff->flags |= PkDspyFlagsWantsScanLineOrder;

  *imagehandle = image;

  /* prevent cleanup code from doing something harmful */
  image = NULL;

cleanup:

  if(image)
    free(image);

#ifdef WIN32
  if(pipe)
    free(pipe);
  if(syncfilename)
    free(syncfilename);
#endif

 return ret;
}


PtDspyError
DspyImageQuery(PtDspyImageHandle imagehandle,
	       PtDspyQueryType querytype,
	       size_t datalen,
	       void *data)
{
 PtDspyError ret;
 fifoimagetype *image = (fifoimagetype*)imagehandle;

  ret = PkDspyErrorNone;

  if(datalen > 0 && NULL != data)
    {
      switch(querytype)
      {
      case PkOverwriteQuery:
	{
	  PtDspyOverwriteInfo overwriteinfo;

	  if(datalen > sizeof(overwriteinfo))
	    {
	      datalen = sizeof(overwriteinfo);
	    }
	  overwriteinfo.overwrite = 1;
	  overwriteinfo.interactive = 0;
	  memcpy(data, &overwriteinfo, datalen);
	  break;
	}
      case PkSizeQuery:
	{
	  PtDspySizeInfo sizeinfo;

	  if(datalen > sizeof(sizeinfo))
	    {
	      datalen = sizeof(sizeinfo);
	    }
	  if(image)
	    {
	      if(0 == image->width || 0 == image->height)
		{
		  image->width = 640;
		  image->height = 480;
		}
	      sizeinfo.width = image->width;
	      sizeinfo.height = image->height;
	      sizeinfo.aspectRatio = 1.0f;
	    }
	  else
	    {
	      sizeinfo.width = 640;
	      sizeinfo.height = 480;
	      sizeinfo.aspectRatio = 1.0f;
	    }
	  memcpy(data, &sizeinfo, datalen);
	  break;
	}
#if 0
      case PkRenderingStartQuery:
	{
	  PtDspyRenderingStartQuery startlocation;

	  if(datalen > sizeof(startlocation))
	    {
	      datalen = sizeof(startlocation);
	    }
	  if(image)
	    {
	      /*
	       * initialize values in startLocation
	       */
	      memcpy(data, &startlocation, datalen);
	    }
	  else
	    {
	      ret = PkDspyErrorUndefined;
	    }
	  break;
	}
#endif
      default:
	ret = PkDspyErrorUnsupported;
	break;
      }
    }
  else
    {
      ret = PkDspyErrorBadParams;
    }

 return ret;
}


PtDspyError
DspyImageData(PtDspyImageHandle imagehandle,
	      int xmin,
	      int xmax_plusone,
	      int ymin,
	      int ymax_plusone,
	      int entrysize,
	      const unsigned char *data)
{
 int x, y;
 int xy[4] = {xmin, xmax_plusone, ymin, ymax_plusone};
 fifoimagetype *image = (fifoimagetype*)imagehandle;

  fwrite(xy, sizeof(int), 4, image->file);

  for(y = ymin; y < ymax_plusone; y++)
    {
      for(x = xmin; x < xmax_plusone; x++)
	{
	  fwrite(data, sizeof(unsigned char), image->channels, image->file);
	  data += entrysize;
	}
    }

 return PkDspyErrorNone;
}


PtDspyError
DspyImageClose(PtDspyImageHandle imagehandle)
{
 fifoimagetype *image = (fifoimagetype*)imagehandle;
 int xy[4] = {-1,-1,-1,-1};

  /* send end of image message */
  fwrite(xy, sizeof(int), 4, image->file);

  fclose(image->file);
  free(image);

 return PkDspyErrorNone;
}

/**************************************/
/* Pixie display driver code follows. */
/**************************************/

#ifdef PIXIEDISPLAY

#ifdef WIN32
#include <synchapi.h>
CRITICAL_SECTION cs;
static int csisinit = 0;
#else
#include <unistd.h>
#include <pthread.h>
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif /* WIN32 */

/*#include <ri/dsply.h>*/

/* as "ri/dsply.h" is C++, we simply repeat the relevant content here */
typedef enum {
	FLOAT_PARAMETER,
	VECTOR_PARAMETER,
	MATRIX_PARAMETER,
	STRING_PARAMETER,
	INTEGER_PARAMETER
} ParameterType;

typedef void *(*TDisplayParameterFunction)(const char *,ParameterType,int);
typedef	void *(*TDisplayStartFunction)(const char *,int,int,int,const char *,TDisplayParameterFunction);
typedef int (*TDisplayDataFunction)(void *,int,int,int,int,float *);
typedef int (*TDisplayRawDataFunction)(void *,int,int,int,int,void *);
typedef void (*TDisplayFinishFunction)(void *);


void *
displayStart(const char *name, int width, int height,
	     int numSamples, const char *samples,
	     TDisplayParameterFunction findParameter)
{
 fifoimagetype *imagehandle;
 PtDspyError dspystat;
 PtDspyDevFormat dspyformat[4];
 PtFlagStuff flagstuff = {0};
 float *tmp;
#ifdef WIN32
  if(!csisinit)
    {
      InitializeCriticalSection(&cs);
      csisinit = 1;
    }
#endif
  if(!strcmp(samples, "z"))
    return NULL;

  dspyformat[0].name = "r";
  dspyformat[1].name = "g";
  dspyformat[2].name = "b";
  dspyformat[3].name = "a";

  dspystat = DspyImageOpen((void**)&imagehandle,
			   /*drivername=*/NULL, /*filename=*/name,
			   width, height,
			   /*paramCount=*/0, /*parameters=*/NULL,
			   /*formatCount=*/4, /*format=*/dspyformat,
			   &flagstuff);

  if(dspystat != PkDspyErrorNone || imagehandle == NULL)
    return NULL;

  imagehandle->cdatalen = 0;
  imagehandle->cdata = NULL;
  imagehandle->numSamples = numSamples;

  if((tmp = (float *) findParameter("quantize", FLOAT_PARAMETER, 4)))
    {
      imagehandle->qzero = tmp[0];
      imagehandle->qone = tmp[1];
      imagehandle->qmin = tmp[2];
      imagehandle->qmax = tmp[3];
    }
  else
    {
      imagehandle->qzero = 0;
      imagehandle->qone = 255.0f;
      imagehandle->qmin = 0;
      imagehandle->qmax = 255.0f;
    }

  if((tmp = (float *) findParameter("dither", FLOAT_PARAMETER, 1)))
    {
      imagehandle->qamp = tmp[0];
    }

  if((tmp = (float *) findParameter("gamma", FLOAT_PARAMETER, 1)))
    {
      imagehandle->gamma = tmp[0];
    }

  if((tmp = (float *) findParameter("gain", FLOAT_PARAMETER, 1)))
    {
      imagehandle->gain = tmp[0];
    }

 return imagehandle;
} /* displayStart */


int
displayData(void *im, int x, int y, int w, int h, float *data)
{
 fifoimagetype *imagehandle = (fifoimagetype*)im;
 unsigned char *cdata;
 int i, j, a, b;
 float dither;

#ifdef WIN32
  EnterCriticalSection(&cs);
#else
  pthread_mutex_lock(&mutex);
#endif

  if(!imagehandle->cdata || (imagehandle->cdatalen < w * h * 4 *
			     sizeof(unsigned char)))
    {
      if(!(imagehandle->cdata = malloc(w * h * 4 * sizeof(unsigned char))))
	{
#ifdef WIN32
	  LeaveCriticalSection(&cs);
#else
	  pthread_mutex_unlock(&mutex);
#endif
	  return 0;
	}
      imagehandle->cdatalen = w * h * 4 * sizeof(unsigned char);
    }

  cdata = imagehandle->cdata;

  memset(cdata, 0, imagehandle->cdatalen);

  /* from file_png.cpp */
  /* Apply the pixel correction if applicable */
  if(imagehandle->gain != 1.0f)
    {
      for(i = 0; i < w * h * imagehandle->numSamples; i++)
	{
	  data[i] *= imagehandle->gain;
	}
    }

  /* Apply the quantization if applicable */
  if(imagehandle->qmax > 0.0f)
    {
      for(i = 0; i < w * h * imagehandle->numSamples; i++)
	{
	  dither = imagehandle->qamp*(2*(rand() / (float) RAND_MAX)-1);
	  data[i] = imagehandle->qzero + (imagehandle->qone -
		     imagehandle->qzero)*data[i] + dither;
	  if (data[i] < imagehandle->qmin)
	    data[i] = imagehandle->qmin;
	  else if (data[i] > imagehandle->qmax)
	    data[i] = imagehandle->qmax;
	}
    }

  /* copy data to char array */
  a = 0;
  b = 0;
  for(i = 0; i < w*h; i++)
    {
      for(j = 0; j < imagehandle->numSamples; j++)
	{
	  cdata[a+j] = data[b+j];
	}
      a += 4;
      b += imagehandle->numSamples;
    }

  DspyImageData(imagehandle, x, x+w, y, y+h, 4, cdata);

#ifdef WIN32
  LeaveCriticalSection(&cs);
#else
  pthread_mutex_unlock(&mutex);
#endif

 return 1;
} /* displayData */


void
displayFinish(void *im)
{
 fifoimagetype *imagehandle = (fifoimagetype*)im;

#ifdef WIN32
  EnterCriticalSection(&cs);
#else
  pthread_mutex_lock(&mutex);
#endif

  if(imagehandle->cdata)
    free(imagehandle->cdata);

  DspyImageClose(im);

#ifdef WIN32
  LeaveCriticalSection(&cs);
  DeleteCriticalSection(&cs);
  csisinit = 0;
#else
  pthread_mutex_unlock(&mutex);
#endif

 return;
} /* displayFinish */

#endif /* PIXIEDISPLAY */
