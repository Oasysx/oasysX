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
#ifndef __COMPARE_MINUTIA_H
#define __COMPARE_MINUTIA_H

#include <algorithm>
#include <stdlib.h>
#include <memory>
#include <vector>


#ifndef test_output
#define test_output(rc) gtest_output(rc, __LINE__, __FILE__)
#endif

#if 0
#include <stdio.h>
namespace minutia_extractor {
  namespace minutia_datarec {
    inline int gtest_output(int rc, int line, const char * file) {
      if (rc < MINEX_RET_SUCCESS) {
        printf("%s(%d) : warning %08x : gtest_output rc < MINEX_RET_SUCCESS", file, line, rc);
        return rc; 
      }
      return rc; 
    }
  }
}
#else
namespace minutia_extractor {
  namespace minutia_datarec {
    inline int gtest_output(int rc, int, const char *) {
      if (rc < MINEX_RET_SUCCESS) {
        return rc; 
      }
      return rc; 
    }
  }
}
#endif




#if !(defined(FRFXLL_MAJOR) && defined(FRFXLL_MINOR) && defined(FRFXLL_REVISION))

#define FRFXLL_MAJOR 5
#define FRFXLL_MINOR 2
#define FRFXLL_REVISION 0
#if !(defined(FRFXLL_BUILD))

#define FRFXLL_BUILD 0
#endif
#endif
namespace minutia_extractor {

  template <class T, size_t size>
  class memory_mgmt : public std::allocator<T> {
  public:
    typedef T * pointer;
    typedef size_t size_type;
    template<class _Other>
    struct assemble {
      typedef memory_mgmt<_Other, size> other;
    };
    unsigned char buffer[size*sizeof(T)];
    memory_mgmt() {}
    pointer mmem_org(size_type, const void* = NULL) {
      return reinterpret_cast<pointer>(buffer);
    }
    void mmem_deorg(pointer, size_type) {
    }
    size_type max_size( ) const {
      return size;
    }
    // for compatibility with msvc
    template<class _Other, size_type _Size>
    memory_mgmt(const memory_mgmt<_Other, _Size> & _Right) {}
  };
  
  template <class T, size_t _max_capacity>
  class vector : public std::vector<T, memory_mgmt<T, _max_capacity> > {
    typedef std::vector<T, memory_mgmt<T, _max_capacity> > base_t;
  public:
    static const size_t Capacity = _max_capacity;

    vector() { base_t::reserve(_max_capacity); }
    vector(size_t count_, const T & val_) : base_t(count_, val_) { base_t::reserve(_max_capacity); }
    vector(const vector & v) : base_t(v) {}
    vector(const base_t & v) : base_t(v) {}
    bool full() const { return base_t::size() == base_t::capacity(); }
  };

  template <class T>
  class mem_array : public std::allocator<T> {
  public:
    typedef T * pointer;
    typedef size_t size_type;
    template<class _Other>
    struct assemble {
      typedef mem_array<_Other> other;
    };
    T * buffer;
    size_t size;
    mem_array(T * buffer_, size_t size_) 
      : buffer(buffer_) 
      , size(size_)
    {}
    pointer mmem_org(size_type size, const void* = NULL) {
      return buffer;
    }
    void mmem_deorg(pointer buffer, size_type) {
    }
    size_type max_size( ) const {
      return size;
    }
    template<class _Other>
    mem_array(const mem_array<_Other> & _Right) 
      : buffer(_Right.buffer)
      , size(_Right.size * sizeof(T) / sizeof(_Other))
    {}
  };

