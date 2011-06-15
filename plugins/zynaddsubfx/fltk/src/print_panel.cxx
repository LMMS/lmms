//
// "$Id: print_panel.cxx 7913 2010-11-29 18:18:27Z greg.ercolano $"
//
// Print panel for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

//
// This file is "work in progress".  The main parts have been copied
// from fluid's print_panel{.fl|.h|.cxx} and hand-edited to produce
// a working version w/o global variables.  The intention is to move
// all static variables into an own class, and to name this class
// Fl_Printer_Chooser or similar...
//
// Todo:
//
//   -	Currently preferences can't be saved, and there are options that
//	are not yet used for printing.
//   -	This file can only be used as an include file in Fl_PS_Printer.cxx
//   -	The use of static variables should be avoided.
//   -	Probably much more ...
//

#include "print_panel.h"
#include <stdio.h>
#include <stdlib.h>
#include "../src/flstring.h"
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Int_Input.H>

static Fl_Preferences print_prefs(Fl_Preferences::USER, "fltk.org", "printers");
static Fl_Double_Window *print_panel=(Fl_Double_Window *)0;
static Fl_Group *print_panel_controls=(Fl_Group *)0;
static Fl_Choice *print_choice=(Fl_Choice *)0;
static Fl_Button *print_properties=(Fl_Button *)0;
static Fl_Box *print_status=(Fl_Box *)0;
static Fl_Round_Button *print_all=(Fl_Round_Button *)0;
static Fl_Round_Button *print_pages=(Fl_Round_Button *)0;
static Fl_Round_Button *print_selection=(Fl_Round_Button *)0;
static Fl_Check_Button *print_collate_button=(Fl_Check_Button *)0;
static Fl_Group *print_collate_group[2]={(Fl_Group *)0};
static Fl_Progress *print_progress=(Fl_Progress *)0;
static Fl_Double_Window *print_properties_panel=(Fl_Double_Window *)0;
static Fl_Choice *print_page_size=(Fl_Choice *)0;
static Fl_Int_Input *print_from=(Fl_Int_Input *)0;
static Fl_Int_Input *print_to=(Fl_Int_Input *)0;
static Fl_Spinner *print_copies=(Fl_Spinner *)0;

static int print_start = 0;	// 1 if print_okay has been clicked

static void cb_print_choice(Fl_Choice*, void*) {
  print_update_status();
}

static void cb_print_properties(Fl_Button*, void*) {
  print_properties_panel->show();
}

static void cb_print_all(Fl_Round_Button*, void*) {
  print_from->deactivate();
  print_to->deactivate();
}

static void cb_print_pages(Fl_Round_Button*, void*) {
  print_from->activate();
  print_to->activate();
}

static void cb_print_selection(Fl_Round_Button*, void*) {
  print_from->deactivate();
  print_to->deactivate();
}

static void cb_print_copies(Fl_Spinner*, void*) {
  if (print_copies->value() == 1) {
    print_collate_button->deactivate();
    print_collate_group[0]->deactivate();
    print_collate_group[1]->deactivate();
  } else {
/*    print_collate_button->activate(); // TODO: manage collate options
    print_collate_group[0]->activate();
    print_collate_group[1]->activate(); */
  };
}

static void cb_print_collate_button(Fl_Check_Button*, void*) {
  int i = print_collate_button->value() != 0;
  print_collate_group[i]->show();
  print_collate_group[1 - i]->hide();
}

static void cb_Cancel(Fl_Button*, void*) {
  print_start = 0;
  print_panel->hide();
}

static void cb_print_properties_panel(Fl_Double_Window*, void*) {
  print_properties_panel->hide();
  print_update_status();
}

