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
#ifndef __MINUTIA_DATA_H
#define __MINUTIA_DATA_H

#include <string.h>

#include <algorithm> // sort


# ifdef PHX_FW_UPGRADE
#   ifndef __IncludeFirst_h
#     error --preinclude=IncludeFirst.h is missing in the command line
#   endif
# endif // PHX_FW_UPGRADE


#include "transformate.h"
#include "image_config.h"
#ifdef USE_DPCOMMON
#include "FullComparison.h"
namespace data_ret = minutia_extractor::minutia_datarec::Full;
#else
namespace data_ret = minutia_extractor::minutia_datarec::merged;
#endif




typedef struct tag_FRFXLL_INPUT_PARAM_ISO_ANSI
{
  size_t length;      // size of this structure, for extensibility
  unsigned int    CBEFF;
  unsigned short  fingerPosition;
  unsigned short  viewNumber;
  unsigned char   rotation;   // <rotation in degrees clockwise> * 128 / 180, keep in mind rounding 
} FRFXLL_INPUT_PARAM_ISO_ANSI;



namespace minutia_extractor {

  template <class Elem, class T> struct memory_copy {
    typedef bool (T::*comp_t)(Elem e1, Elem e2) const;
    const T & t;
    comp_t comp;

    memory_copy(const T & t_, comp_t comp_) 
      : t(t_)
      , comp(comp_)
    {
    }
    bool operator () (Elem e1, Elem e2) const {
      return (t.*comp)(e1, e2);
    }
  };

  template <class Elem, class T>
  inline memory_copy<Elem, T> mem_comp(
    const T & t,
    //typename member_comp<Elem, T>::comp_t comp
    bool (T::*comp)(Elem e1, Elem e2) const
  ) {
    return memory_copy<Elem, T>(t, comp);
  }
  namespace minutia_datarec {
    namespace merged {


  const int ErrorBadFingerprintData = MINEX_RET_FAILURE_UNSPECIFIED;
      const int ErrorBadAlignmentData = MINEX_RET_FAILURE_UNSPECIFIED;
      const int ErrorAlignmentDataDisqualified = MINEX_RET_FAILURE_UNSPECIFIED;

      inline bool big_indian_if() {
        int one = 1;
        return !(*((char *)(&one)));
      };

      struct read_type {
        const unsigned char * start;
        const unsigned char * end;
        const unsigned char * cur;
        bool  dataInBigEndian;
        int rc;

        read_type(const unsigned char data[], size_t size, bool dataInBigEndian_ = false) 
          : start(data)
          , end(data + size)
          , cur(data)
          , dataInBigEndian(dataInBigEndian_)
          , rc(MINEX_RET_SUCCESS) 
        {
        };
        virtual int getch() { 
          if (eof()) {
            SetError(MINEX_RET_BAD_IMAGE_SIZE);
            return -1;
          } else {
            return *(cur++); 
          }
        }
        virtual bool eof() const { return cur >= end; }
        bool bad() const { return rc < MINEX_RET_SUCCESS; }
        void skip(size_t offs) {
          for (;offs && !eof(); --offs) {
            getch();
          }
        }
        read_type & SetError(int rc_) {
          if (!bad()) {
            rc = rc_;
          }
          return *this;
        }
        read_type & operator >> (uint8 & x) { x = (uint8)getch(); return *this; }
        read_type & operator >> (int8 & x) { 
          uint8 t; 
          *this >> t; 
          x = (int8)(t); 
          return *this; 
        }
        read_type & operator >> (uint16 & x) { 
          uint8 msb, lsb;
          if (dataInBigEndian) {
            *this >> msb >> lsb;
          } else {
            *this >> lsb >> msb;
          }
          x = lsb | (uint16(msb) << 8);
          return *this; 
        }
        read_type & operator >> (int16 & x) { 
          uint16 v;
          *this >> v;
          x = (int16)(v);
          return *this;
        }
        read_type & operator >> (uint32 & x) { 
          uint16 msb, lsb;
          if (dataInBigEndian) {
            *this >> msb >> lsb;
          } else {
            *this >> lsb >> msb;
          }
          x = lsb | (uint32(msb) << 16);
          return *this; 
        }
        read_type & operator >> (int32 & x) { 
          uint32 v;
          *this >> v;
          x = (int32)(v);
          return *this;
        }

        read_type & operator >> (uint64 & x) { 
          uint32 msb, lsb;
          if (dataInBigEndian) {
            *this >> msb >> lsb;
          } else {
            *this >> lsb >> msb;
          }
          x = lsb | (uint64(msb) << 32);
          return *this; 
        }

      };

      struct pgm_features {
        static const uint16 DPFP_CODE_BITS_USED_MASK    = 0x8000;
        static const uint16 DPFP_ENCRYPTED_MASK         = 0x4000;
        static const uint16 DPFP_LITTLE_ENDIAN_MASK     = 0x2000;
       
        static const uint16 DPFP_MORE_FEATURES_FOLLOW   = 0x1000;
        static const uint16 DPFP_EXTENDED_FEATURES      = 0x0800;
        static const uint16 DPFP_ALLOW_LEARNING         = 0x0400;
        static const uint16 DPFP_XTEA_MASK              = 0x0200;
        static const uint16 DPFP_PLAIN = 47;      //These remain for backward compatibility.  See FP_ENCRYPTED_MASK and
        static const uint16 DPFP_ENCRYPTED = 74;  //FP_IS_ENCRYPTED below for current usage.
        uint16 value;

