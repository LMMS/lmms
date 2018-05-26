/*
    Copyright (C) 2002 Paul Davis
    Copyright (C) 2003 Jack O'Quin

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifndef __jack_transport_h__
#define __jack_transport_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <jack/types.h>
#include <jack/weakmacros.h>

/**
 * @defgroup TransportControl Transport and Timebase control
 * @{
 */

/**
 * Called by the timebase master to release itself from that
 * responsibility.
 *
 * If the timebase master releases the timebase or leaves the JACK
 * graph for any reason, the JACK engine takes over at the start of
 * the next process cycle.  The transport state does not change.  If
 * rolling, it continues to play, with frame numbers as the only
 * available position information.
 *
 * @see jack_set_timebase_callback
 *
 * @param client the JACK client structure.
 *
 * @return 0 on success, otherwise a non-zero error code.
 */
int  jack_release_timebase (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Register (or unregister) as a slow-sync client, one that cannot
 * respond immediately to transport position changes.
 *
 * The @a sync_callback will be invoked at the first available
 * opportunity after its registration is complete.  If the client is
 * currently active this will be the following process cycle,
 * otherwise it will be the first cycle after calling jack_activate().
 * After that, it runs according to the ::JackSyncCallback rules.
 * Clients that don't set a @a sync_callback are assumed to be ready
 * immediately any time the transport wants to start.
 *
 * @param client the JACK client structure.
 * @param sync_callback is a realtime function that returns TRUE when
 * the client is ready.  Setting @a sync_callback to NULL declares that
 * this client no longer requires slow-sync processing.
 * @param arg an argument for the @a sync_callback function.
 *
 * @return 0 on success, otherwise a non-zero error code.
 */
int  jack_set_sync_callback (jack_client_t *client,
			     JackSyncCallback sync_callback,
			     void *arg) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Set the timeout value for slow-sync clients.
 *
 * This timeout prevents unresponsive slow-sync clients from
 * completely halting the transport mechanism.  The default is two
 * seconds.  When the timeout expires, the transport starts rolling,
 * even if some slow-sync clients are still unready.  The @a
 * sync_callbacks of these clients continue being invoked, giving them
 * a chance to catch up.
 *
 * @see jack_set_sync_callback
 *
 * @param client the JACK client structure.
 * @param timeout is delay (in microseconds) before the timeout expires.
 *
 * @return 0 on success, otherwise a non-zero error code.
 */
int  jack_set_sync_timeout (jack_client_t *client,
			    jack_time_t timeout) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Register as timebase master for the JACK subsystem.
 *
 * The timebase master registers a callback that updates extended
 * position information such as beats or timecode whenever necessary.
 * Without this extended information, there is no need for this
 * function.
 *
 * There is never more than one master at a time.  When a new client
 * takes over, the former @a timebase_callback is no longer called.
 * Taking over the timebase may be done conditionally, so it fails if
 * there was a master already.
 *
 * @param client the JACK client structure.
 * @param conditional non-zero for a conditional request.
 * @param timebase_callback is a realtime function that returns
 * position information.
 * @param arg an argument for the @a timebase_callback function.
 *
 * @return
 *   - 0 on success;
 *   - EBUSY if a conditional request fails because there was already a
 *   timebase master;
 *   - other non-zero error code.
 */
int  jack_set_timebase_callback (jack_client_t *client,
				 int conditional,
				 JackTimebaseCallback timebase_callback,
				 void *arg) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Reposition the transport to a new frame number.
 *
 * May be called at any time by any client.  The new position takes
 * effect in two process cycles.  If there are slow-sync clients and
 * the transport is already rolling, it will enter the
 * ::JackTransportStarting state and begin invoking their @a
 * sync_callbacks until ready.  This function is realtime-safe.
 *
 * @see jack_transport_reposition, jack_set_sync_callback
 *
 * @param client the JACK client structure.
 * @param frame frame number of new transport position.
 *
 * @return 0 if valid request, non-zero otherwise.
 */
int  jack_transport_locate (jack_client_t *client,
			    jack_nframes_t frame) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Query the current transport state and position.
 *
 * This function is realtime-safe, and can be called from any thread.
 * If called from the process thread, @a pos corresponds to the first
 * frame of the current cycle and the state returned is valid for the
 * entire cycle.
 *
 * @param client the JACK client structure.
 * @param pos pointer to structure for returning current transport
 * position; @a pos->valid will show which fields contain valid data.
 * If @a pos is NULL, do not return position information.
 *
 * @return Current transport state.
 */
jack_transport_state_t jack_transport_query (const jack_client_t *client,
					     jack_position_t *pos) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Return an estimate of the current transport frame,
 * including any time elapsed since the last transport
 * positional update.
 *
 * @param client the JACK client structure
 */
jack_nframes_t jack_get_current_transport_frame (const jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Request a new transport position.
 *
 * May be called at any time by any client.  The new position takes
 * effect in two process cycles.  If there are slow-sync clients and
 * the transport is already rolling, it will enter the
 * ::JackTransportStarting state and begin invoking their @a
 * sync_callbacks until ready.  This function is realtime-safe.
 *
 * @see jack_transport_locate, jack_set_sync_callback
 *
 * @param client the JACK client structure.
 * @param pos requested new transport position.
 *
 * @return 0 if valid request, EINVAL if position structure rejected.
 */
int  jack_transport_reposition (jack_client_t *client,
				const jack_position_t *pos) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Start the JACK transport rolling.
 *
 * Any client can make this request at any time.  It takes effect no
 * sooner than the next process cycle, perhaps later if there are
 * slow-sync clients.  This function is realtime-safe.
 *
 * @see jack_set_sync_callback
 *
 * @param client the JACK client structure.
 */
void jack_transport_start (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Stop the JACK transport.
 *
 * Any client can make this request at any time.  It takes effect on
 * the next process cycle.  This function is realtime-safe.
 *
 * @param client the JACK client structure.
 */
void jack_transport_stop (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Gets the current transport info structure (deprecated).
 *
 * @param client the JACK client structure.
 * @param tinfo current transport info structure.  The "valid" field
 * describes which fields contain valid data.
 *
 * @deprecated This is for compatibility with the earlier transport
 * interface.  Use jack_transport_query(), instead.
 *
 * @pre Must be called from the process thread.
 */
void jack_get_transport_info (jack_client_t *client,
			      jack_transport_info_t *tinfo) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Set the transport info structure (deprecated).
 *
 * @deprecated This function still exists for compatibility with the
 * earlier transport interface, but it does nothing.  Instead, define
 * a ::JackTimebaseCallback.
 */
void jack_set_transport_info (jack_client_t *client,
			      jack_transport_info_t *tinfo) JACK_OPTIONAL_WEAK_EXPORT;

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* __jack_transport_h__ */
