rm *.o pv;

clang -c hanning.c -o hanning.o -O3;
clang -c phasevocoder.c -o pv.o -O3;
clang -c main.c -o main.o -O3;
clang -o pv main.o pv.o hanning.o -O3 -lfftw3 -lsndfile;
