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
#ifndef __FILLPIXE_H
#define __FILLPIXE_H



#include <stdlib.h>
#include <string.h>

#ifndef __CROSSWORKS_ARM
  #include <stdio.h>
#else
  #include <__debug_stdio.h>
#endif


#include "complex"
#include "limits"
#include "scale_limits.h"


namespace minutia_extractor {

  namespace minutia_capture {

    #define FRFXLL_DIAG(pointer, method, ...) if (pointer.Valid()) (pointer).ptr->method(__VA_ARGS__)

    template <class T>
    struct pointer_subscribe {
      T * ptr;
      pointer_subscribe() : ptr(0) {}
      bool Valid() const { return ptr != 0; }
    };

    template <class T>
    struct subscribe {
      typedef pointer_subscribe< subscribe<T> > ptr_t;
      virtual void SetData(const T & data) {}
    };

    template <class T>
    struct pixel_subscribe {
      typedef pointer_subscribe< pixel_subscribe<T> > ptr_t;

      virtual void SetSize(size_t width, size_t height) {}
      virtual void SetPixel(size_t x, size_t y, const T & pixel) {}
    };

  }

namespace minutia_datarec {
namespace merged {


      using std::numeric_limits;
      using std::complex;

      template <class T> 
      inline T sign(const T & x) {
        return x > 0 ? 1 : x < 0 ? -1 : 0;
      }

      template <class T> 
      inline T oct_abs(const std::complex<T> & t) {
        T x = minutia_extractor::abs(t.real());
        T y = minutia_extractor::abs(t.imag());
        T n = minutia_extractor::max(x, y);
        return minutia_extractor::max(n, minutia_extractor::reduce((x + y) * 181, 8));
      }

      template <class T> 
      inline complex<int8> oct_sign(const std::complex<T> & t, const T & threshold = 0) {
        T x = minutia_extractor::abs(t.real());
        T y = minutia_extractor::abs(t.imag());
        T n = minutia_extractor::max(x, y) / 127;
        if (n <= threshold) return 0;
        n = minutia_extractor::max(n, (x + y) / 180);
        return complex<int8>(t / n);
      }

      template <class T> 
      inline complex<T> operator >> (const std::complex<T> & t, uint8 n) {
        return complex<T>(t.real() >> n, t.imag() >> n);
      }

      template <class T> 
      inline complex<T> operator << (const std::complex<T> & t, uint8 n) {
        return complex<T>(t.real() << n, t.imag() << n);
      }

      template <class T> 
      inline complex<T> & operator <<= (std::complex<T> & t, uint8 n) {
        t = t << n;
        return t;
      }

      using minutia_extractor::reduce;

      template <class T> 
      inline complex<T> reduce(const std::complex<T> & a, uint8 n){
        const T adder = 1 << (n-1);
        return (a + complex<T>(adder, adder)) >> n;
      }

      template <class T, size_t size> struct delay {
        T buffer[size];
        T * ptr;
        T * end;
        delay(size_t dt, const T & init = T(0)) : ptr(buffer), end(buffer + dt) {
          while(ptr < end) {
            *ptr++ = init;
          }
          ptr = buffer;
        }
        T operator () (T in) {
          T out = *ptr;
          *ptr = in;
          if (++ptr >= end) {
            ptr = buffer;
          }
          return out;
        }
        T operator () () const {
          return *ptr;
        }
      };

      template <class T> struct delay<T,1> {
        T prev;
        T * ptr;
        delay(const T & init = T(0)) : prev(init) {
        }
        T operator () (T in) {
          T out = prev;
          prev = in;
          return out;
        }
        operator T () const {
          return prev;
        }
      };

      template <size_t size> struct delay<bool, size> {
        typedef unsigned char byte;
        static const size_t bytesize = (size + 7) / 8;
        byte buffer[bytesize];
        byte * ptr;
        byte * end;
        byte mask;
        const byte initmask;

        delay(size_t dt, bool init_val = false) 
          : ptr(buffer)
          , end(buffer + (dt + 7) / 8)
          , initmask(1 << (-dt & 7)) 
        {
          mask = initmask;
          while(ptr < end) {
            *ptr++ = init_val ? 0xff : 0;
          }
          ptr = buffer;
        }
        bool operator () (bool in) {
          bool out = (*ptr & mask) != 0;
          if (in) {
            *ptr |= mask;
          } else {
            *ptr &= ~mask;
          }
          if ((mask <<= 1) == 0) {
            if (++ptr >= end) {
              ptr = buffer;
              mask = initmask;
            } else {
              mask = 1;
            }
          }
          return out;
        }
        bool operator () () const {
          return (*ptr & mask) != 0;
        }
      };


namespace minutia_fetch {
#define phasemap_filler ((uint8)127) 
  
