//
//  postcodes.c
//  postcodes.c
//
//  Created by George MacKerron on 2019/01/03.
//  Copyright © 2019 George MacKerron. All rights reserved.
//

#include "postcodes.h"
#include "postcodes.data"

#define LENGTH_OF(x) (sizeof (x) / sizeof *(x))
#define ASPRINTF_OR_DIE(...) if (asprintf(__VA_ARGS__) < 0) exit(EXIT_FAILURE)


// binary searches using poor man's generics

#define DEFINE_INDEXOF(T, TSUFFIX) \
  size_t indexOf ## TSUFFIX(const T needle, const T haystack[], const int haystackLength) { \
    size_t l = 0, r = haystackLength - 1, m; \
    T v; \
    while (l <= r) { \
      m = (l + r) / 2; \
      v = haystack[m]; \
      if (v < needle) l = m + 1; \
      else if (v > needle) r = m - 1; \
      else return m; \
    } \
    return -1; \
  }

#define DEFINE_INDEXOFSTRUCT(THAYSTACK, TNEEDLE, NEEDLEMEMBER) \
  size_t indexOf ## THAYSTACK(const TNEEDLE needle, const THAYSTACK haystack[], const int haystackLength) { \
    size_t l = 0, r = haystackLength - 1, m; \
    TNEEDLE v; \
    while (l <= r) { \
      m = (l + r) / 2; \
      v = haystack[m].NEEDLEMEMBER; \
      if (v < needle) l = m + 1; \
      else if (v > needle) r = m - 1; \
      else return m; \
    } \
    return -1; \
  }

DEFINE_INDEXOF(unsigned char, UnsignedChar)
DEFINE_INDEXOFSTRUCT(OutwardCode, int, codeMapped)
DEFINE_INDEXOFSTRUCT(InwardCode, int, codeMapped)


int applyMappings(const int count, ...) {  // variadic args are (count) times: char c, int mappingLength, unsigned char* mapping
  char c;
  int mappingLength;
  unsigned char* mapping;
  va_list args;
  va_start(args, count);
  int result = 0;
  int runningMaxProduct = 1;
  for (int i = 0; i < count; i ++) {
    c = va_arg(args, int);  // type promotion means we take this as an int
    mappingLength = va_arg(args, int);
    mapping = va_arg(args, unsigned char*);
    size_t value = indexOfUnsignedChar(c, mapping, mappingLength);
    if (value == -1) {
      va_end(args);
      return -1;
    }
    result += value * runningMaxProduct;
    runningMaxProduct *= mappingLength;
  }
  va_end(args);
  return result;
}


PostcodeEastingNorthing eastingNorthingFromPostcodeComponents(const PostcodeComponents pcc) {
  PostcodeEastingNorthing en = (PostcodeEastingNorthing){0};
  int outwardCodeMapped = applyMappings(4,
                                        pcc.district1, LENGTH_OF(district1Mapping), district1Mapping,
                                        pcc.district0, LENGTH_OF(district0Mapping), district0Mapping,
                                        pcc.area1, LENGTH_OF(area1Mapping), area1Mapping,
                                        pcc.area0, LENGTH_OF(area0Mapping), area0Mapping
                                        );
  if (outwardCodeMapped == -1) return en;
  size_t ocIndex = indexOfOutwardCode(outwardCodeMapped, outwardCodes, LENGTH_OF(outwardCodes));
  if (ocIndex == -1) return en;
  OutwardCode oc = outwardCodes[ocIndex];
  
  int inwardCodeMapped = applyMappings(3,
                                       pcc.unit1, LENGTH_OF(unit1Mapping), unit1Mapping,
                                       pcc.unit0, LENGTH_OF(unit0Mapping), unit0Mapping,
                                       pcc.sector, LENGTH_OF(sectorMapping), sectorMapping
                                       );
  if (inwardCodeMapped == -1) return en;
  int inwardCodesCount = (ocIndex < LENGTH_OF(outwardCodes) - 1 ?
                          outwardCodes[ocIndex + 1].inwardCodesOffset :
                          LENGTH_OF(inwardCodes)) - oc.inwardCodesOffset;
  size_t icIndex = indexOfInwardCode(inwardCodeMapped, &inwardCodes[oc.inwardCodesOffset], inwardCodesCount);
  if (icIndex == -1) return en;
  InwardCode ic = inwardCodes[oc.inwardCodesOffset + icIndex];
  
  en.e = oc.originE + ic.offsetE;
  en.n = oc.originN + ic.offsetN;
  en.status = ic.sectorMean ? PostcodeSectorMeanOnly : PostcodeOK;
  return en;  // break here in Xcode 10.1 and check oc.originE in the debugger for a radar to file with Apple
}


