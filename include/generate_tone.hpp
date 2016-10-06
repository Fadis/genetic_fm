#ifndef WAV2IMAGE_GENERATE_TONE_H
#define WAV2IMAGE_GENERATE_TONE_H

#include <cmath>
#include <vector>

std::vector< int16_t > generate_tone(
  int note, int delay, int release, int total_length, const std::vector< float > &config, bool
);

#endif

