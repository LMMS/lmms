
/*******************************************************************************/
/* Copyright (C) 2012 Jonathan Moore Liles                                     */
/*                                                                             */
/* This program is free software; you can redistribute it and/or modify it     */
/* under the terms of the GNU General Public License as published by the       */
/* Free Software Foundation; either version 2 of the License, or (at your      */
/* option) any later version.                                                  */
/*                                                                             */
/* This program is distributed in the hope that it will be useful, but WITHOUT */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       */
/* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   */
/* more details.                                                               */
/*                                                                             */
/* You should have received a copy of the GNU General Public License along     */
/* with This program; see the file COPYING.  If not,write to the Free Software */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */
/*******************************************************************************/

#pragma once

#include <lo/lo.h>

namespace NSM
{
    class Client
    {
        private:

            const char *nsm_url;

            lo_server _server;
            lo_server_thread _st;
            lo_address       nsm_addr;

            bool  nsm_is_active;
            char *nsm_client_id;
            char *_session_manager_name;

        public:

            enum {
                ERR_OK      = 0,
                ERR_GENERAL = -1,
                ERR_INCOMPATIBLE_API = -2,
                ERR_BLACKLISTED      = -3,
                ERR_LAUNCH_FAILED    = -4,
                ERR_NO_SUCH_FILE     = -5,
                ERR_NO_SESSION_OPEN  = -6,
                ERR_UNSAVED_CHANGES  = -7,
                ERR_NOT_NOW = -8
            };

            Client();
            virtual ~Client();

            bool is_active(void) { return nsm_is_active; }

            const char *session_manager_name(void) {
                return
                    _session_manager_name;
            }

            /* Client->Server methods */
            void is_dirty(void);
            void is_clean(void);
            void progress(float f);
            void message(int priority, const char *msg);
            void announce(const char *appliction_name,
                          const char *capabilities,
                          const char *process_name);

            void broadcast(lo_message msg);

            /* init without threading */
            int init(const char *nsm_url);
            /* init with threading */
            int init_thread(const char *nsm_url);

            /* call this periodically to check for new messages */
            void check(int timeout = 0);

            /* or call these to start and stop a thread (must do your own locking in handler!) */
            void start(void);
            void stop(void);

        protected:

            /* Server->Client methods */
            virtual int command_open(const char *name,
                                     const char *display_name,
                                     const char *client_id,
                                     char **out_msg) = 0;
            virtual int command_save(char **out_msg) = 0;

            virtual void command_active(bool) { }

            virtual void command_session_is_loaded(void) { }

            /* invoked when an unrecognized message is received. Should return 0 if you handled it, -1 otherwise. */
            virtual int command_broadcast(const char *, lo_message) { return -1; }

        private:

            /* osc handlers */
            static int osc_open(const char *path,
                                const char *types,
                                lo_arg **argv,
                                int argc,
                                lo_message msg,
                                void *user_data);
            static int osc_save(const char *path,
                                const char *types,
                                lo_arg **argv,
                                int argc,
                                lo_message msg,
                                void *user_data);
            static int osc_announce_reply(const char *path,
                                          const char *types,
                                          lo_arg **argv,
                                          int argc,
                                          lo_message msg,
                                          void *user_data);
            static int osc_error(const char *path,
                                 const char *types,
                                 lo_arg **argv,
                                 int argc,
                                 lo_message msg,
                                 void *user_data);
            static int osc_session_is_loaded(const char *path,
                                             const char *types,
                                             lo_arg **argv,
                                             int argc,
                                             lo_message msg,
                                             void *user_data);
            static int osc_broadcast(const char *path,
                                     const char *types,
                                     lo_arg **argv,
                                     int argc,
                                     lo_message msg,
                                     void *user_data);
    };
};
