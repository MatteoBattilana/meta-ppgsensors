# OSES ASSIGNMENT ON EMBEDDED LINUX - Heart Rate Monitor
The goal of this assingment is to implement a heart rate monitor, that is composed by two components.
* A Linux character-based driver (cDD) used to access a "virtual" Photopletismography (PPG) sensor
* A Linux user application (APP)

For this project I used a Raspberry PI 2 B+.

## Driver - pppgmod.c
> The driver has been implemented as a Linux kernel module that is enabled at the startup. The value, that comes from a  photopletismography (PPG), is simulated. At every read, the module returns a value from a predefined set, `data.h`.
Since this is a character-based driver, the application car read only chars via a buffer; so the value is split among multiple characters, using a shift right of 8 positions. The first char will contains the 8 less significants bits of the binary representation of the simulated value.

#### Memory optimization
The application is based on one additional thread and a `pipe` that exploits inter process communication.
The first problem to solve was about to reduce the memory utilization as much as possible, this can be done using a pipe and a single array; the array is not directly filled by the value read from the sensor, but the reading thread sends the value to the pipe. Once the thread read the value from the pipe, it puts it into the array, once all 2048 values has been received, the heartbeat is computed using the FFT. This avoids that the reading thread is not slowered by the computational time needed by the FFT. Using a blocking pipe, the read is blocking and waits for a value; at the same time, if the read is not called, the values written by the reading thread are buffered by the operating system. So, even if the reading thread reads always at a fixed rate, the other thread that computes the heart beat, during the FFT computation, can not read the value from the pipe. Once it finishes, it will read all values buffered by the OS from the pipe and will be in sync again. 

#### Timing
Since the system has to achieve a sampling frequency of 50Hz, the interval between two reads is 20 ms. I decided to use 


## App - app
The app performs an endless loop, where it samples the PPG sensor, and every 2048 acquired samples it performs a 2048-points FFT, it computes the Power Spectral Density (PSD), it identifies the base frequency of the signal (the frequency where the PSD is maximum), which is the heart rate in Hz, and it prints the heart rate in beat-per-minutes.

## Steps
- cd poky
- source oe-init-build-env build_qemuarm
- bitbake-layers create-layer ../meta-sensor
- bitbake-layers add-layer ../meta-sensor/
- cd ../meta-sensor/recipes-example
- mkdir photopletismography
- cd photopletismography
- edit conf/local.conf and add :
	IMAGE_INSTALL_append = " photopletismography"
	KERNEL_MODULE_AUTOLOAD += "photopletismography"

- bitbake core-image-minimal
