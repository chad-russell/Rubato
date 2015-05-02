#ifndef PHASEVOCODER_H
#define PHASEVOCODER_H

#include <vector>
#include <cmath>
#include <fstream>
#include <stdio.h>
#include </usr/local/include/fftw3.h>
#include </usr/local/include/sndfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

std::vector<double> locate_peaks(double* magnitudes, int size);

class PhaseVocoder
{
public:
    explicit PhaseVocoder();
    ~PhaseVocoder();
    void open_audio_file(char *path);
    void close_audio_file();
    void fft_routine();
    void write_output(char* output_path);
    void set_timestretch_ratio(double);
    double get_timestretch_ratio();
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

private:
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
    double* moduli_idx;
    double* phases_idx;
    double* phases_ddx;
    double* syn_moduli;
    double* syn_phases_idx;
    double* syn_phases_ddx;
    double* phase_increment;
    std::vector<double> peaks;

};

#endif // PHASEVOCODER_H
