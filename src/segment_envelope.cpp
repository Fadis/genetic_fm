#include <cmath>
#include <fstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <tuple>
#include <algorithm>

#include "segment_envelope.hpp"

std::tuple< int, int, int > segment_envelope( const std::vector< float > &input, float a, float b ) {
  if( input.size() <= 1u ) {
//    std::cout << "oops0 " << input.size() << std::endl;
    return std::make_tuple( 0, 0, 0 );
  }
  std::vector< float > grad;
  grad.emplace_back( 0.f );
  for( size_t i = 1u; i != input.size(); ++i )
    grad.emplace_back( ( input[ i ] - input[ i - 1u ] )/( 2.f * a * i + b ) );
  std::vector< float > release( input.size(), 0 );
  const auto blank_iter = std::find_if( input.rbegin(), input.rend(), []( float v ) { return v != 0; } );
  if( blank_iter == input.rend() ) {
//    std::cout << "oops1" << std::endl;

    return std::make_tuple( 0, 0, int( input.size() - 1 ) );
  }
  const auto blank = std::distance( input.rbegin(), blank_iter ) + 1;
  float min_grad = std::abs( grad[ input.size() - ( 1 + blank ) ] );
  float max_release = 0.f;
  size_t end_pos = input.size() - blank;
  for( size_t i = 1u; i != end_pos; ++i ) {
    min_grad = std::min( min_grad, std::max( 0.f, -grad[ input.size() - ( i + blank ) ] ) );
    release[ input.size() - ( i + blank ) ] = min_grad * ( 2.f * a * ( end_pos - i ) + b );
    max_release = std::max( release[ input.size() - ( i + blank ) ], max_release );
  }
  const auto release_pos = ( max_release != 0.f ) ? std::distance( release.begin(), std::max_element( release.begin(), release.end() ) ) : int( input.size() - 1 );
  float delay = 0.f;
  const float max = *std::max_element( input.begin(), std::next( input.begin(), release_pos ) );
  const auto p0 = std::find_if( input.begin(), std::next( input.begin(), release_pos ), [&]( float v ){ return v >= max * 0.2f; } );
  const auto p1 = std::find_if( input.begin(), std::next( input.begin(), release_pos ), [&]( float v ){ return v >= max * 0.4f; } );
  const auto tangent = float( std::distance( p0, p1 ) )/( *p1 - *p0 );
  const auto intercept = float( std::distance( input.begin(), p0 ) ) - tangent * *p0;
  const int delay_pos = std::max( int( intercept ), 0 );
  std::vector< float > attack( input.size(), 0 );
  min_grad = 1.f/tangent;
  for( size_t i = 0u; i != release_pos - delay_pos; ++i ) {
    min_grad = std::min( min_grad, std::max( 0.f, grad[ i + delay_pos ] ) );
    attack[ i + delay_pos ] = min_grad * ( 2.f * a * i + b );
  }
  const auto attack_pos = std::distance( attack.begin(), std::max_element( attack.begin(), attack.end() ) );
//  std::cout << "d : " << delay_pos << " a : " << attack_pos << " r : " << release_pos << std::endl;
  return std::make_tuple( std::max( 0, int( delay_pos ) ), std::max( 0, int( attack_pos ) ), std::max( 0, int( release_pos ) ) );
}

