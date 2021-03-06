#include <cmath>
#include <vector>
#include <complex>
#include <algorithm>
#include <fftw3.h>

#include "fft.hpp"

void init_fft() {
  fftwf_init_threads();
  fftwf_plan_with_nthreads( 4 );
}

window_list_t generate_window() {
  window_list_t result;
  for( unsigned int i = 16u; i != 65536u; i <<= 1 ) {
    float *window;
    std::shared_ptr< float > w( new float[ i ], []( float *p ) { delete[] p; } );
    for( unsigned int j = 0u; j != i; ++j )
      w.get()[ j ] = sinf( float( M_PI ) * float( j ) / i );
    result.insert( result.end(), std::make_pair( i, std::move( w ) ) );
  }
  return std::move( result );
}

std::pair< std::vector< float >, std::shared_ptr< float > > fftref( const window_list_t &window, const std::vector< int16_t > &data, size_t resolution, float a, float b, size_t width ) {
  const auto window_iter = window.find( resolution );
  if( window_iter == window.end() ) throw fft_initialization_failed( "invalid resolution" );
  const float c = data.size() - resolution;
  const float x = ( -b + sqrtf( b*b + 4.f * a * c ) ) / ( 2.f * a );
  const size_t batch = size_t( x ) == 0u ? 1u : size_t( x );
  std::shared_ptr< float > input( (float*)fftwf_malloc( sizeof(float) * resolution ), &fftwf_free );
  if( !input ) throw fft_allocation_failed( "unable to allocate memory for input" );
  std::shared_ptr< fftwf_complex > output( (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*resolution), &fftwf_free );
  if( !output ) throw fft_allocation_failed( "unable to allocate memory for output" );
  fftwf_plan plan = fftwf_plan_dft_r2c_1d( resolution, input.get(), output.get(), FFTW_ESTIMATE );
  if( !plan ) fft_initialization_failed( "unable to create the plan" );
  std::vector< float > envelope;
  std::shared_ptr< float > pixels( new float[ batch * width ], []( float *p ) { delete[] p; } );
  for( size_t current_batch = 0u; current_batch != batch; ++current_batch ) {
    for( size_t i = 0u; i != resolution; ++i )
      input.get()[ i ] = data[ i + size_t( current_batch * current_batch * a + current_batch * b ) ]/32767.f * window_iter->second.get()[ i ];
    fftwf_execute( plan );
    float sum = 0.f;
    for( size_t i = 0u; i != width; ++i ) {
      const float value = std::abs( std::complex< float >( output.get()[ i ][ 0 ], output.get()[ i ][ 1 ] ) );
      //const uint8_t log_value = uint8_t( std::min( std::max( 80.f * std::log10( std::max( value, 1.f ) ), 0.f ), 255.f ) );
      pixels.get()[ i + current_batch * width ] = value;
      sum += value;
    }
    envelope.push_back( sum );
  }
  fftwf_destroy_plan( plan );
  return std::make_pair( std::move( envelope ), pixels );
}
std::pair< float, std::vector< float > > fftcomp( const float *ref, size_t batch_count, const window_list_t &window, const std::vector< int16_t > &data, size_t resolution, float a, float b, size_t width ) {
  const auto window_iter = window.find( resolution );
  if( window_iter == window.end() ) throw fft_initialization_failed( "invalid resolution" );
  const float c = data.size() - resolution;
  const float x = ( -b + sqrtf( b*b + 4.f * a * c ) ) / ( 2.f * a );
  const size_t batch = size_t( x ) == 0u ? 1u : size_t( x );
  std::shared_ptr< float > input( (float*)fftwf_malloc( sizeof(float) * resolution ), &fftwf_free );
  if( !input ) throw fft_allocation_failed( "unable to allocate memory for input" );
  std::shared_ptr< fftwf_complex > output( (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*resolution), &fftwf_free );
  if( !output ) throw fft_allocation_failed( "unable to allocate memory for output" );
  fftwf_plan plan = fftwf_plan_dft_r2c_1d( resolution, input.get(), output.get(), FFTW_ESTIMATE );
  if( !plan ) fft_initialization_failed( "unable to create the plan" );
  std::vector< float > envelope;
  float diff = 0.f;
  size_t current_batch = 0u;
  size_t min_batch_count = std::min( batch_count, batch );
  for( ; current_batch != min_batch_count; ++current_batch ) {
    for( size_t i = 0u; i != resolution; ++i )
      input.get()[ i ] = data[ i + size_t( current_batch * current_batch * a + current_batch * b ) ]/32767.f * window_iter->second.get()[ i ];
    fftwf_execute( plan );
    float sum = 0.f;
    for( size_t i = 0u; i != width; ++i ) {
      const float value = std::abs( std::complex< float >( output.get()[ i ][ 0 ], output.get()[ i ][ 1 ] ) );
      //const uint8_t log_value = uint8_t( std::min( std::max( 80.f * std::log10( std::max( value, 1.f ) ), 0.f ), 255.f ) );
      diff += std::abs( value - ref[ i + current_batch * width ] );
      sum += value;
    }
    envelope.push_back( sum );
  }
  if( batch_count > batch ) {
    for( ; current_batch != batch_count; ++current_batch )
      for( size_t i = 0u; i != width; ++i )
        diff += ref[ i + current_batch * width ];
  }
  else if( batch_count < batch ) {
    for( ; current_batch != batch; ++current_batch ) {
      for( size_t i = 0u; i != resolution; ++i )
        input.get()[ i ] = data[ i + size_t( current_batch * current_batch * a + current_batch * b ) ]/32767.f * window_iter->second.get()[ i ];
      fftwf_execute( plan );
      float sum = 0.f;
      for( size_t i = 0u; i != width; ++i ) {
        const float value = std::abs( std::complex< float >( output.get()[ i ][ 0 ], output.get()[ i ][ 1 ] ) );
        //const uint8_t log_value = uint8_t( std::min( std::max( 80.f * std::log10( std::max( value, 1.f ) ), 0.f ), 255.f ) );
        diff += value;//log_value;
        sum += value;
      }
      envelope.push_back( sum );
    }
  }
  fftwf_destroy_plan( plan );
  return std::make_pair( diff, std::move( envelope ) );
}

