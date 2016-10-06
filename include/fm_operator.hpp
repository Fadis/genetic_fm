#ifndef TINYFM3_FM_OPERATOR_HPP
#define TINYFM3_FM_OPERATOR_HPP

#include <cstdlib>
#include <cstdint>
#include <array>
#include <algorithm>

#include "common.hpp"
#include "envelope.hpp"
#include "channel_state.hpp"

namespace tinyfm3 {

  constexpr const int8_t sine_table[ 512 ] = {
0,1,3,4,6,7,9,10,12,14,15,17,18,20,21,23,24,26,28,29,31,32,34,35,37,38,40,41,43,44,46,47,48,50,51,53,54,56,57,58,60,61,63,64,65,67,68,69,71,72,73,74,76,77,78,79,81,82,83,84,85,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,108,109,110,111,112,112,113,114,115,115,116,117,117,118,118,119,119,120,121,121,122,122,122,123,123,124,124,124,125,125,125,126,126,126,126,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,126,126,126,126,125,125,125,124,124,124,123,123,122,122,122,121,121,120,119,119,118,118,117,117,116,115,115,114,113,112,112,111,110,109,108,108,107,106,105,104,103,102,101,100,99,98,97,96,95,94,93,92,91,90,89,88,87,85,84,83,82,81,79,78,77,76,74,73,72,71,69,68,67,65,64,63,61,60,58,57,56,54,53,51,50,48,47,46,44,43,41,40,38,37,35,34,32,31,29,28,26,24,23,21,20,18,17,15,14,12,10,9,7,6,4,3,1,0,-1,-3,-4,-6,-7,-9,-10,-12,-14,-15,-17,-18,-20,-21,-23,-24,-26,-28,-29,-31,-32,-34,-35,-37,-38,-40,-41,-43,-44,-46,-47,-48,-50,-51,-53,-54,-56,-57,-58,-60,-61,-63,-64,-65,-67,-68,-69,-71,-72,-73,-74,-76,-77,-78,-79,-81,-82,-83,-84,-85,-87,-88,-89,-90,-91,-92,-93,-94,-95,-96,-97,-98,-99,-100,-101,-102,-103,-104,-105,-106,-107,-108,-108,-109,-110,-111,-112,-112,-113,-114,-115,-115,-116,-117,-117,-118,-118,-119,-119,-120,-121,-121,-122,-122,-122,-123,-123,-124,-124,-124,-125,-125,-125,-126,-126,-126,-126,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-126,-126,-126,-126,-125,-125,-125,-124,-124,-124,-123,-123,-122,-122,-122,-121,-121,-120,-119,-119,-118,-118,-117,-117,-116,-115,-115,-114,-113,-112,-112,-111,-110,-109,-108,-108,-107,-106,-105,-104,-103,-102,-101,-100,-99,-98,-97,-96,-95,-94,-93,-92,-91,-90,-89,-88,-87,-85,-84,-83,-82,-81,-79,-78,-77,-76,-74,-73,-72,-71,-69,-68,-67,-65,-64,-63,-61,-60,-58,-57,-56,-54,-53,-51,-50,-48,-47,-46,-44,-43,-41,-40,-38,-37,-35,-34,-32,-31,-29,-28,-26,-24,-23,-21,-20,-18,-17,-15,-14,-12,-10,-9,-7,-6,-4,-3,-1
  };

