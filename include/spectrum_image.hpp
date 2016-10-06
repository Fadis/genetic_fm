#ifndef WAV2IMAGE_SPECTRUM_IMAGE_HPP
#define WAV2IMAGE_SPECTRUM_IMAGE_HPP

#include <cstdint>
#include <vector>

#include "fft.hpp"

class spectrum_image {
public:
  spectrum_image(
    const window_list_t &window,
    const std::vector< int16_t > &audio,
    int x_,
    uint32_t sample_rate_,
    uint32_t resolution_,
    uint32_t interval_
  );
  const std::vector< float > &get_envelope() const { return envelope; }
  const uint8_t *get_pixels() const { return pixels_begin; }
  int get_delay() const { return delay; }
  int get_attack() const { return attack; }
  int get_release() const { return release; }
  int get_width() const { return x; }
  int get_height() const { return y; }
  uint32_t get_resolution() const { return resolution; }
  uint32_t get_interval() const { return interval; }
  uint32_t get_delta_samples() const { return sample_rate / interval; }
  uint32_t get_sample_rate() const { return sample_rate; }
private:
  std::vector< float > envelope;
  std::shared_ptr< uint8_t > pixels;
  uint8_t *pixels_begin;
  int delay;
  int attack;
  int release;
  int x;
  int y;
  uint32_t resolution;
  uint32_t interval;
  uint32_t sample_rate;
};

float get_distance(
  const spectrum_image &ref,
  const window_list_t &window,
  const std::vector< int16_t > &audio
);

#endif