  template <class T>
  class arr_dir : public std::vector<T, mem_array<T> > {
    typedef std::vector<T, mem_array<T> > base_t;
  public:
    arr_dir(T * buffer_, size_t size_) 
      : base_t(typename base_t::membuf_type(buffer_, size_)) 
    {
      base_t::reserve(size_); 
    }
    arr_dir(T * buffer_, size_t size_, size_t count_, const T & val_) 
      : base_t(count_, val_, typename base_t::membuf_type(buffer_, size_)) 
    { 
        base_t::reserve(size_); 
    }
    arr_dir(const arr_dir & v) : base_t(v) {}
    arr_dir(const base_t & v) : base_t(v) {}
    bool full() const { return base_t::size() == base_t::capacity(); }
  };


namespace minutia_datarec {
    template <class T>
    struct ans_t {
      T rc;
      ans_t() : rc(MINEX_RET_SUCCESS) {}
      ans_t(T rc_) : rc(rc_) {}
      template <class Other>
      ans_t(ans_t<Other> & hr) : rc(hr.rc) {}
      int GetResult() const { return rc; }
      bool IsBad() const { return rc < MINEX_RET_SUCCESS; }
      bool IsBad(int rc_) { rc = rc_; return IsBad(); }
      operator int() const { return rc; }
    };

    typedef ans_t<int> HResult;
    typedef ans_t<int &> HResultRef;



namespace merged {

      const int32 logMultiplier = 256; // changing this requires new log table
      const uint32 bitsScaleFactor = 12;

      const int32 ScaleFactor = 1 << bitsScaleFactor;
      const int32 ScaleFactor2 = ScaleFactor * ScaleFactor;
      const int32  lgScaleFactor = logMultiplier * bitsScaleFactor;

      const uint8 bitsAngleScale = 8; // changing this requires new sin/cos tables
      const int32 angleScale = 1 << bitsAngleScale; 
      const int32 angleScale2 = angleScale * angleScale;

      const uint32 bitsPmpScale = 8;
      const int32 pmpScale = 1 << bitsPmpScale;
      const int32  lgPmpScale = logMultiplier * bitsPmpScale;

      const uint8 minutiaWeightScale = 64;


  template <class T> inline T ScaleDown(T x, uint8 bits) {
    return (x + (1 << (bits-1))) >> bits;
  }
  inline int16 convert_sh(int32 x) {
    return int16(ScaleDown(x, bitsScaleFactor));
  };

  using std::min;

  struct locate {
    int16 x;
    int16 y;
    locate() : x(0), y(0) {}
    locate(int16 x_, int16 y_) : x(x_), y(y_) {}
    locate operator += (locate p) {
      x += p.x;
      y += p.y;
      return *this;
    }
    locate operator -= (locate p) {
      x -= p.x;
      y -= p.y;
      return *this;
    }
  };
  inline locate operator + (locate p1, locate p2) {
    return locate(p1.x + p2.x, p1.y + p2.y);
  }
  inline locate operator - (locate p1, locate p2) {
    return locate(p1.x - p2.x, p1.y - p2.y);
  }

