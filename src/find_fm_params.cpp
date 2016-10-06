#include <cmath>
#include <fstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <tuple>
#include <algorithm>
#include <chrono>
#include <boost/program_options.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <sndfile.h>
#include <OpenImageIO/imageio.h>

#include "common.hpp"
#include "channel_state.hpp"
#include "fm_operator.hpp"

#include "load_monoral.hpp"
#include "generate_tone.hpp"
#include "dna.hpp"
#include "get_image_distance.hpp"
#include "spectrum_image.hpp"
#include "fft.hpp"

struct by_sum;
struct by_score;

struct sorted_dnas_element_t {
  sorted_dnas_element_t( double sum_, double score_, const dna data_ ) : sum( sum_ ), score( score_ ), data( data_ ) {}
  double sum;
  double score;
  dna data;
};
using sorted_dnas_t =
  boost::multi_index::multi_index_container<
    sorted_dnas_element_t,
    boost::multi_index::indexed_by<
      boost::multi_index::ordered_non_unique<
        boost::multi_index::tag< by_sum >,
        boost::multi_index::member< sorted_dnas_element_t, double, &sorted_dnas_element_t::sum >
      >,
      boost::multi_index::ordered_non_unique<
        boost::multi_index::tag< by_score >,
        boost::multi_index::member< sorted_dnas_element_t, double, &sorted_dnas_element_t::score >
      >
    >
  >;

template <typename T>
struct output_float_policy : boost::spirit::karma::real_policies< T >
{
  static unsigned precision(T) { return 20u; }
};

