#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <algorithm>
#include <chrono>
#include <boost/program_options.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/container/flat_map.hpp>
#include <sndfile.h>
#include <OpenImageIO/imageio.h>
#ifdef ENABLE_CUDA
#include <cufft.h>
#include <cufftXt.h>
#else
#include <fftw3.h>
#endif

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

#ifdef ENABLE_CUDA

#define checkCudaErrors( expr, exception ) \
{ \
  auto cuda_result = expr; \
  if( cuda_result != cudaSuccess ) \
    throw exception ( cudaGetErrorString( cudaGetLastError() ) ); \
}

#define checkCuFFTErrors( expr, exception ) \
{ \
  auto cuda_result = expr; \
  if( cuda_result != CUFFT_SUCCESS ) \
    throw exception ( cudaGetErrorString( cudaGetLastError() ) ); \
}

struct fft_detail {
  float *window;
  size_t resolution;
  size_t interval;
  size_t width;
  float *envelope;
};

__device__ inline void atomicAdd(float* address, float value) {
  while( ( old = atomicExch( address, atomicExch( address, 0.0f ) + old ) ) != 0.0f );
}

__device__ cufftReal input_cb(
  void *src, 
  size_t offset, 
  void *callerInfo, 
  void *sharedPtr
) {
  fft_detail *detail = (fft_detail*)callerInfo;
  size_t index = offset % detail->resolution;
  size_t batch = offset / detail->resolution;
  int16_t element = ((int16_t*)src)[ index + batch * detail->interval ];
  return ( cufftReal )( element/32767.f * detail->window[ index ] );
}

__device__ void output_cb(
  void *dataOut, 
  size_t offset, 
  cufftComplex element, 
  void *callerInfo, 
  void *sharedPtr
) {
  fft_detail *detail = (fft_detail*)callerInfo;
  size_t index = offset % ( (detail->resolution/2) + 1 );
  size_t batch = offset / ( (detail->resolution/2) + 1 );
  if( index < detail->width ) {
    float abs = cuCabsf( element );
    atomicAdd( detail->envelope + detail->width * batch, abs );
    ( (float*)dataOut )[ index + detail->width * batch ] = abs;
  }
}

__device__ cufftCallbackLoadR input_cb_ptr_d = input_cb; 
__device__ cufftCallbackStoreC output_cb_ptr_d = output_cb;

__global__ void generate_window( fft_detail *detail ) {
  size_t index = threadIdx.x + blockIdx.x * 1024;
  detail->window[ index ] = sinf( float( M_PI ) * float( index ) / detail->resolution ); 
}

__global__ void generate_window( float *window, unsigned int resolution ) {
  size_t index = threadIdx.x + blockIdx.x * 1024;
  window[ index ] = sinf( float( M_PI ) * float( index ) / resolution ); 
}

using window_list_t = boost::container::flat_map< unsigned int, std::shared_ptr< float > >;

window_list_t generate_window() {
  window_list_t result;
  for( unsigned int i = 16u; i != 65536u; i <<= 1 ) {
    float *window;
    checkCudaErrors( cudaMalloc( &window, sizeof(float)*i ), fft_allocation_failed );
    std::shared_ptr< float > wrapped( window, &cudaFree );
    if( i > 1024 ) generate_window<<< i/1024, 1024 >>>( window, i );
    else generate_window<<< 1, i >>>( window, i );
    result.insert( result.end(), std::make_pair( i, wrapped ) );
  }
  checkCudaErrors( cudaDeviceSynchronize(), fft_initialization_failed );
  return std::move( result );
}