  // Outside the footprint is encoded with 255 (filler), normal footprint is in range [0, 254]
  template <size_t ori_scale, class Tout, class ori_t> 
  void freeman_phasemap(size_t width, size_t size, const uint8 * inImage, ori_t * ori, Tout * outImage) {
    static const size_t ori_scale2 = ori_scale * ori_scale;
    const size_t ori_width = width / ori_scale;
    static const size_t flt_size2 = 4; // filter half-size

    const uint8 * p = inImage + (width+1) * flt_size2;
    Tout * p2 = outImage;
    // Uncomment this if you want to try IIR instead of FIR when running in-place 
    // #define IIR
  #ifdef IIR
    for (size_t i = width * flt_size2; i; --i) {
      *p2++ = phasemap_filler;
    }
  #endif

    conv9<-112, -7, 48, 14, 1> x20;
    conv9<122, 78, 20, 2, 0> x22, x33;
    conv9<0, 71, 37, 6, 0> x21;
    conv9<0, -92, -12, 8, 1> x30;
    conv9<0, 52, 27, 4, 0> x32;
    conv9<-90, -23, 21, 7, 1> x31;

    for (size_t y = ori_width; y < size / ori_scale2 - ori_width; y += ori_width) {
      ori_t * pori = ori + y;
      for (size_t i = ori_scale; i; --i) {
        for (size_t x = 0; x < ori_width; ++x) {
          int32 a10 = pori[x].real();
          int32 a11 = pori[x].imag();
      
          int32 a20 = sincosnorm(a10 * a10);
          int32 a21 = sincosnorm(2 * a10 * a11);
          int32 a22 = sincosnorm(a11 * a11);
          
          int32 a30 = sincosnorm(a10 * a20);
          int32 a31 = sincosnorm(3 * a20 * a11);
          int32 a32 = sincosnorm(3 * a22 * a10);
          int32 a33 = sincosnorm(a11 * a22);

          for (size_t j = flt_size2; j; ++p, ++p2, --j) {
            int32 v20 = x20(x22.vert(width, p));
            int32 v21 = x21(x21.vert(width, p));
            int32 v22 = x22(x20.vert(width, p));

            int32 v30 = x30(x33.vert(width, p));
            int32 v31 = x31(x32.vert(width, p));
            int32 v32 = x32(x31.vert(width, p));
            int32 v33 = x33(x30.vert(width, p));

            int32 x2 = a20 * v20 + a21 * v21 + a22 * v22;
            int32 x3 = a30 * v30 + a31 * v31 + a32 * v32 + a33 * v33; 
            x2 = (x2 + 1024) >> 11;
            x3 = (x3 + 1024) >> 11;
            *p2 = x2 ? ((127 - oct_sign(complex<int32>(x2, x3)).real()) & 0xf0) : phasemap_filler;     // 27.3M
          }
        }
      }
    }
  #ifdef IIR
    for (size_t i = width * flt_size2; i; --i) {
  #else
    for (size_t i = width * flt_size2 * 2; i; --i) {
  #endif 
      *p2++ = phasemap_filler;
    }
  }


  struct Templat {
    static const uint8 bits = 7;
    static const uint32 scale = 1 << bits;

    const size_t width;
    const size_t height;
    const size_t size;
    const uint8 * img;

    Templat(size_t width_, size_t size_, const uint8 * img_) 
      : width(width_), height(size_/width_), size(size_), img(img_) {
    }
    const uint8 * ptr(size_t x, size_t y) const {
      return img + x + y * width;
    }
    // bilinear interpolation (x, y are multiplied by 128)
    uint8 operator () (size_t x, size_t y) const {
      uint32 wx1 = x & (scale - 1);
      uint32 wy1 = y & (scale - 1);
      uint32 wx0 = (scale - wx1);
      uint32 wy0 = (scale - wy1);
      size_t x0 = x >> bits;
      size_t y0 = y >> bits;
      if ((y0 > height - 1) || (x0 > width - 1)) {
        return phasemap_filler;
      }
      const uint8 * p0 = ptr(x0, y0);
      uint32 v = p0[0]*wx0*wy0 + p0[1]*wx1*wy0 + p0[width]*wx0*wy1 + p0[width+1]*wx1*wy1;
      return uint8(minutia_extractor::reduce(v, bits * 2));
    }
  };

  struct fill_pixe {
    struct paramet {
      int32 threshold; // 0 to 256;
      int32 ratio;
      int32 type_thresold;
      minutia_capture::pixel_subscribe<fill_pixe>::ptr_t diag;

      paramet() 
        : threshold(80) // 0.65 * 128 //115 //100 // 86
        , ratio(102)     // 0.40 * 256
        , type_thresold(105) // 140 //121 // 105
      {}
    };

  //private:
  #define BifSize17
  #ifndef BifSize17  /// Separable biffilter taps 13
    const static size_t n = 13; // size of filter

    typedef conv13<64, 59, 46, 31, 17, 8, 3>  cv1; // Gxr
    typedef conv13<0, 59, 92, 92, 69, 42, 20> cv2; // Gyr

    typedef conv13<0, 62, -48, -33, 22, -1, -2,  symmetric> ch15;
    typedef conv13<0, 56, -1, -72, -1, 19, -1,   symmetric> ch16;
    typedef conv13<0, 48, 33, -63, -48, 14, 16,  symmetric> ch17;
    typedef conv13<0, 43, 54, -31, -70, -18, 22, symmetric> ch18;
    typedef conv13<0, 36, 60, 1, -57, -41, 1,    symmetric> ch19;
    typedef conv13<64, -17, -27, 12, 2, -1, 0>  ch25;
    typedef conv13<64, 0, -41, 0, 11, 0, -1>    ch26;
    typedef conv13<64, 13, -42, -19, 11, 7, -1> ch27;
    typedef conv13<64, 23, -35, -34, 0, 12, 5>  ch28;
    typedef conv13<64, 30, -25, -38, -13, 7, 8> ch29;

    // normalize biffilt responce b1 + b2 into [-255 255] range (align with taps)
    static int32 norm(int32 val) {
      static const uint8 bits = 12;
      return (val + (1 << (bits - 1))) >> bits;
    }
  #else  /// Separable biffilter taps 17
    const static size_t n = 17; // size of filter

    typedef conv17<50, 48, 41, 32, 23, 14, 8, 4, 2>      cv1; // Gxr
    typedef conv17<0, 48, 82, 96, 90, 72, 50, 30, 16>    cv2; // Gyr

    typedef conv17<0, 55, -53, -61, 74, 3, -28, 7, 3,     symmetric> ch15;
    typedef conv17<0, 42, 0, -83, 3, 63, -1, -25, 1,      symmetric> ch16;
    typedef conv17<0, 29, 25, -50, -58, 22, 51, 5, -24,   symmetric> ch17;
    typedef conv17<-2, 16, 28, -14, -60, -41, 18, 41, 13, symmetric> ch18;
    typedef conv17<0, 13, 28, 13, -27, -50, -29, 14, 38,  symmetric> ch19;
    typedef conv17<52, -14, -32, 22, 6, -9, 1, 1, 0     > ch25;
    typedef conv17<44, 0, -36, 1, 19, 0, -7, 0, 2       > ch26;
    typedef conv17<37, 9, -28, -18, 12, 14, -1, -6, -1  > ch27;
    typedef conv17<31, 13, -17, -24, -5, 13, 11, 0, -5  > ch28;
    typedef conv17<26, 15, -7, -21, -16, 0, 11, 10, 3   > ch29;

    // normalize biffilt responce b1 + b2 into [-255 255] range (align with taps)
    static int32 norm(int32 val) { 
      // biffilt is 2^18 (corresponds to 1), signal is 2^8 (corresponds to pi), in range 0 to 256 (corresponds to 0 to 2)
      static const uint8 bits = 17;
      return minutia_extractor::reduce(val, bits);
    }
  #endif

    const static size_t m = 3;  // size of responce
    const static size_t scales = 5;  // number of scales
    static const size_t resp2offs = m*m*scales;

    const static size_t patch_width = n + m - 1;
    const static size_t patch_size = patch_width * patch_width;
    const static size_t n2 = n/2;
    const static size_t m2 = m/2;
    
    const Templat img;
    const paramet & param;
    uint8 patch[patch_size];
    int32 resp[m*m*scales*2];
    int32 xoffs, yoffs, period, confidence;
    bool type, rotate180, confirmed;

    void rotate(size_t x, size_t y, int8 c, int8 s) {
      static const uint8 bits = Templat::bits;
      x <<= bits;
      y <<= bits;
      x -= patch_width/2 * (c - s);
      y -= patch_width/2 * (s + c);

      uint8 * end = patch + patch_size;
      for (uint8 * p = patch; p < end; p += patch_width, x -= s, y += c) {
        size_t x0 = x, y0 = y;
        uint8 * endline = p + patch_width;
        for (uint8 * p0 = p; p0 < endline; ++p0, x0 +=c, y0 += s) {
          *p0 = img(x0, y0);
        }
      }
    }

    template <class cv, class ch5, class ch6, class ch7, class ch8, class ch9>
    void shape_change(int32 * out) {
      ch5 b5;
      ch6 b6;
      ch7 b7;
      ch8 b8;
      ch9 b9;

      const uint8 * p = patch + patch_width * n2;
      const uint8 * end = p + patch_width * m;
      for (; p < end; p += patch_width) {
        const uint8 * pi = p;
        const uint8 * endline = p + n - 1;
        for (; pi < endline; pi++) {
          int32 bv = cv::vert(patch_width, pi);
          b5.add(bv);
          b6.add(bv);
          b7.add(bv);
          b8.add(bv);
          b9.add(bv);
        }
        endline += m;
        for (; pi < endline; pi++) {
          int32 bv = cv::vert(patch_width, pi);
          *out++ = b5(bv);
          *out++ = b6(bv);
          *out++ = b7(bv);
          *out++ = b8(bv);
          *out++ = b9(bv);
        }
      }
    }


    bool imageproc_algo(int8 c, int8 s) {
      int32 * r1 = resp;
      int32 * r2 = resp + resp2offs;
      int32 * end = r2;
      int32 conf = 0;
      int32 mirr = 0;
      int32 * pmx = r1;
      int8 count = 0;
      //const static uint8 confweight[45] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4};
      const static uint8 confweight[45] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13, 13, 13, 13, 12, 12, 12, 12, 12, 12, 12, 12, 12};
      //const static uint8 confweight[45] = {32, 32, 32, 32, 32, 32, 32, 32, 32, 31, 31, 31, 31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 30, 30, 30, 30, 29, 29, 29, 29, 29, 29, 29, 29, 29, 28, 28, 28, 28, 28, 28, 28, 28, 28};
      
      for (; r1 < end; r1++, r2++) {
        int32 sum  = *r1 + *r2;
        int32 diff = *r1 - *r2;
        *r1 = sum;
        *r2 = diff;
       
        int32 asum = reduce(abs(sum) * confweight[count], 4);
        int32 adif = reduce(abs(diff)* confweight[count], 4);
       
        if (asum > conf) {
          conf = asum; 
          mirr = adif; 
          pmx = r1;
        }
        if (adif > conf) {
          conf = adif;
          mirr = asum;
          pmx = r2;
        }
        count++;
      }
      confidence = conf = norm(conf);
      mirr = norm(mirr);
      if (conf < param.threshold) {
        return false;
      }
      if (conf * param.ratio < 256 * mirr) {
        return false;
      }
      rotate180 = (pmx >= r1);
      uint32 i = (pmx - resp) % resp2offs;
      int32 dy = int32(i / (m*scales)) - m2;
      i %= m*scales;
      int32 dx = int32(i / scales) - m2;
      i %= scales;
      period = i + 5;
      type = *pmx > 0;
      xoffs = dx * c - dy * s;
      yoffs = dx * s + dy * c;
      return true;
    }


  public:
    fill_pixe(size_t width, size_t size, const uint8 * img, const paramet & param_) 
      : img(width, size, img)
      , param(param_)
      , xoffs(0)
      , yoffs(0)
      , rotate180(false)
    {}
    // returns true if minutia, false if not
    // if minutia, use the following fields: xoffs, yoffs, period, type, rotate, confidence
    bool operator () (size_t x, size_t y, int8 c, int8 s) {
      confidence = 0;
      type = 0;
      xoffs = yoffs = 0;
      period = 0;
      rotate(x, y, c, s);
      shape_change<cv1, ch15, ch16, ch17, ch18, ch19>(resp);
      shape_change<cv2, ch25, ch26, ch27, ch28, ch29>(resp + resp2offs);
      confirmed = imageproc_algo(c, s);
      FRFXLL_DIAG(param.diag, SetPixel, x, y, *this);
      return confirmed;
    }
  };

} // namespace
}
}
}

#endif // __FILLPIXE_H