        bool shuffle_dig() const { // should work for any byte order
          if (value == DPFP_PLAIN || value == DPFP_ENCRYPTED) return false;
          if (value == (DPFP_PLAIN << 8) || value == (DPFP_ENCRYPTED << 8)) return true; 
          if (value & DPFP_CODE_BITS_USED_MASK) return false;
          if (value & (DPFP_CODE_BITS_USED_MASK >> 8)) return true;
          return false;
        }
        friend read_type & operator >> (read_type & r, pgm_features & ftr) {
          r >> ftr.value;
          if (ftr.shuffle_dig()) {
            ftr.value = ((ftr.value & 0xff) << 8) | ((ftr.value & 0xff00) >> 8);
            r.dataInBigEndian = !r.dataInBigEndian;
          }
          if (ftr.value & DPFP_CODE_BITS_USED_MASK) {
            if (r.dataInBigEndian == ((ftr.value & DPFP_LITTLE_ENDIAN_MASK) != 0)) {
              r.rc = test_output(ErrorBadFingerprintData);
            }
          }
          if (!ftr.IsValid()) {
            r.rc = test_output(ErrorBadFingerprintData);
          }
          return r;
        }
        bool IsValid() const {
          if (value == DPFP_PLAIN || value == DPFP_ENCRYPTED) return true;
          return (value & DPFP_CODE_BITS_USED_MASK) != 0;
        }
        bool if_enc_done() const { return (value & DPFP_CODE_BITS_USED_MASK) ? (value & DPFP_ENCRYPTED_MASK) != 0 : value != DPFP_PLAIN; }
        bool if_enc_done1() const { return if_enc_done() && !(value & DPFP_XTEA_MASK); }
        bool vive_morethan() const { return (value & DPFP_MORE_FEATURES_FOLLOW) != 0; }
        bool HasEFBs() const { return (value & DPFP_EXTENDED_FEATURES) != 0; }
        pgm_features(uint16 code) : value(code) {}
        pgm_features() 
          : value(DPFP_CODE_BITS_USED_MASK | DPFP_ENCRYPTED_MASK | DPFP_LITTLE_ENDIAN_MASK | DPFP_EXTENDED_FEATURES) 
        {}
      };

      inline bool totallen_true(read_type & r, size_t size) { return r.end >= r.start && size <= (size_t)(r.end - r.start); }
      inline bool len_correct(read_type & r, size_t size) { return r.end >= r.cur && size <= (size_t)(r.end - r.cur); }
      inline bool fp_correct(uint16 fingerPosition) { return fingerPosition <= 10; };
      inline bool resolution_true(uint16 resolution) { return resolution >= 99 && resolution <= 1000; };
      inline bool imagesize_correct(uint16 imageSize) { return imageSize >= 16 && imageSize < (1<<14); };

      struct deserialize_imag {
        static const uint16 Resolution = 167;
        static const uint32 CBEFF_DEFAULT = 0x00330000;
        const unsigned char * const dataIn;
        const size_t & dataInSize;
        size_t totalLength; // in ISO 4 bytes; in ANSI can be 2 bytes (0x001A - 0xFFFF) or 6 bytes (0x000000010000 - 0x0000FFFFFFFF)
        uint8  devType1, devType2;
        uint16 imageSizeX;
        uint16 imageSizeY;
        uint16 resolutionX;
        uint16 resolutionY;
        uint8  numOfFingerViews;
        uint8  numOfMinutiae;

        uint32 CBEFF;
        uint16 specifiedFinger;
        uint16 specifiedView;
        uint8  rotation;

