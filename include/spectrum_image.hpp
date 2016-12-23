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
    uint32_t scale_,
    int weight,
    unsigned int interval
  );
  const std::vector< float > &get_envelope() const { return envelope; }
  const float *get_pixels() const { return pixels_begin; }
  int get_delay() const { return delay; }
  int get_attack() const { return attack; }
  int get_release() const { return release; }
  double get_delay_time() const { return delay_time; }
  double get_attack_time() const { return attack_time; }
  double get_release_time() const { return release_time; }
  double get_total_time() const { return total_time; }
  int get_width() const { return x; }
  int get_height() const { return y; }
  uint32_t get_resolution() const { return resolution; }
  uint32_t get_scale() const { return scale; }
  double get_a() const { return a; }
  double get_b() const { return b; }
  uint32_t get_sample_rate() const { return sample_rate; }
private:
  std::vector< float > envelope;
  std::shared_ptr< float > pixels;
  float *pixels_begin;
  int delay;
  int attack;
  int release;
  int x;
  int y;
  uint32_t resolution;
  uint32_t scale;
  uint32_t sample_rate;
  float a;
  float b;
  double delay_time;
  double attack_time;
  double release_time;
  double total_time;
};

float get_distance(
  const spectrum_image &ref,
  const window_list_t &window,
  const std::vector< int16_t > &audio
);

#endif

