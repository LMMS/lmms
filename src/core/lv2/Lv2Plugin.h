/*
  Copyright 2018 Alexandros Theodotou
  Copyright 2007-2016 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef LV2_PLUGIN_H
#define LV2_PLUGIN_H

#include <QtCore/QObject>

#include <lilv/lilv.h>
#include <serd/serd.h>
#include <suil/suil.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/ext/resize-port/resize-port.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
#include "lv2/lv2plug.in/ns/ext/data-access/data-access.h"
#include "lv2/lv2plug.in/ns/ext/event/event.h"
#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/ext/parameters/parameters.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/port-groups/port-groups.h"
#include "lv2/lv2plug.in/ns/ext/port-props/port-props.h"
#include "lv2/lv2plug.in/ns/ext/presets/presets.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#include "zix/ring.h"
#include "zix/sem.h"
#include "zix/thread.h"

#include <sratom/sratom.h>

#include "control.h"
extern "C" {
#include "lv2_evbuf.h"
#include "symap.h"
}
#include "worker.h"

#include "Plugin.h"

#ifdef __clang__
#    define REALTIME __attribute__((annotate("realtime")))
#else
#    define REALTIME
#endif

#define NS_EXT "http://lv2plug.in/ns/ext/"
#define NS_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_XSD "http://www.w3.org/2001/XMLSchema#"

enum PortFlow {
  FLOW_UNKNOWN,
  FLOW_INPUT,
  FLOW_OUTPUT
};

enum PortType {
  TYPE_UNKNOWN,
  TYPE_CONTROL,
  TYPE_AUDIO,
  TYPE_EVENT,
  TYPE_CV
};

typedef struct {
  const LilvPort* lilv_port;  ///< LV2 port
  enum PortType   type;       ///< Data type
  enum PortFlow   flow;       ///< Data flow direction
  void*           sys_port;   ///< For audio/MIDI ports, otherwise NULL
  LV2_Evbuf*      evbuf;      ///< For MIDI ports, otherwise NULL
  void*           widget;     ///< Control widget, if applicable
  size_t          buf_size;   ///< Custom buffer size, or 0
  uint32_t        index;      ///< Port index
  float           control;    ///< For control ports, otherwise 0.0f
  bool            old_api;    ///< True for event, false for atom
} Port;

typedef struct {
	LilvNode* atom_AtomPort;
	LilvNode* atom_Chunk;
	LilvNode* atom_Float;
	LilvNode* atom_Path;
	LilvNode* atom_Sequence;
	LilvNode* ev_EventPort;
	LilvNode* lv2_AudioPort;
	LilvNode* lv2_CVPort;
	LilvNode* lv2_ControlPort;
	LilvNode* lv2_InputPort;
	LilvNode* lv2_OutputPort;
	LilvNode* lv2_connectionOptional;
	LilvNode* lv2_control;
	LilvNode* lv2_default;
	LilvNode* lv2_enumeration;
	LilvNode* lv2_integer;
	LilvNode* lv2_maximum;
	LilvNode* lv2_minimum;
	LilvNode* lv2_name;
	LilvNode* lv2_reportsLatency;
	LilvNode* lv2_sampleRate;
	LilvNode* lv2_symbol;
	LilvNode* lv2_toggled;
	LilvNode* midi_MidiEvent;
	LilvNode* pg_group;
	LilvNode* pprops_logarithmic;
	LilvNode* pprops_notOnGUI;
	LilvNode* pprops_rangeSteps;
	LilvNode* pset_Preset;
	LilvNode* pset_bank;
	LilvNode* rdfs_comment;
	LilvNode* rdfs_label;
	LilvNode* rdfs_range;
	LilvNode* rsz_minimumSize;
	LilvNode* work_interface;
	LilvNode* work_schedule;
	LilvNode* end;  ///< NULL terminator for easy freeing of entire structure
} Lv2Nodes;

typedef struct {
	LV2_URID atom_Float;
	LV2_URID atom_Int;
	LV2_URID atom_Object;
	LV2_URID atom_Path;
	LV2_URID atom_String;
	LV2_URID atom_eventTransfer;
	LV2_URID bufsz_maxBlockLength;
	LV2_URID bufsz_minBlockLength;
	LV2_URID bufsz_sequenceSize;
	LV2_URID log_Error;
	LV2_URID log_Trace;
	LV2_URID log_Warning;
	LV2_URID midi_MidiEvent;
	LV2_URID param_sampleRate;
	LV2_URID patch_Get;
	LV2_URID patch_Put;
	LV2_URID patch_Set;
	LV2_URID patch_body;
	LV2_URID patch_property;
	LV2_URID patch_value;
	LV2_URID time_Position;
	LV2_URID time_bar;
	LV2_URID time_barBeat;
	LV2_URID time_beatUnit;
	LV2_URID time_beatsPerBar;
	LV2_URID time_beatsPerMinute;
	LV2_URID time_frame;
	LV2_URID time_speed;
	LV2_URID ui_updateRate;
} Lv2URIDs;

typedef int (* PresetSink)
  (Lv2Plugin * jalv,
   const LilvNode* node,
   const LilvNode* title,
   void*           data);

class Lv2Plugin
{
public:
  Lv2Plugin (const QString & uri);

	Lv2URIDs          urids;          ///< URIDs

	Lv2Nodes          nodes;          ///< Nodes
	LV2_Atom_Forge     forge;          ///< Atom forge
	LV2_URID_Map       map;            ///< URI => Int map
	LV2_URID_Unmap     unmap;          ///< Int => URI map
	SerdEnv*           env;            ///< Environment for RDF printing
	Sratom*            sratom;         ///< Atom serialiser
	Sratom*            ui_sratom;      ///< Atom serialiser for UI thread
	Symap*             symap;          ///< URI map
	ZixSem             symap_lock;     ///< Lock for URI map
	ZixRing*           ui_events;      ///< Port events from UI
	ZixRing*           plugin_events;  ///< Port events from plugin
	void*              ui_event_buf;   ///< Buffer for reading UI port events
	Lv2Worker         worker;         ///< Worker thread implementation
	Lv2Worker         state_worker;   ///< Synchronous worker for state restore
	ZixSem             work_lock;      ///< Lock for plugin work() method
	ZixSem*            done;           ///< Exit semaphore
	ZixSem             paused;         ///< Paused signal from process thread
	ZixSem exit_sem;  /**< Exit semaphore */
	char*              temp_dir;       ///< Temporary plugin state directory
	char*              save_dir;       ///< Plugin save directory
	const LilvPlugin*  plugin;         ///< Plugin class (RDF data)
	LilvState*         preset;         ///< Current preset
	LilvUIs*           uis;            ///< All plugin UIs (RDF data)
	const LilvUI*      ui;             ///< Plugin UI (RDF data)
	const LilvNode*    ui_type;        ///< Plugin UI type (unwrapped)
	LilvInstance*      instance;       ///< Plugin instance (shared library)
	SuilHost*          ui_host;        ///< Plugin UI host support
	SuilInstance*      ui_instance;    ///< Plugin UI instance (shared library)
	void*              window;         ///< Window (if applicable)
	Port*       ports;          ///< Port array of size num_ports
	Controls           controls;       ///< Available plugin controls
	uint32_t           block_length;   ///< Audio buffer size (block length)
	size_t             midi_buf_size;  ///< Size of MIDI port buffers
	uint32_t           control_in;     ///< Index of control input port
	uint32_t           num_ports;      ///< Size of the two following arrays:
	uint32_t           longest_sym;    ///< Longest port symbol
	uint32_t           plugin_latency; ///< Latency reported by plugin (if any)
	float              ui_update_hz;   ///< Frequency of UI updates
	uint32_t           sample_rate;    ///< Sample rate
	uint32_t           event_delta_t;  ///< Frames since last update sent to UI
	uint32_t           midi_event_id;  ///< MIDI event class ID in event context
	uint32_t           position;       ///< Transport position in frames
	float              bpm;            ///< Transport tempo in beats per minute
	bool               rolling;        ///< Transport speed (0=stop, 1=play)
	bool               buf_size_set;   ///< True iff buffer size callback fired
	bool               has_ui;         ///< True iff a control UI is present
	bool               request_update; ///< True iff a plugin update is needed
	bool               safe_restore;   ///< Plugin restore() is thread-safe

  LV2_Feature uri_map_feature      = { NS_EXT "uri-map", NULL };
  LV2_Feature map_feature          = { LV2_URID__map, NULL };
  LV2_Feature unmap_feature        = { LV2_URID__unmap, NULL };
  LV2_Feature make_path_feature    = { LV2_STATE__makePath, NULL };
  LV2_Feature sched_feature        = { LV2_WORKER__schedule, NULL };
  LV2_Feature state_sched_feature  = { LV2_WORKER__schedule, NULL };
  LV2_Feature safe_restore_feature = { LV2_STATE__threadSafeRestore, NULL };
  LV2_Feature log_feature          = { LV2_LOG__log, NULL };
  LV2_Feature options_feature      = { LV2_OPTIONS__options, NULL };
  LV2_Feature def_state_feature    = { LV2_STATE__loadDefaultState, NULL };

  const LV2_Feature* state_features[9] = {
    &uri_map_feature, &map_feature, &unmap_feature,
    &make_path_feature,
    &state_sched_feature,
    &safe_restore_feature,
    &log_feature,
    &options_feature,
    NULL
  };


  LV2_Extension_Data_Feature ext_data = { NULL };


  /** These features have no data */
  LV2_Feature buf_size_features[3] = {
    { LV2_BUF_SIZE__powerOf2BlockLength, NULL },
    { LV2_BUF_SIZE__fixedBlockLength, NULL },
    { LV2_BUF_SIZE__boundedBlockLength, NULL } };


  const LV2_Feature* features[12] = {
    &uri_map_feature,
    &map_feature,
    &unmap_feature,
    &sched_feature,
    &log_feature,
    &options_feature,
    &def_state_feature,
    &safe_restore_feature,
    &buf_size_features[0],
    &buf_size_features[1],
    &buf_size_features[2],
    NULL
  };

  /**
   * Instantiate the plugin.
   */
  void              instantiate();

  Port*
  port_by_symbol(const char* sym);

  bool
  send_to_ui (uint32_t    port_index,
                  uint32_t    type,
                  uint32_t    size,
                  const void* body);

  bool
  run (uint32_t nframes);

  bool
  update ();

  int
  apply_preset (const LilvNode* preset);

	bool feature_is_supported(const char* uri);
