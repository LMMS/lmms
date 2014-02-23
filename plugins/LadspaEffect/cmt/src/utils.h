/* utils.h

   Computer Music Toolkit - a library of LADSPA plugins. Copyright (C)
   2000 Richard W.E. Furse. The author may be contacted at
   richard@muse.demon.co.uk.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public Licence as
   published by the Free Software Foundation; either version 2 of the
   Licence, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA. */

#ifndef CMT_UTILS_INCLUDED
#define CMT_UTILS_INCLUDED

/*****************************************************************************/

#include <math.h>
#include <stdlib.h>

/*****************************************************************************/

#include "ladspa_types.h"

/*****************************************************************************/

/** The drag setting is arranged so that the gain drops by a factor of
    1e3 (60dB) in the time specified. This is a bit of an arbitrary
    value but ties in with what the user will probably expect from
    his/her experience with reverb units. */
inline LADSPA_Data 
calculate60dBDrag(const LADSPA_Data fTime,
		  const LADSPA_Data fSampleRate) {
  if (fTime <= 0)
    return 0;
  else 
    return pow(1e3, -1 / (fTime * fSampleRate));
}

/*****************************************************************************/

inline LADSPA_Data
BOUNDED_BELOW(const LADSPA_Data fData,
	      const LADSPA_Data fLowerBound) {
  if (fData <= fLowerBound)
    return fLowerBound;
  else
    return fData;
}

inline LADSPA_Data BOUNDED_ABOVE(const LADSPA_Data fData,
				 const LADSPA_Data fUpperBound) {
  if (fData >= fUpperBound)
    return fUpperBound;
  else
    return fData;
}

inline LADSPA_Data 
BOUNDED(const LADSPA_Data fData,
	const LADSPA_Data fLowerBound,
	const LADSPA_Data fUpperBound) {
  if (fData <= fLowerBound)
    return fLowerBound;
  else if (fData >= fUpperBound)
    return fUpperBound;
  else
    return fData;
}

/*****************************************************************************/

/* Take a reading from a normal RV. The algorithm works by repeated
   sampling of the uniform distribution, the lQuality variable giving
   the number of samples. */
inline double 
sampleNormalDistribution(const double dMean,
			 const double dStandardDeviation,
			 const long   lQuality = 12) {

  double dValue = 0;
  for (long lIter = 0; lIter < lQuality; lIter++)
    dValue += rand();

  double dSampleFromNormal01 = (dValue / RAND_MAX) - (lQuality * 0.5);

  return dMean + dStandardDeviation * dSampleFromNormal01;
}

/*****************************************************************************/

#endif

/* EOF */
