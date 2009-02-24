//
// "$Id: math.h 4288 2005-04-16 00:13:17Z mike $"
//
// Math header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2005 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

#ifndef fl_math_h
#  define fl_math_h

// Apple's ProjectBuilder has the nasty habit of including recursively
// down the file tree. To avoid re-including <FL/math.h> we must 
// directly include the systems math file. (Plus, I could not find a 
// predefined macro for ProjectBuilder builds, so we have to define it 
// in the project)
#  if defined(__APPLE__) && defined(__PROJECTBUILDER__)
#    include "/usr/include/math.h"
#  else
#    include <math.h>
#  endif

#  ifdef __EMX__
#    include <float.h>
#  endif


#  ifndef M_PI
#    define M_PI            3.14159265358979323846
#    define M_PI_2          1.57079632679489661923
#    define M_PI_4          0.78539816339744830962
#    define M_1_PI          0.31830988618379067154
#    define M_2_PI          0.63661977236758134308
#  endif // !M_PI

#  ifndef M_SQRT2
#    define M_SQRT2         1.41421356237309504880
#    define M_SQRT1_2       0.70710678118654752440
#  endif // !M_SQRT2

#  if (defined(WIN32) || defined(CRAY)) && !defined(__MINGW32__) && !defined(__MWERKS__)

inline double rint(double v) {return floor(v+.5);}
inline double copysign(double a, double b) {return b<0 ? -a : a;}

#  endif // (WIN32 || CRAY) && !__MINGW32__ && !__MWERKS__

#endif // !fl_math_h


//
// End of "$Id: math.h 4288 2005-04-16 00:13:17Z mike $".
//