private:

  void
  create_ports ();

  void
  allocate_port_buffers ();


  void
  create_controls( bool writable);

  ControlID*
  control_by_symbol( const char* sym);

  void
  set_control(const ControlID* control,
                   uint32_t         size,
                   LV2_URID         type,
                   const void*      body);

  const char*
  native_ui_type();

  bool
  discover_ui();

  int
  open_ui();

  void
  init_ui();

  int
  close_ui();

  void
  ui_instantiate(const char* native_ui_type,
                      void*       parent);

  bool
  ui_is_resizable();


  void
  apply_ui_events( uint32_t nframes);


  void
  ui_port_event(uint32_t    port_index,
                     uint32_t    buffer_size,
                     uint32_t    protocol,
                     const void* buffer);



  int
  ui_resize (int width, int height);



  int
  unload_presets ();


  int
  delete_current_preset();

  int
  save_preset(const char* dir,
                   const char* uri,
                   const char* label,
                   const char* filename);

  void
  save( const char* dir);

  void
  save_port_values(SerdWriter*     writer,
                        const SerdNode* subject);

  void
  apply_state(LilvState* state);

	void print_control_value(const Port* port, float value);
};

char*
lv2_make_path(LV2_State_Make_Path_Handle handle,
               const char*                path);

void
lv2_ui_write(SuilController controller,
              uint32_t       port_index,
              uint32_t       buffer_size,
              uint32_t       protocol,
              const void*    buffer);

uint32_t
lv2_ui_port_index(SuilController controller, const char* symbol);

int
lv2_load_presets (Lv2Plugin * jalv, PresetSink sink, void* data);

#endif  //
