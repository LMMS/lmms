/* cmt.h

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

#ifndef CMT_BASE_INCLUDED
#define CMT_BASE_INCLUDED

/*****************************************************************************/

#include "ladspa_types.h"

/*****************************************************************************/

/** This class is the baseclass of all CMT plugin implementation
    data. Baseclassed so virtual destructors can be used. */
class CMT_ImplementationData {
public:
  virtual ~CMT_ImplementationData() {
  }
};

/*****************************************************************************/

/** This structure describes a CMT LADSPA Plugin. It is a direct
    ancestor of the _LADSPA_Descriptor structure which allows direct
    casting. A rich constructor function is provided make it easier to
    write LADSPA_Descriptor objects. (Less code is required and the
    compiler will tell you when you have missed an entry.) An
    addPort() function makes configuration of ports more
    straightforward than using the _LADSPA_Descriptor structure
    directly. */

struct CMT_Descriptor : public _LADSPA_Descriptor {
private:

  CMT_Descriptor &operator=(const CMT_Descriptor &) {
    return *this;
  }
  CMT_Descriptor(const CMT_Descriptor &) {
  }

public:

  ~CMT_Descriptor();

  /** The basic constructor for a CMT_Descriptor object. If you do not
      know what the parameters mean, please see the fields in the
      LADSPA_Descriptor structure, described in ladspa.h. Note that
      some parameters may be NULL. Note also that a template is
      provided to generate instantiate functions automatically (see
      CMT_Instantiate<>() below). Implementation data must be NULL if
      not allocated. */
  CMT_Descriptor(unsigned long                       lUniqueID,
		 const char *                        pcLabel,
		 LADSPA_Properties                   iProperties,
		 const char *                        pcName,
		 const char *                        pcMaker,
		 const char *                        pcCopyright,
		 CMT_ImplementationData *            poImplementationData,
		 LADSPA_Instantiate_Function         fInstantiate,
		 LADSPA_Activate_Function            fActivate,
		 LADSPA_Run_Function                 fRun,
		 LADSPA_Run_Adding_Function          fRunAdding,
		 LADSPA_Set_Run_Adding_Gain_Function fSetRunAddingGain,
		 LADSPA_Deactivate_Function          fDeactivate);

  /** This method adds a new port to the descriptor. If you do not
      know what the parameters mean, please see the fields in the
      LADSPA_Descriptor structure, described in ladspa.h. */
  void addPort(LADSPA_PortDescriptor          iPortDescriptor,
	       const char *                   pcPortName,
	       LADSPA_PortRangeHintDescriptor iHintDescriptor = 0,
	       LADSPA_Data                    fLowerBound = 0,
	       LADSPA_Data                    fUpperBound = 0);
  
};

typedef CMT_Descriptor * CMT_Descriptor_ptr;

/*****************************************************************************/

/** Each plugin type must register itself with the descriptor
    registry. This is done by calling the following function, passing
    a newly allocated structure (that will be cleaned up on library
    unload automatically). 

    Each module needs to be initialised in order to have a chance to
    register new plugins. This can be achieved by modifying the list
    of initialiser functions in descriptor.cpp. */
void registerNewPluginDescriptor(CMT_Descriptor * psDescriptor);

/*****************************************************************************/

/** This class is the baseclass of all CMT plugins. It provides
    functionality to handle LADSPA connect_port() and cleanup()
    requirements (as long as plugins have correctly written
    destructors!) A CMT_Instantiate<>() template is provided also,
    which makes LADSPA instantiate() methods easier to write.

    Derived classes access port data through the m_ppfPorts[]
    array. This contains one entry for each port, in the order in
    which ports were added to the corresponding CMT_Descriptor
    object. */
class CMT_PluginInstance {
private:

  CMT_PluginInstance &operator=(const CMT_PluginInstance &) {
    return *this;
  }
  CMT_PluginInstance(const CMT_PluginInstance &) {
  }

protected:
  
  LADSPA_Data ** m_ppfPorts;

  CMT_PluginInstance(const unsigned long lPortCount)
    : m_ppfPorts(new LADSPA_Data_ptr[lPortCount]) {
  }
  virtual ~CMT_PluginInstance() {
    delete [] m_ppfPorts;
  }

  friend void CMT_ConnectPort(LADSPA_Handle Instance,
			      unsigned long Port,
			      LADSPA_Data * DataLocation);
  friend void CMT_Cleanup(LADSPA_Handle Instance);

};

/*****************************************************************************/

/** This template can be used to generate functions to instantiate CMT
    plugins. To be used with this function, the plugin must accept two
    parameters (a LADSPA_Descriptor pointer and a sample rate). See
    the SimpleMixer class and mixer_descriptor() in mixer.cpp for a
    simple example of this: the instantiate function for the mixer
    class is generated within the mixer_descriptor() function as
    "CMT_Instantiate<SimpleMixer>". */
template <class T> LADSPA_Handle 
CMT_Instantiate(const LADSPA_Descriptor * Descriptor,
		unsigned long             SampleRate) {
  return new T(Descriptor, SampleRate);
}

/*****************************************************************************/

/** This macro should be used to fill in the `Maker' field in the
    CMT_Descriptor. */
#define CMT_MAKER(AUTHORS)						\
	"CMT (http://www.ladspa.org/cmt, plugin by " AUTHORS ")"

/** This macro should be used to fill in the `Copyright' field in the
    CMT_Descriptor. */
#define CMT_COPYRIGHT(YEARS, AUTHORS)					\
	"(C)" YEARS ", " AUTHORS ". "                                   \
        "GNU General Public Licence Version 2 applies."

/*****************************************************************************/

#endif

/* EOF */
