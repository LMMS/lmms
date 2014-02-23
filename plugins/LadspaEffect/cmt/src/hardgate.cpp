/* hardgate.cpp

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

/*****************************************************************************/

namespace hardgate {

    enum {
	port_threshold = 0,
	port_input     = 1,
	port_output    = 2,
	n_ports        = 3
    };
    
/** This plugin sets its input signal to 0 if it falls below a threshold. */
    class Plugin : public CMT_PluginInstance {
    public:
	
	Plugin(const LADSPA_Descriptor *,
	       unsigned long)
	    : CMT_PluginInstance(n_ports) {
	}
	
	friend void run(LADSPA_Handle instance,
			unsigned long sample_count);
	
    };
    
    void run(LADSPA_Handle instance,
	     unsigned long sample_count) {
	
	Plugin *pp = (Plugin *) instance;
	
	LADSPA_Data   threshold = *pp->m_ppfPorts[port_threshold];
	LADSPA_Data * in        =  pp->m_ppfPorts[port_input];
	LADSPA_Data * out       =  pp->m_ppfPorts[port_output];
    
	for ( unsigned long i = 0; i < sample_count ; ++i ) 
	{
	    LADSPA_Data insig = *(in++);
	    if ( insig < threshold && insig > -threshold )
		*(out++) = 0.0f;
	    else 
		*(out++) = insig;
	}
    }
    
    void
    initialise() {
  
	CMT_Descriptor * d = new CMT_Descriptor
	    (1845,
	     "hard_gate",
	     LADSPA_PROPERTY_HARD_RT_CAPABLE,
	     "Hard Gate",
	     CMT_MAKER("Nathaniel Virgo"),
	     CMT_COPYRIGHT("2002", "Nathaniel Virgo"),
	     NULL,
	     CMT_Instantiate<Plugin>,
	     NULL,
	     run,
	     NULL,
	     NULL,
	     NULL);

	d->addPort
	    (LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	     "Threshold",
	     (LADSPA_HINT_BOUNDED_BELOW 
	      | LADSPA_HINT_BOUNDED_ABOVE
	      | LADSPA_HINT_DEFAULT_0),
	     0,
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