  constexpr const int8_t noize_table[ 512 ] = {
    90, -11, 4, -92, -126, 1, -29, -51, -76, -11, 47, 37, 100, -42, 89, 23, -4, 93, -77, 28, -116, 48, -62, -32, 79, -89, -60, 89, -113, 31, 72, -60, 105, -57, 40, -80, -47, -43, -79, -102, -41, 97, 85, 33, 55, -57, -90, 30, -105, 32, 28, 112, -116, 115, 89, -14, 110, 63, 61, -105, -60, -103, -114, -47, 106, -67, 86, 21, -42, 61, 82, -11, 11, 124, -33, -79, 28, -46, -36, -120, 42, 120, -110, 79, 106, 8, 48, -90, -26, 47, -81, 122, -17, -13, 56, -36, -48, -11, 60, -31, -15, 103, 114, 23, 35, -47, 92, -64, -15, -111, 111, 4, 33, 89, 116, 112, -38, -5, 47, -79, 66, 30, 76, -88, 120, 6, 41, 69, -98, 93, -114, 115, -10, 117, -22, 109, 71, 8, 27, -68, 21, 46, 124, 50, 9, -83, -71, -87, -19, 108, -31, -92, -8, -76, 80, -9, 92, 16, -119, 61, 57, -15, 42, -113, -19, 0, -124, 124, -29, -67, -71, -54, -125, -84, 41, -6, -94, 54, -50, 94, -41, 78, 7, -18, 30, 16, -64, -16, 33, 32, -24, 8, -55, -76, -87, 120, 112, 0, 125, -123, 104, -7, 124, 28, -22, -119, 61, 14, 42, -3, 4, -9, -76, 125, 0, 49, -78, 44, -99, -85, 89, 56, 111, -37, -97, 71, 97, -36, -45, -21, -98, -39, -81, 71, -50, -25, -52, -83, 22, -33, 18, 83, -112, -89, 16, -21, 7, 56, 125, -76, 5, -1, -60, 43, 94, 123, 83, -106, -59, 125, -24, -97, 56, -107, -32, -35, -100, 10, 99, -23, -2, 50, 46, 86, -65, 121, -6, 68, -57, -59, -21, -69, -79, -74, 72, 35, 21, 65, -117, 13, 117, 124, 46, -19, 44, 126, 118, -117, 87, -28, -119, 84, -68, -73, 78, 66, -39, -62, -2, -7, 85, 69, 99, 124, -63, -114, -81, -2, -67, 104, 37, -73, 101, -86, 116, -68, -59, -18, 101, -120, -51, 35, -57, -82, 43, -3, -85, -24, -43, -41, 49, 112, 114, 67, -25, 87, 42, 85, 70, -72, -9, 47, 77, -55, 15, -27, -21, 109, 104, 45, -97, -44, -14, -116, 106, -39, 51, -36, 54, 102, -69, 48, 65, 1, -17, 27, -30, 87, 68, 126, 48, 98, -27, -107, 69, -66, -50, 8, 122, -49, -50, 20, 39, -50, 29, 86, -94, -88, 23, 103, -81, -27, 6, 18, 119, -74, 6, 94, -26, -84, -14, -110, -88, -2, -24, 20, 40, -94, -2, -35, -17, -62, 69, -16, -122, 125, -18, 86, -58, -97, -10, -105, -76, 55, 3, 36, -46, -123, -90, -45, 53, -32, -25, 84, 103, 115, -107, -118, -84, 77, 71, 63, -32, -72, 31, 42, -82, -124, 50, 20, -62, -32, 54, 36, 13, 81, -72, -93, 121, -48, 11, 108, 109, -43, 3, -119, -78, -54, 63, 88, -115, -95, -79, -71, 2, -39, -82, 114, -9, 99, -9, 7, -113, -9, -1, 111, -29, 74, 117, -108, -19, -54, -37, -55, -44, 5, 24, -31, 58, -35, -79, -24
  };

/*
 * freq 2 000F.FFF0 freq - 1
 * sweep 1 0000.
 * ksr 1 0000.03FC 4msec to 1sec
 * delay 1 0000.03FC 4msec to 1sec
 * attack 1 0000.03FC 4msec to 1sec
 * decay1 1 0000.03FC 4msec to 1sec
 * decay2 1 0000.00FF 16msec to 4sec
 * sustain 1 0000.FF00
 * release 1 0000.00FF 16msec to 4sec
 * mod0 1 0000.FF00 modulation level
 * mod1 1 0000.FF00 modulation level
 * mod2 1 0000.FF00 modulation level
 * mod3 1 0000.FF00 modulation level
 * func 1 0 sin 1 tri 2 square 3 saw 4 noize
 */

  struct fm_operator_config {
  public:
    enum class index_t {
      freq = 0u,
      sweep = 1u,
      envelope = 2u,
      mod0 = 10u,
      mod1 = 11u,
      mod2 = 12u,
      mod3 = 13u,
      lfo = 14u,
      func = 15u
    };
    fm_operator_config() : freq( 0 ) {
      std::fill( mod.begin(), mod.end(), 0 );
    }
    template< typename Iterator >
    fm_operator_config( Iterator begin, Iterator end ) {
      if( std::distance( begin, end ) != 16u ) throw invalid_configuration();
      auto iter = begin;
      freq = *iter++;
      *iter++;
      const auto env_end = std::next( iter, 8u );
      env.reset( iter, env_end );
      iter = env_end;
      mod[ 0 ] = *iter++;
      mod[ 1 ] = *iter++;
      mod[ 2 ] = *iter++;
      mod[ 3 ] = *iter++;
      ++iter;
      func = *iter++;
    }
    template< typename Iterator >
    void reset( Iterator begin, Iterator end ) {
      if( std::distance( begin, end ) != 16u ) throw invalid_configuration();
      auto iter = begin;
      freq = *iter++;
      *iter++;
      const auto env_end = std::next( iter, 8u );
      env.reset( iter, env_end );
      iter = env_end;
      mod[ 0 ] = *iter++;
      mod[ 1 ] = *iter++;
      mod[ 2 ] = *iter++;
      mod[ 3 ] = *iter++;
      ++iter;
      func = *iter++;
    }
    float freq;
    envelope_config env;
    std::array< float, 4u > mod;
    uint32_t func;
  };

