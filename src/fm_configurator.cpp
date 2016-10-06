
#define DISABLE_SINE_TABLE

#include <array>
#include <iostream>
#include <iterator>
#include <vector>
#include <fstream>
#include <string>
#include <boost/program_options.hpp>
#include <boost/spirit/include/qi.hpp>
#include <sndfile.h>

#include "common.hpp"
#include "channel_state.hpp"
#include "fm_operator.hpp"

class wavesink {
public:
  wavesink( const char *filename ) {
    config.frames = 0;
    config.samplerate = tinyfm3::frequency;
    config.channels = 1;
    config.format = SF_FORMAT_WAV|SF_FORMAT_PCM_16;
    config.sections = 0;
    config.seekable = 1;
    file = sf_open( filename, SFM_WRITE, &config );
  }
  ~wavesink() {
    sf_write_sync( file );
    sf_close( file );
  }
  void play_if_not_playing() const {
  }
  bool buffer_is_ready() const {
    return true;
  }
  void operator()( const float data ) {
    int16_t ibuf[ 1 ];
    std::transform( &data, &data + 1, ibuf, []( const float &value ) { return int16_t( value * 32767 ); } );
    sf_write_short( file, ibuf, 1 );
  }
  template< size_t i >
  void operator()( const std::array< int16_t, i > &data ) {
    sf_write_short( file, data.data(), i );
  }
private:
  SF_INFO config;
  SNDFILE* file;
};

int main( int argc, char* argv[] ) {
  boost::program_options::options_description options("オプション");
  options.add_options()
    ("help,h",    "ヘルプを表示")
    ("config,c", boost::program_options::value<std::string>(),  "入力ファイル")
    ("output,o", boost::program_options::value<std::string>(),  "出力ファイル")
    ("note,n", boost::program_options::value<int>()->default_value(60),  "音階")
    ("length,l", boost::program_options::value<float>()->default_value(5.f),  "長さ");
  boost::program_options::variables_map params;
  boost::program_options::store( boost::program_options::parse_command_line( argc, argv, options ), params );
  boost::program_options::notify( params );
  if( params.count("help") || !params.count("config") || !params.count("output") ) { 
    std::cout << options << std::endl;
    return 0;
  }
  const std::string config_filename = params["config"].as<std::string>();
  const std::string output_filename = params["output"].as<std::string>();
  std::vector< float > config;
  namespace qi = boost::spirit::qi;
  std::ifstream config_file( config_filename );
  const auto serialized_config = std::string(
    std::istreambuf_iterator<char>( config_file ),
    std::istreambuf_iterator<char>()
  );
  auto iter = serialized_config.cbegin();
  if( !qi::parse( iter, serialized_config.cend(), qi::skip( +qi::standard::space )[ qi::float_ % ',' ], config ) ) {
    std::cerr << "Invalid configuration" << std::endl;
    return -1;
  }
  if( config.size() != 70u ) {
    std::cerr << "Invalid configuration 2" << std::endl;
    return -1;
  }
  wavesink sink( output_filename.c_str() );
  tinyfm3::fm_config program;
  program.reset( config.begin(), config.end() );
  tinyfm3::channel_state channel( 0 );
  channel.reset();
  tinyfm3::fm fm;
  fm.note_on( uint8_t( params["note"].as<int>() ), 1.0f, &program, &channel );
  for( size_t i = 0u; i < tinyfm3::frequency * params["length"].as<float>(); ++i ) {
    float v = fm();
    sink( v );
    ++fm;
  }
  fm.note_off();
  for( size_t i = 0u; fm && i != tinyfm3::frequency * params["length"].as<float>(); ++i ) {
    sink( fm() );
    ++fm;
  }
}

