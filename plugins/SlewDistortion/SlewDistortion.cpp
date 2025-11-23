/*
 * SlewDistortion.cpp
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
 */

#include "SlewDistortion.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{

extern "C"
{
Plugin::Descriptor PLUGIN_EXPORT slewdistortion_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"Slew Distortion",
	QT_TRANSLATE_NOOP("PluginBrowser", "A 2-band distortion and slew rate limiter plugin."),
	"Lost Robot <r94231/at/gmail/dot/com>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
};
}


SlewDistortion::SlewDistortion(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :
	Effect(&slewdistortion_plugin_descriptor, parent, key),
	m_sampleRate(Engine::audioEngine()->outputSampleRate()),
	m_lp(m_sampleRate),
	m_hp(m_sampleRate),
	m_slewdistortionControls(this)
{
	connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, this, &SlewDistortion::changeSampleRate);
	changeSampleRate();
}


#ifdef __SSE2__
Effect::ProcessStatus SlewDistortion::processImpl(SampleFrame* buf, const fpp_t frames)
{
	const float d = dryLevel();
	const float w = wetLevel();

	const int oversampling = m_slewdistortionControls.m_oversamplingModel.value();
	const int oversampleVal = 1 << oversampling;
	if (oversampleVal != m_oldOversampleVal)
	{
		m_oldOversampleVal = oversampleVal;
		changeSampleRate();
	}

	const SlewDistortionType distType1 = static_cast<SlewDistortionType>(m_slewdistortionControls.m_distType1Model.value());
	const SlewDistortionType distType2 = static_cast<SlewDistortionType>(m_slewdistortionControls.m_distType2Model.value());
	const float drive1 = dbfsToAmp(m_slewdistortionControls.m_drive1Model.value());
	const float drive2 = dbfsToAmp(m_slewdistortionControls.m_drive2Model.value());
	const float slewUp1 = dbfsToAmp(m_slewdistortionControls.m_slewUp1Model.value()) / oversampleVal;
	const float slewUp2 = dbfsToAmp(m_slewdistortionControls.m_slewUp2Model.value()) / oversampleVal;
	const float slewDown1 = dbfsToAmp(m_slewdistortionControls.m_slewDown1Model.value()) / oversampleVal;
	const float slewDown2 = dbfsToAmp(m_slewdistortionControls.m_slewDown2Model.value()) / oversampleVal;
	const float bias1 = m_slewdistortionControls.m_bias1Model.value();
	const float bias2 = m_slewdistortionControls.m_bias2Model.value();
	const float warp1 = m_slewdistortionControls.m_warp1Model.value();
	const float warp2 = m_slewdistortionControls.m_warp2Model.value();
	const float crush1 = dbfsToAmp(m_slewdistortionControls.m_crush1Model.value());
	const float crush2 = dbfsToAmp(m_slewdistortionControls.m_crush2Model.value());
	const float attack1 = msToCoeff(m_slewdistortionControls.m_attack1Model.value());
	const float attack2 = msToCoeff(m_slewdistortionControls.m_attack2Model.value());
	const float attackInv1 = 1.f - attack1;
	const float attackInv2 = 1.f - attack2;
	const float release1 = msToCoeff(m_slewdistortionControls.m_release1Model.value());
	const float release2 = msToCoeff(m_slewdistortionControls.m_release2Model.value());
	const float releaseInv1 = 1.f - release1;
	const float releaseInv2 = 1.f - release2;
	const float dynamics1 = m_slewdistortionControls.m_dynamics1Model.value();
	const float dynamics2 = m_slewdistortionControls.m_dynamics2Model.value();
	const float dynamicSlew1 = m_slewdistortionControls.m_dynamicSlew1Model.value();
	const float dynamicSlew2 = m_slewdistortionControls.m_dynamicSlew2Model.value();
	const float outVol1 = dbfsToAmp(m_slewdistortionControls.m_outVol1Model.value());
	const float outVol2 = dbfsToAmp(m_slewdistortionControls.m_outVol2Model.value());
	const float split = m_slewdistortionControls.m_splitModel.value();
	const bool dcRemove = m_slewdistortionControls.m_dcRemoveModel.value();
	const bool multiband = m_slewdistortionControls.m_multibandModel.value();
	const float mix1 = m_slewdistortionControls.m_mix1Model.value();
	const float mix2 = m_slewdistortionControls.m_mix2Model.value();
	const bool slewLink1 = m_slewdistortionControls.m_slewLink1Model.value();
	const bool slewLink2 = m_slewdistortionControls.m_slewLink2Model.value();

	const __m128 drive = _mm_set_ps(drive2, drive2, drive1, drive1);
	const __m128 slewUp = _mm_set_ps(slewUp2, slewUp2, slewUp1, slewUp1);
	const __m128 slewDown = _mm_set_ps(slewDown2, slewDown2, slewDown1, slewDown1);
	const __m128 warp = _mm_set_ps(warp2, warp2, warp1, warp1);
	const __m128 crush = _mm_set_ps(crush2, crush2, crush1, crush1);
	const __m128 outVol = _mm_set_ps(outVol2, outVol2, outVol1, outVol1);
	const __m128 attack = _mm_set_ps(attack2, attack2, attack1, attack1);
	const __m128 attackInv = _mm_set_ps(attackInv2, attackInv2, attackInv1, attackInv1);
	const __m128 release = _mm_set_ps(release2, release2, release1, release1);
	const __m128 releaseInv = _mm_set_ps(releaseInv2, releaseInv2, releaseInv1, releaseInv1);
	const __m128 dynamics = _mm_set_ps(dynamics2, dynamics2, dynamics1, dynamics1);
	const __m128 dynamicSlew = _mm_set_ps(dynamicSlew2, dynamicSlew2, dynamicSlew1, dynamicSlew1);
	const __m128 mix = _mm_set_ps(mix2, mix2, mix1, mix1);
	const __m128 minFloor = _mm_set1_ps(SLEW_DISTORTION_MIN_FLOOR);
	const int link1Mask = -static_cast<int>(slewLink1);
	const int link2Mask = -static_cast<int>(slewLink2);
	const __m128 slewLinkMask = _mm_castsi128_ps(_mm_set_epi32(link2Mask, link2Mask, link1Mask, link1Mask));

	const __m128 zero = _mm_setzero_ps();
	const __m128 one = _mm_set1_ps(1.0f);

	if (m_slewdistortionControls.m_splitModel.isValueChanged())
	{
		m_lp.setLowpass(split);
		m_hp.setHighpass(split);
	}

	for (fpp_t f = 0; f < frames; ++f)
	{
		// interpolate bias to remove crackling when moving the parameter
		m_trueBias1 = m_biasInterpCoef * m_trueBias1 + (1.f - m_biasInterpCoef) * bias1;
		m_trueBias2 = m_biasInterpCoef * m_trueBias2 + (1.f - m_biasInterpCoef) * bias2;
		const __m128 bias = _mm_set_ps(m_trueBias2, m_trueBias2, m_trueBias1, m_trueBias1);

		if (oversampleVal > 1)
		{
			m_upsampler[0].processSample(m_overOuts[0].data(), buf[f][0]);
			m_upsampler[1].processSample(m_overOuts[1].data(), buf[f][1]);
		}
		else
		{
			m_overOuts[0][0] = buf[f][0];
			m_overOuts[1][0] = buf[f][1];
		}

		for (int overSamp = 0; overSamp < oversampleVal; ++overSamp)
		{
			alignas(16) std::array<float, 4> inArr = {0};
			if (multiband)
			{
				inArr[0] = m_hp.update(m_overOuts[0][overSamp], 0);
				inArr[1] = m_hp.update(m_overOuts[1][overSamp], 1);
				inArr[2] = m_lp.update(m_overOuts[0][overSamp], 0);
				inArr[3] = m_lp.update(m_overOuts[1][overSamp], 1);
			}
			else
			{
				inArr[0] = m_overOuts[0][overSamp];
				inArr[1] = m_overOuts[1][overSamp];
				inArr[2] = 0;
				inArr[3] = 0;
			}

			__m128 in = _mm_load_ps(&inArr[0]);
			__m128 absIn = sse2Abs(in);

			// store volume for display
			_mm_store_ps(&m_inPeakDisplay[0], _mm_max_ps(_mm_load_ps(&m_inPeakDisplay[0]), _mm_mul_ps(absIn, drive)));

			__m128 inEnv   = _mm_load_ps(&m_inEnv[0]);
			__m128 slewOut = _mm_load_ps(&m_slewOut[0]);

			// apply attack and release to envelope follower
			__m128 cmp = _mm_cmpgt_ps(absIn, inEnv);
			__m128 envRise = _mm_add_ps(_mm_mul_ps(inEnv, attack), _mm_mul_ps(absIn, attackInv));
			__m128 envFall = _mm_add_ps(_mm_mul_ps(inEnv, release), _mm_mul_ps(absIn, releaseInv));
			inEnv = _mm_or_ps(_mm_and_ps(cmp, envRise), _mm_andnot_ps(cmp, envFall));
			inEnv = _mm_max_ps(inEnv, minFloor);

			// this is the input signal's slew rate
			__m128 rate = _mm_sub_ps(in, slewOut);

			__m128 scaledLog = _mm_mul_ps(dynamicSlew, fastLog(inEnv));
			// clamp to [-80.0f, 80.0f] since float std::exp breaks outside of those bounds
			__m128 clampedScaledLog = _mm_max_ps(_mm_min_ps(scaledLog, _mm_set1_ps(80.0f)), _mm_set1_ps(-80.0f));
			__m128 slewMult = fastExp(clampedScaledLog);

			// determine whether we should use the slew up or slew down parameter
			__m128 finalMask = _mm_or_ps(_mm_cmpge_ps(rate, zero), slewLinkMask);
			__m128 finalSlew = _mm_or_ps(_mm_and_ps(finalMask, _mm_mul_ps(slewUp, slewMult)),
				_mm_andnot_ps(finalMask, _mm_mul_ps(slewDown, slewMult)));

			__m128 clampedRate = _mm_max_ps(_mm_sub_ps(zero, finalSlew), _mm_min_ps(rate, finalSlew));
			slewOut = _mm_add_ps(slewOut, clampedRate);

			// apply drive and bias
			__m128 biasedIn = _mm_add_ps(_mm_mul_ps(slewOut, drive), bias);

			// apply warp and crush
			// distIn = (biasedIn - std::copysign(warp[i] / crush[i], biasedIn)) / (1.f - warp[i]);
			__m128 signBiasedIn = _mm_and_ps(biasedIn, _mm_castsi128_ps(_mm_set1_epi32(0x80000000)));
			__m128 warpOverCrush = _mm_div_ps(warp, crush);
			__m128 copysignWarpOverCrush = _mm_or_ps(warpOverCrush, signBiasedIn);
			__m128 distIn = _mm_div_ps(_mm_sub_ps(biasedIn, copysignWarpOverCrush), _mm_sub_ps(one, warp));

			alignas(16) std::array<float, 4> distInArr;
			_mm_store_ps(&distInArr[0], distIn);
			alignas(16) std::array<float, 4> distOutArr;

			// if both bands have the same distortion type, we can process all four channels simultaneously
			// otherwise we have to do two at a time
			int loopCount = (distType1 == distType2 || !multiband) ? 1 : 2;

			for (int pair = 0; pair < loopCount; ++pair)
			{
				SlewDistortionType currentDistType = (pair == 0) ? distType1 : distType2;

				__m128 distInFull = _mm_load_ps(&distInArr[0]);
				__m128 distOutFull;

				// switch-case applies the distortion to the full set of 4 values
				switch (currentDistType)
				{
					case SlewDistortionType::HardClip:// Hard Clip => clamp(x, -1, 1)
					{
						__m128 minVal = _mm_set1_ps(-1.0f);
						__m128 maxVal = one;
						distOutFull = _mm_max_ps(_mm_min_ps(distInFull, maxVal), minVal);
						break;
					}
					case SlewDistortionType::Tanh: // Tanh => 2 / (1 + exp(-2x)) - 1
					{
						// clamp to [-80.0f, 80.0f] since float std::exp breaks outside of those bounds
						__m128 clampedInput = _mm_max_ps(_mm_min_ps(_mm_mul_ps(_mm_set1_ps(-2.0f),
							distInFull), _mm_set1_ps(80.0f)), _mm_set1_ps(-80.0f));
						__m128 expResult = fastExp(clampedInput);
						distOutFull = _mm_sub_ps(_mm_div_ps(_mm_set1_ps(2.0f), _mm_add_ps(one, expResult)), one);
						break;
					}
					case SlewDistortionType::FastSoftClip1: // Fast Soft Clip 1 => x / (1 + x^2 / 4)
					{
						__m128 temp = _mm_max_ps(_mm_min_ps(distInFull, _mm_set1_ps(2.f)), _mm_set1_ps(-2.f));// clamp
						distOutFull = _mm_div_ps(temp, _mm_add_ps(one,
							_mm_mul_ps(_mm_set1_ps(0.25f), _mm_mul_ps(temp, temp))));
						break;
					}
					case SlewDistortionType::FastSoftClip2: // Fast Soft Clip 2 => x - (4/27) * x^3
					{
						__m128 temp = _mm_max_ps(_mm_min_ps(distInFull, _mm_set1_ps(1.5f)), _mm_set1_ps(-1.5f));// clamp
						distOutFull = _mm_sub_ps(temp, _mm_mul_ps(_mm_set1_ps(4.f / 27.f),
							_mm_mul_ps(_mm_mul_ps(temp, temp), temp)));
						break;
					}
					case SlewDistortionType::Sinusoidal: // Sinusoidal => sin(x)
					{
						// SSE2 sine approximation I created
						__m128 pi = _mm_set1_ps(std::numbers::pi_v<float>);
						__m128 piOverTwo = _mm_set1_ps(std::numbers::pi_v<float> * 0.5f);
						__m128 tau = _mm_set1_ps(std::numbers::pi_v<float> * 2.f);

						__m128 distMinusPiOverTwo = _mm_sub_ps(distInFull, piOverTwo);
						__m128 divByTwoPi = _mm_div_ps(distMinusPiOverTwo, tau);

						// SSE2 floor replacement
						__m128 floorDivByTwoPi = sse2Floor(divByTwoPi);

						// x mod 2pi = x - floor(x / 2pi) * 2pi
						__m128 floorMulTwoPi = _mm_mul_ps(floorDivByTwoPi, tau);
						__m128 modInput = _mm_sub_ps(distMinusPiOverTwo, floorMulTwoPi);

						// abs(in - pi) - pi/2
						__m128 x = _mm_sub_ps(sse2Abs(_mm_sub_ps(modInput, pi)), piOverTwo);

						// polynomial sine approximation
						// sin(x) ≈ x - x^3 / 6 + x^5 / 120
						__m128 x2 = _mm_mul_ps(x, x);
						__m128 x3 = _mm_mul_ps(x2, x);
						__m128 x5 = _mm_mul_ps(x3, x2);
						__m128 sinApprox = _mm_sub_ps(x, _mm_mul_ps(x3, _mm_set1_ps(1.0f / 6.0f)));
						distOutFull = _mm_add_ps(sinApprox, _mm_mul_ps(x5, _mm_set1_ps(1.0f / 120.0f)));
						break;
					}
					case SlewDistortionType::Foldback: // Foldback => |(|x - 1| mod 4) - 2| - 1 = |2 - |(x - 1) - 4 * floor((x - 1) / 4)|| - 1
					{
						__m128 four = _mm_set1_ps(4.0f);
						__m128 distInMinusOne = _mm_sub_ps(distInFull, one);
						__m128 divByFour = _mm_div_ps(distInMinusOne, four);
						
						// floor
						__m128 floorOverFour = sse2Floor(divByFour);

						distOutFull = _mm_sub_ps(sse2Abs(_mm_sub_ps(_mm_sub_ps(
							distInMinusOne, _mm_mul_ps(floorOverFour, four)), _mm_set1_ps(2.0f))), one);
						break;
					}
					case SlewDistortionType::FullRectify: // |x|
					{
						distOutFull = sse2Abs(distInFull);
						break;
					}
					case SlewDistortionType::SmoothRectify: // sqrt(x^2 + 0.04) - 0.2
					{
						distOutFull = _mm_sub_ps(_mm_sqrt_ps(_mm_add_ps(_mm_mul_ps(distInFull, distInFull),
							_mm_set1_ps(0.04f))), _mm_set1_ps(0.2f));
						break;
					}
					case SlewDistortionType::HalfRectify:  // max(0, x)
					{
						distOutFull = _mm_max_ps(_mm_setzero_ps(), distInFull);
						break;
					}
					case SlewDistortionType::Bitcrush:  // round(x / drive * scale) / scale
					{
						// scale = 16 / drive
						__m128 scale = _mm_div_ps(_mm_set1_ps(16.f), drive);
						__m128 scaledVal = _mm_mul_ps(_mm_div_ps(distInFull, drive), scale);

						// round to nearest, half away from zero
						__m128 rounded = sse2Round(scaledVal);

						distOutFull = _mm_div_ps(rounded, scale);
						break;
					}
					default:
					{
						distOutFull = distInFull;
						break;
					}
				}

				if (loopCount == 1)// we can store all four simultaneously
				{
					_mm_store_ps(&distOutArr[0], distOutFull);
					break;
				}
				else// need to store two at a time
				{
					if (pair == 0)
					{
						// for elements 0 and 1
						_mm_storel_pi(reinterpret_cast<__m64*>(&distOutArr[0]), distOutFull);
					}
					else
					{
						// for elements 2 and 3
						_mm_storeh_pi(reinterpret_cast<__m64*>(&distOutArr[2]), distOutFull);
					}
				}
			}

			__m128 distOut = _mm_load_ps(&distOutArr[0]);

			// (1 - warp) * distOut + std::copysign(warp, biasedIn)
			__m128 distOutScaled = _mm_add_ps(_mm_mul_ps(distOut, _mm_sub_ps(one, warp)), _mm_or_ps(warp, signBiasedIn));

			// if (abs(biasedIn) < warp / crush) {distOut = biasedIn * crush;}
			__m128 absBiasedIn = sse2Abs(biasedIn);
			__m128 condition = _mm_cmplt_ps(absBiasedIn, _mm_div_ps(warp, crush));
			__m128 biasedInCrush = _mm_mul_ps(biasedIn, crush);

			distOut = _mm_or_ps(_mm_and_ps(condition, biasedInCrush), _mm_andnot_ps(condition, distOutScaled));

			// DC offset calculation
			__m128 dcOffset = _mm_load_ps(&m_dcOffset[0]);
			__m128 dcCoeff  = _mm_set1_ps(m_dcCoeff);
			dcOffset = _mm_add_ps(_mm_mul_ps(dcOffset, dcCoeff), _mm_mul_ps(distOut, _mm_sub_ps(one, dcCoeff)));

			__m128 distOutMinusDC = _mm_sub_ps(distOut, dcOffset);

			// even with DC offset removal disabled, we should still apply it for the envelope follower
			__m128 outEnv = _mm_load_ps(&m_outEnv[0]);
			__m128 absOut = sse2Abs(distOutMinusDC);

			cmp = _mm_cmpgt_ps(absOut, outEnv);
			__m128 outEnvRise = _mm_add_ps(_mm_mul_ps(outEnv, attack), _mm_mul_ps(absOut, attackInv));
			__m128 outEnvFall = _mm_add_ps(_mm_mul_ps(outEnv, release), _mm_mul_ps(absOut, releaseInv));
			outEnv = _mm_max_ps(_mm_or_ps(_mm_and_ps(cmp, outEnvRise), _mm_andnot_ps(cmp, outEnvFall)), minFloor);

			// remove DC
			__m128 finalDistOut = (dcRemove) ? distOutMinusDC : distOut;

			// crossfade between a multiplier of 1 and (inEnv/outEnv) for dynamics feature
			__m128 distDyn = _mm_mul_ps(finalDistOut, _mm_add_ps(one,
				_mm_mul_ps(_mm_sub_ps(_mm_div_ps(inEnv, outEnv), one), dynamics)));

			// apply mix
			__m128 outFinal = _mm_mul_ps(_mm_add_ps(in, _mm_mul_ps(mix, _mm_sub_ps(distDyn, in))), outVol);

			// store volume for display
			__m128 outAbs = sse2Abs(outFinal);
			_mm_store_ps(&m_outPeakDisplay[0], _mm_max_ps(_mm_load_ps(&m_outPeakDisplay[0]), outAbs));

			// write updated stuff back into member variables
			_mm_store_ps(&m_inEnv[0], inEnv);
			_mm_store_ps(&m_slewOut[0], slewOut);
			_mm_store_ps(&m_dcOffset[0], dcOffset);
			_mm_store_ps(&m_outEnv[0], outEnv);

			alignas(16) std::array<float, 4> outArr;
			_mm_store_ps(&outArr[0], outFinal);

			m_overOuts[0][overSamp] = outArr[0] + outArr[2];
			m_overOuts[1][overSamp] = outArr[1] + outArr[3];
		}

		std::array<float, 2> s;
		if (oversampleVal > 1)
		{
			s[0] = m_downsampler[0].processSample(m_overOuts[0].data());
			s[1] = m_downsampler[1].processSample(m_overOuts[1].data());
		}
		else
		{
			s[0] = m_overOuts[0][0];
			s[1] = m_overOuts[1][0];
		}

		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];
	}

	return ProcessStatus::ContinueIfNotQuiet;
}



