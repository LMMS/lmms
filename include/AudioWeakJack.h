/* runtime/weak dynamic JACK linking
 *
 * (C) 2014 Robin Gareus <robin@gareus.org>
 *
 * The wrapped jack API itself is
 * (C) 2001 Paul Davis
 * (C) 2004 Jack O'Quin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef _WEAK_JACK_H
#define _WEAK_JACK_H

// LMMS uses LMMS_HAVE_WEAKJACK instead
#ifndef USE_WEAK_JACK
#define USE_WEAK_JACK
#endif

// LMMS doesn't use metadata.h
#ifndef NO_JACK_METADATA
#define NO_JACK_METADATA
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/** check if libjack is available
 *
 * return 0 if libjack is dynamically linked of was
 * successfully dl-opened. Otherwise:
 *
 * -1: library was not initialized
 * -2: libjack was not found
 *  > 0 bitwise flags:
 *      1: a required function was not found in libjack
 *      2: jack_client_open was not found in libjack
 */
int have_libjack(void);

#ifdef __cplusplus
}
#endif

#ifdef USE_WEAK_JACK

/* <jack/jack.h> */
#define jack_client_close                   WJACK_client_close
#define jack_get_client_name                WJACK_get_client_name
#define jack_get_sample_rate                WJACK_get_sample_rate
#define jack_get_buffer_size                WJACK_get_buffer_size
#define jack_frames_since_cycle_start       WJACK_frames_since_cycle_start
#define jack_frame_time                     WJACK_frame_time
#define jack_last_frame_time                WJACK_last_frame_time
#define jack_get_time                       WJACK_get_time
#define jack_cpu_load                       WJACK_cpu_load
#define jack_is_realtime                    WJACK_is_realtime

#define jack_client_name_size               WJACK_client_name_size

#define jack_set_freewheel                  WJACK_set_freewheel
#define jack_set_buffer_size                WJACK_set_buffer_size

#define jack_on_shutdown                    WJACK_on_shutdown
#define jack_on_info_shutdown               WJACK_on_info_shutdown
#define jack_set_process_callback           WJACK_set_process_callback
#define jack_set_freewheel_callback         WJACK_set_freewheel_callback
#define jack_set_buffer_size_callback       WJACK_set_buffer_size_callback
#define jack_set_sample_rate_callback       WJACK_set_sample_rate_callback
#define jack_set_port_registration_callback WJACK_set_port_registration_callback
#define jack_set_port_connect_callback      WJACK_set_port_connect_callback
#define jack_set_graph_order_callback       WJACK_set_graph_order_callback
#define jack_set_xrun_callback              WJACK_set_xrun_callback
#define jack_set_latency_callback           WJACK_set_latency_callback
#define jack_set_error_function             WJACK_set_error_function
#define jack_set_info_function             WJACK_set_info_function

#define jack_activate                       WJACK_activate
#define jack_deactivate                     WJACK_deactivate

#define jack_recompute_total_latencies      WJACK_recompute_total_latencies
#define jack_port_get_total_latency         WJACK_port_get_total_latency
#define jack_port_get_latency_range         WJACK_port_get_latency_range
#define jack_port_set_latency_range         WJACK_port_set_latency_range
#define jack_port_get_buffer                WJACK_port_get_buffer
#define jack_port_request_monitor           WJACK_port_request_monitor
#define jack_port_ensure_monitor            WJACK_port_ensure_monitor
#define jack_port_monitoring_input          WJACK_port_monitoring_input

#define jack_port_name                      WJACK_port_name
#define jack_port_short_name                WJACK_port_short_name
#define jack_port_flags                     WJACK_port_flags
#define jack_get_ports                      WJACK_get_ports
#define jack_port_name_size                 WJACK_port_name_size
#define jack_port_type_size                 WJACK_port_type_size
#define jack_port_type_get_buffer_size      WJACK_port_type_get_buffer_size
#define jack_port_by_name                   WJACK_port_by_name
#define jack_port_by_id                     WJACK_port_by_id
#define jack_port_set_name                  WJACK_port_set_name
#define jack_port_get_aliases               WJACK_port_get_aliases
#define jack_port_rename                    WJACK_port_rename
#define jack_port_disconnect                WJACK_port_disconnect
#define jack_port_register                  WJACK_port_register
#define jack_port_unregister                WJACK_port_unregister
#define jack_port_type                      WJACK_port_type
#define jack_port_get_connections           WJACK_port_get_connections
#define jack_port_get_all_connections       WJACK_port_get_all_connections
#define jack_connect                        WJACK_connect
#define jack_disconnect                     WJACK_disconnect
#define jack_free                           WJACK_free

#define jack_cycle_wait                     WJACK_cycle_wait
#define jack_cycle_signal                   WJACK_cycle_signal
#define jack_set_process_thread             WJACK_set_process_thread
#define jack_set_thread_init_callback       WJACK_set_thread_init_callback

