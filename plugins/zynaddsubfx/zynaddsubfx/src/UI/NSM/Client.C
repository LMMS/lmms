
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

#include "Client.H"
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"

namespace NSM
{
/************************/
/* OSC Message Handlers */
/************************/

#undef OSC_REPLY
#undef OSC_REPLY_ERR

#define OSC_REPLY(value) lo_send_from(((NSM::Client *)user_data)->nsm_addr, \
                                      ((NSM::Client *)user_data)->_server, \
                                      LO_TT_IMMEDIATE, \
                                      "/reply", \
                                      "ss", \
                                      path, \
                                      value)

#define OSC_REPLY_ERR(errcode, value) lo_send_from( \
        ((NSM::Client *)user_data)->nsm_addr, \
        ((NSM::Client *)user_data)->_server, \
        LO_TT_IMMEDIATE, \
        "/error", \
        "sis", \
        path, \
        errcode, \
        value)

    Client::Client()
    {
        nsm_addr      = 0;
        nsm_client_id = 0;
        _session_manager_name = 0;
        nsm_is_active = false;
        _server = 0;
        _st     = 0;
    }

    Client::~Client()
    {
        if(_st)
            stop();

        if(_st)
            lo_server_thread_free(_st);
        else
            lo_server_free(_server);
    }

    void
    Client::announce(const char *application_name,
                     const char *capabilities,
                     const char *process_name)
    {
        // MESSAGE( "Announcing to NSM" );

        lo_address to = lo_address_new_from_url(nsm_url);

        if(!to)
            //    MESSAGE( "Bad address" );
            return;

        int pid = (int)getpid();

        lo_send_from(to,
                     _server,
                     LO_TT_IMMEDIATE,
                     "/nsm/server/announce",
                     "sssiii",
                     application_name,
                     capabilities,
                     process_name,
                     1,
                     /* api_major_version */
                     0,
                     /* api_minor_version */
                     pid);

        lo_address_free(to);
    }

    void
    Client::progress(float p)
    {
        if(nsm_is_active)
            lo_send_from(nsm_addr,
                         _server,
                         LO_TT_IMMEDIATE,
                         "/nsm/client/progress",
                         "f",
                         p);
    }

    void
    Client::is_dirty(void)
    {
        if(nsm_is_active)
            lo_send_from(nsm_addr,
                         _server,
                         LO_TT_IMMEDIATE,
                         "/nsm/client/is_dirty",
                         "");
    }

    void
    Client::is_clean(void)
    {
        if(nsm_is_active)
            lo_send_from(nsm_addr,
                         _server,
                         LO_TT_IMMEDIATE,
                         "/nsm/client/is_clean",
                         "");
    }

    void
    Client::message(int priority, const char *msg)
    {
        if(nsm_is_active)
            lo_send_from(nsm_addr,
                         _server,
                         LO_TT_IMMEDIATE,
                         "/nsm/client/message",
                         "is",
                         priority,
                         msg);
    }


    void
    Client::broadcast(lo_message msg)
    {
        if(nsm_is_active)
            lo_send_message_from(nsm_addr,
                                 _server,
                                 "/nsm/server/broadcast",
                                 msg);
    }

    void
    Client::check(int timeout)
    {
        if(lo_server_wait(_server, timeout))
            while(lo_server_recv_noblock(_server, 0)) {}
    }

    void
    Client::start()
    {
        lo_server_thread_start(_st);
    }

    void
    Client::stop()
    {
        lo_server_thread_stop(_st);
    }

    int
    Client::init(const char *nsm_url)
    {
        this->nsm_url = nsm_url;

        lo_address addr = lo_address_new_from_url(nsm_url);
        int proto = lo_address_get_protocol(addr);
        lo_address_free(addr);

        _server = lo_server_new_with_proto(NULL, proto, NULL);

        if(!_server)
            return -1;

        lo_server_add_method(_server, "/error", "sis", &Client::osc_error, this);
        lo_server_add_method(_server,
                             "/reply",
                             "ssss",
                             &Client::osc_announce_reply,
                             this);
        lo_server_add_method(_server,
                             "/nsm/client/open",
                             "sss",
                             &Client::osc_open,
                             this);
        lo_server_add_method(_server,
                             "/nsm/client/save",
                             "",
                             &Client::osc_save,
                             this);
        lo_server_add_method(_server,
                             "/nsm/client/session_is_loaded",
                             "",
                             &Client::osc_session_is_loaded,
                             this);
        lo_server_add_method(_server, NULL, NULL, &Client::osc_broadcast, this);

        return 0;
    }

    int
    Client::init_thread(const char *nsm_url)
    {
        this->nsm_url = nsm_url;

        lo_address addr = lo_address_new_from_url(nsm_url);
        int proto = lo_address_get_protocol(addr);
        lo_address_free(addr);

        _st     = lo_server_thread_new_with_proto(NULL, proto, NULL);
        _server = lo_server_thread_get_server(_st);

        if(!_server || !_st)
            return -1;

        lo_server_thread_add_method(_st,
                                    "/error",
                                    "sis",
                                    &Client::osc_error,
                                    this);
        lo_server_thread_add_method(_st,
                                    "/reply",
                                    "ssss",
                                    &Client::osc_announce_reply,
                                    this);
        lo_server_thread_add_method(_st,
                                    "/nsm/client/open",
                                    "sss",
                                    &Client::osc_open,
                                    this);
        lo_server_thread_add_method(_st,
                                    "/nsm/client/save",
                                    "",
                                    &Client::osc_save,
                                    this);
        lo_server_thread_add_method(_st,
                                    "/nsm/client/session_is_loaded",
                                    "",
                                    &Client::osc_session_is_loaded,
                                    this);
        lo_server_thread_add_method(_st,
                                    NULL,
                                    NULL,
                                    &Client::osc_broadcast,
                                    this);

        return 0;
    }

