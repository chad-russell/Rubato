# Rubato
A simple time-stretching FFT implementation

See [here](https://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&cad=rja&uact=8&ved=0ahUKEwii-eSe6bPKAhXCPD4KHZLQC5QQFgghMAA&url=http%3A%2F%2Flabrosa.ee.columbia.edu%2F~dpwe%2Fpapers%2FLaroD99-pvoc.pdf&usg=AFQjCNH5qJ55D_EUpLanLnWTzKTHzSdLmw) for the original paper that inspired this technique, and [here](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.18.6415&rep=rep1&type=pdf) and [here](https://www.ee.columbia.edu/~dpwe/papers/LaroD97-phasiness.pdf) for more ideas.

Right now this implements the simplest phase locking that can be done (outlined in the first link), but does not use 
a varying frame rate as is most common.  Instead it uses the same constant frame rate for the analysis and synthesis frames,
as outlined in the second link.  This means that you end up duplicating frames or omitting frames depending on whether
you are making things faster or slower.  It also means that it should be easier to both detect fast changes and skip them
to prevent stretching percussive noises, and do things like change the timestretch ratio while the program is running.  
One day I will get around to doing these things.
