#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex>
#include <fftw3.h>
#include <sndfile.h>
#include "i_indices.h"
#include "j_indices.h"
#include "real_vals.h"
#include "imag_vals.h"
#include <QList>
#include <QPointF>
#include <QDebug>

void calculate_hamming(double* array, int size) {
    for(int i = 0; i < size; i++) {
        array[i] = 0.54 - 0.46*cos(2*M_PI*(double)i/(double)size);
    }
}

double nextpow2(double n) {
    int ret = 1;
    while (ret < n) {
        ret *= 2;
    }
    return (double)ret;
}

QList<qreal> calculate_cqt(double* audio_frames) {
    /* Take FFT */
    double* in = (double*)fftw_malloc(65536 * sizeof(double));
    fftw_complex* out = (fftw_complex*)fftw_malloc(65536 * sizeof(fftw_complex));
    fftw_plan p = fftw_plan_dft_r2c_1d(65536, in, out, FFTW_MEASURE);

    QList< QPointF > output;
    output.reserve(175);
    int match;
    int last_match = 0;

    double fft_real;
    double fft_imag;
    double sk_real;
    double sk_imag;
    double tmp_real;
    double tmp_imag;

    //    FILE* debug_file = fopen("/home/chad/Programming/cq_fast_c/debug.txt","w");

    match = 0;
    last_match = 0;

    /* Take FFT */
    memset(in, 0, 65536*sizeof(double));
    memcpy(&in[30720],audio_frames,4096*2*sizeof(double));

    fftw_execute(p);

    /* Multiply by sparse kernel */
    for(int row = 1; row < 176; row++) {
        for(match = last_match; (int)j_indices[match] == row; match++) {
            fft_real = out[i_indices[match+1]][0];
            fft_imag = out[i_indices[match+1]][1];
            sk_real = (real_vals[i_indices[match-1]]*10)*real_vals[i_indices[match-1]]*10;
            sk_imag = (imag_vals[i_indices[match-1]]*10)*imag_vals[i_indices[match-1]]*10;
            tmp_real = (fft_real*sk_real - fft_imag*sk_imag)*100;
            tmp_imag = fft_real*sk_imag + fft_imag*sk_real*100;
            last_match += 1;

            if (output.size() <= row-1) {
                output.append(QPointF(0,0));
            }
//            output[row-1].setX( output[row-1].x() + tmp_real );
//            output[row-1].setY( output[row-1].y() + tmp_imag );
            output[row-1].setX(output[row-1].x() + tmp_real*tmp_imag * 1000000);
            output[row-1].setY(output[row-1].x());
        }
    }

    QList<qreal> real_output;
    real_output.reserve(175);
    for(int i = 0; i < 175; i++) {
        real_output.append(output.at(i).x()*output.at(i).x() +
                           output.at(i).y()*output.at(i).y());
    }

    free(in);
    free(out);

    return real_output;

    /* Debug to file (for now) */
    //    for(int j = 0; j < 175; j++) {
    //        fprintf( debug_file, "%d,", output[j] );
    //    }
    //    fprintf(debug_file, "\n");
    //    fclose(debug_file);
}
