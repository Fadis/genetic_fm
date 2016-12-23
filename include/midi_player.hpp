#ifndef TINYFM3_MIDI_PLAYER_HPP
#define TINYFM3_MIDI_PLAYER_HPP

#include <array>

#include "common.hpp"
#include "channel_state.hpp"

namespace tinyfm3 {
  constexpr const std::array< std::pair< scale_t, std::array< float, 70u > >, 4u > c = {{ // piano
    std::pair< scale_t, std::array< float, 70u > >( 0, std::array< float, 70u >{{
    0.9796082,0.027333,3.1014066e-04,0.3553782,0.0,0.0,0.999701,0.0,0.0,0.0952946,0.0019682,0.0344333,0.1222082,1.0472875,0.1374503,0.0018104,0.0621257,0.5214623,0.4422576,0.0798205,0.0,0.0,1.0054471,0.0,0.0,0.5863671,0.9928878,0.9980734,3.9977785,0.0306404,0.0639717,0.0039026,0.4017407,0.1531342,0.0185301,0.1214061,0.0,2.0,0.2660897,0.0,0.0,0.7332967,0.9828789,0.5512452,1.4921884,2.5392514,0.8627927,0.0123824,0.318959,0.6230813,0.1354245,0.5086474,0.0,1.0,1.0056306,0.0,0.0,0.2150809,0.155934,0.0171293,0.2404514,3.9270459,0.2475623,0.0172155,0.0608255,0.3450612,0.2006269,0.0601531,0.0,0.0
    }}),
    std::pair< scale_t, std::array< float, 70u > >( 24, std::array< float, 70u >{{
    0.0156041,0.4687458,0.1250004,0.0468734,0.0,0.0,0.201306,0.0,0.0,0.0284362,1.7868821e-05,1.8013642e-05,0.0017878,0.6610319,0.2498922,0.0199585,0.1282028,1.2986114e-04,0.3335928,0.5159912,0.0,2.0,2.0,0.0,0.0,0.1796875,0.0039232,0.1330602,1.2518352,3.0010064,0.3985858,0.0417325,0.6664157,0.0583343,0.1458065,1.0010166e-06,0.0,0.0,5.0215235,0.0,0.0,0.1250431,0.0625124,1.1302997e-05,0.9994371,3.9995268,0.3796004,0.0509822,0.6655889,8.3570681e-07,2.5288513e-07,0.2086921,0.0,0.0,2.0014379,0.0,0.0,0.9999135,0.9993215,0.9999304,3.9995883,3.0080235,0.003861,0.0247912,0.2709723,0.6666109,3.8641353e-05,0.1808434,0.0,2.0
    }}),
    std::pair< scale_t, std::array< float, 70u > >( 45, std::array< float, 70u >{{
    0.0312291,0.0312435,0.8428526,0.5000056,0.0,0.0,1.0054299,0.0,0.0,0.9998373,0.9999974,0.9999899,1.0574003,3.9994107,0.999982,0.0136532,0.066683,1.7181039e-05,0.0151768,0.6327946,0.0,2.0,1.0050896,0.0,0.0,0.1249091,0.7505561,0.9996063,3.9989203,3.9112103e-04,0.1245966,0.0070038,0.344417,0.0999991,0.3332958,0.0306326,0.0,0.0,1.0,0.0,0.0,0.1249975,0.0781287,0.1250022,0.4375192,3.9986823,0.1249998,0.0172563,1.3981066e-04,0.0289136,0.1001681,0.5374452,0.0,0.0,0.1495607,0.0,0.0,0.1813896,0.0012362,6.1609249e-04,8.3872957e-04,0.3439013,0.0779141,0.0048954,0.3231604,0.2959239,0.005716,0.0418665,0.0,4.0
    }}),
    std::pair< scale_t, std::array< float, 70u > >( 56, std::array< float, 70u >{{
    0.9998872,0.1328185,0.1091074,0.0031796,0.0,0.0,1.0,0.0,0.0,0.1800623,0.001678,0.0602197,0.0039134,0.4558271,0.7500252,3.0845764e-04,0.0306528,0.0054125,0.0172467,0.6133548,0.0,0.0,1.0027121,0.0,0.0,0.7744199,0.2499444,0.1170707,1.5017331,3.9994185,0.4383087,0.0027611,0.4763643,0.0702953,0.1200011,6.0301752e-06,0.0,0.0,1.0069851,0.0,0.0,0.1226282,0.3283757,0.0019901,0.8027105,0.438731,0.8325416,0.0094569,0.1224948,0.4943282,0.1000211,0.6183775,0.0,0.0,0.0647725,0.0,0.0,0.9993695,0.9999353,0.999666,3.9999958,3.9875672,0.0547375,0.0013088,0.1252818,0.0268754,0.2647462,0.2497633,0.0,3.0
    }}),
  }};
  class midi_player {
  public:
    midi_player() :
      channels{{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }} {
        for( auto &program: programs ) {
          program[ 0 ].first = c[ 0 ].first;
          program[ 1 ].first = c[ 1 ].first;
          program[ 2 ].first = c[ 2 ].first;
          program[ 3 ].first = c[ 3 ].first;
          program[ 0 ].second.reset( c[ 0 ].second.begin(), c[ 0 ].second.end() );
          program[ 1 ].second.reset( c[ 1 ].second.begin(), c[ 1 ].second.end() );
          program[ 2 ].second.reset( c[ 2 ].second.begin(), c[ 2 ].second.end() );
          program[ 3 ].second.reset( c[ 3 ].second.begin(), c[ 3 ].second.end() );
        }
      }
    bool event( uint8_t v ) {
      if( v < 0x80 ) return (this->*state)( v );
      else return new_event( v );
    }
    template< typename Iterator >
    void operator()( Iterator begin, Iterator end ) {
      for( Iterator cur = begin; cur != end; ++cur,++mapper )
        *cur = int16_t( mapper() * 32767.f );
    }
  private:
    bool waiting_for_event( uint8_t ) { return true; }
    bool note_off_key_number( uint8_t v ) { 
      message_buffer[ 0 ] = v;
      state = &midi_player::note_off_velocity;
      return false;
    }
    bool note_off_velocity( uint8_t ) {
      mapper.note_off( channel, scale_t( message_buffer[ 0 ] ) );
      state = &midi_player::note_off_key_number;
      return true;
    }
    bool note_on_key_number( uint8_t v ) { 
      message_buffer[ 0 ] = v;
      state = &midi_player::note_on_velocity;
      return false;
    }
    fm_config *bisect( channel_t channel, scale_t scale ) {
      auto &program = programs[ channel ];
      if( program[ 2 ].first > scale ) {
        if( program[ 1 ].first > scale ) return &program[ 0 ].second;
        else return &program[ 1 ].second;
      }
      else {
        if( program[ 3 ].first > scale ) return &program[ 2 ].second;
        else return &program[ 3 ].second;
      }
    }
    bool note_on_velocity( uint8_t v ) {
      if( channel != 10 ) {
        const scale_t scale = scale_t( message_buffer[ 0 ] - 12 );
        const auto prog = bisect( channel, scale );
        mapper.note_on( scale, velocity_t( v ), prog, &channels[ channel ] );
      }
      state = &midi_player::note_on_key_number;
      return true;
    }
    bool polyphonic_key_pressure_key_number( uint8_t ) { 
      state = &midi_player::polyphonic_key_pressure_value;
      return false;
    }
    bool polyphonic_key_pressure_value( uint8_t ) {
      state = &midi_player::polyphonic_key_pressure_key_number;
      return true;
    }
    bool control_change_key( uint8_t v ) {
      if( v == 1 )
        state = &midi_player::set_modulation;
      else if( v == 7 )
        state = &midi_player::set_volume;
      else if( v == 10 )
        state = &midi_player::set_pan;
      else if( v == 11 )
        state = &midi_player::set_expression;
      else if( v == 64 )
        state = &midi_player::set_dumper_pedal;
      else if( v == 121 )
        state = &midi_player::reset;
      else if( v == 123 )
        state = &midi_player::all_notes_off;
      else
        state = &midi_player::unknown_control;
      return false;
    }
    bool program_change( uint8_t v ) {
      programs[ channel ][ 0 ].first = c[ 0 ].first;
      programs[ channel ][ 1 ].first = c[ 1 ].first;
      programs[ channel ][ 2 ].first = c[ 2 ].first;
      programs[ channel ][ 3 ].first = c[ 3 ].first;
      programs[ channel ][ 0 ].second.reset( c[ 0 ].second.begin(), c[ 0 ].second.end() );
      programs[ channel ][ 1 ].second.reset( c[ 1 ].second.begin(), c[ 1 ].second.end() );
      programs[ channel ][ 2 ].second.reset( c[ 2 ].second.begin(), c[ 2 ].second.end() );
      programs[ channel ][ 3 ].second.reset( c[ 3 ].second.begin(), c[ 3 ].second.end() );
      state = &midi_player::program_change;
      return true;
    }
    bool channel_pressure( uint8_t v ) {
      //state = &midi_player::channel_pressure;
      return true;
    }
    bool pitch_bend_lower( uint8_t v ) {
      message_buffer[ 0 ] = v;
      state = &midi_player::pitch_bend_higher;
      return false;
    }
    bool pitch_bend_higher( uint8_t v ) {
      channels[ channel ].pitch_bend = ( ( ( int( v ) << 7 )|( int( message_buffer[ 0 ] ) ) ) - 8192 )/8191.f;
      channels[ channel ].final_pitch = channels[ channel ].pitch_bend * channels[ channel ].pitch_sensitivity;
      mapper.pitch_bend( channel );
      state = &midi_player::pitch_bend_lower;
      return true;
    }
    bool set_modulation( uint8_t v ) { // cc 1
      channels[ channel ].modulation = int( v )/127.f;
      state = &midi_player::control_change_key;
      return true;
    }
    bool set_volume( uint8_t v ) { // cc 7
      channels[ channel ].volume = int( v )/127.f;
      channels[ channel ].final_volume = channels[ channel ].volume * channels[ channel ].expression;
      state = &midi_player::control_change_key;
      return true;
    }
    bool set_pan( uint8_t v ) { // cc 10
      channels[ channel ].pan = ( int( v ) - 64 )/63.f;
      state = &midi_player::control_change_key;
      return true;
    }
    bool set_expression( uint8_t v ) { // cc 11
      channels[ channel ].expression = int( v )/127.f;
      channels[ channel ].final_volume = channels[ channel ].volume * channels[ channel ].expression;
      state = &midi_player::control_change_key;
      return true;
    }
    bool set_dumper_pedal( uint8_t v ) { // cc 64
      channels[ channel ].sustain = v >= 64;
      state = &midi_player::control_change_key;
      return true;
    }
    bool reset( uint8_t ) { // cc 121
      mapper.reset();
      std::for_each( channels.begin(), channels.end(), []( channel_state &channel ) { channel.reset(); } );
      for( auto &program: programs ) {
        program[ 0 ].first = c[ 0 ].first;
        program[ 1 ].first = c[ 1 ].first;
        program[ 2 ].first = c[ 2 ].first;
        program[ 3 ].first = c[ 3 ].first;
        program[ 0 ].second.reset( c[ 0 ].second.begin(), c[ 0 ].second.end() );
        program[ 1 ].second.reset( c[ 1 ].second.begin(), c[ 1 ].second.end() );
        program[ 2 ].second.reset( c[ 2 ].second.begin(), c[ 2 ].second.end() );
        program[ 3 ].second.reset( c[ 3 ].second.begin(), c[ 3 ].second.end() );
      }
      state = &midi_player::control_change_key;
      return true;
    }
    bool all_notes_off( uint8_t ) { // cc 123
      mapper.reset();
      state = &midi_player::control_change_key;
      return true;
    }
    bool unknown_control( uint8_t ) {
      state = &midi_player::control_change_key;
      return true;
    }
    bool new_event( uint8_t v ) {
      constexpr static const std::array< bool(midi_player::*)( uint8_t ), 8u > initial_states{{
        &tinyfm3::midi_player::note_off_key_number,
        &tinyfm3::midi_player::note_on_key_number,
        &tinyfm3::midi_player::polyphonic_key_pressure_key_number,
        &tinyfm3::midi_player::control_change_key,
        &tinyfm3::midi_player::program_change,
        &tinyfm3::midi_player::channel_pressure,
        &tinyfm3::midi_player::pitch_bend_lower,
        &tinyfm3::midi_player::waiting_for_event
      }};
      const uint8_t event = ( v >> 4 ) & 0x07;
      channel = v & 0x0F;
      state = initial_states[ event ];
      return false;
    }
    bool(midi_player::*state)( uint8_t );
    channel_t channel;
    unsigned int skip_length;
    std::array< channel_state, 16u > channels;
    std::array< std::array< std::pair< uint8_t, fm_config >, 4u >, 16u > programs;
    tinyfm3::note_mapper mapper;
    std::array< uint8_t, 16u > message_buffer;
  };
}

#endif


