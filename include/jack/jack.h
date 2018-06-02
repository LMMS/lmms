/*
  Copyright (C) 2001 Paul Davis
  Copyright (C) 2004 Jack O'Quin

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

#ifndef __jack_h__
#define __jack_h__

#ifdef __cplusplus
extern "C"
{
#endif

#include <jack/systemdeps.h>
#include <jack/types.h>
#include <jack/transport.h>

/**
 * Note: More documentation can be found in jack/types.h.
 */

    /*************************************************************
     * NOTE: JACK_WEAK_EXPORT ***MUST*** be used on every function
     * added to the JACK API after the 0.116.2 release.
     *
     * Functions that predate this release are marked with
     * JACK_WEAK_OPTIONAL_EXPORT which can be defined at compile
     * time in a variety of ways. The default definition is empty,
     * so that these symbols get normal linkage. If you wish to
     * use all JACK symbols with weak linkage, include
     * <jack/weakjack.h> before jack.h.
     *************************************************************/

#include <jack/weakmacros.h>

/**
 * Call this function to get version of the JACK, in form of several numbers
 *
 * @param major_ptr pointer to variable receiving major version of JACK.
 *
 * @param minor_ptr pointer to variable receiving minor version of JACK.
 *
 * @param major_ptr pointer to variable receiving micro version of JACK.
 *
 * @param major_ptr pointer to variable receiving protocol version of JACK.
 *
 */
void
jack_get_version(
    int *major_ptr,
    int *minor_ptr,
    int *micro_ptr,
    int *proto_ptr) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Call this function to get version of the JACK, in form of a string
 *
 * @return Human readable string describing JACK version being used.
 *
 */
