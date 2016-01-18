#include "phasevocoder.h"

int main(int argc, char *argv[])
{
    PhaseVocoder pv;
		pv_init(&pv);

		printf("opening audio file...\n");
    pv_read_audio_file(&pv, "./input.wav");

    pv.timestretch_ratio = 1.8;

		printf("processing...\n");
    for(int i = 0; i < 1000; i++)
    {
			if (i % 100 == 0)
			{
				printf("%d\n", i);
			}

			pv_fft_routine(&pv);
			pv.an_idx += 1;
			pv.tempo_an_idx += 1;
    }

		printf("writing output...\n");
    pv_write_output(&pv, "/Users/chadrussell/Desktop/output.wav", 2000000);

		pv_cleanup(&pv);
		printf("done!\n");
}