PostcodeComponents postcodeComponentsFromString(const char s[]) {
  // note that this validates slightly more loosely than it could
  // -- e.g. it allows A-Z in the last two characters, not just the 20 characters that ever appear there --
  // so that it matches what most people will think is a potentially valid postcode
  
  PostcodeComponents pcc = (PostcodeComponents){0};
  char pc[7] = {0};  // 7 chars is the longest a valid postcode can be, and we don't need a terminal '\0'
  unsigned char lenPc = 0;
  char c;
  size_t i;
  
  // copy s to pc, removing whitespace and transforming to uppercase
  for (i = 0; /* keep going */; i ++) {
    c = s[i];
    if (c == '\0') break;  // break out at end of string
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;  // ignore whitespace
    if (lenPc > 6) return pcc;  // too long (6 is the last position of a 7-digit code)
    pc[lenPc++] = c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c;  // homegrown toupper
  }
  if (lenPc < 5) return pcc;  // too short
  
  // pick apart: inward
  c = pc[--lenPc];
  if (c < 'A' || c > 'Z') return pcc;  // unit1 is (roughly) A-Z
  pcc.unit1 = c;
  c = pc[--lenPc];
  if (c < 'A' || c > 'Z') return pcc;  // unit0 is (roughly) A-Z
  pcc.unit0 = c;
  c = pc[--lenPc];
  if (c < '0' || c > '9') return pcc;  // sector is 0-9
  pcc.sector = c;
  
  // pick apart: outward
  i = 0;
  c = pc[i++];
  if (c < 'A' || c > 'Z') return pcc;  // area0 is (roughly) A-Z
  pcc.area0 = c;
  c = pc[i++];
  if (c >= 'A' && c <= 'Z') {  // if this is A-Z, it must be area1
    pcc.area1 = c;
    if (lenPc - i < 1) return pcc;  // need at least 1 char left for the district
    c = pc[i++];  // consume a new character for district0
  }
  if (c < '0' || c > '9') return pcc;  // district0 is 0-9
  pcc.district0 = c;
  if (lenPc - i >= 1) {  // if we have a character left, it must be district1
    c = pc[i++];
    if ((c < '0' || c > '9') && (c < 'A' || c > 'Z')) return pcc;  // district1 is (roughly) 0-9, A-Z
    pcc.district1 = c;
  }
  if (lenPc - i > 0) return pcc;  // extraneous character alert!
  
  // ... and that, ladies and gentlemen, is why we have regular expressions
  
  pcc.valid = true;
  return pcc;
}


char* stringFromPostcodeComponents(const PostcodeComponents pcc) {
  char *s;
  char area1str[] = { pcc.area1, '\0' };  // if area1 is null, this is a zero-length string, as desired
  char district1str[] = { pcc.district1, '\0' };  // ditto for district1
  ASPRINTF_OR_DIE(&s, "%c%s%c%s %c%c%c", pcc.area0, area1str, pcc.district0, district1str, pcc.sector, pcc.unit0, pcc.unit1);
  return s;
}
