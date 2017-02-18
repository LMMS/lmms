/*
 * exprfront.h - header file to a Frontend to ExprTk
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

#ifndef EXPRFRONT_H_
#define EXPRFRONT_H_

#include <cmath>
#include <cstddef>

class ExprFrontData;

inline float positiveFraction(float x)
{
	if (std::isnan(x) || std::isinf(x))
		return 0;
	if (x<0)
	{
		x+=static_cast<int>(1-x);
	}
	return x-static_cast<int>(x);
}

class ExprFront
{
public:
	typedef float (*ff1data_functor)(void*, float);
	ExprFront(const char* expr);
	~ExprFront();
	bool compile();
	inline bool isValid() { return m_valid; }
	float evaluate();
	bool add_variable(const char* name, float & ref);
	bool add_constant(const char* name, float  ref);
	bool add_cyclic_vector(const char* name, const float* data, size_t length, bool interp = false);
	void setIntegrate(const unsigned int* frameCounter, unsigned int sample_rate);
	ExprFrontData* getData() { return m_data; }
private:
	ExprFrontData *m_data;
	bool m_valid;
};


#endif /* EXPRFRONT_H_ */
