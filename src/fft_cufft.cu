#include <cstdint>
#include <vector>
#include <memory>
#include <iostream>
#include <cufft.h>
#include <cufftXt.h>

#include "fft.hpp"

void init_fft() {
}

#define checkCudaErrors( expr, exception ) \
{ \
  auto cuda_result = expr; \
  if( cuda_result != cudaSuccess ) { \
    std::cout << __FILE__ << " " << __LINE__ << std::endl; \
    throw exception ( cudaGetErrorString( cudaGetLastError() ) );  \
  }\
}

#define checkCuFFTErrors( expr, exception ) \
{ \
  auto cuda_result = expr; \
  if( cuda_result != CUFFT_SUCCESS ) { \
    std::cout << __FILE__ << " " << __LINE__ << " " << int( cuda_result ) << std::endl; \
    throw exception ( cudaGetErrorString( cudaGetLastError() ) ); \
  } \
}

struct fft_detail {
  float *window;
  size_t resolution;
  size_t interval;
  size_t width;
  const uint8_t *reference;
  size_t reference_batch_count;
  float diff;
  float *envelope;
  size_t batch_offset;
};

__device__ cufftReal input_cb(
  void *src, 
  size_t offset, 
  void *callerInfo, 
  void *sharedPtr
) {
  fft_detail *detail = (fft_detail*)callerInfo;
  size_t index = offset % detail->resolution;
  size_t batch = detail->batch_offset + offset / detail->resolution;
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
  size_t batch = detail->batch_offset + offset / ( (detail->resolution/2) + 1 );
  if( index < detail->width ) {
    float abs = cuCabsf( element );
    atomicAdd( detail->envelope + batch, abs );
    ( (float*)dataOut )[ index + detail->width * batch ] = abs;
  }
}

__device__ void output_log_cb(
  void *dataOut, 
  size_t offset, 
  cufftComplex element, 
  void *callerInfo, 
  void *sharedPtr
) {
  fft_detail *detail = (fft_detail*)callerInfo;
  size_t index = offset % ( (detail->resolution/2) + 1 );
  size_t batch = detail->batch_offset + offset / ( (detail->resolution/2) + 1 );
  if( index < detail->width ) {
    const float value = cuCabsf( element );
    atomicAdd( detail->envelope + batch, value );
    const float log_value_ = 80.f * __log10f( value < 1.f ? 1.f : value );
    const int log_value = log_value_ > 255.f ? int( 255 ) : int( log_value_ );
    ( (uint8_t*)dataOut )[ index + detail->width * batch ] = log_value;
  }
}

__device__ void output_comp_cb(
  void *dataOut, 
  size_t offset, 
  cufftComplex element, 
  void *callerInfo, 
  void *sharedPtr
) {
  fft_detail *detail = (fft_detail*)callerInfo;
  const size_t index = offset % ( (detail->resolution/2) + 1 );
  const size_t batch = detail->batch_offset + offset / ( (detail->resolution/2) + 1 );
  if( index < detail->width ) {
    if( batch < detail->reference_batch_count ) {
      const float value = cuCabsf( element );
      atomicAdd( detail->envelope + batch, value );
      const float log_value_ = 80.f * __log10f( value < 1.f ? 1.f : value );
      const int log_value = log_value_ > 255.f ? int( 255 ) : int( log_value_ );
      atomicAdd( &detail->diff, float( __sad( log_value, int( detail->reference[ index + detail->width * batch ] ), 0 ) ) );
    }
    else {
      const float value = cuCabsf( element );
      atomicAdd( detail->envelope + batch, value );
      const float log_value_ = 80.f * __log10f( value < 1.f ? 1.f : value );
      const float log_value = log_value_ > 255.f ? 255.f : float( log_value_ );
      atomicAdd( &detail->diff, log_value );
    }
  }
}


__device__ cufftCallbackLoadR input_cb_ptr_d = input_cb; 
__device__ cufftCallbackStoreC output_cb_ptr_d = output_cb;
__device__ cufftCallbackStoreC output_log_cb_ptr_d = output_log_cb;
__device__ cufftCallbackStoreC output_comp_cb_ptr_d = output_comp_cb;

__global__ void generate_window( fft_detail *detail ) {
  size_t index = threadIdx.x + blockIdx.x * 1024;
  detail->window[ index ] = __sinf( float( M_PI ) * float( index ) / detail->resolution ); 
}

__global__ void generate_window( float *window, unsigned int resolution ) {
  size_t index = threadIdx.x + blockIdx.x * 1024;
  window[ index ] = __sinf( float( M_PI ) * float( index ) / resolution ); 
}

