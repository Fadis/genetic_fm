#ifndef TINYFM3_MIDI_SEQUENCER_HPP
#define TINYFM3_MIDI_SEQUENCER_HPP

#include <array>

#include "common.hpp"
#include "midi_player.hpp"

namespace tinyfm3 {
  struct sequencer_state {
    sequencer_state() : track_count( 0u ), resolution( 480 ), ms_to_delta_time( 0.96 ) {}
    unsigned int track_count;
    float resolution;
    float ms_to_delta_time;
  };

  template< typename Iterator >
  class track_sequencer {
  public:
    track_sequencer() : player( nullptr ), state( nullptr ), next_event_time( std::numeric_limits< uint32_t >::max() ), cur( nullptr ), end( nullptr ) {
    }
    void load( midi_player *player_, sequencer_state *state_, Iterator begin, Iterator end_ ) {
      player = player_;
      state = state_;
      cur = begin;
      end = end_;
      next_event_time = 0u;
      next_event_time = delta_time();
    }
    bool is_end() const { return cur == end; }
    void operator()( uint32_t now ) {
      while( next_event_time <= now ) {
        event();
        next_event_time = delta_time();
      }
    }
  private:
    uint32_t delta_time() {
      uint32_t delta = 0u;
      for( ; cur != end; ++cur ) {
        delta <<= 7;
        delta += *cur & 0x7F;
        if( !( *cur & 0x80 ) ) {
          ++cur;
          return next_event_time + delta;
        }
      }
      return std::numeric_limits< uint32_t >::max();
    }
    uint32_t data_length() {
      uint32_t delta = 0u;
      for( ; cur != end; ++cur ) {
        delta <<= 7;
        delta += *cur & 0x7F;
        if( !( *cur & 0x80 ) ) {
          ++cur;
          return delta;
        }
      }
      return 0u;
    }
    void midi_event( uint8_t head ) {
      if( player->event( head ) ) return;
      for( ; cur != end; ++cur ) {
        if( player->event( *cur ) ) {
          ++cur;
          return;
        }
      }
    }
    void sysex( uint8_t head ) {
      const uint32_t length = data_length();
      if( std::distance( cur, end ) <= length )
        cur = std::next( cur, length );
      else
        cur = end;
    }
    void meta_event( uint8_t head ) {
      if( cur == end ) return;
      const auto event_type = *cur;
      ++cur;
      switch( event_type ) {
        case 0x2F:
          cur = end;
          break;
        case 0x51:
          {
            const auto length = data_length();
            if( length == 3u && std::distance( cur, end ) >= 3u ) {
              uint32_t beat = *cur;
              ++cur;
              beat <<= 8u;
              beat |= *cur;
              ++cur;
              beat <<= 8u;
              beat |= *cur;
              ++cur;
              state->ms_to_delta_time = float( state->resolution ) * 1000.f / float( beat );
            }
            else {
              if( std::distance( cur, end ) >= length )
                cur = std::next( cur, length );
              else
                cur = end;
            }
            break;
          }
        default:
          {
            const auto length = data_length();
            if( std::distance( cur, end ) >= length )
              cur = std::next( cur, length );
            else
              cur = end;
          }
      };
    }
    void event() {
      if( cur == end ) return;
      const auto head = *cur;
      ++cur;
      if( head == 0xF0 || head == 0xF7 )
        sysex( head );
      else if( head == 0xFF )
        meta_event( head );
      else
        midi_event( head );
    }
    midi_player *player;
    sequencer_state *state;
    uint32_t next_event_time;
    Iterator cur;
    Iterator end;
  };

  template< typename Iterator >
  class midi_sequencer {
  public:
    midi_sequencer() {}
    bool load( Iterator begin, Iterator end ) {
      if( std::distance( begin, end ) < 14 ) return false;
      constexpr static const std::array< uint8_t, 8u > header_magic {{
        'M', 'T', 'h', 'd', 0, 0, 0, 6
      }};
      if( !std::equal( header_magic.begin(), header_magic.end(), begin ) ) return false;
      auto cur = std::next( begin, header_magic.size() );
      uint16_t format = *cur;
      ++cur;
      format <<= 8;
      format |= *cur;
      ++cur;
      if( format >= 2 ) return false;
      uint16_t track_count = *cur;
      ++cur;
      track_count <<= 8;
      track_count |= *cur;
      ++cur;
      if( track_count > 16u ) track_count = 16u;
      state.track_count = track_count;
      uint16_t resolution = *cur;
      ++cur;
      resolution <<= 8;
      resolution |= *cur;
      ++cur;
      state.resolution = resolution;
      state.ms_to_delta_time = state.resolution * 1000.f / 500000.f;
      constexpr static const std::array< uint8_t, 4u > track_magic {{
        'M', 'T', 'r', 'k'
      }};
      now = 0.f;
      for( unsigned int i = 0u; i != track_count; ++i ) {
        if( std::distance( cur, end ) < 4 ) return false;
        if( !std::equal( track_magic.begin(), track_magic.end(), cur ) ) return false;
        cur = std::next( cur, track_magic.size() );
        if( std::distance( cur, end ) < 4 ) return false;
        uint32_t track_length = *cur;
        ++cur;
        track_length <<= 8;
        track_length |= *cur;
        ++cur;
        track_length <<= 8;
        track_length |= *cur;
        ++cur;
        track_length <<= 8;
        track_length |= *cur;
        ++cur;
        if( std::distance( cur, end ) < track_length ) return false;
        const auto track_end = std::next( cur, track_length );
        tracks[ i ].load( &player, &state, cur, track_end );
        cur = track_end;
      }
      return true;
    }
    template< typename OutputIterator >
    void operator()( OutputIterator begin, OutputIterator end ) {
      for( unsigned int i = 0u; i != state.track_count; ++i )
        tracks[ i ]( now );
      player( begin, end );
      now += std::distance( begin, end ) * delta * 1000.f * state.ms_to_delta_time;
    }
    bool is_end() {
      return std::find_if( tracks.begin(), std::next( tracks.begin(), state.track_count ), []( const track_sequencer< Iterator > &t ) { return !t.is_end(); } ) == std::next( tracks.begin(), state.track_count );
    }
  private:
    midi_player player;
    sequencer_state state;
    std::array< track_sequencer< Iterator >, 16u > tracks;
    float now;
  };
}

#endif


