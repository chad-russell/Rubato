#include <math.h>
#include <stdlib.h>
#include <string.h>

/* calculate hamming window for size */
double *hanning_matlab(int N)
{
    int half, i, idx, n;
    double *w;

    w = (double*) calloc(N, sizeof(double));
    memset(w, 0, N*sizeof(double));

		n = N-1;

    if(n%2==0)
    {
        half = n/2;
        for(i=0; i<half; i++)
				{
            w[i] = 0.5 * (1 - cos(2*M_PI*(i+1) / (n+1)));
				}

        idx = half-1;
        for(i=half; i<n; i++) 
				{
            w[i] = w[idx];
            idx--;
        }
    }
    else
    {
        half = (n+1)/2;
        for(i=0; i<half; i++)
				{
            w[i] = 0.5 * (1 - cos(2*M_PI*(i+1) / (n+1)));
				}

        idx = half-2;
        for(i=half; i<n; i++) 
				{
            w[i] = w[idx];
            idx--;
        }
    }

		for(i=N-1; i>=1; i--)
		{
			w[i] = w[i-1];
		}
		w[0] = 0.0;

    return w;
}