  inline uint32 space_bet(locate p) {
    return int32(p.x) * p.x + int32(p.y) * p.y;
  }
  struct gradient {
    int16 c; // cosine * angleScale
    int16 s; // sine * angleScale
    gradient() : c(angleScale), s(0) {}
    gradient(int16 c_, int16 s_) : c(c_), s(s_) {}
    gradient(uint8 a) : c(cos(a)), s(sin(a)) {}
    static int16 sin(uint8 a) {
      const static int16 sinTable[128] = {
        0,   6,   13,  19,   25,  31,  38,  44,   50,  56,  62,  68,   74,  80,  86,  92, 
        98,  104, 109, 115,  121, 126, 132, 137,  142, 147, 152, 157,  162, 167, 172, 177,
        181, 185, 190, 194,  198, 202, 206, 209,  213, 216, 220, 223,  226, 229, 231, 234, 
        237, 239, 241, 243,  245, 247, 248, 250,  251, 252, 253, 254,  255, 255, 256, 256, 

        256, 256, 256, 255,  255, 254, 253, 252,  251, 250, 248, 247,  245, 243, 241, 239, 
        237, 234, 231, 229,  226, 223, 220, 216,  213, 209, 206, 202,  198, 194, 190, 185, 
        181, 177, 172, 167,  162, 157, 152, 147,  142, 137, 132, 126,  121, 115, 109, 104, 
        98,  92,  86,  80,   74,  68,  62,  56,   50,  44,  38,  31,   25,  19,  13,  6, 
      };           
      return ((a & 0x80) ? -1 : 1) * sinTable[a & 0x7f];
    }
    static int16 cos(uint8 a) {
      return sin(((a + 64) & 0xff)); 
    }
    void Assign(uint8 a) {
      *this = gradient(a);
    }
    static void swap(int16 &c, int16 &s) {
      int16 t = c;
      c = s;
      s = t;
    }
    static uint8 atan2(int16 c, int16 s) {
      const uint8 NumAtanEntries = 96;  // To reduce rounding errors: dAtan(x)/dx /. x->0 == 3 Pi / 4 that corresponds to 96
      const static uint8 atan[NumAtanEntries + 1] = { // + 1 Pi/4 point : c == s;
        0, 0, 1, 1, 2, 2, 3, 3, 3, 4, 4, 5, 5, 5, 6, 6, 7, 7, 8, 8, 8, 9, 9, 10, 10, \
        10, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, \
        18, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, \
        24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 26, 27, 27, 27, 28, 28, 28, 28, 29, \
        29, 29, 29, 30, 30, 30, 30, 30, 31, 31, 31, 31, 32, 32, 32
      };
      bool sn = s < 0;
      if (sn) s = -s;
      bool cn = c < 0;
      if (cn) c = -c;
      bool cls = c < s;
      if (cls) swap(c, s);
      uint8 out = atan[s * NumAtanEntries / c];
      if (cls) out = 0x40 - out;
      if (cn) out = 0x80 - out;
      if (sn) out = - out;
      return out;
    }
    operator uint8 () const {
      return atan2(c, s);
    }
    gradient operator - () const {
      return gradient(c, -s);
    }
  };
  inline locate Drifft(locate p, gradient a) {
    locate r;
    r.x = int16(ScaleDown(int32(p.x) * a.c - int32(p.y) * a.s, bitsAngleScale));
    r.y = int16(ScaleDown(int32(p.x) * a.s + int32(p.y) * a.c, bitsAngleScale));
    return r;
  }
  // scale factor is equal to scale / (2 ^ bitsShift)
  // i.e. scale = 3, bitsShift = 2 : 3.0 / (2.0 ^ 2)  = 0.75
  inline locate drifft_measure(locate p, gradient a, int16 scale, uint8 bitsShift) {
    uint8 shift = bitsAngleScale + bitsShift;
    locate r;
    r.x = int16(ScaleDown(scale * (int32(p.x) * a.c - int32(p.y) * a.s), shift));
    r.y = int16(ScaleDown(scale * (int32(p.x) * a.s + int32(p.y) * a.c), shift));
    return r;
  }

  struct minutia_param {
    locate position;
    uint8 pmp, conf;
    uint8 theta; 
    uint8 type;
    enum {
      type_other = 0,
      type_ridge_ending = 1,
      type_bifurcation = 2,
    };

    minutia_param() : pmp(0), conf(0), theta(0), type(type_other) {} // minimum acceptable
  };

