/* pink.cpp

   Interpolated pink noise plugins for use as control signals.

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

#include "pinknoise.h"
#include "utils.h"

/*****************************************************************************/

namespace pink {

    enum {
	port_frequency   = 0,
	port_output      = 1,
	n_ports          = 2
    };
    
    /** This plugin generates a signal which approximates the effect of low-pass
	filtered pink noise, which makes for an interesting randomly changing 
	control parameter.  It should probably use sinc interpolation, but in fact
	it uses third-order splines, which sound more-or-less okay to me. */
    class Plugin : public CMT_PluginInstance {
    private:
	
	LADSPA_Data sample_rate;
	
	PinkNoise noise_source;
	LADSPA_Data *data_points;
	int first_point;
	unsigned long counter;
	float multiplier;   // 1/(max counter value)

    public:
	
        Plugin(const LADSPA_Descriptor *,
	       unsigned long s_rate) : 
	    CMT_PluginInstance(n_ports), 
	    sample_rate(s_rate) {
	    data_points = new LADSPA_Data[4];
	}
	
	~Plugin() {
	    delete [] data_points;
	}
	
	friend void activate(LADSPA_Handle instance);
	
	friend void run_interpolated_audio(LADSPA_Handle instance,
					   unsigned long sample_count);
	
	friend void run_interpolated_control(LADSPA_Handle instance,
					     unsigned long sample_count);
	
    };
    
    void activate(LADSPA_Handle instance) {
	Plugin *pp = (Plugin *) instance;
	Plugin &p  = *pp;
	
	p.noise_source.reset();
	for (int i=0; i<4; ++i)
	    p.data_points[i] = p.noise_source.getValue();
	p.first_point = 0;
	p.counter = 0;
	p.multiplier = 1;
    }

    inline float thirdInterp(const float &x,
			     const float &L1,const float &L0,
			     const float &H0,const float &H1) {
      return 
	L0 +
	.5f*
	x*(H0-L1 +
	   x*(H0 + L0*(-2) + L1 +
	      x*( (H0 - L0)*9 + (L1 - H1)*3 +
		  x*((L0 - H0)*15 + (H1 - L1)*5 +
		     x*((H0 - L0)*6 + (L1 - H1)*2 )))));
    }

    void run_interpolated_audio(LADSPA_Handle instance,
				unsigned long sample_count) {

	Plugin *pp = (Plugin *) instance;
	Plugin &p  = *pp;

	LADSPA_Data   frequency = *pp->m_ppfPorts[port_frequency];
	LADSPA_Data * out       =  pp->m_ppfPorts[port_output];

	if (frequency<=0) {
	    LADSPA_Data value = thirdInterp( 1 - p.counter*p.multiplier,
					     p.data_points[  p.first_point        ],
					     p.data_points[ (p.first_point+1) % 4 ],
					     p.data_points[ (p.first_point+2) % 4 ],
					     p.data_points[ (p.first_point+3) % 4 ] );
	    for (unsigned long i=0; i<sample_count; ++i)
		*(out++) = value;
	} else {
	    frequency = BOUNDED_ABOVE(frequency, p.sample_rate);
	    unsigned long remain = sample_count;
	    while (remain) {
		unsigned long jump_samples = (remain<p.counter) ? remain : p.counter;
		for (unsigned long j=0; j<jump_samples; ++j) {
		    *(out++) = thirdInterp( 1 - p.counter*p.multiplier,
					    p.data_points[  p.first_point        ],
					    p.data_points[ (p.first_point+1) % 4 ],
					    p.data_points[ (p.first_point+2) % 4 ],
					    p.data_points[ (p.first_point+3) % 4 ] );
		    --p.counter;
		}
		remain -= jump_samples;
		if (p.counter == 0) {
		    p.data_points[p.first_point] = p.noise_source.getValue();
		    p.first_point = (p.first_point + 1) % 4;
		    p.multiplier = frequency/p.sample_rate;
		    p.counter = (unsigned long)(p.sample_rate/frequency);
		}
	    }
	}
    }

    void run_interpolated_control(LADSPA_Handle instance,
				  unsigned long sample_count) {

	Plugin *pp = (Plugin *) instance;
 	Plugin &p  = *pp;
	
	LADSPA_Data   frequency = *pp->m_ppfPorts[port_frequency];
	LADSPA_Data * out       =  pp->m_ppfPorts[port_output];

	float value = thirdInterp( 1 - p.counter*p.multiplier, 
	                           p.data_points[  p.first_point        ],
				   p.data_points[ (p.first_point+1) % 4 ],
				   p.data_points[ (p.first_point+2) % 4 ],
				   p.data_points[ (p.first_point+3) % 4 ] );
	if (frequency>0) {
	    frequency = BOUNDED_ABOVE(frequency, p.sample_rate/sample_count);
	    while (p.counter <= sample_count) {
		p.data_points[ p.first_point ] = p.noise_source.getValue();
		p.first_point = (p.first_point + 1) % 4;
		p.multiplier = frequency/p.sample_rate;
		p.counter += (unsigned long)(p.sample_rate/frequency);
	    }
	    p.counter -= (p.counter < sample_count) ? p.counter : sample_count;
	}
	*(out)=value;
    }

    void initialise() {
	CMT_Descriptor * d = new CMT_Descriptor
	    (1841,
	     "pink_interpolated_audio",
	     0,
	     "Pink Noise (Interpolated)",
	     CMT_MAKER("Nathaniel Virgo"),
	     CMT_COPYRIGHT("2002", "Nathaniel Virgo"),
	     NULL,
	     CMT_Instantiate<Plugin>,
	     activate,
	     run_interpolated_audio,
	     NULL,
	     NULL,
	     NULL);
	d->addPort
	    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	     "Highest frequency",
	     (LADSPA_HINT_BOUNDED_BELOW 
	      | LADSPA_HINT_BOUNDED_ABOVE
	      | LADSPA_HINT_SAMPLE_RATE
	      | LADSPA_HINT_DEFAULT_1),
	     0,
	     1);
	d->addPort
	    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	     "Output");
	registerNewPluginDescriptor(d);

	// the following has been commented out because I'm pretty sure that
	// control-rate outputs don't make sense for the vast majority of hosts.
	// (SSM being the notable exception)
	/*
	d = new CMT_Descriptor
	    (1842,
	     "pink_interpolated_control",
	     0,
	     "Pink Noise (Interpolated, control rate)",
	     CMT_MAKER("Nathaniel Virgo"),
	     CMT_COPYRIGHT("2002", "Nathaniel Virgo"),
	     NULL,
	     CMT_Instantiate<Plugin>,
	     activate,
	     run_interpolated_control,
	     NULL,
	     NULL,
	     NULL);
	d->addPort
	    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	     "Highest frequency",
	     (LADSPA_HINT_BOUNDED_BELOW 
	      | LADSPA_HINT_BOUNDED_ABOVE
	      | LADSPA_HINT_SAMPLE_RATE
	      | LADSPA_HINT_DEFAULT_1), 
	     0,
	     0.002);  // arbitrary low value (sensible for sample_count around 500)
	d->addPort
	    (LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL,
	     "Output");
	registerNewPluginDescriptor(d);
	*/
    }

} // end of namespace

/*****************************************************************************/

/* EOF */