/* <jack/transport.h> */
#define jack_get_current_transport_frame    WJACK_get_current_transport_frame
#define jack_transport_locate               WJACK_transport_locate
#define jack_transport_start                WJACK_transport_start
#define jack_transport_stop                 WJACK_transport_stop
#define jack_transport_query                WJACK_transport_query
#define jack_set_sync_callback              WJACK_set_sync_callback
#define jack_set_timebase_callback          WJACK_set_timebase_callback
#define jack_release_timebase               WJACK_release_timebase

/* <jack/midiport.h> */
#define jack_midi_get_event_count           WJACK_midi_get_event_count
#define jack_midi_event_get                 WJACK_midi_event_get
#define jack_midi_event_write               WJACK_midi_event_write
#define jack_midi_clear_buffer              WJACK_midi_clear_buffer

/* <jack/session.h> */
#define jack_set_session_callback           WJACK_set_session_callback
#define jack_session_reply                  WJACK_session_reply
#define jack_session_event_free             WJACK_session_event_free

/* <jack/ringbuffer.h> */
#define jack_ringbuffer_create              WJACK_ringbuffer_create
#define jack_ringbuffer_free                WJACK_ringbuffer_free
#define jack_ringbuffer_reset               WJACK_ringbuffer_reset
#define jack_ringbuffer_read_advance        WJACK_ringbuffer_read_advance
#define jack_ringbuffer_write_advance       WJACK_ringbuffer_write_advance
#define jack_ringbuffer_read_space          WJACK_ringbuffer_read_space
#define jack_ringbuffer_write_space         WJACK_ringbuffer_write_space
#define jack_ringbuffer_read                WJACK_ringbuffer_read
#define jack_ringbuffer_write               WJACK_ringbuffer_write
#define jack_ringbuffer_mlock               WJACK_ringbuffer_mlock
#define jack_ringbuffer_get_read_vector     WJACK_ringbuffer_get_read_vector
#define jack_ringbuffer_get_write_vector    WJACK_ringbuffer_get_write_vector
#define jack_ringbuffer_peek                WJACK_ringbuffer_peek

/* <jack/thread.h> */
#define jack_client_real_time_priority      WJACK_client_real_time_priority
#define jack_client_max_real_time_priority  WJACK_client_max_real_time_priority
#define jack_acquire_real_time_scheduling   WJACK_acquire_real_time_scheduling
#define jack_client_create_thread           WJACK_client_create_thread
#define jack_drop_real_time_scheduling      WJACK_drop_real_time_scheduling
#define jack_client_stop_thread             WJACK_client_stop_thread
#define jack_client_kill_thread             WJACK_client_kill_thread
#define jack_set_thread_creator             WJACK_set_thread_creator

#define jack_client_open                    WJACK_client_client_openXXX

#ifndef NO_JACK_METADATA
/* <jack/metadata.h> */
#define jack_get_uuid_for_client_name       WJACK_get_uuid_for_client_name
#define jack_get_client_name_by_uuid        WJACK_get_client_name_by_uuid
#define jack_port_uuid                      WJACK_port_uuid

#define jack_set_property                   WJACK_set_property
#define jack_get_property                   WJACK_get_property
#define jack_free_description               WJACK_free_description
#define jack_get_properties                 WJACK_get_properties
#define jack_get_all_properties             WJACK_get_all_properties
#define jack_remove_property                WJACK_remove_property
#define jack_remove_properties              WJACK_remove_properties
#define jack_remove_all_properties          WJACK_remove_all_properties
#define jack_set_property_change_callback   WJACK_set_property_change_callback
#endif

/* <jack/statistics.h> */
#define jack_get_max_delayed_usecs WJACK_get_max_delayed_usecs
#define jack_get_xrun_delayed_usecs WJACK_get_xrun_delayed_usecs
#define jack_reset_max_delayed_usecs WJACK_reset_max_delayed_usecs

#endif // end USE_WEAK_JACK

#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/ringbuffer.h>
#include <jack/midiport.h>
#include <jack/session.h>
#include <jack/thread.h>

#ifndef NO_JACK_METADATA
#include <jack/metadata.h>
#endif

#ifdef USE_WEAK_JACK

#undef jack_client_open

/* var-args hack */

#ifdef __cplusplus
extern "C" {
#endif
void (* WJACK_get_client_open (void)) (void);
jack_client_t * WJACK_no_client_open (const char *client_name, jack_options_t options, jack_status_t *status, ...);
#ifdef __cplusplus
}
#endif

#define jack_client_open(...) \
( \
	(WJACK_get_client_open() != NULL) \
	?  ((jack_client_t* (*)(const char *, jack_options_t, jack_status_t *, ...))(WJACK_get_client_open()))(__VA_ARGS__) \
	: WJACK_no_client_open(__VA_ARGS__) \
)

#endif // end USE_WEAK_JACK

#endif // _WEAK_JACK_H
