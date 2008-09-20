/* disintegrator.cpp

   (c) 2002 Nathaniel Virgo

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

#include <stdlib.h>

/*****************************************************************************/

#include "cmt.h"
#include "run_adding.h"

/*****************************************************************************/

namespace disintegrator {

    enum {
        port_probability = 0,
	port_multiplier  = 1,
	port_input       = 2,
	port_output      = 3,
	n_ports          = 4
    };
    
/** This plugin multiplies random half-waveforms by port_multiplier,
    with probability port_probability */
    class Plugin : public CMT_PluginInstance {
	LADSPA_Data run_adding_gain;
	bool active;
	LADSPA_Data last_input;
    public:
	Plugin(const LADSPA_Descriptor *,
	       unsigned long)
	    : CMT_PluginInstance(n_ports) {
	    active = false; last_input = 0.0f;
	}

	template<OutputFunction write_output>
	friend void run(LADSPA_Handle instance,
			unsigned long sample_count);

	friend void set_run_adding_gain(LADSPA_Handle instance,
					LADSPA_Data new_gain);
    };

    template<OutputFunction write_output>
    void run(LADSPA_Handle instance,
	     unsigned long sample_count) {
	
	Plugin *pp = (Plugin *) instance;
	Plugin &p  = *pp;

	LADSPA_Data   prob      = *pp->m_ppfPorts[port_probability];
	LADSPA_Data   mult      = *pp->m_ppfPorts[port_multiplier];
	LADSPA_Data * in        =  pp->m_ppfPorts[port_input];
	LADSPA_Data * out       =  pp->m_ppfPorts[port_output];
    
	mult *= get_gain<write_output>(p.run_adding_gain);

	for ( unsigned long i = 0; i < sample_count ; ++i ) {
	    LADSPA_Data insig = *(in++);
	    if ( ( p.last_input>0 && insig<0 ) || ( p.last_input<0 && insig>0 ) )
		p.active = rand() < prob*RAND_MAX;
	    p.last_input = insig;
	    if (p.active) 
		write_output(out, insig*mult, 1.0f);
	    else
		write_output(out, insig, p.run_adding_gain);
	}
    }
    
    void set_run_adding_gain(LADSPA_Handle instance,
			     LADSPA_Data new_gain) {
	((Plugin *) instance)->run_adding_gain = new_gain;
    }

    void
    initialise() {
  
	CMT_Descriptor * d = new CMT_Descriptor
	    (1846,
	     "disintegrator",
	     LADSPA_PROPERTY_HARD_RT_CAPABLE,
	     "Disintegrator",
	     CMT_MAKER("Nathaniel Virgo"),
	     CMT_COPYRIGHT("2002", "Nathaniel Virgo"),
	     NULL,
	     CMT_Instantiate<Plugin>,
	     NULL,
	     run<write_output_normal>,
	     run<write_output_adding>,
	     set_run_adding_gain,
	     NULL);

	d->addPort
	    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	     "Probability",
	     (LADSPA_HINT_BOUNDED_BELOW 
	      | LADSPA_HINT_BOUNDED_ABOVE
	      | LADSPA_HINT_DEFAULT_0),
	     0,
	     1);
	d->addPort
	    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	     "Multiplier",
	     (LADSPA_HINT_BOUNDED_BELOW 
	      | LADSPA_HINT_BOUNDED_ABOVE
	      | LADSPA_HINT_DEFAULT_0),
	     -1,
	     1);
	d->addPort
	    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	     "Input");
	d->addPort
	    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	     "Output");

	registerNewPluginDescriptor(d);

    }

} // end of namespace

/*****************************************************************************/

/* EOF */





