// Copyright 2011 Google Inc.
//
// This code is licensed under the same terms as WebM:
//  Software License Agreement:  http://www.webmproject.org/license/software/
//  Additional IP Rights Grant:  http://www.webmproject.org/license/additional/
// -----------------------------------------------------------------------------
//
// Cost tables for level and modes.
//
// Author: Skal (pascal.massimino@gmail.com)

#ifndef WEBP_ENC_COST_H_
#define WEBP_ENC_COST_H_

#include "vp8enci.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern const uint16_t VP8LevelFixedCosts[2048];   // approximate cost per level
extern const uint16_t VP8EntropyCost[256];        // 8bit fixed-point log(p)

// Cost of coding one event with probability 'proba'.
static inline int VP8BitCost(int bit, uint8_t proba) {
  return !bit ? VP8EntropyCost[proba] : VP8EntropyCost[255 - proba];
}

// Cost of coding 'nb' 1's and 'total-nb' 0's using 'proba' probability.
static inline uint64_t VP8BranchCost(uint64_t nb, uint64_t total, uint8_t proba) {
  return nb * VP8BitCost(1, proba) + (total - nb) * VP8BitCost(0, proba);
}

// Level cost calculations
void VP8CalculateLevelCosts(VP8Proba* const proba);
static inline int VP8LevelCost(const uint16_t* const table, int level) {
  return VP8LevelFixedCosts[level]
       + table[level > MAX_VARIABLE_LEVEL ? MAX_VARIABLE_LEVEL : level];
}

// Mode costs
extern const uint16_t VP8FixedCostsUV[4];
extern const uint16_t VP8FixedCostsI16[4];
extern const uint16_t VP8FixedCostsI4[NUM_BMODES][NUM_BMODES][NUM_BMODES];

//-----------------------------------------------------------------------------

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif

#endif  // WEBP_ENC_COST_H_