std::vector< float > fft( const window_list_t &window, const std::vector< int16_t > &data, size_t resolution, size_t interval, size_t width ) {
  const auto window_iter = window.find( resolution );
  if( window_iter == window.end() ) throw fft_initialization_failed( "invalid resolution" );
  const size_t batch = ( data.size() - resolution )/interval + 1;
  int16_t *envelope;
  checkCudaErrors(cudaMalloc( &envelope, sizeof(float)*batch), fft_allocation_failed );
  std::shared_ptr< float > wrapped_envelope( envelope, &cudaFree );
  checkCudaErrors( cudaMemset( envelope, 0, batch * sizeof(float) ), fft_initialization_failed );
  fft_detail *detail;
  checkCudaErrors(cudaMallocManaged( &detail, sizeof(fft_detail),cudaMemAttachGlobal), fft_allocation_failed );
  std::shared_ptr< fft_detail > wrapped_detail( detail, &cudaFree );
  detail->resolution = resolution;
  detail->interval = interval;
  detail->width = width;
  detail->window = window_iter->second.get();
  detail->envelope = envelope;
  int16_t *input;
  checkCudaErrors(cudaMalloc( &input, sizeof(int16_t)*data.size()), fft_allocation_failed );
  std::shared_ptr< int16_t > wrapped_input( input, &cudaFree );
  checkCudaErrors(cudaMemcpy( input, data.data(), sizeof(int16_t)*data.size(), cudaMemcpyHostToDevice ), fft_data_transfar_failed );
  float *output;
  checkCudaErrors(cudaMalloc( &output, sizeof(float)*width*batch), fft_allocation_failed );
  std::shared_ptr< float > wrapped_output( output, &cudaFree );
  cufftHandle plan;
  checkCuFFTErrors( cufftCreate( &plan ), fft_initialization_failed );
  int signal_size = resolution;
  size_t work_size;
  checkCuFFTErrors( cufftMakePlanMany( plan, 1, &signal_size, 0, 0, 0, 0, 0, 0, CUFFT_R2C, batch, &work_size ), fft_initialization_failed );
  cufftCallbackLoadR input_cb_ptr_h;
  checkCudaErrors( cudaMemcpyFromSymbol( &input_cb_ptr_h, input_cb_ptr_d, sizeof( cufftCallbackLoadR ) ), fft_data_transfar_failed );
  cufftCallbackStoreC output_cb_ptr_h;
  checkCudaErrors( cudaMemcpyFromSymbol( &output_cb_ptr_h, output_cb_ptr_d, sizeof( cufftCallbackStoreC ) ), fft_data_transfar_failed );
  checkCuFFTErrors( cufftXtSetCallback( plan, (void **)&input_cb_ptr_h, CUFFT_CB_LD_REAL, (void **)&detail ), fft_initialization_failed );
  checkCuFFTErrors( cufftXtSetCallback( plan, (void **)&output_cb_ptr_h, CUFFT_CB_ST_COMPLEX, (void **)&detail ), fft_initialization_failed );
  checkCuFFTErrors( cufftExecR2C( plan, (cufftReal*)input, (cufftComplex *)output ), fft_execution_failed );
  std::vector< float > result( width*batch );
  checkCudaErrors( cudaDeviceSynchronize(), fft_execution_failed );
  checkCudaErrors(cudaMemcpy( result.data(), output, sizeof(float)*width*batch, cudaMemcpyDeviceToHost ),fft_data_transfar_failed);
  return std::move( result );
}
#else
using window_list_t = boost::container::flat_map< unsigned int, std::vector< float > >;

window_list_t generate_window() {
  window_list_t result;
  for( unsigned int i = 16u; i != 65536u; i <<= 1 ) {
    float *window;
    std::vector< float > w( i );
    for( unsigned int j = 0u; j != i; ++j )
      w[ j ] = sinf( float( M_PI ) * float( j ) / i );
    result.insert( result.end(), std::make_pair( i, std::move( w ) ) );
  }
  return std::move( result );
}

std::vector< float > fft( const window_list_t &window, const std::vector< int16_t > &data, size_t resolution, size_t interval, size_t width ) {
  const auto window_iter = window.find( resolution );
  if( window_iter == window.end() ) throw fft_initialization_failed( "invalid resolution" );
  const size_t batch = ( data.size() - resolution )/interval + 1;
  std::shared_ptr< float > input( (float*)fftwf_malloc(sizeof(float)*resolution), &fftwf_free );
  if( !input ) throw fft_allocation_failed( "unable to allocate memory for input" );
  std::shared_ptr< fftwf_complex > output( (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex)*resolution), &fftwf_free );
  if( !output ) throw fft_allocation_failed( "unable to allocate memory for output" );
  std::vector< float > result;
  fftwf_plan plan = fftwf_plan_dft_r2c_1d( resolution, input.get(), output.get(), FFTW_ESTIMATE );
  if( !plan ) fft_initialization_failed( "unable to create the plan" );
  for( size_t current_batch = 0u; current_batch != batch; ++current_batch ) {
    for( size_t i = 0u; i != resolution; ++i )
      input.get()[ i ] = data[ i + current_batch * interval ]/32767.f * window_iter->second[ i ];
    fftwf_execute( plan );
    for( size_t i = 0u; i != width; ++i )
      result.push_back( std::abs( std::complex< float >( output.get()[ i ][ 0 ], output.get()[ i ][ 1 ] ) ) );
  }
  fftwf_destroy_plan( plan );
  return std::move( result );
}

#endif

