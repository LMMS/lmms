==============================================================================

        hiir
        Version 1.32

        An oversampling and Hilbert transform library in C++

        By Laurent de Soras, 2005-2020
        Some AVX and SSE2 code by Dario Mambro, 2019

==============================================================================



Contents:

1. Legal
2. What is hiir?
3. Using hiir
4. Compilation
5. Oversampling to higher ratios
6. History
7. Contact



1. Legal
--------

Check the file license.txt to get full information about the license.



2. What is hiir?
----------------

hiir is a DSP (digital signal processing) library in C++, with two purposes:

    – Changing the sampling rate of a signal by a factor two, in both
directions (upsampling and downsampling).
    – Obtaining two signals with a pi/2 phase difference (Hilbert transform)

These distinct operations are actually sharing the same filter design method
and processing kernel, that is why they are included in the same package. The
filter (a two-path polyphase IIR) is very efficient and can be used to achieve
high-quality oversampling or phase shift at a low CPU cost. It is made of a
symetric half-band elliptic low-pass filter, hence having an extremely flat
frequency response in the passband.

Various implementations are supplied with the library, using special CPU
instructions to optimise the calculation speed. Currently 3DNow!, SSE, SSE2,
AVX and NEON instruction sets are supported, as well as the classic and portable
FPU implementation.

Source code may be downloaded from this webpage:
http://ldesoras.free.fr/prod.html#src_hiir



3. Using hiir
-------------

Your compiler must support C++11 or higher. However the library does not use
C++11 features extensively, so the code could be easily changed to support
older C++ versions if this is a strong requirement.

To avoid any collision, names have been encapsulated in the namespace “hiir”.
So you have to prefix every name of this library with hiir:: or put a line
“using namespace hiir;” into your code. Includes are expecting to find headers
in a “hiir” directory; make sure this location is available from your compiler. 

The filter design class is PolyphaseIir2Designer. It generates coefficients
for the filters from a specification: stopband attenuation, transition
bandwidth, number of coefficients and/or group delay.

The main processing classes are Downsampler2x*, Upsampler2x* and PhaseHalfPi*.
The suffix indicates the implementation. Choose “Fpu” if you are not sure
about the right one to use. All implementations of a class category have the
same function syntax, so you can use them easily with C++ templates.

The implementations should have a consistent behaviour, based on the FPU one.
Some of them have specific requirement, like object alignment in memory or
delay in processing. See the header file (.h) of the class for details about
the constraints and inconsistencies, and the code file (.cpp/.hpp) for details
about function calls.

The *SseOld filters uses an old SSE implementation which may be sligthly faster
than the standard SSE filters on ancient hardware.

Similarly, the *NeonOld filters use a different processing scheme and may be
faster in some cases, mostly with a high numer of coefficients. You will have
to check the performances on your target architecture with the test application.
For some reason, with GCC (not Clang), the NEON versions are slow as hell on
32-bit ARM. With full optimisation, the FPU versions are faster in almost all
cases (tested on Cortex A53 and A72 processors).

Note that all SIMD versions (except 3DNow!) require the processing objects to
be aligned on a 16-, 32- or 64-byte boundary. Now this should be automatically
taken care of by the compiler anyway, excepted if you use “placement new” for
creation. Processed data haven’t to be aligned.

As you can see, almost all classes are templates based on the number of
coefficients. This means it is not possible to change this number at run-time.
This is the most important constraint of this library. However the reward is
the high speed of the execution. Anyway, you could build a wrapper to support
variable number of coefficients, althought it means that you will have
probably to compile a large number of variations on the same code.

The library processes only 32-bit and 64-bit floating point data.

hiir is intended to be portable, but has some architecture-dependant pieces of
code. So far, it has been built and tested on:

    – MS Windows (x86, x64) / MS Visual C++ 2019 (FPU/SSEx/AVX/3DNow)
    – MS Windows (x86, x64) / GCC 9.2.0 (FPU/SSEx/AVX only) on MSYS 2
    – MS Windows (x86, x64) / Clang 10.0.1 (FPU/SSEx/AVX only) on MSYS 2
    – MacOS 10.5 (x86, x64) / GCC 4 (FPU/SSE only, old HIIR version)
    – Linux (ARM32, ARM64) / GCC 8.3.0 (FPU/NEON)
    – Linux (ARM32, ARM64) / Clang 10.0.1 (FPU/NEON)