        deserialize_imag(const unsigned char * dataIn_,
                           const size_t & dataInSize_)
          : dataIn(dataIn_)
          , dataInSize(dataInSize_)
          , CBEFF(0) 
          , specifiedFinger(MINEX_FINGER_UNKNOWN)
          , specifiedView(MINEX_FINGER_UNKNOWN)
          , rotation(0)
        {
        }
        virtual void minu_data_total_read(read_type & r) {} // in ISO 4 bytes; in ANSI can be 2 bytes (0x001A - 0xFFFF) or 6 bytes (0x000000010000 - 0x0000FFFFFFFF)
        virtual void ReadDeviceInfo(read_type & r) {}
        virtual void ReadTheta(read_type & r, minutia_param & mp) {}
        int ReadRecordHeaders(read_type & r) {
          int rc = MINEX_RET_SUCCESS;
          uint32 formatID;
          uint8  majorVersion;
          uint8  minorVersion;
          r >> formatID;
          if ( 0x464D5200 != formatID ) return test_output(ErrorBadFingerprintData);
          r.cur += 1;                                       // skip the leading space.
          r >> majorVersion;
          if ( majorVersion - '0' < 2 ) return test_output(ErrorBadFingerprintData);
          r >> minorVersion;
          r.cur += 1;                                       // skip the ending zero.
          minu_data_total_read(r); // in ISO 4 bytes; in ANSI can be 2 bytes (0x001A - 0xFFFF) or 6 bytes (0x000000010000 - 0x0000FFFFFFFF)
          if ( !totallen_true(r, totalLength) ) return test_output(ErrorBadFingerprintData);
          ReadDeviceInfo(r);
          r >> imageSizeX;
          if ( !imagesize_correct(imageSizeX) ) return test_output(ErrorBadFingerprintData);
          r >> imageSizeY;
          if ( !imagesize_correct(imageSizeY) ) return test_output(ErrorBadFingerprintData);
          r >> resolutionX;
          if ( !resolution_true(resolutionX) ) return test_output(ErrorBadFingerprintData);
          r >> resolutionY;
          if ( !resolution_true(resolutionY) ) return test_output(ErrorBadFingerprintData);
          r >> numOfFingerViews;
          r.cur += 1;                                       // skip the reserved byte.
          return rc;
        }
        static uint8 nfiq_conf(uint8 quality) {
          return quality * 2;
        }
        static uint8 nfiq_qt_fp(uint8 quality) {
          return quality;
        }
        int min_rd_fp(read_type & r, compare_minutia & md) {
          uint16 x;
          uint16 y;
          const unsigned char * saved = r.cur;
          md.numMinutia = min((uint8)compare_minutia::Capacity, numOfMinutiae);
          for ( uint16 i = 0; i < md.numMinutia; i++ ) {
            r >> x;
            md.minutia[i].type = uint8(x >> 14);
            x &= 0x3FFF;
            md.minutia[i].position.x = muldiv(x, Resolution, resolutionX);
            r >> y;
            y &= 0x3FFF;
            md.minutia[i].position.y = muldiv(y, Resolution, resolutionY);
            ReadTheta(r, md.minutia[i]);
            uint8 b;
            r >> b;
            AssertR( b <= 100, ErrorBadFingerprintData );
            md.minutia[i].conf = nfiq_conf(b);
          }
          r.cur = saved + 6 * numOfMinutiae;
          return MINEX_RET_SUCCESS;
        }
        void fp_foot_base(compare_minutia & md) {
          md.offset.x = 0; md.offset.y = 0;
          if ( md.numMinutia > 0 ) {
            md.offset = md.minutia[0].position;
            for ( uint16 i = 1; i < md.numMinutia; i++ ) {
              locate & pos = md.minutia[i].position;
              if ( pos.x < md.offset.x ) md.offset.x = pos.x;
              if ( pos.y < md.offset.y ) md.offset.y = pos.y;
            }
          }
          uint16 xmax = 0x0000;
          uint16 ymax = 0x0000;
          uint16 j = 0;
          for (uint16 i = 0; i < md.numMinutia; i++) {
            locate & pos = md.minutia[i].position;
            pos.x -= md.offset.x;
            pos.y -= md.offset.y;
            if ( pos.x > 256 || pos.y > 400 ) { // rare case of out-of-bound minutia.
              continue;
            }
            if ( pos.x > xmax ) xmax = pos.x;
            if ( pos.y > ymax ) ymax = pos.y;
            if ( i != j ) {                     // shift the minutia as necessary.
              md.minutia[j] = md.minutia[i];
            }
            j++;
          }
          md.numMinutia = j;

          md.footprint.area = uint32(xmax) * ymax;
          xmax = (xmax+4)/8; ymax = (ymax+4)/8;
          for ( int32 y = 0; y <= ymax; y++ ) {
            for ( int32 x = 0; x <=xmax; x++ ) {
              md.footprint.Pixel(x, y) = true;
            }
          }
        }
        bool CheckView(uint8 finger, uint8 view) {
          return ( specifiedFinger == MINEX_FINGER_UNKNOWN && specifiedView == MINEX_FINGER_UNKNOWN ) ||
                 ( specifiedFinger == finger && specifiedView == MINEX_FINGER_UNKNOWN ) ||
                 ( specifiedFinger == finger && specifiedView == view );
        }
        int minu_read_view(read_type & r, compare_minutia & md) {
          int rc = MINEX_RET_FAILURE_UNSPECIFIED;
          uint8 finger;
          uint8 b;
          uint8 view;
          uint8 impressionType;
          uint8 fingerQuality;
          uint16 viewEDBLength;

          r >> finger;
          r >> b;
          view = b >> 4;
          impressionType = b & 0x0F;
          r >> fingerQuality;
          r >> numOfMinutiae;

          bool viewChecked = CheckView(finger, view);
          if ( viewChecked ) {
            md.Init();
            CheckR( min_rd_fp(r, md) );
            if (rotation) {
              md.drift_n_swap(rotation);
            }
            fp_foot_base(md);
            rc = MINEX_RET_SUCCESS;
          } else {
            r.cur += 6 * numOfMinutiae;
          }

          r >> viewEDBLength;
          if ( !len_correct(r, viewEDBLength) ) return test_output(ErrorBadFingerprintData);
          r.cur += viewEDBLength;
          md.calculate();
          return rc;
        }
        int ReadViews(read_type & r, compare_minutia & md) {
          int rc = MINEX_RET_SUCCESS;
          for (uint8 i = 0; i < numOfFingerViews; i++) {
            rc = minu_read_view(r, md);
            if ( MINEX_RET_FAILURE_UNSPECIFIED != rc ) break;
          }
          return rc;
        }
        int param_unbind(const void * parameters) {
          if ( NULL == parameters ) return MINEX_RET_SUCCESS;
          const FRFXLL_INPUT_PARAM_ISO_ANSI * pParams = reinterpret_cast<const FRFXLL_INPUT_PARAM_ISO_ANSI *>(parameters);
          if ( NULL == pParams ) return MINEX_RET_FAILURE_NULL_TEMPLATE;
          uint8 version = 0;
          switch ( pParams->length ) {
            case sizeof(FRFXLL_INPUT_PARAM_ISO_ANSI):
              version = 1;
              break;
            default:
              return MINEX_RET_FAILURE_NULL_TEMPLATE;
          }
          if (version >= 1 && pParams->CBEFF != 0) CBEFF = pParams->CBEFF;
          if (version >= 1 && pParams->fingerPosition != MINEX_FINGER_UNKNOWN) specifiedFinger = pParams->fingerPosition;
          if (version >= 1 && pParams->viewNumber != MINEX_FINGER_UNKNOWN) specifiedView = pParams->viewNumber;
          if (version >= 1) rotation = pParams->rotation;
          return MINEX_RET_SUCCESS;
        }
        int feature_deseial(compare_minutia & md, const void * parameters) {
          int rc = param_unbind(parameters);
          if ( rc < MINEX_RET_SUCCESS ) return rc;

          read_type r(dataIn, dataInSize, true);
          rc = ReadRecordHeaders(r);
          if ( MINEX_RET_SUCCESS == rc && numOfFingerViews > 0 ) {
            rc = ReadViews(r, md);
          }
          return rc;
        }
      };

