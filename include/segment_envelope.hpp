#ifndef WAV2IMAGE_SEGMENT_ENVELOPE_H
#define WAV2IMAGE_SEGMENT_ENVELOPE_H

#include <vector>
#include <tuple>

std::tuple< int, int, int > segment_envelope( const std::vector< float > &input, unsigned int sample_rate );

#endif

