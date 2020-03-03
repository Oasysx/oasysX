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
#ifndef __MINUTIA_FETCH_H
#define __MINUTIA_FETCH_H


#include "datafir.h"
#include "image_arr.h"

#include <stdlib.h>
#include <string.h>

#ifndef __CROSSWORKS_ARM
  #include <stdio.h>
#else
  #include <__debug_stdio.h>
#endif

namespace minutia_extractor {
namespace minutia_datarec {
namespace merged {
  typedef complex<int8> ori_t;

  template <size_t stride, size_t ori_scale>
  inline void origigina_raw_data(size_t width, size_t size, const uint8 * inImage, bool compute_footprint, ori_t * ori, uint8 * footprint) {
    static const size_t ori_scale2 = ori_scale * ori_scale;
    static const size_t ori_stride = stride / ori_scale;
    const size_t ori_width = width / ori_scale;
    const size_t ori_size = size / ori_scale2;
    const int16 filler = 255; // typical background value: TODO: compute from the image

    delay<int16, stride + 1> x100(width + 1, filler * 2), x102(width + 1, filler * 5), x10d(width + 1), x11d(width + 1);
    delay<int16, stride - 1> x103(width - 1, filler * 10);
    inImage += (width + 1) * 2 + 4;  // offset to put in sync with orientation map
    const uint8 * p0 = inImage;
    const uint8 * p1 = inImage + width - 1;

    delay<int16, stride> x10c(width, filler * 5), x101(width, filler * 25), x201(width), x221(width), x301(width), x321(width);
    delay<int16, 1> x0, x10(filler * 50), x11, x20, x21, x22, x30, x31, x32, x33;
    delay<complex<int32>, 1> dom;

    for (size_t y = 0; y < ori_size - ori_width; y += ori_width) {
      complex<int32> ori_mag[ori_stride] = {0};
      for (size_t i = ori_scale; i; --i) {
        for (size_t x = 0; x < ori_width; ++x) {
          complex<int32> z(0);
          for (size_t j = ori_scale; j; ++p0, ++p1, --j) {
            int16 cur = int16(*p0) + *p1;
            int16 v1 = x100(cur) + cur + x0(*p0);
            int16 v2 = x102(v1) + v1;
            v1 = x103(v2) + v2 + x10c(v1);
            int16 v1x = x101(v1);
            int16 v10 = v1x + v1;
            v10 = x10(v10) - v10;
            int16 v11 = v1x - v1;
            v11 = x11(v11) + v11;
            complex<int32> Z1(v10, v11);
            Z1 = Z1 * Z1;

            int16 v10x = x201(v10);
            int16 v20 = v10x + v10;
            v20 = x20(v20) - v20;
            int16 v21 = v10x - v10;
            v21 = x21(v21) + v21;
            int16 v22 = x221(v11) - v11;
            v22 = x22(v22) + v22;

            int32 G20 = v20 + v22;
            int32 G21 = v20 - v22;
            int32 G22 = 2 * v21;
            complex<int32> Z2(G20 * G21, G20 * G22);

            int16 v20x = x301(v20);
            int16 v30 = v20x + v20;
            int32 X30 = x30(v30) - v30;
            int16 v31 = v20x - v20;
            int32 X31 = x31(v31) + v31;
            int16 v22x = x321(v22);
            int16 v32 = v22x + v22;
            int32 X32 = x32(v32) - v32;
            int16 v33 = v22x - v22;
            int32 X33 = x33(v33) + v33;

            complex<int32> Z3(5*(X30*X30 - X33*X33) + 3*(X31*X31 - X32*X32) + 2*(X30*X32 - X31*X33),
                              10*(X32*X33 + X30*X31) + 18*X31*X32 + 2*X30*X33);

            const int32 k1 = 0; //1; // 20;
            const int32 k2 = 1; // 24;
            const int32 k3 = 0; // 1;
            complex<int32> Z = k1 * Z1 + k2 * Z2 + k3 * Z3;
            z += Z;
          }
          ori_mag[x] += z;
        }
      }
      for (size_t x = 0; x < ori_width; ++x) {
        ori_t o = oct_sign(dom(ori_mag[x]), 50000); // 100000
        if (compute_footprint) footprint[x + y] = (o != ori_t(0)) ? 1 : 0;
        if (ori)               ori[x + y] = o;
      }
    }
    for (size_t x = ori_size - ori_width; x < ori_size; ++x) {
      if (compute_footprint) footprint[x] = 0;
      if (ori)               ori[x] = ori_t(0);
    }
  }

