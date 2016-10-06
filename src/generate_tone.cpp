#ifndef WAV2IMAGE_GENERATE_TONE_H
#define WAV2IMAGE_GENERATE_TONE_H

#include <cmath>
#include <fstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <tuple>
#include <algorithm>

#include "common.hpp"
#include "channel_state.hpp"
#include "fm_operator.hpp"

#include "generate_tone.hpp"

std::vector< int16_t > generate_tone(
  int note, int delay, int release, int total_length, const std::vector< float > &config, bool has_release
) {
  tinyfm3::fm_config program;
  program.reset( config.begin(), config.end() );
  tinyfm3::channel_state channel( 0 );
  channel.reset();
  tinyfm3::fm fm;
  std::vector< int16_t > samples( delay, 0 );
  samples.reserve( total_length );
  fm.note_on( uint8_t( note ), 1.0f, &program, &channel );
  for( size_t i = delay; i != release; ++i ) {
    float v = fm();
    samples.emplace_back( v * 32767 );
    ++fm;
  }
  if( has_release ) fm.note_off();
  for( size_t i = release; i != total_length; ++i ) {
    float v = fm();
    samples.emplace_back( v * 32767 );
    ++fm;
  }
  return std::move( samples );
}

#endif