int main( int argc, char* argv[] ) {
  boost::program_options::options_description options("オプション");
  options.add_options()
    ("help,h",    "ヘルプを表示")
    ("input,i", boost::program_options::value<std::string>(),  "入力ファイル")
    ("output,o", boost::program_options::value<std::string>(),  "出力ディレクトリ")
    ("note,n", boost::program_options::value<int>()->default_value(60),  "音階")
    ("mipmap,m", boost::program_options::value<int>()->default_value(0),  "初期ミップマップレベル")
    ("has-release,r", boost::program_options::value<bool>()->default_value(true),  "NOTE_OFFを有する楽器か");
  boost::program_options::variables_map params;
  boost::program_options::store( boost::program_options::parse_command_line( argc, argv, options ), params );
  boost::program_options::notify( params );
  if( params.count("help") || !params.count("input") || !params.count("output") || !params.count( "note" ) ) {
    std::cout << options << std::endl;
    return 0;
  }
  const std::string input_filename = params["input"].as<std::string>();
  const std::string output_dir = params["output"].as<std::string>();
  init_fft();
  const auto window = generate_window();
  const auto audio = load_monoral( input_filename );
  const int x = 256;
  spectrum_image eref( window, audio, 1024, 44100, 4096, 300 );
  /*const std::array< spectrum_image, 14 > references{{
    spectrum_image( window, audio, 32, 44100, 128, 2 ),
    spectrum_image( window, audio, 64, 44100, 256, 2 ),
    spectrum_image( window, audio, 64, 44100, 256, 5 ),
    spectrum_image( window, audio, 128, 44100, 512, 5 ),
    spectrum_image( window, audio, 128, 44100, 512, 10 ),
    spectrum_image( window, audio, 256, 44100, 1024, 10 ),
    spectrum_image( window, audio, 256, 44100, 1024, 25 ),
    spectrum_image( window, audio, 512, 44100, 2048, 25 ),
    spectrum_image( window, audio, 512, 44100, 2048, 50 ),
    spectrum_image( window, audio, 1024, 44100, 4096, 50 ),
    spectrum_image( window, audio, 1024, 44100, 4096, 100 ),
    spectrum_image( window, audio, 2048, 44100, 8192, 100 ),
    spectrum_image( window, audio, 2048, 44100, 8192, 300 ),
    spectrum_image( window, audio, 4096, 44100, 8192, 900 ),
  }};*/
  const std::array< spectrum_image, 15 > references{{
    spectrum_image( window, audio, 32, 44100, 128, 2 ),
    spectrum_image( window, audio, 64, 44100, 256, 2 ),
    spectrum_image( window, audio, 64, 44100, 256, 5 ),
    spectrum_image( window, audio, 128, 44100, 512, 5 ),
    spectrum_image( window, audio, 128, 44100, 512, 10 ),
    spectrum_image( window, audio, 256, 44100, 1024, 10 ),
    spectrum_image( window, audio, 256, 44100, 1024, 25 ),
    spectrum_image( window, audio, 512, 44100, 2048, 25 ),
    spectrum_image( window, audio, 512, 44100, 2048, 50 ),
    spectrum_image( window, audio, 1024, 44100, 4096, 50 ),
    spectrum_image( window, audio, 1024, 44100, 4096, 100 ),
    spectrum_image( window, audio, 2048, 44100, 8192, 100 ),
    spectrum_image( window, audio, 2048, 44100, 8192, 300 ),
    spectrum_image( window, audio, 4096, 44100, 8192, 300 ),
    spectrum_image( window, audio, 4096, 44100, 8192, 900 ),
  }};
  std::cout << "ready" << std::endl;
  std::random_device seed_generator;
  std::mt19937 random_generator( seed_generator() );
  std::vector< dna > dnas( 3000 );
  double sum = 0.0;
  size_t mipmap_level = params["mipmap"].as<int>();
  const float attack_time = ( eref.get_attack() - eref.get_delay() )/ float( eref.get_resolution() );
  const float release_time = ( eref.get_height() - eref.get_release() )/ float( eref.get_resolution() );
  const float delta_samples = eref.get_delta_samples();
  std::array< float, 5 > history{{ 16, 8, 4, 2, 1 }};
  size_t history_head = 0u;
  std::vector< double > cached_scores;
  std::vector< dna > survived;
  const bool has_release = params["has-release"].as<bool>();
  std::vector< double > scores;
  for( size_t cycle = 0u; cycle != 4001; ++cycle ) {
    scores.clear();
    scores.reserve( dnas.size() );
    size_t counter = 0u;
    const auto begin = std::chrono::high_resolution_clock::now();
    for( size_t i = 0u; i != dnas.size(); ++i ) {
      if( !cached_scores.empty() && ( i % 11 ) == 0u ) scores.push_back( cached_scores[ i / 11u ] );
      else {
        const auto audio = generate_tone(
          params["note"].as<int>(),
          eref.get_delay() * delta_samples,
          eref.get_release() * delta_samples,
          eref.get_height() * delta_samples,
          dnas[ i ]( attack_time, release_time, has_release ),
          has_release
        );
        double distance = get_distance(
          references[ mipmap_level ],
          window,
          audio
        );
        scores.push_back( 1.0/distance );
      }
    }
    survived.clear();
    survived.reserve( 10 );
    float dist = 0.f;
    double top_score = 0.0;
    size_t top_index = 0;
    cached_scores.clear();
    cached_scores.reserve( 10u );
    {
      const auto top = std::distance( scores.begin(), std::max_element( scores.begin(), scores.end() ) );
      survived.emplace_back( std::move( dnas[ top ] ) );
      top_score = 1.0/scores[ top ];
      cached_scores.emplace_back( scores[ top ] );
      top_index = top;
      dnas.erase( std::next( dnas.begin(), top ) );
      scores.erase( std::next( scores.begin(), top ) );
      for( size_t i = 0u; i != 9u; ++i ) {
        std::discrete_distribution< size_t > distribution( scores.begin(), scores.end() );
        size_t pos = distribution( random_generator );
	survived.emplace_back( std::move( dnas[ pos ] ) );
        cached_scores.emplace_back( scores[ pos ] );
        dnas.erase( std::next( dnas.begin(), pos ) );
        scores.erase( std::next( scores.begin(), pos ) );
      }
      history[ ++history_head % history.size() ] = top_score;
      const auto min_max = std::minmax_element( history.begin(), history.end() );
      dist = *min_max.second - *min_max.first;
      if( dist < 0.00048828125f && mipmap_level != references.size() - 1u ) {
        cached_scores.clear();
        mipmap_level = mipmap_level + 1u;
      }
    }
    dnas.clear();
    const int mutation_rate = ( cycle % 20 ) ? 80+cycle/5 : 8+cycle/50;
    for( size_t l = 0u; l != survived.size(); ++l ) {
      for( size_t r = 0u; r != survived.size(); ++r ) {
        if( l != r ) dnas.emplace_back( survived[ l ].crossover( survived[ r ], mutation_rate ) );
        else dnas.emplace_back( survived[ l ] );
      }
    }
    const auto end = std::chrono::high_resolution_clock::now();
    std::cout << cycle << " " << dist << " " << top_index << " " << top_score << " " << mipmap_level << " " << std::chrono::duration_cast< std::chrono::microseconds >( end - begin ).count() << std::endl;
    if ( !( cycle % 10 ) ) {
      namespace karma = boost::spirit::karma;
      std::string filename;
      karma::generate( std::back_inserter( filename ), karma::string << "/" << karma::right_align( 4, '0' )[ karma::uint_ ] << ".conf", boost::fusion::make_vector( output_dir, cycle ) );
      std::string serialized;
      const auto config = survived.front()( attack_time, release_time, has_release );
      karma::real_generator< float, output_float_policy< float > > float_p;
      karma::generate( std::back_inserter( serialized ), float_p % ',', config );
      std::fstream file( filename.c_str(), std::ios::out );
      file.write( serialized.c_str(), serialized.size() );
      file.close();
    }
  }
}
