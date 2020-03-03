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
#ifndef __SCALE_LIMITS_h
#define __SCALE_LIMITS_h

#include <algorithm> // swap
#include <limits>
#include <stdlib.h>
#include <limits>

#include <string.h>


#ifndef NULL
#define NULL (0)
#endif

#include <stddef.h>

#define JOIN1(x, y) x##y
#define JOIN(x, y) JOIN1(x, y)

#define STATIC_ASSERT(expr) \
  enum { JOIN(__static_assert, __LINE__) = sizeof(::minutia_extractor::fixed_parameter::Asserter<(bool)(expr)>) }



namespace minutia_extractor {

  namespace fixed_parameter {

    template <bool>  class Asserter;
    template <>  class Asserter<true> { enum { x = 1 }; };

  };


  template <class T, bool singed_>
  class integer_limits_base : public std::numeric_limits<T> {
  protected:
    static const T _min = 0;
  };
  template <class T>
  class integer_limits_base<T, true> : public std::numeric_limits<T> {
  protected:
    static const T _min = T(typename integer<sizeof(T), false>::type(1) << std::numeric_limits<T>::digits);
  };
  template <class T>
  class integer_limits : public integer_limits_base<T, std::numeric_limits<T>::is_signed> {
    typedef integer_limits_base<T, std::numeric_limits<T>::is_signed> base_t;
    STATIC_ASSERT(std::numeric_limits<T>::is_integer);
  public:
    typedef typename integer<sizeof(T), false>::type unsigned_t;
    typedef typename integer<sizeof(T), true>::type  signed_t;

    static const T min = base_t::_min;
    static const T max = T(unsigned_t(min) - 1);
  };

#ifndef countof
  
  template <typename T, size_t N>
  char ( &_ArraySizeHelper( T (&array)[N] ))[N];
# define countof( array ) (sizeof( minutia_extractor::_ArraySizeHelper( array ) ))
#endif

  inline int32 sincosnorm(int32 x) {
    return (x + 64) >> 7;
  }

  inline int8 sin(uint8 a) {
    const static int8 sinTable[128] = { // 19
        0,   3,   6,   9,   12,  15,  19,  22,   25,  28,  31,  34,   37,  40,  43,  46, 
       49,  51,  54,  57,   60,  63,  65,  68,   71,  73,  76,  78,   81,  83,  85,  88, 
       90,  92,  94,  96,   98, 100, 102, 104,  106, 107, 109, 111,  112, 113, 115, 116, 
      117, 118, 120, 121,  122, 122, 123, 124,  125, 125, 126, 126,  126, 127, 127, 127, 

      127, 127, 127, 127,  126, 126, 126, 125,  125, 124, 123, 122,  122, 121, 120, 118, 
      117, 116, 115, 113,  112, 111, 109, 107,  106, 104, 102, 100,   98,  96,  94,  92, 
       90,  88,  85,  83,   81,  78,  76,  73,   71,  68,  65,  63,   60,  57,  54,  51, 
       49,  46,  43,  40,   37,  34,  31,  28,   25,  22,  19,  15,   12,   9,   6,   3, 
    };           
    return ((a & 0x80) ? -1 : 1) * sinTable[a & 0x7f];
  }

  inline int8 cos(uint8 a) {
    return sin(((a + 64) & 0xff)); 
  }

 
  inline uint8 atan2(int32 c, int32 s) {
    const uint8 NumAtanEntries = 96;  // To reduce rounding errors: dAtan(x)/dx /. x->0 == 3 Pi / 4 that corresponds to 96
    const static uint8 atan[NumAtanEntries + 1] = { // + 1 Pi/4 point : c == s;
       0,   1,   1,   2,    2,   3,   3,   3,    4,   4,   5,   5,    6,   6,   6,   7, 
       7,   8,   8,   8,    9,   9,  10,  10,   10,  11,  11,  12,   12,  12,  13,  13, 
      13,  14,  14,  15,   15,  15,  16,  16,   16,  17,  17,  18,   18,  18,  19,  19, 
      19,  20,  20,  20,   21,  21,  21,  22,   22,  22,  22,  23,   23,  23,  24,  24, 

      24,  25,  25,  25,   25,  26,  26,  26,   27,  27,  27,  27,   28,  28,  28,  28, 
      29,  29,  29,  29,   30,  30,  30,  30,   31,  31,  31,  31,   31,  32,  32,  32, 
      32
    };
    bool sn = s < 0;
    if (sn) s = -s;
    bool cn = c < 0;
    if (cn) c = -c;
    bool cls = c < s;
    if (cls) std::swap(c, s);
    if (c == 0) return 0;
    uint8 out = atan[s * NumAtanEntries / c];
    if (cls) out = 0x40 - out;
    if (cn) out = 0x80 - out;
    if (sn) out = - out;
    return out;
  }

