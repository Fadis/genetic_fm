#ifndef TINYFM3_COMMON_HPP
#define TINYFM3_COMMON_HPP

namespace tinyfm3 {
  struct invalid_configuration {};
  constexpr static size_t fm_operator_count = 4u;
  constexpr static size_t frequency = 44100u;
  constexpr static float delta = 1.f/44100.f;
  constexpr static int polyphony_count = 16;
  using slot_id_t = uint32_t;
  using scale_t = uint8_t;
  using channel_t = uint8_t;
  using velocity_t = uint8_t;
}

#endif