const char *
jack_get_version_string(void) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @defgroup ClientFunctions Creating & manipulating clients
 * @{
 */

/**
 * Open an external client session with a JACK server.  This interface
 * is more complex but more powerful than jack_client_new().  With it,
 * clients may choose which of several servers to connect, and control
 * whether and how to start the server automatically, if it was not
 * already running.  There is also an option for JACK to generate a
 * unique client name, when necessary.
 *
 * @param client_name of at most jack_client_name_size() characters.
 * The name scope is local to each server.  Unless forbidden by the
 * @ref JackUseExactName option, the server will modify this name to
 * create a unique variant, if needed.
 *
 * @param options formed by OR-ing together @ref JackOptions bits.
 * Only the @ref JackOpenOptions bits are allowed.
 *
 * @param status (if non-NULL) an address for JACK to return
 * information from the open operation.  This status word is formed by
 * OR-ing together the relevant @ref JackStatus bits.
 *
 *
 * <b>Optional parameters:</b> depending on corresponding [@a options
 * bits] additional parameters may follow @a status (in this order).
 *
 * @arg [@ref JackServerName] <em>(char *) server_name</em> selects
 * from among several possible concurrent server instances.  Server
 * names are unique to each user.  If unspecified, use "default"
 * unless \$JACK_DEFAULT_SERVER is defined in the process environment.
 *
 * @return Opaque client handle if successful.  If this is NULL, the
 * open operation failed, @a *status includes @ref JackFailure and the
 * caller is not a JACK client.
 */
jack_client_t * jack_client_open (const char *client_name,
                                  jack_options_t options,
                                  jack_status_t *status, ...) JACK_OPTIONAL_WEAK_EXPORT;

/**
* \bold THIS FUNCTION IS DEPRECATED AND SHOULD NOT BE USED IN
*  NEW JACK CLIENTS
*
* @deprecated Please use jack_client_open().
*/
jack_client_t * jack_client_new (const char *client_name) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;

/**
 * Disconnects an external client from a JACK server.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_client_close (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the maximum number of characters in a JACK client name
 * including the final NULL character.  This value is a constant.
 */
int jack_client_name_size (void) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return pointer to actual client name.  This is useful when @ref
 * JackUseExactName is not specified on open and @ref
 * JackNameNotUnique status was returned.  In that case, the actual
 * name will differ from the @a client_name requested.
 */
char * jack_get_client_name (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Get the session ID for a client name.
 *
 * The session manager needs this to reassociate a client name to the session_id.
 *
 * The caller is responsible for calling jack_free(3) on any non-NULL
 * returned value.
 */
char *jack_get_uuid_for_client_name (jack_client_t *client,
                                     const char    *client_name) JACK_WEAK_EXPORT;

/**
 * Get the client name for a session_id.
 *
 * In order to snapshot the graph connections, the session manager needs to map
 * session_ids to client names.
 *
 * The caller is responsible for calling jack_free(3) on any non-NULL
 * returned value.
 */
char *jack_get_client_name_by_uuid (jack_client_t *client,
                                    const char    *client_uuid ) JACK_WEAK_EXPORT;

/**
 * Load an internal client into the Jack server.
 *
 * Internal clients run inside the JACK server process.  They can use
 * most of the same functions as external clients.  Each internal
 * client must declare jack_initialize() and jack_finish() entry
 * points, called at load and unload times.  See inprocess.c for an
 * example of how to write an internal client.
 *
 * @deprecated Please use jack_internal_client_load().
 *
 * @param client_name of at most jack_client_name_size() characters.
 *
 * @param load_name of a shared object file containing the code for
 * the new client.
 *
 * @param load_init an arbitary string passed to the jack_initialize()
 * routine of the new client (may be NULL).
 *
 * @return 0 if successful.
 */
int jack_internal_client_new (const char *client_name,
                              const char *load_name,
                              const char *load_init) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;

/**
 * Remove an internal client from a JACK server.
 *
 * @deprecated Please use jack_internal_client_unload().
 */
void jack_internal_client_close (const char *client_name) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;

/**
 * Tell the Jack server that the program is ready to start processing
 * audio.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_activate (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Tell the Jack server to remove this @a client from the process
 * graph.  Also, disconnect all ports belonging to it, since inactive
 * clients have no port connections.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_deactivate (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return pid of client. If not available, 0 will be returned.
 */
int jack_get_client_pid (const char *name) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the pthread ID of the thread running the JACK client side
 * real-time code.
 */
jack_native_thread_t jack_client_thread_id (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

/*@}*/

/**
 * @param client pointer to JACK client structure.
 *
 * Check if the JACK subsystem is running with -R (--realtime).
 *
 * @return 1 if JACK is running realtime, 0 otherwise
 */
int jack_is_realtime (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @defgroup NonCallbackAPI The non-callback API
 * @{
 */

/**
 * \bold THIS FUNCTION IS DEPRECATED AND SHOULD NOT BE USED IN
 *  NEW JACK CLIENTS.
 *
 * @deprecated Please use jack_cycle_wait() and jack_cycle_signal() functions.
 */
jack_nframes_t jack_thread_wait (jack_client_t *client, int status) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Wait until this JACK client should process data.
 *
 * @param client - pointer to a JACK client structure
 *
 * @return the number of frames of data to process
 */
jack_nframes_t jack_cycle_wait (jack_client_t* client) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Signal next clients in the graph.
 *
 * @param client - pointer to a JACK client structure
 * @param status - if non-zero, calling thread should exit
 */
void jack_cycle_signal (jack_client_t* client, int status) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Tell the Jack server to call @a thread_callback in the RT thread.
 * Typical use are in conjunction with @a jack_cycle_wait and @a jack_cycle_signal functions.
 * The code in the supplied function must be suitable for real-time
 * execution.  That means that it cannot call functions that might
 * block for a long time. This includes malloc, free, printf,
 * pthread_mutex_lock, sleep, wait, poll, select, pthread_join,
 * pthread_cond_wait, etc, etc. See
 * http://jackit.sourceforge.net/docs/design/design.html#SECTION00411000000000000000
 * for more information.
 *
 * NOTE: this function cannot be called while the client is activated
 * (after jack_activate has been called.)
 *
 * @return 0 on success, otherwise a non-zero error code.
*/
int jack_set_process_thread(jack_client_t* client, JackThreadCallback thread_callback, void *arg) JACK_OPTIONAL_WEAK_EXPORT;

/*@}*/

/**
 * @defgroup ClientCallbacks Setting Client Callbacks
 * @{
 */

/**
 * Tell JACK to call @a thread_init_callback once just after
 * the creation of the thread in which all other callbacks
 * will be handled.
 *
 * The code in the supplied function does not need to be
 * suitable for real-time execution.
 *
 * NOTE: this function cannot be called while the client is activated
 * (after jack_activate has been called.)
 *
 * @return 0 on success, otherwise a non-zero error code, causing JACK
 * to remove that client from the process() graph.
 */
int jack_set_thread_init_callback (jack_client_t *client,
                                   JackThreadInitCallback thread_init_callback,
                                   void *arg) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @param client pointer to JACK client structure.
 * @param function The jack_shutdown function pointer.
 * @param arg The arguments for the jack_shutdown function.
 *
 * Register a function (and argument) to be called if and when the
 * JACK server shuts down the client thread.  The function must
 * be written as if it were an asynchonrous POSIX signal
 * handler --- use only async-safe functions, and remember that it
 * is executed from another thread.  A typical function might
 * set a flag or write to a pipe so that the rest of the
 * application knows that the JACK client thread has shut
 * down.
 *
 * NOTE: clients do not need to call this.  It exists only
 * to help more complex clients understand what is going
 * on.  It should be called before jack_client_activate().
 *
 * NOTE: if a client calls this AND jack_on_info_shutdown(), then
 * in case of a client thread shutdown, the callback
 * passed to this function will not be called, and the one passed to
 * jack_on_info_shutdown() will.
 *
 * NOTE: application should typically signal another thread to correctly 
 * finish cleanup, that is by calling "jack_client_close" 
 * (since "jack_client_close" cannot be called directly in the context 
 * of the thread that calls the shutdown callback).
 */
void jack_on_shutdown (jack_client_t *client,
                       JackShutdownCallback shutdown_callback, void *arg) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @param client pointer to JACK client structure.
 * @param function The jack_info_shutdown function pointer.
 * @param arg The arguments for the jack_info_shutdown function.
 *
 * Register a function (and argument) to be called if and when the
 * JACK server shuts down the client thread.  The function must
 * be written as if it were an asynchonrous POSIX signal
 * handler --- use only async-safe functions, and remember that it
 * is executed from another thread.  A typical function might
 * set a flag or write to a pipe so that the rest of the
 * application knows that the JACK client thread has shut
 * down.
 *
 * NOTE: clients do not need to call this.  It exists only
 * to help more complex clients understand what is going
 * on.  It should be called before jack_client_activate().
 *
 * NOTE: if a client calls this AND jack_on_shutdown(), then
 * in case of a client thread shutdown, the callback passed to
 * jack_on_info_shutdown() will be called.
 *
 * NOTE: application should typically signal another thread to correctly 
 * finish cleanup, that is by calling "jack_client_close" 
 * (since "jack_client_close" cannot be called directly in the context 
 * of the thread that calls the shutdown callback).
 */
void jack_on_info_shutdown (jack_client_t *client,
                            JackInfoShutdownCallback shutdown_callback, void *arg) JACK_WEAK_EXPORT;

/**
 * Tell the Jack server to call @a process_callback whenever there is
 * work be done, passing @a arg as the second argument.
 *
 * The code in the supplied function must be suitable for real-time
 * execution.  That means that it cannot call functions that might
 * block for a long time. This includes malloc, free, printf,
 * pthread_mutex_lock, sleep, wait, poll, select, pthread_join,
 * pthread_cond_wait, etc, etc. See
 * http://jackit.sourceforge.net/docs/design/design.html#SECTION00411000000000000000
 * for more information.
 *
 * NOTE: this function cannot be called while the client is activated
 * (after jack_activate has been called.)
 *
 * @return 0 on success, otherwise a non-zero error code.
 */
int jack_set_process_callback (jack_client_t *client,
                               JackProcessCallback process_callback,
                               void *arg) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Tell the Jack server to call @a freewheel_callback
 * whenever we enter or leave "freewheel" mode, passing @a
 * arg as the second argument. The first argument to the
 * callback will be non-zero if JACK is entering freewheel
 * mode, and zero otherwise.
 *
 * All "notification events" are received in a seperated non RT thread,
 * the code in the supplied function does not need to be
 * suitable for real-time execution.
 *
 * NOTE: this function cannot be called while the client is activated
 * (after jack_activate has been called.)
 *
 * @return 0 on success, otherwise a non-zero error code.
 */
int jack_set_freewheel_callback (jack_client_t *client,
                                 JackFreewheelCallback freewheel_callback,
                                 void *arg) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Tell JACK to call @a bufsize_callback whenever the size of the the
 * buffer that will be passed to the @a process_callback is about to
 * change.  Clients that depend on knowing the buffer size must supply
 * a @a bufsize_callback before activating themselves.
 *
 * All "notification events" are received in a seperated non RT thread,
 * the code in the supplied function does not need to be
 * suitable for real-time execution.
 *
 * NOTE: this function cannot be called while the client is activated
 * (after jack_activate has been called.)
 *
 * @param client pointer to JACK client structure.
 * @param bufsize_callback function to call when the buffer size changes.
 * @param arg argument for @a bufsize_callback.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_set_buffer_size_callback (jack_client_t *client,
                                   JackBufferSizeCallback bufsize_callback,
                                   void *arg) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Tell the Jack server to call @a srate_callback whenever the system
 * sample rate changes.
 *
 * All "notification events" are received in a seperated non RT thread,
 * the code in the supplied function does not need to be
 * suitable for real-time execution.
 *
 * NOTE: this function cannot be called while the client is activated
 * (after jack_activate has been called.)
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_set_sample_rate_callback (jack_client_t *client,
                                   JackSampleRateCallback srate_callback,
                                   void *arg) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Tell the JACK server to call @a client_registration_callback whenever a
 * client is registered or unregistered, passing @a arg as a parameter.
 *
 * All "notification events" are received in a seperated non RT thread,
 * the code in the supplied function does not need to be
 * suitable for real-time execution.
 *
 * NOTE: this function cannot be called while the client is activated
 * (after jack_activate has been called.)
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_set_client_registration_callback (jack_client_t *client,
                                            JackClientRegistrationCallback
                                            registration_callback, void *arg) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Tell the JACK server to call @a registration_callback whenever a
 * port is registered or unregistered, passing @a arg as a parameter.
 *
 * All "notification events" are received in a seperated non RT thread,
 * the code in the supplied function does not need to be
 * suitable for real-time execution.
 *
 * NOTE: this function cannot be called while the client is activated
 * (after jack_activate has been called.)
 *
 * @return 0 on success, otherwise a non-zero error code
 */
 int jack_set_port_registration_callback (jack_client_t *client,
                                          JackPortRegistrationCallback
                                          registration_callback, void *arg) JACK_OPTIONAL_WEAK_EXPORT;

 /**
 * Tell the JACK server to call @a connect_callback whenever a
 * port is connected or disconnected, passing @a arg as a parameter.
 *
 * All "notification events" are received in a seperated non RT thread,
 * the code in the supplied function does not need to be
 * suitable for real-time execution.
 *
 * NOTE: this function cannot be called while the client is activated
 * (after jack_activate has been called.)
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_set_port_connect_callback (jack_client_t *client,
                                    JackPortConnectCallback
                                    connect_callback, void *arg) JACK_OPTIONAL_WEAK_EXPORT;

 /**
 * Tell the JACK server to call @a rename_callback whenever a
 * port is renamed, passing @a arg as a parameter.
 *
 * All "notification events" are received in a seperated non RT thread,
 * the code in the supplied function does not need to be
 * suitable for real-time execution.
 *
 * NOTE: this function cannot be called while the client is activated
 * (after jack_activate has been called.)
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_set_port_rename_callback (jack_client_t *client,
                                   JackPortRenameCallback
                                   rename_callback, void *arg) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Tell the JACK server to call @a graph_callback whenever the
 * processing graph is reordered, passing @a arg as a parameter.
 *
 * All "notification events" are received in a seperated non RT thread,
 * the code in the supplied function does not need to be
 * suitable for real-time execution.
 *
 * NOTE: this function cannot be called while the client is activated
 * (after jack_activate has been called.)
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_set_graph_order_callback (jack_client_t *client,
                                   JackGraphOrderCallback graph_callback,
                                   void *) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Tell the JACK server to call @a xrun_callback whenever there is a
 * xrun, passing @a arg as a parameter.
 *
 * All "notification events" are received in a seperated non RT thread,
 * the code in the supplied function does not need to be
 * suitable for real-time execution.
 *
 * NOTE: this function cannot be called while the client is activated
 * (after jack_activate has been called.)
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_set_xrun_callback (jack_client_t *client,
                            JackXRunCallback xrun_callback, void *arg) JACK_OPTIONAL_WEAK_EXPORT;

/*@}*/

/**
 * Tell the Jack server to call @a latency_callback whenever it
 * is necessary to recompute the latencies for some or all
 * Jack ports.
 *
 * @a latency_callback will be called twice each time it is
 * needed, once being passed JackCaptureLatency and once
 * JackPlaybackLatency. See @ref LatencyFunctions for
 * the definition of each type of latency and related functions.
 *
 * <b>IMPORTANT: Most JACK clients do NOT need to register a latency
 * callback.</b>
 *
 * Clients that meet any of the following conditions do NOT
 * need to register a latency callback:
 *
 *    - have only input ports
 *    - have only output ports
 *    - their output is totally unrelated to their input
 *    - their output is not delayed relative to their input
 *        (i.e. data that arrives in a given process()
 *         callback is processed and output again in the
 *         same callback)
 *
 * Clients NOT registering a latency callback MUST also
 * satisfy this condition:
 *
 *    - have no multiple distinct internal signal pathways
 *
 * This means that if your client has more than 1 input and
 * output port, and considers them always "correlated"
 * (e.g. as a stereo pair), then there is only 1 (e.g. stereo)
 * signal pathway through the client. This would be true,
 * for example, of a stereo FX rack client that has a
 * left/right input pair and a left/right output pair.
 *
 * However, this is somewhat a matter of perspective. The
 * same FX rack client could be connected so that its
 * two input ports were connected to entirely separate
 * sources. Under these conditions, the fact that the client
 * does not register a latency callback MAY result
 * in port latency values being incorrect.
 *
 * Clients that do not meet any of those conditions SHOULD
 * register a latency callback.
 *
 * See the documentation for  @ref jack_port_set_latency_range()
 * on how the callback should operate. Remember that the @a mode
 * argument given to the latency callback will need to be
 * passed into @ref jack_port_set_latency_range()
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_set_latency_callback (jack_client_t *client,
			       JackLatencyCallback latency_callback,
			       void *) JACK_WEAK_EXPORT;
/*@}*/

/**
 * @defgroup ServerClientControl Controlling & querying JACK server operation
 * @{
 */

/**
 * Start/Stop JACK's "freewheel" mode.
 *
 * When in "freewheel" mode, JACK no longer waits for
 * any external event to begin the start of the next process
 * cycle.
 *
 * As a result, freewheel mode causes "faster than realtime"
 * execution of a JACK graph. If possessed, real-time
 * scheduling is dropped when entering freewheel mode, and
 * if appropriate it is reacquired when stopping.
 *
 * IMPORTANT: on systems using capabilities to provide real-time
 * scheduling (i.e. Linux kernel 2.4), if onoff is zero, this function
 * must be called from the thread that originally called jack_activate().
 * This restriction does not apply to other systems (e.g. Linux kernel 2.6
 * or OS X).
 *
 * @param client pointer to JACK client structure
 * @param onoff  if non-zero, freewheel mode starts. Otherwise
 *                  freewheel mode ends.
 *
 * @return 0 on success, otherwise a non-zero error code.
 */
int jack_set_freewheel(jack_client_t* client, int onoff) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Change the buffer size passed to the @a process_callback.
 *
 * This operation stops the JACK engine process cycle, then calls all
 * registered @a bufsize_callback functions before restarting the
 * process cycle.  This will cause a gap in the audio flow, so it
 * should only be done at appropriate stopping points.
 *
 * @see jack_set_buffer_size_callback()
 *
 * @param client pointer to JACK client structure.
 * @param nframes new buffer size.  Must be a power of two.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_set_buffer_size (jack_client_t *client, jack_nframes_t nframes) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the sample rate of the jack system, as set by the user when
 * jackd was started.
 */
jack_nframes_t jack_get_sample_rate (jack_client_t *) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the current maximum size that will ever be passed to the @a
 * process_callback.  It should only be used *before* the client has
 * been activated.  This size may change, clients that depend on it
 * must register a @a bufsize_callback so they will be notified if it
 * does.
 *
 * @see jack_set_buffer_size_callback()
 */
jack_nframes_t jack_get_buffer_size (jack_client_t *) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Old-style interface to become the timebase for the entire JACK
 * subsystem.
 *
 * @deprecated This function still exists for compatibility with the
 * earlier transport interface, but it does nothing.  Instead, see
 * transport.h and use jack_set_timebase_callback().
 *
 * @return ENOSYS, function not implemented.
 */
int jack_engine_takeover_timebase (jack_client_t *) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;

/**
 * @return the current CPU load estimated by JACK.  This is a running
 * average of the time it takes to execute a full process cycle for
 * all clients as a percentage of the real time available per cycle
 * determined by the buffer size and sample rate.
 */
float jack_cpu_load (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

/*@}*/

/**
 * @defgroup PortFunctions Creating & manipulating ports
 * @{
 */

/**
 * Create a new port for the client. This is an object used for moving
 * data of any type in or out of the client.  Ports may be connected
 * in various ways.
 *
 * Each port has a short name.  The port's full name contains the name
 * of the client concatenated with a colon (:) followed by its short
 * name.  The jack_port_name_size() is the maximum length of this full
 * name.  Exceeding that will cause the port registration to fail and
 * return NULL.
 *
 * The @a port_name must be unique among all ports owned by this client.
 * If the name is not unique, the registration will fail.
 *
 * All ports have a type, which may be any non-NULL and non-zero
 * length string, passed as an argument.  Some port types are built
 * into the JACK API, currently only JACK_DEFAULT_AUDIO_TYPE.
 *
 * @param client pointer to JACK client structure.
 * @param port_name non-empty short name for the new port (not
 * including the leading @a "client_name:"). Must be unique.
 * @param port_type port type name.  If longer than
 * jack_port_type_size(), only that many characters are significant.
 * @param flags @ref JackPortFlags bit mask.
 * @param buffer_size must be non-zero if this is not a built-in @a
 * port_type.  Otherwise, it is ignored.
 *
 * @return jack_port_t pointer on success, otherwise NULL.
 */
jack_port_t * jack_port_register (jack_client_t *client,
                                  const char *port_name,
                                  const char *port_type,
                                  unsigned long flags,
                                  unsigned long buffer_size) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Remove the port from the client, disconnecting any existing
 * connections.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_port_unregister (jack_client_t *client, jack_port_t *port) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * This returns a pointer to the memory area associated with the
 * specified port. For an output port, it will be a memory area
 * that can be written to; for an input port, it will be an area
 * containing the data from the port's connection(s), or
 * zero-filled. if there are multiple inbound connections, the data
 * will be mixed appropriately.
 *
 * FOR OUTPUT PORTS ONLY : DEPRECATED in Jack 2.0 !!
 * ---------------------------------------------------
 * You may cache the value returned, but only between calls to
 * your "blocksize" callback. For this reason alone, you should
 * either never cache the return value or ensure you have
 * a "blocksize" callback and be sure to invalidate the cached
 * address from there.
 *
 * Caching output ports is DEPRECATED in Jack 2.0, due to some new optimization (like "pipelining").
 * Port buffers have to be retrieved in each callback for proper functionning.
 */
void * jack_port_get_buffer (jack_port_t *port, jack_nframes_t) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the UUID of the jack_port_t
 *
 * @see jack_uuid_to_string() to convert into a string representation
 */
jack_uuid_t jack_port_uuid (const jack_port_t *port) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the full name of the jack_port_t (including the @a
 * "client_name:" prefix).
 *
 * @see jack_port_name_size().
 */
const char * jack_port_name (const jack_port_t *port) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the short name of the jack_port_t (not including the @a
 * "client_name:" prefix).
 *
 * @see jack_port_name_size().
 */
const char * jack_port_short_name (const jack_port_t *port) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the @ref JackPortFlags of the jack_port_t.
 */
int jack_port_flags (const jack_port_t *port) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the @a port type, at most jack_port_type_size() characters
 * including a final NULL.
 */
const char * jack_port_type (const jack_port_t *port) JACK_OPTIONAL_WEAK_EXPORT;

 /**
 * @return the @a port type id.
 */
jack_port_type_id_t jack_port_type_id (const jack_port_t *port) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return TRUE if the jack_port_t belongs to the jack_client_t.
 */
int jack_port_is_mine (const jack_client_t *client, const jack_port_t *port) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return number of connections to or from @a port.
 *
 * @pre The calling client must own @a port.
 */
int jack_port_connected (const jack_port_t *port) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return TRUE if the locally-owned @a port is @b directly connected
 * to the @a port_name.
 *
 * @see jack_port_name_size()
 */
int jack_port_connected_to (const jack_port_t *port,
                            const char *port_name) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return a null-terminated array of full port names to which the @a
 * port is connected.  If none, returns NULL.
 *
 * The caller is responsible for calling jack_free() on any non-NULL
 * returned value.
 *
 * @param port locally owned jack_port_t pointer.
 *
 * @see jack_port_name_size(), jack_port_get_all_connections()
 */
const char ** jack_port_get_connections (const jack_port_t *port) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return a null-terminated array of full port names to which the @a
 * port is connected.  If none, returns NULL.
 *
 * The caller is responsible for calling jack_free() on any non-NULL
 * returned value.
 *
 * This differs from jack_port_get_connections() in two important
 * respects:
 *
 *     1) You may not call this function from code that is
 *          executed in response to a JACK event. For example,
 *          you cannot use it in a GraphReordered handler.
 *
 *     2) You need not be the owner of the port to get information
 *          about its connections.
 *
 * @see jack_port_name_size()
 */
const char ** jack_port_get_all_connections (const jack_client_t *client,
                                             const jack_port_t *port) JACK_OPTIONAL_WEAK_EXPORT;

/**
 *
 * @deprecated This function will be removed from a future version
 * of JACK. Do not use it. There is no replacement. It has
 * turned out to serve essentially no purpose in real-life
 * JACK clients.
 */
int jack_port_tie (jack_port_t *src, jack_port_t *dst) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;

/**
 *
 * @deprecated This function will be removed from a future version
 * of JACK. Do not use it. There is no replacement. It has
 * turned out to serve essentially no purpose in real-life
 * JACK clients.
 */
int jack_port_untie (jack_port_t *port) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;

/**
 * \bold THIS FUNCTION IS DEPRECATED AND SHOULD NOT BE USED IN
 *  NEW JACK CLIENTS
 *
 * Modify a port's short name.  May be called at any time.  If the
 * resulting full name (including the @a "client_name:" prefix) is
 * longer than jack_port_name_size(), it will be truncated.
 *
 * @return 0 on success, otherwise a non-zero error code.
 */
int jack_port_set_name (jack_port_t *port, const char *port_name) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;

/**
 * Modify a port's short name.  May NOT be called from a callback handling a server event.
 * If the resulting full name (including the @a "client_name:" prefix) is
 * longer than jack_port_name_size(), it will be truncated.
 *
 * @return 0 on success, otherwise a non-zero error code.
 *
 * This differs from jack_port_set_name() by triggering PortRename notifications to
 * clients that have registered a port rename handler.
 */
int jack_port_rename (jack_client_t* client, jack_port_t *port, const char *port_name) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Set @a alias as an alias for @a port.  May be called at any time.
 * If the alias is longer than jack_port_name_size(), it will be truncated.
 *
 * After a successful call, and until JACK exits or
 * @function jack_port_unset_alias() is called, @alias may be
 * used as a alternate name for the port.
 *
 * Ports can have up to two aliases - if both are already
 * set, this function will return an error.
 *
 * @return 0 on success, otherwise a non-zero error code.
 */
int jack_port_set_alias (jack_port_t *port, const char *alias) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Remove @a alias as an alias for @a port.  May be called at any time.
 *
 * After a successful call, @a alias can no longer be
 * used as a alternate name for the port.
 *
 * @return 0 on success, otherwise a non-zero error code.
 */
int jack_port_unset_alias (jack_port_t *port, const char *alias) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Get any aliases known for @port.
 *
 * @return the number of aliases discovered for the port
 */
int jack_port_get_aliases (const jack_port_t *port, char* const aliases[2]) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * If @ref JackPortCanMonitor is set for this @a port, turn input
 * monitoring on or off.  Otherwise, do nothing.
 */
int jack_port_request_monitor (jack_port_t *port, int onoff) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * If @ref JackPortCanMonitor is set for this @a port_name, turn input
 * monitoring on or off.  Otherwise, do nothing.
 *
 * @return 0 on success, otherwise a non-zero error code.
 *
 * @see jack_port_name_size()
 */
int jack_port_request_monitor_by_name (jack_client_t *client,
                                       const char *port_name, int onoff) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * If @ref JackPortCanMonitor is set for a port, this function turns
 * on input monitoring if it was off, and turns it off if only one
 * request has been made to turn it on.  Otherwise it does nothing.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_port_ensure_monitor (jack_port_t *port, int onoff) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return TRUE if input monitoring has been requested for @a port.
 */
int jack_port_monitoring_input (jack_port_t *port) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Establish a connection between two ports.
 *
 * When a connection exists, data written to the source port will
 * be available to be read at the destination port.
 *
 * @pre The port types must be identical.
 *
 * @pre The @ref JackPortFlags of the @a source_port must include @ref
 * JackPortIsOutput.
 *
 * @pre The @ref JackPortFlags of the @a destination_port must include
 * @ref JackPortIsInput.
 *
 * @return 0 on success, EEXIST if the connection is already made,
 * otherwise a non-zero error code
 */
int jack_connect (jack_client_t *client,
                  const char *source_port,
                  const char *destination_port) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Remove a connection between two ports.
 *
 * @pre The port types must be identical.
 *
 * @pre The @ref JackPortFlags of the @a source_port must include @ref
 * JackPortIsOutput.
 *
 * @pre The @ref JackPortFlags of the @a destination_port must include
 * @ref JackPortIsInput.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_disconnect (jack_client_t *client,
                     const char *source_port,
                     const char *destination_port) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Perform the same function as jack_disconnect() using port handles
 * rather than names.  This avoids the name lookup inherent in the
 * name-based version.
 *
 * Clients connecting their own ports are likely to use this function,
 * while generic connection clients (e.g. patchbays) would use
 * jack_disconnect().
 */
int jack_port_disconnect (jack_client_t *client, jack_port_t *port) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the maximum number of characters in a full JACK port name
 * including the final NULL character.  This value is a constant.
 *
 * A port's full name contains the owning client name concatenated
 * with a colon (:) followed by its short name and a NULL
 * character.
 */
int jack_port_name_size(void) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the maximum number of characters in a JACK port type name
 * including the final NULL character.  This value is a constant.
 */
int jack_port_type_size(void) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the buffersize of a port of type @arg port_type.
 *
 * this function may only be called in a buffer_size callback.
 */
size_t jack_port_type_get_buffer_size (jack_client_t *client, const char *port_type) JACK_WEAK_EXPORT;

/*@}*/

/**
 * @defgroup LatencyFunctions Managing and determining latency
 *
 * The purpose of JACK's latency API is to allow clients to
 * easily answer two questions:
 *
 * - How long has it been since the data read from a port arrived
 *   at the edge of the JACK graph (either via a physical port
 *   or being synthesized from scratch)?
 *
 * - How long will it be before the data written to a port arrives
 *   at the edge of a JACK graph?

 * To help answering these two questions, all JACK ports have two
 * latency values associated with them, both measured in frames:
 *
 * <b>capture latency</b>: how long since the data read from
 *                  the buffer of a port arrived at
 *                  a port marked with JackPortIsTerminal.
 *                  The data will have come from the "outside
 *                  world" if the terminal port is also
 *                  marked with JackPortIsPhysical, or
 *                  will have been synthesized by the client
 *                  that owns the terminal port.
 *
 * <b>playback latency</b>: how long until the data
 *                   written to the buffer of port will reach a port
 *                   marked with JackPortIsTerminal.
 *
 * Both latencies might potentially have more than one value
 * because there may be multiple pathways to/from a given port
 * and a terminal port. Latency is therefore generally
 * expressed a min/max pair.
 *
 * In most common setups, the minimum and maximum latency
 * are the same, but this design accomodates more complex
 * routing, and allows applications (and thus users) to
 * detect cases where routing is creating an anomalous
 * situation that may either need fixing or more
 * sophisticated handling by clients that care about
 * latency.
 *
 * See also @ref jack_set_latency_callback for details on how
 * clients that add latency to the signal path should interact
 * with JACK to ensure that the correct latency figures are
 * used.
 * @{
 */

/**
 * The port latency is zero by default. Clients that control
 * physical hardware with non-zero latency should call this
 * to set the latency to its correct value. Note that the value
 * should include any systemic latency present "outside" the
 * physical hardware controlled by the client. For example,
 * for a client controlling a digital audio interface connected
 * to an external digital converter, the latency setting should
 * include both buffering by the audio interface *and* the converter.
 *
 * @deprecated This method will be removed in the next major
 * release of JACK. It should not be used in new code, and should
 * be replaced by a latency callback that calls @ref
 * jack_port_set_latency_range().
 */
void jack_port_set_latency (jack_port_t *port, jack_nframes_t) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;

/**
 * return the latency range defined by @a mode for
 * @a port, in frames.
 *
 * See @ref LatencyFunctions for the definition of each latency value.
 *
 * This is normally used in the LatencyCallback.
 * and therefor safe to execute from callbacks.
 */
void jack_port_get_latency_range (jack_port_t *port, jack_latency_callback_mode_t mode, jack_latency_range_t *range) JACK_WEAK_EXPORT;

/**
 * set the minimum and maximum latencies defined by
 * @a mode for @a port, in frames.
 *
 * See @ref LatencyFunctions for the definition of each latency value.
 *
 * This function should ONLY be used inside a latency
 * callback. The client should determine the current
 * value of the latency using @ref jack_port_get_latency_range()
 * (called using the same mode as @a mode)
 * and then add some number of frames to that reflects
 * latency added by the client.
 *
 * How much latency a client adds will vary
 * dramatically. For most clients, the answer is zero
 * and there is no reason for them to register a latency
 * callback and thus they should never call this
 * function.
 *
 * More complex clients that take an input signal,
 * transform it in some way and output the result but
 * not during the same process() callback will
 * generally know a single constant value to add
 * to the value returned by @ref jack_port_get_latency_range().
 *
 * Such clients would register a latency callback (see
 * @ref jack_set_latency_callback) and must know what input
 * ports feed which output ports as part of their
 * internal state. Their latency callback will update
 * the ports' latency values appropriately.
 *
 * A pseudo-code example will help. The @a mode argument to the latency
 * callback will determine whether playback or capture
 * latency is being set. The callback will use
 * @ref jack_port_set_latency_range() as follows:
 *
 * \code
 * jack_latency_range_t range;
 * if (mode == JackPlaybackLatency) {
 *  foreach input_port in (all self-registered port) {
 *   jack_port_get_latency_range (port_feeding_input_port, JackPlaybackLatency, &range);
 *   range.min += min_delay_added_as_signal_flows_from port_feeding to input_port;
 *   range.max += max_delay_added_as_signal_flows_from port_feeding to input_port;
 *   jack_port_set_latency_range (input_port, JackPlaybackLatency, &range);
 *  }
 * } else if (mode == JackCaptureLatency) {
 *  foreach output_port in (all self-registered port) {
 *   jack_port_get_latency_range (port_fed_by_output_port, JackCaptureLatency, &range);
 *   range.min += min_delay_added_as_signal_flows_from_output_port_to_fed_by_port;
 *   range.max += max_delay_added_as_signal_flows_from_output_port_to_fed_by_port;
 *   jack_port_set_latency_range (output_port, JackCaptureLatency, &range);
 *  }
 * }
 * \endcode
 *
 * In this relatively simple pseudo-code example, it is assumed that
 * each input port or output is connected to only 1 output or input
 * port respectively.
 *
 * If a port is connected to more than 1 other port, then the
 * range.min and range.max values passed to @ref
 * jack_port_set_latency_range() should reflect the minimum and
 * maximum values across all connected ports.
 *
 * See the description of @ref jack_set_latency_callback for more
 * information.
 */
void jack_port_set_latency_range (jack_port_t *port, jack_latency_callback_mode_t mode, jack_latency_range_t *range) JACK_WEAK_EXPORT;

/**
 * Request a complete recomputation of all port latencies. This
 * can be called by a client that has just changed the internal
 * latency of its port using  jack_port_set_latency
 * and wants to ensure that all signal pathways in the graph
 * are updated with respect to the values that will be returned
 * by  jack_port_get_total_latency. It allows a client
 * to change multiple port latencies without triggering a
 * recompute for each change.
 *
 * @return zero for successful execution of the request. non-zero
 *         otherwise.
 */
int jack_recompute_total_latencies (jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the time (in frames) between data being available or
 * delivered at/to a port, and the time at which it arrived at or is
 * delivered to the "other side" of the port.  E.g. for a physical
 * audio output port, this is the time between writing to the port and
 * when the signal will leave the connector.  For a physical audio
 * input port, this is the time between the sound arriving at the
 * connector and the corresponding frames being readable from the
 * port.
 *
 * @deprecated This method will be removed in the next major
 * release of JACK. It should not be used in new code, and should
 * be replaced by jack_port_get_latency_range() in any existing
 * use cases.
 */
jack_nframes_t jack_port_get_latency (jack_port_t *port) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;

/**
 * The maximum of the sum of the latencies in every
 * connection path that can be drawn between the port and other
 * ports with the @ref JackPortIsTerminal flag set.
 *
 * @deprecated This method will be removed in the next major
 * release of JACK. It should not be used in new code, and should
 * be replaced by jack_port_get_latency_range() in any existing
 * use cases.
 */
jack_nframes_t jack_port_get_total_latency (jack_client_t *client,
					    jack_port_t *port) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;

/**
 * Request a complete recomputation of a port's total latency. This
 * can be called by a client that has just changed the internal
 * latency of its port using  jack_port_set_latency
 * and wants to ensure that all signal pathways in the graph
 * are updated with respect to the values that will be returned
 * by  jack_port_get_total_latency.
 *
 * @return zero for successful execution of the request. non-zero
 *         otherwise.
 *
 * @deprecated This method will be removed in the next major
 * release of JACK. It should not be used in new code, and should
 * be replaced by jack_recompute_total_latencies() in any existing
 * use cases.
 */
int jack_recompute_total_latency (jack_client_t*, jack_port_t* port) JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT;

/*@}*/

/**
 * @defgroup PortSearching Looking up ports
 * @{
 */

/**
 * @param port_name_pattern A regular expression used to select
 * ports by name.  If NULL or of zero length, no selection based
 * on name will be carried out.
 * @param type_name_pattern A regular expression used to select
 * ports by type.  If NULL or of zero length, no selection based
 * on type will be carried out.
 * @param flags A value used to select ports by their flags.
 * If zero, no selection based on flags will be carried out.
 *
 * @return a NULL-terminated array of ports that match the specified
 * arguments.  The caller is responsible for calling jack_free() any
 * non-NULL returned value.
 *
 * @see jack_port_name_size(), jack_port_type_size()
 */
const char ** jack_get_ports (jack_client_t *client,
                              const char *port_name_pattern,
                              const char *type_name_pattern,
                              unsigned long flags) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return address of the jack_port_t named @a port_name.
 *
 * @see jack_port_name_size()
 */
jack_port_t * jack_port_by_name (jack_client_t *client, const char *port_name) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return address of the jack_port_t of a @a port_id.
 */
jack_port_t * jack_port_by_id (jack_client_t *client,
                               jack_port_id_t port_id) JACK_OPTIONAL_WEAK_EXPORT;

/*@}*/

/**
 * @defgroup TimeFunctions Handling time
 * @{
 *
 * JACK time is in units of 'frames', according to the current sample rate.
 * The absolute value of frame times is meaningless, frame times have meaning
 * only relative to each other.
 */

/**
 * @return the estimated time in frames that has passed since the JACK
 * server began the current process cycle.
 */
jack_nframes_t jack_frames_since_cycle_start (const jack_client_t *) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the estimated current time in frames.
 * This function is intended for use in other threads (not the process
 * callback).  The return value can be compared with the value of
 * jack_last_frame_time to relate time in other threads to JACK time.
 */
jack_nframes_t jack_frame_time (const jack_client_t *) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the precise time at the start of the current process cycle.
 * This function may only be used from the process callback, and can
 * be used to interpret timestamps generated by jack_frame_time() in
 * other threads with respect to the current process cycle.
 *
 * This is the only jack time function that returns exact time:
 * when used during the process callback it always returns the same
 * value (until the next process callback, where it will return
 * that value + nframes, etc).  The return value is guaranteed to be
 * monotonic and linear in this fashion unless an xrun occurs.
 * If an xrun occurs, clients must check this value again, as time
 * may have advanced in a non-linear way (e.g. cycles may have been skipped).
 */
jack_nframes_t jack_last_frame_time (const jack_client_t *client) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * This function may only be used from the process callback.
 * It provides the internal cycle timing information as used by
 * most of the other time related functions. This allows the
 * caller to map between frame counts and microseconds with full
 * precision (i.e. without rounding frame times to integers),
 * and also provides e.g. the microseconds time of the start of
 * the current cycle directly (it has to be computed otherwise).
 *
 * If the return value is zero, the following information is
 * provided in the variables pointed to by the arguments:
 *
 * current_frames: the frame time counter at the start of the
 *                 current cycle, same as jack_last_frame_time().
 * current_usecs:  the microseconds time at the start of the
 *                 current cycle.
 * next_usecs:     the microseconds time of the start of the next
 *                 next cycle as computed by the DLL.
 * period_usecs:   the current best estimate of the period time in
 *                  microseconds.
 *
 * NOTES:
 * 
 * Because of the types used, all the returned values except period_usecs
 * are unsigned. In computations mapping between frames and microseconds
 * *signed* differences are required. The easiest way is to compute those
 * separately and assign them to the appropriate signed variables,
 * int32_t for frames and int64_t for usecs. See the implementation of
 * jack_frames_to_time() and Jack_time_to_frames() for an example.
 * 
 * Unless there was an xrun, skipped cycles, or the current cycle is the
 * first after freewheeling or starting Jack, the value of current_usecs
 * will always be the value of next_usecs of the previous cycle.
 *
 * The value of period_usecs will in general NOT be exactly equal to
 * the difference of next_usecs and current_usecs. This is because to
 * ensure stability of the DLL and continuity of the mapping, a fraction
 * of the loop error must be included in next_usecs. For an accurate
 * mapping between frames and microseconds, the difference of next_usecs
 * and current_usecs should be used, and not period_usecs.
 *
 * @return zero if OK, non-zero otherwise.
 */
int jack_get_cycle_times(const jack_client_t *client,
                        jack_nframes_t *current_frames,
                        jack_time_t    *current_usecs,
                        jack_time_t    *next_usecs,
                        float          *period_usecs) JACK_OPTIONAL_WEAK_EXPORT;
                  
/**
 * @return the estimated time in microseconds of the specified frame time
 */
jack_time_t jack_frames_to_time(const jack_client_t *client, jack_nframes_t) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return the estimated time in frames for the specified system time.
 */
jack_nframes_t jack_time_to_frames(const jack_client_t *client, jack_time_t) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * @return return JACK's current system time in microseconds,
 *         using the JACK clock source.
 *
 * The value returned is guaranteed to be monotonic, but not linear.
 */
jack_time_t jack_get_time(void) JACK_OPTIONAL_WEAK_EXPORT;

/*@}*/

/**
 * @defgroup ErrorOutput Controlling error/information output
 */
/*@{*/

/**
 * Display JACK error message.
 *
 * Set via jack_set_error_function(), otherwise a JACK-provided
 * default will print @a msg (plus a newline) to stderr.
 *
 * @param msg error message text (no newline at end).
 */
extern void (*jack_error_callback)(const char *msg) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Set the @ref jack_error_callback for error message display.
 * Set it to NULL to restore default_jack_error_callback function.
 *
 * The JACK library provides two built-in callbacks for this purpose:
 * default_jack_error_callback() and silent_jack_error_callback().
 */
void jack_set_error_function (void (*func)(const char *)) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Display JACK info message.
 *
 * Set via jack_set_info_function(), otherwise a JACK-provided
 * default will print @a msg (plus a newline) to stdout.
 *
 * @param msg info message text (no newline at end).
 */
extern void (*jack_info_callback)(const char *msg) JACK_OPTIONAL_WEAK_EXPORT;

/**
 * Set the @ref jack_info_callback for info message display.
 * Set it to NULL to restore default_jack_info_callback function.
 *
 * The JACK library provides two built-in callbacks for this purpose:
 * default_jack_info_callback() and silent_jack_info_callback().
 */
void jack_set_info_function (void (*func)(const char *)) JACK_OPTIONAL_WEAK_EXPORT;

/*@}*/

/**
 * The free function to be used on memory returned by jack_port_get_connections,
 * jack_port_get_all_connections, jack_get_ports and jack_get_internal_client_name functions.
 * This is MANDATORY on Windows when otherwise all nasty runtime version related crashes can occur.
 * Developers are strongly encouraged to use this function instead of the standard "free" function in new code.
 *
 * @param ptr the memory pointer to be deallocated.
 */
void jack_free(void* ptr) JACK_OPTIONAL_WEAK_EXPORT;


#ifdef __cplusplus
}
#endif

#endif /* __jack_h__ */
