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

#ifndef __img_arr_h
#define __img_arr_h

#include <bitset>


namespace minutia_extractor {

namespace minutia_datarec {
namespace merged {

namespace minutia_fetch {
  namespace complex_form {

    struct Template {
      uint8 * start;
      int32 width;
      int32 size;
      uint8 dummy;
      static const uint8 filler = 0;

      uint8 & operator ()(int32 x, int32 yw) {
        return (0 <= x && x < width && 0 <= yw && yw < size) ? start[x + yw] : (dummy=0, dummy); // SmallerTypeCheck : dummy can overflow 
      }
      uint8 operator ()(int32 x, int32 yw) const {
        return (0 <= x && x < width && 0 <= yw && yw < size) ? ~start[x + yw] : filler;
      }
      Template(uint8 * start_, int32 width_, int32 size_) 
        : start(start_), width(width_), size(size_) {}
    };

    inline void init(Template & img, int32 x0, int32 w, int32 yw0, int32 hw) {
      for (int32 yw = yw0; yw < yw0 + hw; yw += img.width) {
        for (int32 x = x0; x < x0 + w; ++x) {
          img(x, yw) = 0;
        }
      }
    }

    template <int32 side>
    inline void copy(const Template & img, int32 x0, int32 yw0, int32 * block) {
      int yw1 = yw0 + side * img.width;
      for (int32 yw = yw0; yw < yw1; yw += img.width) {
        for (int32 x = x0; x < x0 + side; ++x) {
          *block++ = img(x, yw);
        }
      }
    }

    template <int32 side>
    inline void add(Template & img, int32 x0, int32 yw0, int32 * block) {
      for (int32 yw = yw0; yw < yw0 + side * img.width; yw += img.width) {
        for (int32 x = x0; x < x0 + side; ++x) {
          img(x, yw) += (uint8)(*block++);
        }
      }
    }

    inline void inverse(Template & img) {
      for (uint8 * p = img.start; p < img.start + img.size; ++p) {
        *p = ~*p;
      }
    }


  static const uint8 sin_bits = 5;
  inline int16 sin(int32 a) {
    const static int16 sinTable[] = {0, 799, 1567, 2276, 2896, 3406, 3784, 4017, 4096, 4017, 3784, 3406, 2896, 2276, 1567, 799};
    STATIC_ASSERT(countof(sinTable) == (1 << (sin_bits - 1)));
    return ((a & countof(sinTable)) ? -1 : 1) * sinTable[a & (countof(sinTable) - 1)];
  }

  inline int16 cos(int32 a) {
    return sin(a + 8);
  }

  using namespace minutia_extractor;
  using namespace minutia_extractor::minutia_datarec::merged;

  template <uint8 bits, uint8 shift> struct bit_reversal {
    static int32 reverse(int32 x) { 
      std::bitset<bits> s(x >> shift);
      for (uint8 i = 0; i < bits/2; i++) {
        bool t = s[i];
        s[i] = s[bits - i - 1];
        s[bits - i - 1] = t;
      }
      return int32(s.to_ulong() << shift);
    }
  };
  
  template <int32 a, uint8 s>
  inline int32 swap2bitsets(int32 x) {
    return ((x & a) << s) | ((x & (a << s)) >> s) | (x & ~((a << s) | a));
  }
  // bit reverse any 4 bits (4, 5, 9 are the examples)
  template <int32 a1, uint8 s1, int32 a2, uint8 s2>
  inline int32 bitreverse4(int32 x) {
    x = swap2bitsets<a1, s1>(x);
    x = swap2bitsets<a2, s2>(x);
    return x;
  }

  // reverse bits 0..8
  template <uint8 shift> struct bit_reversal<9, shift> {
    static int32 reverse(int32 x) {
      return bitreverse4<0x49 << shift, 2, 0x7 << shift, 6>(x);
    }
  };
  // reverse bits 0..4
  template <uint8 shift> struct bit_reversal<5, shift> {
    static int32 reverse(int32 x) {
      return bitreverse4<0x9 << shift, 1, 0x3 << shift, 3>(x);
    }
  };
  // reverse bits 0..3
  template <uint8 shift> struct bit_reversal<4, shift> {
    static int32 reverse(int32 x) {
      return bitreverse4<0x5 << shift, 1, 0x3 << shift, 2>(x);
    }
  };