  template <class T> inline T abs(T n) {
    return n >= 0 ? n : -n;
  }

  template <class T, class T2> inline T max(T x, T2 y) {
    return x > y ? x : y;
  }
  template <class T, class T2> inline T min(T x, T2 y) {
    return x < y ? x : y;
  }

  template <class T> inline T median3(T a, T b, T c) {
    if (a < b) {
      if (b < c) {
        return b;
      } else if (a < c) {
        return c;
      } else {
        return a;
      }
    } else if (a < c) {
      return a;
    } else if (b < c) {
      return c;
    } else {
      return b;
    }
  }

  template <class T> inline T sqr(T a){
    return a * a;
  }
  template <class T1, class T> inline T1 sqr(T a){
    return T1(a) * a;
  }

  template <class T> inline T reduce(const T & a, uint8 n){
    return (a + (1 << (n-1))) >> n;
  }
 
  inline unsigned int mcrowne_isqrt (unsigned long val) {
    unsigned int temp, g=0;
  
    if (val >= 0x40000000) {
      g = 0x8000; 
      val -= 0x40000000;
    }
  
  #define INNER_ISQRT(s)                        \
    temp = (g << (s)) + (1 << ((s) * 2 - 2));   \
    if (val >= temp) {                          \
      g += 1 << ((s)-1);                        \
      val -= temp;                              \
    }
  
    INNER_ISQRT (15)
    INNER_ISQRT (14)
    INNER_ISQRT (13)
    INNER_ISQRT (12)
    INNER_ISQRT (11)
    INNER_ISQRT (10)
    INNER_ISQRT ( 9)
    INNER_ISQRT ( 8)
    INNER_ISQRT ( 7)
    INNER_ISQRT ( 6)
    INNER_ISQRT ( 5)
    INNER_ISQRT ( 4)
    INNER_ISQRT ( 3)
    INNER_ISQRT ( 2)
  
  #undef INNER_ISQRT
  
    temp = g+g+1;
    if (val >= temp) g++;
    return g;
  }
 
  inline unsigned fred_sqrt(unsigned long x) {
    static const unsigned char sqq_table[] = {
      0,  16,  22,  27,  32,  35,  39,  42,  45,  48,  50,  53,  55,  57,
      59,  61,  64,  65,  67,  69,  71,  73,  75,  76,  78,  80,  81,  83,
      84,  86,  87,  89,  90,  91,  93,  94,  96,  97,  98,  99, 101, 102,
      103, 104, 106, 107, 108, 109, 110, 112, 113, 114, 115, 116, 117, 118,
      119, 120, 121, 122, 123, 124, 125, 126, 128, 128, 129, 130, 131, 132,
      133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 144, 145,
      146, 147, 148, 149, 150, 150, 151, 152, 153, 154, 155, 155, 156, 157,
      158, 159, 160, 160, 161, 162, 163, 163, 164, 165, 166, 167, 167, 168,
      169, 170, 170, 171, 172, 173, 173, 174, 175, 176, 176, 177, 178, 178,
      179, 180, 181, 181, 182, 183, 183, 184, 185, 185, 186, 187, 187, 188,
      189, 189, 190, 191, 192, 192, 193, 193, 194, 195, 195, 196, 197, 197,
      198, 199, 199, 200, 201, 201, 202, 203, 203, 204, 204, 205, 206, 206,
      207, 208, 208, 209, 209, 210, 211, 211, 212, 212, 213, 214, 214, 215,
      215, 216, 217, 217, 218, 218, 219, 219, 220, 221, 221, 222, 222, 223,
      224, 224, 225, 225, 226, 226, 227, 227, 228, 229, 229, 230, 230, 231,
      231, 232, 232, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 238,
      239, 240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245, 246,
      246, 247, 247, 248, 248, 249, 249, 250, 250, 251, 251, 252, 252, 253,
      253, 254, 254, 255
    };

    unsigned long xn;

    if (x >= 0x10000) {
      if (x >= 0x1000000) {
        if (x >= 0x10000000) {
          if (x >= 0x40000000) {
            if (x >= 65535UL*65535UL) {
              return 65535;
            }
            xn = sqq_table[x>>24] << 8;
          } else {
            xn = sqq_table[x>>22] << 7;
          }
        } else {
          if (x >= 0x4000000) {
            xn = sqq_table[x>>20] << 6;
          } else {
            xn = sqq_table[x>>18] << 5;
          }
        }
      } else {
        if (x >= 0x100000) {
          if (x >= 0x400000) {
            xn = sqq_table[x>>16] << 4;
          } else {
            xn = sqq_table[x>>14] << 3;
          }
        } else {
          if (x >= 0x40000) {
            xn = sqq_table[x>>12] << 2;
          } else {
            xn = sqq_table[x>>10] << 1;
          }
        }
        goto nr1;
      }
    } else {
      if (x >= 0x100) {
        if (x >= 0x1000) {
          if (x >= 0x4000) {
            xn = (sqq_table[x>>8] >> 0) + 1;
          } else {
            xn = (sqq_table[x>>6] >> 1) + 1;
          }
        } else {
          if (x >= 0x400) {
            xn = (sqq_table[x>>4] >> 2) + 1;
          } else {
            xn = (sqq_table[x>>2] >> 3) + 1;
          }
        }
        goto adj;
      } else {
        return sqq_table[x] >> 4;
      }
     
    }
    xn = (xn + 1 + x / xn) / 2;
nr1:
    xn = (xn + 1 + x / xn) / 2;
adj:
    if (xn * xn > x) { /* Correct rounding if necessary */
      xn--;
    }
    return xn;
  }


