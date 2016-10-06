#ifndef TINYFM3_ENVELOPE_HPP
#define TINYFM3_ENVELOPE_HPP

#include <cstdlib>
#include <cstdint>
#include <iterator>

#include "common.hpp"

namespace tinyfm3 {
  struct envelope_config {
    enum class index_t {
      ksr = 0u,
      delay = 1u,
      attack = 2u,
      hold = 3u,
      decay1 = 4u,
      decay2 = 5u,
      sustain = 6u,
      release = 7u,
    };
    envelope_config() : ksr( 0 ), delay( 0 ), attack( 0 ), hold( 0 ), decay1( 0 ), decay2( 0 ), sustain( 0 ), release( 0 ) {}
    template< typename Iterator >
    envelope_config( Iterator begin, Iterator end ) {
      if( std::distance( begin, end ) != 8u ) throw invalid_configuration();
      auto iter = begin;
      ksr = *iter++;
      delay = *iter++;
      attack = *iter++;
      hold = *iter++;
      decay1 = *iter++;
      decay2 = *iter++;
      sustain = *iter++;
      release = *iter++;
      attack = attack != 0.f ? ( 1.f / attack ) : 0.f;
      decay1 = decay1 != 0.f ? ( -1.f/ decay1 ) : 0.f;
      decay2 = decay2 != 0.f ? ( -1.f/ decay2 ) : 0.f;
      release = release != 0.f ? ( -1.f/ release ) : 0.f;
    }
    template< typename Iterator >
    void reset( Iterator begin, Iterator end ) {
      if( std::distance( begin, end ) != 8u ) throw invalid_configuration();
      auto iter = begin;
      ksr = *iter++;
      delay = *iter++;
      attack = *iter++;
      hold = *iter++;
      decay1 = *iter++;
      decay2 = *iter++;
      sustain = *iter++;
      release = *iter++;
    }
    float ksr;
    float delay;
    float attack;
    float hold;
    float decay1;
    float decay2;
    float sustain;
    float release;
  };
  class envelope {
  public:
    enum class index_t {
      ksr = 0u,
      delay = 1u,
      attack = 2u,
      hold = 3u,
      decay1 = 4u,
      decay2 = 5u,
      sustain = 6u,
      release = 7u,
    };
    envelope() : config( nullptr ), advance_( &envelope::advance_inactive ) {}
    void reset() {
      advance_ = &envelope::advance_inactive;
      config = nullptr;
    }
    float operator()() const {
      return current_level;
    }
    void note_on( uint8_t key, float pos, const envelope_config *lconfig, const envelope_config *rconfig ) {
      config = config_;
      current_time = 0u;
      current_level = 0u;
      delay = lconfig->delay * ( 1.f - pos ) + rconfig->delay * pos;
      attack = ( lconfig->attack * ( 1.f - pos ) + rconfig->attack * pos );
      hold = lconfig->hold * ( 1.f - pos ) + rconfig->hold * pos;
      decay1 = ( lconfig->decay1 * ( 1.f - pos ) + rconfig->decay1 * pos );
      decay2 = ( lconfig->decay2 * ( 1.f - pos ) + rconfig->decay2 * pos );
      sustain = lconfig->sustain * ( 1.f - pos ) + rconfig->sustain * pos;
      release = ( lconfig->release * ( 1.f - pos ) + rconfig->release * pos );
      attack = attack > delta ? ( 1.f / attack * delta ) : 0.f;
      decay1 = decay1 > delta ? ( -1.f/ decay1 * delta ) : 0.f;
      decay2 = decay2 > delta ? ( -1.f/ decay2 * delta ) : 0.f;
      release = release > delta ? ( -1.f/ release * delta ) : 0.f;
      if( delay > delta )
        advance_ = &envelope::advance_delay;
      else if( attack )
        advance_ = &envelope::advance_attack;
      else if( hold > delta ) {
        current_level = 1.f;
        advance_ = &envelope::advance_hold;
      }
      else if( decay1 ) {
        current_level = 1.f;
        advance_ = &envelope::advance_decay;
      }
      else {
        current_level = sustain;
        advance_ = &envelope::advance_sustain;
      }
    }
    void note_off() {
      if( config )
        advance_ = &envelope::advance_release;
    }
    void operator++() {
      ( this ->* advance_ )( 1u );
    }
    void operator+=( uint32_t count ) {
      ( this ->* advance_ )( count );
    }
    operator bool() const { return advance_ != &envelope::advance_inactive; }
  private:
    void advance_inactive( uint32_t ) {}
    void advance_delay( uint32_t count ) {
      current_time += delta * count;
      if( current_time >= delay ) {
        if( attack )
          advance_ = &envelope::advance_attack;
        else if( hold > delta ) {
          current_time = 0.f;
          current_level = 1.f;
          advance_ = &envelope::advance_hold;
        }
        else if( decay1 ) {
          current_level = 1.f;
          advance_ = &envelope::advance_decay;
        }
        else {
          current_level = sustain;
          advance_ = &envelope::advance_sustain;
        }
      }
    }
    void advance_attack( uint32_t count ) {
      current_level += attack * count;
      if( current_level >= 1.f ) {
        current_level = 1.f;
        current_time = 0.f;
        if( hold > delta )
          advance_ = &envelope::advance_hold;
        else if( decay1 )
          advance_ = &envelope::advance_decay;
        else {
          current_level = sustain;
          advance_ = &envelope::advance_sustain;
        }
      }
    }
    void advance_hold( uint32_t count ) {
      current_time += delta * count;
      if( current_time >= hold ) {
        if( decay1 ) {
          advance_ = &envelope::advance_decay;
        }
        else {
          current_level = sustain;
          advance_ = &envelope::advance_sustain;
        }
      }
    }
    void advance_decay( uint32_t count ) {
      current_level += decay1 * count;
      if( current_level <= sustain ) {
        current_level = sustain;
        advance_ = &envelope::advance_sustain;
      }
    }
    void advance_sustain( uint32_t count ) {
      current_level += decay2 * count;
      if( current_level <= 0 ) {
        current_level = 0;
        advance_ = &envelope::advance_inactive;
      }
    }
    void advance_release( uint32_t count ) {
      current_level += release * count;
      if( current_level <= 0 ) {
        current_level = 0;
        advance_ = &envelope::advance_inactive;
        config = nullptr;
      }
    }
    const envelope_config *config;
    float current_time;
    float current_level;
    float ksr_d;
    float attack;
    float decay1;
    float decay2;
    float release;
    void(envelope::*advance_)( uint32_t );
  };
}

#endif