#else
Effect::ProcessStatus SlewDistortion::processImpl(SampleFrame* buf, const fpp_t frames)
{
	const float d = dryLevel();
	const float w = wetLevel();
	
	const int oversampling = m_slewdistortionControls.m_oversamplingModel.value();
	const int oversampleVal = 1 << oversampling;
	if (oversampleVal != m_oldOversampleVal)
	{
		m_oldOversampleVal = oversampleVal;
		changeSampleRate();
	}
	
	const SlewDistortionType distType1 = static_cast<SlewDistortionType>(m_slewdistortionControls.m_distType1Model.value());
	const SlewDistortionType distType2 = static_cast<SlewDistortionType>(m_slewdistortionControls.m_distType2Model.value());
	const float drive1 = dbfsToAmp(m_slewdistortionControls.m_drive1Model.value());
	const float drive2 = dbfsToAmp(m_slewdistortionControls.m_drive2Model.value());
	const float slewUp1 = dbfsToAmp(m_slewdistortionControls.m_slewUp1Model.value()) / oversampleVal;
	const float slewUp2 = dbfsToAmp(m_slewdistortionControls.m_slewUp2Model.value()) / oversampleVal;
	const float slewDown1 = dbfsToAmp(m_slewdistortionControls.m_slewDown1Model.value()) / oversampleVal;
	const float slewDown2 = dbfsToAmp(m_slewdistortionControls.m_slewDown2Model.value()) / oversampleVal;
	const float bias1 = m_slewdistortionControls.m_bias1Model.value();
	const float bias2 = m_slewdistortionControls.m_bias2Model.value();
	const float warp1 = m_slewdistortionControls.m_warp1Model.value();
	const float warp2 = m_slewdistortionControls.m_warp2Model.value();
	const float crush1 = dbfsToAmp(m_slewdistortionControls.m_crush1Model.value());
	const float crush2 = dbfsToAmp(m_slewdistortionControls.m_crush2Model.value());
	const float attack1 = msToCoeff(m_slewdistortionControls.m_attack1Model.value());
	const float attack2 = msToCoeff(m_slewdistortionControls.m_attack2Model.value());
	const float attackInv1 = 1.f - attack1;
	const float attackInv2 = 1.f - attack2;
	const float release1 = msToCoeff(m_slewdistortionControls.m_release1Model.value());
	const float release2 = msToCoeff(m_slewdistortionControls.m_release2Model.value());
	const float releaseInv1 = 1.f - release1;
	const float releaseInv2 = 1.f - release2;
	const float dynamics1 = m_slewdistortionControls.m_dynamics1Model.value();
	const float dynamics2 = m_slewdistortionControls.m_dynamics2Model.value();
	const float dynamicSlew1 = m_slewdistortionControls.m_dynamicSlew1Model.value();
	const float dynamicSlew2 = m_slewdistortionControls.m_dynamicSlew2Model.value();
	const float outVol1 = dbfsToAmp(m_slewdistortionControls.m_outVol1Model.value());
	const float outVol2 = dbfsToAmp(m_slewdistortionControls.m_outVol2Model.value());
	const float split = m_slewdistortionControls.m_splitModel.value();
	const bool dcRemove = m_slewdistortionControls.m_dcRemoveModel.value();
	const bool multiband = m_slewdistortionControls.m_multibandModel.value();
	const float mix1 = m_slewdistortionControls.m_mix1Model.value();
	const float mix2 = m_slewdistortionControls.m_mix2Model.value();
	const bool slewLink1 = m_slewdistortionControls.m_slewLink1Model.value();
	const bool slewLink2 = m_slewdistortionControls.m_slewLink2Model.value();

	std::array<float, 4> in = {0};
	std::array<float, 4> out = {0};
	const std::array<float, 4> drive = {drive1, drive1, drive2, drive2};
	const std::array<float, 4> slewUp = {slewUp1, slewUp1, slewUp2, slewUp2};
	const std::array<float, 4> slewDown = {slewDown1, slewDown1, slewDown2, slewDown2};
	const std::array<SlewDistortionType, 4> distType = {distType1, distType1, distType2, distType2};
	const std::array<float, 4> warp = {warp1, warp1, warp2, warp2};
	const std::array<float, 4> crush = {crush1, crush1, crush2, crush2};
	const std::array<float, 4> outVol = {outVol1, outVol1, outVol2, outVol2};
	const std::array<float, 4> attack = {attack1, attack1, attack2, attack2};
	const std::array<float, 4> attackInv = {attackInv1, attackInv1, attackInv2, attackInv2};
	const std::array<float, 4> release = {release1, release1, release2, release2};
	const std::array<float, 4> releaseInv = {releaseInv1, releaseInv1, releaseInv2, releaseInv2};
	const std::array<float, 4> dynamics = {dynamics1, dynamics1, dynamics2, dynamics2};
	const std::array<float, 4> dynamicSlew = {dynamicSlew1, dynamicSlew1, dynamicSlew2, dynamicSlew2};
	const std::array<float, 4> mix = {mix1, mix1, mix2, mix2};
	const std::array<bool, 4> slewLink = {slewLink1, slewLink1, slewLink2, slewLink2};
	
	if (m_slewdistortionControls.m_splitModel.isValueChanged())
	{
		m_lp.setLowpass(split);
		m_hp.setHighpass(split);
	}
	
	for (fpp_t f = 0; f < frames; ++f)
	{
		// interpolate bias to remove crackling when moving the parameter
		m_trueBias1 = m_biasInterpCoef * m_trueBias1 + (1.f - m_biasInterpCoef) * bias1;
		m_trueBias2 = m_biasInterpCoef * m_trueBias2 + (1.f - m_biasInterpCoef) * bias2;
		const std::array<float, 4> bias = {m_trueBias1, m_trueBias1, m_trueBias2, m_trueBias2};
		
		if (oversampleVal > 1)
		{
			m_upsampler[0].processSample(m_overOuts[0].data(), buf[f][0]);
			m_upsampler[1].processSample(m_overOuts[1].data(), buf[f][1]);
		}
		else
		{
			m_overOuts[0][0] = buf[f][0];
			m_overOuts[1][0] = buf[f][1];
		}
		
		for (int overSamp = 0; overSamp < oversampleVal; ++overSamp)
		{
			if (multiband)
			{
				in[0] = m_hp.update(m_overOuts[0][overSamp], 0);
				in[1] = m_hp.update(m_overOuts[1][overSamp], 1);
				in[2] = m_lp.update(m_overOuts[0][overSamp], 0);
				in[3] = m_lp.update(m_overOuts[1][overSamp], 1);
			}
			else
			{
				in[0] = m_overOuts[0][overSamp];
				in[1] = m_overOuts[1][overSamp];
				in[2] = 0;
				in[3] = 0;
			}
			
			m_inPeakDisplay[0] = std::max(m_inPeakDisplay[0], std::abs(in[0] * drive[0]));
			m_inPeakDisplay[1] = std::max(m_inPeakDisplay[1], std::abs(in[1] * drive[1]));
			m_inPeakDisplay[2] = std::max(m_inPeakDisplay[2], std::abs(in[2] * drive[2]));
			m_inPeakDisplay[3] = std::max(m_inPeakDisplay[3], std::abs(in[3] * drive[3]));
			
			for (int i = 0; i < 4 - !multiband * 2; ++i) {
				const float absIn = std::abs(in[i]);
				m_inEnv[i] = absIn > m_inEnv[i] ? m_inEnv[i] * attack[i] + absIn * attackInv[i] : m_inEnv[i] * release[i] + absIn * releaseInv[i];
				m_inEnv[i] = std::max(m_inEnv[i], SLEW_DISTORTION_MIN_FLOOR);
			
				float rate = in[i] - m_slewOut[i];
				float slewMult = dynamicSlew[i] ? std::pow(m_inEnv[i], dynamicSlew[i]) : 1.f;
				const float trueSlew = ((rate >= 0 || slewLink[i]) ? slewUp[i] : slewDown[i]) * slewMult;
				rate = std::clamp(rate, -trueSlew, trueSlew);
				m_slewOut[i] = m_slewOut[i] + rate;
				
				float biasedIn = m_slewOut[i] * drive[i] + bias[i];
				float distIn = (biasedIn - std::copysign(warp[i] / crush[i], biasedIn)) / (1.f - warp[i]);
				float distOut;
				switch (static_cast<SlewDistortionType>(distType[i]))
				{
					case SlewDistortionType::HardClip: {
						distOut = std::clamp(distIn, -1.f, 1.f);
						break;
					}
					case SlewDistortionType::Tanh: {
						const float temp = std::clamp(distIn, -40.f, 40.f);
						distOut = 2.f / (1.f + std::exp(-2.f * temp)) - 1;
						break;
					}
					case SlewDistortionType::FastSoftClip1: {
						const float temp = std::clamp(distIn, -2.f, 2.f);
						distOut = temp / (1 + 0.25f * temp * temp);
						break;
					}
					case SlewDistortionType::FastSoftClip2: {
						const float temp = std::clamp(distIn, -1.5f, 1.5f);
						distOut = temp - (4.f / 27.f) * temp * temp * temp;
						break;
					}
					case SlewDistortionType::Sinusoidal: {
						// using a polynomial approximation so it matches with the SSE2 code
						// x - x^3 / 6 + x^5 / 120
						float modInput = std::fmod(distIn - std::numbers::pi_v<float> * 0.5f, 2.f * std::numbers::pi_v<float>);
						if (modInput < 0) {modInput += 2.f * std::numbers::pi_v<float>;}
						const float x = std::abs(modInput - std::numbers::pi_v<float>) - std::numbers::pi_v<float> * 0.5f;
						const float x2 = x * x;
						const float x3 = x2 * x;
						const float x5 = x3 * x2;
						distOut = x - (x3 / 6.0f) + (x5 / 120.0f);
						break;
					}
					case SlewDistortionType::Foldback: {
						distOut = std::abs(std::abs(std::fmod(distIn - 1.f, 4.f)) - 2.f) - 1.f;
						break;
					}
					case SlewDistortionType::FullRectify: {
						distOut = std::abs(distIn);
						break;
					}
					case SlewDistortionType::SmoothRectify:
					{
						distOut = std::sqrt(distIn * distIn + 0.04f) - 0.2f;
						break;
					}
					case SlewDistortionType::HalfRectify:
					{
						distOut = std::max(0.0f, distIn);
						break;
					}
					case SlewDistortionType::Bitcrush:
					{
						const float scale = 16 / drive[i];
						distOut = std::round(distIn / drive[i] * scale) / scale;
						break;
					}
					default:
					{
						distOut = distIn;
					}
				}
				distOut = distOut * (1.f - warp[i]) + std::copysign(warp[i], biasedIn);
				if (std::abs(biasedIn) < warp[i] / crush[i]) {distOut = biasedIn * crush[i];}
				
				m_dcOffset[i] = m_dcOffset[i] * m_dcCoeff + distOut * (1.f - m_dcCoeff);
				
				// even with DC offset removal disabled, we should still apply it for the envelope follower
				const float absOut = std::abs(distOut - m_dcOffset[i]);
				m_outEnv[i] = absOut > m_outEnv[i] ? m_outEnv[i] * attack[i] + absOut * attackInv[i] : m_outEnv[i] * release[i] + absOut * releaseInv[i];
				m_outEnv[i] = std::max(m_outEnv[i], SLEW_DISTORTION_MIN_FLOOR);
				
				if (dcRemove) { distOut -= m_dcOffset[i]; }
				
				distOut *= std::lerp(1.f, m_inEnv[i] / m_outEnv[i], dynamics[i]);
				
				out[i] = std::lerp(in[i], distOut, mix[i]) * outVol[i];
			}
			
			m_outPeakDisplay[0] = std::max(m_outPeakDisplay[0], std::abs(out[0]));
			m_outPeakDisplay[1] = std::max(m_outPeakDisplay[1], std::abs(out[1]));
			m_outPeakDisplay[2] = std::max(m_outPeakDisplay[2], std::abs(out[2]));
			m_outPeakDisplay[3] = std::max(m_outPeakDisplay[3], std::abs(out[3]));
			
			m_overOuts[0][overSamp] = out[0] + out[2];
			m_overOuts[1][overSamp] = out[1] + out[3];
		}
		
		std::array<float, 2> s;
		if (oversampleVal > 1)
		{
			s[0] = m_downsampler[0].processSample(m_overOuts[0].data());
			s[1] = m_downsampler[1].processSample(m_overOuts[1].data());
		}
		else
		{
			s[0] = m_overOuts[0][0];
			s[1] = m_overOuts[1][0];
		}
		
		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];
	}

	return ProcessStatus::ContinueIfNotQuiet;
}
#endif

