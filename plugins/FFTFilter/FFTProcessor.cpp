

#include <fftw3.h>
#include <vector>
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
	m_normSpectrum.resize(outputSize(m_blockSize));

	m_in = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * m_blockSize);
	m_out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * m_blockSize);
	// fftwf_plan fftwf_plan_dft_r2c_1d(int n, float *in, fftwf_complex *out, unsigned flags);
	// fftwf_plan fftwf_plan_dft_c2r_1d(int n, fftwf_complex *in, float *out, unsigned flags);
	m_plan = fftwf_plan_dft_r2c_1d(m_blockSize, m_samplesIn.data(), m_out, FFTW_ESTIMATE);
	m_iplan = fftwf_plan_dft_c2r_1d(m_blockSize, m_in, m_samplesIn.data(), FFTW_ESTIMATE);

	m_fftWindow.resize(m_blockSize, 1.0);
	precomputeWindow(m_fftWindow.data(), m_blockSize, m_FFTWindowType);
}
FFTProcessor::~FFTProcessor()
{
	qDebug("FFTProcessor destrc -1");

	threadedTerminate();
	if (m_plan != nullptr)
	{
		fftwf_destroy_plan(m_plan);
	}
	if (m_iplan != nullptr)
	{
		fftwf_destroy_plan(m_iplan);
	}
	qDebug("FFTProcessor destrc 0");
	if (m_in != nullptr)
	{
		fftwf_free(m_in);
	}
	qDebug("FFTProcessor destrc 1");
	if (m_out != nullptr)
	{
		fftwf_free(m_out);
	}
	
	qDebug("FFTProcessor destrc 2");
	m_in = nullptr;
	m_out = nullptr;
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
		m_FFTThread = std::thread(FFTProcessor::threadedAnalyze, &m_terminate, &m_plan, m_out, &m_samplesIn, &m_outputSpectrumChanged, &m_outputSamplesChanged,
			ringBufferIn, sampLocIn, blockSize, &m_normSpectrum, &m_outputAccess);
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


void FFTProcessor::threadedAnalyze(std::atomic<bool>* terminateIn, fftwf_plan* planIn, fftwf_complex* complexIn, std::vector<float>* samplesIn, std::atomic<bool>* spectrumChangedOut, std::atomic<bool>* samplesChangedOut,
	LocklessRingBuffer<sampleFrame>* ringBufferIn, unsigned int sampLocIn, unsigned int blockSizeIn,
	std::vector<float>* spectrumOut, std::mutex* outputAccessIn)
{
	LocklessRingBufferReader<sampleFrame> reader(*ringBufferIn);
	
	//std::vector<float> samples(blockSizeIn);
	std::vector<float> absSpectrum(blockSizeIn);
	std::vector<float> outputSpectrum(blockSizeIn);
	fpp_t frameFillLoc = 0;

	qDebug("analyzeB 0");
	while (*terminateIn == false)
	{
		if (reader.empty() == false)
		{
			reader.waitForData();
		}

		auto bufferIn = reader.read_max(ringBufferIn->capacity());
		size_t frameCount = bufferIn.size();

		fpp_t frameIn = 0;

		while (frameIn < frameCount)
		{
			qDebug("analyzeB 1");
			while (frameIn < frameCount && frameFillLoc < blockSizeIn)
			{
			//qDebug("analyzeB 2");
				samplesIn->operator[](frameFillLoc) = bufferIn[frameIn][sampLocIn];
				frameIn++;
				frameFillLoc++;
			}
			

			if (frameFillLoc < blockSizeIn)
			{
				break;
			}
			frameFillLoc = 0;

			qDebug("analyzeB run");

			
			/*
			for (unsigned int i = 0; i < m_blockSize; i++)
			{
				m_samplesIn[i] = m_samplesIn * m_FFTWindow[i];
			}
			*/

			fftwf_execute(*planIn);


			absspec(complexIn, absSpectrum.data(), outputSize(blockSizeIn));
			normalize(absSpectrum, outputSpectrum, blockSizeIn);

			/*
			for (unsigned int i = 0; i < blockSizeIn; i++)
			{
				qDebug("[%d] %f, %f, %f, - %f, %f", i, complexIn[i][0], complexIn[i][1], samplesIn->operator[](i), absSpectrum[i], outputSpectrum[i]);
			}
			*/

			outputAccessIn->lock();
			for (unsigned int i = 0; i < outputSize(blockSizeIn); i++)
			{
				spectrumOut->operator[](i) = outputSpectrum[i];
			}
			outputAccessIn->unlock();
			*spectrumChangedOut = true;
		}
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
	m_FFTWindowType = FFTWindowIn;
	precomputeWindow(m_fftWindow.data(), m_blockSize, FFTWindowIn);
}

// TODO multithread

unsigned int FFTProcessor::outputSize(unsigned int blockSizeIn)
{
	return blockSizeIn / 2 + 1;
}

} // namespace lmms
