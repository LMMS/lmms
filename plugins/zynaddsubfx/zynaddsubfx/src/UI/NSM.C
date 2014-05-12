
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


#include "NSM.H"

#include "../Nio/Nio.h"

#include "MasterUI.h"
#include <FL/Fl.H>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern int Pexitprogram;
extern MasterUI *ui;

extern NSM_Client *nsm;
extern char       *instance_name;

NSM_Client::NSM_Client()
{
    project_filename = 0;
    display_name     = 0;
}

int command_open(const char *name,
                 const char *display_name,
                 const char *client_id,
                 char **out_msg);
int command_save(char **out_msg);

int
NSM_Client::command_save(char **out_msg)
{
    (void) out_msg;
    int r = ERR_OK;

    ui->do_save_master(project_filename);

    return r;
}

int
NSM_Client::command_open(const char *name,
                         const char *display_name,
                         const char *client_id,
                         char **out_msg)
{
    Nio::stop();

    if(instance_name)
        free(instance_name);

    instance_name = strdup(client_id);

    Nio::start();

    char *new_filename;

    asprintf(&new_filename, "%s.xmz", name);

    struct stat st;

    int r = ERR_OK;

    if(0 == stat(new_filename, &st)) {
        if(ui->do_load_master_unconditional(new_filename, display_name) < 0) {
            *out_msg = strdup("Failed to load for unknown reason");
            r = ERR_GENERAL;

            return r;
        }
    }
    else
        ui->do_new_master_unconditional();

    if(project_filename)
        free(project_filename);

    if(this->display_name)
        free(this->display_name);

    project_filename = new_filename;

    this->display_name = strdup(display_name);

    return r;
}

static void save_callback(Fl_Widget *, void *v)
{
    MasterUI *ui = static_cast<MasterUI*>(v);
    ui->do_save_master();
}

void
NSM_Client::command_active(bool active)
{
    if(active) {
        Fl_Menu_Item *m;
        //TODO see if there is a cleaner way of doing this without voiding
        //constness
        if((m=const_cast<Fl_Menu_Item *>(ui->mastermenu->find_item(
                                       "&File/&Open Parameters..."))))
            m->label("&Import Parameters...");
        if((m=const_cast<Fl_Menu_Item *>(ui->simplemastermenu->find_item(
                                       "&File/&Open Parameters..."))))
            m->label("&Import Parameters...");

        //TODO get this menu entry inserted at the right point
        if((m=const_cast<Fl_Menu_Item *>(ui->mastermenu->find_item("&File/&Export Parameters..."))))
            m->show();
        else
            ui->mastermenu->add("&File/&Export Parameters...",0,save_callback,ui);

        if((m=const_cast<Fl_Menu_Item *>(ui->simplemastermenu->find_item("&File/&Export Parameters..."))))
            m->show();
        else
            ui->simplemastermenu->add("&File/&Export Parameters...",0,save_callback,ui);

        ui->sm_indicator1->value(1);
        ui->sm_indicator2->value(1);
        ui->sm_indicator1->tooltip(session_manager_name());
        ui->sm_indicator2->tooltip(session_manager_name());
    }
    else {
        Fl_Menu_Item *m;
        if((m=const_cast<Fl_Menu_Item *>(ui->mastermenu->find_item(
                                       "&File/&Import Parameters..."))))
            m->label("&Open Parameters...");
        if((m=const_cast<Fl_Menu_Item *>(ui->simplemastermenu->find_item(
                                       "&File/&Open Parameters..."))))
            m->label("&Open Parameters...");

        if((m=const_cast<Fl_Menu_Item *>(ui->mastermenu->find_item("&File/&Export Parameters..."))))
            m->hide();
        if((m=const_cast<Fl_Menu_Item *>(ui->simplemastermenu->find_item("&File/&Export Parameters..."))))
            m->hide();

        ui->sm_indicator1->value(0);
        ui->sm_indicator2->value(0);
        ui->sm_indicator1->tooltip(NULL);
        ui->sm_indicator2->tooltip(NULL);
    }
}