If you happen to have another system and tweak it to make it run successfully,
pleeeeease send me your modification so I can include it to the main
distribution. Run main.cpp in Debug mode before, then in Release mode, in
order to be sure that everything is fine. I would also be glad to include
implementations for other processors/compilers.

References for filter use and design:

    – Scott Wardle, “A Hilbert-Transformer Frequency Shifter for Audio”
http://www.iua.upf.es/dafx98/papers/

    – Valenzuela and Constantinides, “Digital Signal Processing Schemes for
Efficient Interpolation and Decimation”, IEEE Proceedings, Dec 1983

    – Artur Krukowski, Izzet Kale, “The design of arbitrary-band multi-path
polyphase IIR filters”, ISCAS 2, page 741-744. IEEE, 2001

Example
¨¨¨¨¨¨¨

Typical use for a 8-coefficient downsampler:

#include "hiir/PolyphaseIir2Designer.h"
#include "hiir/Downsampler2xFpu.h"

    ...

// Coefficient calculation
constexpr int nbr_coef = 8;
double coefs [nbr_coef];
hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw (
    coefs, nbr_coef, 0.04
);

    ...

// Filter creation and initialisation
hiir::Downsampler2xFpu <nbr_coef> dspl;
dspl.set_coefs (coefs);

    ...

// Filter run
dspl.process_block (dst_ptr, src_ptr, nbr_spl);



4. Compilation and testing
--------------------------

Drop the following files into your project or makefile:

hiir/def.h
hiir/Downsampler2x*.*
hiir/fnc*.*
hiir/PhaseHalfPi*.*
hiir/PolyphaseIir2Designer.*
hiir/Stage*.*
hiir/Upsampler2x*.*

Other files (in the hiir/test directory) are for testing purpose only, do not
include them if you just need to use the library; they are not needed to use
hiir in your own programs.

hiir may be compiled in two versions: release and debug. Debug version
has checks that could slow down the code. Define NDEBUG to set the Release
mode. For example, the command line to compile the test bench on GCC or
Clang for an x86 hardware would look like:

