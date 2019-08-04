//
//  postcodes.h
//  postcodes.c
//
//  Created by George MacKerron on 2019/01/03.
//  Copyright © 2019 George MacKerron. All rights reserved.
//

#ifndef postcodes_h
#define postcodes_h

#include <stdbool.h>

typedef enum {
  PostcodeNotFound = 0,
  PostcodeSectorMeanOnly = 1,
  PostcodeOK = 2
} PostcodeStatus;

typedef struct {
  unsigned char area0;
  unsigned char area1;
  unsigned char district0;
  unsigned char district1;
  unsigned char sector;
  unsigned char unit0;
  unsigned char unit1;
  bool valid;
} PostcodeComponents;

typedef struct {
  unsigned int e;
  unsigned int n;
  PostcodeStatus status;
} PostcodeEastingNorthing;

typedef struct {
  PostcodeComponents components;
  PostcodeEastingNorthing en;
  double distance;
} NearbyPostcode;

PostcodeEastingNorthing eastingNorthingFromPostcodeComponents(const PostcodeComponents pcc);
NearbyPostcode nearbyPostcodeFromEastingNorthing(const PostcodeEastingNorthing en);

PostcodeComponents postcodeComponentsFromString(const char s[]);
int stringFromPostcodeComponents(char s[9], const PostcodeComponents pcc);

const char* codePointVersionNumber(void);
const char* codePointCopyrightYear(void);

#endif /* postcodes_h */
