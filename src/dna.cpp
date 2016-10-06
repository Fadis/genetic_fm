#include <cmath>
#include <fstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <tuple>
#include <algorithm>
#include <random>

#include "dna.hpp"

dna::dna() {
  std::random_device seed_generator;
  std::mt19937 random_generator( seed_generator() );
  std::uniform_int_distribution< uint32_t > distribution( 0u, std::numeric_limits< uint32_t >::max() );
  for( uint32_t &v : data ) v = distribution( random_generator );
  data[ 4 ] |= 0xC0000000;
  data[ 17 ] |= 0xC0000000;
  data[ 30 ] |= 0xC0000000;
  data[ 43 ] = 0x80000000;
}
dna::dna( const std::array< uint32_t, 56u > &src ) {
  std::copy( src.begin(), src.end(), data.begin() );
}
std::vector< float > dna::operator()( float attack, float release, bool has_release ) const {
  std::array< float, 4u > fm0 = {{
    float( double( data[ 12 ] )/std::numeric_limits< uint32_t >::max()*0.3 ),
    float( double( data[ 13 ] )/std::numeric_limits< uint32_t >::max() ),
    float( double( data[ 14 ] )/std::numeric_limits< uint32_t >::max() ),
    float( double( data[ 15 ] )/std::numeric_limits< uint32_t >::max() )
  }};
  float fm0_scale = 1.f/(std::min( 1.0f, std::accumulate( fm0.begin(), fm0.end(), 0.f ) )*1.5f);
  std::array< float, 4u > fm1 = {{
    float( double( data[ 25 ] )/std::numeric_limits< uint32_t >::max() ),
    float( double( data[ 26 ] )/std::numeric_limits< uint32_t >::max()*0.3 ),
    float( double( data[ 27 ] )/std::numeric_limits< uint32_t >::max() ),
    float( double( data[ 28 ] )/std::numeric_limits< uint32_t >::max() )
  }};
  float fm1_scale = 1.f/(std::min( 1.0f, std::accumulate( fm1.begin(), fm1.end(), 0.f ) )*1.5f);
  std::array< float, 4u > fm2 = {{
    float( double( data[ 38 ] )/std::numeric_limits< uint32_t >::max() ),
    float( double( data[ 39 ] )/std::numeric_limits< uint32_t >::max() ),
    float( double( data[ 40 ] )/std::numeric_limits< uint32_t >::max()*0.3 ),
    float( double( data[ 41 ] )/std::numeric_limits< uint32_t >::max() )
  }};
  float fm2_scale = 1.f/(std::min( 1.0f, std::accumulate( fm2.begin(), fm2.end(), 0.f ) )*1.5f);
  std::array< float, 4u > fm3 = {{
    float( double( data[ 51 ] )/std::numeric_limits< uint32_t >::max() ),
    float( double( data[ 52 ] )/std::numeric_limits< uint32_t >::max() ),
    float( double( data[ 53 ] )/std::numeric_limits< uint32_t >::max() ),
    float( double( data[ 54 ] )/std::numeric_limits< uint32_t >::max()*0.3 )
  }};
  float fm3_scale = 1.f/(std::min( 1.0f, std::accumulate( fm3.begin(), fm3.end(), 0.f ) )*1.5f);
  std::array< float, 4u > mix = {{
    float( double( data[ 0 ] )/std::numeric_limits< uint32_t >::max() ),
    float( double( data[ 1 ] )/std::numeric_limits< uint32_t >::max() ),
    float( double( data[ 2 ] )/std::numeric_limits< uint32_t >::max() ),
    float( double( data[ 3 ] )/std::numeric_limits< uint32_t >::max() )
  }};
  //float mix_scale = 1.f/(std::max( std::accumulate( mix.begin(), mix.end(), 0.f ), 0.00048828125f ));
  return std::vector< float >{
    mix[ 0 ],
    mix[ 1 ],
    mix[ 2 ],
    mix[ 3 ],
    0.f, // fazz
    0.f, // lfo
    float( std::pow( 2.0, double( data[ 4 ] )*8.0/std::numeric_limits< uint32_t >::max() - 4.0 ) ), // fm0.freq
    0.f, // fm0.sweep
    0.f, // fm0.ksr
    float( double( data[ 5 ] )/std::numeric_limits< uint32_t >::max() ), // fm0.delay
    float( double( data[ 6 ] )/std::numeric_limits< uint32_t >::max() ), // fm0.attack
    float( double( data[ 7 ] )/std::numeric_limits< uint32_t >::max() ), // fm0.hold
    float( double( data[ 8 ] )*4.f/std::numeric_limits< uint32_t >::max() ), // fm0.decay1
    float( double( data[ 9 ] )*4.f/std::numeric_limits< uint32_t >::max() ), // fm0.decay2
    float( double( data[ 10 ] )/std::numeric_limits< uint32_t >::max() ), // fm0.sustain
    has_release ? 
      float( double( data[ 11 ] )*release*2.f/std::numeric_limits< uint32_t >::max() ) :
      float( double( data[ 9 ] )*4.f/std::numeric_limits< uint32_t >::max() ), // fm0.release
    fm0[ 0 ]*fm0_scale,
    fm0[ 1 ]*fm0_scale,
    fm0[ 2 ]*fm0_scale,
    fm0[ 3 ]*fm0_scale,
    0.f, // fm0.lfo
    float( ( data[ 16 ] >> 24 ) % 5 ), // fm0.func
    float( std::pow( 2.0, double( data[ 17 ] )*8.0/std::numeric_limits< uint32_t >::max() - 4.0 ) ), // fm1.freq
    0.f, // fm1.sweep
    0.f, // fm1.ksr
    float( double( data[ 18 ] )/std::numeric_limits< uint32_t >::max() ), // fm1.delay
    float( double( data[ 19 ] )/std::numeric_limits< uint32_t >::max() ), // fm1.attack
    float( double( data[ 20 ] )/std::numeric_limits< uint32_t >::max() ), // fm1.hold
    float( double( data[ 21 ] )*4.f/std::numeric_limits< uint32_t >::max() ), // fm1.decay1
    float( double( data[ 22 ] )*4.f/std::numeric_limits< uint32_t >::max() ), // fm1.decay2
    float( double( data[ 23 ] )/std::numeric_limits< uint32_t >::max() ), // fm1.sustain
    has_release ? 
      float( double( data[ 24 ] )*release*2.f/std::numeric_limits< uint32_t >::max() ) :
      float( double( data[ 22 ] )*4.f/std::numeric_limits< uint32_t >::max() ), // fm0.release
    fm1[ 0 ]*fm1_scale,
    fm1[ 1 ]*fm1_scale,
    fm1[ 2 ]*fm1_scale,
    fm1[ 3 ]*fm1_scale,
    0.f, // fm1.lfo
    float( ( data[ 29 ] >> 24 ) % 5 ), // fm1.func
    float( std::pow( 2.0, double( data[ 30 ] )*8.0/std::numeric_limits< uint32_t >::max() - 4.0 ) ), // fm2.freq
    0.f, // fm2.sweep
    0.f, // fm2.ksr
    float( double( data[ 31 ] )/std::numeric_limits< uint32_t >::max() ), // fm2.delay
    float( double( data[ 32 ] )/std::numeric_limits< uint32_t >::max() ), // fm2.attack
    float( double( data[ 33 ] )/std::numeric_limits< uint32_t >::max() ), // fm2.hold
    float( double( data[ 34 ] )*4.f/std::numeric_limits< uint32_t >::max() ), // fm2.decay1
    float( double( data[ 35 ] )*4.f/std::numeric_limits< uint32_t >::max() ), // fm2.decay2
    float( double( data[ 36 ] )/std::numeric_limits< uint32_t >::max() ), // fm2.sustain
    has_release ? 
      float( double( data[ 37 ] )*release*2.f/std::numeric_limits< uint32_t >::max() ) :
      float( double( data[ 35 ] )*4.f/std::numeric_limits< uint32_t >::max() ), // fm0.release
    fm2[ 0 ]*fm2_scale,
    fm2[ 1 ]*fm2_scale,
    fm2[ 2 ]*fm2_scale,
    fm2[ 3 ]*fm2_scale,
    0.f, // fm2.lfo
    float( ( data[ 42 ] >> 24 ) % 5 ), // fm2.func
    float( std::pow( 2.0, double( data[ 43 ] )*8.0/std::numeric_limits< uint32_t >::max() - 4.0 ) ), // fm3.freq
    0.f, // fm3.sweep
    0.f, // fm3.ksr
    float( double( data[ 44 ] )/std::numeric_limits< uint32_t >::max() ), // fm3.delay
    float( double( data[ 45 ] )/std::numeric_limits< uint32_t >::max() ), // fm3.attack
    float( double( data[ 46 ] )/std::numeric_limits< uint32_t >::max() ), // fm3.hold
    float( double( data[ 47 ] )*4.f/std::numeric_limits< uint32_t >::max() ), // fm3.decay1
    float( double( data[ 48 ] )*4.f/std::numeric_limits< uint32_t >::max() ), // fm3.decay2
    float( double( data[ 49 ] )/std::numeric_limits< uint32_t >::max() ), // fm3.sustain
    has_release ? 
      float( double( data[ 50 ] )*release*2.f/std::numeric_limits< uint32_t >::max() ) :
      float( double( data[ 48 ] )*4.f/std::numeric_limits< uint32_t >::max() ), // fm0.release
    fm3[ 0 ]*fm3_scale,
    fm3[ 1 ]*fm3_scale,
    fm3[ 2 ]*fm3_scale,
    fm3[ 3 ]*fm3_scale,
    0.f, // fm3.lfo
    float( ( data[ 55 ] >> 24 ) % 5 ), // fm3.func
  };
}
dna dna::crossover( const dna &r, int mutation_rate ) const {
  std::random_device seed_generator;
  std::mt19937 random_generator( seed_generator() );
  std::uniform_int_distribution< uint32_t > distribution( 0u, std::numeric_limits< uint32_t >::max() );
  std::array< uint32_t, 56u > generated;
  for( size_t i = 0u; i != data.size(); ++i ) {
    uint32_t mask = distribution( random_generator );
    generated[ i ] = ( data[ i ] & mask ) | ( r.data[ i ] & ~mask );
  }
  for( size_t i = 0u; i != data.size(); ++i ) {
    for( size_t b = 0u; b != 32u; ++b ) {
      uint32_t score = distribution( random_generator );
      if( score < std::numeric_limits< uint32_t >::max()/mutation_rate )
        generated[ i ] = generated[ i ] ^ ( 1 << b );
    }
  }
  return dna( generated );
}
bool dna::operator==( const dna &r ) const {
  return std::equal( data.begin(), data.end(), r.data.begin() );
}
bool dna::operator!=( const dna &r ) const {
  return std::mismatch( data.begin(), data.end(), r.data.begin() ).first != data.end();
}

