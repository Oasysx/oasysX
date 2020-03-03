/*
    OasysX -- Oayss Extractor, Open Source Edition

    Copyright (c) 2020 by Oasys Cybernetics Pvt Ltd, Inc. All rights reserved.

    OasysX and Oasys are registered trademarks or trademarks of Oasys. in the India and other
    countries.

    Oasys Extractor is open source software that you may modify and/or
    redistribute under the terms of the GNU Lesser General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version, provided that the
    conditions specified in the COPYRIGHT.txt file provided with this
    software are met.
 
    For more information, please visit http://www.oasys.co/oasysx

      IMPLEMENTATION:   Doss, Deepak

      DATE:           02/01/2020
*/

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "minexiii_create.h"
#include "minexiii.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stddef.h>

#include "minutia_points.h"


#ifdef WIN32
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif

#ifndef _STLP_WCE
long _sync_match(long volatile *, long, long);
#pragma intrinsic(_sync_match)
# define sync_match _sync_match
#else
_STLP_IMPORT_DECLSPEC long _STLP_STDCALL sync_match(long volatile *, long, long);
#endif

#ifdef __cplusplus
}
#endif
#endif

#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
#define HAS_SYNC_FUNCTIONS
#endif


static void * memory_allocation(size_t size, void * _) {
  return malloc(size);
}
static void memory_free(void * p, void * _) {
  free(p);
}

static unsigned int current_clock_millisec() {
  return (unsigned int)(clock());
}

static long memory_sync_increment(volatile long * pv) {
#ifdef WIN32
  return sync_data_inc(pv);
#elif defined(HAS_SYNC_FUNCTIONS)
  return __sync_fetch_and_add(pv, 1L);
#else
  return ++(*pv);
#endif
}

static long memory_sync_dec(volatile long * pv) {
#ifdef WIN32
  return sync_data_dec(pv);
#elif defined(HAS_SYNC_FUNCTIONS)
  return __sync_fetch_and_sub(pv, 1L);
#else
  return --(*pv);
#endif
}

static long memory_sync_exch(volatile long * pv, long val) {
#ifdef WIN32
  return InterlockedExchange(pv, val);
#elif defined(HAS_SYNC_FUNCTIONS)
  return __sync_lock_test_and_set(pv, val);
#else
  long old = *pv;
  *pv = val;
  return old;
#endif
}

static long memory_sync_compare(volatile long * pv, long val, long cmp) {
#ifdef WIN32
  return sync_match(pv, val, cmp);
#elif defined(HAS_SYNC_FUNCTIONS)
  return __sync_val_compare_and_swap(pv, cmp, val);
#else
  long old = *pv;
  if (*pv == cmp) *pv = val;
  return old;
#endif
}





#define CBEFF (0x00740001)
#define VER_STRINGIZE_(x) #x
#define VER_STRINGIZE(x) VER_STRINGIZE_(x)

#define VERSION VER_STRINGIZE(FRFXLL_MAJOR) "." VER_STRINGIZE(FRFXLL_MINOR)"." \
                VER_STRINGIZE(FRFXLL_REVISION) "." VER_STRINGIZE(FRFXLL_BUILD)

#define test(x, err) { if ((x) < MINEX_RET_SUCCESS) return err; }
#define extractor_test(x)    test(x, MINEX_RET_FAILURE_UNSPECIFIED);

uint16_t new_width;
uint16_t new_height;

unsigned char * crop_image(const uint8_t*raw_image,const uint16_t height,const uint16_t width);

void printarray(unsigned char **array, int r, int c)
{
     int i, j;
     for(i=0; i<r; i++ )
     {
         for(j=0; j<c; j++)
         {
            //printf("%c ", array[i][j] );
			printf("%d ", array[i][j] );
         }
        printf( "\n" );
     }
}

struct Image_procc {
  void * h;

  Image_procc(void * _h = NULL) : h(_h) {}

  ~Image_procc() {
    if (h)
      Close();
  }

  int Close() {
    int rc = MINEX_RET_SUCCESS;
    if (h) {
      rc = free_object(h);
    }
    h = NULL;
    return rc;
  }

