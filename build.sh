rm *.o pv;

clang -c phasevocoder.c -o pv.o;
clang -c main.c -o main.o;
clang -o pv main.o pv.o -lfftw3 -lsndfile;
