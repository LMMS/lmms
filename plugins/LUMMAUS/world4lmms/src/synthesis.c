#include <stdio.h> // for debug
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include "synthesis.h"
#include "matlab.h"
#include "constant.h"
#include "common.h"

// spectrum, cepstrumは毎回malloc, freeするのが面倒だから．
/*
int getOneFrameSegment(double *f0, int tLen, double **specgram, double **aperiodicity, int fftl, double framePeriod, double currentTime, int fs, double defaultF0,
						fftw_complex *spectrum, fftw_complex *cepstrum, 
						double *response, int xLen);
*/

void getMinimumPhaseSpectrum(double *inputSpec, fftw_complex *spectrum, fftw_complex *cepstrum, int fftl)
{
	int i;
	double real, imag;
	fftw_plan forwardFFT, inverseFFT;
	forwardFFT = fftw_plan_dft_1d(fftl, spectrum, cepstrum, FFTW_FORWARD, FFTW_ESTIMATE);
	inverseFFT = fftw_plan_dft_1d(fftl, cepstrum, spectrum, FFTW_BACKWARD, FFTW_ESTIMATE);

	// 値を取り出す
	for (i = 0; i <= fftl / 2; i++)
	{
		spectrum[i][0] = log(inputSpec[i] ? inputSpec[i] : 1.0e-20) / 2.0;
		spectrum[i][1] = 0.0;
	}
	for (; i < fftl; i++)
	{
		spectrum[i][0] = spectrum[fftl - i][0];
		spectrum[i][1] = 0.0;
	}
	fftw_execute(forwardFFT);
	for (i = 1; i < fftl / 2; i++)
	{
		cepstrum[i][0] = 0.0;
		cepstrum[i][1] = 0.0;
	}
	for (; i < fftl; i++)
	{
		cepstrum[i][0] *= 2.0;
		cepstrum[i][1] *= 2.0;
	}
	fftw_execute(inverseFFT);
	for (i = 0; i < fftl; i++)
	{
		real = exp(spectrum[i][0] / (double)fftl) * cos(spectrum[i][1] / (double)fftl);
		imag = exp(spectrum[i][0] / (double)fftl) * sin(spectrum[i][1] / (double)fftl);
		spectrum[i][0] = real;
		spectrum[i][1] = imag;
	}
}

// 特定時刻の応答を取得する．
void getOneFrameSegment(double *f0, int tLen, double **specgram, double **residualSpecgram, int fftl, double framePeriod, double currentTime, int fs, double defaultF0,
						fftw_complex *spectrum, fftw_complex *cepstrum,
						double *response, int xLen)
{
	int i;
	double real, imag, tmp;
	fftw_plan inverseFFT_RP; // FFTセット

	int currentFrame, currentPosition;

	inverseFFT_RP = fftw_plan_dft_c2r_1d(fftl, spectrum, response, FFTW_ESTIMATE);

	currentFrame = (int)(currentTime / (framePeriod / 1000.0) + 0.5);
	currentPosition = (int)(currentTime * (double)fs);

	tmp = currentTime + 1.0 / (f0[currentFrame] == 0.0 ? defaultF0 : f0[currentFrame]);

	// 値を取り出す
	getMinimumPhaseSpectrum(specgram[currentFrame], spectrum, cepstrum, fftl);

	spectrum[0][0] *= residualSpecgram[currentFrame][0];
	for (i = 1; i < fftl / 2; i++)
	{
		real = spectrum[i][0] * residualSpecgram[currentFrame][(i - 1) * 2 + 1] - spectrum[i][1] * residualSpecgram[currentFrame][i * 2];
		imag = spectrum[i][0] * residualSpecgram[currentFrame][i * 2] + spectrum[i][1] * residualSpecgram[currentFrame][(i - 1) * 2 + 1];
		spectrum[i][0] = real;
		spectrum[i][1] = imag;
	}
	spectrum[fftl / 2][0] *= residualSpecgram[currentFrame][fftl - 1];
	spectrum[fftl / 2][1] = 0;
	fftw_execute(inverseFFT_RP);

	fftw_destroy_plan(inverseFFT_RP);
}

void synthesis(double *f0, int tLen, double **specgram, double **residualSpecgram, int fftl, double framePeriod, int fs,
			   double *synthesisOut, int xLen)
{
	int i, j;
	double *impulseResponse;
	impulseResponse = (double *)malloc(sizeof(double) * fftl);
	fftw_complex *cepstrum, *spectrum; // ケプストラムとスペクトル
	cepstrum = (fftw_complex *)malloc(sizeof(fftw_complex) * fftl);
	spectrum = (fftw_complex *)malloc(sizeof(fftw_complex) * fftl);

	double currentTime = 0.0;
	int currentPosition = 0; //currentTime / framePeriod;
	int currentFrame = 0;
	for (i = 0;; i++)
	{
		for (j = 0; j < fftl; j++)
			impulseResponse[j] = 0.0; // 配列は毎回初期化

		getOneFrameSegment(f0, tLen, specgram, residualSpecgram, fftl, framePeriod, currentTime, fs, DEFAULT_F0,
						   spectrum, cepstrum, impulseResponse, xLen);

		currentPosition = (int)(currentTime * (double)fs);
		//		for(j = 0;j < fftl/2;j++)
		for (j = 0; j < 3 * fftl / 4; j++)
		{
			if (j + currentPosition >= xLen)
				break;
			synthesisOut[j + currentPosition] += impulseResponse[j];
		}

		// 更新
		currentTime += 1.0 / (f0[currentFrame] == 0.0 ? DEFAULT_F0 : f0[currentFrame]);
		currentFrame = (int)(currentTime / (framePeriod / 1000.0) + 0.5);
		currentPosition = (int)(currentTime * (double)fs);
		if (fftl / 8 + currentPosition >= xLen || currentFrame >= tLen)
			break;
	}

	free(cepstrum);
	free(spectrum);
	free(impulseResponse);
	return;
}
