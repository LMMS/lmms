#ifndef _H_WORLD_
#define _H_WORLD_

// 音声分析合成法 WORLD by M. Morise
//
// FFTWを使うので，別途インストールが必要です．
//

// 現状で分かっているバグ
// decimateForF0 : 開始直後・終了間際4サンプルくらいに誤差が入ります．

// 语音分析和合成方法WORLD，作者：M。Morise
//
// 由于它使用FFTW，因此您需要单独安装。
//

// 已知错误
// decimateForF0：大约在开始之后和结束之前有4个样本存在错误。

#include <stdlib.h>
#if defined(__WIN32__) || defined(_WIN32) || defined(_WINDOWS)
#include <windows.h>
#endif
#include <math.h>
#include "matlab.h"

//メモリリークチェック：
//* VS2008とかだと通らないかもしれないのでその場合はコメントにすること。

//内存泄漏检查：
// *如果是VS2008，则可能无法通过，因此在这种情况下请对其进行注释。

#ifdef WIN32
#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
//*/

#include "constant.h"
#include "dio.h"
#include "matlab.h"
#include "platinum.h"
#include "star.h"
#include "synthesis.h"
#include "wavread.h"
#include "common.h"
#include "timer.h"
#include "log.h"
#ifdef __cplusplus
extern "C"{
#endif
int World4UTAUMain(const char *inputFile,
                   const char *outputFile,
                   const char* note,
                   double consonantVelocity,
                   const char *flags,
                   double input_offset,
                   double scaledLength,
                   double consonantLength,
                   double cutoff,
                   double intensity,
                   double modulation,
                   double tempo,
                   const char *pitchbend);

#ifdef __cplusplus
}
#endif
#endif