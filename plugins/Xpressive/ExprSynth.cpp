/*
 * ExprSynth.cpp - Implementation of a Frontend to ExprTk
 *
 * Copyright (c) 2016-2017 Orr Dvori
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


#include "ExprSynth.h"

#include <string>
#include <vector>
#include <cmath>
#include <random>


#include "interpolation.h"
#include "lmms_math.h"
#include "NotePlayHandle.h"
#include "SampleFrame.h"


#include <exprtk.hpp>

#define WARN_EXPRTK qWarning("ExprTk exception")

namespace lmms
{

using symbol_table_t = exprtk::symbol_table<float>;
using expression_t = exprtk::expression<float>;
using parser_t = exprtk::parser<float>;

template <typename T,typename Functor,bool optimize>
struct freefunc0 : public exprtk::ifunction<T>
{
	using exprtk::ifunction<T>::operator();

	freefunc0() : exprtk::ifunction<T>(0) {
		if (optimize) { exprtk::disable_has_side_effects(*this); }
	}
	inline T operator()() override
	{ return Functor::process(); }
};
template <typename T,typename Functor,bool optimize>
struct freefunc1 : public exprtk::ifunction<T>
{
	using exprtk::ifunction<T>::operator();

	freefunc1() : exprtk::ifunction<T>(1) {
		if (optimize) { exprtk::disable_has_side_effects(*this); }
	}
	inline T operator()(const T& x) override
	{ return Functor::process(x); }
};

template <typename T>
struct IntegrateFunction : public exprtk::ifunction<T>
{

	using exprtk::ifunction<T>::operator();
	~IntegrateFunction() override
	{
		delete [] m_counters;
	}

	IntegrateFunction(const unsigned int* frame, unsigned int sample_rate,unsigned int max_counters) :
	exprtk::ifunction<T>(1),
	m_firstValue(0),
	m_frame(frame),
	m_sampleRate(sample_rate),
	m_maxCounters(max_counters),
	m_nCounters(0),
	m_nCountersCalls(0),
	m_cc(0)
	{
		m_counters=new double[max_counters];
		clearArray(m_counters,max_counters);
	}

	inline T operator()(const T& x) override
	{
		if (m_frame)
		{
			if (m_nCountersCalls == 0)
			{
				m_firstValue = *m_frame;
			}
			if (m_firstValue == *m_frame)
			{
				++m_nCountersCalls;
				if (m_nCountersCalls > m_maxCounters)
				{
					return 0;
				}
				m_cc = m_nCounters;
				++m_nCounters;
			}
			else // we moved to the next frame
			{
				m_frame = 0; // this will indicate that we are no longer in init phase.
			}
		}

		T res = 0;
		if (m_cc < m_nCounters)
		{
			res = m_counters[m_cc];
			m_counters[m_cc] += x;
		}
		m_cc = (m_cc + 1) % m_nCountersCalls;
		return res / m_sampleRate;
	}
	unsigned int m_firstValue;
	const unsigned int* m_frame;
	const unsigned int m_sampleRate;
	// number of counters allocated
	const unsigned int m_maxCounters;
	// number of integrate instances that has counters allocated
	unsigned int m_nCounters;
	// real number of integrate instances
	unsigned int m_nCountersCalls;
	unsigned int m_cc;
	double *m_counters;
};

template <typename T>
struct LastSampleFunction : public exprtk::ifunction<T>
{

	using exprtk::ifunction<T>::operator();
	~LastSampleFunction() override
	{
		delete [] m_samples;
	}

	LastSampleFunction(unsigned int history_size) :
	exprtk::ifunction<T>(1),
	m_history_size(history_size),
	m_pivot_last(history_size - 1)
	{
		m_samples = new T[history_size];
		clearArray(m_samples, history_size);
	}

	inline T operator()(const T& x) override
	{
		if (!std::isnan(x) && x >= 1 && x <= m_history_size) {
			return m_samples[(static_cast<std::size_t>(x) + m_pivot_last) % m_history_size];
		}
		return 0;
	}
	void setLastSample(const T& sample)
	{
		if (!std::isnan(sample) && !std::isinf(sample))
		{
			m_samples[m_pivot_last] = sample;
		}
		if (m_pivot_last == 0)
		{
			m_pivot_last = m_history_size - 1;
		}
		else {
			--m_pivot_last;
		}
	}
	unsigned int m_history_size;
	unsigned int m_pivot_last;
	T *m_samples;
};

template <typename T>
struct WaveValueFunction : public exprtk::ifunction<T>
{
	using exprtk::ifunction<T>::operator();

	WaveValueFunction(const T* v, std::size_t s)
	: exprtk::ifunction<T>(1),
	m_vec(v),
	m_size(s)
	{}

	inline T operator()(const T& index) override
	{
		return m_vec[(int) ( positiveFraction(index) * m_size )];
	}
	const T *m_vec;
	const std::size_t m_size;
};
template <typename T>
struct WaveValueFunctionInterpolate : public exprtk::ifunction<T>
{
	using exprtk::ifunction<T>::operator();

	WaveValueFunctionInterpolate(const T* v, std::size_t s)
	: exprtk::ifunction<T>(1),
	m_vec(v),
	m_size(s)
	{}

	inline T operator()(const T& index) override
	{
		const T x = positiveFraction(index) * m_size;
		const int ix = (int)x;
		const float xfrc = fraction(x);
		return linearInterpolate(m_vec[ix], m_vec[(ix + 1) % m_size], xfrc);
	}
	const T *m_vec;
	const std::size_t m_size;
};
static const auto random_data = std::array<unsigned int, 257>{
0xd76a33ec, 0x4a767724, 0xb34ebd08 ,0xf4024196,
0x17b426e2, 0x8dc6389a, 0x1b5dcb93 ,0xa771bd3f,
0x078d502e, 0x8980988a, 0x1f64f846 ,0xb5b48ed7,
0xf0742cfb, 0xe7c66303, 0xc9472876 ,0x6c7494a5,
0x5c2203a1, 0x23986344, 0x7d344fa0 ,0x4f39474a,
0x28ac8b2b, 0x10f779b2, 0x6e79e659 ,0x32e44c52,
0xf790aa55, 0x98b05083, 0xb5d44f1c ,0xe553da04,
0xa884c6d2, 0x43274953, 0xbcb57404 ,0x43f7d32a,
0xf1890f8b, 0x019f4dce, 0x5c4ede33 ,0x2dec1a7e,
0x0f3eab09, 0x2197c93c, 0xae933f42 ,0x80d4b111,
0x6e5bd30a, 0x17139c70, 0xe15f7eb0 ,0x1798f893,
0xe1c6be1c, 0xe21edf9b, 0x4702e081 ,0x8a2cb85a,
0xbf3c1f15, 0x147f4685, 0x9221d731 ,0x3c7580f3,
0xc1c08498, 0x8e198b35, 0xf821c15a ,0x4d3cd2d4,
0xad89a3b7, 0xd48f915f, 0xcaf893f0 ,0xa64a4b8e,
0x20715f54, 0x1ba4de0a, 0x17ac6e91 ,0xd82ea8c0,
0x638a0ba5, 0xe7a76c0f, 0x486c5476 ,0x334bbd0a,
0xffe29c55, 0x7247efaf, 0x15f98e83 ,0x7a4a79ac,
0x350cd175, 0xc7107908, 0xa85c67f7 ,0x9c5002c4,
0x3cf27d2c, 0x314d8450, 0x05552886 ,0x87a73642,
0x827832e4, 0x9412cc67, 0x261979e6 ,0xb31da27f,
0x3e6bbafb, 0x663f1968, 0xd84274e2, 0xdd91d982,
0xd25c4805, 0x9567f860, 0xab99675c, 0x2254733b,
0x18799dd7, 0xee328916, 0xb9419a1b, 0x01b7a66f,
0xbcdc05e1, 0x788de4ae, 0x366e77cf, 0x81a1ebd2,
0x97be859a, 0x17d4b533, 0x22dab3a9, 0xc99871ea,
0xc7502c91, 0x4474b65f, 0x655d059d, 0x0ddc1348,
0x8325909b, 0x4873c155, 0x9fa30438, 0x7250b7a8,
0x90db2715, 0xf65e1cef, 0x41b74cf4, 0x38fba01c,
0xe9eefb40, 0x9e5524ea, 0x1d3fc077, 0x04ec39db,
0x1c0d501c, 0xb93f26d9, 0xf9f910b9, 0x806fce99,
0x5691ffdf, 0x1e63b27a, 0xf2035d42, 0xd3218a0b,
0x12eae6db, 0xeba372a9, 0x6f975260, 0xc514ae91,
0xebddb8ad, 0xc53207c0, 0xdbda57dc, 0x8fb38ef4,
0xfaa4f1bc, 0x40caf49f, 0xcb394b41, 0x424fc698,
0xb79a9ece, 0x331202d6, 0xc604ed4d, 0x5e85819f,
0x67222eda, 0xd976ba71, 0x7d083ec6, 0x040c819e,
0xc762c934, 0xa6684333, 0x2eaccc54, 0x69dc04b9,
0x0499cf36, 0x6351f438, 0x6db2dc34, 0x787ae036,
0x11b5c6ac, 0x552b7227, 0x32a4c993, 0xf7f4c49d,
0x7ac9e2d9, 0xf3d32020, 0x4ff01f89, 0x6f0e60bb,
0x3c6ed445, 0x7ca01986, 0x96901ecf, 0xe10df188,
0x62a6da6d, 0x8deee09f, 0x5347cb66, 0x5249f452,
0x22704d4d, 0x6221555f, 0x6aa0ea90, 0xe1f7bae3,
0xd106626f, 0x6365a9db, 0x1989bb81, 0xfc2daa73,
0x303c60b3, 0xcd867baa, 0x7c5787c2, 0x60082b30,
0xa68d3a81, 0x15a10f5d, 0x81b21c8a, 0x4bfb82e2,
0xff01c176, 0xff3c8b65, 0x8cc6bd29, 0xc678d6ff,
0x99b86508, 0x3c47e314, 0x766ecc05, 0xba186cb0,
0x42f57199, 0x5ef524f4, 0xb8419750, 0x6ae2a9d0,
0x291eaa18, 0x4e64b189, 0x506eb1d3, 0x78361d46,
0x6a2fcb7e, 0xbc0a46de, 0xb557badf, 0xad3de958,
0xa2901279, 0x491decbf, 0x257383df, 0x94dd19d1,
0xd0cfbbe2, 0x9063d36d, 0x81e44c3b, 0x973e9cc9,
0xfbe34690, 0x4eee3034, 0x1c413676, 0xf6735b8f,
0xf2991aca, 0x0ec85159, 0x6ce00ade, 0xad49de57,
0x025edf30, 0x42722b67, 0x30cfa6b2, 0x32df8676,
0x387d4500, 0x97fa67fd, 0x027c994a, 0x77c71d0c,
0x478eb75a, 0x898370a6, 0x73e7cca3, 0x34ace0ad,
0xc8ecb388, 0x5375c3aa, 0x9c194d87, 0x1b65246d,
0xca000bcf, 0x8a0fb13d, 0x81b957b0, 0xac627bfb,
0xc0fe47e5, 0xf3db0ad8, 0x1c605c7d, 0x5f579884,
0x63e079b5, 0x3d96f7cf, 0x3edd46e9, 0xc347c61e,
0x7b2b2a0e, 0x63dfcf51, 0x596781dd, 0x80304c4d,
0xa66f8b47
};

inline unsigned int rotateLeft(unsigned int x, const int b)
{
	if (b > -32 && b < 32 && b != 0)
	{
		if (b < 0)
		{
			x= ( x >> (-b) ) | ( x << (32 + b) );
		}
		else
		{
			x= ( x << b ) | ( x >> (32 - b) );
		}
	}
	return x;
}

struct RandomVectorSeedFunction : public exprtk::ifunction<float>
{
	using exprtk::ifunction<float>::operator();

	RandomVectorSeedFunction() :
	exprtk::ifunction<float>(2)
	{ exprtk::disable_has_side_effects(*this); }

	static inline float randv(const float& index,int irseed)
	{
		if (index < 0 || std::isnan(index) || std::isinf(index))
		{
			return 0;
		}
		const auto xi = (unsigned int)index;
		const unsigned int si = irseed % data_size;
		const unsigned int sa = irseed / data_size;
		unsigned int res=rotateLeft(random_data[(xi + 23 * si + 1) % data_size] ^ random_data[(xi / data_size + sa) % data_size],sa % 31 + 1);
		res ^= rotateLeft(random_data[(3 * xi + si + 13) % data_size],(xi+2*si) % 32) ^rotateLeft( random_data[(xi / data_size + 2 * sa) % data_size],xi % 31 + 1);
		return static_cast<int>(res) / (float)(1 << 31);
	}

	inline float operator()(const float& index,const float& seed) override
	{
		const int irseed = seed < 0 || std::isnan(seed) || std::isinf(seed) ? 0 : static_cast<int>(seed);
		return randv(index, irseed);
	}

	static const int data_size=sizeof(random_data)/sizeof(int);
};
static RandomVectorSeedFunction randsv_func;

struct RandomVectorFunction : public exprtk::ifunction<float>
{
	using exprtk::ifunction<float>::operator();

	RandomVectorFunction(const unsigned int seed) :
	exprtk::ifunction<float>(1),
	m_rseed(seed)
	{ exprtk::disable_has_side_effects(*this); }

	inline float operator()(const float& index) override
	{
		return RandomVectorSeedFunction::randv(index,m_rseed);
	}

	const unsigned int m_rseed;
};

namespace SimpleRandom {
	std::mt19937 generator (17);  // mt19937 is a standard mersenne_twister_engine
	std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
	struct float_random_with_engine
	{
		static inline float process()
		{
			return dist(generator);
		}
	};
} // namespace SimpleRandom

static freefunc0<float,SimpleRandom::float_random_with_engine,false> simple_rand;

class ExprFrontData
{
public:
	ExprFrontData(int last_func_samples):
	m_rand_vec(SimpleRandom::generator()),
	m_integ_func(nullptr),
	m_last_func(last_func_samples)
	{}
	~ExprFrontData()
	{
		for (const auto& cyclic : m_cyclics)
		{
			delete cyclic;
		}
		for (const auto& cyclic : m_cyclics_interp)
		{
			delete cyclic;
		}
		if (m_integ_func)
		{
			delete m_integ_func;
		}
	}

	symbol_table_t m_symbol_table;
	expression_t m_expression;
	std::string m_expression_string;
	std::vector<WaveValueFunction<float>* > m_cyclics;
	std::vector<WaveValueFunctionInterpolate<float>* > m_cyclics_interp;
	RandomVectorFunction m_rand_vec;
	IntegrateFunction<float> *m_integ_func;
	LastSampleFunction<float> m_last_func;

};


struct sin_wave
{
	static inline float process(float x)
	{
		x = positiveFraction(x);
		return std::sin(x * numbers::tau_v<float>);
	}
};
static freefunc1<float,sin_wave,true> sin_wave_func;
struct square_wave
{
	static inline float process(float x)
	{
		x = positiveFraction(x);
		if (x >= 0.5f) { return -1.0f; }
		else { return 1.0f; }
	}
};
static freefunc1<float,square_wave,true> square_wave_func;
struct triangle_wave
{
	static inline float process(float x)
	{
		x=positiveFraction(x);
		if (x < 0.25f)
		{
			return x * 4.0f;
		}
		else
		{
			if (x < 0.75f) { return 2.0f - x * 4.0f; }

			else { return x * 4.0f - 4.0f; }
		}
	}
};
static freefunc1<float,triangle_wave,true> triangle_wave_func;
struct saw_wave
{
	static inline float process(float x)
	{
		x=positiveFraction(x);
		return 2.0f * x - 1.0f;
	}
};
static freefunc1<float,saw_wave,true> saw_wave_func;
struct moogsaw_wave
{
	static inline float process(float x)
	{
		x = positiveFraction(x);
		if( x < 0.5f )
		{
			return -1.0f + x * 4.0f;
		}
		return 1.0f - 2.0f * x;
	}
};
static freefunc1<float,moogsaw_wave,true> moogsaw_wave_func;
struct moog_wave
{
	static inline float process(float x)
	{
		x = positiveFraction(x);
		if( x > 0.5f )
		{
			x = 1.0f - x;
			return -1.0f + 4.0f * x * x;
		}
		return -1.0f + 8.0f * x * x;
	}
};
static freefunc1<float,moog_wave,true> moog_wave_func;
struct exp_wave
{
	static inline float process(float x)
	{
		x = positiveFraction(x);
		if( x > 0.5f )
		{
			x = 1.0f - x;
		}
		return -1.0f + 8.0f * x * x;
	}
};
static freefunc1<float,exp_wave,true> exp_wave_func;
struct exp2_wave
{
	static inline float process(float x)
	{
		x = positiveFraction(x);
		if( x > 0.5f )
		{
			return -1.0f + 8.0f * (1.0f-x) * x;
		}
		return -1.0f + 8.0f * x * x;
	}
};
static freefunc1<float,exp2_wave,true> exp2_wave_func;
struct harmonic_cent
{
	static inline float process(float x)
	{
		return std::exp2(x / 1200);
	}
};
static freefunc1<float,harmonic_cent,true> harmonic_cent_func;
struct harmonic_semitone
{
	static inline float process(float x)
	{
		return std::exp2(x / 12);
	}
};
static freefunc1<float,harmonic_semitone,true> harmonic_semitone_func;


ExprFront::ExprFront(const char * expr, int last_func_samples)
{
	m_valid = false;
	try
	{
		m_data = new ExprFrontData(last_func_samples);

		m_data->m_expression_string = expr;
		m_data->m_symbol_table.add_pi();

		m_data->m_symbol_table.add_constant("e", numbers::e_v<float>);

		m_data->m_symbol_table.add_constant("seed", SimpleRandom::generator() & max_float_integer_mask);

		m_data->m_symbol_table.add_function("sinew", sin_wave_func);
		m_data->m_symbol_table.add_function("squarew", square_wave_func);
		m_data->m_symbol_table.add_function("trianglew", triangle_wave_func);
		m_data->m_symbol_table.add_function("saww", saw_wave_func);
		m_data->m_symbol_table.add_function("moogsaww", moogsaw_wave_func);
		m_data->m_symbol_table.add_function("moogw", moog_wave_func);
		m_data->m_symbol_table.add_function("expw", exp_wave_func);
		m_data->m_symbol_table.add_function("expnw", exp2_wave_func);
		m_data->m_symbol_table.add_function("cent", harmonic_cent_func);
		m_data->m_symbol_table.add_function("semitone", harmonic_semitone_func);
		m_data->m_symbol_table.add_function("rand", simple_rand);
		m_data->m_symbol_table.add_function("randv", m_data->m_rand_vec);
		m_data->m_symbol_table.add_function("randsv", randsv_func);
		m_data->m_symbol_table.add_function("last", m_data->m_last_func);
	}
	catch(...)
	{
		WARN_EXPRTK;
	}
}
ExprFront::~ExprFront()
{
	try
	{
		delete m_data;
	}
	catch(...)
	{
		WARN_EXPRTK;
	}
}

bool ExprFront::compile()
{
	m_valid = false;
	try
	{
		m_data->m_expression.register_symbol_table(m_data->m_symbol_table);
		parser_t::settings_store sstore;
		sstore.disable_all_logic_ops();
		sstore.disable_all_assignment_ops();
		sstore.disable_all_control_structures();
		parser_t parser(sstore);

		m_valid=parser.compile(m_data->m_expression_string, m_data->m_expression);
	}
	catch(...)
	{
		WARN_EXPRTK;
	}
	return m_valid;
}
float ExprFront::evaluate()
{
	try
	{
		if (!m_valid) return 0;
		float res = m_data->m_expression.value();
		m_data->m_last_func.setLastSample(res);
		return res;
	}
	catch(...)
	{
		WARN_EXPRTK;
	}
	return 0;

}
bool ExprFront::add_variable(const char* name, float& ref)
{
	try
	{
		return m_data->m_symbol_table.add_variable(name, ref);
	}
	catch(...)
	{
		WARN_EXPRTK;
	}
	return false;
}

bool ExprFront::add_constant(const char* name, float ref)
{
	try
	{
		return m_data->m_symbol_table.add_constant(name, ref);
	}
	catch(...)
	{
		WARN_EXPRTK;
	}
	return false;
}

bool ExprFront::add_cyclic_vector(const char* name, const float* data, size_t length, bool interp)
{
	try
	{
		if (interp)
		{
			auto wvf = new WaveValueFunctionInterpolate<float>(data, length);
			m_data->m_cyclics_interp.push_back(wvf);
			return m_data->m_symbol_table.add_function(name, *wvf);
		}
		else
		{
			auto wvf = new WaveValueFunction<float>(data, length);
			m_data->m_cyclics.push_back(wvf);
			return m_data->m_symbol_table.add_function(name, *wvf);
		}
	}
	catch(...)
	{
		WARN_EXPRTK;
	}
	return false;
}
size_t find_occurances(const std::string& haystack, const char* const needle)
{
	size_t last_pos = 0;
	size_t count = 0;
	const size_t len = strlen(needle);
	if (len > 0)
	{
		while (last_pos + len <= haystack.length())
		{
			last_pos = haystack.find(needle, last_pos);
			if (last_pos == std::string::npos)
				break;
			++count;
			last_pos += len;
		}
	}
	return count;
}

void ExprFront::setIntegrate(const unsigned int* const frameCounter, const unsigned int sample_rate)
{
	if (m_data->m_integ_func == nullptr)
	{
		const unsigned int ointeg = find_occurances(m_data->m_expression_string,"integrate");
		if ( ointeg > 0 )
		{
			m_data->m_integ_func = new IntegrateFunction<float>(frameCounter,sample_rate,ointeg);
			try
			{
				m_data->m_symbol_table.add_function("integrate",*m_data->m_integ_func);
			}
			catch(...)
			{
				WARN_EXPRTK;
			}
		}
	}
}

ExprSynth::ExprSynth(const WaveSample *gW1, const WaveSample *gW2, const WaveSample *gW3,
	ExprFront *exprO1, ExprFront *exprO2,
	NotePlayHandle *nph, const sample_rate_t sample_rate,
	const FloatModel* pan1, const FloatModel* pan2, float rel_trans):
	m_exprO1(exprO1),
	m_exprO2(exprO2),
	m_W1(gW1),
	m_W2(gW2),
	m_W3(gW3),
	m_nph(nph),
	m_sample_rate(sample_rate),
	m_pan1(pan1),
	m_pan2(pan2),
	m_rel_transition(rel_trans)
{
	m_note_sample = 0;
	m_note_rel_sample = 0;
	m_note_rel_sec = 0;
	m_note_sample_sec = 0;
	m_released = 0;
	m_frequency = m_nph->frequency();
	m_rel_inc = 1000.0 / (m_sample_rate * m_rel_transition);//rel_transition in ms. compute how much increment in each frame

	auto init_expression_step2 = [this](ExprFront * e) {
		e->add_cyclic_vector("W1", m_W1->m_samples,m_W1->m_length, m_W1->m_interpolate);
		e->add_cyclic_vector("W2", m_W2->m_samples,m_W2->m_length, m_W2->m_interpolate);
		e->add_cyclic_vector("W3", m_W3->m_samples,m_W3->m_length, m_W3->m_interpolate);
		e->add_variable("t", m_note_sample_sec);
		e->add_variable("f", m_frequency);
		e->add_variable("rel",m_released);
		e->add_variable("trel",m_note_rel_sec);
		e->setIntegrate(&m_note_sample,m_sample_rate);
		e->compile();
	};
	init_expression_step2(m_exprO1);
	init_expression_step2(m_exprO2);

}

ExprSynth::~ExprSynth()
{
	if (m_exprO1)
	{
		delete m_exprO1;
	}
	if (m_exprO2)
	{
		delete m_exprO2;
	}
}

void ExprSynth::renderOutput(fpp_t frames, SampleFrame* buf)
{
	try
	{
		bool o1_valid = m_exprO1->isValid();
		bool o2_valid = m_exprO2->isValid();
		if (!o1_valid && !o2_valid)
		{
			return;
		}
		float o1 = 0, o2 = 0;
		float pn1 = m_pan1->value() * 0.5;
		float pn2 = m_pan2->value() * 0.5;
		const float new_freq = m_nph->frequency();
		const float freq_inc = (new_freq - m_frequency) / frames;
		const bool is_released = m_nph->isReleased();

		expression_t *o1_rawExpr = &(m_exprO1->getData()->m_expression);
		expression_t *o2_rawExpr = &(m_exprO2->getData()->m_expression);
		LastSampleFunction<float> * last_func1 = &m_exprO1->getData()->m_last_func;
		LastSampleFunction<float> * last_func2 = &m_exprO2->getData()->m_last_func;
		if (is_released && m_note_rel_sample == 0)
		{
			m_note_rel_sample = m_note_sample;
		}
		if (o1_valid && o2_valid)
		{
			for (fpp_t frame = 0; frame < frames ; ++frame)
			{
				if (is_released && m_released < 1)
				{
					m_released = fmin(m_released+m_rel_inc, 1);
				}
				o1 = o1_rawExpr->value();
				o2 = o2_rawExpr->value();
				last_func1->setLastSample(o1);//put result in the circular buffer for the "last" function.
				last_func2->setLastSample(o2);
				buf[frame][0] = (-pn1 + 0.5) * o1 + (-pn2 + 0.5) * o2;
				buf[frame][1] = ( pn1 + 0.5) * o1 + ( pn2 + 0.5) * o2;
				m_note_sample++;
				m_note_sample_sec = m_note_sample / (float)m_sample_rate;
				if (is_released)
				{
					m_note_rel_sec = (m_note_sample - m_note_rel_sample) / (float)m_sample_rate;
				}
				m_frequency += freq_inc;
			}
		}
		else
		{

			if (o2_valid)
			{
				o1_rawExpr = o2_rawExpr;
				last_func1 = last_func2;
				pn1 = pn2;
			}
			for (fpp_t frame = 0; frame < frames ; ++frame)
			{
				if (is_released && m_released < 1)
				{
					m_released = fmin(m_released+m_rel_inc, 1);
				}
				o1 = o1_rawExpr->value();
				last_func1->setLastSample(o1);
				buf[frame][0] = (-pn1 + 0.5) * o1;
				buf[frame][1] = ( pn1 + 0.5) * o1;
				m_note_sample++;
				m_note_sample_sec = m_note_sample / (float)m_sample_rate;
				if (is_released)
				{
					m_note_rel_sec = (m_note_sample - m_note_rel_sample) / (float)m_sample_rate;
				}
				m_frequency += freq_inc;
			}
		}
		m_frequency = new_freq;
	}
	catch(...)
	{
		WARN_EXPRTK;
	}
}


} // namespace lmms
