/*
    Copyright (C) 2001 Paul Davis
    Copyright (C) 2004 Jack O'Quin
    Copyright (C) 2010 Torben Hohn

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

#ifndef __jack_session_h__
#define __jack_session_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <jack/types.h>
#include <jack/weakmacros.h>

/**
 * @defgroup SessionClientFunctions Session API for clients.
 * @{
 */


/**
 * Session event type.
 *
 * If a client cant save templates, i might just do a normal save.
 *
 * There is no "quit without saving" event because a client might refuse to
 * quit when it has unsaved data, but other clients may have already quit.
 * This results in too much confusion, so it is unsupported.
 */
enum JackSessionEventType {
	/**
	 * Save the session completely.
	 *
	 * The client may save references to data outside the provided directory,
	 * but it must do so by creating a link inside the provided directory and
	 * referring to that in any save files. The client must not refer to data
	 * files outside the provided directory directly in save files, because
	 * this makes it impossible for the session manager to create a session
	 * archive for distribution or archival.
	 */
    JackSessionSave = 1,

    /**
     * Save the session completly, then quit.
     *
     * The rules for saving are exactly the same as for JackSessionSave.
     */
    JackSessionSaveAndQuit = 2,

    /**
     * Save a session template.
     *
     * A session template is a "skeleton" of the session, but without any data.
     * Clients must save a session that, when restored, will create the same
     * ports as a full save would have. However, the actual data contained in
     * the session may not be saved (e.g. a DAW would create the necessary
     * tracks, but not save the actual recorded data).
     */
    JackSessionSaveTemplate = 3
};

typedef enum JackSessionEventType jack_session_event_type_t;

/**
 * @ref jack_session_flags_t bits
 */
enum JackSessionFlags {
    /**
     * An error occured while saving.
     */
    JackSessionSaveError = 0x01,

    /**
     * Client needs to be run in a terminal.
     */
    JackSessionNeedTerminal = 0x02
};

/**
 * Session flags.
 */
typedef enum JackSessionFlags jack_session_flags_t;

struct _jack_session_event {
    /**
     * The type of this session event.
     */
    jack_session_event_type_t type;

    /**
     * Session directory path, with trailing separator.
     *
     * This directory is exclusive to the client; when saving the client may
     * create any files it likes in this directory.
     */
    const char *session_dir;

    /**
     * Client UUID which must be passed to jack_client_open on session load.
     *
     * The client can specify this in the returned command line, or save it
     * in a state file within the session directory.
     */
    const char *client_uuid;

    /**
     * Reply (set by client): the command line needed to restore the client.
     *
     * This is a platform dependent command line. It must contain
     * ${SESSION_DIR} instead of the actual session directory path. More
     * generally, just as in session files, clients should not include any
     * paths outside the session directory here as this makes
     * archival/distribution impossible.
     *
     * This field is set to NULL by Jack when the event is delivered to the
     * client.  The client must set to allocated memory that is safe to
     * free(). This memory will be freed by jack_session_event_free.
     */
    char *command_line;

    /**
     * Reply (set by client): Session flags.
     */
    jack_session_flags_t flags;

    /**
     * Future flags. Set to zero for now.
     */
    uint32_t future;
};

typedef struct _jack_session_event jack_session_event_t;

/**
 * Prototype for the client supplied function that is called
 * whenever a session notification is sent via jack_session_notify().
 *
 * Ownership of the memory of @a event is passed to the application.
 * It must be freed using jack_session_event_free when its not used anymore.
 *
 * The client must promptly call jack_session_reply for this event.
 *
 * @param event The event structure.
 * @param arg Pointer to a client supplied structure.
 */
typedef void (*JackSessionCallback)(jack_session_event_t *event,
                                    void                 *arg);

/**
 * Tell the JACK server to call @a session_callback when a session event
 * is to be delivered.
 *
 * setting more than one session_callback per process is probably a design
 * error. if you have a multiclient application its more sensible to create
 * a jack_client with only a session callback set.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_set_session_callback (jack_client_t       *client,
                               JackSessionCallback  session_callback,
                               void                *arg) JACK_WEAK_EXPORT;

/**
 * Reply to a session event.
 *
 * This can either be called directly from the callback, or later from a
 * different thread.  For example, it is possible to push the event through a
 * queue and execute the save code from the GUI thread.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int jack_session_reply (jack_client_t        *client,
                        jack_session_event_t *event) JACK_WEAK_EXPORT;


/**
 * Free memory used by a jack_session_event_t.
 *
 * This also frees the memory used by the command_line pointer, if its non NULL.
 */
void jack_session_event_free (jack_session_event_t *event) JACK_WEAK_EXPORT;


/**
 * Get the assigned uuid for client.
 * Safe to call from callback and all other threads.
 *
 * The caller is responsible for calling jack_free(3) on any non-NULL
 * returned value.
 */
char *jack_client_get_uuid (jack_client_t *client) JACK_WEAK_EXPORT;

/**
 * @}
 */

/**
 * @defgroup JackSessionManagerAPI API for a session manager.
 *
 * @{
 */

typedef struct  {
	const char           *uuid;
	const char           *client_name;
	const char           *command;
	jack_session_flags_t  flags;
} jack_session_command_t;

/**
 * Send an event to all clients listening for session callbacks.
 *
 * The returned strings of the clients are accumulated and returned as an array
 * of jack_session_command_t. its terminated by ret[i].uuid == NULL target ==
 * NULL means send to all interested clients. otherwise a clientname
 */
jack_session_command_t *jack_session_notify (
	jack_client_t*             client,
	const char                *target,
	jack_session_event_type_t  type,
	const char                *path) JACK_WEAK_EXPORT;

/**
 * Free the memory allocated by a session command.
 */
void jack_session_commands_free (jack_session_command_t *cmds) JACK_WEAK_EXPORT;

/**
 * Reserve a client name and associate it with a UUID.
 *
 * When a client later calls jack_client_open() and specifies the UUID, jackd
 * will assign the reserved name. This allows a session manager to know in
 * advance under which client name its managed clients will appear.
 *
 * @return 0 on success, otherwise a non-zero error code
 */
int
jack_reserve_client_name (jack_client_t *client,
                          const char    *name,
                          const char    *uuid) JACK_WEAK_EXPORT;

/**
 * Find out whether a client has set up a session callback.
 *
 * @return 0 when the client has no session callback, 1 when it has one.
 *        -1 on error.
 */
int
jack_client_has_session_callback (jack_client_t *client, const char *client_name) JACK_WEAK_EXPORT;

#ifdef __cplusplus
}
#endif
#endif