float accumulate( const std::vector< float > &input, int x, float kp, const std::vector< std::tuple< int, float > > &window ) {
  float sum = std::accumulate(
    window.begin(), window.end(), 0.f,
    [&]( float s, const std::tuple< int, float > &v ) -> float {
      int pos = x + std::get< 0 >( v );
      if( pos < 0 || pos >= input.size() ) return s;
      else return s + std::get< 1 >( v ) * input[ x + std::get< 0 >( v ) ];
    }
  );
  return sum / kp;
}

std::tuple< int, int, int > segment_envelope( const std::vector< float > &input, unsigned int sample_rate ) {
  std::vector< float > grad;
  for( size_t i = 1u; i != input.size(); ++i )
    grad.emplace_back( ( input[ i ] - input[ i - 1u ] )*sample_rate );
  std::vector< float > release( input.size(), 0 );
  const auto blank = std::distance(
    input.rbegin(),
    std::find_if( input.rbegin(), input.rend(), []( float v ) { return v != 0; } )
  ) + 1;
  float min_grad = std::abs( grad[ input.size() - ( 1 + blank ) ] );
  for( size_t i = 1u; i != input.size() - blank; ++i ) {
    min_grad = std::min( min_grad, std::max( 0.f, -grad[ input.size() - ( i + blank ) ] ) );
    release[ input.size() - ( i + blank ) ] = min_grad * ( float( i ) / sample_rate );
  }
  const auto release_pos = std::distance( release.begin(), std::max_element( release.begin(), release.end() ) );
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
    attack[ i + delay_pos ] = min_grad * ( float( i ) / sample_rate );
  }
  const auto attack_pos = std::distance( attack.begin(), std::max_element( attack.begin(), attack.end() ) );
  std::cout << attack_pos << " " << release_pos << std::endl;
  return std::make_tuple( delay_pos, attack_pos, release_pos );
}

float rect( float x ) {
  x -= floorf( x );
  if( x < 0.5f ) return -1.f;
  else return 1.f;
}

float tri( float x ) {
  x -= floorf( x );
  if( x < 0.25f ) return 4.f*x;
  else if( x < 0.75f ) return -4.f*x+2.f;
  else return 4.f*x-4.f;
}

std::vector< int16_t > load_monoral( const std::string &filename, bool norm ) {
  SF_INFO info;
  info.frames = 0;
  info.samplerate = 0;
  info.channels = 0;
  info.format = 0;
  info.sections = 0;
  info.seekable = 0;
  std::cout << filename << std::endl;
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
  std::cout << std::distance( monoral.begin(), audio_begin ) << std::endl;
  monoral.erase( monoral.begin(), audio_begin );
  const auto max_iter = std::max_element( monoral.begin(), monoral.end() );
  const auto min_iter = std::min_element( monoral.begin(), monoral.end() );
  const int16_t max =
    std::max(
      ( max_iter != monoral.end() ) ? std::abs( *max_iter ) : int16_t( 0 ),
      ( min_iter != monoral.end() ) ? std::abs( *min_iter ) : int16_t( 0 )
    );
  if( norm ) {
    if( max != 0 ) {
      for( auto &elem: monoral )
        elem = int32_t( elem ) * 32768 / max;
    }
  }
  return monoral;
}

