#ifndef PHASEVOCODER_H
#define PHASEVOCODER_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fftw3.h>
#include <samplerate.h>
#include <sndfile.h>

typedef struct PhaseVocoder
{
    int an_idx, syn_idx, tempo_syn_idx, tempo_an_idx;
    int has_written_until;
    float* audio_frames_out;
    int offset;
    sf_count_t num_frames;
    /* number of FFT windows for entire file (if doing all at once) */
    long num_windows;
    double* audio_frames;
    double* audio_frames_reading_from;
    char* file_path;

    /* basic FFT variables */
    int fft_size;
    double* hanning_window;
    double timestretch_ratio;
    int overlap_factor;
    int hop_size;
    double* current_window;
    double* freq;

    /* fftw plans for forward and inverse transform */
    fftw_plan forward;
    fftw_plan inverse;

    /* sndfile variables */
    SF_INFO info;

    /* array for moduli (magnitudes), and phases */
    double *moduli_idx;
    double *phases_idx;
    double *phases_ddx;
    double *syn_moduli;
    double *syn_phases_idx;
    double *syn_phases_ddx;
    double *phase_increment;
    double *peaks;
} PhaseVocoder;

void pv_init(PhaseVocoder *pv);
void pv_cleanup(PhaseVocoder *pv);
void pv_read_audio_file(PhaseVocoder *pv, char *path);
void pv_fft_routine(PhaseVocoder *pv);
void locate_peaks(PhaseVocoder *pv, double *magnitudes, int size);
void pv_write_output(PhaseVocoder *pv, char *output_path, int hack);

#endif // PHASEVOCODER_H
