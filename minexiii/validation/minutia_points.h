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
#ifndef __MINUTIA_POINT_H
#define __MINUTIA_POINT_H

#include "minutia_datas.h"
#include "minutia_fetch.h"



namespace minutia_extractor {
  namespace minutia_datarec {
    using namespace minutia_data;
    template <class minutia_fetch1>
    struct minutia_object  : public Signature<0x65787446, 0x00727478>, public Object, public HResult {
      minutia_fetch1 fex;

      minutia_object(const Context * ctx_) 
        : Object(ctx_)
        , fex(ctx_->settings.fex)
      {
      }
      template <class T>
      int minutia_points(
        T data[],                      ///< [in] sample
        size_t size,                   ///< [in] size of the sample buffer
        unsigned int dataType,       ///< [in] type of the sample, for instance image format
        uint32       flags,
        void * * phFtrSet         ///< [out] handle to feature set
      ) {
        Ptr<feature_object> ftrSet(new(ctx) feature_object(ctx));
        if (!ftrSet) {
          rc = test_output(MINEX_RET_FAILURE_UNSPECIFIED);
          return rc;
        }

        switch (dataType) {
          case FRFXLL_DT_ANSI_381_SAMPLE:
            rc = fex.template FromFIRSample<ANSIImageRecord>(data, size, flags, ftrSet->fpFtrSet);
            break;
         /* case FRFXLL_DT_ISO_19794_4_SAMPLE:
            rc = fex.template FromFIRSample<ISOImageRecord>(data, size, flags, ftrSet->fpFtrSet);
            break;*/
          case base_fmtt:
            rc = fex.template FromFIRSample<RawImageRecord>(data, size, flags, ftrSet->fpFtrSet);
            break;
          default:
            rc = MINEX_RET_FAILURE_NULL_TEMPLATE;
            break;
        }
        return ftrSet->data_proc(phFtrSet, GetResult());
      }

      template <class T>
      int minutia_points(
        T data[],                      ///< [in] sample
        size_t size,                   ///< [in] size of the sample buffer
        uint32 width,                  ///< [in] width of the image
        uint32 height,                 ///< [in] heidht of the image
        uint32 dpi,                    ///< [in] image resolution [DPI]
        uint32 flags,                  ///< [in] select which features of the algorithm to use
        void * * phFtrSet       ///< [out] handle to feature set
      ) {
		
        if (width > 2000 || height > 2000)                         return MINEX_RET_FAILURE_UNSPECIFIED;
      
        if (width  <MINEX_MIN_IMAGE_WIDTH  || width > MINEX_MAX_IMAGE_WIDTH)   return MINEX_RET_FAILURE_UNSPECIFIED; // in range 0.3..1.62 in
        if (height < MINEX_MIN_IMAGE_HEIGHT || height  > MINEX_MAX_IMAGE_HEIGHT) return MINEX_RET_FAILURE_UNSPECIFIED; // in range 0.3..2.0 in
        
        Ptr<feature_object> ftrSet(new(ctx) feature_object(ctx));
        if (!ftrSet) {
          rc = test_output(MINEX_RET_FAILURE_UNSPECIFIED);
          return rc;
        }
        rc = fex.get_raw_data(data, size, width, height, dpi, flags, ftrSet->fpFtrSet);
        return ftrSet->data_proc(phFtrSet, GetResult());
      }
    };
  }
}
#endif // __MINUTIA_POINT_H
