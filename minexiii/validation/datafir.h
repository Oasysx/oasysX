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
#ifndef __datafir_h
#define __datafir_h


namespace minutia_extractor {
  namespace minutia_datarec {
    using namespace merged;
    namespace minutia_data {

      struct uint48 {
        uint32  lsb;
        uint16  msb;
        operator uint64 () const { return lsb | (uint64(msb) << 32) ; }
      };

      inline read_type & operator >> (read_type & rd, uint48 & x) {
        if (rd.dataInBigEndian) {
          rd >> x.msb >> x.lsb;
        } else {
          rd >> x.lsb >> x.msb;
        }
        return rd;
      }

      inline uint16 convert_pp(uint8 scaleUnits, uint16 resolution) {
        switch (scaleUnits) {
          case 1: return resolution;
          case 2: return resolution * 254L / 100;
        }
        return uint16(-1);
      }
/*
      Format identifier                     4 bytes         0x46495200 ('F' 'I' 'R' 0x0) "FIR" � Finger Image Record
      Version number                        4 bytes         0x30313000 ('0' '1' '0' 0x0) "010"
      Record length                         6 bytes         36+ Number Views * (14 bytes + Data length ) Includes all finger views
      CBEFF Product Identifier              4 bytes         CBEFF PID (registry managed by IBIA)
      Capture device ID                     2 byte          Vendor specified
      Image acquisition level               2 bytes         See Table 1 Combination of parameters
      Number of fingers/palms               1 byte          >=1
      Scale units                           1 byte          1-2 cm or inch
      Scan resolution (horiz)               2 bytes         See Table 1 Up to 1000 ppi
      Scan resolution (vert)                2 bytes         See Table 1 Up to 1000 ppi
      Image resolution (horiz)              2 bytes         <= Scan Resolution (horiz) Quality level dependent
      Image resolution (vert)               2 bytes         <= Scan Resolution (vert) Quality level dependent
      Pixel depth                           1 byte          1 -16 bits 2 � 65536 gray levels
      Image compression algorithm           1 byte          See Table 3 Uncompressed or algorithm used
      Reserved                              2 bytes         Bytes set to '0x0'


      Format identifier                     1 - 4           46 49 52 00   "FIR" � Finger Image Record
      Version number                        5 - 8           30 31 30 00   "010"
      Record length                         9 -14           03 93 b9      One finger view 36+1*(14+234,375)
      CBEFF Product Identifier              15-18           XX XX XX XX   CBEFF PID (registry managed by IBIA)
      Scanner ID                            19-20           XX XX         Vendor specified
      Image acquisition level               2 bytes         See Table 1   Combination of parameters
      Number of fingers/palms               23              01
      Scale units                           24              01            Pixels/inch
      Scan resolution (horiz)               25-26           01 F4         500 pixels/inch
      Scan resolution (vert)                27-28           01 F4         500 pixels/inch
      Image resolution (horiz)              29-30           01 F4         500 pixels/inch
      Image resolution (vert)               31-32           01 F4         500 pixels/inch
      Pixel depth                           33              08            256 gray levels
      Image compression Algorithm           34              00            Uncompressed (no bit packing)
      Reserved                              35-36           00 00
*/

      enum ScaleUnits {
        FT_SU_DPI=1,
        FT_SU_DPCM=2,
      };

      enum ImageCompression {
        FT_IC_UNCOMPRESSED_NOPACK=0,
        FT_IC_UNCOMPRESSED_PACKED=1,
        FT_IC_COMPRESSED_WSQ=2,
        FT_IC_COMPRESSED_JPEG=3,
        FT_IC_COMPRESSED_JPEG2000=4,
        FT_IC_COMPRESSED_PNG=5,
      };

      template <typename _Tp, size_t _Sz = 36>
      struct _headfir {
        static const size_t size = _Sz;
        const static uint16 CBEFF_OWNER_UPEK      = 0x0012;
        const static uint16 CBEFF_OWNER_AUTHENTEC = 0x0042;
        const static uint16 CBEFF_UPEK_AREA = 0x0300;
        const static uint16 CBEFF_UPEK_SWIPE = 0x0100;
        const static uint16 CBEFF_AUTHENTEC_2810 = 0x0100;

        char szFormatIdFIR[4];      // "FIR" � Finger Image Record
        char szVersionNumber[4];    // "010"
        uint48 nRecordLength;       // Record length 6 bytes
        uint32 nCBEFFProductID;     // (registry managed by IBIA)
        uint16 nDeviceID;
        uint16 nImageAcquisitionLevel;
        uint8 nImageCount;
        // uint8 nScaleUnits;       // 1: Pixels/inch, 2: Pixels/cm
        uint16 nScanResolutionX;    // Always in PPI (same as DPI)
        uint16 nScanResolutionY;    // Always in PPI (same as DPI)
        uint16 nImageResolutionX;   // Always in PPI (same as DPI)
        uint16 nImageResolutionY;   // Always in PPI (same as DPI)
        uint8 nPixelDepth;
        uint8 nImageCompression;
        uint16 nReserved;