  class fm_operator {
  public:
    fm_operator() {}
    void note_on( uint8_t scale, float freq, const fm_operator_config *config_ ) {
      config = config_;
      tone_clock_grad_d = uint32_t( freq * delta * config->freq * 0x80000000 );
      tone_clock = 0;
      if( config->func == 0u ) generator = &fm_operator::sine;
      else if( config->func == 1u ) generator = &fm_operator::noize;
      else if( config->func == 2u ) generator = &fm_operator::triangle;
      else if( config->func == 3u ) generator = &fm_operator::rect;
      else if( config->func == 4u ) generator = &fm_operator::saw;
      else if( config->func == 5u ) generator = &fm_operator::half;
      else generator = &fm_operator::sine;
      env.note_on( scale, &config->env );
    }
    void note_off() {
      env.note_off();
    }
    void pitch_bend( float freq ) {
      tone_clock_grad_d = uint32_t( freq * delta * config->freq * 0x80000000 );
    }
    float operator()( float drift ) {
      return ( this->*generator )( drift );
    }
    void operator++() {
      ++env;
      tone_clock += tone_clock_grad_d;
    }
    operator bool() const {
      return env;
    }
    const fm_operator_config *config;
    float freq;
    uint32_t tone_clock_grad_d;
    envelope env;
    uint32_t tone_clock;
    float ( fm_operator::*generator )( float );
  private:
    float sine( float drift ) {
//#ifdef DISABLE_SINE_TABLE
      return sin( 2.0 * M_PI * ( tone_clock/double( 0x80000000 ) + drift )  ) * env();
//#else
//      return float( sine_table[ uint32_t( ( tone_clock/float( 0x80000000 ) + drift ) * 512 ) & 0x1FF ]/127.f ) * env();
//#endif
    }
    float cosine( float drift ) {
//#ifdef DISABLE_SINE_TABLE
      return cos( 2.0 * M_PI * ( tone_clock/double( 0x80000000 ) + drift )  ) * env();
//#else
//      return float( sine_table[ uint32_t( ( tone_clock/float( 0x80000000 ) + drift ) * 512 ) & 0x1FF ]/127.f ) * env();
//#endif
    }
    float triangle( float drift ) {
      float t = tone_clock/float( 0x80000000 ) + drift;
      float l = t - std::floor( t );
      float v;
      if( l < 0.25f ) v = 4.f * l;
      else if( l < 0.75f ) v = -4.f * l + 2.f;
      else v = 4.f * l - 4.f;
      return v * env();
    }
    float rect( float drift ) {
      float t = tone_clock/float( 0x80000000 ) + drift;
      float l = t - std::floor( t );
      return ( ( l < 0.5f ) ? 1.f : -1.f ) * env();
    }
    float saw( float drift ) {
      float t = tone_clock/float( 0x80000000 ) + drift;
      float l = t - std::floor( t );
      return ( l * 2.f - 1.f ) * env();
    }
    float noize( float drift ) {
#ifdef DISABLE_SINE_TABLE
      return ( rand() / float( RAND_MAX ) * 2.f - 1.f ) * env();
#else
      return float( noize_table[ uint32_t( ( tone_clock/float( 0x80000000 ) + drift ) * 512 ) & 0x1FF ]/127.f ) * env();
#endif
    }
    float half( float drift ) {
#ifdef DISABLE_SINE_TABLE
      return fabsf( sinf( 2.f * M_PI * ( tone_clock/float( 0x80000000 ) + drift )  ) ) * env();
#else
      return float( fabsf( sine_table[ uint32_t( ( tone_clock/float( 0x80000000 ) + drift ) * 512 ) & 0x1FF ]/127.f ) ) * env();
#endif
    }
  };

