#ifndef _H_CONSTANT_
#define _H_CONSTANT_

#define PI 3.1415926535897932384

// windowsならでは
// Windows特有的
#pragma warning( disable : 4996 )

#pragma comment(lib, "libfftw3-3.lib")
#pragma comment(lib, "libfftw3f-3.lib")
#pragma comment(lib, "libfftw3l-3.lib")
#define MAX_FFT_LENGTH 2048
#define FLOOR_F0 71.0
#define DEFAULT_F0 150.0
#define LOW_LIMIT 65.0
// 71は，fs: 44100においてFFT長を2048にできる下限．
// 70 Hzにすると4096点必要になる．
// DEFAULT_F0は，0.0.4での新機能．調整の余地はあるが，暫定的に決定する．

// 71是可以将FFT长度设置为fs的下限：2048：44100。
// 70 Hz需要4096点。
// DEFAULT_F0是0.0.4中的新功能。 有调整的空间，但是将暂时确定。

#endif