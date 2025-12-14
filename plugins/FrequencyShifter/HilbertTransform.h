/*
 * HilbertTransform.h
 *
 * Copyright (c) 2025 Lost Robot <r94231/at/gmail/dot/com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * -------------------------------------------------------------------------
 * Full credit for the Hilbert IIR coefficients and general usage goes to:
 *
 *   Copyright (c) 2024 Geraint Luff / Signalsmith Audio Ltd.
 *   Released under the 0BSD (Zero-Clause BSD) License.
 * -------------------------------------------------------------------------
 */

#ifndef LMMS_HILBERT_TRANSFORM_H
#define LMMS_HILBERT_TRANSFORM_H
#include <cmath>

#ifdef __SSE2__
	#include <emmintrin.h>
#endif

namespace lmms
{

template<int Channels>
struct HilbertIIRFloat
{
	static constexpr int order = 12;

	alignas(16) static constexpr float baseCoeffsR[order] = {
		-0.000224352093802f,  0.0107500557815f, -0.0456795873917f,
		 0.11282500582f,     -0.208067578452f,  0.28717837501f,
		-0.254675294431f,     0.0481081835026f, 0.227861357867f,
		-0.365411839137f,     0.280729061131f, -0.0935061787728f
	};
	alignas(16) static constexpr float baseCoeffsI[order] = {
		 0.00543499018201f,  -0.0173890685681f, 0.0229166931429f,
		 0.00278413661237f,  -0.104628958675f,  0.33619239719f,
		-0.683033899655f,     0.954061589374f, -0.891273574569f,
		 0.525088317271f,    -0.155131206606f,  0.00512245855404f
	};
	alignas(16) static constexpr float basePolesR[order] = {
		-0.00495335976478f, -0.017859491302f,  -0.0413714373155f,
		-0.0882148408885f,  -0.17922965812f,   -0.338261800753f,
		-0.557688699732f,   -0.735157736148f,  -0.719057381172f,
		-0.517871025209f,   -0.280197469471f,  -0.0852751354531f
	};
	alignas(16) static constexpr float basePolesI[order] = {
		 0.0092579876872f,   0.0273493725543f,  0.0744756910287f,
		 0.178349677457f,    0.39601340223f,    0.829229533354f,
		 1.61298538328f,     2.79987398682f,    4.16396166128f,
		 5.29724826804f,     5.99598602388f,    6.3048492377f
	};
	static constexpr float baseDirect = 0.000262057212648f;

	alignas(16) float coeffsR[order];
	alignas(16) float coeffsI[order];
	alignas(16) float polesR[order];
	alignas(16) float polesI[order];
	float direct;
	alignas(16) float stateR[Channels][order];
	alignas(16) float stateI[Channels][order];

	HilbertIIRFloat(float sampleRate = 48000.0f, float passbandGain = 2.0f)
	{
		const float freqFactor = std::fmin(0.46f, 20000.0f / sampleRate);
		const float coeffScale = freqFactor * passbandGain;
		direct = baseDirect * 2.0f * passbandGain * freqFactor;
		for (int i = 0; i < order; ++i)
		{
			coeffsR[i] = baseCoeffsR[i] * coeffScale;
			coeffsI[i] = baseCoeffsI[i] * coeffScale;
			const float a = basePolesR[i] * freqFactor;
			const float b = basePolesI[i] * freqFactor;
			const float ea = std::exp(a);
			polesR[i] = ea * std::cos(b);
			polesI[i] = ea * std::sin(b);
		}
		reset();
	}

	inline void reset()
	{
		for (int ch = 0; ch < Channels; ++ch)
		{
			for (int i = 0; i < order; ++i)
			{
				stateR[ch][i] = stateI[ch][i] = 0.0f;
			}
		}
	}

