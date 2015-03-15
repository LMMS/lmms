#include "BankView.h"
#include "../Misc/Util.h"
#include <FL/Fl.H>
#include <FL/Fl_Check_Button.H>
#include <FL/fl_ask.H>
#include <rtosc/rtosc.h>
#include <cstdio>
#include <cstring>
#include <cassert>

BankList::BankList(int x,int y, int w, int h, const char *label)
    :Fl_Osc_Choice(x,y,w,h,label)
{}

void BankList::init(std::string path)
{
    ext = path;
    oscRegister(path.c_str());
    osc->createLink("/bank-list", this);
}

void BankList::OSC_raw(const char *msg)
{
    if(!strcmp(msg, "/bank-list")) {

        const int   pos  = rtosc_argument(msg, 0).i;
        const char *path = rtosc_argument(msg, 1).s;

        value(0);
        if(pos == 0)
            this->clear();

        this->add(path);
        osc->write("/loadbank");
    }
    if(!strcmp(msg, "/loadbank")) {
        value(rtosc_argument(msg, 0).i);
    }
}

BankSlot::BankSlot(int x,int y, int w, int h, const char *label)
:Fl_Button(x,y,w,h,label), nslot(-1)
{
    memset(labelstr, 0, sizeof(labelstr));
    box(FL_THIN_UP_BOX);
    labelfont(0);
    labelsize(13);
    align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_CLIP);
}

int BankSlot::handle(int event)
{
    int what = 0;
    if (Fl::event_inside(this))
    {
        what=0;
        if ((event==FL_RELEASE)&&(Fl::event_button()==1))
            what=1;
        if ((event==FL_RELEASE)&&(Fl::event_button()==3))
            what=2;
    }

    int tmp=Fl_Button::handle(event);

    if (what && Fl::event_inside(this))
        bv->react(what, nslot);

    return tmp;
}

void BankSlot::init(int nslot_, BankView *bv_)
{
    nslot = nslot_;
    bv    = bv_;

    snprintf(labelstr, 127, "%d.", nslot_);
    label(labelstr);
}

void BankSlot::update(const char *name__, const char *fname__)
{
    name_     = name__;
    filename_ = fname__;
    snprintf(labelstr, 127, "%d. %s", nslot, name_.c_str());
    label(labelstr);
    
    if(name_.empty())
        label("");

    color(empty() ? 46 : 51);
#ifdef NTK_GUI
    redraw();
#endif
}

bool BankSlot::empty(void) const
{
    return filename_.empty();
}

const char *BankSlot::name(void) const
{
    return name_.c_str();
}

const char *BankSlot::filename(void) const
{
    return filename_.c_str();
}

/*
   void BankSlot::init(int nslot_, int *what_, int *whatslot_,void (BankProcess_:: *fnc_)(void),BankProcess_ *bp_,Bank *bank_,int *nselected_) {
   nslot=nslot_;
   what=what_;
   whatslot=whatslot_;
   fnc=fnc_;
   bp=bp_;
//bank=bank_;
nselected=nselected_;
box(FL_THIN_UP_BOX);
labelfont(0);
labelsize(13);
align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_CLIP);

highlight=0;
//refresh();
}
*/

/*
   void BankSlot::refresh() {
   if (bank->emptyslot(nslot))
   color(46);
   else if (bank->isPADsynth_used(nslot))
   color(26);
   else
   color(51);


   if (*nselected==nslot)
   color(6);


   copy_label(bank->getnamenumbered(nslot).c_str());
   }
   */
static int modeCb(const char *label)
{
    if(!strcmp("Read", label))
        return 1;
    else if(!strcmp("Write", label))
        return 2;
    else if(!strcmp("Clear", label))
        return 3;
    else if(!strcmp("Swap", label))
        return 4;
    return -1;
}

static void modeButtonCb(Fl_Widget *w, void *v)
{
    BankViewControls *bvc = (BankViewControls*)v;
    bvc->mode(modeCb(w->label()));
}

BankViewControls::BankViewControls(int x, int y, int w, int h, const char *label)
    :Fl_Group(x,y,w,h,label)
{
    //Margin
    const int m = 10;
    //Width per elm
    const float W = w/4;

    read  = new Fl_Check_Button(x+m+0*W, y+m, W-2*m, h-2*m, "Read");
    write = new Fl_Check_Button(x+m+1*W, y+m, W-2*m, h-2*m, "Write");
    clear = new Fl_Check_Button(x+m+2*W, y+m, W-2*m, h-2*m, "Clear");
    swap  = new Fl_Check_Button(x+m+3*W, y+m, W-2*m, h-2*m, "Swap");
    read->box(FL_BORDER_BOX);
    write->box(FL_BORDER_BOX);
    clear->box(FL_BORDER_BOX);
    swap->box(FL_BORDER_BOX);
    read->callback(modeButtonCb, this);
    write->callback(modeButtonCb, this);
    clear->callback(modeButtonCb, this);
    swap->callback(modeButtonCb, this);
    mode(1);
}

