/*
 * Copyright (C) Your copyright.
 *
 * Author: Matteo
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the PG_ORGANIZATION nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY	THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS-IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>		/* for setitimer */
#include <unistd.h>		/* for pause */
#include <signal.h>		/* for signal */

#define INTERVAL 20		/* number of milliseconds to go off */

#define q	11		    /* for 2^11 points */
#define N	(1<<q)		/* N-point FFT, iFFT */

typedef float real;
typedef struct{real Re; real Im;} complex;

#ifndef PI
# define PI	3.14159265358979323846264338327950288
#endif
#include <fcntl.h>

struct timeval  tv1, tv2;
  struct itimerval it_val;	/* for setting itimer */

void sampleValue(void);
void fft( complex *v, int n, complex *tmp )
{
  if(n>1) {			/* otherwise, do nothing and return */
    int k,m;    complex z, w, *vo, *ve;
    ve = tmp; vo = tmp+n/2;
    for(k=0; k<n/2; k++) {
      ve[k] = v[2*k];
      vo[k] = v[2*k+1];
    }
    fft( ve, n/2, v );		/* FFT on even-idxed elements of v[] */
    fft( vo, n/2, v );		/* FFT on odd-idxed elements of v[] */
    for(m=0; m<n/2; m++) {
      w.Re = cos(2*PI*m/(double)n);
      w.Im = -sin(2*PI*m/(double)n);
      z.Re = w.Re*vo[m].Re - w.Im*vo[m].Im;	/* Re(w*vo[m]) */
      z.Im = w.Re*vo[m].Im + w.Im*vo[m].Re;	/* Im(w*vo[m]) */
      v[  m  ].Re = ve[m].Re + z.Re;
      v[  m  ].Im = ve[m].Im + z.Im;
      v[m+n/2].Re = ve[m].Re - z.Re;
      v[m+n/2].Im = ve[m].Im - z.Im;
    }
  }
  return;
}

int computeFft(complex v[N]){

	  complex scratch[N];
	  float abs[N];
	  int k;
	  int m;
	  int i;
	  int minIdx, maxIdx;
	// FFT computation
	  fft( v, N, scratch );

	// PSD computation
	  for(k=0; k<N; k++) {
		abs[k] = (50.0/2048)*((v[k].Re*v[k].Re)+(v[k].Im*v[k].Im));
	  }

	  minIdx = (0.5*2048)/50;   // position in the PSD of the spectral line corresponding to 30 bpm
	  maxIdx = 3*2048/50;       // position in the PSD of the spectral line corresponding to 180 bpm

	// Find the peak in the PSD from 30 bpm to 180 bpm
	  m = minIdx;
	  for(k=minIdx; k<(maxIdx); k++) {
	    if( abs[k] > abs[m] )
		m = k;
	  }

	// Print the heart beat in bpm
	  return (m)*60*50/2048;
}

int bufferToInt(char * buffer, int count)
{
	int value = 0;
	for(int i = 0; i < count; i++)
	{
		int v = (int) buffer[i];
		int h = v << (8 * i);
		value += h;
	}

	return value;
}


void setupTimer(void){
	  if (signal(SIGALRM, (void (*)(int)) sampleValue) == SIG_ERR) {
    perror("Unable to catch SIGALRM");
    exit(1);
  }
  it_val.it_value.tv_sec =     INTERVAL/1000;
  it_val.it_value.tv_usec =    (INTERVAL*1000) % 1000000;
  it_val.it_interval = it_val.it_value;
  if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
    perror("error calling setitimer()");
    exit(1);
  }

}

int fd = -1;
void openFile(void){
	char *dev_name = "/dev/photopletismography_dev";

	if ((fd = open(dev_name, O_RDWR)) < 0)
	{
		fprintf(stderr, "unable to open %s: %s\n", dev_name, strerror(errno));
	}

}

int idx = 0;
complex v[N];
void sampleValue(void){
  gettimeofday(&tv2, NULL);
  printf("E = %f seconds\n",
     (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
     (double) (tv2.tv_sec - tv1.tv_sec));
  gettimeofday(&tv1, NULL);

	char buffer[100];
	int read_count = read (fd, &buffer, 100);

	    v[idx].Re = bufferToInt(buffer, read_count);
	    v[++idx].Im = 0;

      printf("Read %d\n", idx);
	if (idx == N){
		// computer ft
		complex c[N];
		memcpy(&c, &v, sizeof(v));
		int ddd = computeFft(c);
		printf("######## %d\n",ddd);
		printf("2048\n");
		idx = 0;
	}
}

int main(int argc, char **argv)
{
  gettimeofday(&tv1, NULL);
	openFile();
	setupTimer();
	printf("Started 1\n");

  while (1)
    pause();


	// WRITE

		close( fd );

	return 0;
}
