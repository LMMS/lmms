/* run_adding.h

   (c) 2002 Nathaniel Virgo

   a few simple inline functions that can be used with templates
   to get run_adding for free.

   Computer Music Toolkit - a library of LADSPA plugins. Copyright (C)
   2000-2002 Richard W.E. Furse. The author may be contacted at
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

/*****************************************************************************/

/*
   How to use this
   ---------------

   Templates can be used to automatically generate code for both the run and
   run_adding LADSPA functions.  Simply define the plugin's run function as
   
     template<OutputFunction write_output>
     void run_foo(LADSPA_Handle Instance, unsigned long SampleCount);

   and in the body of the function use

     write_output(pfOutput, fValue, poFoo->m_fRunAddingGain);

   instead of

     *(pfOutput++) = fValue.

   and be sure to include a set_run_adding_gain function.
   then set the LADSPA run function as

     run_foo<write_output_normal>;

   and the run_adding function as

     run_foo<write_output_adding>;

   With -O1 or greater g++ will inline the write_output function, and
   although the code ends up slightly bigger there is no overhead compared to
   having two seperate functions.

   Sometimes the run_adding_gain function can be made more efficient than this
   - for instance, if the output is multiplied by a gain already then you are
   doing one more multiplication than necessary on every sample.  It's a lot
   less code to maintain, though, and it should still save some work for the
   host compared to not having a run_adding function. 
*/

/*****************************************************************************/

#include <ladspa.h>

/*****************************************************************************/

typedef void OutputFunction(LADSPA_Data *&, const LADSPA_Data &, 
			    const LADSPA_Data &);

inline void write_output_normal(LADSPA_Data *&out, const LADSPA_Data &value, 
				const LADSPA_Data &run_adding_gain)
{
    *(out++) = value;
}

inline void write_output_adding(LADSPA_Data *&out, const LADSPA_Data &value,
				const LADSPA_Data &run_adding_gain)
{
    *(out++) += value*run_adding_gain;
}

/*****************************************************************************/

/*
  If the plugin has a control-rate ouput then you don't want the write_output
  function to try to increment the pointer.  To achieve this, use

    write_control<write_output>(pfOutput, fValue, poFoo->m_fRunAddingGain);

  instead of just

    write_output(...);

  I realise this feels a bit hacky, but it works.
*/

template <OutputFunction ouput_mode>
inline void write_control(LADSPA_Data *const, 
			  const LADSPA_Data &, const LADSPA_Data &);

template <>
inline void write_control<write_output_normal>(LADSPA_Data *const out, 
					       const LADSPA_Data &value,
					       const LADSPA_Data &run_adding_gain)
{
    *out = value;
}

template <>
inline void write_control<write_output_adding>(LADSPA_Data *const out, 
					       const LADSPA_Data &value,
					       const LADSPA_Data &run_adding_gain)
{
    *out += value*run_adding_gain;
}


/*****************************************************************************/

/*
   This next bit is an attempt to facilitate the writing of slightly
   more efficent run_adding functions without writing two seperate pieces of
   code.  You can say something like

     LADSPA_Data fOutputGain = ... ;
     ...
     fOutputGain *= get_gain<write_output>(poFoo->m_fRunAddingGain);
     ...
     write_output(pfOutput, fValue*fOutputGain, 1.0f);

   in run_foo.  With -O1 or greater g++ should inline the functions and
   optimise away the multiplies by 1.0f, so in run_foo<write_output_adding>
   fOutputGain will be multiplied by m_fRunAddingGain and in 
   run_foo<write_output_normal> it will be left alone.

   This does not make for very clear code, sorry about that.  See disintegrator.cpp
   for an example.
*/

template <OutputFunction output_mode> 
inline float get_gain(const LADSPA_Data &);

template <>
inline float get_gain<write_output_normal>(const LADSPA_Data &)
{
    return 1.0f;
}

template <>
inline float get_gain<write_output_adding>(const LADSPA_Data &run_adding_gain)
{
    return run_adding_gain;
}