  // cannot implement divide(T x, T2 y) without using traits
  template <bool _signed, class T>
  struct divider {
    static T divide(T x, T y) {
      T t = ((x >= 0) != (y > 0)) ? -1 : 1;
      x = abs(x); 
      y = abs(y);
      return (x + (y >> 1)) / y * t;
    }
  };

  template <class T> 
  struct divider<false,T> {
    static T divide(T x, T y) {
      return (x + (y >> 1)) / y;
    }
  };

  template <class T> 
  inline T divide(T x, T y) {
    //STATIC_ASSERT(sizeof(T) <= 4);
    return divider<std::numeric_limits<T>::is_signed,T>::divide(x, y);
  }
  template <class X, class Y, class Z> 
  inline X muldiv(X x, Y y, Z z) {
    typedef typename integer<sizeof(X)*2, std::numeric_limits<X>::is_signed>::type T2;
    return X(divide<T2>(T2(x) * y, z));
  }

  // Compute 256 * Log2(x)
  inline int32 log2_256_8(uint32 x) {
    // Note: x = 0 encodes 1/4, according to PMP encoding rules, 256 Log2[1/4] = -512
    const static int16 LogTable[256] = {
      -512, 0, 256, 406, 512, 594, 662, 719, 768, 812, 850, 886, 918, 947, 975, \
      1000, 1024, 1046, 1068, 1087, 1106, 1124, 1142, 1158, 1174, 1189, 1203, 1217, \
      1231, 1244, 1256, 1268, 1280, 1291, 1302, 1313, 1324, 1334, 1343, 1353, 1362, \
      1372, 1380, 1389, 1398, 1406, 1414, 1422, 1430, 1437, 1445, 1452, 1459, 1466, \
      1473, 1480, 1487, 1493, 1500, 1506, 1512, 1518, 1524, 1530, 1536, 1542, 1547, \
      1553, 1558, 1564, 1569, 1574, 1580, 1585, 1590, 1595, 1599, 1604, 1609, 1614, \
      1618, 1623, 1628, 1632, 1636, 1641, 1645, 1649, 1654, 1658, 1662, 1666, 1670, \
      1674, 1678, 1682, 1686, 1690, 1693, 1697, 1701, 1705, 1708, 1712, 1715, 1719, \
      1722, 1726, 1729, 1733, 1736, 1739, 1743, 1746, 1749, 1752, 1756, 1759, 1762, \
      1765, 1768, 1771, 1774, 1777, 1780, 1783, 1786, 1789, 1792, 1795, 1798, 1801, \
      1803, 1806, 1809, 1812, 1814, 1817, 1820, 1822, 1825, 1828, 1830, 1833, 1836, \
      1838, 1841, 1843, 1846, 1848, 1851, 1853, 1855, 1858, 1860, 1863, 1865, 1867, \
      1870, 1872, 1874, 1877, 1879, 1881, 1884, 1886, 1888, 1890, 1892, 1895, 1897, \
      1899, 1901, 1903, 1905, 1908, 1910, 1912, 1914, 1916, 1918, 1920, 1922, 1924, \
      1926, 1928, 1930, 1932, 1934, 1936, 1938, 1940, 1942, 1944, 1946, 1947, 1949, \
      1951, 1953, 1955, 1957, 1959, 1961, 1962, 1964, 1966, 1968, 1970, 1971, 1973, \
      1975, 1977, 1978, 1980, 1982, 1984, 1985, 1987, 1989, 1990, 1992, 1994, 1995, \
      1997, 1999, 2000, 2002, 2004, 2005, 2007, 2008, 2010, 2012, 2013, 2015, 2016, \
      2018, 2020, 2021, 2023, 2024, 2026, 2027, 2029, 2030, 2032, 2033, 2035, 2036, \
      2038, 2039, 2041, 2042, 2044, 2045, 2047
    };
    return LogTable[x];
  }
  // Compute 256 * Log2(x)
  template <class T>
  inline int32 log2_256_32(T x_) {
    typedef typename integer_limits<T>::unsigned_t type;
    type x = std::max(x_, T(0));
    int32 l = 0;
    for (int k = std::numeric_limits<type>::digits/2; k; k >>= 1) {
      if (x & (type(-1) << (k+7))) { 
        x >>= k; 
        l += 256*k; 
      }
    }
    return l + log2_256_8(uint32(x));
  }
  