  template <size_t stride, size_t n, class F>
  inline void raw_data_filt(size_t width, size_t size, ori_t * ori, const uint8 * footprint, const F & postproc) {
    const static size_t n1 = n - 1;
    complex<int16> o1[stride] = {0}, o2[stride + n] = {0};
    delay<complex<int16>, n*stride> od1(n*width), od2(n*width);
    for (size_t y = 0; y < size + n1*width; y += width) {
      ori_t * pori = ori + y;
      delay<complex<int32>, n> od3(n), od4(n);
      complex<int32> o3(0), o4(0);
      for (size_t x = 0; x < width + n1; ++x) {
        complex<int32> o2t(0);
        if (x < width) {
          o1[x] += (y < size) ? pori[x] : 0;
          o2[x] += o1[x] - od1(o1[x]);
          o2t = o2[x] - od2(o2[x]);
        }
        if (y >= n1 * width) {
          o3 += o2t;
          o4 += o3 - od3(o3);
          if (x < n1) {
            od4(o4);
          } else {
            o2t = o4 - od4(o4);
            size_t pos = y + x - (width+1)*n1;
            ori[pos] = footprint[pos] ? postproc(o2t) : ori_t(0);
          }
        }
      }
    }
  }

  template <size_t stride, size_t boxsize, class T, class F>
  inline void boxfilt(size_t width, size_t size, T * inout, const F & f) {
    static const size_t n = boxsize;
    static const size_t n2 = boxsize / 2;
    T s1[stride] = {0};
    delay<T, n*stride> d1(n*width);
    for (size_t y = 0; y < size + n2*width; y += width) {
      T * p = inout + y;
      delay<T, n> d2(n);
      T s2(0);
      for (size_t x = 0; x < width + n2; ++x) {
        T t(0);
        if (x < width) {
          T v = (y < size) ? p[x] : 0;
          s1[x] += v;
          t = s1[x] -= d1(v);
        }
        if (y >= n2*width) {
          s2 += t; // SmallerTypeCheck
          if (x < n2) {
            d2(t);
          } else {
            t = s2 -= d2(t);
            t = f(t);
            inout[y - (width+1)*n2 + x] = t;
          }
        }
      }
    }
  }

  template <class T> inline T self(const T & x) { return x; }
  template <size_t stride, size_t boxsize, class T>
  inline void boxfilt(size_t width, size_t size, T * inout) {
    return boxfilt<stride, boxsize>(width, size, inout, self<T>);
  }

  template <uint8 N> inline uint8 g(uint8 x) { return x > N ? 1 : 0; }

  template <size_t stride, size_t size>
  inline void raw_data_root(ori_t * ori, uint8 * footprint) {
    for (size_t i = 0; i < size; i ++) {
      ori_t * pori = ori + i;
      if (pori[0] != ori_t(0)) {
        uint8 a = minutia_extractor::atan2(pori[0].real(), pori[0].imag()) / 2;
        pori[0] = footprint[i] ? complex<int16>(cos(a), sin(a)) : 0;
      }
    }
  }

  
  inline void data_bit_ins(size_t stride_x, size_t size_x, size_t stride_y, size_t size_y, uint8 * footprint) {
    for (size_t y = 0; y < size_y; y += stride_y) {
      uint8 * p = footprint + y;
      size_t x1, x2;
      for (x1 = 0; x1 < size_x; x1 += stride_x) {
        if (p[x1]) break;
      }
      for (x2 = size_x - stride_x; x2 > x1; x2 -= stride_x) {
        if (p[x2]) break;
      }
      for (size_t x = x1 + stride_x; x < x2; x += stride_x) {
        p[x] = 1;
      }
    }
  }

