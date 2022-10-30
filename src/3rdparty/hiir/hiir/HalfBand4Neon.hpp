/*****************************************************************************

        HalfBand4Neon.hpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_HalfBand4Neon_CODEHEADER_INCLUDED)
#define hiir_HalfBand4Neon_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/fnc_neon.h"
#include "hiir/StageProc4Neon.h"

#include <cassert>



namespace hiir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int 	HalfBand4Neon <NC>::_nbr_chn;
template <int NC>
constexpr int 	HalfBand4Neon <NC>::NBR_COEFS;
template <int NC>
constexpr double	HalfBand4Neon <NC>::_delay;



/*
==============================================================================
Name: ctor
Throws: Nothing
==============================================================================
*/

template <int NC>
HalfBand4Neon <NC>::HalfBand4Neon () noexcept
:	_bifilter ()
,	_phase (0)
{
	for (int i = 0; i < NBR_COEFS + 2; ++i)
	{
		storea (_bifilter [0] [i]._coef, vdupq_n_f32 (0));
		storea (_bifilter [1] [i]._coef, vdupq_n_f32 (0));
	}

	clear_buffers ();
}



/*
==============================================================================
Name: set_coefs
Description:
	Sets filter coefficients. Generate them with the PolyphaseIir2Designer
	class.
	Call this function before doing any processing.
Input parameters:
	- coef_arr: Array of coefficients. There should be as many coefficients as
		mentioned in the class template parameter.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBand4Neon <NC>::set_coefs (const double coef_arr []) noexcept
{
	assert (coef_arr != nullptr);

	for (int i = 0; i < NBR_COEFS; ++i)
	{
		const auto     c = vdupq_n_f32 (DataType (coef_arr [i]));
		storea (_bifilter [0] [i + 2]._coef, c);
		storea (_bifilter [1] [i + 2]._coef, c);
	}
}



/*
==============================================================================
Name: process_sample
Description:
	Performs a half-band low-pass filtering on a multichannel vector.
Input parameters:
	- in: sample from the stream to be filtered.
Returns: filtered vector.
Throws: Nothing
==============================================================================
*/

template <int NC>
float32x4_t	HalfBand4Neon <NC>::process_sample (float32x4_t input) noexcept
{
	const auto     path_arr = process_2_paths (input);
	const auto     low      = path_arr._e + path_arr._o;

	return low;
}



/*
==============================================================================
Name: process_block
Description:
	Performs a half-band low-pass filtering on a block of multichannel vectors.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl vectors.
	- nbr_spl: Number of vectors to output, > 0
Output parameters:
	- out_ptr: Array for the output vectors, capacity: nbr_spl vectors.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBand4Neon <NC>::process_block (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (out_ptr != nullptr);
	assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl * _nbr_chn);

	process_block_2_paths (
		out_ptr, nullptr, in_ptr, nbr_spl, store_low, bypass
	);
}



/*
==============================================================================
Name: process_sample_hpf
Description:
	Performs a half-band high-pass filtering on a multichannel vector.
Input parameters:
	- in: sample from the stream to be filtered.
Returns: filtered vector.
Throws: Nothing
==============================================================================
*/

template <int NC>
float32x4_t	HalfBand4Neon <NC>::process_sample_hpf (float32x4_t input) noexcept
{
	const auto     path_arr = process_2_paths (input);
	const auto     high     = path_arr._e - path_arr._o;

	return high;
}



/*
==============================================================================
Name: process_block_hpf
Description:
	Performs a half-band high-pass filtering on a block of multichannel
	vectors.
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl vectors.
	- nbr_spl: Number of vectors to output, > 0
Output parameters:
	- out_ptr: Array for the output vectors, capacity: nbr_spl vectors.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBand4Neon <NC>::process_block_hpf (float out_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (out_ptr != nullptr);
	assert (out_ptr <= in_ptr || out_ptr >= in_ptr + nbr_spl * _nbr_chn);

	process_block_2_paths (
		nullptr, out_ptr, in_ptr, nbr_spl, bypass, store_high
	);
}



/*
==============================================================================
Name: process_sample_split
Description:
	Splits (spectrum-wise) in half a vector from a 4-channel stream. Both part
	are results of a low-pass and a high-pass filtering, equivalent to the
	output 	of process_sample() and process_sample_hpf().
Input parameters:
	- input: sample from the stream to be filtered.
Output parameters:
	- low: output sample, lower part of the spectrum.
	- high: output sample, higher part of the spectrum.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBand4Neon <NC>::process_sample_split (float32x4_t &low, float32x4_t &high, float32x4_t input) noexcept
{
	const auto     path_arr = process_2_paths (input);
	low  = path_arr._e + path_arr._o;
	high = path_arr._e - path_arr._o;
}



/*
==============================================================================
Name: process_block_split
Description:
	Splits (spectrum-wise) in half a block of 4-channel vectors. Both part are
	results of a low-pass and a high-pass filtering, equivalent to the output
	of process_sample() and process_sample_hpf().
	Input and output blocks may overlap, see assert() for details.
Input parameters:
	- in_ptr: Input array, containing nbr_spl vectors.
	- nbr_spl: Number of vectors to output, > 0
Output parameters:
	- out_l_ptr: Array for the output samples, lower part of the spectrum
		Capacity: nbr_spl vectors.
	- out_h_ptr: Array for the output samples, higher part of the spectrum.
		Capacity: nbr_spl vectors.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBand4Neon <NC>::process_block_split (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl) noexcept
{
	assert (out_l_ptr != nullptr);
	assert (out_h_ptr != nullptr);
	assert (out_l_ptr <= in_ptr || out_l_ptr >= in_ptr + nbr_spl * _nbr_chn);
	assert (out_h_ptr <= in_ptr || out_h_ptr >= in_ptr + nbr_spl * _nbr_chn);
	assert (   out_l_ptr + nbr_spl * _nbr_chn <= out_h_ptr
	        || out_h_ptr + nbr_spl * _nbr_chn <= out_l_ptr);

	process_block_2_paths (
		out_l_ptr, out_h_ptr, in_ptr, nbr_spl, store_low, store_high
	);
}



/*
==============================================================================
Name: clear_buffers
Description:
	Clears filter memory, as if it processed silence since an infinite amount
	of time.
Throws: Nothing
==============================================================================
*/