/************************/
/* OSC Message Handlers */
/************************/

    int
    Client::osc_broadcast(const char *path,
                          const char *types,
                          lo_arg **argv,
                          int argc,
                          lo_message msg,
                          void *user_data)
    {
        return ((NSM::Client *)user_data)->command_broadcast(path, msg);
    }

    int
    Client::osc_save(const char *path,
                     const char *types,
                     lo_arg **argv,
                     int argc,
                     lo_message msg,
                     void *user_data)
    {
        char *out_msg = NULL;

        int r = ((NSM::Client *)user_data)->command_save(&out_msg);

        if(r)
            OSC_REPLY_ERR(r, (out_msg ? out_msg : ""));
        else
            OSC_REPLY("OK");

        if(out_msg)
            free(out_msg);

        return 0;
    }

    int
    Client::osc_open(const char *path,
                     const char *types,
                     lo_arg **argv,
                     int argc,
                     lo_message msg,
                     void *user_data)
    {
        char *out_msg = NULL;

        NSM::Client *nsm = (NSM::Client *)user_data;

        nsm->nsm_client_id = strdup(&argv[2]->s);

        int r = ((NSM::Client *)user_data)->command_open(&argv[0]->s,
                                                         &argv[1]->s,
                                                         &argv[2]->s,
                                                         &out_msg);

        if(r)
            OSC_REPLY_ERR(r, (out_msg ? out_msg : ""));
        else
            OSC_REPLY("OK");

        if(out_msg)
            free(out_msg);

        return 0;
    }

    int
    Client::osc_session_is_loaded(const char *path,
                                  const char *types,
                                  lo_arg **argv,
                                  int argc,
                                  lo_message msg,
                                  void *user_data)
    {
        NSM::Client *nsm = (NSM::Client *)user_data;

        nsm->command_session_is_loaded();

        return 0;
    }

    int
    Client::osc_error(const char *path,
                      const char *types,
                      lo_arg **argv,
                      int argc,
                      lo_message msg,
                      void *user_data)
    {
        if(strcmp(&argv[0]->s, "/nsm/server/announce"))
            return -1;

        NSM::Client *nsm = (NSM::Client *)user_data;


//        WARNING( "Failed to register with NSM: %s", &argv[2]->s );
        nsm->nsm_is_active = false;

        nsm->command_active(nsm->nsm_is_active);

        return 0;
    }

    int
    Client::osc_announce_reply(const char *path,
                               const char *types,
                               lo_arg **argv,
                               int argc,
                               lo_message msg,
                               void *user_data)
    {
        if(strcmp(&argv[0]->s, "/nsm/server/announce"))
            return -1;

        NSM::Client *nsm = (NSM::Client *)user_data;

//        MESSAGE( "Successfully registered. NSM says: %s", &argv[1]->s );
        nsm->nsm_is_active = true;
        nsm->_session_manager_name = strdup(&argv[2]->s);
        nsm->nsm_addr =
            lo_address_new_from_url(lo_address_get_url(lo_message_get_source(
                                                           msg)));

        nsm->command_active(nsm->nsm_is_active);

        return 0;
    }
};
