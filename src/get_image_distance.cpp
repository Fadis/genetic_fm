#include <cmath>
#include <fstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <tuple>
#include <algorithm>

#include "get_image_distance.hpp"

double get_image_distance( const std::vector< uint8_t > &l, const std::vector< uint8_t > &r ) {
  if( l.size() > r.size() ) {
    return get_image_distance( r, l );
  }
  double sum = 0.0;
  double local = 0.0;
  for( size_t i = 0; i != l.size(); ++i ) {
    double d = r[ i ] - l[ i ];
    local += std::abs( d );
    if( !( i % 100000 ) ) {
      sum += local;
      local = 0.0;
    }
  }
  for( size_t i = l.size(); i != r.size(); ++i ) {
    double d = r[ i ];
    local += std::abs( d );
    if( !( i % 100000 ) ) {
      sum += local;
      local = 0.0;
    }
  }
  sum += local;
  return sum;
}