static Fl_Menu_Item menu_print_page_size[] = {
 {"Letter", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {"A4", 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

#include <FL/Fl_Pixmap.H>
static const char *idata_print_color[] = {
"24 24 17 1",
" \tc None",
".\tc #FFFF00",
"+\tc #C8FF00",
"@\tc #00FF00",
"#\tc #FFC800",
"$\tc #FF0000",
"%\tc #00FFFF",
"&\tc #000000",
"*\tc #FF00FF",
"=\tc #00FFC8",
"-\tc #FF00C8",
";\tc #00C800",
">\tc #C80000",
",\tc #0000C8",
"\'\tc #0000FF",
")\tc #00C8FF",
"!\tc #C800FF",
"         ......         ",
"       ..........       ",
"      ............      ",
"     ..............     ",
"     ..............     ",
"    ................    ",
"    ................    ",
"    ................    ",
"    +@@@@@@+#$$$$$$#    ",
"   %@@@@@@@&&$$$$$$$*   ",
"  %%@@@@@@&&&&$$$$$$**  ",
" %%%=@@@@&&&&&&$$$$-*** ",
" %%%%@@@;&&&&&&>$$$**** ",
"%%%%%%@@&&&&&&&&$$******",
"%%%%%%%@&&&&&&&&$*******",
"%%%%%%%%,&&&&&&,********",
"%%%%%%%%\'\'\'\'\'\'\'\'********",
"%%%%%%%%\'\'\'\'\'\'\'\'********",
"%%%%%%%%\'\'\'\'\'\'\'\'********",
" %%%%%%%)\'\'\'\'\'\'!******* ",
" %%%%%%%%\'\'\'\'\'\'******** ",
"  %%%%%%%%\'\'\'\'********  ",
"   %%%%%%%%\'\'********   ",
"     %%%%%%  ******     "
};
static Fl_Pixmap image_print_color(idata_print_color);

static const char *idata_print_gray[] = {
"24 24 17 1",
" \tc None",
".\tc #E3E3E3",
"+\tc #D2D2D2",
"@\tc #969696",
"#\tc #C2C2C2",
"$\tc #4C4C4C",
"%\tc #B2B2B2",
"&\tc #000000",
"*\tc #696969",
"=\tc #ACACAC",
"-\tc #626262",
";\tc #767676",
">\tc #3C3C3C",
",\tc #161616",
"\'\tc #1C1C1C",
")\tc #929292",
"!\tc #585858",
"         ......         ",
"       ..........       ",
"      ............      ",
"     ..............     ",
"     ..............     ",
"    ................    ",
"    ................    ",
"    ................    ",
"    +@@@@@@+#$$$$$$#    ",
"   %@@@@@@@&&$$$$$$$*   ",
"  %%@@@@@@&&&&$$$$$$**  ",
" %%%=@@@@&&&&&&$$$$-*** ",
" %%%%@@@;&&&&&&>$$$**** ",
"%%%%%%@@&&&&&&&&$$******",
"%%%%%%%@&&&&&&&&$*******",
"%%%%%%%%,&&&&&&,********",
"%%%%%%%%\'\'\'\'\'\'\'\'********",
"%%%%%%%%\'\'\'\'\'\'\'\'********",
"%%%%%%%%\'\'\'\'\'\'\'\'********",
" %%%%%%%)\'\'\'\'\'\'!******* ",
" %%%%%%%%\'\'\'\'\'\'******** ",
"  %%%%%%%%\'\'\'\'********  ",
"   %%%%%%%%\'\'********   ",
"     %%%%%%  ******     "
};
static Fl_Pixmap image_print_gray(idata_print_gray);

static Fl_Button *print_output_mode[4]={(Fl_Button *)0};

static void cb_Save(Fl_Return_Button*, void*) {
  print_properties_panel->hide();

  char name[1024];
  int val;
  const char *printer = (const char *)print_choice->menu()[print_choice->value()].user_data();

  snprintf(name, sizeof(name), "%s/page_size", printer);
  print_prefs.set(name, print_page_size->value());

  snprintf(name, sizeof(name), "%s/output_mode", printer);
  for (val = 0; val < 4; val ++) {
    if (print_output_mode[val]->value()) break;
  }
  print_prefs.set(name, val);
}

static void cb_Cancel1(Fl_Button*, void*) {
  print_properties_panel->hide();
  print_update_status();
}

static void cb_Use(Fl_Button*, void*) {
  print_properties_panel->hide();
}

Fl_Double_Window* make_print_panel() {
  { print_panel = new Fl_Double_Window(465, 235, Fl_Printer::dialog_title);
    { print_panel_controls = new Fl_Group(10, 10, 447, 216);
      { print_choice = new Fl_Choice(133, 10, 181, 25, Fl_Printer::dialog_printer);
        print_choice->down_box(FL_BORDER_BOX);
        print_choice->labelfont(1);
        print_choice->callback((Fl_Callback*)cb_print_choice);
        print_choice->when(FL_WHEN_CHANGED);
      } // Fl_Choice* print_choice
      { print_properties = new Fl_Button(314, 10, 115, 25, Fl_Printer::dialog_properties);
        print_properties->callback((Fl_Callback*)cb_print_properties);
      } // Fl_Button* print_properties
      { print_status = new Fl_Box(0, 41, print_panel_controls->w(), 17, "printer/job status");
        print_status->align(Fl_Align(FL_ALIGN_CLIP|FL_ALIGN_INSIDE|FL_ALIGN_LEFT));
      } // Fl_Box* print_status
      { Fl_Group* o = new Fl_Group(10, 86, 227, 105, Fl_Printer::dialog_range);
        o->box(FL_THIN_DOWN_BOX);
        o->labelfont(1);
        o->align(Fl_Align(FL_ALIGN_TOP_LEFT));
        { print_all = new Fl_Round_Button(20, 96, 38, 25, Fl_Printer::dialog_all);
          print_all->type(102);
          print_all->down_box(FL_ROUND_DOWN_BOX);
          print_all->value(1);
          print_all->callback((Fl_Callback*)cb_print_all);
        } // Fl_Round_Button* print_all
        { print_pages = new Fl_Round_Button(20, 126, 64, 25, Fl_Printer::dialog_pages);
          print_pages->type(102);
          print_pages->down_box(FL_ROUND_DOWN_BOX);
          print_pages->callback((Fl_Callback*)cb_print_pages);
        } // Fl_Round_Button* print_pages
        { print_selection = new Fl_Round_Button(20, 156, 82, 25, "Selection");
          print_selection->type(102);
          print_selection->down_box(FL_ROUND_DOWN_BOX);
          print_selection->callback((Fl_Callback*)cb_print_selection);
        } // Fl_Round_Button* print_selection
        { print_from = new Fl_Int_Input(136, 126, 28, 25, Fl_Printer::dialog_from);
          print_from->type(2);
          print_from->textfont(4);
          print_from->deactivate();
        } // Fl_Int_Input* print_from
        { print_to = new Fl_Int_Input(199, 126, 28, 25, Fl_Printer::dialog_to);
          print_to->type(2);
          print_to->textfont(4);
          print_to->deactivate();
        } // Fl_Int_Input* print_to
        o->end();
      } // Fl_Group* o
      { Fl_Group* o = new Fl_Group(247, 86, 210, 105, Fl_Printer::dialog_copies);
        o->box(FL_THIN_DOWN_BOX);
        o->labelfont(1);
        o->align(Fl_Align(FL_ALIGN_TOP_LEFT));
        { print_copies = new Fl_Spinner(321, 96, 45, 25, Fl_Printer::dialog_copyNo);
          print_copies->callback((Fl_Callback*)cb_print_copies);
          print_copies->when(FL_WHEN_CHANGED);
        } // Fl_Spinner* print_copies
        { print_collate_button = new Fl_Check_Button(376, 96, 64, 25, "Collate");
          print_collate_button->down_box(FL_DOWN_BOX);
          print_collate_button->callback((Fl_Callback*)cb_print_collate_button);
          print_collate_button->when(FL_WHEN_CHANGED);
          print_collate_button->deactivate();
        } // Fl_Check_Button* print_collate_button
        { print_collate_group[0] = new Fl_Group(257, 131, 191, 50);
          print_collate_group[0]->deactivate();
          { Fl_Box* o = new Fl_Box(287, 141, 30, 40, "1");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
            o->deactivate();
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(272, 136, 30, 40, "1");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
            o->deactivate();
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(257, 131, 30, 40, "1");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
            o->deactivate();
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(352, 141, 30, 40, "2");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
            o->deactivate();
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(337, 136, 30, 40, "2");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
            o->deactivate();
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(322, 131, 30, 40, "2");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
            o->deactivate();
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(417, 141, 30, 40, "3");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
            o->deactivate();
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(402, 136, 30, 40, "3");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
            o->deactivate();
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(387, 131, 30, 40, "3");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
            o->deactivate();
          } // Fl_Box* o
          print_collate_group[0]->end();
        } // Fl_Group* print_collate_group[0]
        { print_collate_group[1] = new Fl_Group(257, 131, 191, 50);
          print_collate_group[1]->hide();
          print_collate_group[1]->deactivate();
          { Fl_Box* o = new Fl_Box(287, 141, 30, 40, "3");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(272, 136, 30, 40, "2");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(257, 131, 30, 40, "1");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(352, 141, 30, 40, "3");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(337, 136, 30, 40, "2");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(322, 131, 30, 40, "1");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(417, 141, 30, 40, "3");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(402, 136, 30, 40, "2");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
          } // Fl_Box* o
          { Fl_Box* o = new Fl_Box(387, 131, 30, 40, "1");
            o->box(FL_BORDER_BOX);
            o->color(FL_BACKGROUND2_COLOR);
            o->labelsize(11);
            o->align(Fl_Align(FL_ALIGN_BOTTOM_RIGHT|FL_ALIGN_INSIDE));
          } // Fl_Box* o
          print_collate_group[1]->end();
        } // Fl_Group* print_collate_group[1]
        o->end();
      } // Fl_Group* o
      { Fl_Return_Button* o = new Fl_Return_Button(279, 201, 100, 25, Fl_Printer::dialog_print_button);
        o->callback((Fl_Callback*)print_cb);
      } // Fl_Return_Button* o
      { Fl_Button* o = new Fl_Button(389, 201, 68, 25, Fl_Printer::dialog_cancel_button);
        o->callback((Fl_Callback*)cb_Cancel);
      } // Fl_Button* o
      print_panel_controls->end();
    } // Fl_Group* print_panel_controls
    { print_progress = new Fl_Progress(10, 203, 289, 21);
      print_progress->selection_color((Fl_Color)4);
      print_progress->hide();
    } // Fl_Progress* print_progress
    print_panel->set_modal();
    print_panel->end();
  } // Fl_Double_Window* print_panel
  { print_properties_panel = new Fl_Double_Window(290, 130, Fl_Printer::property_title);
    print_properties_panel->callback((Fl_Callback*)cb_print_properties_panel);
    { print_page_size = new Fl_Choice(150, 10, 80, 25, Fl_Printer::property_pagesize);
      print_page_size->down_box(FL_BORDER_BOX);
      print_page_size->labelfont(FL_HELVETICA);
      print_page_size->menu(menu_print_page_size);
    } // Fl_Choice* print_page_size
    { Fl_Group* o = new Fl_Group(110, 45, 170, 40, Fl_Printer::property_mode);
      o->labelfont(FL_HELVETICA);
      o->align(Fl_Align(FL_ALIGN_LEFT));
      { print_output_mode[0] = new Fl_Button(110, 45, 30, 40);
        print_output_mode[0]->type(102);
        print_output_mode[0]->box(FL_BORDER_BOX);
        print_output_mode[0]->down_box(FL_BORDER_BOX);
        print_output_mode[0]->value(1);
        print_output_mode[0]->color(FL_BACKGROUND2_COLOR);
        print_output_mode[0]->selection_color(FL_FOREGROUND_COLOR);
        print_output_mode[0]->image(image_print_color);
      } // Fl_Button* print_output_mode[0]
      { print_output_mode[1] = new Fl_Button(150, 50, 40, 30);
        print_output_mode[1]->type(102);
        print_output_mode[1]->box(FL_BORDER_BOX);
        print_output_mode[1]->down_box(FL_BORDER_BOX);
        print_output_mode[1]->color(FL_BACKGROUND2_COLOR);
        print_output_mode[1]->selection_color(FL_FOREGROUND_COLOR);
        print_output_mode[1]->image(image_print_color);
      } // Fl_Button* print_output_mode[1]
      { print_output_mode[2] = new Fl_Button(200, 45, 30, 40);
        print_output_mode[2]->type(102);
        print_output_mode[2]->box(FL_BORDER_BOX);
        print_output_mode[2]->down_box(FL_BORDER_BOX);
        print_output_mode[2]->color(FL_BACKGROUND2_COLOR);
        print_output_mode[2]->selection_color(FL_FOREGROUND_COLOR);
        print_output_mode[2]->image(image_print_gray);
      } // Fl_Button* print_output_mode[2]
      { print_output_mode[3] = new Fl_Button(240, 50, 40, 30);
        print_output_mode[3]->type(102);
        print_output_mode[3]->box(FL_BORDER_BOX);
        print_output_mode[3]->down_box(FL_BORDER_BOX);
        print_output_mode[3]->color(FL_BACKGROUND2_COLOR);
        print_output_mode[3]->selection_color(FL_FOREGROUND_COLOR);
        print_output_mode[3]->image(image_print_gray);
      } // Fl_Button* print_output_mode[3]
      o->end();
    } // Fl_Group* o
    { Fl_Return_Button* o = new Fl_Return_Button(93, 95, 99, 25, Fl_Printer::property_save);
      o->callback((Fl_Callback*)cb_Save);
    } // Fl_Return_Button* o
    { Fl_Button* o = new Fl_Button(202, 95, 78, 25, Fl_Printer::property_cancel);
      o->callback((Fl_Callback*)cb_Cancel1);
    } // Fl_Button* o
    { Fl_Button* o = new Fl_Button(10, 95, 73, 25, Fl_Printer::property_use);
      o->callback((Fl_Callback*)cb_Use);
    } // Fl_Button* o
    print_properties_panel->set_modal();
    print_properties_panel->end();
  } // Fl_Double_Window* print_properties_panel
  return print_properties_panel;
}

void print_cb(Fl_Return_Button *, void *) {
  print_start = 1;
  print_panel->hide();
}

void print_load() {
  FILE *lpstat;
  char line[1024], name[1024], *nptr, qname[2048], *qptr, defname[1024];
  int i;

  if (print_choice->size() > 1) {
    for (i = 1; print_choice->text(i); i ++) {
      free(print_choice->menu()[i].user_data());
    }
  }

  print_choice->clear();
  print_choice->add(Fl_Printer::dialog_print_to_file, 0, 0, 0, FL_MENU_DIVIDER);
  print_choice->value(0);
  
  print_start = 0;

  defname[0] = '\0';

  if ((lpstat = popen("LC_MESSAGES=C LANG=C lpstat -p -d", "r")) != NULL) {
    while (fgets(line, sizeof(line), lpstat)) {
      if (!strncmp(line, "printer ", 8) &&
          sscanf(line + 8, "%s", name) == 1) {
        for (nptr = name, qptr = qname; *nptr; *qptr++ = *nptr++) {
          if (*nptr == '/') *qptr++ = '\\';
        }
        *qptr = '\0';

        print_choice->add(qname, 0, 0, (void *)strdup(name), 0);
      } else if (!strncmp(line, "system default destination: ", 28)) {
        if (sscanf(line + 28, "%s", defname) != 1) defname[0] = '\0';
      }
    }
    pclose(lpstat);
  }

  if (defname[0]) {
    for (i = 1; print_choice->text(i); i ++) {
      if (!strcmp((char *)print_choice->menu()[i].user_data(), defname)) {
        print_choice->value(i);
        break;
      }
    }
  } else if (print_choice->size() > 2) print_choice->value(1);

  print_update_status();

} // print_load()

void print_update_status() {
  FILE *lpstat;
  char command[1024];
  static char status[1024];
  const char *printer = (const char *)print_choice->menu()[print_choice->value()].user_data();

  if (print_choice->value()) {
    snprintf(command, sizeof(command), "lpstat -p '%s'", printer);
    if ((lpstat = popen(command, "r")) != NULL) {
      if (fgets(status, sizeof(status), lpstat)==0) { /* ignore */ }
      pclose(lpstat);
    } else strcpy(status, "printer status unavailable");
  } else status[0] = '\0';

  print_status->label(status);

  char name[1024];
  int val;

  snprintf(name, sizeof(name), "%s/page_size", printer);
  print_prefs.get(name, val, 1);
  print_page_size->value(val);

  snprintf(name, sizeof(name), "%s/output_mode", printer);
  print_prefs.get(name, val, 0);
  print_output_mode[val]->setonly();
}

//
// End of "$Id: print_panel.cxx 7913 2010-11-29 18:18:27Z greg.ercolano $".
//
