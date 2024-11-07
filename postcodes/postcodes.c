//
//  postcodes.c
//  postcodes.c
//
//  Created by George MacKerron on 2019/01/03.
//  Copyright © 2019 George MacKerron. All rights reserved.
//

#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "postcodes.h"
#include "postcodes.data"

#define LENGTH_OF(x) (sizeof (x) / sizeof *(x))

// binary searches using poor man's generics

#define DEFINE_INDEXOF(T, TSUFFIX) \
  int indexOf ## TSUFFIX(const T needle, const T haystack[], const int haystackLength) { \
    int l = 0, r = haystackLength - 1, m; \
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
  int indexOf ## THAYSTACK(const TNEEDLE needle, const THAYSTACK haystack[], const int haystackLength) { \
    int l = 0, r = haystackLength - 1, m; \
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


// forward lookup (postcode -> location)

int intByMappingChars(const int count, ...) {  // variadic args are (count) times: char c, int mappingLength, unsigned char* mapping
  va_list args;
  va_start(args, count);
  int result = 0;
  int runningMaxProduct = 1;
  for (int i = 0; i < count; i ++) {
    char c = va_arg(args, int);  // type promotion means we take this as an int
    int mappingLength = va_arg(args, int);
    unsigned char *mapping = va_arg(args, unsigned char *);
    int value = indexOfUnsignedChar(c, mapping, mappingLength);
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

bool outwardCodeFromPostcodeComponents(OutwardCode *oc, const PostcodeComponents pcc) {
  int outwardCodeMapped = intByMappingChars(4,
                                            pcc.district1, LENGTH_OF(district1Mapping), district1Mapping,
                                            pcc.district0, LENGTH_OF(district0Mapping), district0Mapping,
                                            pcc.area1, LENGTH_OF(area1Mapping), area1Mapping,
                                            pcc.area0, LENGTH_OF(area0Mapping), area0Mapping);
  if (outwardCodeMapped == -1) return false;
  int ocIndex = indexOfOutwardCode(outwardCodeMapped, outwardCodes, LENGTH_OF(outwardCodes));
  if (ocIndex == -1) return false;
  *oc = outwardCodes[ocIndex];
  return true;
}

PostcodeEastingNorthing eastingNorthingFromPostcodeComponents(const PostcodeComponents pcc) {
  PostcodeEastingNorthing en = (PostcodeEastingNorthing){0};
  int outwardCodeMapped = intByMappingChars(4,
                                            pcc.district1, LENGTH_OF(district1Mapping), district1Mapping,
                                            pcc.district0, LENGTH_OF(district0Mapping), district0Mapping,
                                            pcc.area1, LENGTH_OF(area1Mapping), area1Mapping,
                                            pcc.area0, LENGTH_OF(area0Mapping), area0Mapping);
  if (outwardCodeMapped == -1) return en;
  int ocIndex = indexOfOutwardCode(outwardCodeMapped, outwardCodes, LENGTH_OF(outwardCodes));
  if (ocIndex == -1) return en;
  OutwardCode oc = outwardCodes[ocIndex];
  
  int inwardCodeMapped = intByMappingChars(3,
                                           pcc.unit1, LENGTH_OF(unit1Mapping), unit1Mapping,
                                           pcc.unit0, LENGTH_OF(unit0Mapping), unit0Mapping,
                                           pcc.sector, LENGTH_OF(sectorMapping), sectorMapping);
  if (inwardCodeMapped == -1) return en;
  int inwardCodesCount = (ocIndex < LENGTH_OF(outwardCodes) - 1 ?
                          outwardCodes[ocIndex + 1].inwardCodesOffset :
                          LENGTH_OF(inwardCodes)) - oc.inwardCodesOffset;
  int icIndex = indexOfInwardCode(inwardCodeMapped, &inwardCodes[oc.inwardCodesOffset], inwardCodesCount);
  if (icIndex == -1) return en;
  InwardCode ic = inwardCodes[oc.inwardCodesOffset + icIndex];
  
  en.e = oc.originE + ic.offsetE;
  en.n = oc.originN + ic.offsetN;
  en.status = ic.sectorMean ? PostcodeSectorMeanOnly : PostcodeOK;

  return en;  // break here in Xcode 10.1 and check oc.originE in the debugger for a radar to file with Apple
}


// reverse lookup

void charsByUnmappingInt(int mapped, int count, ...) {  // variadic args are (count) times: char* c, int mappingLength, unsigned char* mapping
  va_list args;
  unsigned char *mapping = NULL;
  unsigned char *c = NULL;
  for (int i = 0; i < count; i ++) {
    va_start(args, count);
    int maxProduct = 1;
    for (int j = count - 1 - i; j >= 0; j --) {
      c = va_arg(args, unsigned char *);
      int mappingLength = va_arg(args, int);
      mapping = va_arg(args, unsigned char *);
      if (j > 0) maxProduct *= mappingLength;
    }
    int index = mapped / maxProduct;
    mapped %= maxProduct;
    if (mapping) *c = mapping[index];  // there's no way mapping is still NULL here, but the conditional gives clang some reassurance
    va_end(args);
  }
}

NearbyPostcode nearbyPostcodeFromEastingNorthing(const PostcodeEastingNorthing en) {
  NearbyPostcode np = {0};

  long minDSq = LONG_MAX;
  int minDSqOutwardIndex = -1;
  int minDSqInwardIndex = -1;

  for (int ocIndex = 0, ocLen = LENGTH_OF(outwardCodes); ocIndex < ocLen; ocIndex ++) {
    OutwardCode oc = outwardCodes[ocIndex];
    if (en.e < oc.originE ||
        en.n < oc.originN ||
        en.e > oc.originE + oc.maxOffsetE ||
        en.n > oc.originN + oc.maxOffsetN) continue;

    int offsetE = en.e - oc.originE;
    int offsetN = en.n - oc.originN;

    int nextInwardCodesOffset = ocIndex < ocLen - 1 ? 
      outwardCodes[ocIndex + 1].inwardCodesOffset : 
      LENGTH_OF(inwardCodes);

    /*
    PostcodeComponents pcc = {0};
    charsByUnmappingInt(oc.codeMapped, 4,
                        &pcc.district1, LENGTH_OF(district1Mapping), district1Mapping,
                        &pcc.district0, LENGTH_OF(district0Mapping), district0Mapping,
                        &pcc.area1, LENGTH_OF(area1Mapping), area1Mapping,
                        &pcc.area0, LENGTH_OF(area0Mapping), area0Mapping);
    char *outw = stringFromPostcodeComponents(pcc);
    printf("Evaluating %i codes in %s\n", nextInwardCodesOffset - oc.inwardCodesOffset, outw);
    free(outw);
    */

    for (int icIndex = oc.inwardCodesOffset; icIndex < nextInwardCodesOffset; icIndex ++) {
      InwardCode ic = inwardCodes[icIndex];
      int deltaE = offsetE - ic.offsetE;
      int deltaN = offsetN - ic.offsetN;
      long dSq = deltaE * deltaE + deltaN * deltaN;
      if (dSq < minDSq) {
        minDSq = dSq;
        minDSqOutwardIndex = ocIndex;
        minDSqInwardIndex = icIndex;
      }
    }
  }
  
  if (minDSqOutwardIndex == -1) return np;

  OutwardCode oc = outwardCodes[minDSqOutwardIndex];
  InwardCode ic = inwardCodes[minDSqInwardIndex];
  
  PostcodeComponents *pcc = &np.components;
  charsByUnmappingInt(oc.codeMapped, 4,
                      &pcc->district1, LENGTH_OF(district1Mapping), district1Mapping,
                      &pcc->district0, LENGTH_OF(district0Mapping), district0Mapping,
                      &pcc->area1, LENGTH_OF(area1Mapping), area1Mapping,
                      &pcc->area0, LENGTH_OF(area0Mapping), area0Mapping);
  charsByUnmappingInt(ic.codeMapped, 3,
                      &pcc->unit1, LENGTH_OF(unit1Mapping), unit1Mapping,
                      &pcc->unit0, LENGTH_OF(unit0Mapping), unit0Mapping,
                      &pcc->sector, LENGTH_OF(sectorMapping), sectorMapping);
  pcc->valid = true;

  np.distance = sqrt((double)minDSq);
  np.en = (PostcodeEastingNorthing){
      oc.originE + ic.offsetE,
      oc.originN + ic.offsetN,
      ic.sectorMean ? PostcodeSectorMeanOnly : PostcodeOK};

  return np;
}

// parsing and formatting

PostcodeComponents postcodeComponentsFromString(const char s[], bool outwardOnly) {
  // note that this validates slightly more loosely than it could
  // -- e.g. it allows A-Z in the last two characters, not just the 20 characters that ever appear there --
  // so that it matches what most people will think is a potentially valid postcode
  
  int minLength = outwardOnly ? 2 : 5;
  int maxLength = minLength + 2;
  
  PostcodeComponents pcc = (PostcodeComponents){0};
  char pc[maxLength];  // we don't need a terminal '\0'
  unsigned char lenPc = 0;
  char c;
  int i;
  
  // copy s to pc, removing whitespace and transforming to uppercase
  for (i = 0; /* keep going */; i ++) {
    c = s[i];
    if (c == '\0') break;  // break out at end of string
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;  // ignore whitespace
    if (lenPc > maxLength - 1) return pcc;  // too long (n - 1 is the last position of an n-digit code)
    pc[lenPc++] = c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c;  // homegrown toupper
  }
  if (lenPc < minLength) return pcc;  // too short
  
  if (! outwardOnly) {
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
  }
  
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

int stringFromPostcodeComponents(char s[9], const PostcodeComponents pcc) {
  char area1str[] = { pcc.area1, '\0' };  // if area1 is null, this is a zero-length string, as desired
  char district1str[] = { pcc.district1, '\0' };  // ditto for district1
  return sprintf(s, "%c%s%c%s%c%c%c%c",
                 pcc.area0, area1str, pcc.district0, district1str,
                 pcc.sector == '\0' ? '\0' : ' ',
                 pcc.sector, pcc.unit0, pcc.unit1);
}

const char* codePointVersionNumber(void) {
  return dataSetVersionNumber;
}

const char* codePointCopyrightYear(void) {
  return dataSetCopyrightYear;
}