  class fm_config {
  public:
    enum class index_t {
      mix0 = 0u,
      mix1 = 1u,
      mix2 = 2u,
      mix3 = 3u,
      fazz = 4u,
      lfo_freq = 5u,
      operator0 = 6u,
      operator1 = 6u + 16u,
      operator2 = 6u + 16u * 2u,
      operator3 = 6u + 16u * 3u
    };
    fm_config() {
      std::fill( mixer.begin(), mixer.end(), 0 );
    }
    template< typename Iterator >
    fm_config( Iterator begin, Iterator end ) {
      if( std::distance( begin, end ) != 70u ) throw invalid_configuration();
      auto iter = begin;
      mixer[ 0 ] = *iter++;
      mixer[ 1 ] = *iter++;
      mixer[ 2 ] = *iter++;
      mixer[ 3 ] = *iter++;
      const float sum = std::accumulate( mixer.begin(), mixer.end(), 0.f );
      if( sum > 1.f ) {
        mixer[ 0 ] /= sum;
        mixer[ 1 ] /= sum;
        mixer[ 2 ] /= sum;
        mixer[ 3 ] /= sum;
      }
      iter++;
      iter++;
      for( int i = 0; i != 4; ++i ) {
        const auto oper_end = std::next( iter, 16u );
        oper[ i ].reset( iter, oper_end );
        iter = oper_end;
      }
    }
    template< typename Iterator >
    void reset( Iterator begin, Iterator end ) {
      if( std::distance( begin, end ) != 70u ) throw invalid_configuration();
      auto iter = begin;
      mixer[ 0 ] = *iter++;
      mixer[ 1 ] = *iter++;
      mixer[ 2 ] = *iter++;
      mixer[ 3 ] = *iter++;
      const float sum = std::accumulate( mixer.begin(), mixer.end(), 0.f );
      if( sum > 1.f ) {
        mixer[ 0 ] /= sum;
        mixer[ 1 ] /= sum;
        mixer[ 2 ] /= sum;
        mixer[ 3 ] /= sum;
      }
      iter++;
      iter++;
      for( int i = 0; i != 4; ++i ) {
        const auto oper_end = std::next( iter, 16u );
        oper[ i ].reset( iter, oper_end );
        iter = oper_end;
      }
    }
    std::array< float, 4u > mixer;
    std::array< fm_operator_config, 4u > oper;
  };

  class fm {
  public:
    fm() : config( nullptr ), calc( &fm::inactive ), advance( &fm::advance_inactive ) {}
    float operator()() { return (this->*calc)(); }
    void note_on( uint8_t scale_, float velocity_, const fm_config *config_, const channel_state *cs_ ) {
      scale = scale_;
      config = config_;
      cs = cs_;
      std::fill( sample_level.begin(), sample_level.end(), 0 );
      calc = &fm::active;
      advance = &fm::advance_active;
      const float freq = exp2f( ( ( float( scale ) + cs->final_pitch +  3.f ) / 12.f ) ) * 6.875f;
      velocity = velocity_ * cs->final_volume;
      for( size_t i = 0u; i != 4u; ++i )
        oper[ i ].note_on( scale, freq, &config->oper[ i ] );
    }
    void note_off() {
      for( auto &op: oper )
        op.note_off();
    }
    void pitch_bend() {
      const auto freq = exp2f( ( ( float( scale ) + cs->final_pitch + 3.f ) / 12.f ) ) * 6.875f;
      for( auto &op: oper )
        op.pitch_bend( freq );
    }
    void operator++() { (this->*advance)(); }
    operator bool() const {
      return oper[ 0 ] || oper[ 1 ] || oper[ 2 ] || oper[ 3 ];
    }
    float get_level() const {
      if( oper[ 0 ] || oper[ 1 ] || oper[ 2 ] || oper[ 3 ] )
        return velocity;
      else
        return 0;
    }
    void reset() {
      config = nullptr;
      calc = &fm::inactive;
      advance = &fm::advance_inactive;
    }
  private:
    float active() {
      float output = 0;
      for( uint32_t i = 0u; i != 4u; ++i ) {
        float k = oper[ i ].tone_clock/float( 0x80000000 );
        for( uint32_t modulator = 0u; modulator != 4u; ++modulator )
          k += oper[ i ].config->mod[ modulator ] * sample_level[ modulator ];
        sample_level[ i ] = oper[ i ]( k );
        output += config->mixer[ i ] * sample_level[ i ];
      }
      return output * velocity;
    }
    float inactive() {
      return 0;
    }
    void advance_active() {
      for( auto &op: oper )
        ++op;
      if( !( oper[ 0 ] || oper[ 1 ] || oper[ 2 ] || oper[ 3 ] ) ) {
        config = nullptr;
        calc = &fm::inactive;
        advance = &fm::advance_inactive;
      }
    }
    void advance_inactive() {}
    const fm_config *config;
    const channel_state *cs;
    std::array< float, 4u > sample_level;
    std::array< fm_operator, 4u > oper;
    uint8_t scale;
    float velocity;
    float(fm::*calc)();
    void(fm::*advance)();
  };
}

#endif