  template <int32 threshold> inline ori_t oct_sign_tr(const complex<int32> & x) {
    return oct_sign(x, threshold);
  }

  inline ori_t raw_data_division(const complex<int32> & x) {
    uint8 a = minutia_extractor::atan2(x.real(), x.imag()) / 2;
    return ori_t(cos(a), sin(a));
  }

  template <size_t stride, size_t ori_scale>
  inline void data_oriand_base(
    size_t width, 
    size_t size, 
    const uint8 * inImage, 
    bool compute_footprint, 
    ori_t * ori, 
    uint8 * footprint
  ) {
    #define ori_stride (stride / ori_scale)  // Workaround for ARM compiler: static const does not work
    const size_t ori_width = width / ori_scale;
    const size_t ori_size = size / ori_scale / ori_scale;

    origigina_raw_data<stride, ori_scale>(width, size, inImage, compute_footprint, ori, footprint);
    if (compute_footprint) {
      boxfilt<ori_stride, 3>(ori_width, ori_size, footprint, g<3>);
      boxfilt<ori_stride, 5>(ori_width, ori_size, footprint, g<11>);
      boxfilt<ori_stride, 5>(ori_width, ori_size, footprint, g<11>);
      //boxfilt<ori_stride, 5>(ori_width, ori_size, footprint, g<11>);
      data_bit_ins(1, ori_width, ori_width, ori_size, footprint);
      data_bit_ins(ori_width, ori_size, 1, ori_width, footprint);
      //boxfilt<ori_stride, 5>(ori_width, ori_size, footprint, g<14>);
      boxfilt<ori_stride, 5>(ori_width, ori_size, footprint, g<14>);
      boxfilt<ori_stride, 5>(ori_width, ori_size, footprint, g<14>);
    }
    if (ori) {
      raw_data_filt<ori_stride, 5>(ori_width, ori_size, ori, footprint, oct_sign_tr<128>);
      raw_data_filt<ori_stride, 5>(ori_width, ori_size, ori, footprint, raw_data_division);
    }
    #undef ori_stride
  }

  using namespace minutia_fetch;

  inline void off_added(compare_minutia & md){
    static const int16 xMax = 255, yMax = 399;
    int16 x0 = std::numeric_limits<int16>::max();
    int16 x1 = std::numeric_limits<int16>::min();
    int16 y0 = std::numeric_limits<int16>::max();
    int16 y1 = std::numeric_limits<int16>::min();
    size_t i;
    for (i = 0; i < md.size(); i++) {
      int16 x = md.minutia[i].position.x;
      int16 y = md.minutia[i].position.y;
      x0 = std::min(x0, x);
      x1 = std::max(x1, x);
      y0 = std::min(y0, y);
      y1 = std::max(y1, y);
    }
    int16 xoffs = 0;
    if (x1 > xMax) {
      xoffs = (x0 + x1 - xMax) >> 1;
      md.offset.x += xoffs;
    }
    int16 yoffs = 0;
    if (y1 > yMax) {
      yoffs = std::min(y0, int16(y1 - yMax));
      md.offset.y += yoffs;
    }
    //size_t j = 0;
    for (i = 0; i < md.size(); i++) {
      int16 x = md.minutia[i].position.x -= xoffs;
      int16 y = md.minutia[i].position.y -= yoffs;
      if (x < 0 || x > xMax || y < 0 || y > yMax) {
        memcpy(md.minutia + i, md.minutia + i + 1, ((md.size() - (i + 1)) * sizeof(*md.minutia)));
        i--;
        md.numMinutia--;
      }
    }
  }

  struct minutia_extract {
    static const size_t maxwidth = 256;
    static const size_t maxheight = 400;

