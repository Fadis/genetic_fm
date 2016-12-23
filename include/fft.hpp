#ifndef WAV2IMAGE_FFT_H
#define WAV2IMAGE_FFT_H

#include <cstdlib>
#include <cstdint>
#include <string>
#include <exception>
#include <vector>
#include <memory>
#include <boost/container/flat_map.hpp>

using window_list_t = boost::container::flat_map< unsigned int, std::shared_ptr< float > >;

struct fft_failed : public std::runtime_error {
  fft_failed( const std::string &what ) : std::runtime_error( what ) {}
  fft_failed( const char *what ) : std::runtime_error( what ) {}
};

struct fft_initialization_failed : public fft_failed {
  fft_initialization_failed( const std::string &what ) : fft_failed( what ) {}
  fft_initialization_failed( const char *what ) : fft_failed( what ) {}
};
struct fft_allocation_failed : public fft_failed {
  fft_allocation_failed( const std::string &what ) : fft_failed( what ) {}
  fft_allocation_failed( const char *what ) : fft_failed( what ) {}
};
struct fft_execution_failed : public fft_failed {
  fft_execution_failed( const std::string &what ) : fft_failed( what ) {}
  fft_execution_failed( const char *what ) : fft_failed( what ) {}
};
struct fft_data_transfar_failed : public fft_failed {
  fft_data_transfar_failed( const std::string &what ) : fft_failed( what ) {}
  fft_data_transfar_failed( const char *what ) : fft_failed( what ) {}
};

void init_fft();
window_list_t generate_window();
//std::shared_ptr< float > fft( const window_list_t &window, const std::vector< int16_t > &data, size_t resolution, size_t interval, size_t width );
std::pair< std::vector< float >, std::shared_ptr< float > > fftref( const window_list_t &window, const std::vector< int16_t > &data, size_t resolution, float a, float b, size_t width );
std::pair< float, std::vector< float > > fftcomp( const float*, size_t, const window_list_t &window, const std::vector< int16_t > &data, size_t resolution, float a, float b, size_t width );


#endif

