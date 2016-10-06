#include <cmath>
#include <fstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <tuple>
#include <algorithm>
#include <boost/range/iterator_range.hpp>

#include "fft.hpp"
#include "spectrum_image.hpp"
#include "segment_envelope.hpp"

spectrum_image::spectrum_image(
  const window_list_t &window,
  const std::vector< int16_t > &audio,
  int x_,
  uint32_t sample_rate_,
  uint32_t resolution_,
  uint32_t interval_
) : x( x_ ), resolution( resolution_ ), interval( interval_ ), sample_rate( sample_rate_ ) {
  const auto converted = fftref( window, audio, resolution, sample_rate/interval, x );
  pixels_begin = converted.second.get();
  pixels = converted.second;
  envelope = std::move( converted.first );
  const auto delay_end = std::find_if( envelope.begin(), envelope.end(), []( float v ) { return v != 0.f; } );
  const auto delay_size = std::distance( envelope.begin(), delay_end );
  pixels_begin += delay_size * x;
  envelope.erase( envelope.begin(), delay_end );
  y = envelope.size();
  const auto tail_blank_end = std::find_if( envelope.rbegin(), envelope.rend(), []( float v ) { return v != 0.f; } );
  const auto tail_blank_size = std::distance( envelope.rbegin(), tail_blank_end );
  envelope.resize( envelope.size() - tail_blank_size );
  y = envelope.size();
  std::tie( delay, attack, release ) = segment_envelope( envelope, interval );
}

float get_distance(
  const spectrum_image &ref,
  const window_list_t &window,
  const std::vector< int16_t > &audio
) {
  const auto converted = fftcomp( ref.get_pixels(), ref.get_height(), window, audio, ref.get_resolution(), ref.get_delta_samples(), ref.get_width() );
  float delay, attack, release;
  std::tie( delay, attack, release ) = segment_envelope( converted.second, ref.get_interval() );
  double delay_distance = std::abs( ref.get_delay()/float( ref.get_resolution() ) - delay/float( ref.get_resolution() ) );
  double attack_distance = std::abs( ref.get_attack()/float( ref.get_resolution() ) - attack/float( ref.get_resolution() ) );
  double release_distance = std::abs( ref.get_release()/float( ref.get_resolution() ) - release/float( ref.get_resolution() ) );
  return double( converted.first )/ref.get_width()/ref.get_height() * ( delay_distance * 10.f + attack_distance * 10.f + release_distance * 10.f + 1.f );
}