      struct deserialize_iso : public deserialize_imag {
        deserialize_iso(const unsigned char * dataIn_,
                           const size_t & dataInSize_)
        : deserialize_imag(dataIn_, dataInSize_) {
        }
        virtual void minu_data_total_read(read_type & r) { // in ISO 4 bytes
          uint32 length;
          r >> length;
          totalLength = length;
        }
        virtual void ReadDeviceInfo(read_type & r) {
          r >> devType1 >> devType2;                                // Device type ID.
        }
        virtual void ReadTheta(read_type & r, minutia_param & mp) {
          r >> mp.theta;
        }
      };

      struct deserialize_ansi : public deserialize_imag {
        deserialize_ansi(const unsigned char * dataIn_,
                            const size_t & dataInSize_)
        : deserialize_imag(dataIn_, dataInSize_) {}
        
        virtual void minu_data_total_read(read_type & r) { // in ANSI can be 2 bytes (0x001A - 0xFFFF) or 6 bytes (0x000000010000 - 0x0000FFFFFFFF)
          uint32 length;
          r >> (uint16&)length;
          if ((uint16&)length != 0) {
            length = (uint16&)length;
          } else {
            r >> length;
          }
          totalLength = length;
        }
        virtual void ReadDeviceInfo(read_type & r) {
          r >> CBEFF;                                       // CBEFF product ID.
          r >> devType1 >> devType2;                        // Equipment ID.
        }
        virtual void ReadTheta(read_type & r, minutia_param & mp) {
          uint8 theta;
          r >> theta;
          theta = muldiv(theta, 256, 180);
          theta = -theta + 64;
        }
      };

      inline int minu_iso_read(
        const unsigned char data[],
        size_t size, 
        compare_minutia & md, 
        const void * parameters
      ) {
        deserialize_iso isoFDS(data, size);
        return isoFDS.feature_deseial(md, parameters);
      }

      inline int minu_ansi_read(
        const unsigned char data[], 
        size_t size, 
        compare_minutia & md, 
        const void * parameters
      ) {
        deserialize_ansi ansiFDS(data, size);
        return ansiFDS.feature_deseial(md, parameters);
      }

      struct Writer : public HResult {
        unsigned char * data;
        size_t size;
        size_t cur;
        bool dataInBigEndian;
        bool reversed; // data written in the stream will be reversed

        Writer(unsigned char * data_,  size_t size_, bool dataInBigEndian_ = false)
          : data(data_), size(size_), cur(0), dataInBigEndian(dataInBigEndian_), reversed(false) {
        };
        virtual int putch(int32 c) { 
          if (!data) {
          } else if (eof()) {
            data = NULL;
          } else {
            data[cur] = uint8(c & 0xff);  
          }
          cur++;
          return GetResult();
        }
        virtual bool eof() const { return data ? cur >= size : false; }
        bool bad() const { return rc < MINEX_RET_SUCCESS; }
        bool bigindian_ins() const {
          return dataInBigEndian != reversed;
        }
        Writer & SetError(int rc_) {
          if (!bad()) rc = rc_;
          return *this;
        }
        Writer & operator << (uint8 x) {
          putch(x);
          return *this;
        }
        Writer & operator << (int8 x) {
          putch(x);
          return *this;
        }
        Writer & operator << (uint16 x) {
          if (bigindian_ins()) {
            putch(x >> 8);
            putch(x);
          } else {
            putch(x);
            putch(x >> 8);
          }
          return *this;
        }
        Writer & operator << (int16 x) {
          return *this << (uint16)x;
        }
        Writer & operator << (uint32 x) {
          if (bigindian_ins()) {
            *this << (uint16)(x >> 16) << (uint16)(x);
          } else {
            *this << (uint16)(x) << (uint16)(x >> 16);
          }
          return *this;
        }
        Writer & operator << (int32 x) {
          return *this << (uint32)x;
        }
      };
      struct Reversed {
        Writer & w;
        bool oldReversed;

        Reversed(Writer & w_, bool reversed = true) 
          : w(w_)
          , oldReversed(w_.reversed) 
        {
          w.reversed = reversed;
        }
        ~Reversed() { w.reversed = oldReversed; }
      };

      inline void Reverse(unsigned char data[], size_t size) {
        for (size_t i = 0; i < --size; i++) {
          unsigned char t = data[i];
          data[i] = data[size];
          data[size] = t;
        }
      }

      struct serialize_fingerimage {
        static const uint16 ImageSizeX = 250;
        static const uint16 ImageSizeY = 400;
        static const uint16 Resolution = deserialize_imag::Resolution;
        static const uint16 DefaultResolution = 197; // 500DPI / 2.54

        unsigned char * const dataOut;
        const size_t dataOutSize;
        size_t recordHeaderLength;
        size_t viewDataOffset;
        size_t viewDataLength;
        size_t viewEDBOffset;
        size_t viewEDBLength;

        uint32 CBEFF;
        uint16 fingerPosition;
        uint16 viewNumber;
        uint16 resolutionX;
        uint16 resolutionY;
        uint16 imageSizeX;
        uint16 imageSizeY;
        uint8  rotation;
        uint8  fingerQuality;
        uint8  impressionType;