    static const size_t int_resolution = 333;
    static const size_t int_resolution_min_in_place = 362; // ~= 333 * sqrt((4^2+3)/4^2)
    static const size_t ori_scale = 4;
    static const int32 imageScale = 127 * 5;
    // flags for reserved (flags) parameters
    static const uint32 flag_disable_fft_enhancement = 0x1;
    static const uint32 flag_enable_fft_enhancement  = 0x2;

    static const uint8 enh_block_bits = 5;
    static const int32 enh_spacing = 17;

    const uint8 * imgIn;
    uint8 * img;
    size_t height;
    size_t width;
    size_t size;

    int32 xOffs; // Phasemap offset
    int32 yOffs; // Phasemap offset

    uint8 * footprint;
    ori_t * ori;
    size_t ori_width;
    size_t ori_size;
    uint16 imageResolution; // In DPI
    uint8 readerCode;
    const minutia_fetch::Parameters & param;

    minutia_extract(const minutia_fetch::Parameters & param_)
      : imgIn(NULL)
      , img(NULL)
      , height(0)
      , width(0)
      , size(0)
      , xOffs(1)
      , yOffs(6)
      , footprint(NULL)
      , ori(NULL)
      , ori_width(0)
      , ori_size(0)
      , imageResolution(0)
      , readerCode(0)
      , param(param_)
    {}
    
    void input_size_back() {
      ori_width = int((width - 1) / ori_scale + 1);
      size_t ori_height = int((size/width - 1) / ori_scale + 1);
      ori_size = ori_height * ori_width;
    }

    void fp_db_ins(datapoint & fp, const locate & offset) {
      uint32 area = 0;
      for (int32 y = 0; y < fp.height; y++) {
        for (int32 x = 0; x < fp.width; x++) {
          size_t xi = reduce(((x * 8 + offset.x) * imageResolution + imageResolution / 2) / imageScale, 2);
          size_t yi = reduce(((y * 8 + offset.y) * imageResolution + imageResolution / 2) / imageScale, 2) * ori_width;
          bool b = (xi < ori_width) && (yi < ori_size) && (footprint[xi + yi]);
          if (b) area++;
          fp.Pixel(x,y) = b;
        }
      }
      fp.area = area * 8 * 8;
    }
    void head_update(compare_minutia & md) {
      md.offset.x = int16(xOffs * imageScale / imageResolution);
      md.offset.y = int16(yOffs * imageScale / imageResolution);
    }

    void minutia_rearrange(compare_minutia & md) {
      for (size_t i = 0; i < md.size(); i++) {
        md.minutia[i].position.x = int16(md.minutia[i].position.x * imageScale / imageResolution);
        md.minutia[i].position.y = int16(md.minutia[i].position.y * imageScale / imageResolution);
      }
    }

    int get_data_ppi(
      uint32 flags,
      size_t buffer_size,
      compare_minutia &md
    ) {
      ori = reinterpret_cast<ori_t *>(img + size);
      if ((flags & flag_enable_fft_enhancement) != 0) {
        size_t enh_buffer_size = size + max(ori_size * sizeof(ori_t), width << enh_block_bits);
        AssertR(enh_buffer_size + ori_size <= buffer_size, MINEX_RET_FAILURE_UNSPECIFIED); 
        footprint = img + enh_buffer_size;
        data_oriand_base<maxwidth, ori_scale>(width, size, img, true, NULL, footprint);
        if (! four_enh<enh_block_bits, enh_spacing>(img, width, size, enh_buffer_size)) {
          return MINEX_RET_FAILURE_UNSPECIFIED;
        }
        data_oriand_base<maxwidth, ori_scale>(width, size, img, false, ori, footprint);
      } else {
        footprint = img + size + ori_size * sizeof(ori_t);
        data_oriand_base<maxwidth, ori_scale>(width, size, img, true, ori, footprint);
      }
      freeman_phasemap<ori_scale>(width, size, img, ori, img);
      
      top_n<minutia_param> top_minutia(md.minutia, md.minutia + md.capacity());
      extract_minutia<maxwidth, ori_scale>(img, width, size, footprint, top_minutia, param);
      md.numMinutia = top_minutia.size();
      top_minutia.sort();
      minutia_rearrange(md);
      head_update(md);                // order is important : this fills offset
      off_added(md);
      fp_db_ins(md.footprint, md.offset);  // this uses offset
      if (   md.size() < param.user_feedback.minimum_number_of_minutia
          || md.footprint.area < param.user_feedback.minimum_footprint_area) {
        return MINEX_RET_FAILURE_BAD_IMPRESSION;
      }
      return MINEX_RET_SUCCESS;
    }

