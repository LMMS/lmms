/* sledgehammer.cpp

   "Dynamic Sledgehammer" - a thing to brutally mangle the dynamics of 
   a sound.

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

#include <math.h>
#include <stdlib.h>

/*****************************************************************************/

#include "cmt.h"
#include "run_adding.h"

/*****************************************************************************/

namespace sledgehammer {

    enum {
	port_rate      = 0,
	port_mod_infl  = 1, // modulator influence
	port_car_infl  = 2, // carrier influence (0 to 1 for compression)
	port_modulator = 3,
	port_carrier   = 4,
	port_output    = 5,
	n_ports        = 6
    };
    
/** This plugin imposes the dynamics of one sound onto another.
    It can be seen as a brutal compressor with a sidechain, or
    as a kind of one-band vocoder. */
    class Plugin : public CMT_PluginInstance {
	LADSPA_Data run_adding_gain;
	LADSPA_Data running_ms_mod;
	LADSPA_Data running_ms_car; // current mean square average
    public:
	Plugin(const LADSPA_Descriptor *,
	       unsigned long)
	    : CMT_PluginInstance(n_ports) {}

	friend void activate(LADSPA_Handle instance);
	
	template<OutputFunction write_output>
	friend void run(LADSPA_Handle instance,
			unsigned long sample_count);
	
	friend void set_run_adding_gain(LADSPA_Handle instance,
					LADSPA_Data new_gain);
    };

    void activate(LADSPA_Handle instance) {
	Plugin *pp = (Plugin *) instance;
	Plugin &p  = *pp;

	p.running_ms_mod = 0;
	p.running_ms_car = 0;
    }

    template<OutputFunction write_output>
    void run(LADSPA_Handle instance,
	     unsigned long sample_count) {
	
	Plugin *pp = (Plugin *) instance;
	Plugin &p  = *pp;

	LADSPA_Data   rate      = *pp->m_ppfPorts[port_rate];
	LADSPA_Data   mod_infl  = *pp->m_ppfPorts[port_mod_infl];
	LADSPA_Data   car_infl  = *pp->m_ppfPorts[port_car_infl];
	LADSPA_Data * modptr    =  pp->m_ppfPorts[port_modulator];
	LADSPA_Data * carptr    =  pp->m_ppfPorts[port_carrier];
	LADSPA_Data * out       =  pp->m_ppfPorts[port_output];

	for ( unsigned long i = 0; i < sample_count ; ++i ) {
	    LADSPA_Data mod = *(modptr++);
	    LADSPA_Data car = *(carptr++);

	    p.running_ms_mod = p.running_ms_mod*(1-rate) + (mod*mod)*rate;
	    p.running_ms_car = p.running_ms_car*(1-rate) + (car*car)*rate;
	    
	    LADSPA_Data rms_mod = sqrt(p.running_ms_mod);
	    LADSPA_Data rms_car = sqrt(p.running_ms_car);

	    LADSPA_Data outsig = car;

	    if (rms_car>0)
		outsig *= ((rms_car-0.5)*car_infl+0.5)/rms_car;

	    outsig *= ((rms_mod-0.5)*mod_infl+0.5);

	    write_output(out, outsig ,p.run_adding_gain);
	}
    }
    
    void set_run_adding_gain(LADSPA_Handle instance,
			     LADSPA_Data new_gain) {
	((Plugin *) instance)->run_adding_gain = new_gain;
    }

    void
    initialise() {
  
	CMT_Descriptor * d = new CMT_Descriptor
	    (1848,
	     "sledgehammer",
	     LADSPA_PROPERTY_HARD_RT_CAPABLE,
	     "Dynamic Sledgehammer",
	     CMT_MAKER("Nathaniel Virgo"),
	     CMT_COPYRIGHT("2002", "Nathaniel Virgo"),
	     NULL,
	     CMT_Instantiate<Plugin>,
	     activate,
	     run<write_output_normal>,
	     run<write_output_adding>,
	     set_run_adding_gain,
	     NULL);

	d->addPort
	    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	     "Rate",
	     (LADSPA_HINT_BOUNDED_BELOW 
	      | LADSPA_HINT_BOUNDED_ABOVE
	      | LADSPA_HINT_DEFAULT_MIDDLE),
	     0.00001,
 	     0.001);
	d->addPort
	    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	     "Modulator influence",
	     (LADSPA_HINT_BOUNDED_BELOW 
	      | LADSPA_HINT_BOUNDED_ABOVE
	      | LADSPA_HINT_DEFAULT_0),
	     -1,
	     1);
	d->addPort
	    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	     "Carrier influence",
	     (LADSPA_HINT_BOUNDED_BELOW 
	      | LADSPA_HINT_BOUNDED_ABOVE
	      | LADSPA_HINT_DEFAULT_1),
	     -1,
	     1);
	d->addPort
	    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	     "Modulator");
	d->addPort
	    (LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	     "Carrier");
	d->addPort
	    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	     "Output");

	registerNewPluginDescriptor(d);

    }

} // end of namespace

/*****************************************************************************/

/* EOF */





