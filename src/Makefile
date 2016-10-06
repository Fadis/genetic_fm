FIND_FM_PARAMS_CXX_SOURCES= dna.cpp generate_tone.cpp get_image_distance.cpp find_fm_params.cpp load_monoral.cpp segment_envelope.cpp spectrum_image.cpp
FIND_FM_PARAMS_CUDA_SOURCES= fft_cufft.cu
FIND_FM_PARAMS_CPU_SOURCES= fft_fftw.cpp
CUFIND_FM_PARAMS_OBJ = $(FIND_FM_PARAMS_CXX_SOURCES:%.cpp=%.o) $(FIND_FM_PARAMS_CUDA_SOURCES:%.cu=%.o)
FIND_FM_PARAMS_OBJ = $(FIND_FM_PARAMS_CXX_SOURCES:%.cpp=%.o) $(FIND_FM_PARAMS_CPU_SOURCES:%.cpp=%.o)
WAV2IMAGE_CUDA_SOURCES= cuwav2image.cu
WAV2IMAGE_CPU_SOURCES= wav2image.cpp
CUWAV2IMAGE_OBJ = $(WAV2IMAGE_CUDA_SOURCES:%.cu=%.o)
WAV2IMAGE_OBJ = $(WAV2IMAGE_CPU_SOURCES:%.cpp=%.o)
FM_CONFIGURATOR_CXX_SOURCES= fm_configurator.cpp
FM_CONFIGURATOR_OBJ = $(FM_CONFIGURATOR_CXX_SOURCES:%.cpp=%.o)
MIDI_PLAYER_CXX_SOURCE= midi_player.cpp
MIDI_PLAYER_OBJ = $(MIDI_PLAYER_CXX_SOURCE:%.cpp=%.o)
ALL_OBJS= $(CUFIND_FM_PARAMS_OBJ) $(FIND_FM_PARAMS_OBJ) $(CUWAV2IMAGE_OBJ) $(WAV2IMAGE_OBJ) $(FM_CONFIGURATOR_OBJ) $(MIDI_PLAYER_OBJ) find_fm_params cufind_fm_params wav2image cuwav2image fm_configurator midi_player

all: find_fm_params cufind_fm_params wav2image cuwav2image fm_configurator midi_player

%.o: %.cpp
	g++ -std=c++11 -c -o $@ $< -march=native -O3 -I../include/

%.o: %.cu
	nvcc -std=c++11 -dc -O3 -DENABLE_CUDA -o $@ $< -I../include/

cufind_fm_params: $(CUFIND_FM_PARAMS_OBJ)
	nvcc -std=c++11 -m64 -lcufft_static -lculibos -O3 -lsndfile -lboost_program_options -lOpenImageIO $(CUFIND_FM_PARAMS_OBJ) -o cufind_fm_params

find_fm_params: $(FIND_FM_PARAMS_OBJ)
	g++ -std=c++11 -O3 -march=native -lsndfile -lboost_program_options -lOpenImageIO -lfftw3f -lfftw3f_omp $(FIND_FM_PARAMS_OBJ) -o find_fm_params

cuwav2image: $(CUWAV2IMAGE_OBJ)
	nvcc -std=c++11 -m64 -lcufft_static -lculibos -O3 -lsndfile -lboost_program_options -lOpenImageIO $(CUWAV2IMAGE_OBJ) -o cuwav2image

wav2image: $(WAV2IMAGE_OBJ)
	g++ -std=c++11 -O3 -march=native -lsndfile -lboost_program_options -lOpenImageIO -lfftw3f -lfftw3f_omp $(WAV2IMAGE_OBJ) -o wav2image

fm_configurator: $(FM_CONFIGURATOR_OBJ)
	g++ -std=c++11 -O3 -march=native -lsndfile -lboost_program_options $(FM_CONFIGURATOR_OBJ) -o fm_configurator

midi_player: $(MIDI_PLAYER_OBJ)
	g++ -std=c++11 -O3 -march=native -lsndfile -lboost_program_options $(MIDI_PLAYER_OBJ) -o midi_player

clean:
	rm -f $(ALL_OBJS)

.PHONY: all