struct spectrum_image {
  spectrum_image(
    const window_list_t &window,
    const std::vector< int16_t > &audio,
    int x_,
    uint8_t note,
    unsigned int sample_rate_,
    uint32_t resolution_,
    uint32_t interval_,
    bool delayed
  ) : x( x_ ), resolution( resolution_ ), interval( interval_ ), sample_rate( sample_rate_ ) {
    y = audio.size() / ( sample_rate / interval );
    const int channels = 3;
    pixels.reserve( x * y * channels );
    const float freq = exp2f( ( ( float( note ) +  3.f ) / 12.f ) ) * 6.875f;
    std::vector< uint32_t > harmonic;
    for( size_t i = 0u; i != 16u; ++ i ) {
      harmonic.emplace_back( ceilf( freq * std::pow( 2.f, float( i ) ) * float( resolution ) * 2.f /float( sample_rate ) ) );
    }
    uint32_t base = harmonic.front();
    std::cout << "base : " << base << std::endl;
    size_t offset = 0;
    size_t y_pos = 0;
    size_t mute_since = 0;
    const auto converted = fft( window, audio, resolution, sample_rate/interval, x );
    const auto begin_date = std::chrono::high_resolution_clock::now();
    for( auto iter = converted.begin(); iter != converted.end(); iter += x ) {
      auto end = std::next( iter, x );
      if( !delayed || ( x > base && *std::next( iter, base ) != 0 ) ) {
        delayed = false;
        auto harmonic_iter = harmonic.begin();
        size_t x_pos = 0;
        for( const auto &l : boost::make_iterator_range( iter, std::next( iter, x ) ) ) {
          const auto p = uint8_t( std::min( std::max( 80.f * std::log10( std::max( l, 1.f ) ), 0.f ), 255.f ) );
          if( *harmonic_iter == x_pos ) {
            pixels.emplace_back( 128 + p / 2 );
            pixels.emplace_back( p );
            pixels.emplace_back( p );
            ++harmonic_iter;
          }
          else if( y_pos % interval == 0 ) {
            pixels.emplace_back( 128 + p / 2 );
            pixels.emplace_back( p );
            pixels.emplace_back( p );
          }
          else {
            pixels.emplace_back( p );
            pixels.emplace_back( p );
            pixels.emplace_back( p );
          }
          ++x_pos;
        }
        envelope.emplace_back( std::accumulate( iter, end, 0.f ) );
        ++y_pos;
        if( std::find_if( iter, end, []( float v ) { return v != 0; } ) != end ) mute_since = y_pos;
      }
      else --y;
    }
    const auto end_date = std::chrono::high_resolution_clock::now();
    std::cout << "Elapsed: " << std::chrono::duration_cast< std::chrono::microseconds >( end_date - begin_date ).count() << "us" << std::endl;
    pixels.resize( x * mute_since * channels );
  }
  std::vector< float > envelope;
  std::vector< uint8_t > pixels;
  int x;
  int y;
  uint32_t resolution;
  uint32_t interval;
  uint32_t sample_rate;
};


int main( int argc, char* argv[] ) {
  boost::program_options::options_description options("オプション");
  options.add_options()
    ("help,h",    "ヘルプを表示")
    ("input,i", boost::program_options::value<std::string>(),  "入力ファイル")
    ("output,o", boost::program_options::value<std::string>(),  "出力ファイル")
    ("envelope,e", boost::program_options::value<std::string>(),  "エンベロープ")
    ("note,n", boost::program_options::value<int>()->default_value(60),  "音階")
    ("width,w", boost::program_options::value<int>()->default_value(1024),  "幅")
    ("resolution,r", boost::program_options::value<int>()->default_value(13),  "分解能")
    ("interval,j", boost::program_options::value<int>()->default_value(100),  "間隔");
  boost::program_options::variables_map params;
  boost::program_options::store( boost::program_options::parse_command_line( argc, argv, options ), params );
  boost::program_options::notify( params );
  if( params.count("help") || !params.count("input") || !params.count("output") || !params.count( "note" ) || !params.count( "envelope" ) ) {
    std::cout << options << std::endl;
    return 0;
  }
  const std::string input_filename = params["input"].as<std::string>();
  const std::string output_filename = params["output"].as<std::string>();
  const auto audio = load_monoral( input_filename, true );
  const int x = params["width"].as<int>();
  const auto window = generate_window();
  const spectrum_image image( window, audio, x, params["note"].as<int>(), 44100, 1 << params["resolution"].as<int>(), params["interval"].as<int>(), true );
  const int channels = 3;
  using namespace OIIO_NAMESPACE;
  ImageOutput *out = ImageOutput::create( output_filename );
  if ( !out ) {
    std::cerr << "Unable to open output file" << std::endl;
    return -1;
  }
  ImageSpec spec ( image.x, image.y, channels, TypeDesc::UINT8);
  out->open( output_filename, spec );
  out->write_image( TypeDesc::UINT8, image.pixels.data() );
 out->close();

  std::fstream envelope_file( params["envelope"].as<std::string>(), std::ios::out );
  for( int i = 0; i != image.envelope.size(); ++i ) {
    std::string line;
    namespace karma = boost::spirit::karma;
    karma::generate( std::back_inserter( line ), karma::float_ << '\t' << karma::float_ << karma::eol, boost::fusion::make_vector( float( i )/image.interval, image.envelope[ i ] ) );
    envelope_file.write( line.c_str(), line.size() );
  }
  envelope_file.close();
  const auto grad = segment_envelope( image.envelope, image.interval );
  std::cout << "delay : " << std::get< 0 >( grad )/float( image.interval ) << std::endl;
  std::cout << "attack : " << std::get< 1 >( grad )/float( image.interval ) << std::endl;
  std::cout << "release : " << std::get< 2 >( grad )/float( image.interval ) << std::endl;
//  for( size_t i = 0; i != grad.size(); ++i )
//    std::cout << i * 0.01f << " " << grad[ i ] << " " << image.envelope[ i ] << std::endl;
//  ImageOutput::destroy( out );
}

