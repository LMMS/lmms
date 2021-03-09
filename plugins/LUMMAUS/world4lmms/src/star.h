#ifndef _H_STAR_
#define _H_STAR_

// スペクトル包絡推定法 STAR : Synchronous Technique and Adroit Restoration
//频谱包络估计方法STAR：Synchronous Technique and Adroit Restoration
int getFFTLengthForStar(int fs);
void star(double *x, int xLen, int fs, double *timeAxis, double *f0,
          double **specgram);

#endif