int BankViewControls::mode(void) const
{
    return mode_;
}

void BankViewControls::mode(int m)
{
    mode_ = m;
    int M = m-1;
    assert(0 <= M && M <= 3);
    Fl_Button *buttons[4]{read, write, clear, swap};

    for(int i=0; i<4; ++i)
        buttons[i]->value(i==M);
}


BankView::BankView(int x,int y, int w, int h, const char *label)
    :Fl_Group(x,y,w,h,label), bvc(NULL), slots{0}, osc(0),
    loc(""), nselected(-1), npart(0), cbwig_(0)
{}


BankView::~BankView(void)
{
    if(osc)
        osc->removeLink("/bankview", this);
}

void BankView::init(Fl_Osc_Interface *osc_, BankViewControls *bvc_, int *npart_)
{
    assert(osc_);

    osc = osc_;
    bvc = bvc_;
    npart = npart_;

    osc->createLink("/bankview", this);

    //Element Size
    const float width  = w()/5.0;
    const float height = h()/32.0;

    //Offsets
    const int X = x();
    const int Y = y();

    begin();
    //Place All Slots
    for(int i=0; i<5; ++i)
        for(int j=0; j<32; ++j)
            slots[i*32 + j] =
                new BankSlot(X + i*width, Y + j*height, width, height);

    end();

    //Initialize callbacks
    for(int i=0; i<160; ++i)
        slots[i]->init(i, this);

    //Request Values
    for(int i=0; i<160; ++i)
        osc->write("/refresh_bank", "i", i);
}

/*
 * React to user input.
 * This consists of the events:
 * - Rename Slot (right click)
 * - Read From Slot
 * - Write To Slot
 * - Swap Slot First Selection
 * - Swap Slot Second Selction
 *
 *   TODO restore autoclose functionality
 */
void BankView::react(int event, int nslot)
{
    BankSlot &slot = *slots[nslot];
    const bool isempty = slot.empty();
    const int  mode    = bvc->mode();

    //Rename slot
    if (event==2 && !isempty && mode!=4) {
        if(const char *name=fl_input("Slot (instrument) name:", slot.name())) {
            osc->write("/bank-rename", "is", nslot, name);
            osc->write("/refresh_bank", "i", nslot);
        }
    }

    //Reads from slot
    if ((event==1)&&(mode==1)&&(!slot.empty())){
        printf("Loading a part #%d with file '%s'\n", nslot, slot.filename());
        osc->write("/load-part", "is", *npart, slot.filename());
        osc->writeValue("/part"+to_s(*npart)+"/name", slot.name());
        if(cbwig_)
            cbwig_->do_callback();
    }

    //save(write) to slot
    if(event==1 && mode==2){
        if(!isempty && !fl_choice("Overwrite the slot no. %d ?","No","Yes",NULL,nslot+1))
            return;

        osc->write("/save-bank-part", "ii", *npart, nslot);
        osc->write("/refresh_bank", "i", nslot);
        //pthread_mutex_lock(&master->part[*npart]->load_mutex);
        //bank->savetoslot(slot,master->part[*npart]);
        //pthread_mutex_unlock(&master->part[*npart]->load_mutex);

        bvc->mode(1);
    }


    //Clears the slot
    if(event==1 && mode==3 && !isempty) {
        if (fl_choice("Clear the slot no. %d ?","No","Yes",NULL, nslot+1)) {
            osc->write("/clear-bank-slot", "i", nslot);
            osc->write("/refresh_bank", "i", nslot);
        }
    }

    //Swap
    if(mode==4) {
        if(event==1 && nselected>=0){
            osc->write("/swap-bank-slots", "ii", nselected, nslot);
            osc->write("/refresh_bank", "i", nslot);
            osc->write("/refresh_bank", "i", nselected);
            //bank->swapslot(nselected,slot);
            nselected=-1;
        } else if(nselected<0 || event==2) {
            nselected=nslot;
        };
    };
}

void BankView::OSC_raw(const char *msg)
{
    if(strcmp(rtosc_argument_string(msg), "iss"))
        return;

    int nslot         = rtosc_argument(msg,0).i;
    const char *name  = rtosc_argument(msg,1).s;
    const char *fname = rtosc_argument(msg,2).s;

    if(0 <= nslot && nslot < 160)
        slots[nslot]->update(name, fname);
}
        
void BankView::cbwig(Fl_Widget *w)
{
    cbwig_ = w;
}

void BankView::refresh(void)
{
    assert(osc);
    //Odd case during initialization
    if(!osc)
        return;

    for(int i=0; i<160; ++i)
        osc->write("/refresh_bank", "i", i);
}

