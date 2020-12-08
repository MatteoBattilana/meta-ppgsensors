/*
 * setitimer.c - simple use of the interval timer
 */

#include <sys/time.h>		/* for setitimer */
#include <unistd.h>		/* for pause */
#include <signal.h>		/* for signal */

#define INTERVAL 20		/* number of milliseconds to go off */

/* function prototype */
void DoStuff(void);
struct timeval  tv1, tv2;
  struct itimerval it_val;	/* for setting itimer */
int main(int argc, char *argv[]) {

gettimeofday(&tv1, NULL);
  struct itimerval it_val;	/* for setting itimer */

  /* Upon SIGALRM, call DoStuff().
   * Set interval timer.  We want frequency in ms, 
   * but the setitimer call needs seconds and useconds. */
  if (signal(SIGALRM, (void (*)(int)) DoStuff) == SIG_ERR) {
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

  while (1) 
    pause();

}

/*
 * DoStuff
 */
void DoStuff(void) {
	gettimeofday(&tv2, NULL);
	printf("E = %f seconds\n",
		 (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
		 (double) (tv2.tv_sec - tv1.tv_sec));
	gettimeofday(&tv1, NULL);
 }
