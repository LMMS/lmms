/*
 * Copyright (c) 2014 Hanspeter Portner (dev@open-music-kontrollers.ch)
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 *     1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 * 
 *     2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 * 
 *     3. This notice may not be removed or altered from any source
 *     distribution.
 */

#ifndef __JACK_OSC_H
#define __JACK_OSC_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <jack/jack.h>
#include <jack/types.h>
#include <jack/midiport.h>

/*
 * It is not necessary to use this header since it contains transparent
 * macros referring to the Jack MIDI API, but developers are encouraged
 * to use this header (or at least the same definitions) to make their
 * code more clear.
 */

/*
 * Use this as port type in jack_port_register to make it clear that
 * this MIDI port is used to route OSC messages.
 *
 * jack_port_t *osc_in;
 * osc_in = jack_port_register(client, "osc.in", JACK_DEFAULT_OSC_TYPE,
 *                             JackPortIsInput, 0);
 */
#define JACK_DEFAULT_OSC_TYPE           JACK_DEFAULT_MIDI_TYPE

/*
 * Use this as value for metadata key JACKEY_EVENT_TYPES 
 * (http://jackaudio.org/metadata/event-type) to mark/query/unmark a port
 * as OSC carrier in jack_{set,get,remove}_property.
 *
 * jack_uuid_t uuid_in = jack_port_uuid(osc_in);
 *
 * // set port event type to OSC
 * jack_set_property(client, uuid_in, JACKEY_EVENT_TYPES,
 *                   JACK_EVENT_TYPE__OSC, NULL);
 *
 * // query port event type
 * char *value = NULL;
 * char *type = NULL;
 * if( (jack_get_property(uuid, JACKEY_EVENT_TYPES, &value, &type) == 0)
 *  && (strstr(value, JACK_EVENT_TYPE__OSC) != NULL) )
 * 	printf("This port routes OSC!\n");
 * jack_free(value);
 * jack_free(type);
 *
 * // clear port event type
 * jack_remove_property(client, uuid_in, JACKEY_EVENT_TYPES);
 */
#define JACK_EVENT_TYPE__OSC            "OSC"

/*
 * The Jack OSC API is a direct map to the Jack MIDI API.
 */
typedef jack_midi_data_t                jack_osc_data_t;
typedef jack_midi_event_t               jack_osc_event_t;

#define jack_osc_get_event_count        jack_midi_get_event_count
#define jack_osc_event_get              jack_midi_event_get
#define jack_osc_clear_buffer           jack_midi_clear_buffer
#define jack_osc_max_event_size         jack_midi_max_event_size
#define jack_osc_event_reserve          jack_midi_event_reserve
#define jack_osc_event_write            jack_midi_event_write
#define jack_osc_get_lost_event_count   jack_midi_get_lost_event_count

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __JACK_OSC_H */
