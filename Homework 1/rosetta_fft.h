#ifndef __ROSETTA_FFT_H__
#define __ROSETTA_FFT_H__

#define PI (3.14159265359)

typedef double complex cplx;

void _fft(cplx buf[], cplx out[], int n, int step);
void fft(cplx buf[], int n);

#endif