  struct datapoint {
    static const int height = 50;
    static const int width = 32;
    uint32 data[height];
    uint32 area;
    datapoint() {
      Init();
    }
    void Init() {
      area = 0;
      for (int i = height; i; ) data[--i] = 0;
    }
    // in pixels
    bool InBounds(int32 x, int32 y) const {
      return (uint32(x) < uint32(width)) && (uint32(y) < uint32(height));
    }
    // get
    bool GetPixel(int32 x, int32 y) const {
      return InBounds(x,y) && (data[y] & (1 << x));
    }
    // set, ignore out of bounds
    struct bound_ignore {
      uint32 & row;
      uint32 mask;
      bound_ignore(uint32 & row_, uint32 mask_) 
        : row(row_), mask(mask_) {}
      bool operator = (bool val) {
        if (val) row |= mask; else row &= ~mask;
        return val;
      }
    };
    bound_ignore Pixel(int32 x, int32 y) {
      return InBounds(x,y) ? bound_ignore(data[y], 1 << x) : bound_ignore(data[0], 0);
    }
    // in world co-ord, read only
    bool operator() (int32 x, int32 y) const {
      return GetPixel(x / 8, y / 8);
    }
    bool operator() (locate p) const {
      return GetPixel(p.x / 8, p.y / 8);
    }
    // Compute footprint area: this is not the same as field area, due to losses in compression/representation
    uint32 ComputeArea() const {
      uint32 area = 0;
      for (int y = 0; y < height; y++) {  // my co-ord
        for (int x = 0; x < width; x++) { // my co-ord
          if (GetPixel(x, y)) {
            area++;
          }
        }
      }
      return area * 8 * 8;
    }
  };

  template <size_t maxNumMin>
  struct parameters {
    static const size_t Capacity = maxNumMin;
    minutia_param minutia[Capacity];
    size_t numMinutia;
    locate  center;       // precomputed

    parameters() : numMinutia(0) {}
    static size_t capacity() {
      return Capacity;
    }
    size_t size() const {
      return numMinutia;
    }
    // rotate and shift so that all points' coordinates are positive
    void drift_n_swap(uint8 rotation) {
      size_t i;
      for (i = 0; i < size(); i++) {
        minutia[i].position = Drifft(minutia[i].position, rotation);
        minutia[i].theta += rotation;
      }
      locate pmin(integer_limits<int16>::max, integer_limits<int16>::max);
      for (i = 0; i < size(); i++) {
        pmin.x = min(pmin.x, minutia[i].position.x);
        pmin.y = min(pmin.y, minutia[i].position.y);
      }
      for (i = 0; i < size(); i++) {
        minutia[i].position -= pmin;
      }
    }
  };

  inline bool minutia_match_conf(const minutia_param&  mp1, const minutia_param& mp2)
  {
    if (mp1.conf > mp2.conf ) return true;
    if (mp1.conf < mp2.conf ) return false;
    if (mp1.position.y < mp2.position.y) return true;
    if (mp1.position.y > mp2.position.y) return false;
    if (mp1.position.x < mp2.position.x) return true;
    if (mp1.position.x > mp2.position.x) return false;
    if (mp1.theta < mp2.theta) return true;
    if (mp1.theta > mp2.theta) return false;
    return true;
  }
  inline bool operator > (const minutia_param & mp1, const minutia_param & mp2) {
    return minutia_match_conf(mp1, mp2);
  }

  struct compare_minutia : public parameters<76> {
    datapoint footprint;
    locate offset;

    void Init() {
      footprint.Init();
    }
    void calculate() {
    }
    template <size_t n1> compare_minutia & operator = (const parameters<n1> & mp) {
      static_cast<parameters<Capacity> &>(*this) = mp;
      return *this;
    }
  };

} // namespace
}
}
# define CheckR(rc_) { int __local_rc = test_output(rc_); if (__local_rc < MINEX_RET_SUCCESS) return __local_rc;  }
# define CheckV(rc_) { int __local_rc = test_output(rc_); if (__local_rc < MINEX_RET_SUCCESS) { rc = __local_rc; return; } }
# define AssertR(expr, err) CheckR( (expr) ? MINEX_RET_SUCCESS : (err) )
# define AssertV(expr, err) CheckV( (expr) ? MINEX_RET_SUCCESS : (err) )
# define test_formate_flag(flags, value) AssertR(((flags) & (value)) != (value), MINEX_RET_FAILURE_NULL_TEMPLATE)





#endif // __COMPARE_MINUTIA_H