Debug mode:
g++ -std=c++11 -mavx -I. -o ./hiir_debug.exe -g3 -O0 hiir/*.cpp hiir/test/*.cpp
clang++ -D_X86_ -std=c++11 -mavx512f -I. -o ./hiir_debug.exe -g3 -O0 hiir/*.cpp hiir/test/*.cpp

Release mode:
g++ -std=c++11 -mavx -I. -o ./hiir_release.exe -DNDEBUG -O3 hiir/*.cpp hiir/test/*.cpp
clang++ -D_X86_ -std=c++11 -mavx512f -I. -o ./hiir_release.exe -DNDEBUG -O3 hiir/*.cpp hiir/test/*.cpp

The “-mavx512f” option enables the compilation of the Intel intrinsics. If you
want to compile for other instruction sets, use “-mavx”, “-msse2”, “-msse” or
“-m3dnow”.

Note: be careful, when AVX is used, SSEx instructions may be replaced with
their AVX equivalent. In your own code, make sure each compilation unit uses
only the flags it requires if you want to support multiple architectures within
the same executable file.

Another two quirks with AVX versions compiled with GCC on 64-bit MinGW/Cygwin:

    – With standard AVX, there might be segmentation faults resulting from
aligned access to unaligned variables on the stack. The issue is known but not
solved yet. A workaround is to use the always_inline function attribute, but
there is not guarantee it will always work.
Ref: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54412

    – AVX-512 may produce assembler errors. Use the following as a workaround:
“-fno-asynchronous-unwind-tables -fno-exception”. This issue should be fixed
on the most recent compiler versions.
Ref: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65782

The included test bench checks roughly the accuracy of the filters. It also
tests the speed of every available function. Therefore, implementing a new
instruction set should be facilitated.

If you want to compile and run the test bench, please first edit the
test/conf.h file, in order to select the instruction sets available for your
CPU using #define/#undef. There is now an automatic detection, but the the
whole program compiled with all the available instruction sets could not even
start on a platform which doesn’t support them all. With the marcros you can
manually disable sets you can’t or don’t want to test.

In the same file, you have also testing options. You can save on the disk all
the samples generated during tests in order to check them in an audio editor.
However the files may take a lot of space on the disk, so it is recommended
to disable this option if it is not required. The “long tests” options are
intended to provide extensive checks on various filter sizes. It takes longer
to compile, but is safer if you want to change anything in the lib.



5. Oversampling to higher ratios
--------------------------------

It is possible to oversample a signal at a higher ratio than 2. You just have
to cascade up- or down-samplers to achieve a power-of-2 ratio. Depending on
your requirements, you can reduce the filter order as the sampling rate is
getting bigger by reducing the transition bandwidth (TBW).

For example, let’s suppose one wants 16x downsampling, with 96 dB of stopband
attenuation and a 0.49*Fs passband. You’ll need the following specifications
for the TBW of each stage:

 2x <-> 1x: TBW = (0.50-0.49)                      = 0.01
 4x <-> 2x: TBW = (0.50-0.49)/2 + 1/4              = 0.255
 8x <-> 4x: TBW = (0.50-0.49)/4 + 1/8  + 1/4       = 0.3775
16x <-> 8x: TBW = (0.50-0.49)/8 + 1/16 + 1/8 + 1/4 = 0.43865

The reason is that you do not need to preserve spectrum parts that will be
wiped out by subsequent stages. Only the spectrum part present after the
final stage has to be perserved.

More generally:

TBW[stage] = (TBW[stage-1] + 0.5) / 2
or
TBW[stage] = TBW[0] * (0.5^stage) + 0.5 * (1 - 0.5^stage)

So transition bandwidth requirement is significantely low until the last
stage (0). Thus, the optimal performance would be reached by using hiir
downsampler for the last stage because the requirement on the transition
bandwidth is important, and by using a classic FIR filtering for other
stages. Of course, it’s possible to use hiir at every stage, but a well-
optimised polyphase FIR routine is probably more efficient than a 1- or 2-
coefficent IIR downsampler. Indeed, these recursive filter implementations
have little or no benefit for low-order filters, whereas small FIR filters
can benefit from SIMD. Check the speed test results to make your mind.

Note that when cascading filters, the residual aliasing will accumulate in the
passband, so the global stopband attenuation is probably a bit lower than the
attenuation of each (or the weakest) stage.

These filters have a quite flat group delay from DC up to about Fs/20, then
the group delay starts rising faster and faster up to the cutoff frequency.
The oversampling.txt annex file lists a few examples of filter combinations
for oversampling, with a specific requirement in mind: the total group delay
of the upsampling filter combined with the downsampling filter should be
integer.



6. History
----------

v1.32 (2020-10-10)
    - Slight speed improvement for the FPU implementations
    - Standard and easier object alignment with the help of alignas()
    - Replacement of the internal enum constants with static constexpr int

v1.31 (2020-07-15)
    – Added a function to design a filter based on a group delay constraint
    – Fixed a bug in the global group delay calculation

v1.30 (2020-03-13)
    – Support for 64-bit float (double)
    – AVX and SSE2 processing, thanks to Dario Mambro
    – AVX512 processing
    – Added parallel processing for PhaseHalfPi* classes
    – Speedup for the SSE and NEON 1-channel up- and down-samplers
    – Improved tests

v1.20 (2019-06-23)
    – HIIR requires now C++11. Old compiler versions have not been tested.
    – Added support for Arm NEON instruction set
    – Added up/down-sampling 4-channel SIMD operations for SSE and NEON
    – Added functions to compute the filter phase and group delays
    – Fixed a coefficient numbering inconsistency in a comment in
      PolyphaseIir2Designer.h

v1.11 (2012-06-26)
    – Changed the license to the WTFPL
    – Fixed some compilation warnings

v1.10 (2008-05-28)
    – Changed directory structure
    – Test code is now in its own namespace (hiir::test)
    – Uses intrinsics for SSE code, making the code compilable on GCC.

v1.00 (2005-03-29)
    – Initial release



7. Contact
----------

Please address any comment, bug report or flame to:

Laurent de Soras
http://ldesoras.free.fr

