//
//  postcodeTests.c
//  postcodes.c
//
//  Created by George MacKerron on 2019/01/17.
//  Copyright © 2019 George MacKerron. All rights reserved.
//

#include "postcodeTests.h"

#define LENGTH_OF(x) (sizeof (x) / sizeof *(x))
#define ASPRINTF_OR_DIE(...) if (asprintf(__VA_ARGS__) < 0) exit(EXIT_FAILURE)

static const PostcodeTestItem postcodeTestItems[] = {
  // note -- format tests are unlikely to change but status tests could be broken
  // by future data updates if postcodes are added/moved/removed

  // test some valid postcodes, in all valid formats
  {"e 10aa", true, "E1 0AA", {535267, 181084, PostcodeOK}},         // A0 0AA
  {" bn15pq\n", true, "BN1 5PQ", {530475, 105697, PostcodeOK}},     // AA00AA
  {"B10\t9NN", true, "B10 9NN", {410441, 285551, PostcodeOK}},      // A00 0AA
  {"Sy 21 0Hd   ", true, "SY21 0HD", {306655, 307234, PostcodeOK}}, // AA00 0AA
  {"\tw1a 5w w", true, "W1A 5WW", {531073, 182317, PostcodeOK}},    // A0A 0AA
  {"  ec1v7jJ", true, "EC1V 7JJ", {531760, 182831, PostcodeOK}},    // AA0A0AA

  // test some firsts and lasts (looking for off-by-one errors)
  {"AB101AB", true, "AB10 1AB", {394235, 806529, PostcodeOK}},
  {"AB101AF", true, "AB10 1AF", {394181, 806429, PostcodeOK}},
  {"AB998AB", true, "AB99 8AB", {394406, 802269, PostcodeOK}},
  {"AB998AF", true, "AB99 8AF", {394406, 802269, PostcodeOK}},
  {"ZE1 0AA", true, "ZE1 0AA", {447759, 1141280, PostcodeOK}},
  {"ZE1 0AB", true, "ZE1 0AB", {447830, 1141252, PostcodeOK}},
  {"ZE3 9JY", true, "ZE3 9JY", {438498, 1112029, PostcodeOK}},
  {"ZE3 9JZ", true, "ZE3 9JZ", {438662, 1112122, PostcodeOK}},

  // test some sector means
  {"BN99 9AA", true, "BN99 9AA", {517706, 104201, PostcodeSectorMeanOnly}},
  {"M29 8SQ", true, "M29 8SQ", {369594, 402199, PostcodeSectorMeanOnly}},
  {"RH121BW", true, "RH12 1BW", {516485, 130520, PostcodeSectorMeanOnly}},

  // test some valid formats that don't exist
  {"CR90 9SA", true, "CR90 9SA", {0, 0, PostcodeNotFound}},
  {"DD1 1DE", true, "DD1 1DE", {0, 0, PostcodeNotFound}},
  {"FK8 3RG", true, "FK8 3RG", {0, 0, PostcodeNotFound}},

  // test some close but invalid formats
  {"eee10aa", false, "", {0}},       // AAA0 0AA
  {"e1 0aaa", false, "", {0}},       // A0 0AAA
  {"e1010aa", false, "", {0}},       // A00 00AA
  {" bn1115pq ", false, "", {0}},    // AA000 0AA / AA00 00AA
  {" bn15pqr ", false, "", {0}},     // AA0 0AAA
  {"B100 9NN", false, "", {0}},      // A000 0AA
  {"Sy 21 0Hdd   ", false, "", {0}}, // AA00 0AAA
  {"   w1aa 5w w", false, "", {0}},  // A0AA 0AA
  {"ec1v77jJ", false, "", {0}},      // AA0A 00AA
  {" bn15p! ", false, "", {0}},      // AA0 0A!
  {" b.15pq ", false, "", {0}},      // A.0 0AA
  {" b5pq ", false, "", {0}},        // A 0AA
  {" 215pq ", false, "", {0}},       // 00 0AA
  {"2215pq", false, "", {0}},        // 000 0AA
  {"bn1 5p", false, "", {0}},        // AA0 0A
  {"bn1 pp", false, "", {0}},        // AA0 AA

  // test some wildly invalid formats
  {"", false, "", {0}},
  {"     \t    ", false, "", {0}},
  {"           ", false, "", {0}},
  {"xxz", false, "", {0}},
  {"90210", false, "", {0}},
  {"... ...", false, "", {0}},
};