        serialize_fingerimage(unsigned char * dataOut_,
                         const size_t & dataOutSize_,
                         bool & isExtended_)
          : dataOut(dataOut_)
          , dataOutSize(dataOutSize_)
        {
          viewDataOffset =  0;
          viewDataLength =  0;
          viewEDBOffset =   0;
          viewEDBLength =   0;
          CBEFF =           0x00330000;
          fingerPosition =  0;
          viewNumber =      0;
          resolutionX =     DefaultResolution;
          resolutionY =     DefaultResolution;
          imageSizeX  =     0;
          imageSizeY  =     0;
          rotation    =     0;
          fingerQuality  =  86;
          impressionType =  0; // live scan
        }
        static uint8 nfi_from_conf(uint8 confidence) {
          return min<uint8>((confidence + 1) / 2, 100);
        }
        virtual void tot_len_ins(Writer & wr, size_t len) {}; // in ISO 4 bytes in ANSI can be 2 bytes (0x001A - 0xFFFF) or 6 bytes (0x000000010000 - 0x0000FFFFFFFF)
        virtual void dev_info_ins(Writer & wr, const compare_minutia & md) {};
        virtual void im_size_info(Writer & wr, uint16 sizeX, uint16 sizeY) {};
        virtual void thet_ins(Writer & wr, uint8 theta) {};
        void WriteMinutiae(Writer & wr, const compare_minutia & md) {
          uint16 sizeX = 0;
          uint16 sizeY = 0;

          size_t i;
          for ( i = 0; i < md.numMinutia; i++ ) {
            locate pos = md.minutia[i].position + md.offset;
            if ( resolutionX != Resolution ) {
              pos.x = muldiv(pos.x, resolutionX, Resolution);
            }
            if ( pos.x > sizeX ) sizeX = pos.x;
            if ( resolutionY != Resolution ) {
              pos.y = muldiv(pos.y, resolutionY, Resolution);
            }
            if ( pos.y > sizeY ) sizeY = pos.y;
            wr << uint16((pos.x & 0x3FFF) | (md.minutia[i].type << 14));    // X
            wr << uint16(pos.y & 0x3FFF);                                   // Y
            thet_ins(wr, md.minutia[i].theta);                            // Theta
            wr << nfi_from_conf(md.minutia[i].conf);                // Quality
          }
          size_t saved = wr.cur;
          im_size_info(wr, imageSizeX != 0 ? imageSizeX : (sizeX + 8), imageSizeY != 0 ? imageSizeY : (sizeY + 8));
          wr.cur = saved;
        }
        void rec_hea_ins(Writer & wr, const compare_minutia & md) {
          wr.cur = 0;
          wr << uint32(0x464D5200);                                         // Format Identifier
          wr << uint32(0x20323000);                                         // Version of this standard
          tot_len_ins(wr, 0);                                          // Length of total record in bytes
          dev_info_ins(wr, md);                                          // Capture device/system information
          wr << uint16(imageSizeX);                                         // Image Size in X
          wr << uint16(imageSizeY);                                         // Image Size in Y
          wr << uint16(resolutionX);                                        // X (horizontal) Resolution
          wr << uint16(resolutionY);                                        // Y (vertical) Resolution
          wr << uint8(0);                                                   // Number of Finger Views
          wr << uint8(0);                                                   // Reserved byte
        }
        void readable_data_ins(Writer & wr, const compare_minutia & md) {
          wr.cur = viewDataOffset;
          wr << uint8(fingerPosition);                                      // Finger Position
          wr << uint8((viewNumber<<4) + (impressionType & 0x0F));           // View Number
                                                                            // Impression Type
          wr << fingerQuality;                                              // Finger Quality
          wr << uint8(md.numMinutia);                                       // Number of Minutiae
          WriteMinutiae(wr, md);
          wr << uint16(0);                                                  // Extended Data Block Length
          wr.cur = viewDataOffset - 2;
          wr << uint8(1);                                                   // Number of Finger Views
        }
        void readable_datahead_ins(Writer & wr) {
          wr.cur = viewEDBOffset - 2;
          wr << uint16(viewEDBLength);                                      // Extended Data Block Length
          wr << uint8(0x01) << uint8(0x01);                                 // Extended Data Area Type Code
          wr << uint16(0);                                                  // Extended Data Area Length
        }
        void set_offset_calc(size_t & size, const compare_minutia & md) {
          viewDataOffset = recordHeaderLength;
          viewDataLength = 6 + 6 * md.numMinutia; // 4 = view header length; 2 = EDB Length length
          viewEDBOffset = viewDataOffset + viewDataLength;
          viewEDBLength = 0;
          size = recordHeaderLength + viewDataLength + viewEDBLength;
        }
        int param_unbind(const void * parameters) {
          if ( NULL == parameters ) return MINEX_RET_SUCCESS;
          const result_formates * pParams = reinterpret_cast<const result_formates *>(parameters);
          if ( NULL == pParams ) return MINEX_RET_FAILURE_NULL_TEMPLATE;
          uint8 version = 0;
          switch ( pParams->length ) {
            case sizeof(result_formates):
              version = 1;
              break;
            default:
              return MINEX_RET_FAILURE_NULL_TEMPLATE;
          }
          if (version >= 1 && pParams->CBEFF != 0) CBEFF = pParams->CBEFF;
          if (version >= 1 && pParams->fingerPosition != MINEX_FINGER_UNKNOWN) fingerPosition = pParams->fingerPosition;
          if ( !fp_correct(fingerPosition) ) return MINEX_RET_FAILURE_NULL_TEMPLATE;
          if (version >= 1 && pParams->viewNumber != MINEX_FINGER_UNKNOWN)   viewNumber = pParams->viewNumber;
          if (version >= 1 && pParams->resolutionX != MINEX_FINGER_UNKNOWN) resolutionX = pParams->resolutionX;
          if ( !resolution_true(resolutionX) ) return MINEX_RET_FAILURE_NULL_TEMPLATE;
          if (version >= 1 && pParams->resolutionY != MINEX_FINGER_UNKNOWN) resolutionY = pParams->resolutionY;
          if ( !resolution_true(resolutionY) ) return MINEX_RET_FAILURE_NULL_TEMPLATE;
          if (version >= 1 && pParams->imageSizeX != 0) {
            imageSizeX = pParams->imageSizeX;
            if ( !imagesize_correct(imageSizeX) ) return MINEX_RET_FAILURE_NULL_TEMPLATE;
          }
          if (version >= 1 && pParams->imageSizeY != 0) {
            imageSizeY = pParams->imageSizeY;
            if ( !imagesize_correct(imageSizeY) ) return MINEX_RET_FAILURE_NULL_TEMPLATE;
          }
          if (version >= 1) rotation = pParams->rotation;
          if (version >= 1) {
            if (pParams->fingerQuality > 100) return MINEX_RET_FAILURE_NULL_TEMPLATE;
            fingerQuality = pParams->fingerQuality;
          }
          if (version >= 1) {
            if (pParams->impressionType > 8)  return MINEX_RET_FAILURE_NULL_TEMPLATE;
            impressionType = pParams->impressionType;
          }
          return MINEX_RET_SUCCESS;
        }
        int SerializeFeatureSet(
          size_t & size,
          const compare_minutia & md0,
          const void * parameters,
          bool fixedSize,
          pgm_features ftrCode
        ) {
          int rc = param_unbind(parameters);
          if ( rc < MINEX_RET_SUCCESS ) return rc;
          compare_minutia md(md0);
          if (rotation) {
            md.drift_n_swap(rotation);
          }

          set_offset_calc(size, md);
          if ( NULL == dataOut ) return MINEX_RET_SUCCESS;
          if ( dataOutSize < recordHeaderLength ) return MINEX_RET_BAD_IMAGE_SIZE;

          size_t   totalLength = 0;
          Writer   wr(dataOut, dataOutSize, true);
          rc = MINEX_RET_BAD_IMAGE_SIZE;
          rec_hea_ins(wr, md);
          totalLength = recordHeaderLength;
          if ( dataOutSize >= viewDataOffset + viewDataLength ) {
            readable_data_ins(wr, md);
            totalLength += viewDataLength;
          }
          tot_len_ins(wr, totalLength); // in ISO 4 bytes; in ANSI can be 2 bytes (0x001A - 0xFFFF) or 6 bytes (0x000000010000 - 0x0000FFFFFFFF)
          return MINEX_RET_SUCCESS;
        }
      };

