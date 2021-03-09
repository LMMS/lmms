#ifndef _H_MATLAB_
#define _H_MATLAB_

//-----------------------------------------------------------------------------
// m_std() calculates the standard deviation of the input vector.
//
// Input:
//   x          : Input vector
//   x_length   : Length of x
//
// Output:
//   Calculated standard deviation
//-----------------------------------------------------------------------------
double m_std(double *x, int x_length);


void m_inv(double **r, int n, double **invr);

//-----------------------------------------------------------------------------
// fast_fftfilt() carries out the convolution on the frequency domain.
//
// Input:
//   x                : Input signal
//   x_length         : Length of x
//   h                : Impulse response
//   h_length         : Length of h
//   fft_size         : Length of FFT
//   forward_real_fft : Struct to speed up the forward FFT
//   inverse_real_fft : Struct to speed up the inverse FFT
//
// Output:
//   y                : Calculated result.
//-----------------------------------------------------------------------------
void m_fftfilt(double *x, int xlen, double *h, int hlen, int fftl, double *y);

//-----------------------------------------------------------------------------
// randn() generates pseudorandom numbers based on xorshift method.
//
// Output:
//   A generated pseudorandom number
//-----------------------------------------------------------------------------
float m_randn(void);

//-----------------------------------------------------------------------------
// histc() counts the number of values in vector x that fall between the
// elements in the edges vector (which must contain monotonically
// nondecreasing values). n is a length(edges) vector containing these counts.
// No elements of x can be complex.
// http://www.mathworks.co.jp/help/techdoc/ref/histc.html
//
// Input:
//   x              : Input vector
//   x_length       : Length of x
//   edges          : Input matrix (1-dimension)
//   edges_length   : Length of edges
//
// Output:
//   index          : Result counted in vector x
// Caution:
//   Lengths of index and edges must be the same.
//-----------------------------------------------------------------------------
void m_histc(double *x, int x_length, double *y, int yLen, int *index);

//-----------------------------------------------------------------------------
// interp1() interpolates to find yi, the values of the underlying function Y
// at the points in the vector or array xi. x must be a vector.
// http://www.mathworks.co.jp/help/techdoc/ref/interp1.html
//
// Input:
//   x          : Input vector (Time axis)
//   y          : Values at x[n]
//   x_length   : Length of x (Length of y must be the same)
//   xi         : Required vector
//   xi_length  : Length of xi (Length of yi must be the same)
//
// Output:
//   yi         : Interpolated vector
//-----------------------------------------------------------------------------
void m_interp1(double *t, double *y, int iLen, double *t1, int oLen, double *y1);

//-----------------------------------------------------------------------------
// decimate() carries out down sampling by both IIR and FIR filters.
// Filter coeffiencts are based on FilterForDecimate().
//
// Input:
//   x          : Input signal
//   x_length   : Length of x
//   r          : Coefficient used for down sampling
//                (fs after down sampling is fs/r)
// Output:
//   y          : Output signal
//-----------------------------------------------------------------------------
long m_decimateForF0(const double *x, int x_length, double *y, int r);


//-----------------------------------------------------------------------------
// m_round() calculates rounding.
//
// Input:
//   x    : Input value
//
// Output:
//   y    : Rounded value
//-----------------------------------------------------------------------------
int m_round(double x);

//-----------------------------------------------------------------------------
// diff() calculates differences and approximate derivatives
// http://www.mathworks.co.jp/help/techdoc/ref/diff.html
//
// Input:
//   x          : Input signal
//   x_length   : Length of x
//
// Output:
//   y          : Output signal
//-----------------------------------------------------------------------------
void m_diff(double *x, int x_length, double *ans);


//-----------------------------------------------------------------------------
// interp1Q() is the special case of interp1().
// We can use this function, provided that All periods of x-axis is the same.
//
// Input:
//   x          : Origin of the x-axis
//   shift      : Period of the x-axis
//   y          : Values at x[n]
//   x_length   : Length of x (Length of y must be the same)
//   xi         : Required vector
//   xi_length  : Length of xi (Length of yi must be the same)
//
// Output:
//   yi         : Interpolated vector
//
// Caution:
//   Length of xi and yi must be the same.
//-----------------------------------------------------------------------------
void m_interp1Q(double x, double shift, double *y, int x_length, double *xi, int xi_length, double *ans);


//-----------------------------------------------------------------------------
// fftshift() swaps the left and right halves of input vector.
// http://www.mathworks.com/help/matlab/ref/fftshift.html
//
// Input:
//   x              : Input vector
//   x_length       : Length of x
//
// Output:
//   y              : Swapped vector x
//
// Caution:
//   Lengths of index and edges must be the same.
//-----------------------------------------------------------------------------
void fftshift(const double *x, int x_length, double *y);

#endif