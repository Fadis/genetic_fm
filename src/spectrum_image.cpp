#include <cmath>
#include <fstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <tuple>
#include <algorithm>
#include <boost/range/iterator_range.hpp>

#include "common.hpp"
#include "fft.hpp"
#include "spectrum_image.hpp"
#include "segment_envelope.hpp"

spectrum_image::spectrum_image(
  const window_list_t &window,
  const std::vector< int16_t > &audio,
  int x_,
  uint32_t sample_rate_,
  uint32_t resolution_,
  uint32_t scale_,
  int weight,
  unsigned int interval
) : x( x_ ), resolution( resolution_ ), scale( scale_ ), sample_rate( sample_rate_ ) {
  a = powf( 2.f, float( scale ) + weight );
  b = float( interval ) * float( scale );
  const auto converted = fftref( window, audio, resolution, a, b, x );
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
  std::tie( delay, attack, release ) = segment_envelope( envelope, a, b );
  delay_time = ( a * delay * delay + b * delay ) * tinyfm3::delta;
  attack_time = ( a * attack * attack + b * attack ) * tinyfm3::delta;
  release_time = ( a * release * release + b * release ) * tinyfm3::delta;
  total_time = ( a * envelope.size() * envelope.size() + b * envelope.size() ) * tinyfm3::delta;
  std::cout << __FILE__ << " " << __LINE__ << " " << delay_time << " " << attack_time << " " << release_time << " " << total_time << std::endl;
}
float get_distance(
  const spectrum_image &ref,
  const window_list_t &window,
  const std::vector< int16_t > &audio
) {
  const float a = ref.get_a();
  const float b = ref.get_b();
  const auto converted = fftcomp( ref.get_pixels(), ref.get_height(), window, audio, ref.get_resolution(), a, b, ref.get_width() );
  float delay, attack, release;
  std::tie( delay, attack, release ) = segment_envelope( converted.second, ref.get_a(), ref.get_b() );
  double delay_time = ( a * delay * delay + b * delay ) * tinyfm3::delta;
  double attack_time = ( a * attack * attack + b * attack ) * tinyfm3::delta;
  double release_time = ( a * release * release + b * release ) * tinyfm3::delta;
  double delay_distance = std::abs( ref.get_delay_time() - delay_time );
  double attack_distance = std::abs( ref.get_attack_time() - attack_time );
  double release_distance = std::abs( ref.get_release_time() - release_time );
//  std::cout << delay_distance << " " << attack_distance << " " << release_distance << std::endl;
  return double( converted.first )/ref.get_width()/ref.get_height() * ( delay_distance * 40.f + attack_distance * 40.f + release_distance * 40.f + 1.f );
}

