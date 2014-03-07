Rubato
======

Rubato is a music application that aims to help musicians learn more about their favorite songs. It uses a custom modified version of the Constant-q transform, where the input signal is first multiplied by a sparse "temporal kernel", and then fed into an Fast Fourier Transform (FFT).  This is used to extract pitches from the song. A separate realtime FFT algorithm is used to change the speed of the song without changing the pitch.  The usual corrections are made to reduce phasiness and also to detect and preserve beats.

======

![alt text](https://raw.github.com/chad-russell/Rubato/master/Rubato.png)

The main window shows a plot with time on the horizontal axis and frequency on the vertical axis.  It is possible to click anywhere in the main window and place midi notes, which will be output through a JACK audio port.  This makes it very easy to transcribe your favorite songs, record them into your favorite music program (Rosegarden, Ardour, etc.) and create sheet music.
