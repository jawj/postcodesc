
//
//  main.c
//  postcodes.c
//
//  Created by George MacKerron on 19/01/2019.
//  Copyright (c) 2019 George MacKerron. All rights reserved.
//

#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "postcodes.h"
#include "postcodeTests.h"

int main(int argc, const char *argv[]) {
  char pc[9];

  if (argc == 2 && strcmp(argv[1], "test") == 0) {
    // with arg 'test', run tests
    bool passed = postcodeTest(true);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;

  } else if (argc == 2) {
    // with any other single argument, treat as a full or outward postcode
    PostcodeComponents pcc = {0};

    // try outward
    pcc = postcodeComponentsFromString(argv[1], true);

    if (pcc.valid) {
      stringFromPostcodeComponents(pc, pcc);
      puts(pc);

      OutwardCode oc;
      bool found = outwardCodeFromPostcodeComponents(&oc, pcc);

      if (!found) {
        puts("Outward postcode not found");
        return EXIT_FAILURE;
      }

      printf("E %i - %i  N %i - %i\n", 
             oc.originE, oc.originE + oc.maxOffsetE, 
             oc.originN, oc.originN + oc.maxOffsetN);
      return EXIT_SUCCESS;
    }

    // try full
    pcc = postcodeComponentsFromString(argv[1], false);
    
    if (! pcc.valid) {
      puts("Not a valid postcode (neither outward nor full)");
      return EXIT_FAILURE;
    }

    stringFromPostcodeComponents(pc, pcc);
    puts(pc);

    PostcodeEastingNorthing en = eastingNorthingFromPostcodeComponents(pcc);

    if (en.status == PostcodeNotFound) {
      puts("Full postcode not found");
      return EXIT_FAILURE;
    }

    printf("E %i  N %i%s\n", en.e, en.n, en.status == PostcodeSectorMeanOnly ? "  (sector mean)" : "");
    return EXIT_SUCCESS;

  } else if (argc == 3) {
    // with two arguments, treat as a reverse lookup from E/N
    char *dummy;
    long e = strtol(argv[1], &dummy, 10);
    long n = strtol(argv[2], &dummy, 10);

    PostcodeEastingNorthing en = (PostcodeEastingNorthing){e, n};
    NearbyPostcode np = nearbyPostcodeFromEastingNorthing(en);

    if (! np.components.valid) {
      puts("No postcode near that location");
      return EXIT_FAILURE;
    }

    stringFromPostcodeComponents(pc, np.components);
    printf("%s (%im from centroid)\n", pc, (int)round(np.distance));
    return EXIT_SUCCESS;

  } else {
    // with any other arguments, show help text
    puts("postcodes.c -- https://github.com/jawj/postcodes.c -- Built " __DATE__ " " __TIME__ "\n"
         "\n"
         "Parse, format and look up GB postcodes <-> grid references.\n"
         "\n"
         "Usage:\n"
         "  postcodesc test  - run tests \n"
         "  postcodesc POSTCODE  - look up location from full/outward postcode (note: use quotes or omit spaces)\n"
         "  postcodesc EASTING NORTHING  - look up postcode from location\n"
         "\n"
         "Derived from Ordnance Survey CodePoint Open data\n"
         "Contains OS data (C) Crown copyright and database right 2019\n"
         "Contains Royal Mail data (C) Royal Mail copyright and database right 2019\n"
         "Contains National Statistics data (C) Crown copyright and database right 2019\n");

    return EXIT_FAILURE;
  }
}