/* pink_sh.cpp

   A sample-and-hold pink noise generator.

   (c) 2002 Nathaniel Virgo

   Part of the Computer Music Toolkit - a library of LADSPA plugins. 
   The Computer Music Toolkit is Copyright (C) 2000-2002 
   Richard W.E. Furse. The author may be contacted at
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

#include "pinknoise.h"
#include "utils.h"

/*****************************************************************************/

namespace pink_sh {

    enum {
	port_frequency   = 0,
	port_output      = 1,
	n_ports          = 2
    };
    
    /** This plugin generates a signal which approximates stepped 
	(sample-and-hold like) pink noise, using the
	Voss-McCartney algorithm described at www.firstpr.com.au/dsp/pink-noise/ */
    class Plugin : public CMT_PluginInstance {
    private:
	LADSPA_Data sample_rate;
	PinkNoise noise_source;
	unsigned counter;
    public:
	
        Plugin(const LADSPA_Descriptor *,
	       unsigned long s_rate) : 
	    CMT_PluginInstance(n_ports), 
	    sample_rate(s_rate) {
	}
	
	~Plugin() {
	}
	
	friend void activate(LADSPA_Handle instance);
	
	friend void run(LADSPA_Handle instance,
			unsigned long sample_count);	
    };
    
    void activate(LADSPA_Handle instance) {
	Plugin *pp = (Plugin *) instance;
	Plugin &p  = *pp;
	
	p.noise_source.reset();
	p.counter = 0;
    }

    void run(LADSPA_Handle instance,
	     unsigned long sample_count) {

	Plugin *pp = (Plugin *) instance;
	Plugin &p  = *pp;

	LADSPA_Data   frequency = *pp->m_ppfPorts[port_frequency];
	LADSPA_Data * out       =  pp->m_ppfPorts[port_output];

	frequency = BOUNDED_ABOVE(frequency,p.sample_rate);
	unsigned remain = sample_count;

	if (frequency > 0) {
	    while (remain) {
		unsigned jump_samples = (remain<p.counter) ? remain : p.counter;
		for (unsigned j=0; j<jump_samples; ++j) {
		    *(out++) = p.noise_source.getLastValue();
		}
		p.counter -= jump_samples;
		remain -= jump_samples;
		if (p.counter==0) {
		    p.noise_source.getValue();
		    p.counter = (unsigned)(p.sample_rate/frequency);
		}
	    }
	}
	else
	{
	    for (unsigned long i=0; i<sample_count; ++i)
		*(out++) = p.noise_source.getLastValue();
	}
    }

    void initialise() {
	CMT_Descriptor * d = new CMT_Descriptor
	    (1843,
	     "pink_sh",
	     0,
	     "Pink Noise (sample and hold)",
	     CMT_MAKER("Nathaniel Virgo"),
	     CMT_COPYRIGHT("2002", "Nathaniel Virgo"),
	     NULL,
	     CMT_Instantiate<Plugin>,
	     activate,
	     run,
	     NULL,
	     NULL,
	     NULL);
	d->addPort
	    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	     "Sample and hold frequency",
	     (LADSPA_HINT_BOUNDED_BELOW
	      | LADSPA_HINT_BOUNDED_ABOVE
	      | LADSPA_HINT_SAMPLE_RATE
	      | LADSPA_HINT_DEFAULT_1),
	     0, 0.02);
	d->addPort
	    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	     "Output");
	registerNewPluginDescriptor(d);
    }

} // end of namespace

/*****************************************************************************/

/* EOF */
    