    template <class T_FIR>
    int get_img_header(
      const uint8 data[],             ///< [in] sample buffer
      size_t buffer_size              ///< [in] size of the sample buffer
    ) {
      T_FIR record(data, buffer_size);
      if (!record.IsValid()) return record;
      CheckR(Init(record.pData, buffer_size, record.Header.get_wid(), record.Header.get_heig(), record.Header.GetResolution()));
      CheckR(record.Header.GetDeviceCode(readerCode));
      return MINEX_RET_SUCCESS;
    }

    int Init(
      const uint8 data[],             ///< [in] sample buffer
      size_t size_,                   ///< [in] size of the sample buffer
      uint32 width_,                  ///< [in] width of the image
      uint32 height_,                 ///< [in] heidht of the image
      uint32 imageResolution_         ///< [in] pixel resolution [DPI]
    ) {
      AssertR(size_ >= width_ * height_, MINEX_RET_FAILURE_UNSPECIFIED);
      imgIn = data;
      width = width_;
      height = height_;
      size = width_ * height_;
      imageResolution = imageResolution_;
      xOffs = 1;
      yOffs = 6;
      return MINEX_RET_SUCCESS;
    }

    int origin_to_databuff(
      const uint8 in_img[],           ///< [in] input sample buffer, not preserved
      uint8 buffer[],                 ///< [] working buffer that is modified, can be the same as image buffer, or start before it
      size_t buffer_size              ///< [in] size of the working buffer
    ) {
      AssertR(width != 0 && size != 0, MINEX_RET_FAILURE_UNSPECIFIED);

      AssertR(height >= 32 && width >= 32, MINEX_RET_FAILURE_UNSPECIFIED);

      size_t in_width = width, in_height = height;
      if (imageResolution <= 550 && imageResolution >= 450) {
       
        width  = in_width  / 6 * 4; // width should be multiple of 4
        height = in_height / 6 * 4;
      } else {
        width  = (in_width  * int_resolution / imageResolution) & ~(ori_scale - 1);
        height = (in_height * int_resolution / imageResolution) & ~(ori_scale - 1);
      }

      if (height > maxheight) {        // center the image vertically
        size_t maxheight_in = (imageResolution <= 550 && imageResolution >= 450) ? maxheight * 3 / 2 : maxheight * imageResolution / int_resolution;
        size_t diff = in_height - maxheight_in;
        AssertR(diff > 0, MINEX_RET_FAILURE_UNSPECIFIED);
        in_img += (diff / 2) * in_width;
        yOffs += int32((height - maxheight) / 2);
        height = maxheight;
      }

      if (width > maxwidth) {        // center the image horizontally
        size_t maxwidth_in = (imageResolution <= 550 && imageResolution >= 450) ? maxwidth * 3 / 2 : maxwidth * imageResolution / int_resolution;
        size_t diff = in_width - maxwidth_in;
        AssertR(diff > 0, MINEX_RET_FAILURE_UNSPECIFIED);
        in_img += diff / 2;
        xOffs += int32((width - maxwidth) / 2);
        width = maxwidth;
      }

      size = width * height;

      AssertR(width != 0 && size != 0, MINEX_RET_FAILURE_UNSPECIFIED);

      input_size_back();
      size_t buffer_size_needed = size + ori_size * (1 + sizeof(ori_t));
      AssertR(imageResolution >= int_resolution_min_in_place || in_img < buffer || in_img >= buffer + size, MINEX_RET_FAILURE_NULL_TEMPLATE); 
      AssertR(buffer_size_needed <= buffer_size, MINEX_RET_FAILURE_UNSPECIFIED); //FRFXLL_ERR_INVALID_BUFFER); // FRFXLL_ERR_FB_TOO_SMALL_AREA);//

      if (imageResolution <= 550 && imageResolution >= 450) {
        imresize23<maxwidth>(buffer, width, size, in_img, in_width);
      } else if (imageResolution <= int_resolution + 33 && imageResolution >= int_resolution - 33 && (in_width % 4) == 0) {
        memmove(buffer, in_img, size);
        imageResolution = muldiv(imageResolution, 5, 3);
      } else {
        size_t scale256 = imageResolution * size_t(256) / int_resolution;
        imresize(buffer, width, size, in_img, in_width, scale256);
        imageResolution = 500;
      }
      return MINEX_RET_SUCCESS;
    }