      struct serialize_iso : public serialize_fingerimage {
        serialize_iso(unsigned char * dataOut_,
                         const size_t & dataOutSize_,
                         bool & isExtended_)
                         : serialize_fingerimage(dataOut_, dataOutSize_, isExtended_) {
          recordHeaderLength = 24;
        }
        virtual void tot_len_ins(Writer & wr, size_t len) { // in ISO 4 bytes
          wr.cur = 8;
          wr << uint32(len);
        }
        virtual void dev_info_ins(Writer & wr, const compare_minutia & md) {
          wr << uint8(0) << uint8(0);
        }
        virtual void im_size_info(Writer & wr, uint16 sizeX, uint16 sizeY) {
          wr.cur = 14;
          wr << sizeX;
          wr << sizeY;
        }
        virtual void thet_ins(Writer & wr, uint8 theta) {
          wr << uint8(-theta + 64);
        }
      };

      struct serialize_ansi : public serialize_fingerimage {
        serialize_ansi(unsigned char * dataOut_,
                          const size_t & dataOutSize_,
                          bool & isExtended_)
                          : serialize_fingerimage(dataOut_, dataOutSize_, isExtended_) {
          recordHeaderLength = 26;
        }
        virtual void tot_len_ins(Writer & wr, size_t len) { // in ANSI can be 2 bytes (0x001A - 0xFFFF) or 6 bytes (0x000000010000 - 0x0000FFFFFFFF)
          wr.cur = 8;
          wr << uint16(len); // 2 bytes should be enough
        }
        virtual void dev_info_ins(Writer & wr, const compare_minutia & md) {
          wr << CBEFF;                                      // CBEFF product identifier
          wr << uint8(0) << uint8(0);
        }
        virtual void im_size_info(Writer & wr, uint16 sizeX, uint16 sizeY) {
          wr.cur = 16;
          wr << sizeX;
          wr << sizeY;
        }
        virtual void thet_ins(Writer & wr, uint8 theta) {
          theta = -theta + 64;
          theta = muldiv(theta, 180, 256);
          wr << uint8(theta);
        }
      };

      inline int iso_data_insert(
        unsigned char dataOut[], 
        size_t & size, 
        const compare_minutia & md,
        const void * parameters,
        bool isExtended,
        bool fixedSize = false,
        bool isTemplate = false,
        pgm_features ftrCode = pgm_features()
      ) {
        serialize_iso isoFS(dataOut, size, isExtended);
        return isoFS.SerializeFeatureSet(size, md, parameters, fixedSize, ftrCode);
      }

      inline int WriteAnsiFeatures(
        unsigned char dataOut[], 
        size_t & size, 
        const compare_minutia & md, 
        const void * parameters,
        bool isExtended,
        bool fixedSize = false,
        bool isTemplate = false,
        pgm_features ftrCode = pgm_features()
      ) {
        serialize_ansi ansiFS(dataOut, size, isExtended);
        return ansiFS.SerializeFeatureSet(size, md, parameters, fixedSize, ftrCode);
      }

    }
    using namespace merged;

    template <uint32 v0, uint32 v1> class Signature {
      volatile uint32 data[2];
      void Clear() { data[0] = data[1] = 0; }
    public:
      Signature() {
        data[0] = v0;
        data[1] = v1;
      };
      bool Check1() const {
        if (this == NULL) return false;
        return v0 == data[0] && v1 == data[1];
      }
      ~Signature() { Clear(); }
    };