  operator void *() const  { return h; }
  void ** operator &()     { return &h; }
};


int32_t create_template(const uint8_t*raw_image, const uint8_t finger_quality,const uint8_t finger_position,const uint8_t impression_type,const uint16_t height,const uint16_t width,uint8_t *output_template)
{
  	if (output_template == NULL)   
	{
		return MINEX_RET_FAILURE_NULL_TEMPLATE;
	}
  	if (raw_image == NULL) 
	{
		return MINEX_RET_FAILURE_BAD_IMPRESSION;
	}
	if (impression_type != 0 && impression_type!=2) 
	{
		return MINEX_RET_FAILURE_BAD_IMPRESSION;
	}
  	if (width > 2000 || height > 2000)
	{ 
        	return MINEX_RET_BAD_IMAGE_SIZE;
	}
  	if (finger_quality < 20 || finger_quality > 100)
	{
        	return MINEX_RET_BAD_IMAGE_SIZE;
	}
  	if (width  <MINEX_MIN_IMAGE_WIDTH  || height < MINEX_MIN_IMAGE_HEIGHT )
 	{
  		return MINEX_RET_BAD_IMAGE_SIZE; // in range 0.3..1.62 in
	}
  	/*if (height < MINEX_MIN_IMAGE_HEIGHT || height  > MINEX_MAX_IMAGE_HEIGHT)
	{
	 	return MINEX_RET_BAD_IMAGE_SIZE; // in range 0.3..2.0 in
	}*/
	unsigned char *new_data;
	if (width  >MINEX_MAX_IMAGE_WIDTH  || height > MINEX_MAX_IMAGE_HEIGHT )
 	{
  		//return MINEX_RET_BAD_IMAGE_SIZE; // in range 0.3..1.62 in
		new_data=crop_image(raw_image,height,width);
		//printf("new_width=%d",new_width);
		//printf("new_height=%d",new_height);

		//===============================================================================================
	
		size_t size = output_buffer;
  		if (size < output_buffer)                           return MINEX_RET_BAD_IMAGE_SIZE;

		unsigned int dt=FRFXLL_DT_ANSI_FEATURE_SET;
  	
  		Image_procc hContext, hFtrSet;
  		extractor_test( minutia_fP_enviromnent(&hContext) );
 		switch ( minutia_fetch_rawdata(hContext, reinterpret_cast<const unsigned char *>(new_data), new_width * new_height, new_width, new_height,500, 			0x00000002U, &hFtrSet ) ) 
		{

    			case MINEX_RET_SUCCESS:
      				break;
    			case MINEX_RET_FAILURE_BAD_IMPRESSION:
      				return MINEX_RET_FAILURE_BAD_IMPRESSION;
   			default:
      				return MINEX_RET_FAILURE_UNSPECIFIED;
  		}	
  		unsigned short int dpcm = (500 * 100 + 50) / 254;

  	result_formates param = {sizeof(result_formates), CBEFF, finger_position, 0, dpcm, dpcm, new_width, new_height, 0, finger_quality, impression_type};
  		unsigned char * tmpl = reinterpret_cast<unsigned char *>(output_template);

  		extractor_test( write_output_template(hFtrSet, dt, &param, tmpl, &size) );
  		return MINEX_RET_SUCCESS;
		

		//================================================================================================

	}
	
 	
  	size_t size = output_buffer;
  	if (size < output_buffer)                           return MINEX_RET_BAD_IMAGE_SIZE;

	unsigned int dt=FRFXLL_DT_ANSI_FEATURE_SET;
  	
  	Image_procc hContext, hFtrSet;
  	extractor_test( minutia_fP_enviromnent(&hContext) );
 	switch ( minutia_fetch_rawdata(hContext, reinterpret_cast<const unsigned char *>(raw_image), width * height, width, height,500, 			0x00000002U, &hFtrSet ) ) 
	{

    		case MINEX_RET_SUCCESS:
      			break;
    		case MINEX_RET_FAILURE_BAD_IMPRESSION:
      			return MINEX_RET_FAILURE_BAD_IMPRESSION;
   		default:
      			return MINEX_RET_FAILURE_UNSPECIFIED;
  	}	
  	unsigned short int dpcm = (500 * 100 + 50) / 254;

  	result_formates param = {sizeof(result_formates), CBEFF, finger_position, 0, dpcm, dpcm, width, height, 0, finger_quality, impression_type};
  	unsigned char * tmpl = reinterpret_cast<unsigned char *>(output_template);

  	extractor_test( write_output_template(hFtrSet, dt, &param, tmpl, &size) );
  	return MINEX_RET_SUCCESS;
}


