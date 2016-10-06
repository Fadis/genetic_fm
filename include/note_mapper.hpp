#include <cstdint>
#include "common.hpp"
#include "fm_operator.hpp"
#include "normalizer.hpp"

namespace tinyfm3 {
  template< unsigned int i, unsigned int j = 0u >
  struct least_pot : public least_pot< ( i >> 1 ), j + 1u > {};
  template< unsigned int j >
  struct least_pot< 1u, j > {
    constexpr static unsigned int value = j;
  };
  template< unsigned int i, unsigned int j = 1u >
  struct least_mask : public least_mask< ( i >> 1 ), j << 1 > {};
  template< unsigned int j >
  struct least_mask< 1u, j > {
    constexpr static unsigned int value = j - 1;
  };
  template< unsigned int size_ >
  class slot_queue {
    constexpr static unsigned int shift = least_pot< polyphony_count >::value;
    constexpr static unsigned int capacity_ = ( 1u << least_pot< size_ >::value );
    constexpr static unsigned int mask = least_mask< size_ >::value;
    using data_type = std::array< slot_id_t, capacity_ >;
  public:
    slot_queue() : head( 0u ), tail( 0u ) {}
    bool push( slot_id_t elem ) {
      if( full() ) gc();
      if( full() ) return false;
      data[ head++ ] = elem;
      if( head == capacity_ ) head = 0u;
      return true;
    }
    slot_id_t pop() {
      if( empty() ) {
        return std::numeric_limits<slot_id_t>::max();
      }
      slot_id_t value = data[ tail++ ];
      if( tail == capacity_ ) tail = 0u;
      return value;
    }
    bool erase( slot_id_t elem ) {
      if( tail < head ) {
        const auto search_result = std::find( std::next( data.begin(), tail ), std::next( data.end(), head ), elem );
        if( search_result != data.end() ) {
          *search_result = std::numeric_limits<slot_id_t>::max();
          return true;
        }
      }
      else if( head < tail ) {
        const auto search_result = std::find( data.begin(), std::next( data.begin(), head ), elem );
        if( search_result != std::next( data.begin(), head ) ) {
          *search_result = std::numeric_limits<slot_id_t>::max();
          return true;
        }
        else {
          const auto search_result = std::find( std::next( data.begin(), tail ), data.end(), elem );
          if( search_result != data.end() ) {
            *search_result = std::numeric_limits<slot_id_t>::max();
            return true;
          }
        }
      }
      return false;
    }
    slot_id_t erase( channel_t channel, scale_t scale ) {
      const slot_id_t id( ( channel << ( shift + 7u ) )|( scale << shift ) );
      const slot_id_t mask_( ~mask );
      if( tail < head ) {
        const auto search_result = std::find_if( std::next( data.begin(), tail ), std::next( data.begin(), head ), [&]( slot_id_t v ) { return v & mask_ == id; } );
        if( search_result != data.end() ) {
          const slot_id_t old = *search_result;
          *search_result = std::numeric_limits<slot_id_t>::max();
          return old;
        }
      }
      else if( head < tail ) {
        const auto search_result = std::find_if( data.begin(), std::next( data.begin(), head ), [&]( slot_id_t v ) { return v & mask_ == id; } );
        if( search_result != std::next( data.begin(), head ) ) {
          const slot_id_t old = *search_result;
          *search_result = std::numeric_limits<slot_id_t>::max();
          return old;
        }
        else {
          const auto search_result = std::find_if( std::next( data.begin(), tail ), data.end(), [&]( slot_id_t v ) { return v & mask_ == id; } );
          if( search_result != data.end() ) {
            const slot_id_t old = *search_result;
            *search_result = std::numeric_limits<slot_id_t>::max();
            return old;
          }
        }
      }
      return std::numeric_limits<slot_id_t>::max();
    }
    bool empty() {
      if( head == tail ) return true;
      else if( data[ tail ] != std::numeric_limits<slot_id_t>::max() ) return false;
      ++tail;
      return empty();
    }
    bool full() const {
      return (( head + 1 ) & mask ) == tail;
    }
    template< typename F >
    void channel_event( channel_t channel, F f ) {
      for( unsigned int cur = tail; cur != head; ++cur ) {
        if( cur == size_ ) cur = 0u;
        if( data[ cur ] >> ( shift + 7u ) == channel )
          f( data[ cur ] );
      }
    }
  private:
    void gc() {
      unsigned int dest = tail;
      for( unsigned int src = tail; ( src & mask ) != head; ++src ) {
        if( data[ src & mask ] != std::numeric_limits<slot_id_t>::max() )
          data[ dest++ & mask ] = data[ src & mask ];
      }
      head = dest & mask;
    }
    data_type data;
    unsigned int head;
    unsigned int tail;
  };
  class note_mapper {
    constexpr static unsigned int shift = least_pot< polyphony_count >::value;
    constexpr static unsigned int mask = least_mask< polyphony_count >::value;
  public:
    note_mapper() {
      for( unsigned int i = 0u; i != polyphony_count; ++i )
        released_slots.push( i );
    }
    slot_id_t note_on( uint8_t scale, uint8_t velocity, const fm_config *config_, const channel_state *cs ) {
      const auto old_slot = get_slot();
      const auto new_slot = ( old_slot & mask ) | ( scale << shift ) | ( cs->index << ( shift + 7u ) );
      slots[ new_slot & mask ].note_on( scale, velocity/127.f, config_, cs );
      active_slots.push( new_slot );
      return new_slot;
    }
    void note_off( channel_t channel, scale_t scale ) {
      const auto slot = active_slots.erase( channel, scale );
      if( slot != std::numeric_limits<slot_id_t>::max() ) {
        slots[ slot & mask ].note_off();
        released_slots.push( slot );
      }
    }
    void pitch_bend( channel_t channel ) {
      active_slots.channel_event( channel, [&]( slot_id_t slot ) { slots[ slot & mask ].pitch_bend(); } );
    }
    void operator++() {
      for( auto &slot: slots )
        ++slot;
    }
    float operator()() {
      float output = 0;
      float active_channels = 0;
      for( auto &slot: slots ) {
        output += slot();
        active_channels += slot.get_level();
      }
      return norm( output, active_channels );
    }
    void reset() {
      slot_id_t slot;
      while( ( slot = active_slots.pop() ) != std::numeric_limits<slot_id_t>::max() ) {
        released_slots.push( slot );
      }
      for( auto &slot: slots )
        slot.reset();
    }
    void all_note_off() {
      slot_id_t slot;
      while( ( slot = active_slots.pop() ) != std::numeric_limits<slot_id_t>::max() ) {
        slots[ slot & mask ].note_off();
        released_slots.push( slot );
      }
    }
  private:
    slot_id_t get_slot() {
      const auto slot = released_slots.pop();
      if( slot != std::numeric_limits<slot_id_t>::max() )
        return slot;
      return active_slots.pop();
    }
    std::array< fm, polyphony_count > slots;
    slot_queue< polyphony_count > active_slots;
    slot_queue< polyphony_count > released_slots;
    normalizer norm;
  };
}

