

#include <fftw3.h>
#include <vector>
#include <cmath>
#include <QMutexLocker>
#include <atomic>

#include "FFTProcessor.h"
#include "fft_helpers.h"
#include "lmms_basics.h"
#include "LocklessRingBuffer.h"

// some parts of the code were copyed from SaProcessor.cpp
// credit: 2019 Martin Pavelek <he29/dot/HS/at/gmail/dot/com>

namespace lmms
{

FFTProcessor::FFTProcessor(unsigned int bufferSizeIn, bool isThreadedIn, FFTWindow FFTWindowIn)// :
	//m_reader(bufferSizeIn * 4)
{
	m_blockSize = bufferSizeIn;
	m_isThreaded = isThreadedIn;
	m_FFTWindowType = FFTWindowIn;

	m_terminate = true;
	m_frameFillLoc = 0;

	m_samplesIn.resize(m_blockSize);
	m_samplesOut.resize(m_blockSize);
	m_normSpectrum.resize(binCount(m_blockSize));
	//m_complexMultiplier;

	/*
	m_out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * binCount(m_blockSize));
	// fftwf_plan fftwf_plan_dft_r2c_1d(int n, float *in, fftwf_complex *out, unsigned flags);
	// fftwf_plan fftwf_plan_dft_c2r_1d(int n, fftwf_complex *in, float *out, unsigned flags);
	m_plan = fftwf_plan_dft_r2c_1d(m_blockSize, m_samplesIn.data(), m_out, FFTW_ESTIMATE);
	m_iplan = fftwf_plan_dft_c2r_1d(m_blockSize, m_out, m_samplesOut.data(), FFTW_ESTIMATE);
	*/

	//m_fftWindow.resize(m_blockSize, 1.0);
	//precomputeWindow(m_fftWindow.data(), m_blockSize, m_FFTWindowType);
}
FFTProcessor::~FFTProcessor()
{
	qDebug("FFTProcessor destrc -1");

	threadedTerminate();
	/*
	if (m_plan != nullptr)
	{
		fftwf_destroy_plan(m_plan);
	}
	if (m_iplan != nullptr)
	{
		fftwf_destroy_plan(m_iplan);
	}
	qDebug("FFTProcessor destrc 0");
	qDebug("FFTProcessor destrc 1");
	if (m_out != nullptr)
	{
		fftwf_free(m_out);
	}
	*/
	
	qDebug("FFTProcessor destrc 2");
	//m_out = nullptr;
	qDebug("FFTProcessor destrc 3");
}

void FFTProcessor::analyze(sampleFrame* sampleBufferIn, unsigned int sizeIn, unsigned int sampLocIn)
{
	if (m_isThreaded == false)
	{
		/*
		while (frameIn < frameCount && m_frameFillLoc < blockSizeIn)
		{
		//qDebug("analyzeB 2");
			samples[frameFillLoc] = bufferIn[frameIn][sampLocIn];
			frameIn++;
			frameFillLoc++;
		}
		*/
		
	}
}

void FFTProcessor::threadedStartThread(LocklessRingBuffer<sampleFrame>* ringBufferIn, unsigned int sampLocIn, bool computeSpectrum, bool computeInverse)
{
	qDebug("analyze threaded start");
	if (m_isThreaded == true && m_terminate == true && ringBufferIn != nullptr &&
		(computeSpectrum == true || computeInverse == true))
	{
		qDebug("analyze threaded  thread init");
		m_terminate = false;
		unsigned int blockSize = m_blockSize;
		m_FFTThread = std::thread(FFTProcessor::threadedAnalyze, &m_terminate, &m_samplesOut, &m_samplesOut, &m_outputSpectrumChanged, &m_outputSamplesChanged,
			ringBufferIn, &m_complexMultiplier, sampLocIn, blockSize, &m_normSpectrum, &m_outputAccess);
		qDebug("analyze threaded  thread running");
	}
}

std::vector<float> FFTProcessor::getNormSpectrum()
{
	m_outputAccess.lock();
	std::vector<float> output = m_normSpectrum;
	m_outputAccess.unlock();
	return output;
}
std::vector<float> FFTProcessor::getSample()
{
	m_outputAccess.lock();
	std::vector<float> output = m_samplesOut;
	m_outputAccess.unlock();
	return output;
}
bool FFTProcessor::getOutputSpectrumChanged()
{
	bool output = m_outputSpectrumChanged;
	m_outputSpectrumChanged = false;
	return output;
}
bool FFTProcessor::getOutputSamplesChanged()
{
	bool output = m_outputSamplesChanged;
	m_outputSamplesChanged = false;
	return output;
}
void FFTProcessor::setComplexMultiplier(std::vector<float>* complexMultiplierIn)
{
	m_complexMultiplier = *complexMultiplierIn;
}

void FFTProcessor::threadedAnalyze(std::atomic<bool>* terminateIn, std::vector<float>* samplesInB, std::vector<float>* samplesOutB,
	std::atomic<bool>* spectrumChangedOut, std::atomic<bool>* samplesChangedOut,
	LocklessRingBuffer<sampleFrame>* ringBufferIn, std::vector<float>* filterSpectrumIn, unsigned int sampLocIn, unsigned int blockSizeIn,
	std::vector<float>* spectrumOut, std::mutex* outputAccessIn)
{
	LocklessRingBufferReader<sampleFrame> reader(*ringBufferIn);
	
	std::vector<float> samplesIn(blockSizeIn);
	//std::vector<float> samplesInBuffer(blockSizeIn);
	std::vector<float> samplesOut(blockSizeIn);
	std::vector<float> samplesOutBufferA(blockSizeIn);
	std::vector<float> samplesOutBufferB(blockSizeIn);
	std::vector<float> absSpectrum(blockSizeIn);
	std::vector<float> outputSpectrum(blockSizeIn);

	fftwf_complex* fftComplex = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * binCount(blockSizeIn));
	// fftwf_plan fftwf_plan_dft_r2c_1d(int n, float *in, fftwf_complex *out, unsigned flags);
	// fftwf_plan fftwf_plan_dft_c2r_1d(int n, fftwf_complex *in, float *out, unsigned flags);
	fftwf_plan plan = fftwf_plan_dft_r2c_1d(blockSizeIn, samplesIn.data(), fftComplex, FFTW_ESTIMATE);
	fftwf_plan iplan = fftwf_plan_dft_c2r_1d(blockSizeIn, fftComplex, samplesOut.data(), FFTW_ESTIMATE);

