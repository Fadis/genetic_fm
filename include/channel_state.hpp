#ifndef TINYFM3_CHANNEL_STATE_HPP
#define TINYFM3_CHANNEL_STATE_HPP

#include "common.hpp"

namespace tinyfm3 {
  struct channel_state {
    channel_state( channel_t index_ ) : index( index_ ), modulation( 0 ), volume( 1 ), expression( 1 ), final_volume( 1 ), pitch_bend( 0 ), pitch_sensitivity( 1 ), final_pitch( 0 ), pan( 0 ) {}
    void reset() {
      modulation = 0;
      volume = 1;
      expression = 1;
      final_volume = 1;
      pitch_bend = 0;
      pitch_sensitivity = 1;
      final_pitch = 0;
      pan = 0;
    }
    channel_t index;
    float modulation;
    float volume;
    float expression;
    float final_volume;
    float pitch_bend;
    float pitch_sensitivity;
    float final_pitch;
    float pan;
    bool sustain;
  };
}

#endif

