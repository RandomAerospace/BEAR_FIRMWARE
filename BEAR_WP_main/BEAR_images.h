#ifndef BEAR_IMAGES_H
#define BEAR_IMAGES_H

// Auto-generated. Re-run the notebook to regenerate.
// #includes every per-image header and exposes BEAR_IMAGES[] so
// firmware can cycle through them (e.g. one per SSTV transmission).

#include "BEAR320x240_01.h"
#include "BEAR320x240_03.h"


#define BEAR_IMAGE_COUNT 2
#define BEAR_IMAGE_WIDTH (320)
#define BEAR_IMAGE_HEIGHT (240)
#define BEAR_IMAGE_COMPONENTS (1)

struct BearImage {
  const uint8_t (*ref)[BEAR_IMAGE_WIDTH][BEAR_IMAGE_COMPONENTS];
  const uint8_t (*cm)[4];
};

const BearImage BEAR_IMAGES[BEAR_IMAGE_COUNT] = {
  { BEAR320x240_01, BEAR320x240_01_cm },
  { BEAR320x240_03, BEAR320x240_03_cm },

};

#endif
