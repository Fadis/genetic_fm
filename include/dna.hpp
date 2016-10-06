#ifndef WAV2IMAGE_DNA_H
#define WAV2IMAGE_DNA_H

#include <array>
#include <vector>

class dna {
public:
  dna();
  dna( const std::array< uint32_t, 56u > &src );
  dna( const dna& ) = default;
  dna( dna&& ) = default;
  dna &operator=( const dna& ) = default;
  dna &operator=( dna&& ) = default;
  std::vector< float > operator()( float attack, float release, bool ) const;
  dna crossover( const dna &r, int mutation_rate ) const;
  bool operator==( const dna &r ) const;
  bool operator!=( const dna &r ) const;
private:
  std::array< uint32_t, 56 > data;
};

#endif