    class Context;
    class ObjectBase {
    public:
      struct FreeDelegate {
        void      (*free)     (void * block, void * heapContext);     // Function to free block of memory on the heap
        void * heapContext;   ///< Context passed into heap functions. Heap should be ready to use prior to calling FRFXLLCreateContext
      };
      static void operator delete (void * p) throw() {
        if (p == NULL) return;
        FreeDelegate * fd = reinterpret_cast<FreeDelegate*>(p) - 1;
        fd->free(fd, fd->heapContext);
      }
      // We are not planning to use exceptions, but just in case
      static void operator delete (void * p, const minu_fetch_begin * ctxi)  throw() {
        if (p == NULL) return;
        ctxi->free(p, ctxi->heapContext);
      }
      static void * operator new (size_t size, const minu_fetch_begin * ctxi)  throw() {
        if (!ctxi) return NULL;
        FreeDelegate * fd = reinterpret_cast<FreeDelegate*>(ctxi->malloc(size + sizeof(FreeDelegate), ctxi->heapContext));
        if (fd == NULL) return NULL;
        fd->heapContext = ctxi->heapContext;
        fd->free = ctxi->free;
        return fd + 1;
      }
      // We need placement new as well
      static void * operator new (size_t size, void * p)  throw() {
        return p;
      }
      // We are not planning to use exceptions, but just in case
      static void operator delete (void * p, void * ctxi)  throw() {
      }
    };
    class Object : public ObjectBase {
      friend class Context;
      template <class T> friend struct Ptr;
      // Signature<0x656a624f, 0x96007463> objSignature;
    public:
      mutable volatile long refCount;
      const Context * ctx;

      Object(const Context * ctx_);
      virtual ~Object();
      int AddRef() const;
      int Release() const;

      int data_proc(void * * pHandle, int rc = MINEX_RET_SUCCESS) const;
      int data_proc(void * * pHandle, int rc = MINEX_RET_SUCCESS);
    };

    class Context : public Signature<0x747e7f63, 0x747865>, public Object, public minu_fetch_begin {
    public:
      data_ret::Image_config settings;
    private:
      static long inc_data(volatile long * value) {
        return ++(*value);
      }
      static long dec_data(volatile long * value) {
        return --(*value);
      }
      static long data_trnsf(volatile long * dest, long value) {
        long tmp = *dest;
        *dest = value;
        return tmp;
      }
      static long data_match_exc(volatile long * dest, long value, long comperand) {
        long tmp = *dest;
        if (*dest == comperand) *dest = value;
        return tmp;
      }
    public:
      Context(minu_fetch_begin &ctxi_) : Object(NULL), minu_fetch_begin(ctxi_) {
        ctx = this;
        if (interlocked_decrement == NULL) interlocked_decrement = &dec_data;
        if (interlocked_increment == NULL) interlocked_increment = &inc_data;
        if (interlocked_exchange  == NULL) interlocked_exchange  = &data_trnsf;
        if (interlocked_compare_exchange == NULL) interlocked_compare_exchange = &data_match_exc;
      }
      virtual ~Context() {
        ctx = NULL;  // To prevent form releasing itself one extra time in ~Object()
      }
      long sync_data_inc(volatile long & value) const {
        return interlocked_increment(&value);
      }
      long sync_data_dec(volatile long & value) const {
        return interlocked_decrement(&value);
      }
      long InterlockedExchange(volatile long & target, long value) const {
        return interlocked_exchange(&target, value);
      }
      long sync_match(volatile long & target, long value, long comperand) const {
        return interlocked_compare_exchange(&target, value, comperand);
      }
    };

    inline Object::Object(const Context * ctx_) : refCount(0), ctx(ctx_) { 
      
      if (ctx) ctx->AddRef(); // Object created on a context should increase refcount of a context
    }
    inline Object::~Object() {
      if (ctx) ctx->Release(); // When object is destroyed it should release context
    }
    inline int Object::AddRef() const  { 
      if (!ctx) return test_output(MINEX_RET_FAILURE_NULL_TEMPLATE);
      ctx->sync_data_inc(refCount);
      return MINEX_RET_SUCCESS;
    }

    inline int Object::Release() const {
      if (!ctx) return test_output(MINEX_RET_FAILURE_NULL_TEMPLATE);
      if (ctx->sync_data_dec(refCount) == 0) {
        delete this;
      }
      return MINEX_RET_SUCCESS;
    }

    template <class T> T * FromHandle(void * h);

    template <class T> struct Ptr : public ObjectBase {
      T * ptr;
      Ptr() : ptr(NULL) {}
      Ptr(void * h);

      template <class T1> Ptr(T1 * p) : ptr(p) {
        if (ptr) ptr->AddRef();
      }
      template <class T1> Ptr(const Ptr<T1> & p) : ptr(p.ptr) {
        if (ptr) ptr->AddRef();
      }
      // Important: template c-tor does not override implicit copy c-tor!!!
      Ptr(const Ptr<T> & p) : ptr(p.ptr) {
        if (ptr) ptr->AddRef();
      }
      ~Ptr() {
        if (ptr) ptr->Release();
      }
      // operator bool () const { return ptr != NULL; }
      operator T * () { return ptr; }
      T * operator -> () { return ptr; }
      const T * operator -> () const { return ptr; }
      T & operator * () { return *ptr; }
      const T & operator * () const { return *ptr; }
      Ptr & operator = (Ptr p) {    // AI: cannot use const Ptr & as a parameter because p may be a member of *ptr,
        if (ptr != p.ptr) {         //     in which case it may be destroyed too early
          if (p) p->AddRef();       // Must go first!!!
          if (ptr) ptr->Release();
          ptr = p.ptr;
        }
        return *this;
      }
    };

    struct Handle : public Signature<0x646e6168, 0x0000656c>, public ObjectBase {
      Ptr<Object> ptr;
      Handle(Object * pObj)
        : ptr(pObj)
      {}
    };
    struct ConstHandle : public Signature<0x646e6148, 0x0100656c>, public ObjectBase {
      Ptr<const Object> ptr;
      ConstHandle(const Object * pObj)
        : ptr(pObj)
      {}
    };
    inline int Object::data_proc(void * * pHandle, int rc) const {
      if (rc < MINEX_RET_SUCCESS) return rc;
      void * handle = new(ctx) ConstHandle(this);
      if (handle == NULL)
	{
	 return MINEX_RET_FAILURE_UNSPECIFIED;
	}
      *pHandle = handle;
      return rc;
    }
    inline int Object::data_proc(void * * pHandle, int rc) {
      if (rc < MINEX_RET_SUCCESS) return rc;
      void * handle = new(ctx) Handle(this);
      if (handle == NULL) return MINEX_RET_FAILURE_UNSPECIFIED;
      *pHandle = handle;
      return rc;
    }

