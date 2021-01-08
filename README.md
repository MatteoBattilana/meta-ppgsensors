# OSES ASSIGNMENT ON EMBEDDED LINUX - Heart Rate Monitor
The goal of this assignment is to implement a heart rate monitor, that is composed of two components.
* A Linux character-based driver (cDD) used to access a "virtual" Photopletismography (PPG) sensor
* A Linux user application (APP)

For this project I used a Raspberry Pi 2 B.


## Steps to test the two recipes
Assuming you have an already build a Linux distribution for a Raspberry Pi 2, so you have a container folder for the build into the poky directory called `build_rpi2` and a layer called `meta-example`, you have to perform the following steps:

* Clone the repository and copy only the necessary folders
```
cd meta-example/recipes-example/
git clone https://github.com/MatteoBattilana/OSESAssignment2.git
cp -r OSESAssignment2/app OSESAssignment2/ppgmod .
rm -rf OSESAssignment2
```
* Move to the build folder, with the command `cd ../../build_rpi2`
* At this point you need to add the application and the kernel module to the configuration of the Linux distribution; you have to add these lines at the end of the `conf/local.conf` file:
    ```
    IMAGE_INSTALL_append = " app"
    IMAGE_INSTALL_append = " ppgmod"
    KERNEL_MODULE_AUTOLOAD += "ppgmod"
    ```
* You have now to add the layer `meta-example` to the Linux distribution, add it to the `conf/bblayers.conf` file under the `BBLAYERS` parameter. The following section, shows an example for Raspberry Pi 2 B:
    ```
    BBLAYERS ?= " \
      /opt/poky/meta \
      /opt/poky/meta-poky \
      /opt/poky/meta-yocto-bsp \
      /opt/poky/meta-openembedded/meta-oe \
      /opt/poky/meta-openembedded/meta-multimedia \
      /opt/poky/meta-openembedded/meta-networking \
      /opt/poky/meta-openembedded/meta-python \
      /opt/poky/meta-raspberrypi \
      /opt/poky/meta-example \
      "
    ```
* You should now go back in the poky main folder and setup the build environment
```
cd ..
source oe-init-build-env build_rpi2
```
* At this point you can build the Linux distribution with `bitbake core-image-full-cmdline`

 
The built image can be flashed in a SDCard and the app can be tested using the `app` command from the terminal of the Raspberry.

## Driver - pppgmod
The driver has been implemented as a Linux kernel module that is enabled at the startup. The value, that comes from a  photopletismography (PPG), is simulated. At every read, the module returns a value from a predefined set, `data.h`. The `copy_to_user` function has been used in order to copy the integer sensor value from the kernel space to the user space.
Since multiple instance of the app can read the driver, the access to the structure has been managed via a mutex lock.


## App - app
> The app performs an endless loop, where it samples the PPG sensor, and every 2048 acquired samples it performs a 2048-points FFT, it computes the Power Spectral Density (PSD), it identifies the base frequency of the signal (the frequency where the PSD is maximum), which is the heart rate in Hz, and it prints the heart rate in beat-per-minutes.


#### Memory optimization
The application is based on one additional thread and a `pipe` that exploits inter process communication.
The first problem to solve was to reduce the memory utilization as much as possible, this can be done using a pipe and a single array; the array is not directly filled by the value read from the sensor, but the reading thread sends the value to the pipe. Once the thread reads the value from the pipe, it puts it into the array, once all 2048 values have been received, the heartbeat is computed using the FFT. This avoids the fact that the reading thread is not slowered by the computational time needed by the FFT. Using a blocking pipe, the read is blocking and waits for a value; at the same time, if the read is not called, the values written by the reading thread are buffered by the operating system. So, even if the reading thread reads always at a fixed rate, the other thread that computes the heart beat, during the FFT computation, can not read the value from the pipe. Once it finishes, it will read all values buffered by the OS from the pipe and will be in sync again. 

| text | data |  bss  |  dec  |  hex |
|:----:|:----:|:-----:|:-----:|:----:|
| 4869 | 444  | 16444 | 21757 | 54fd |
#### Timing
Since the system has to achieve a sampling frequency of 50Hz, the interval between two reads is 20 ms. I decided to use an alarm with the `SIGALRM` signal that is triggered every 20 ms.
Even if the time for writing to the pipeline can be neglected, using an alarm ensures precision on a call method; with a delay I should have to remove from the following sleep the time needed for the read and the write to the pipe. This could have been simply solved by performing a check on the current time: if the previous execution was done more than 20 ms ago, a new one is performed. This solution keeps the CPU at an high usage.

Using an almar, simplifies the time management and the usage of the CPU is very low. On a Raspberry Pi 2 B, the CPU usage is 0.1%.