template <int NC>
void	HalfBand4Neon <NC>::clear_buffers () noexcept
{
	_phase = 0;
	for (int i = 0; i < NBR_COEFS + 2; ++i)
	{
		storea (_bifilter [0] [i]._mem, vdupq_n_f32 (0));
		storea (_bifilter [1] [i]._mem, vdupq_n_f32 (0));
	}
	storea (_prev, vdupq_n_f32 (0));
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <int NC>
constexpr int	HalfBand4Neon <NC>::_nbr_phases;



// Shared processing function, outputs both paths of the all-pass filter pair.
// Returns: even path, odd path
template <int NC>
typename HalfBand4Neon <NC>::EvenOdd	HalfBand4Neon <NC>::process_2_paths (float32x4_t input) noexcept
{
	const auto     half = vdupq_n_f32 (0.5f);
	input *= half;

	auto           path_arr = EvenOdd { input, load4a (_prev) };

	StageProc4Neon <NBR_COEFS>::process_sample_pos (
		NBR_COEFS, path_arr._e, path_arr._o, &_bifilter [_phase] [0]
	);

	storea (_prev, input);
	_phase = 1 - _phase;

	return path_arr;
}



// One of the out_l_ptr or out_h_ptr can be nullptr
template <int NC>
template <typename FL, typename FH>
void	HalfBand4Neon <NC>::process_block_2_paths (float out_l_ptr [], float out_h_ptr [], const float in_ptr [], long nbr_spl, FL fnc_l, FH fnc_h) noexcept
{
	assert (in_ptr != nullptr);
	assert (nbr_spl > 0);

	long           pos  = 0;
	const auto     half = vdupq_n_f32 (0.5f);

	if (_phase == 1)
	{
		const auto     path_arr = process_2_paths (load4u (in_ptr));
		fnc_l (out_l_ptr, path_arr._e, path_arr._o);
		fnc_h (out_h_ptr, path_arr._e, path_arr._o);
		++ pos;
	}

	const long     end  = ((nbr_spl - pos) & -_nbr_phases) + pos;
	float32x4_t         prev = load4a (_prev);
	while (pos < end)
	{
		const auto     ofs_0   = pos * _nbr_chn;
		const auto     ofs_1   = ofs_0 + _nbr_chn;
		const auto     input_0 = load4u (in_ptr + ofs_0) * half;
		const auto     input_1 = load4u (in_ptr + ofs_1) * half;

		auto           tmp_0   = input_0;
		auto           tmp_1   = prev;
		StageProc4Neon <NBR_COEFS>::process_sample_pos (
			NBR_COEFS, tmp_0, tmp_1, &_bifilter [0] [0]
		);

		auto           tmp_2 = input_1;
		auto           tmp_3 = input_0;  // prev
		StageProc4Neon <NBR_COEFS>::process_sample_pos (
			NBR_COEFS, tmp_2, tmp_3, &_bifilter [1] [0]
		);

		fnc_l (out_l_ptr + ofs_0, tmp_0, tmp_1);
		fnc_h (out_h_ptr + ofs_0, tmp_0, tmp_1);
		fnc_l (out_l_ptr + ofs_1, tmp_2, tmp_3);
		fnc_h (out_h_ptr + ofs_1, tmp_2, tmp_3);

		prev = input_1;
		pos += 2;
	}
	storea (_prev, prev);

	if (pos < nbr_spl)
	{
		assert (pos + 1 == nbr_spl);
		const auto     ofs      = pos * _nbr_chn;
		const auto     path_arr = process_2_paths (load4u (in_ptr + ofs));
		fnc_l (out_l_ptr + ofs, path_arr._e, path_arr._o);
		fnc_h (out_h_ptr + ofs, path_arr._e, path_arr._o);
	}
}



template <int NC>
void	HalfBand4Neon <NC>::store_low (float *ptr, float32x4_t tmp_0, float32x4_t tmp_1) noexcept
{
	const auto     low = tmp_0 + tmp_1;
	storeu (ptr, low);
}



template <int NC>
void	HalfBand4Neon <NC>::store_high (float *ptr, float32x4_t tmp_0, float32x4_t tmp_1) noexcept
{
	const auto     high = tmp_0 - tmp_1;
	storeu (ptr, high);
}



}  // namespace hiir



#endif   // hiir_HalfBand4Neon_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