	inline void processReal(float x, int channel, float *out)
	{
		float *sR = stateR[channel], *sI = stateI[channel];
#ifdef __SSE2__
		const __m128 vx = _mm_set1_ps(x);
		__m128 sumR = _mm_setzero_ps(), sumI = _mm_setzero_ps();
		for (int i = 0; i < order; i += 4)
		{
			__m128 vr  = _mm_load_ps(&sR[i]);
			__m128 vi  = _mm_load_ps(&sI[i]);
			__m128 vpr = _mm_load_ps(&polesR[i]);
			__m128 vpi = _mm_load_ps(&polesI[i]);
			__m128 vcr = _mm_load_ps(&coeffsR[i]);
			__m128 vci = _mm_load_ps(&coeffsI[i]);

			__m128 rpr  = _mm_mul_ps(vr, vpr);
			__m128 impi = _mm_mul_ps(vi, vpi);
			__m128 xcr  = _mm_mul_ps(vx, vcr);
			__m128 nr   = _mm_add_ps(_mm_sub_ps(rpr, impi), xcr);

			__m128 rpi  = _mm_mul_ps(vr, vpi);
			__m128 impr = _mm_mul_ps(vi, vpr);
			__m128 xci  = _mm_mul_ps(vx, vci);
			__m128 ni   = _mm_add_ps(_mm_add_ps(rpi, impr), xci);

			_mm_store_ps(&sR[i], nr);
			_mm_store_ps(&sI[i], ni);

			sumR = _mm_add_ps(sumR, nr);
			sumI = _mm_add_ps(sumI, ni);
		}
		float tmpR[4], tmpI[4];
		_mm_storeu_ps(tmpR, sumR);
		_mm_storeu_ps(tmpI, sumI);
		out[0] = x * direct + (tmpR[0] + tmpR[1] + tmpR[2] + tmpR[3]);
		out[1] =              (tmpI[0] + tmpI[1] + tmpI[2] + tmpI[3]);
#else
		float sumR = 0.0f, sumI = 0.0f;
		for (int i = 0; i < order; ++i)
		{
			const float r = sR[i], im = sI[i], pr = polesR[i], pi = polesI[i];
			const float nr = r * pr - im * pi + x * coeffsR[i];
			const float ni = r * pi + im * pr + x * coeffsI[i];
			sR[i] = nr; sI[i] = ni;
			sumR += nr; sumI += ni;
		}
		out[0] = x * direct + sumR;
		out[1] = sumI;
#endif
	}

	inline void processComplex(const float *x, int channel, float *out)
	{
		const float xr = x[0], xi = x[1];
		float *sR = stateR[channel], *sI = stateI[channel];
#ifdef __SSE2__
		const __m128 vxr = _mm_set1_ps(xr), vxi = _mm_set1_ps(xi);
		__m128 sumR = _mm_setzero_ps(), sumI = _mm_setzero_ps();
		for (int i = 0; i < order; i += 4)
		{
			__m128 vr  = _mm_load_ps(&sR[i]);
			__m128 vi  = _mm_load_ps(&sI[i]);
			__m128 vpr = _mm_load_ps(&polesR[i]);
			__m128 vpi = _mm_load_ps(&polesI[i]);
			__m128 vcr = _mm_load_ps(&coeffsR[i]);
			__m128 vci = _mm_load_ps(&coeffsI[i]);

			__m128 xrcr = _mm_mul_ps(vxr, vcr);
			__m128 xici = _mm_mul_ps(vxi, vci);
			__m128 xrci = _mm_mul_ps(vxr, vci);
			__m128 xicr = _mm_mul_ps(vxi, vcr);

			__m128 rpr  = _mm_mul_ps(vr, vpr);
			__m128 impi = _mm_mul_ps(vi, vpi);
			__m128 rpi  = _mm_mul_ps(vr, vpi);
			__m128 impr = _mm_mul_ps(vi, vpr);

			__m128 nr = _mm_add_ps(_mm_sub_ps(rpr, impi), _mm_sub_ps(xrcr, xici));
			__m128 ni = _mm_add_ps(_mm_add_ps(rpi, impr), _mm_add_ps(xrci, xicr));

			_mm_store_ps(&sR[i], nr);
			_mm_store_ps(&sI[i], ni);

			sumR = _mm_add_ps(sumR, nr);
			sumI = _mm_add_ps(sumI, ni);
		}
		float tmpR[4], tmpI[4];
		_mm_storeu_ps(tmpR, sumR);
		_mm_storeu_ps(tmpI, sumI);
		const float sr = tmpR[0] + tmpR[1] + tmpR[2] + tmpR[3];
		const float si = tmpI[0] + tmpI[1] + tmpI[2] + tmpI[3];
		out[0] = xr * direct + sr;
		out[1] = xi * direct + si;
#else
		float sumR = 0.0f, sumI = 0.0f;
		for (int i = 0; i < order; ++i)
		{
			const float r = sR[i], im = sI[i];
			const float pr = polesR[i], pi = polesI[i], cr = coeffsR[i], ci = coeffsI[i];
			const float xrcr = xr * cr, xici = xi * ci, xrci = xr * ci, xicr = xi * cr;
			const float nr = r * pr - im * pi + (xrcr - xici);
			const float ni = r * pi + im * pr + (xrci + xicr);
			sR[i] = nr; sI[i] = ni;
			sumR += nr; sumI += ni;
		}
		out[0] = xr * direct + sumR;
		out[1] = xi * direct + sumI;
#endif
	}
};

} // namespace lmms

#endif // LMMS_HILBERT_TRANSFORM_H
