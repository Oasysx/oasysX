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
#ifndef __minexiii_create_h
#define __minexiii_create_h

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#define FJFX_FMD_ANSI_378_2004        (0x001B0201)   // ANSI INCIT 378-2004 data format

#define output_buffer         (32 + 128 * 6) // Output data buffer must be at least this size, in bytes (34 bytes for header + 6 bytes per minutiae point, for up to 256 minutiae points)

int32_t create_template(const uint8_t*raw_image, const uint8_t finger_quality,const uint8_t finger_position,const uint8_t impression_type,const uint16_t height,const uint16_t width,uint8_t *output_template);

int32_t get_pids(uint32_t *template_generator,uint32_t *template_matcher);

int32_t match_templates(const uint8_t *verification_template,const uint8_t *enrollment_template,float *similarity);

int32_t crop_template();


#ifndef data_write_temp
#if defined(__GNUC__) && (__GNUC__ >= 4)
#define data_write_temp                    // __attribute__((visibility("default")))
#else
#define data_write_temp
#endif
#endif // data_write_temp


#define base_fmtt           ((unsigned int)0x00000001)  ///< Sample in RAW format
#define FRFXLL_DT_ANSI_381_SAMPLE      ((unsigned int)0x001B0401)  ///< Sample in ANSI 381-2004 data format

typedef struct input_data_ext {
  unsigned short width, height, dpi, reserved;
  unsigned char * pixels;
} input_data_ext; // to be used with create features

typedef struct minu_fetch_begin {
  size_t length;        ///< Length of this structure: for extensibility
  void * heapContext;   ///< Context passed into heap functions. Heap should be ready to use prior to calling minu_contest
  void *    (*malloc)   (size_t size, void * heapContext);      ///< Function to allocate block of memory on the heap
  void      (*free)     (void * block, void * heapContext);     ///< Function to free block of memory on the heap
  unsigned int (*current_clock_millisec)( void );               ///< Get current tick count im milliseconds, optional. If not supplied timeout parameter is not honored
  long (*interlocked_increment) (volatile long *);              ///< optional
  long (*interlocked_decrement) (volatile long *);              ///< optional
  long (*interlocked_exchange)  (volatile long *, long);        ///< optional
  long (*interlocked_compare_exchange) (volatile long *, long, long);  ///< optional
} minu_fetch_begin;

int data_write_temp minutia_extractor_env(
  minu_fetch_begin * contextInit,  ///< [in] pointer to filled context initialization structure
  void ** phContext          ///< [out] pointer to where to put an open handle to created context
);

int data_write_temp minutia_fP_enviromnent(
  void ** phContext          ///< [out] pointer to where to put an open handle to created context
);

int data_write_temp free_object(
  void * handle            ///< [in] Handle to close. Object is freed when the reference count is zero
);

int minutia_fetch_rawdata(
  void * hContext,          ///< [in] Handle to a fingerprint recognition context
  const unsigned char pixels[],    ///< [in] sample as 8bpp pixel array (no line padding for alignment)
  size_t size,                     ///< [in] size of the sample buffer
  unsigned int width,              ///< [in] width of the image
  unsigned int height,             ///< [in] heidht of the image
  unsigned int imageResolution,    ///< [in] image resolution [DPI]
  unsigned int flags,              ///< [in] Set to 0 for default or bitwise or of any of the FRFXLL_FEX_xxx flags
  void ** phFeatureSet    ///< [out] pointer to where to put an open handle to the feature set
);

#define FRFXLL_DT_ANSI_FEATURE_SET    ((unsigned int)0x001B8001)  ///< Fingerprint feature set in ANSI data format.

typedef struct result_formates {
  size_t length;                     ///< size of this structure, for extensibility
  unsigned int    CBEFF;
  unsigned short  fingerPosition;
  unsigned short  viewNumber;
  unsigned short  resolutionX;       ///< in pixels per cm
  unsigned short  resolutionY;       ///< in pixels per cm
  unsigned short  imageSizeX;        ///< in pixels
  unsigned short  imageSizeY;        ///< in pixels
  unsigned char   rotation;          ///< rotation in degrees clockwise> * 128 / 180, keep in mind rounding
  unsigned char   fingerQuality;     ///< finger quality is a mandatory field ISO 19794-2 and ANSI 378, 
  unsigned char   impressionType;    ///< finger impression type
} result_formates;


int data_write_temp write_output_template(
  void * handle,          ///< [in] Handle to data object to export
  unsigned int dataType,     ///< [in] Type and format of data to export
  const result_formates * parameters,     ///< [in] parameters structure, specific to the data type
  unsigned char pbData[],        ///< [out] Buffer where to export the data, optional
  size_t * pcbData               ///< [in/out] Pointer where to store the length of exported data, optional
);



#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __minexiii_create_h
