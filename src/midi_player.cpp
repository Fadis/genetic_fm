#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <array>
#include <iostream>
#include <boost/program_options.hpp>
#include "envelope.hpp"
#include "fm_operator.hpp"
#include "note_mapper.hpp"
#include "midi_player.hpp"
#include "midi_sequencer2.hpp"
#include <sndfile.h>

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
    ("input,i", boost::program_options::value<std::string>(),  "入力ファイル")
    ("output,o", boost::program_options::value<std::string>(),  "出力ファイル");
  boost::program_options::variables_map params;
  boost::program_options::store( boost::program_options::parse_command_line( argc, argv, options ), params );
  boost::program_options::notify( params );
  if( params.count("help") || !params.count("input") || !params.count("output") ) { 
    std::cout << options << std::endl;
    return 0;
  }
  const std::string input_filename = params["input"].as<std::string>();
  const std::string output_filename = params["output"].as<std::string>();
  wavesink sink( output_filename.c_str() );
  tinyfm3::midi_sequencer< const uint8_t* > seq;
  const int fd = open( input_filename.c_str(), O_RDONLY );
  if( fd < 0 ) {
    std::cout << __FILE__ << " " << __LINE__ << std::endl;
    return -1;
  }
  struct stat buf;
  if( fstat( fd, &buf ) < 0 ) {
    std::cout << __FILE__ << " " << __LINE__ << std::endl;
    return -1;
  }
  void * const mapped = mmap( NULL, buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );
  if( mapped == nullptr ) {
    std::cout << __FILE__ << " " << __LINE__ << std::endl;
    return -1;
  }
  const auto midi_begin = reinterpret_cast< uint8_t* >( mapped );
  const auto midi_end = std::next( midi_begin, buf.st_size );
  if( !seq.load( midi_begin, midi_end ) ) {
    std::cout << __FILE__ << " " << __LINE__ << std::endl;
    return -1;
  }
  std::array< int16_t, 16000u > buffer;
  while( !seq.is_end() ) {
    auto begin = buffer.begin();
    auto end = std::next( buffer.begin(), 16u );
    for( ; begin != buffer.end(); begin += 16, end += 16 ) {
      seq( begin, end );
    }
    sink( buffer );
  }
}

