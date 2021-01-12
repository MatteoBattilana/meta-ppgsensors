/*
 * Author: Matteo Battilana
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#ifndef PI
# define PI	3.14159265358979323846264338327950288
#endif
#include <fcntl.h>

#define INTERVAL 20							// number of milliseconds to trigger the read
#define q	11		    						// for 2^11 points
#define N	(1<<q)							// N-point FFT, iFFT
#define DEVNAME "/dev/ppgmod_dev"			// name of file

typedef float real;
typedef struct {
	real Re;
	real Im;
} complex;
static complex v[N];

static int pipefd[2];							// pipe for the thread
struct itimerval it_val; 						// structure for setting itimer
static int idx = 0;							// array index
static int fd = -1;							// file descriptor
pthread_t thread_id;

void fft(complex *v, int n, complex *tmp) {
	if (n > 1) {							// otherwise, do nothing and return
		int k, m;
		complex z, w, *vo, *ve;
		ve = tmp;
		vo = tmp + n / 2;
		for (k = 0; k < n / 2; k++) {
			ve[k] = v[2 * k];
			vo[k] = v[2 * k + 1];
		}
		fft(ve, n / 2, v); 					// FFT on even-idxed elements of v[]
		fft(vo, n / 2, v); 					// FFT on odd-idxed elements of v[]
		for (m = 0; m < n / 2; m++) {
			w.Re = cos(2 * PI * m / (double) n);
			w.Im = -sin(2 * PI * m / (double) n);
			z.Re = w.Re * vo[m].Re - w.Im * vo[m].Im; // Re(w*vo[m])
			z.Im = w.Re * vo[m].Im + w.Im * vo[m].Re; // Im(w*vo[m])
			v[m].Re = ve[m].Re + z.Re;
			v[m].Im = ve[m].Im + z.Im;
			v[m + n / 2].Re = ve[m].Re - z.Re;
			v[m + n / 2].Im = ve[m].Im - z.Im;
		}
	}
	return;
}


/*
 * Method used to compute the FFT
 * */
int computeFft(complex v[N]) {
	complex scratch[N];
	float abs[N];
	int k;
	int m;
	int i;
	int minIdx, maxIdx;
	// FFT computation
	fft(v, N, scratch);

	// PSD computation
	for (k = 0; k < N; k++) {
		abs[k] = (50.0 / 2048) * ((v[k].Re * v[k].Re) + (v[k].Im * v[k].Im));
	}

	minIdx = (0.5 * 2048) / 50; 	// position in the PSD of the spectral line corresponding to 30 bpm
	maxIdx = 3 * 2048 / 50; 		// position in the PSD of the spectral line corresponding to 180 bpm

	// Find the peak in the PSD from 30 bpm to 180 bpm
	m = minIdx;
	for (k = minIdx; k < (maxIdx); k++) {
		if (abs[k] > abs[m])
			m = k;
	}
	// Print the heart beat in bpm
	return (m) * 60 * 50 / 2048;
}

/*
 * Scheduled method that is called every 20ms, using the SIGALRM signal.
 * The values are written into the pipe, in order to be read by the thread.
 * */
void sampleValue(void) {
	int value;
	int read_count = read(fd, &value, 4);					// sample the value
	if(read_count > 0){								// check if read a value
		if (write(pipefd[1], &value, sizeof(value)) == -1) {	// write to the pipe
			fprintf(stderr, "[ERROR] Unable to write to thread pipe: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
}

/*
 * Thread for reading the sensor value coming from the scheduled method.
 * The read is performed using a blocking pipe, this solves the problem about
 * the concurrent manipulation of the array that contains the sample value.
 * The pipe brigs another advantage: if the computeFft method require more time,
 * the value sent over the pipe are automatically buffered by the system until
 * the next read. This let the application to perform a continuous sampling.
 * */
void* valueHandlerThread(void* p) {
	while (1) {
		int n;
		if (read(pipefd[0], &n, sizeof(n)) == -1) {		// cannot read from the pipe
			fprintf(stderr, "[ERROR] Unable to read from thread pipe: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		} else {
			// read next element
			v[idx].Re = n;
			v[idx++].Im = 0;
			if (idx == N) {
				// computer FFT
				int res = computeFft(v);
				printf("[INFO] BPM: %d\n", res);
				idx = 0;
			}
		}
	}
	return NULL;
}

/**
 * Method that closes the file descriptor and stops the thread
 */
void cleanBeforeExit() {
	if (fd != -1)
		close(fd);
	pthread_cancel(thread_id);
}

/*
 * Method used to handle the Ctrl-c during the read.
 * It call a method that closes the file descriptor and the stops the thread
 * */
void ctrlHandler(int sig) {
	signal(sig, SIG_IGN);
	printf("\r[INFO] Exiting\n");
	cleanBeforeExit();
	exit(0);
}

int main(int argc, char **argv) {
	printf("[INFO] Starting reading\n");

	// Setup Ctrl-C handler
	signal(SIGINT, ctrlHandler);

	// Setup pipe for communication
	if (pipe(pipefd) < 0) {
		fprintf(stderr, "[ERROR] Unable to open pipe: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Start a new thread for BPM
	if(pthread_create(&thread_id, NULL, valueHandlerThread, NULL) != 0) {
		fprintf(stderr, "[ERROR] Unable to start the thread: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Open file
	if ((fd = open(DEVNAME, O_RDWR)) < 0) {
		fprintf(stderr, "[ERROR] Unable to open %s: %s\n", DEVNAME, strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Schedule alarm every 20 ms
	if (signal(SIGALRM, (void (*)(int)) sampleValue) == SIG_ERR) {
		fprintf(stderr, "[ERROR] Unable to catch SIGALRM: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	it_val.it_value.tv_sec = INTERVAL / 1000;
	it_val.it_value.tv_usec = (INTERVAL * 1000) % 1000000;
	it_val.it_interval = it_val.it_value;
	if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
		fprintf(stderr, "[ERROR] Error calling setitimer()");
		exit(EXIT_FAILURE);
	}

	printf("[INFO] Read started\n");
	while (1)
		pause();

	return 0;
}

