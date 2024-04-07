

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

	m_out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * binCount(m_blockSize));
	// fftwf_plan fftwf_plan_dft_r2c_1d(int n, float *in, fftwf_complex *out, unsigned flags);
	// fftwf_plan fftwf_plan_dft_c2r_1d(int n, fftwf_complex *in, float *out, unsigned flags);
	m_plan = fftwf_plan_dft_r2c_1d(m_blockSize, m_samplesIn.data(), m_out, FFTW_ESTIMATE);
	m_iplan = fftwf_plan_dft_c2r_1d(m_blockSize, m_out, m_samplesOut.data(), FFTW_ESTIMATE);

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
	qDebug("FFTProcessor destrc 1");
	if (m_out != nullptr)
	{
		fftwf_free(m_out);
	}
	
	qDebug("FFTProcessor destrc 2");
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
		m_FFTThread = std::thread(FFTProcessor::threadedAnalyze, &m_terminate, &m_plan, &m_iplan, m_out, &m_samplesIn, &m_samplesOut, &m_outputSpectrumChanged, &m_outputSamplesChanged,
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

void FFTProcessor::threadedAnalyze(std::atomic<bool>* terminateIn, fftwf_plan* planIn, fftwf_plan* inversePlanIn, fftwf_complex* complexIn, std::vector<float>* samplesIn, std::vector<float>* samplesOut,
	std::atomic<bool>* spectrumChangedOut, std::atomic<bool>* samplesChangedOut,
	LocklessRingBuffer<sampleFrame>* ringBufferIn, std::vector<float>* filterSpectrumIn, unsigned int sampLocIn, unsigned int blockSizeIn,
	std::vector<float>* spectrumOut, std::mutex* outputAccessIn)
{
	LocklessRingBufferReader<sampleFrame> reader(*ringBufferIn);
	
	//std::vector<float> samples(blockSizeIn);
	std::vector<float> absSpectrum(blockSizeIn);
	std::vector<float> outputSpectrum(blockSizeIn);

	// TODO might not be the correct size
	float pi = 3.141592654f;
	std::vector<float> hanningWindow(blockSizeIn);
	precomputeWindow(hanningWindow.data(), blockSizeIn, FFTWindow::Hanning);
	float sumWindow = 0.0f;
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
	for (int i = 0; i < blockSizeIn; i++)
	{
		// normalise
		hanningWindow[i] = hanningWindow[i] / sumWindow;
		//qDebug("FFTProcessor sumWindow [%d]: %f,  %f", i, hanningWindow[i], sumWindow);
	}

	std::vector<float> inverseOldReal(binCount(blockSizeIn));
	std::vector<float> inverseOldImg(binCount(blockSizeIn));


	std::vector<float> storedSamples(binCount(blockSizeIn));

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
			outputAccessIn->lock();
			while (frameIn < frameCount && frameFillLoc < blockSizeIn)
			//while (frameIn < frameCount && frameFillLoc < blockSizeIn / 2)
			{
			//qDebug("analyzeB 2");
				samplesIn->operator[](frameFillLoc + blockSizeIn / 2) = bufferIn[frameIn][sampLocIn];
				samplesIn->operator[](frameFillLoc) = storedSamples[frameFillLoc];
				storedSamples[frameFillLoc] = bufferIn[frameIn][sampLocIn];
				frameIn++;
				frameFillLoc++;
			}
			outputAccessIn->unlock();
			

			if (frameFillLoc < blockSizeIn)
			//if (frameFillLoc < blockSizeIn / 2)
			{
				break;
			}
			frameFillLoc = 0;

			qDebug("analyzeB run");

			for (unsigned int i = 0; i < blockSizeIn; i++)
			{
				//qDebug("FFTProcessor analyze samplesIn [%d]: %f", i, samplesIn->operator[](i));
			}
			
			// applying window
			// this is needed to attenuate the reverse complex correctly
			for (unsigned int i = 0; i < blockSizeIn; i++)
			{
				//qDebug("FFTProcessor analyze samplesIn before window [%d]: %f", i, samplesIn->operator[](i));
				//samplesIn->operator[](i) = samplesIn->operator[](i) * hanningWindow[i];
				//qDebug("FFTProcessor analyze samplesOut after window [%d]: %f", i, samplesIn->operator[](i));
			}

			// TODO 1.:
			// TODO x = samples (x is the sound input with a big length)		Done
			// TODO create hanning widnow		Done
			// TODO window = element wise multiplication of hanning and sinc(?) TODO function		Done
			// TODO normalise window		Done
			// TODO window_complex = fft(window, N) (it can be applyed before too?)		Done
			// TODO complex * window_complex		Done
			// TODO ifft()
			//
			// TODO 2.:
			// TODO size = 512
			// TODO buffer(size * 2, 0) // create a (size * 2) buffer filled with 0-s
			// TODO index = iterator througth 1 - 512
			// TODO output_buffer(size * 2, 0)
			// TODO for (unsingned int j = 0; j < size; j++)  // process 512 points of x at a time
			// TODO		shift buffer's second half to the first half
			// TODO		fill buffer's second half with new data x[index] (=samples)
			// TODO		fftbuffer = ifft(fft(buffer) * fft(window, size * 2)) // multiply complexes, window_complex is sized (size * 2) and contains the fft of window
			// TODO		output_buffer[index] = real(fftbuffer(size + 1:end)) // getting the real part (when complex?) of fftbuffer (from size + 1 to end) (setting output_buffers first 512 (sie) values)
			// TODO		index = index + size; // iterate througth the next 512 x (=sample) values
			//
			// TODO 3.:
			// TODO 

			//unsigned int reverseSize = blockSizeIn / 2;
			

			//fftwf_execute(*planIn);


			absspec(complexIn, absSpectrum.data(), binCount(blockSizeIn));
			normalize(absSpectrum, outputSpectrum, blockSizeIn);

			/*
			for (unsigned int i = 0; i < blockSizeIn; i++)
			{
				qDebug("[%d] %f, %f, %f, - %f, %f", i, complexIn[i][0], complexIn[i][1], samplesIn->operator[](i), absSpectrum[i], outputSpectrum[i]);
			}
			*/

			outputAccessIn->lock();
			for (unsigned int i = 0; i < binCount(blockSizeIn); i++)
			{
				spectrumOut->operator[](i) = outputSpectrum[i];
			}
			outputAccessIn->unlock();
			*spectrumChangedOut = true;

			qDebug("analyzeB try inverse");
			if (filterSpectrumIn->size() == blockSizeIn && false)
			{
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
					/*
					complexIn[i][0] = complexIn[i][0] * complexMultiplierIn->operator[](i);
					complexIn[i][1] = complexIn[i][1] * complexMultiplierIn->operator[](i);
					*/
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
				fftwf_execute(*inversePlanIn);
				for (unsigned int i = 0; i < blockSizeIn; i++)
				{
					samplesOut->operator[](i) = samplesOut->operator[](i) / blockSizeIn;
					//samplesOut->operator[](i) = samplesOut->operator[](i) / hanningWindow[i] / blockSizeIn;
					//qDebug("FFTProcessor analyze samplesOut [%d]: %f", i, samplesOut->operator[](i));
				}
				outputAccessIn->unlock();
				*samplesChangedOut = true;
			}
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

unsigned int FFTProcessor::binCount(unsigned int blockSizeIn)
{
	return blockSizeIn / 2 + 1;
}

} // namespace lmms