int32_t crop_template()
{
	printf("i m in cropping function\n");
	return MINEX_RET_FAILURE_BAD_IMPRESSION;
}



int minutia_fP_enviromnent(
  void ** phContext          ///< [out] pointer to where to put an open handle to created context
) {

  minu_fetch_begin ci = {sizeof(ci)};
  ci.malloc = &memory_allocation;
  ci.free = &memory_free;
  if (CLOCKS_PER_SEC == (clock_t)1000)
  {
    ci.current_clock_millisec = &current_clock_millisec;
  }
  ci.interlocked_increment = memory_sync_increment;
  ci.interlocked_decrement = memory_sync_dec;
  ci.interlocked_exchange  = memory_sync_exch;
  ci.interlocked_compare_exchange = memory_sync_compare;
  return minutia_extractor_env(&ci, phContext);
}


int minutia_extractor_env(
  minu_fetch_begin * contextInit,  ///< [in] pointer to filled context initialization structure
  void ** phContext          ///< [out] pointer to where to put an open handle to created context
) {
 
  minu_fetch_begin ctxi = {0};
  if (contextInit == NULL) return test_output(MINEX_RET_FAILURE_NULL_TEMPLATE);
  if (phContext == NULL) return test_output(MINEX_RET_FAILURE_NULL_TEMPLATE);
  size_t length = contextInit->length;
  switch (length) {
    case offsetof(minu_fetch_begin, interlocked_increment):
    case sizeof(minu_fetch_begin):
	 break;
    default: return test_output(MINEX_RET_FAILURE_NULL_TEMPLATE);
  }
  memcpy(&ctxi, contextInit, length);
  const Context * ctx = new(&ctxi) Context(ctxi);
  if (ctx == NULL)
{
	 return test_output(MINEX_RET_FAILURE_UNSPECIFIED);
}
  Ptr<const Context> pctx = ctx;
  // TODO: update settings here as needed
 
  Context * rwCtx = const_cast<Context *>(ctx); 
  rwCtx->settings.fex.user_feedback.minimum_footprint_area = 0;
  rwCtx->settings.fex.user_feedback.minimum_number_of_minutia = 0;
  return ctx->data_proc(phContext);
}

int free_object(
  void * handle            ///< [in] Handle to close. Object is freed when the reference count is zero
) {
  if (handle == NULL) return MINEX_RET_SUCCESS; // closing NULL handle is not an error (like in case of operator delete)
  Handle * ph = reinterpret_cast<Handle *>(handle);
  if (ph->Check1()) {
    delete ph;
    return MINEX_RET_SUCCESS;
  }
  ConstHandle * pch = reinterpret_cast<ConstHandle *>(handle);
  if (pch->Check1()) {
    delete pch;
    return MINEX_RET_SUCCESS;
  }
  return test_output(MINEX_RET_FAILURE_UNSPECIFIED);
}