        int Init(read_type & rd) {
          size_t Size = rd.end - rd.start;

          uint8 nScaleUnits;
          memcpy(szFormatIdFIR,   rd.cur, 4);  rd.cur += 4;
          memcpy(szVersionNumber, rd.cur, 4);  rd.cur += 4;

          //nRecordLength = GetInt48(&rd.cur);
          rd >> nRecordLength;
          nCBEFFProductID = ReadCBEFFProductID(rd);
          rd >> nDeviceID;
          rd >> nImageAcquisitionLevel;
          rd >> nImageCount;
          rd >> nScaleUnits;
          switch (nScaleUnits) {
            case 1: case 2: break;
            default: return MINEX_RET_FAILURE_UNSPECIFIED;
          }
          uint16 nTmp;
          rd >> nTmp; nScanResolutionX  = convert_pp(nScaleUnits, nTmp);
          rd >> nTmp; nScanResolutionY  = convert_pp(nScaleUnits, nTmp);
          rd >> nTmp; nImageResolutionX = convert_pp(nScaleUnits, nTmp);
          rd >> nTmp; nImageResolutionY = convert_pp(nScaleUnits, nTmp);
          rd >> nPixelDepth;
          rd >> nImageCompression;
          rd >> nReserved;

          if (nRecordLength > Size)           return MINEX_RET_FAILURE_UNSPECIFIED;

          return MINEX_RET_SUCCESS;
        }

        static uint16 CBEFF_OWNER(uint32 pid) { return uint16(pid >> 16); }
        static uint16 CBEFF_PID(uint32 pid) { return uint16(pid & 0xffff); }
        static uint16 CAPTURE_DEVICE_ID(uint16 did) { return did & 0xfff; }

        uint32 ReadCBEFFProductID(read_type & rd) {
          _Tp* pT = static_cast<_Tp*>(this);
          return pT->ReadCBEFFProductID(rd); // otherwise - recursive call
        }

        int GetDeviceCode(uint8 & devCode) const {
          _Tp* pT = static_cast<_Tp*>(this);
          return pT->GetDeviceCode(devCode); // otherwise - recursive call
        }

        uint16 get_ppi() const {
          return nImageResolutionX;
        }

      };

/*
      Length of finger data block (bytes)   4 bytes         Includes header, and largest image data block
      Finger/palm position                  1 byte          0-15; 20-36 See Tables 5-6
      Count of views                        1 byte          1-256
      View number                           1 byte          1-256
      Finger/palm image quality             1 byte          254 Undefined
      Impression type                       1 byte          Table 7
      Horizontal line length                2 bytes         Number of pixels per horizontal line
      Vertical line length                  2 bytes         Number of horizontal lines
      Reserved                              1 byte          __________ Byte set to �0x0�
      Finger/palm image data                < 43x10^8 bytes __________ Compressed or uncompressed image data


      Length of finger data block (bytes)   1-4             00 03 93 95 Includes header, and largest image data block
      Finger/palm position                  5               07 Left index finger
      Count of views                        6               01
      View number                           7               01
      Finger/palm image quality             7               FE Undefined - 254
      Impression type                       9               00 Live-scan plain
      Horizontal line length                10-11           01 77 375 pixels per horizontal line
      Vertical line length                  12-13           02 71 625 horizontal lines
      Reserved                              14 00           Byte set to �0x0�
      Finger/palm image data 1              15- 234,389     __________ Uncompressed image data
*/
      template <size_t _Sz = 14>
      struct headerfir {
        static const size_t size = _Sz;

        uint32 nDataLength;
        uint8 nFingerPosition;
        uint8 nViewsCount;
        uint8 nViewsNumber;
        uint8 nImageQuality;
        uint8 nImpressionType;
        uint16 nHorizontalLineLength;
        uint16 nVerticalLineLength;
        uint8 nReserved;

        int Init(read_type & rd) {
          size_t Size = rd.end - rd.start;

          rd >> nDataLength;
          rd >> nFingerPosition;
          rd >> nViewsCount;
          rd >> nViewsNumber;
          rd >> nImageQuality;
          rd >> nImpressionType;
          rd >> nHorizontalLineLength;
          rd >> nVerticalLineLength;
          rd >> nReserved;

          if (nDataLength > Size)             return MINEX_RET_FAILURE_UNSPECIFIED;

          return MINEX_RET_SUCCESS;
        }

        uint16 get_wid() const {
          return nHorizontalLineLength;
        }

        uint16 get_heig() const {
          return nVerticalLineLength;
        }

      };

