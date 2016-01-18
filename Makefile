all:
	clang -o pv hanning.c phasevocoder.c main.c -lfftw3 -lsndfile