    template <class T> const T * FromHandle(void * h, const Ptr<const T> &) {
      if (h == NULL) return NULL;
      ConstHandle * constHandle = reinterpret_cast<ConstHandle*>(h);
      if (constHandle->Check1()) {
        const T * ptr = static_cast<const T*>(constHandle->ptr.ptr);
        if (ptr->Check1()) {
          ptr->AddRef();
        } else {
          ptr = NULL;
        }
        return ptr;
      }
      return FromHandle(h, Ptr<T>());
    }
    template <class T> T * FromHandle(void * h, const Ptr<T> &) {
      if (h == NULL) return NULL;
      Handle * handle = reinterpret_cast<Handle*>(h);
      if (handle->Check1()) {
        T * ptr = static_cast<T*>(handle->ptr.ptr);
        if (ptr->Check1()) {
          ptr->AddRef();
        } else {
          ptr = NULL;
        }
        return ptr;
      }
      return NULL;
    }

    template <class T> Ptr<T>::Ptr(void * h)
      : ptr(NULL) 
    {
      ptr = FromHandle(h, *this);
    }

    struct feature_object : public Signature<0x74465046, 0x74655372>, public Object {
      compare_minutia fpFtrSet;

      feature_object(const Context * ctx_)  : Object(ctx_) {}

      int Import(
        const unsigned char data[],  ///< [in] (fingerprint) data to import
        size_t size,                 ///< [in] size of the (fingerprint) data
        unsigned int dataType,   ///< [in] type and format of the (fingerprint) data
        const void * parameters      ///< [in] parameters structure, specific to the data type
      ) {
        switch (dataType) {
         /* case FRFXLL_DT_ISO_FEATURE_SET:
            return minu_iso_read(data, size, fpFtrSet, parameters);*/
          case FRFXLL_DT_ANSI_FEATURE_SET:
            return minu_ansi_read(data, size, fpFtrSet, parameters);
          default:
            return test_output(MINEX_RET_FAILURE_NULL_TEMPLATE);
        }
      }

      int Export(
        unsigned int dataType,   ///< [in] type and format of the (fingerprint) data
        const void * parameters,     ///< [in] parameters structure, specific to the data type
        unsigned char pData[],       ///< [in] (fingerprint) data to import
        size_t *pSize                ///< [in] size of the (fingerprint) data
      ) const {
        int rc = MINEX_RET_SUCCESS;
        unsigned char * data = pData;
        size_t size = data ? *pSize : size_t(-1);
        switch (dataType) {
        /*  case FRFXLL_DT_ISO_FEATURE_SET:
            rc = iso_data_insert(data, size, fpFtrSet, parameters, false);
            break;*/
          case FRFXLL_DT_ANSI_FEATURE_SET:
            rc = WriteAnsiFeatures(data, size, fpFtrSet, parameters, false);
            break;
          default:
            return test_output(MINEX_RET_FAILURE_NULL_TEMPLATE);
        }
        if (rc >= MINEX_RET_SUCCESS || rc == MINEX_RET_BAD_IMAGE_SIZE) {
          *pSize = size;
        }
        return rc;
      }
    };

    template <class T> inline int call_func(
      int (T::*import_f) (
        const unsigned char data[],  ///< [in] (fingerprint) data to import
        size_t size,                 ///< [in] size of the (fingerprint) data
        unsigned int dataType,     ///< [in] type and format of the (fingerprint) data
        const void * parameters      ///< [in] parameters structure, specific to the data type
      ),
      const Context * ctx,         ///< [in] Context
      const unsigned char data[],  ///< [in] (fingerprint) data to import
      size_t size,                 ///< [in] size of the (fingerprint) data
      unsigned int dataType,     ///< [in] type and format of the (fingerprint) data
      const void * parameters,     ///< [in] parameters structure, specific to the data type
      void ** pHandle       ///< [out] pointer to where to put an open handle to the fingerprint data
    ) {
      Ptr<T> pT(new(ctx) T(ctx));
      if (!pT)
	{
	 return test_output(MINEX_RET_FAILURE_UNSPECIFIED); 
	}
      int rc = (pT->*import_f)(data, size, dataType, parameters);
      if (!MINEX_RET_SUCCESS) return rc;
      const T * cpT = pT;
      return cpT->data_proc(pHandle, rc);
    }

    template <class T> int call_func(
      int (T::*export_f) (
        unsigned int dataType,     ///< [in] type and format of the (fingerprint) data
        const void * parameters,     ///< [in] parameters structure, specific to the data type
        unsigned char pData[],       ///< [in] (fingerprint) data to import
        size_t *pSize                ///< [in] size of the (fingerprint) data
      ) const,
      void * handle,          ///< [in] Handle to data object to export
      unsigned int dataType,     ///< [in] Type and format of data to export
      const void * parameters,     ///< [in] parameters structure, specific to the data type
      unsigned char pbData[],      ///< [out] Buffer where to export the data, optional
      size_t * pcbData             ///< [in/out] Pointer where to store the length of exported data, optional
    ) {
	
      Ptr<const T> ptr(handle);
      if (!ptr)
	{
	 
	return test_output(MINEX_RET_FAILURE_UNSPECIFIED);
	}
      size_t size = *pcbData;
      int rc = ((*ptr).*export_f)(dataType, parameters, pbData, pcbData); // AI: don't user ->* because of bug in ARM compiler
      if (rc < MINEX_RET_SUCCESS && pbData != NULL) {
        memset(pbData, 0, size);
      }
      return rc;
    }

  }
}


using namespace minutia_extractor::minutia_datarec;


#endif // __MINUTIA_DATA_H
