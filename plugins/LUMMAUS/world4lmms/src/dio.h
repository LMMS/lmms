#ifndef _H_DIO_
#define _H_DIO_

// F0推定法 DIO : Distributed Inline-filter Operation
void dio(double *x, int xLen, int fs, double framePeriod, double *timeAxis, double *f0);
int getSamplesForDIO(int fs, int xLen, double framePeriod);

#endif