typedef minutia_object<data_ret::minutia_fetch1> FexObj;
int minutia_fetch_rawdata(
  void * hContext,          ///< [in] Handle to a fingerprint recognition context
  const unsigned char fpData[],    ///< [in] sample
  size_t size,                     ///< [in] size of the sample buffer
  unsigned int width,              ///< [in] width of the image
  unsigned int height,             ///< [in] heidht of the image
  unsigned int imageResolution,    ///< [in] image resolution [DPI]
  unsigned int flags,              ///< [in] Set to 0 for default or bitwise or of any of the  flags
  void ** phFeatureSet    ///< [out] pointer to where to put an open handle to the feature set
) {
  	if (fpData == NULL)
	{
	 	return test_output(MINEX_RET_FAILURE_NULL_TEMPLATE);
	}

  	if (phFeatureSet == NULL)
	{
 		return test_output(MINEX_RET_FAILURE_NULL_TEMPLATE);
	}

  	//test_formate_flag(flags, FRFXLL_FEX_DISABLE_ENHANCEMENT | FRFXLL_FEX_ENABLE_ENHANCEMENT);
		test_formate_flag(flags, 0x00000011U);
  	Ptr<const Context> ctx(hContext);

  	if (!ctx)
	{
 		return test_output(MINEX_RET_FAILURE_UNSPECIFIED);
	}
  	Ptr<FexObj> fex(new(ctx) FexObj(ctx));


  	if (!fex)
	{
 		return test_output(MINEX_RET_FAILURE_UNSPECIFIED);
	}
  return fex->minutia_points(fpData, size, width, height, imageResolution, flags, phFeatureSet);
}



int write_output_template(
  void * handle,          ///< [in] Handle to data object to export
  unsigned int dataType,     ///< [in] Type and format of data to export
  const result_formates * parameters,     ///< [in] parameters structure, specific to the data type
  unsigned char pbData[],      ///< [out] Buffer where to export the data, optional
  size_t * pcbData             ///< [in/out] Pointer where to store the length of exported data, optional
) {
  if (pcbData == NULL) {
    return MINEX_RET_FAILURE_NULL_TEMPLATE;
  }
  switch (dataType) 
{
    //case FRFXLL_DT_ISO_FEATURE_SET:
    case FRFXLL_DT_ANSI_FEATURE_SET:
      return call_func(&feature_object::Export, handle, dataType, parameters, pbData, pcbData);
  }
  return MINEX_RET_FAILURE_NULL_TEMPLATE;
}




int32_t get_pids(uint32_t *template_generator,uint32_t *template_matcher)
{
	*template_generator=CBEFF;
	*template_matcher=0;
	return MINEX_RET_SUCCESS;
}

int32_t match_templates(const uint8_t *verification_template,const uint8_t *enrollment_template,float *similarity)
{
	*similarity=-1;
	return MINEX_RET_SUCCESS;
}