    int minu_ext (
      const uint8 in_img[],           ///< [in] input sample buffer, not preserved
      uint8 buffer[],                 ///< [] working buffer that is modified, can be the same as image buffer, or start before it
      size_t buffer_size,             ///< [in] size of the working buffer
      uint32 flags,
      compare_minutia &md
    ) {
      CheckR(origin_to_databuff(in_img, buffer, buffer_size));
      img = buffer;
      return get_data_ppi(flags, buffer_size, md);
    }
  };

  struct minutia_point_locate : public minutia_extract {

    minutia_point_locate(const minutia_fetch::Parameters & param_)
      : minutia_extract (param_)
    {}

    template <class T_FIR>
    int get_sample_record(
      uint8 data[],                   ///< [in] sample buffer, modified to save memory
      size_t data_size,               ///< [in] size of the sample buffer
      uint32 flags,
      compare_minutia &md
    ) {
      CheckR(get_img_header<T_FIR>(data, data_size));
      AssertR(imageResolution >= int_resolution_min_in_place, MINEX_RET_FAILURE_UNSPECIFIED);
      return minu_ext(imgIn, data, data_size, flags, md);
    }

    int get_raw_data(
      uint8 data[],                   ///< [in] sample buffer
      size_t size_,                   ///< [in] size of the sample buffer
      uint32 width_,                  ///< [in] width of the image
      uint32 height_,                 ///< [in] heidht of the image
      uint32 imageResolution_,        ///< [in] pixel resolution [DPI]
      uint32 flags,
      compare_minutia &md
    ) {
      CheckR(Init(data, size_, width_, height_, imageResolution_));
      AssertR(imageResolution >= int_resolution_min_in_place, MINEX_RET_FAILURE_UNSPECIFIED);
      return minu_ext(imgIn, data, size_, flags, md);
    }

  };

  /// Contains the temporary buffer, so the size is larger, that's why we create a separate class
  struct minutia_fetch1 : public minutia_extract {
    static const size_t maxsize = maxwidth * (maxheight + maxheight / 4);
    uint8 buffer[maxsize];

    minutia_fetch1(const minutia_fetch::Parameters & param_)
      : minutia_extract (param_)
    {
    }

    template <class T_FIR>
    int get_sample_record(
      const uint8 data[],             ///< [in] input sample buffer, preserved
      size_t data_size,               ///< [in] size of the input sample buffer
      uint32 flags,
      compare_minutia &md
    ) {
      CheckR(get_img_header<T_FIR>(data, data_size));
      return minu_ext(imgIn, buffer, maxsize, flags, md);
    }

    int get_raw_data(
      const uint8 data[],             ///< [in] sample buffer
      size_t size_,                   ///< [in] size of the sample buffer
      uint32 width_,                  ///< [in] width of the image
      uint32 height_,                 ///< [in] heidht of the image
      uint32 imageResolution_,        ///< [in] pixel resolution [DPI]
      uint32 flags,                   ///< [in] select which features of the algorithm to use
      compare_minutia &md
    ) {
      CheckR(Init(data, size_, width_, height_, imageResolution_));
      return minu_ext(imgIn, buffer, maxsize, flags, md);
    }
  };

}
}
}

#endif // __MINUTIA_FETCH_H

