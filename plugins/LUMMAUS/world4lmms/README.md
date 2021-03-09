# World4UTAU

*This branch is based on world4utau in http://utau2008.xrea.jp/mp3/engine_hikaku.html. The initial code in this repo is from https://github.com/Luk3M/world4utau which is not maintained for a long time.*

It's ported to Linux / Mac including lots of optimizations, it's compatiable with OpenUATU and can be called by C#. So if you wanna use OpenUATU on Mac/Linux, this is your *only* option as far as I see.

# Build
Currently support Ubunt and MacOS. To use windows version, please refer the original link.
Later, all platforms will be supported.

## Ubuntu
Install dependence.

`sudo apt install git gcc texinfo`

Build `fftw`. 
*fftw* is a C subroutine library for computing the discrete Fourier transform (DFT) in one or more dimensions, of arbitrary input size, and of both real and complex data (as well as of even/odd data, i.e. the discrete cosine/sine transforms or DCT/DST). [http://www.fftw.org](http://www.fftw.org)

In this repo, we include fftw-3.3.8 source code for convienent. You can download it from official site by yourself.
```
cd fftw-3.3.8
./configure
make
make install
```

Build `world4utau`.
```
cd world4utau
make build
```

## MacOS

Suppose you already have XCode tools installed for C program.

Build `fftw`. 
*fftw* is a C subroutine library for computing the discrete Fourier transform (DFT) in one or more dimensions, of arbitrary input size, and of both real and complex data (as well as of even/odd data, i.e. the discrete cosine/sine transforms or DCT/DST). [http://www.fftw.org](http://www.fftw.org)

In this repo, we include fftw-3.3.8 source code for convienent. You can download it from official site by yourself.
```
cd fftw-3.3.8
./configure
make
make install
```

Build `world4utau`.
```
cd world4utau
make build
```

# Performance
![](https://github.com/xrdavies/world4utau/blob/master/docs/valgrind.png?raw=true)