  template <uint8 bits, uint8 shift> static int32 bitreverse(int32 x) {
    return bit_reversal<bits, shift>::reverse(x);
  }

  template <uint8 size_bits, uint8 stride_bits>
  inline void shuffle(int32 * data) {
    static const uint32 size   = 1 << size_bits;
    static const uint32 stride = 1 << stride_bits;
    for (int i = stride; i < (int)(size - stride); i += stride) {
      const int32 j = bit_reversal<size_bits - stride_bits, stride_bits>::reverse(i);
      if (i > j) {
        std::swap(data[i], data[j]);
        std::swap(data[i+1], data[j+1]);
      }
    }
  }

  typedef complex<int32> ci32;

  template <bool inverse, uint8 size_bits, uint8 stride_bits>
  inline void f_trans(int32 * data) {
    static const int32 n = 1 << size_bits;
    static const uint32 stride = 1 << stride_bits;
    shuffle<size_bits, stride_bits>(data);
    int32 dt = (inverse ? 1 : -1) * (1 << sin_bits);
    for (uint8 ll = stride_bits; ll < size_bits; ll++) {
      const int32 mmax = 1 << ll;
      const int32 istep = mmax << 1;
      dt >>= 1;
      for (int32 m = 0, t = 0; m < mmax; m += stride, t += dt) {
        int32 wr = cos(t);
        int32 wi = sin(t);
        for (uint32 i = m; i < n; i += istep) {
          int32 * pi = data + i;
          int32 * pj = pi + mmax;
          int32 tr = reduce(wr * pj[0] - wi * pj[1], 12);
          int32 ti = reduce(wr * pj[1] + wi * pj[0], 12);
          pj[0] = pi[0] - tr;
          pj[1] = pi[1] - ti;
          pi[0] += tr;
          pi[1] += ti;
        }
      }
    }
  }

  template <bool real, bool inverse, uint8 size_bits, uint8 stride_bits>
  inline void ff_trans(int32 * data) {
    if (!inverse) {
      f_trans<inverse, size_bits, stride_bits>(data);
    }
    if (real) {
      static const int32 stride = 1 << stride_bits;
      static const int32 size = 1 << size_bits;
      int32 dt = (inverse ? 1: -1) * (1 << (sin_bits - (size_bits - stride_bits + 1)));
      int32 t = dt + (inverse ? (1 << (sin_bits - 1)) : 0);
      for (int32 i = stride; i <= size/2; i += stride, t += dt) {
        ci32 w(cos(t), sin(t));
        int32 * p1 = data + i;
        int32 * p2 = data + size - i;
        ci32 h1 = ci32(p2[0] + p1[0], p1[1] - p2[1]) << 12;
        ci32 h2 = w * ci32(p1[1] + p2[1], p2[0] - p1[0]);
        ci32 f1 = reduce(h1 + h2, 12 + (inverse ? 0 : 1));
        ci32 f2 = reduce(conj(h1 - h2), 12 + (inverse ? 0 : 1));
        p1[0] = f1.real(); p1[1] = f1.imag(); 
        p2[0] = f2.real(); p2[1] = f2.imag();
      }
      int32 tr = data[0], ti = data[1];
      data[0] = tr + ti; data[1] =  inverse ? (tr - ti) : 0; //0; // suppress highest freaquency
    }
    if (inverse) {
      f_trans<inverse, size_bits, stride_bits>(data);
    }
  }

  template <bool real, bool inverse, uint8 dim_bits>
  inline void four_trans(int32 * data) {
    const int32 size1 = 1 << dim_bits;
    const int32 size2 = size1 << dim_bits;
    if (!inverse) {
      for (int32 y = 0; y < size2; y += size1) {
        ff_trans<real, inverse, dim_bits, 1>(data + y);
      }
    }
    for (int32 x = 0; x < size1; x += 2) {
      f_trans<inverse, dim_bits*2, dim_bits>(data + x);
    }
    if (inverse) {
      for (int32 y = 0; y < size2; y += size1) {
        ff_trans<real, inverse, dim_bits, 1>(data + y);
      }
    }
  }

  template <uint8 size_bits, uint8 shift>
  inline void reduce_array(int32 * data) {
    for (int32 * end = data + (1 << size_bits); data < end; data++) {
      *data = reduce(*data, shift);
    }
  }