      template <class FIR_RH, class FIR_IH>
      struct FIR_bytes_header {
        FIR_RH RecordHeader;
        FIR_IH ImageHeader;

        int image_header_iniatialize(const uint8 * buffer, size_t Size) {
          if (Size < RecordHeader.size + ImageHeader.size) return MINEX_RET_FAILURE_UNSPECIFIED;

          read_type rd(buffer, Size, true);
          int rc = RecordHeader.Init(rd);
          if (rc < MINEX_RET_SUCCESS) return rc;

          rc = ImageHeader.Init(rd);
          return rc;
        }

        int Validate() const {
          int nHorizontalLineLengthInch, nVerticalLineLengthInch;

          if (memcmp(RecordHeader.szFormatIdFIR  ,"FIR", 4))    return MINEX_RET_FAILURE_UNSPECIFIED;
       
          if (RecordHeader.nImageCount != 1)                    return MINEX_RET_FAILURE_UNSPECIFIED;

          if (RecordHeader.nScanResolutionX < 300)              return MINEX_RET_FAILURE_UNSPECIFIED;
          if (RecordHeader.nScanResolutionX > 1000)             return MINEX_RET_FAILURE_UNSPECIFIED;
          if (RecordHeader.nScanResolutionY < 300)              return MINEX_RET_FAILURE_UNSPECIFIED;
          if (RecordHeader.nScanResolutionY > 1000)             return MINEX_RET_FAILURE_UNSPECIFIED;
     
          if (RecordHeader.nImageResolutionX < 300)             return MINEX_RET_FAILURE_UNSPECIFIED;
          if (RecordHeader.nImageResolutionX > 1000)            return MINEX_RET_FAILURE_UNSPECIFIED;
          if (RecordHeader.nImageResolutionX 
           != RecordHeader.nImageResolutionY)                   return MINEX_RET_FAILURE_UNSPECIFIED;

          if (RecordHeader.nPixelDepth != 8)                    return MINEX_RET_FAILURE_UNSPECIFIED;
          if (RecordHeader.nImageCompression !=
              FT_IC_UNCOMPRESSED_NOPACK)                        return MINEX_RET_FAILURE_UNSPECIFIED;

          if (ImageHeader.nFingerPosition > 10)                 return MINEX_RET_FAILURE_UNSPECIFIED;

          if (ImageHeader.nViewsCount != 1)                     return MINEX_RET_FAILURE_UNSPECIFIED;
          if (ImageHeader.nViewsNumber != 1)                    return MINEX_RET_FAILURE_UNSPECIFIED;
      
          nHorizontalLineLengthInch = ImageHeader.nHorizontalLineLength * 1000L / RecordHeader.nImageResolutionX;
          nVerticalLineLengthInch   = ImageHeader.nVerticalLineLength   * 1000L / RecordHeader.nImageResolutionY;
          if (nHorizontalLineLengthInch > 1600)                 return MINEX_RET_FAILURE_UNSPECIFIED;
          if (nVerticalLineLengthInch   > 1500)                 return MINEX_RET_FAILURE_UNSPECIFIED;

       
          if (ImageHeader.nDataLength <
            ImageHeader.nHorizontalLineLength *
            ImageHeader.nVerticalLineLength +
            ImageHeader.size)                                   return MINEX_RET_FAILURE_UNSPECIFIED;

          if (RecordHeader.nRecordLength <
            ImageHeader.nDataLength +
            RecordHeader.size)                                  return MINEX_RET_FAILURE_UNSPECIFIED;

          return MINEX_RET_SUCCESS;
        }

        uint16 get_wid() const {
          return ImageHeader.get_wid();
        }

        uint16 get_heig() const {
          return ImageHeader.get_heig();
        }

        uint16 get_ppi() const {
          return RecordHeader.get_ppi();
        }

        template <class T> 
        T * GetPixels(T * buffer) const {
          return buffer + RecordHeader.size + ImageHeader.size;
        }

        int GetDeviceCode(uint8 & devCode) const {
          return RecordHeader.GetDeviceCode(devCode);
        }

      };

      template <class FIR_H, class T = uint8>
      struct FIR_Record { //: public HResult
        FIR_H Header;
        const T * pData;
      private:
        int rc;
      public:
        STATIC_ASSERT(sizeof(T) == 1);

        operator int() const { return rc; }
        bool IsBad() const { return rc < MINEX_RET_SUCCESS; }

        FIR_Record(const T * buffer, size_t size) {
          rc = imagerec_ini(buffer, size);
        }

        int imagerec_ini(const T * buffer, size_t size) {
          int _rc = Header.image_header_iniatialize(buffer, size);
          if (_rc < MINEX_RET_SUCCESS) return _rc;

          pData = Header.GetPixels(buffer);
          return MINEX_RET_SUCCESS;
        }