  inline int32 log2_factorial_ramanujan(uint32 n) {
    if (n < 2) return 0;
    static const uint32 scale = (1 << 11);
    static const int32 scale6 = scale / 6;
    static const int32 log2_es  = 756388;
    static const int32 log2_pis2 = 432930;
    return (n * (log2_256_32(n) * scale - log2_es) + log2_256_32(n * (1 + 4 * n * (1 + 2 * n))) * scale6 + log2_pis2) >> 11;
  }
  // 256 * log2(x!) : http://en.wikipedia.org/wiki/Factorial
  inline int32 log2_factorial_stirling(uint32 n) {
    if (n < 2) return 0;
    //static const uint32 scale = (1 << 11);
    static const int32 log2_es  = 756388;
    static const int32 log2_2pis2 = 695074;
    int32 ln = log2_256_32(n);
    return (n * ((ln << 11) - log2_es) + (ln << 10) + log2_2pis2) >> 11;
  }
  // 256 * log2(x!)
  inline int32 log2_factorial_table(uint32 n) {
    const static int32 table[] = {
      0, 0, 256, 662, 1174, 1768, 2430, 3149, 3917, 4728, 5579, 6464, 7382, 8329, 
      9304, 10304, 11328, 12374, 13442, 14529, 15636, 16760, 17902, 19060, 20234, 
      21422, 22626, 23843, 25074, 26317, 27574, 28842, 30122, 31413, 32716, 34029,
      35352, 36686, 38029, 39382, 40745, 42116, 43497, 44886, 46283, 47689, 49103,
      50525, 51955, 53392, 54837, 56289, 57749, 59215, 60688, 62168, 63655, 65148,
      66648, 68154, 69666, 71184, 72709, 74239, 75775, 77316, 78864, 80417, 81975,
      83539, 85108, 86682, 88262, 89846, 91436, 93031, 94630
    };
    return n < countof(table) ? table[n] : log2_factorial_stirling(n);
  }
  inline int32 log2_factorial(uint32 n) {
    return log2_factorial_table(n);
  }
  // 10^(-x/10)
  inline int32 pow10_x_10(int32 x) {
    const static int32 table[] = {
      2147483647, 1705806894, 1354970579, 1076291388, 854928639, 679093956, \
      539423503, 428479319, 340353221, 270352173, 214748365, 170580689, 135497058, \
      107629139, 85492864, 67909396, 53942350, 42847932, 34035322, 27035217, \
      21474836, 17058069, 13549706, 10762914, 8549286, 6790940, 5394235, 4284793, \
      3403532, 2703522, 2147484, 1705807, 1354971, 1076291, 854929, 679094, 539424, \
      428479, 340353, 270352, 214748, 170581, 135497, 107629, 85493, 67909, 53942, \
      42848, 34035, 27035, 21475, 17058, 13550, 10763, 8549, 6791, 5394, 4285, \
      3404, 2704, 2147, 1706, 1355, 1076, 855, 679, 539, 428, 340, 270, 215, 171, \
      135, 108, 85, 68, 54, 43, 34, 27, 21, 17, 14, 11, 9, 7, 5, 4, 3, 3, 2, 2, 1, \
      1, 1, 1, 1
    };
    return x < 0 ? table[0] : x < (int16)countof(table) ? table[x] : 0;
  }
  //-10*log10(x * INT_MAX)
  inline int32 minus10_log10(int32 x) {
    if (x <= 0) return std::numeric_limits<int16>::max();
    return reduce(6115774 - 771 * log2_256_32(x), 16);
  }
  //10*log10(x)
  inline int32 ten_log10(size_t x) {
    if (x == 0) return std::numeric_limits<int16>::min();
    return reduce(771 * log2_256_32(x), 16);
  }
  // 256 * log2(pi)
  static const int32 log2_pi = 423;
  // 256 * log2(e)
  static const int32 log2_e  = 369;


namespace minutia_datarec {
namespace merged {
namespace minutia_fetch {