  template <uint8 dim_bits, class F>
  inline void enhance_array(int32 * data, F f) {
    int32 size = 1 << dim_bits;
    int32 halfsize = 1 << (dim_bits - 1);
    for (int32 y = 0; y < size; y++) {
      for (int32 x = 0; x < halfsize; x++, data += 2) {
        ci32 v(data[0], data[1]);
        v = f(v, x, y - ((y < halfsize) ? 0 : size));
        data[0] = v.real(); data[1] = v.imag();
      }      
    }
  }

  inline ci32 enhance(ci32 v, int32 x, int32 y) {
 

 

    int32 r2 = sqr(x) + sqr(y);
    if (r2 <= 6 || r2 >= 169) {
      return 0;
    }

    int32 a = reduce(oct_abs(v), 5);
    ci32 v1 = reduce(v * a, 7); //7
    v = reduce(v + v1, 3);
    return v;
  }

  template <size_t size, size_t spacing>
  class outer_layer {
    const static size_t _mirr = size + 1 - spacing;
    bool left, right;
  public:
    outer_layer(bool left_, bool right_) : left(left_), right(right_) {
      STATIC_ASSERT(spacing <= size);
    }
    int32 operator [](int32 i) const { 
      const static size_t _floor = _mirr < spacing ? _mirr : spacing;
      int32 t = (int32) std::min(std::min(left ? _floor : i + 1, right ? _floor : size - i), _floor); 
      return t;
    }
    int32 norm() const { return _mirr; }
  };

  template <uint8 dim_bits, int32 spacing>
  inline void normalize(int32 * data, const outer_layer<1 << dim_bits, spacing> & xenv, const outer_layer<1 << dim_bits, spacing> & yenv) {
    int32 * end = data + (1 << (dim_bits * 2));
    int32 mn = *std::min_element(data, end);
    int32 mx = *std::max_element(data, end);
    int32 range = mx - mn;
    int32 div = range;
    int32 trsh = 16;
    if (range < trsh) {
      div = trsh;
      mn -= (trsh - range) / 2;
    }
    div *= xenv.norm() * yenv.norm();
    const static int a = 1 << dim_bits;
    for (int y = 0; y < a; ++y) {
      int32 ye = yenv[y];
      for (int x = 0; x < a; ++x, ++data) {
        *data = divide((*data - mn) * 251 * xenv[x] * ye, div);
      }
    }
  }

  template <uint8 dim_bits, int32 spacing>
  inline void enhance_block(int32 * data, const outer_layer<1 << dim_bits, spacing> & xenv, const outer_layer<1 << dim_bits, spacing> & yenv) {
    four_trans<true, false, dim_bits>(data);
    enhance_array<dim_bits>(data, enhance);
    four_trans<true, true, dim_bits>(data);
    reduce_array<dim_bits*2, dim_bits*2>(data);
    normalize<dim_bits, spacing>(data, xenv, yenv);
  }

  }
 
  template <uint8 block_bits, int32 spacing>
  inline bool four_enh(uint8 * img, size_t width_, size_t size_, size_t buffer_size) {
    int32 width = (int32) width_;
    int32 size  = (int32) size_;

    using namespace complex_form;
    using complex_form::Template;
    const static int32 block_dim = 1 << block_bits;
    STATIC_ASSERT(0 < spacing && spacing <= block_dim);

    const int32 bw = block_dim * width;
    if (size_t(size + bw) > buffer_size) return false;
    memmove(img + bw, img, size);
    Template in_img(img + bw, width, size);
    Template out_img(img, width, size);

    const static size_t block_size = 1 << block_bits * 2;
    int32 block[block_size];
    const int32 ywmax = size;
    const int32 xmax = width;
    const int32 yspacing = width * spacing;
    init(out_img, 0, width, 0, bw);
    for (int32 yw = yspacing - bw; yw < ywmax; yw += yspacing) {
      
      complex_form::outer_layer<block_dim, spacing> yenv(false, false);
      for (int32 x = spacing - block_dim; x < xmax; x += spacing) {
        copy<block_dim>(in_img, x, yw, block);
       
        complex_form::outer_layer<block_dim, spacing> xenv(false, false);
        complex_form::enhance_block<block_bits, spacing>(block, xenv, yenv);
        add<block_dim>(out_img, x, yw, block);
      }
      init(out_img, 0, width, yw + bw, yspacing);
    }
    inverse(out_img);
    return true;
  }
}
}
}
}

#endif
