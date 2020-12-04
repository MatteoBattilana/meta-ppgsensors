# OSES ASSIGNMENT ON EMBEDDED LINUX - Heart Rate Monitor



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