  inline void imresize23_block(uint8* po, size_t o_w, const uint8* pi, size_t i_w) {
    static const uint16 div = 256;        // power of 2
    static const uint16 mul = div / 9;    //28
    for (uint8 * endline = po + o_w; po < endline; pi += 3, po += 2) {
      uint16 i00 = pi[    0], i01 = pi[      1], i02 = pi[      2];
      uint16 i10 = pi[i_w  ], i11 = pi[i_w  +1], i12 = pi[i_w  +2];
      uint16 i20 = pi[i_w*2], i21 = pi[i_w*2+1], i22 = pi[i_w*2+2];
      uint16 o00 = i00*4 + i01*2 + i10*2 + i11, o01 = i02*4 + i01*2 + i12*2 + i11;
      uint16 o10 = i20*4 + i21*2 + i10*2 + i11, o11 = i22*4 + i21*2 + i12*2 + i11;
      po[0] = o00 * mul / div;              
      po[1] = o01 * mul / div;
      po[o_w] = o10 * mul / div;
      po[o_w + 1] = o11 * mul / div;
    }
  }

  // Can work in-place
  template <size_t out_stride> 
  inline void imresize23(uint8 * out_img, size_t out_width, size_t out_size, const uint8 * in_img, size_t in_width) {
    uint8 buf[2 * out_stride] = {0};
    imresize23_block(buf, out_width, in_img, in_width);
    memcpy(out_img, buf, out_width * 2);

    const uint8 * pi = in_img + in_width * 3;
    uint8 * po = out_img + out_width * 2;
    uint8 * end = out_img + out_size;
    for (; po < end; pi += in_width * 3, po += out_width * 2) {
      imresize23_block(po, out_width, pi, in_width);
    }
  }

  // Can work in-place; AI: remains here for compatibility with MEX only
  template <size_t out_stride> 
  inline void imresize23(uint8 * out_img, size_t & out_width, size_t & out_size, const uint8 * in_img, size_t in_width, size_t in_size) {
    out_width = in_width / 6 * 4; // width should be multiple of 4
    if (out_width > out_stride) {
      // center the image
      in_img += (out_width - out_stride) * 3 / 4;
      out_width = out_stride;
    }
    out_size = in_size / in_width / 6 * 4 * out_width; // height should be multiple of 4
    imresize23<out_stride>(out_img, out_width, out_size, in_img, in_width);
  }

  inline void imresize(uint8 * out_img, size_t o_w, size_t o_size, const uint8 * in_img, size_t i_w, size_t scale256) {
    const uint8 * piv = in_img;
    size_t dy = 0;
    uint8 * end = out_img + o_size;
    for (uint8 * po = out_img; po < end; ) {
      const uint8 * pi = piv;
      size_t dx = 0;
      for (uint8 * endline = po + o_w; po < endline; po++) {
        *po = uint8(((pi[  0] * (256 - dx) + pi[    1] * dx) * (256 - dy)
                   + (pi[i_w] * (256 - dx) + pi[i_w+1] * dx) * dy        ) >> 16);
        dx += scale256;
        pi += dx >> 8;
        dx &= 0xff;
      }
      dy += scale256;
      piv += (dy >> 8) * i_w;
      dy &= 0xff;
    }
  }

} // namespace
}
}
}

#endif // __SCALE_LIMITS_h