void SlewDistortion::changeSampleRate()
{
	m_sampleRate = Engine::audioEngine()->outputSampleRate();
	const int oversampleStages = m_slewdistortionControls.m_oversamplingModel.value();
	const int oversampleVal = 1 << oversampleStages;
	float sampleRateOver = m_sampleRate * oversampleVal;
	
	for (int i = 0; i < 2; ++i)
	{
		m_upsampler[i].setup(oversampleStages, m_sampleRate);
		m_downsampler[i].setup(oversampleStages, m_sampleRate);
	}
	
	m_lp.setSampleRate(sampleRateOver);
	m_lp.setLowpass(m_slewdistortionControls.m_splitModel.value());
	m_lp.clearHistory();
	
	m_hp.setSampleRate(sampleRateOver);
	m_hp.setHighpass(m_slewdistortionControls.m_splitModel.value());
	m_hp.clearHistory();
	
	m_coeffPrecalc = -1.f / (sampleRateOver * 0.001f);
	
	m_dcCoeff = std::exp(-2.f * std::numbers::pi_v<float> * SLEW_DISTORTION_DC_FREQ / sampleRateOver);
	
	std::fill(std::begin(m_inPeakDisplay), std::end(m_inPeakDisplay), 0.0f);
	std::fill(std::begin(m_slewOut), std::end(m_slewOut), 0.0f);
	std::fill(std::begin(m_dcOffset), std::end(m_dcOffset), 0.0f);
	std::fill(std::begin(m_inEnv), std::end(m_inEnv), 0.0f);
	std::fill(std::begin(m_outEnv), std::end(m_outEnv), 0.0f);
	std::fill(std::begin(m_outPeakDisplay), std::end(m_outPeakDisplay), 0.0f);
	for (auto& subArray : m_overOuts) {std::fill(subArray.begin(), subArray.end(), 0.0f);}
	
	m_biasInterpCoef = std::exp(-1 / (0.01f * m_sampleRate));
}


extern "C"
{
// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data)
{
	return new SlewDistortion(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
}
}

} // namespace lmms