        bool IsValid() {
          if (IsBad()) return false;
          rc = Header.Validate();
          return !IsBad();
        }

        uint16 get_wid() const {
          return Header.get_wid();
        }

        uint16 get_heig() const {
          return Header.get_heig();
        }

        int GetDeviceCode(uint8 & devCode) const {
          return Header.GetDeviceCode(devCode);
        }

        uint16 get_ppi() const {
          return Header.get_ppi();
        }

      };



      struct image_header_ansi : public _headfir<image_header_ansi, 36> {
        uint32 ReadCBEFFProductID(read_type & rd) {
          uint32 nProductID;
          rd >> nProductID;
          return nProductID;
        }

        int GetDeviceCode(uint8 & devCode) const {
        
          if ((nCBEFFProductID & 0xffffff00) == 0x0033fe00) {
            devCode = nCBEFFProductID & 0xff; 
            return MINEX_RET_SUCCESS;
          }
          switch (CBEFF_OWNER(nCBEFFProductID)) {
            case CBEFF_OWNER_UPEK:
              switch (CBEFF_PID(nCBEFFProductID)) {
                case CBEFF_UPEK_AREA:           devCode = 0xA5; break;
                case CBEFF_UPEK_SWIPE:          devCode = 0xAF; break;
                default:                        devCode = 0;    break;     // Unknown UPEK fingerprint reader
                case 0:
                  switch (CAPTURE_DEVICE_ID(nDeviceID)) {
                    case CBEFF_UPEK_AREA:       devCode = 0xA5; break;
                    case CBEFF_UPEK_SWIPE:      devCode = 0xAF; break;
                    default:                    devCode = 0;    break;     // Unknown UPEK fingerprint reader
                  }
                  break;
                }
                break;
            case CBEFF_OWNER_AUTHENTEC:
              switch (CBEFF_PID(nCBEFFProductID)) {
                default:                        devCode = 0xBF; break;     // Unknown Authentec fingerprint reader, assume AES2810
                case 0:
                  switch (CAPTURE_DEVICE_ID(nDeviceID)) {
                    case 0:
                    default:                    devCode = 0xBF; break;     // Unknown Authentec fingerprint reader, assume AES2810
                  }
                  break;
              }
              break;
            case 0:                             return MINEX_RET_FAILURE_UNSPECIFIED; // 0 is not allowed by standard
            default:                            devCode = 0;    break;     // Unknown vendor fingerprint reader
          }
          return MINEX_RET_SUCCESS;
          
        }
      };

      typedef headerfir<14> ANSIImageHeader;

      typedef FIR_bytes_header<image_header_ansi, ANSIImageHeader> ANSIImageRecordHeader;

      typedef FIR_Record<ANSIImageRecordHeader> ANSIImageRecord;


      struct image_header_iso : public _headfir<image_header_iso, 32> {
        uint32 ReadCBEFFProductID(read_type & rd) {
          return 0;
        }

        int GetDeviceCode(uint8 & devCode) const {
          devCode = 0;
          return MINEX_RET_SUCCESS;
        }
      };

      typedef headerfir<14> ISOImageHeader;

      typedef FIR_bytes_header<image_header_iso, ISOImageHeader> ISOImageRecordHeader;

      typedef FIR_Record<ISOImageRecordHeader> ISOImageRecord;


      struct detail_rawimage : public input_data_ext {

        int image_header_iniatialize(const uint8 * buffer, size_t size) {
          if (size < sizeof(input_data_ext)) return MINEX_RET_FAILURE_UNSPECIFIED;
          memcpy(static_cast<input_data_ext *>(this), buffer, sizeof(input_data_ext));
          return MINEX_RET_SUCCESS;
        }

        int Validate() const {
          if (width  < 150)  return MINEX_RET_FAILURE_UNSPECIFIED;
          if (height < 166)  return MINEX_RET_FAILURE_UNSPECIFIED;
          if (width  > 812)  return MINEX_RET_FAILURE_UNSPECIFIED;
          if (height > 1000) return MINEX_RET_FAILURE_UNSPECIFIED;
          if (dpi < 362)     return MINEX_RET_FAILURE_UNSPECIFIED;
          if (dpi > 1008)    return MINEX_RET_FAILURE_UNSPECIFIED;
          return MINEX_RET_SUCCESS;
        }

        uint16 get_wid() const {
          return width;
        }

        uint16 get_heig() const {
          return height;
        }

        uint16 get_ppi() const {
          return dpi;
        }

        template <class T> 
        T * GetPixels(T * buffer) const {
          return pixels;
        }

        int GetDeviceCode(uint8 & devCode) const {
          return 0xff;
        }
      };

      typedef FIR_Record<detail_rawimage> RawImageRecord;
    }
  }
}

#endif // __datafir_h