unsigned char * crop_image(const uint8_t*raw_image,const uint16_t height,const uint16_t width)
{
	/*printf("-----------maximum size image found-------------");
	printf("height=%d \t width=%d \n",height,width);*/
	
	
	int k=0;
	const unsigned char*raw_new=reinterpret_cast<const unsigned char *>(raw_image);


	unsigned char **array = (unsigned char **) malloc(height * sizeof(unsigned char*));
 
    	for (int a=0; a<height; a++) 
    	{
        	array[a] = (unsigned char *) malloc(width* sizeof(unsigned char));
    	}

	for (int a=0;a<height;a++)
	{
		for(int b=0;b<width;b++)
		{
			array[a][b]=raw_new[k];
			k++;	
		}
	}
	//printarray(array,height,width);

	//---------------------------------------------------------------------------------------------------
	
	int i, j, x;
	
	int maxv, maxs;
	
	int startr, startc, endr, endc;

	int graydensity[height];
	 
	int graycount=0;
	
	int t_height, t_width;
	
	 
    	for(i=0; i<height; i++ )
    	{
		graycount=0;
	 
		for(j=0; j<width; j++)
        	{
			
			if (array[i][j]<=200)
			{	
				graycount++;		
			}
			
        	}
        
		graydensity[i] = graycount;
    	}

	int maximum = graydensity[0];
	int location;

    	for (x = 1; x < height; x++)
    	{
        	if (graydensity[x] > maximum)
        	{
        		maximum  = graydensity[x];
        		location = x;
        	}
    	}

	
	for (x = 0; x < width; x++)
	{
		if (array[location][x] <= 225)
		{
			startc = x;
			break;
		}
	}

	for (x = width-1; x >= 0; x--)
	{
		if (array[location][x] <= 225)
		{
			endc = x;
			break;
		}
	}

//-----------------------------------------------------------------------------------------


	t_width = endc - startc;
	if (t_width > 800 && t_width < 1000)
	{
		startc = startc + 100;
		endc = endc - 100;
		t_width = endc - startc;
	}
	else if (t_width > 1000 && t_width < 1200)
	{
		startc = startc + 200;
		endc = endc - 200;
		t_width = endc - startc;
	}
	else if (t_width > 1200 && t_width < 1400)
	{
		startc = startc + 300;
		endc = endc - 300;
		t_width = endc - startc;
	}
	else if (t_width > 1400 && t_width < 1600)
	{
		startc = startc + 400;
		endc = endc - 400;
		t_width = endc - startc;
	}
	else if (t_width > 1600 && t_width < 1800)
	{
		startc = startc + 500;
		endc = endc - 500;
		t_width = endc - startc;
	}
	else if (t_width > 1800 && t_width < 2000)
	{
		startc = startc + 600;
		endc = endc - 600;
		t_width = endc - startc;
	}


//-----------------------------------------------------------------------------------------------------------

	for (x=location; x>0; x--)
	{
		if (graydensity[x]==0)
		{
			break;
		}
	}

	startr=x;

	for (x=location; x<height; x++)
	{
		if (graydensity[x]==0)
		{
			break;
		}
	}

	endr=x;
	

	if (startr < 0)
	{
		startr=0;
	}

	if (endr > height)
	{
		endr=height;
	}

//------------------------------------------------------------------------------------------------------

	t_height = endr - startr;
	if (t_height > 800 && t_height < 1000)
	{
		startr = startr + 100;
		endr = endr - 100;
		t_height = endr - startr;
	}
	else if (t_height > 1000 && t_height < 1200)
	{
		startr = startr + 200;
		endr = endr - 200;
		t_height = endr - startr;
	}
	else if (t_height > 1200 && t_height < 1400)
	{
		startr = startr + 300;
		endr = endr - 300;
		t_height = endr - startr;
	}
	else if (t_height > 1400 && t_height < 1600)
	{
		startr = startr + 400;
		endr = endr - 400;
		t_height = endr - startr;
	}
	else if (t_height > 1600 && t_height < 1800)
	{
		startr = startr + 500;
		endr = endr - 500;
		t_height = endr - startr;
	}
	else if (t_height > 1800 && t_height < 2000)
	{
		startr = startr + 600;
		endr = endr - 600;
		t_height = endr - startr;
	}
	

//----------------------------------------------------------------------------------------------------------
	

	/*printf("Given Rows %d and Given Columns %d.\n", height, width);	
    	printf("Maximum element is present at location %d and it's value is %d.\n", location, maximum);	
	printf("Begin Col %d, End Col %d\n",startc,endc);
	printf("Begin Row %d, End Row %d\n",startr,endr);
	printf("%s\n","img_4.pgm");*/


	unsigned char *one_d;
	int l=0;
	one_d = (unsigned char*)malloc((t_width*t_height) * sizeof(unsigned char) );

	

	new_width=t_width;
	new_height=t_height;

	for(int a=startr;a<endr;a++)
	{
		for(int b=startc;b<endc;b++)
		{
			one_d[l]=array[a][b];
			//printf("%d",one_d[l]);
			l++;	
		}
	}
	
	return one_d;
	
	
	//write pgm
			/*FILE* pgmimg;
			
			int newwidth, newheight;
			
			newwidth = endc - startc;
			newheight = endr - startr;
			
			printf("Saving output pgm file %s\n", "img_4.pgm");

			pgmimg = fopen("img_4.pgm", "wb"); 
			
			// Writing Magic Number to the File 
			fprintf(pgmimg, "P2\n"); 

			// Writing Width and Height 
			fprintf(pgmimg, "%d %d\n", newwidth, newheight); 

			// Writing the maximum gray value 
			fprintf(pgmimg, "255\n"); 
			
			int count = 0; */
	
			/*----------READ GRAYSCALE DATA----------*/

			/*int tr, tc;
				
			
			for(tr=startr; tr < endr; tr++)
			{
				for(tc=startc; tc < endc; tc++)
				{
				  fprintf(pgmimg, "%d ", array[tr][tc]);
				}
				fprintf(pgmimg, "\n");
			}
			
			fclose(pgmimg);*/

	//---------------------------------------------------------------------------------------------------
	

}










