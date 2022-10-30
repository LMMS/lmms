/*****************************************************************************

        fnc.hpp
        Author: Laurent de Soras, 2005

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (hiir_fnc_CURRENT_CODEHEADER)
	#error Recursive inclusion of fnc code header.
#endif
#define hiir_fnc_CURRENT_CODEHEADER

#if ! defined (hiir_fnc_CODEHEADER_INCLUDED)
#define hiir_fnc_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <cassert>
#include <cmath>



namespace hiir
{



int	round_int (double x)
{
	return int (floor (x + 0.5));
}



int	ceil_int (double x)
{
	return int (ceil (x));
}



template <class T>
T	ipowp (T x, long n) noexcept
{
	assert (n >= 0);

	T              z (1);
	while (n != 0)
	{
		if ((n & 1) != 0)
		{
			z *= x;
		}
		n >>= 1;
		x *= x;
	}

	return z;
}



}  // namespace hiir



#endif   // hiir_fnc_CODEHEADER_INCLUDED

#undef hiir_fnc_CURRENT_CODEHEADER



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
