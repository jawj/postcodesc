
//
//  main.c
//  postcodes.c
//
//  Created by George MacKerron on 19/01/2019.
//  Copyright (c) 2019 George MacKerron. All rights reserved.
//

#include <stdbool.h>
#include <math.h>
#include "postcodes.h"
#include "postcodeTests.h"

int main(int argc, const char *argv[]) {

  if (argc == 2 && strcmp(argv[1], "test") == 0) {
    // with arg 'test', run tests
    bool passed = postcodeTest(true);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;

  } else if (argc == 2) {
    // with any other single argument, treat as a postcode
    PostcodeComponents pcc = postcodeComponentsFromString(argv[1]);
    
    if (! pcc.valid) {
      puts("Not a postcode");
      return EXIT_FAILURE;
    }

    char *formatted = stringFromPostcodeComponents(pcc);
    puts(formatted);
    free(formatted);

    PostcodeEastingNorthing en = eastingNorthingFromPostcodeComponents(pcc);

    if (en.status == PostcodeNotFound) {
      puts("Postcode not found");
      return EXIT_FAILURE;
    }

    printf("E %i  N %i%s\n", en.e, en.n, en.status == PostcodeSectorMeanOnly ? "  (sector mean)" : "");

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

    char *pc = stringFromPostcodeComponents(np.components);
    printf("%s (%im from centroid)\n", pc, (int)round(np.distance));
    free(pc);

  } else {
    // with any other arguments, show help text
    puts("postcodes.c -- https://github.com/jawj/postcodes.c -- Built " __DATE__ " " __TIME__ "\n"
         "\n"
         "Parse, format and look up GB postcodes <-> grid references.\n"
         "\n"
         "Usage:\n"
         "  postcodesc test  - run tests \n"
         "  postcodesc POSTCODE  - look up location from postcode (note: quote postcode or omit spaces)\n"
         "  postcodesc EASTING NORTHING  - look up postcode from location\n"
         "\n"
         "Derived from Ordnance Survey CodePoint Open data\n"
         "Contains OS data (C) Crown copyright and database right 2018\n"
         "Contains Royal Mail data (C) Royal Mail copyright and database right 2018\n"
         "Contains National Statistics data (C) Crown copyright and database right 2018\n");
  }

  return EXIT_SUCCESS;
}