	// TODO might not be the correct size
	float pi = 3.141592654f;
	std::vector<float> hanningWindow(blockSizeIn);
	precomputeWindow(hanningWindow.data(), blockSizeIn, FFTWindow::Hanning);
	float maxWindow = 0.0f;
	/*
	for (int i = 0; i < blockSizeIn; i++)
	{
		// applying sinc function
		float sincVal = pi * 0.25f * (i - (static_cast<float>(blockSizeIn) / 2.0f));
		if (sincVal != 0)
		{
			hanningWindow[i] = hanningWindow[i] * std::sin(sincVal) / sincVal;
		}
		sumWindow = sumWindow + hanningWindow[i];
		qDebug("FFTProcessor final window [%d]: %f, %f", i, hanningWindow[i], sumWindow);
	}
	*/
	for (int i = 0; i < blockSizeIn; i++)
	{
		maxWindow = maxWindow < hanningWindow[i] ? hanningWindow[i] : maxWindow;
	}
	for (int i = 0; i < blockSizeIn; i++)
	{
		// normalise
		hanningWindow[i] = hanningWindow[i] / maxWindow;
		qDebug("FFTProcessor sumWindow [%d]: %f,  %f", i, hanningWindow[i], maxWindow);
	}


	fpp_t frameFillLoc = blockSizeIn / 2;
	//fpp_t frameFillLoc = 0;
	int outFillLoc = 0;

