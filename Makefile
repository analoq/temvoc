temvoc: main.cpp TemVoc.hpp
	g++ -O3 -std=c++11 main.cpp kiss_fft/kiss_fft.c kiss_fft/kiss_fftr.c -Ikiss_fft -I/usr/local/include -L/usr/local/lib -lsndfile -o temvoc