static const PostcodeTestItem reverseLookupTestItems[] = {  // we only use formatted, easting and northing
  {"", true, "WC1A 2TA", {530300, 181600}},  // central London location in 16 outward bboxes
  {"", true, "BN1 9QQ", {534523, 109340}},  // University of Sussex
  {"", true, "SY21 0HD", {306415, 307052}},  // rural Powys, one outward box
  {"", true, "BN1 8YL", {524900, 109400}},  // rural Sussex, will fail with naive point bboxes
  {"", true, "BN41 2RF", {524200, 109400}},  // rural Sussex, will fail with naive point bboxes
  {"", true, "TR22 0PL", {86000, 7000}},  // Isles of Scilly, will fail with naive point bboxes
  {"", true, "", {215000, 630000}},  // non-existent
};

char *stringFromPostcodeTestItem(PostcodeTestItem pti) { // be sure to free(result) after use
  char *s;
  if (pti.valid && pti.en.status != PostcodeNotFound) {
    ASPRINTF_OR_DIE(&s, "%s  E %i  N %i%s", pti.formatted, pti.en.e, pti.en.n, pti.en.status == PostcodeSectorMeanOnly ? "  (sector mean)" : "");
  } else if (pti.valid) {
    ASPRINTF_OR_DIE(&s, "%s  (not found)", pti.formatted);
  } else {
    ASPRINTF_OR_DIE(&s, "INVALID");
  }
  return s;
}

bool postcodeTest(const bool noisily) {
  short numTested = 0;
  short numPassed = 0;
  
  for (int i = 0, len = LENGTH_OF(postcodeTestItems); i < len; i ++) {
    numTested ++;
    PostcodeTestItem expectedPti = postcodeTestItems[i];
    char* expectedStr = stringFromPostcodeTestItem(expectedPti);
    
    if (noisily) {
      printf("Input:    '%s'\n", expectedPti.input);
      printf("Expected: %s\n", expectedStr);
    }
    
    PostcodeTestItem actualPti = {0};
    PostcodeComponents pcc = postcodeComponentsFromString(expectedPti.input);
    actualPti.valid = pcc.valid;
    
    if (actualPti.valid) {
      char* formatted = stringFromPostcodeComponents(pcc);
      strcpy(actualPti.formatted, formatted);
      free(formatted);
      actualPti.en = eastingNorthingFromPostcodeComponents(pcc);
    }
    
    char* actualStr = stringFromPostcodeTestItem(actualPti);
    bool testPassed = strcmp(expectedStr, actualStr) == 0;
    if (testPassed) numPassed ++;
    
    if (noisily) {
      printf("Actual:   %s\n", actualStr);
      printf("%s\n\n", testPassed ? "PASSED" : "FAILED");
    }
    
    free(expectedStr);
    free(actualStr);
  }

  for (int i = 0, len = LENGTH_OF(reverseLookupTestItems); i < len; i++) {
    numTested++;
    PostcodeTestItem expectedPti = reverseLookupTestItems[i];

    if (noisily) {
      printf("Input:    E %i  N %i \n", expectedPti.en.e, expectedPti.en.n);
      printf("Expected: %s\n", expectedPti.formatted);
    }
    
    NearbyPostcode np = nearbyPostcodeFromEastingNorthing(expectedPti.en);
    char *actualStr = stringFromPostcodeComponents(np.components);
    bool testPassed = strcmp(expectedPti.formatted, actualStr) == 0;
    if (testPassed) numPassed ++;

    if (noisily) {
      printf("Actual:   %s\n", actualStr);
      printf("%s\n\n", testPassed ? "PASSED" : "FAILED");
    }

    free(actualStr);
  }

  bool allPassed = numTested == numPassed;
  if (noisily) printf("%i tests; %i passed; %i failed\n\n",
                      numTested, numPassed, numTested - numPassed);
  return allPassed;
}

