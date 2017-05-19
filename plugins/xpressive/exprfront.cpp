/*
 * exprfront.cpp - implementation of a Frontend to ExprTk
 *
 * Copyright (c) 2016-2017 Orr Dvori
 *
 * This file is part of LMMS - http://lmms.io
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

#include "exprtk.hpp"
#include "exprfront.h"
#include <string>
#include <math.h>
#include <cstdlib>
#include "lmms_math.h"
typedef exprtk::symbol_table<float> symbol_table_t;
typedef exprtk::expression<float> expression_t;
typedef exprtk::parser<float> parser_t;


class ExprFrontData
{
public:
	symbol_table_t symbol_table;
	expression_t expression;
	std::string expression_string;
};


static float sin_wave(float x)
{
	x=positiveFraction(x);
	return sinf(x*F_2PI);
}
static float square_wave(float x)
{
	x=positiveFraction(x);
	if (x>=0.5)
		return -1;
	else
		return 1;
}
static float triangle_wave(float x)
{
	x=positiveFraction(x);
	if (x<0.25)
		return x*4;
	else
	{
		if(x<0.75)
		{	return 2-x*4;}

		else return x*4-4;
	}

}
static float saw_wave(float x)
{
	x=positiveFraction(x);
	return 2*x-1;
}
static float moogsaw_wave( float x )
{
	x = positiveFraction(x);
	if( x < 0.5f )
	{
		return -1.0f + x * 4.0f;
	}
	return 1.0f - 2.0f * x;
}
static float moog_wave( float x )
{
	x = positiveFraction(x);
	if( x > 0.5f )
	{
		x = 1.0f - x;
		return -1.0f + 4.0f * x * x;
	}
	return -1.0f + 8.0f * x * x;
}

static float exp_wave( float x )
{
	x = positiveFraction(x);
	if( x > 0.5f )
	{
		x = 1.0f - x;
	}
	return -1.0f + 8.0f * x * x;
}
static float exp2_wave( float x )
{
	x = positiveFraction(x);
	if( x > 0.5f )
	{
		return -1.0f + 8.0f * (1.0f-x) * x;
	}
	return -1.0f + 8.0f * x * x;
}
static float harmonic_cent(float x)
{
	return powf(2,x/1200);
}
static const unsigned int random_data[257]={
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
static float rand_array(void *s,float x)
{
	if (x<0)
		return 0;
	const int data_size=sizeof(random_data)/sizeof(int);
	int xi=(int)x;
	int si=(*static_cast<int*>(s))%data_size;
	int sa=(*static_cast<int*>(s))/data_size;
	int res=random_data[(xi+si)%data_size]^random_data[(xi/data_size+sa)%data_size];
	return res/(float)(1<<31);

}
static float float_random()
{
	return 1.0f - rand() * 2.0f / RAND_MAX;
}

ExprFront::ExprFront(const char * expr)
{
	m_valid=false;
	m_data=new ExprFrontData();
	m_rseed=rand();
	m_data->expression_string=expr;
	m_data->symbol_table.add_constants();

	m_data->symbol_table.add_function("sinew",sin_wave);
	m_data->symbol_table.add_function("squarew",square_wave);
	m_data->symbol_table.add_function("trianglew",triangle_wave);
	m_data->symbol_table.add_function("saww",saw_wave);
	m_data->symbol_table.add_function("moogsaww",moogsaw_wave);
	m_data->symbol_table.add_function("moogw",moog_wave);
	m_data->symbol_table.add_function("expw",exp_wave);
	m_data->symbol_table.add_function("expnw",exp2_wave);
	m_data->symbol_table.add_function("cent",harmonic_cent);
	m_data->symbol_table.add_function("rand",float_random);
	m_data->symbol_table.add_function("randv",rand_array,(void*)&m_rseed);
}
ExprFront::~ExprFront()
{
	delete m_data;
}

bool ExprFront::compile()
{
	m_data->expression.register_symbol_table(m_data->symbol_table);

	parser_t::settings_store sstore;
	sstore.disable_all_logic_ops();
	sstore.disable_all_assignment_ops();
	sstore.disable_all_control_structures();
	parser_t parser(sstore);

	return m_valid=parser.compile(m_data->expression_string, m_data->expression);
}
float ExprFront::evaluate()
{
	if (!m_valid) return 0;
	return m_data->expression.value();
}
bool ExprFront::add_variable(const char * name, float & ref)
{
	return m_data->symbol_table.add_variable(name, ref);
}

bool ExprFront::add_constant(const char *name, float ref)
{
	return m_data->symbol_table.add_constant(name, ref);
}

bool ExprFront::add_function(const char *name, void *data, ExprFront::ff1data_functor f)
{
	return m_data->symbol_table.add_function(name,f,data);
}

