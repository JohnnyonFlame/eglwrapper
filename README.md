### What is in this repository?

This repo contains a wrapper for libIMGegl, providing libEGL functionality while fixing gaps in the behavior of the
included libEGL on the Actions S500 PowerVR blobs.

With this, you should be able to use the PowerVR blobs without X11, using either EGL directly (as seen in `eglconfig.c`)
or with a compatible [SDL fork](https://github.com/JohnnyonFlame/SDL-rg35xx).

This is inteded for usage with the Anbernic RG35xx, with any GPU-enabled CFW such as [Batocera Linux](https://github.com/rg35xx-cfw/batocera.linux).

### Dependencies

* [SGX544 PowerVR Blobs for the Actions S500](https://github.com/JohnnyonFlame/cubieboard6_ubuntu_gpu_drivers).
* A compatible cross-compiler toolchain, such as `crossbuild-essential-armhf` from Ubuntu 20.04's repository.
* `powervr.ini` file in either `/etc/powervr.ini` or your application's directory, and it must contain the following:

```ini
[default]
WindowSystem=libpvrPVR2D_FLIPWSEGL.so
```

### How to build it?

```bash
git checkout --recurse-submodules https://github.com/JohnnyonFlame/eglwrapper
cd eglwrapper
make PVR_PATH="blobs/v2" CROSS="arm-linux-gnueabihf-" EGL_PATH=/usr/lib/arm-linux-gnueabihf/
```

### Acknowledgements:

Thanks @shauninman for the owlfb ioctls necessary for achieving VSync.
Thanks @acmeplus for the PowerVR blobs and Batocera Lite CFW.

### Licensing

Copyright © 2023 João H. All Rights Reserved.
This is free software. The source files in this repository are released under
the [GPLv2 License](LICENSE.md), see the license file for more information.