__global__ void add_lacking_batches( fft_detail *detail, size_t offset ) {
  size_t index = threadIdx.x + blockIdx.x * 1024u + offset;
  atomicAdd( &detail->diff, float( detail->reference[ index ] ) );
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

std::shared_ptr< float > fft( const window_list_t &window, const std::vector< int16_t > &data, size_t resolution, size_t interval, size_t width ) {
  const auto window_iter = window.find( resolution );
  if( window_iter == window.end() ) throw fft_initialization_failed( "invalid resolution" );
  const size_t batch = ( data.size() - resolution )/interval + 1;
  float *envelope;
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
  detail->batch_offset = 0u;
  int16_t *input;
  checkCudaErrors(cudaMalloc( &input, sizeof(int16_t)*interval*batch), fft_allocation_failed );
  std::shared_ptr< int16_t > wrapped_input( input, &cudaFree );
  checkCudaErrors(cudaMemcpy( input, data.data(), sizeof(int16_t)*data.size(), cudaMemcpyHostToDevice ), fft_data_transfar_failed );
  checkCudaErrors(cudaMemset( input + sizeof(int16_t)*data.size(), 0, sizeof( int16_t )*( interval*batch - data.size() ) ), fft_data_transfar_failed );
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
  checkCudaErrors( cudaDeviceSynchronize(), fft_execution_failed );
  return std::move( wrapped_output );
}

std::pair< std::vector< float >, std::shared_ptr< uint8_t > > fftref( const window_list_t &window, const std::vector< int16_t > &data, size_t resolution, size_t interval, size_t width ) {
  const auto window_iter = window.find( resolution );
  if( window_iter == window.end() ) throw fft_initialization_failed( "invalid resolution" );
  const size_t batch = data.size() > resolution ? ( data.size() - resolution )/interval + 1 : 1u;
  float *envelope_d;
  checkCudaErrors(cudaMalloc( &envelope_d, sizeof(float)*batch), fft_allocation_failed );
  std::shared_ptr< float > wrapped_envelope( envelope_d, &cudaFree );
  checkCudaErrors( cudaMemset( envelope_d, 0, batch * sizeof(float) ), fft_initialization_failed );
  fft_detail *detail;
  checkCudaErrors(cudaMallocManaged( &detail, sizeof(fft_detail),cudaMemAttachGlobal), fft_allocation_failed );
  std::shared_ptr< fft_detail > wrapped_detail( detail, &cudaFree );
  detail->resolution = resolution;
  detail->interval = interval;
  detail->width = width;
  detail->window = window_iter->second.get();
  detail->envelope = envelope_d;
  detail->batch_offset = 0u;
  int16_t *input;
  size_t input_size = std::max( interval*batch, data.size() );
  checkCudaErrors(cudaMalloc( &input, sizeof(int16_t)*input_size), fft_allocation_failed );
  std::shared_ptr< int16_t > wrapped_input( input, &cudaFree );
  checkCudaErrors(cudaMemcpy( input, data.data(), sizeof(int16_t)*data.size(), cudaMemcpyHostToDevice ), fft_data_transfar_failed );
  if( input_size > data.size() ) {
    checkCudaErrors( cudaMemset( input + data.size(), 0, sizeof( int16_t )*( input_size - data.size() ) ), fft_initialization_failed );
  }
  uint8_t *output;
  checkCudaErrors(cudaMalloc( &output, sizeof(uint8_t)*width*batch), fft_allocation_failed );
  std::shared_ptr< uint8_t > wrapped_output( output, &cudaFree );
  cufftCallbackLoadR input_cb_ptr_h;
  checkCudaErrors( cudaMemcpyFromSymbol( &input_cb_ptr_h, input_cb_ptr_d, sizeof( cufftCallbackLoadR ) ), fft_data_transfar_failed );
  cufftCallbackStoreC output_cb_ptr_h;
  checkCudaErrors( cudaMemcpyFromSymbol( &output_cb_ptr_h, output_log_cb_ptr_d, sizeof( cufftCallbackStoreC ) ), fft_data_transfar_failed );
  for( size_t batch_offset = 0u; batch_offset < batch; batch_offset += 4200u ) {
    std::cout << batch << " " << batch_offset << " " << std::min( batch - batch_offset, size_t( 4200u ) ) << std::endl;
    detail->batch_offset = batch_offset;
    cufftHandle plan;
    checkCuFFTErrors( cufftCreate( &plan ), fft_initialization_failed );
    int signal_size = resolution;
    size_t work_size;
    checkCuFFTErrors( cufftMakePlanMany( plan, 1, &signal_size, 0, 0, 0, 0, 0, 0, CUFFT_R2C, std::min( batch - batch_offset, size_t( 4200u ) ), &work_size ), fft_initialization_failed );
    checkCuFFTErrors( cufftXtSetCallback( plan, (void **)&input_cb_ptr_h, CUFFT_CB_LD_REAL, (void **)&detail ), fft_initialization_failed );
    checkCuFFTErrors( cufftXtSetCallback( plan, (void **)&output_cb_ptr_h, CUFFT_CB_ST_COMPLEX, (void **)&detail ), fft_initialization_failed );
    checkCuFFTErrors( cufftExecR2C( plan, (cufftReal*)input, (cufftComplex *)output ), fft_execution_failed );
    checkCudaErrors( cudaDeviceSynchronize(), fft_execution_failed );
    checkCuFFTErrors( cufftDestroy( plan ), fft_allocation_failed );
  }
  std::vector< float > envelope_h( batch );
  checkCudaErrors( cudaMemcpy( envelope_h.data(), detail->envelope, sizeof(float)*batch, cudaMemcpyDeviceToHost ), fft_data_transfar_failed );
  return std::make_pair( std::move( envelope_h ), std::move( wrapped_output ) );
}

std::pair< float, std::vector< float > > fftcomp( const uint8_t *ref, size_t reference_batch_count, const window_list_t &window, const std::vector< int16_t > &data, size_t resolution, size_t interval, size_t width ) {
  const auto window_iter = window.find( resolution );
  if( window_iter == window.end() ) throw fft_initialization_failed( "invalid resolution" );
  const size_t batch = data.size() > resolution ? ( data.size() - resolution )/interval + 1 : 1u;
  float *envelope_d;
  checkCudaErrors(cudaMalloc( &envelope_d, sizeof(float)*batch), fft_allocation_failed );
  std::shared_ptr< float > wrapped_envelope( envelope_d, &cudaFree );
  checkCudaErrors( cudaMemset( envelope_d, 0, batch * sizeof(float) ), fft_initialization_failed );
  fft_detail *detail;
  checkCudaErrors(cudaMallocManaged( &detail, sizeof(fft_detail),cudaMemAttachGlobal), fft_allocation_failed );
  std::shared_ptr< fft_detail > wrapped_detail( detail, &cudaFree );
  detail->resolution = resolution;
  detail->interval = interval;
  detail->width = width;
  detail->window = window_iter->second.get();
  detail->envelope = envelope_d;
  detail->reference = ref;
  detail->reference_batch_count = reference_batch_count;
  detail->diff = 0.0f;
  detail->batch_offset = 0u;
  int16_t *input;
  size_t input_size = std::max( data.size(), batch*interval );
  checkCudaErrors(cudaMallocManaged( &input, sizeof(int16_t)*input_size ), fft_allocation_failed );
  std::shared_ptr< int16_t > wrapped_input( input, &cudaFree );
  checkCudaErrors(cudaMemcpy( input, data.data(), sizeof(int16_t)*data.size(), cudaMemcpyHostToDevice ), fft_data_transfar_failed );
  if( input_size > data.size() ) {
    checkCudaErrors( cudaMemset( input + data.size(), 0, sizeof( int16_t )*( input_size - data.size() ) ), fft_initialization_failed );
  }
  cufftCallbackLoadR input_cb_ptr_h;
  checkCudaErrors( cudaMemcpyFromSymbol( &input_cb_ptr_h, input_cb_ptr_d, sizeof( cufftCallbackLoadR ) ), fft_data_transfar_failed );
  cufftCallbackStoreC output_cb_ptr_h;
  checkCudaErrors( cudaMemcpyFromSymbol( &output_cb_ptr_h, output_comp_cb_ptr_d, sizeof( cufftCallbackStoreC ) ), fft_data_transfar_failed );
  for( size_t batch_offset = 0u; batch_offset < batch; batch_offset += 4200u ) {
    detail->batch_offset = batch_offset;
    cufftHandle plan;
    checkCuFFTErrors( cufftCreate( &plan ), fft_initialization_failed );
    int signal_size = resolution;
    size_t work_size;
    checkCuFFTErrors( cufftMakePlanMany( plan, 1, &signal_size, 0, 0, 0, 0, 0, 0, CUFFT_R2C, std::min( batch - batch_offset, size_t( 4200u ) ), &work_size ), fft_initialization_failed );
    checkCuFFTErrors( cufftXtSetCallback( plan, (void **)&input_cb_ptr_h, CUFFT_CB_LD_REAL, (void **)&detail ), fft_initialization_failed );
    checkCuFFTErrors( cufftXtSetCallback( plan, (void **)&output_cb_ptr_h, CUFFT_CB_ST_COMPLEX, (void **)&detail ), fft_initialization_failed );
    checkCuFFTErrors( cufftExecR2C( plan, (cufftReal*)input, (cufftComplex *)nullptr ), fft_execution_failed );
    checkCudaErrors( cudaDeviceSynchronize(), fft_execution_failed );
    checkCuFFTErrors( cufftDestroy( plan ), fft_allocation_failed );
  }
  if( batch < reference_batch_count ) {
    size_t left_count = ( reference_batch_count - batch ) * width;
    size_t left_block = left_count / 1024u;
    size_t left_mod = left_count % 1024u;
    if( left_block )
      add_lacking_batches<<< left_block, 1024u >>>( detail, size_t( batch * width ) );
    if( left_mod )
      add_lacking_batches<<< 1u, left_mod >>>( detail, size_t( batch * width + left_block * 1024u ) );
  }
  checkCudaErrors( cudaDeviceSynchronize(), fft_execution_failed );
  std::vector< float > envelope_h( batch );
  checkCudaErrors( cudaMemcpy( envelope_h.data(), detail->envelope, sizeof(float)*batch, cudaMemcpyDeviceToHost ), fft_data_transfar_failed );
  return std::make_pair( detail->diff, std::move( envelope_h ) );

}