	bool writeSamplesOut = true;
	bool writeToBufferA = true;
	int runFFTCount = 0;
	qDebug("analyzeB 0");
	while (*terminateIn == false)
	{
		if (reader.empty() == false)
		{
			reader.waitForData();
		}

		auto bufferIn = reader.read_max(ringBufferIn->capacity());
		size_t frameCount = bufferIn.size();

		if (frameCount > 0)
		{
			qDebug("analyzeB 0.5 DEBUG DEBUG DEBUG DEBUG DEBUG DEBUG count: %ld", frameCount);
		}

		fpp_t frameIn = 0;

		while (frameIn < frameCount)
		{
			qDebug("analyzeB 1");
			while (frameIn < frameCount && frameFillLoc < blockSizeIn)
			//while (frameIn < frameCount && frameFillLoc < blockSizeIn / 2)
			{
				//qDebug("analyzeB 2");
				//samplesInBuffer[frameFillLoc] = bufferIn[frameIn][sampLocIn];
				samplesIn[frameFillLoc - blockSizeIn / 2] = samplesIn[frameFillLoc];
				samplesIn[frameFillLoc] = bufferIn[frameIn][sampLocIn];

				//samplesIn[frameFillLoc + blockSizeIn / 2] = bufferIn[frameIn][sampLocIn];
				//samplesIn[frameFillLoc] = storedSamples[frameFillLoc];
				//storedSamples[frameFillLoc] = bufferIn[frameIn][sampLocIn];
				frameIn++;
				frameFillLoc++;
			}

			if (frameFillLoc < blockSizeIn)
			//if (frameFillLoc < blockSizeIn / 2)
			{
				break;
			}
			frameFillLoc = blockSizeIn / 2;
			//frameFillLoc = 0;
			/*
			runFFTCount = 2;
		}
		while (runFFTCount > 0)
		{
			runFFTCount--;
			int readStartLocation = runFFTCount == 1 ? 0 : blockSizeIn / 2;
			for (unsigned int i = 0; i < blockSizeIn / 2; i++)
			{
				samplesIn[i] = samplesIn[i + blockSizeIn / 2];
				samplesIn[i + blockSizeIn / 2] = samplesInBuffer[i + readStartLocation];
			}
			*/


			qDebug("analyzeB run");

			for (unsigned int i = 0; i < blockSizeIn; i++)
			{
				qDebug("FFTProcessor analyze samplesIn [%d]: %f", i, samplesIn[i]);
			}
			qDebug("analyzeB run2 size: %ld", samplesIn.size());
			
			// applying window
			// this is needed to attenuate the reverse complex correctly
			for (unsigned int i = 0; i < blockSizeIn; i++)
			{
				//qDebug("FFTProcessor analyze samplesIn before window [%d]: %f", i, samplesIn->operator[](i));
				//samplesIn[i] = samplesIn[i] * hanningWindow[i];
				//qDebug("FFTProcessor analyze samplesOut after window [%d]: %f", i, samplesIn->operator[](i));
			}


			//unsigned int reverseSize = blockSizeIn / 2;
			

			fftwf_execute(plan);

			qDebug("analyzeB run3");

			absspec(fftComplex, absSpectrum.data(), binCount(blockSizeIn));
			normalize(absSpectrum, outputSpectrum, blockSizeIn);

			/*
			for (unsigned int i = 0; i < blockSizeIn; i++)
			{
				qDebug("[%d] %f, %f, %f, - %f, %f", i, complexIn[i][0], complexIn[i][1], samplesIn->operator[](i), absSpectrum[i], outputSpectrum[i]);
			}
			*/

			qDebug("analyzeB run4");
			outputAccessIn->lock();
			for (unsigned int i = 0; i < binCount(blockSizeIn); i++)
			{
				spectrumOut->operator[](i) = outputSpectrum[i];
			}
			outputAccessIn->unlock();
			*spectrumChangedOut = true;

			qDebug("analyzeB try inverse");
			if (filterSpectrumIn->size() == blockSizeIn)
			{
				/*
				for (unsigned int i = 0; i < binCount(blockSizeIn); i++)
				{
					inverseOldReal[i] = complexIn[i][0];
					inverseOldImg[i] = complexIn[i][1];
					//qDebug("analyzeB complex %d, %f, %f", i, inverseOldReal[i], inverseOldImg[i]);
				}

			qDebug("analyzeB inverse");
				//int inverseabsspec(const fftwf_complex *complex_buffer, float *absspec_buffer, unsigned int compl_length)
				// the input filter spectrum gets copyed in 2 halfs (rotation part)
				// and then it gets run througth an fft
				for (unsigned int i = 0; i < blockSizeIn / 2; i++)
				{
					
					complexIn[i][0] = complexIn[i][0] * complexMultiplierIn->operator[](i);
					complexIn[i][1] = complexIn[i][1] * complexMultiplierIn->operator[](i);
					
					samplesIn->operator[](i) = filterSpectrumIn->operator[](i + blockSizeIn / 2);
					//changedAbsSpectum[i] = absSpectrum[i];
					//qDebug("FFTProcessor analyze complexMultiplier [%d]: %f", i, complexMultiplierIn->operator[](i));
				}
				for (unsigned int i = blockSizeIn / 2; i < blockSizeIn; i++)
				{
					samplesIn->operator[](i) = filterSpectrumIn->operator[](i);
				}
				for (unsigned int i = 0; i < blockSizeIn; i++)
				{
					//qDebug("FFTProcessor filterIn [%d]: %f %f", i, samplesIn->operator[](i), filterSpectrumIn->operator[](i));
				}
				fftwf_execute(*planIn);

				outputAccessIn->lock();
				//inverseabsspec(complexIn, changedAbsSpectum.data(), binCount(blockSizeIn));
				for (unsigned int i = 0; i < binCount(blockSizeIn); i++)
				{
					//qDebug("analyzeB complex %d, %f, %f, multiply: %f, %f", i, inverseOldReal[i], inverseOldImg[i], complexIn[i][0], complexIn[i][1]);
					complexIn[i][0] = inverseOldReal[i] * complexIn[i][0];
					complexIn[i][1] = inverseOldImg[i] * complexIn[i][1];
					//complexIn[i][0] = inverseOldReal[i];// * complexIn[i][0];
					//complexIn[i][1] = inverseOldImg[i];// * complexIn[i][1];
				}
				*/
				/*
				for (unsigned int i = 0; i < binCount(blockSizeIn); i++)
				{
					qDebug("analyzeB complex start %d, %f, %f", i, fftComplex[i][0], fftComplex[i][1]);
				}
				*/
				std::vector<float> absSpectrumB(blockSizeIn);
				for (unsigned int i = 0; i < blockSizeIn; i++)
				{
					absSpectrumB[i] = absSpectrum[i] * filterSpectrumIn->operator[](i);
				}
				inverseAbsspec(fftComplex, absSpectrumB.data(), 44100, binCount(blockSizeIn));
				/*
				for (unsigned int i = 0; i < binCount(blockSizeIn); i++)
				{
					qDebug("analyzeB complex end %d, %f, %f", i, fftComplex[i][0], fftComplex[i][1]);
				}
				*/
				fftwf_execute(iplan);

				for (unsigned int i = 0; i < blockSizeIn; i++)
				{
					samplesOut[i] = samplesOut[i] / blockSizeIn * hanningWindow[i];
					//samplesOut[i] = samplesOut[i] / blockSizeIn;
					//qDebug("FFTProcessor analyze 0. samplesOut convert_back: [%d]:%f", i, samplesOut[i]);
				}
				/*
				if (writeSamplesOut == false)
				{
					if (writeToBufferA == true)
					{
						samplesOutBufferA = samplesOut;
						qDebug("FFTProcessor analyze A reset reset reset reset reset reset reset reset reset reset reset reset reset");
					}
					else
					{
						samplesOutBufferB = samplesOut;
						qDebug("FFTProcessor analyze B reset reset reset reset reset reset reset reset reset reset reset reset reset");
					}
					writeToBufferA = !writeToBufferA;
					writeSamplesOut = true;
				}
				*/
				//else
				//{
					outputAccessIn->lock();
					float lastDebug = samplesOutB->operator[](samplesOutB->size() - 1);
					/*
					if (writeToBufferA == true)
					{
						for (unsigned int i = 0; i < blockSizeIn / 2; i++)
						{
							samplesOutB->operator[](i) = samplesOutBufferA[i + blockSizeIn / 2];
							qDebug("FFTProcessor analyze 1.A samplesOut [%d]: %f, %f, %d", i, samplesOutB->operator[](i), samplesOutBufferA[i + blockSizeIn / 2], i + blockSizeIn / 2);
						}
						for (unsigned int i = blockSizeIn / 2; i < blockSizeIn; i++)
						{
							samplesOutB->operator[](i) = samplesOutBufferB[i - blockSizeIn / 2];
							qDebug("FFTProcessor analyze 3.B samplesOut [%d]: %f, %f, %d", i, samplesOutB->operator[](i), samplesOutBufferB[i - blockSizeIn / 2], i - blockSizeIn / 2);
						}
					}
					else
					{
						for (unsigned int i = 0; i < blockSizeIn / 2; i++)
						{
							samplesOutB->operator[](i) = samplesOutBufferB[i + blockSizeIn / 2];
							qDebug("FFTProcessor analyze 1.B samplesOut [%d]: %f, %f, %d", i, samplesOutB->operator[](i), samplesOutBufferB[i + blockSizeIn / 2], i + blockSizeIn / 2);
						}
						for (unsigned int i = blockSizeIn / 2; i < blockSizeIn; i++)
						{
							samplesOutB->operator[](i) = samplesOutBufferA[i - blockSizeIn / 2];
							qDebug("FFTProcessor analyze 3.A samplesOut [%d]: %f, %f, %d", i, samplesOutB->operator[](i), samplesOutBufferA[i - blockSizeIn / 2], i - blockSizeIn / 2);
						}
					}

					for (unsigned int i = 0; i < blockSizeIn; i++)
					{
						//qDebug("FFTProcessor analyze 2. samplesOut [%d]: %f, %f, %d", i, samplesOutB->operator[](i), samplesOut[i], i);
						samplesOutB->operator[](i) += samplesOut[i];
						qDebug("FFTProcessor analyze 2. samplesOut [%d]: %f, %f, %d", i, samplesOutB->operator[](i), samplesOut[i], i);
					}
					*/

					if (outFillLoc == 0)
					{
						for (unsigned int i = 0; i < blockSizeIn; i++)
						{
							samplesOutB->operator[](i) = 0.0f;
						}
						for (unsigned int i = 0; i < blockSizeIn / 2; i++)
						{
							samplesOutB->operator[](i) = samplesOutBufferA[i + blockSizeIn / 2];
							//qDebug("FFTProcessor analyze 1. samplesOut [%d]: %f, %f, %d", i, samplesOutB->operator[](i), samplesOutBufferA[i + blockSizeIn / 2], i + blockSizeIn / 2);
						}
					}
					for (unsigned int i = outFillLoc; i < blockSizeIn; i++)
					{
						samplesOutB->operator[](i) += samplesOut[i - outFillLoc];
						//samplesOutB->operator[](i) = samplesOut[i - outFillLoc];
						//qDebug("FFTProcessor analyze 2. samplesOut [%d]: %f, %f, %d", i, samplesOutB->operator[](i), samplesOut[i - outFillLoc], i - outFillLoc);
					}
					outputAccessIn->unlock();

					outFillLoc += blockSizeIn / 2;
					if (outFillLoc + 1 >= blockSizeIn)
					{
						qDebug("FFTProcessor analyze write out, samplesOut copyed");
						samplesOutBufferA = samplesOut;
						*samplesChangedOut = true;
						outFillLoc = 0;
					}

					/*
					if (std::abs(samplesOutB->operator[](0) - lastDebug) > 0.08f && lastDebug != 0.0f)
					{
						qDebug("FFTProcessor analyze break 2");
						int* debugval = nullptr;
						// *debugval = 1;
					}

					for (unsigned int i = 0; i < blockSizeIn; i++)
					{
						qDebug("FFTProcessor analyze end samplesOut [%d]: %f", i, samplesOutB->operator[](i));
						if (i > 0 && std::abs(samplesOutB->operator[](i - 1) - samplesOutB->operator[](i)) > 0.15f && std::abs(samplesOutB->operator[](i)) >= 0.01f && std::abs(samplesOutB->operator[](i - 1)) >= 0.01f)
						{

						qDebug("FFTProcessor analyze break 1");
						int* debugval = nullptr;
						// *debugval = 1;

						}
					}
					*/
					writeSamplesOut = false;
				//}
			}
		}
	}

	if (plan != nullptr)
	{
		fftwf_destroy_plan(plan);
	}
	if (iplan != nullptr)
	{
		fftwf_destroy_plan(iplan);
	}
	qDebug("FFTProcessor destrc 0");
	qDebug("FFTProcessor destrc 1");
	if (fftComplex != nullptr)
	{
		fftwf_free(fftComplex);
	}

	qDebug("analyzeB end");
}

// TODO
void FFTProcessor::reverse(std::vector<float> processedDataIn)
{
	
}

void FFTProcessor::threadedTerminate()
{
	m_terminate = true;
	if (m_isThreaded == true)
	{
		m_FFTThread.join();
	}
}

// TODO multithread
void FFTProcessor::rebuildWindow(FFTWindow FFTWindowIn)
{
	//m_FFTWindowType = FFTWindowIn;
	//precomputeWindow(m_fftWindow.data(), m_blockSize, FFTWindowIn);
}

// TODO multithread

unsigned int FFTProcessor::binCount(unsigned int blockSizeIn)
{
	return blockSizeIn / 2 + 1;
}

} // namespace lmms
