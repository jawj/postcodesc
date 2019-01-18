//
//  postcodeTests.h
//  GridPoint GB
//
//  Created by George MacKerron on 2019/01/17.
//  Copyright © 2019 George MacKerron. All rights reserved.
//

#ifndef postcodeTests_h
#define postcodeTests_h

#include <stdbool.h>
#include <stdlib.h>
#include "postcodes.h"


typedef struct {
  char input[16];  // space for some extra characters and spaces
  bool valid;
  char formatted[9]; // space for e.g. AA1A 1AA\0
  PostcodeEastingNorthing en;
} PostcodeTestItem;

bool postcodeTest(const bool noisily);

#endif /* postcodeTests_h */
