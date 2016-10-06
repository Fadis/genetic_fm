#include <cmath>
#include <fstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <tuple>
#include <algorithm>
#include <sndfile.h>

#include "load_monoral.hpp"

std::vector< int16_t > load_monoral( const std::string &filename ) {
  SF_INFO info;
  info.frames = 0;
  info.samplerate = 0;
  info.channels = 0;
  info.format = 0;
  info.sections = 0;
  info.seekable = 0;
  auto audio_file = sf_open( filename.c_str(), SFM_READ, &info );
  if( !audio_file ) {
    std::cerr << "Unable to open audio file" << std::endl;
    throw -1;
  }
  std::vector< int16_t > monoral;
  std::vector< int16_t > multi_channel;
  multi_channel.resize( info.frames * info.channels );
  const auto read_count = sf_read_short( audio_file, multi_channel.data(), info.frames * info.channels );
  if( read_count == 0u ) {
    std::cerr << "Unable to read audio file" << std::endl;
    throw -1;
  }
  sf_close( audio_file );
  if( read_count % info.channels != 0 ) {
    std::cerr << "Invalid audio file" << std::endl;
    throw -1;
  }
  multi_channel.resize( read_count );
  monoral.reserve( read_count / info.channels );
  for( auto iter = multi_channel.begin(); iter != multi_channel.end(); ) {
    auto next_iter = std::next( iter, info.channels );
    monoral.emplace_back( std::accumulate( iter, next_iter, int16_t( 0 ) ) / info.channels );
    iter = next_iter;
  }
  const auto audio_begin = std::find_if( monoral.begin(), monoral.end(), []( int16_t v ) { return v != 0; } );
  monoral.erase( monoral.begin(), audio_begin );
  const auto max_iter = std::max_element( monoral.begin(), monoral.end() );
  const auto min_iter = std::min_element( monoral.begin(), monoral.end() );
  const int16_t max =
    std::max(
      ( max_iter != monoral.end() ) ? std::abs( *max_iter ) : int16_t( 0 ),
      ( min_iter != monoral.end() ) ? std::abs( *min_iter ) : int16_t( 0 )
    );
  if( max != 0 ) {
    for( auto &elem: monoral )
      elem = int32_t( elem ) * 32768 / max;
  }
  return monoral;
}

