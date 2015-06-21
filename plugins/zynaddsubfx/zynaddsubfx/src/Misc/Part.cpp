/*
  ZynAddSubFX - a software synthesizer

  Part.cpp - Part implementation
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include "Part.h"
#include "Microtonal.h"
#include "Util.h"
#include "XMLwrapper.h"
#include "Allocator.h"
#include "../Effects/EffectMgr.h"
#include "../Params/ADnoteParameters.h"
#include "../Params/SUBnoteParameters.h"
#include "../Params/PADnoteParameters.h"
#include "../Synth/Resonance.h"
#include "../Synth/SynthNote.h"
#include "../Synth/ADnote.h"
#include "../Synth/SUBnote.h"
#include "../Synth/PADnote.h"
#include "../DSP/FFTwrapper.h"
#include "../Misc/Util.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>

using rtosc::Ports;
using rtosc::RtData;

#define rObject Part
static Ports partPorts = {
    rRecurs(kit, 16, "Kit"),//NUM_KIT_ITEMS
    rRecursp(partefx, 3, "Part Effect"),
    rRecur(ctl,       "Controller"),
    rToggle(Penabled, "Part enable"),
#undef rChangeCb
#define rChangeCb obj->setPvolume(obj->Pvolume);
    rParamZyn(Pvolume, "Part Volume"),
#undef rChangeCb
#define rChangeCb
    rParamZyn(Pminkey, "Min Used Key"),
    rParamZyn(Pmaxkey, "Max Used Key"),
    rParamZyn(Pkeyshift, "Part keyshift"),
    rParamZyn(Prcvchn,  "Active MIDI channel"),
    rParamZyn(Ppanning, "Set Panning"),
    rParamZyn(Pvelsns,   "Velocity sensing"),
    rParamZyn(Pveloffs,  "Velocity offset"),
    rToggle(Pnoteon,  "If the channel accepts note on events"),
    //TODO FIXME Change to 0=OFF 1=MULTI 2=SINGLE
    rParamI(Pkitmode, "Kit mode enable"),
    rToggle(Pdrummode, "Drum mode enable"),
    rToggle(Ppolymode,  "Polyphoney mode"),
    rToggle(Plegatomode, "Legato enable"),
    rParamZyn(Pkeylimit,   "Key limit per part"),
    rParamZyn(info.Ptype, "Class of Instrument"),
    rString(info.Pauthor, MAX_INFO_TEXT_SIZE, "Instrument Author"),
    rString(info.Pcomments, MAX_INFO_TEXT_SIZE, "Instrument Comments"),
    rString(Pname, PART_MAX_NAME_LEN, "Kit User Specified Label"),
    rArray(Pefxroute, NUM_PART_EFX,  "Effect Routing"),
    rArrayT(Pefxbypass, NUM_PART_EFX, "If an effect is bypassed"),
    {"captureMin:", NULL, NULL, [](const char *, RtData &r)
        {Part *p = (Part*)r.obj; p->Pminkey = p->lastnote;}},
    {"captureMax:", NULL, NULL, [](const char *, RtData &r)
        {Part *p = (Part*)r.obj; p->Pmaxkey = p->lastnote;}},
    {"polyType::c:i", NULL, NULL, [](const char *msg, RtData &d)
        {
            Part *p = (Part*)d.obj;
            if(!rtosc_narguments(msg)) {
                int res = 0;
                if(!p->Ppolymode)
                    res = p->Plegatomode ? 2 : 1;
                d.reply(d.loc, "c", res);
                return;
            }

            int i = rtosc_argument(msg, 0).i;
            if(i == 0) {
                p->Ppolymode = 1;
                p->Plegatomode = 0;
            } else if(i==1) {
                p->Ppolymode = 0;
                p->Plegatomode = 0;
            } else {
                p->Ppolymode = 0;
                p->Plegatomode = 1;
            }}},

    //{"kit#16::T:F", "::Enables or disables kit item", 0,
    //    [](const char *m, RtData &d) {
    //        auto loc = d.loc;
    //        Part *p = (Part*)d.obj;
    //        unsigned kitid = -1;
    //        //Note that this event will be captured before transmitted, thus
    //        //reply/broadcast don't matter
    //        for(int i=0; i<NUM_KIT_ITEMS; ++i) {
    //            d.reply("/middleware/oscil", "siisb", loc, kitid, i,
    //                    "oscil", sizeof(void*),
    //                    p->kit[kitid]->adpars->voice[i]->OscilSmp);
    //            d.reply("/middleware/oscil", "siisb", loc, kitid, i, "oscil-mod"
    //                    sizeof(void*),
    //                    p->kit[kitid]->adpars->voice[i]->somethingelse);
    //        }
    //        d.reply("/middleware/pad", "sib", loc, kitid,
    //                sizeof(PADnoteParameters*),
    //                p->kit[kitid]->padpars)
    //    }}
};

#undef  rObject
#define rObject Part::Kit
static Ports kitPorts = {
    rRecurp(padpars, "Padnote parameters"),
    rRecurp(adpars, "Adnote parameters"),
    rRecurp(subpars, "Adnote parameters"),
    rToggle(Penabled, "Kit item enable"),
    rToggle(Pmuted,   "Kit item mute"),
    rParamZyn(Pminkey,   "Kit item min key"),
    rParamZyn(Pmaxkey,   "Kit item max key"),
    rToggle(Padenabled, "ADsynth enable"),
    rToggle(Psubenabled, "SUBsynth enable"),
    rToggle(Ppadenabled, "PADsynth enable"),
    rParamZyn(Psendtoparteffect, "Effect Levels"),
    rString(Pname, PART_MAX_NAME_LEN, "Kit User Specified Label"),
    {"padpars-data:b", rProp(internal), 0, 
        [](const char *msg, RtData &d) {
            rObject &o = *(rObject*)d.obj;
            assert(o.padpars == NULL);
            o.padpars = *(decltype(o.padpars)*)rtosc_argument(msg, 0).b.data;
        }},
    {"adpars-data:b", rProp(internal), 0, 
        [](const char *msg, RtData &d) {
            rObject &o = *(rObject*)d.obj;
            assert(o.adpars == NULL);
            o.adpars = *(decltype(o.adpars)*)rtosc_argument(msg, 0).b.data;
        }},
    {"subpars-data:b", rProp(internal), 0, 
        [](const char *msg, RtData &d) {
            rObject &o = *(rObject*)d.obj;
            assert(o.subpars == NULL);
            o.subpars = *(decltype(o.subpars)*)rtosc_argument(msg, 0).b.data;
        }},

    //    [](
};

Ports &Part::Kit::ports = kitPorts;
Ports &Part::ports = partPorts;

Part::Part(Allocator &alloc, Microtonal *microtonal_, FFTwrapper *fft_)
    :memory(alloc)
{
    microtonal = microtonal_;
    fft      = fft_;
    partoutl = new float [synth->buffersize];
    partoutr = new float [synth->buffersize];

    monomemClear();

    for(int n = 0; n < NUM_KIT_ITEMS; ++n) {
        kit[n].Pname   = new char [PART_MAX_NAME_LEN];
        kit[n].adpars  = nullptr;
        kit[n].subpars = nullptr;
        kit[n].padpars = nullptr;
    }
        
    kit[0].adpars  = new ADnoteParameters(fft);

    //Part's Insertion Effects init
    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx) {
        partefx[nefx]    = new EffectMgr(memory, 1);
        Pefxbypass[nefx] = false;
    }

    for(int n = 0; n < NUM_PART_EFX + 1; ++n) {
        partfxinputl[n] = new float [synth->buffersize];
        partfxinputr[n] = new float [synth->buffersize];
    }

    killallnotes = false;
    oldfreq      = -1.0f;


    for(int i = 0; i < POLIPHONY; ++i) {
        partnote[i].status = KEY_OFF;
        partnote[i].note   = -1;
        partnote[i].itemsplaying = 0;
        for(int j = 0; j < NUM_KIT_ITEMS; ++j) {
            partnote[i].kititem[j].adnote  = NULL;
            partnote[i].kititem[j].subnote = NULL;
            partnote[i].kititem[j].padnote = NULL;
        }
        partnote[i].time = 0;
    }
    cleanup();

    Pname = new char[PART_MAX_NAME_LEN];

    oldvolumel = oldvolumer = 0.5f;
    lastnote   = -1;
    lastpos    = 0; // lastpos will store previously used NoteOn(...)'s pos.
    lastlegatomodevalid = false; // To store previous legatomodevalid value.

    defaults();
}
        
void Part::cloneTraits(Part &p) const
{
#define CLONE(x) p.x = this->x
    CLONE(Penabled);

    p.setPvolume(this->Pvolume);
    p.setPpanning(this->Ppanning);

    CLONE(Pminkey);
    CLONE(Pmaxkey);
    CLONE(Pkeyshift);
    CLONE(Prcvchn);

    CLONE(Pvelsns);
    CLONE(Pveloffs);

    CLONE(Pnoteon);
    CLONE(Ppolymode);
    CLONE(Plegatomode);
    CLONE(Pkeylimit);

    CLONE(ctl);
}

void Part::defaults()
{
    Penabled    = 0;
    Pminkey     = 0;
    Pmaxkey     = 127;
    Pnoteon     = 1;
    Ppolymode   = 1;
    Plegatomode = 0;
    setPvolume(96);
    Pkeyshift = 64;
    Prcvchn   = 0;
    setPpanning(64);
    Pvelsns   = 64;
    Pveloffs  = 64;
    Pkeylimit = 15;
    defaultsinstrument();
    ctl.defaults();
}

void Part::defaultsinstrument()
{
    ZERO(Pname, PART_MAX_NAME_LEN);

    info.Ptype = 0;
    ZERO(info.Pauthor, MAX_INFO_TEXT_SIZE + 1);
    ZERO(info.Pcomments, MAX_INFO_TEXT_SIZE + 1);

    Pkitmode  = 0;
    Pdrummode = 0;

    for(int n = 0; n < NUM_KIT_ITEMS; ++n) {
        kit[n].Penabled    = false;
        kit[n].Pmuted      = false;
        kit[n].Pminkey     = 0;
        kit[n].Pmaxkey     = 127;
        kit[n].Padenabled  = false;
        kit[n].Psubenabled = false;
        kit[n].Ppadenabled = false;
        ZERO(kit[n].Pname, PART_MAX_NAME_LEN);
        kit[n].Psendtoparteffect = 0;
        if(n != 0)
            setkititemstatus(n, 0);
    }
    kit[0].Penabled   = 1;
    kit[0].Padenabled = 1;
    kit[0].adpars->defaults();

    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx) {
        partefx[nefx]->defaults();
        Pefxroute[nefx] = 0; //route to next effect
    }
}



/*
 * Cleanup the part
 */
void Part::cleanup(bool final_)
{
    for(int k = 0; k < POLIPHONY; ++k)
        KillNotePos(k);
    for(int i = 0; i < synth->buffersize; ++i) {
        partoutl[i] = final_ ? 0.0f : denormalkillbuf[i];
        partoutr[i] = final_ ? 0.0f : denormalkillbuf[i];
    }
    ctl.resetall();
    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx)
        partefx[nefx]->cleanup();
    for(int n = 0; n < NUM_PART_EFX + 1; ++n)
        for(int i = 0; i < synth->buffersize; ++i) {
            partfxinputl[n][i] = final_ ? 0.0f : denormalkillbuf[i];
            partfxinputr[n][i] = final_ ? 0.0f : denormalkillbuf[i];
        }
}

Part::~Part()
{
    cleanup(true);
    for(int n = 0; n < NUM_KIT_ITEMS; ++n) {
        delete kit[n].adpars;
        delete kit[n].subpars;
        delete kit[n].padpars;
        delete [] kit[n].Pname;
    }

    delete [] Pname;
    delete [] partoutl;
    delete [] partoutr;
    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx)
        delete partefx[nefx];
    for(int n = 0; n < NUM_PART_EFX + 1; ++n) {
        delete [] partfxinputl[n];
        delete [] partfxinputr[n];
    }
}

/*
 * Note On Messages
 */
void Part::NoteOn(unsigned char note,
                  unsigned char velocity,
                  int masterkeyshift)
{
    // Legato and MonoMem used vars:
    int  posb = POLIPHONY - 1; // Just a dummy initial value.
    bool legatomodevalid = false; //true when legato mode is determined applicable.
    bool doinglegato     = false; // true when we determined we do a legato note.
    bool ismonofirstnote = false; /*(In Mono/Legato) true when we determined
                                    no other notes are held down or sustained.*/
    int lastnotecopy     = lastnote; //Useful after lastnote has been changed.

    if(!Pnoteon || !inRange(note, Pminkey, Pmaxkey))
        return;

    // MonoMem stuff:
    if(!Ppolymode) { // If Poly is off
        monomemPush(note); // Add note to the list.
        monomem[note].velocity  = velocity; // Store this note's velocity.
        monomem[note].mkeyshift = masterkeyshift; /* Store masterkeyshift too*/
        if((partnote[lastpos].status != KEY_PLAYING)
           && (partnote[lastpos].status != KEY_RELASED_AND_SUSTAINED))
            ismonofirstnote = true;  // No other keys are held or sustained.
    } else if(!monomemEmpty())
        monomemClear();

    lastnote = note;

    int pos = -1;
    for(int i = 0; i < POLIPHONY; ++i)
        if(partnote[i].status == KEY_OFF) {
            pos = i;
            break;
        }

    if(Plegatomode && !Pdrummode) {
        if(Ppolymode != 0) {
            fprintf(
                stderr,
                "ZynAddSubFX WARNING: Poly and Legato modes are both On, that should not happen ! ... Disabling Legato mode ! - (Part.cpp::NoteOn(..))\n");
            Plegatomode = 0;
        }
        else {
            // Legato mode is on and applicable.
            legatomodevalid = true;
            if((not ismonofirstnote) && (lastlegatomodevalid)) {
                // At least one other key is held or sustained, and the
                // previous note was played while in valid legato mode.
                doinglegato = true; // So we'll do a legato note.
                pos  = lastpos; // A legato note uses same pos as previous..
                posb = lastposb; // .. same goes for posb.
            }
            else {
                // Legato mode is valid, but this is only a first note.
                for(int i = 0; i < POLIPHONY; ++i)
                    if((partnote[i].status == KEY_PLAYING)
                       || (partnote[i].status == KEY_RELASED_AND_SUSTAINED))
                        RelaseNotePos(i);

                // Set posb
                posb = (pos + 1) % POLIPHONY; //We really want it (if the following fails)
                for(int i = 0; i < POLIPHONY; ++i)
                    if((partnote[i].status == KEY_OFF) && (pos != i)) {
                        posb = i;
                        break;
                    }
            }
            lastposb = posb; // Keep a trace of used posb
        }
    }
    else     // Legato mode is either off or non-applicable.
    if(!Ppolymode) {   //if the mode is 'mono' turn off all other notes
        for(int i = 0; i < POLIPHONY; ++i)
            if(partnote[i].status == KEY_PLAYING)
                RelaseNotePos(i);
        RelaseSustainedKeys();
    }
    lastlegatomodevalid = legatomodevalid;

    if(pos == -1)
        fprintf(stderr,
                "%s",
                "NOTES TOO MANY (> POLIPHONY) - (Part.cpp::NoteOn(..))\n");
    else {
        //start the note
        partnote[pos].status = KEY_PLAYING;
        partnote[pos].note   = note;
        if(legatomodevalid) {
            partnote[posb].status = KEY_PLAYING;
            partnote[posb].note   = note;
        }

        //this computes the velocity sensing of the part
        float vel = VelF(velocity / 127.0f, Pvelsns);

        //compute the velocity offset
        vel = limit(vel + (Pveloffs - 64.0f) / 64.0f, 0.0f, 1.0f);

        //compute the keyshift
        int partkeyshift = (int)Pkeyshift - 64;
        int keyshift     = masterkeyshift + partkeyshift;

        //initialise note frequency
        float notebasefreq;
        if(Pdrummode == 0) {
            notebasefreq = microtonal->getnotefreq(note, keyshift);
            if(notebasefreq < 0.0f)
                return;//the key is no mapped
        }
        else
            notebasefreq = 440.0f * powf(2.0f, (note - 69.0f) / 12.0f);

        //Portamento
        if(oldfreq < 1.0f)
            oldfreq = notebasefreq;//this is only the first note is played

        // For Mono/Legato: Force Portamento Off on first
        // notes. That means it is required that the previous note is
        // still held down or sustained for the Portamento to activate
        // (that's like Legato).
        bool portamento = false;
        if(Ppolymode || !ismonofirstnote)
            // I added a third argument to the
            // ctl.initportamento(...) function to be able
            // to tell it if we're doing a legato note.
            portamento = ctl.initportamento(oldfreq, notebasefreq, doinglegato);

        if(portamento)
            ctl.portamento.noteusing = pos;
        oldfreq = notebasefreq;

        lastpos = pos; // Keep a trace of used pos.

        if(doinglegato) {
            // Do Legato note
            if(Pkitmode == 0) { // "normal mode" legato note

                auto note1 = partnote[pos].kititem[0];
                auto note2 = partnote[posb].kititem[0];
                LegatoParams pars = {notebasefreq, vel, portamento, note, true};
                if(kit[0].Padenabled && note1.adnote && note2.adnote) {
                    note1.adnote->legatonote(pars);
                    note2.adnote->legatonote(pars);
                }

                if(kit[0].Psubenabled && note1.subnote && note2.subnote) {
                    note1.subnote->legatonote(pars);
                    note2.subnote->legatonote(pars);
                }

                if(kit[0].Ppadenabled && note1.padnote && note2.padnote) {
                    note1.padnote->legatonote(pars);
                    note2.padnote->legatonote(pars);
                }
            }
            else {   // "kit mode" legato note
                int ci = 0;
                for(int item = 0; item < NUM_KIT_ITEMS; ++item) {

                    //Make sure the key is valid and not across multiple ranges
                    if(kit[item].Pmuted || !inRange(note, kit[item].Pminkey, kit[item].Pmaxkey)
                                        || !inRange((unsigned char)lastnotecopy, kit[item].Pminkey, kit[item].Pmaxkey))
                        continue;

                    auto note1 = partnote[pos].kititem[ci];
                    auto note2 = partnote[posb].kititem[ci];
                    LegatoParams pars = {notebasefreq, vel, portamento, note, true};
                    note1.sendtoparteffect = limit((int)kit[item].Psendtoparteffect, 0, NUM_PART_EFX);
                    note2.sendtoparteffect = limit((int)kit[item].Psendtoparteffect, 0, NUM_PART_EFX);

                    if(kit[item].Padenabled && kit[item].adpars && note1.adnote && note2.adnote) {
                        note1.adnote->legatonote(pars);
                        note2.adnote->legatonote(pars);
                    }
                    if(kit[item].Psubenabled && kit[item].subpars && note1.subnote && note2.subnote) {
                        note1.subnote->legatonote(pars);
                        note2.subnote->legatonote(pars);
                    }
                    if(kit[item].Ppadenabled && kit[item].padpars && note1.padnote && note2.padnote) {
                        note1.padnote->legatonote(pars);
                        note2.padnote->legatonote(pars);
                    }

                    if(kit[item].adpars || kit[item].subpars || kit[item].padpars) {
                        ci++;
                        if((kit[item].Padenabled || kit[item].Psubenabled || kit[item].Ppadenabled) && (Pkitmode == 2))
                            break;
                    }
                }
                if(ci == 0) {
                    // No legato were performed at all, so pretend nothing happened:
                    monomemPop(monomemBack()); // Remove last note from the list.
                    lastnote = lastnotecopy; // Set lastnote back to previous value.
                }
            }
            return; // Ok, Legato note done, return.
        }

        partnote[pos].itemsplaying = 0;
        if(legatomodevalid)
            partnote[posb].itemsplaying = 0;

        if(Pkitmode == 0) { //init the notes for the "normal mode"
            partnote[pos].kititem[0].sendtoparteffect = 0;
            SynthParams pars{memory, ctl, notebasefreq, vel, (bool) portamento, note, false};

            if(kit[0].Padenabled)
                partnote[pos].kititem[0].adnote =
                    memory.alloc<ADnote>(kit[0].adpars, pars);
            if(kit[0].Psubenabled)
                partnote[pos].kititem[0].subnote =
                    memory.alloc<SUBnote>(kit[0].subpars, pars);
            if(kit[0].Ppadenabled)
                partnote[pos].kititem[0].padnote =
                    memory.alloc<PADnote>(kit[0].padpars, pars);


            if(kit[0].Padenabled || kit[0].Psubenabled || kit[0].Ppadenabled)
                partnote[pos].itemsplaying++;

            // Spawn another note (but silent) if legatomodevalid==true
            if(legatomodevalid) {
                partnote[posb].kititem[0].sendtoparteffect = 0;
                pars.quiet = true;

                if(kit[0].Padenabled)
                    partnote[posb].kititem[0].adnote =
                        memory.alloc<ADnote>(kit[0].adpars, pars);
                if(kit[0].Psubenabled)
                    partnote[posb].kititem[0].subnote =
                        memory.alloc<SUBnote>(kit[0].subpars, pars);
                if(kit[0].Ppadenabled)
                    partnote[posb].kititem[0].padnote =
                        memory.alloc<PADnote>(kit[0].padpars, pars);

                if(kit[0].Padenabled || kit[0].Psubenabled || kit[0].Ppadenabled)
                    partnote[posb].itemsplaying++;
            }
        }
        else    //init the notes for the "kit mode"
            for(int item = 0; item < NUM_KIT_ITEMS; ++item) {
                if(kit[item].Pmuted || !inRange(note, kit[item].Pminkey, kit[item].Pmaxkey))
                    continue;

                int ci = partnote[pos].itemsplaying; //ci=current item
                auto &note1 = partnote[pos].kititem[ci];

                //if this parameter is 127 for "unprocessed"
                note1.sendtoparteffect = limit((int)kit[item].Psendtoparteffect, 0, NUM_PART_EFX);

                SynthParams pars{memory, ctl, notebasefreq, vel, (bool) portamento, note, false};

                if(kit[item].adpars && kit[item].Padenabled)
                    note1.adnote =
                        memory.alloc<ADnote>(kit[item].adpars, pars);

                if(kit[item].subpars && kit[item].Psubenabled)
                    note1.subnote =
                        memory.alloc<SUBnote>(kit[item].subpars, pars);

                if(kit[item].padpars && kit[item].Ppadenabled)
                    note1.padnote =
                        memory.alloc<PADnote>(kit[item].padpars, pars);

                // Spawn another note (but silent) if legatomodevalid==true
                if(legatomodevalid) {
                    auto &note2 = partnote[pos].kititem[ci];
                    note2.sendtoparteffect = limit((int)kit[item].Psendtoparteffect, 0, NUM_PART_EFX);

                    pars.quiet = true;
                    if(kit[item].adpars && kit[item].Padenabled)
                        note2.adnote =
                            memory.alloc<ADnote>(kit[item].adpars, pars);
                    if(kit[item].subpars && kit[item].Psubenabled)
                        note2.subnote =
                            memory.alloc<SUBnote>(kit[item].subpars, pars);
                    if(kit[item].padpars && kit[item].Ppadenabled)
                        note2.padnote = 
                            memory.alloc<PADnote>(kit[item].padpars, pars);

                    if(kit[item].adpars || kit[item].subpars || kit[item].padpars)
                        partnote[posb].itemsplaying++;
                }

                if(kit[item].adpars || kit[item].subpars) {
                    partnote[pos].itemsplaying++;
                    if((kit[item].Padenabled || kit[item].Psubenabled || kit[item].Ppadenabled) && (Pkitmode == 2))
                        break;
                }
            }
    }

    //this only relase the keys if there is maximum number of keys allowed
    setkeylimit(Pkeylimit);
}

/*
 * Note Off Messages
 */
void Part::NoteOff(unsigned char note) //relase the key
{
    // This note is released, so we remove it from the list.
    if(!monomemEmpty())
        monomemPop(note);

    for(int i = POLIPHONY - 1; i >= 0; i--) //first note in, is first out if there are same note multiple times
        if((partnote[i].status == KEY_PLAYING) && (partnote[i].note == note)) {
            if(!ctl.sustain.sustain) { //the sustain pedal is not pushed
                if(!Ppolymode && !monomemEmpty())
                    MonoMemRenote();//Play most recent still active note
                else
                    RelaseNotePos(i);
            }
            else    //the sustain pedal is pushed
                partnote[i].status = KEY_RELASED_AND_SUSTAINED;
        }
}

void Part::PolyphonicAftertouch(unsigned char note,
                                unsigned char velocity,
                                int masterkeyshift)
{
    (void) masterkeyshift;

    if(!Pnoteon || !inRange(note, Pminkey, Pmaxkey) || Pdrummode)
        return;

    // MonoMem stuff:
    if(!Ppolymode)   // if Poly is off
        monomem[note].velocity = velocity;       // Store this note's velocity.


    for(int i = 0; i < POLIPHONY; ++i)
        if((partnote[i].note == note) && (partnote[i].status == KEY_PLAYING)) {
            /* update velocity */
            // compute the velocity offset
            float vel = VelF(velocity / 127.0f, Pvelsns) + (Pveloffs - 64.0f) / 64.0f;
            vel = limit(vel, 0.0f, 1.0f);

            if(!Pkitmode) { // "normal mode"
                if(kit[0].Padenabled && partnote[i].kititem[0].adnote)
                    partnote[i].kititem[0].adnote->setVelocity(vel);
                if(kit[0].Psubenabled && partnote[i].kititem[0].subnote)
                    partnote[i].kititem[0].subnote->setVelocity(vel);
                if(kit[0].Ppadenabled && partnote[i].kititem[0].padnote)
                    partnote[i].kititem[0].padnote->setVelocity(vel);
            }
            else     // "kit mode"
                for(int item = 0; item < NUM_KIT_ITEMS; ++item) {
                    if(kit[item].Pmuted || !inRange(note, kit[item].Pminkey, kit[item].Pmaxkey))
                        continue;

                    if(kit[item].Padenabled && partnote[i].kititem[item].adnote)
                        partnote[i].kititem[item].adnote->setVelocity(vel);
                    if(kit[item].Psubenabled && partnote[i].kititem[item].subnote)
                        partnote[i].kititem[item].subnote->setVelocity(vel);
                    if(kit[item].Ppadenabled && partnote[i].kititem[item].padnote)
                        partnote[i].kititem[item].padnote->setVelocity(vel);
                }
        }

}

/*
 * Controllers
 */
void Part::SetController(unsigned int type, int par)
{
    switch(type) {
        case C_pitchwheel:
            ctl.setpitchwheel(par);
            break;
        case C_expression:
            ctl.setexpression(par);
            setPvolume(Pvolume); //update the volume
            break;
        case C_portamento:
            ctl.setportamento(par);
            break;
        case C_panning:
            ctl.setpanning(par);
            setPpanning(Ppanning); //update the panning
            break;
        case C_filtercutoff:
            ctl.setfiltercutoff(par);
            break;
        case C_filterq:
            ctl.setfilterq(par);
            break;
        case C_bandwidth:
            ctl.setbandwidth(par);
            break;
        case C_modwheel:
            ctl.setmodwheel(par);
            break;
        case C_fmamp:
            ctl.setfmamp(par);
            break;
        case C_volume:
            ctl.setvolume(par);
            if(ctl.volume.receive != 0)
                volume = ctl.volume.volume;
            else
                setPvolume(Pvolume);
            break;
        case C_sustain:
            ctl.setsustain(par);
            if(ctl.sustain.sustain == 0)
                RelaseSustainedKeys();
            break;
        case C_allsoundsoff:
            AllNotesOff(); //Panic
            break;
        case C_resetallcontrollers:
            ctl.resetall();
            RelaseSustainedKeys();
            if(ctl.volume.receive != 0)
                volume = ctl.volume.volume;
            else
                setPvolume(Pvolume);
            setPvolume(Pvolume); //update the volume
            setPpanning(Ppanning); //update the panning

            for(int item = 0; item < NUM_KIT_ITEMS; ++item) {
                if(kit[item].adpars == NULL)
                    continue;
                kit[item].adpars->GlobalPar.Reson->
                sendcontroller(C_resonance_center, 1.0f);

                kit[item].adpars->GlobalPar.Reson->
                sendcontroller(C_resonance_bandwidth, 1.0f);
            }
            //more update to add here if I add controllers
            break;
        case C_allnotesoff:
            RelaseAllKeys();
            break;
        case C_resonance_center:
            ctl.setresonancecenter(par);
            for(int item = 0; item < NUM_KIT_ITEMS; ++item) {
                if(kit[item].adpars == NULL)
                    continue;
                kit[item].adpars->GlobalPar.Reson->
                sendcontroller(C_resonance_center,
                               ctl.resonancecenter.relcenter);
            }
            break;
        case C_resonance_bandwidth:
            ctl.setresonancebw(par);
            kit[0].adpars->GlobalPar.Reson->
            sendcontroller(C_resonance_bandwidth, ctl.resonancebandwidth.relbw);
            break;
    }
}
/*
 * Relase the sustained keys
 */

void Part::RelaseSustainedKeys()
{
    // Let's call MonoMemRenote() on some conditions:
    if(Ppolymode == 0 && !monomemEmpty())
        if(monomemBack() != lastnote) // Sustain controller manipulation would cause repeated same note respawn without this check.
            MonoMemRenote();  // To play most recent still held note.

    for(int i = 0; i < POLIPHONY; ++i)
        if(partnote[i].status == KEY_RELASED_AND_SUSTAINED)
            RelaseNotePos(i);
}

/*
 * Relase all keys
 */

void Part::RelaseAllKeys()
{
    for(int i = 0; i < POLIPHONY; ++i)
        if((partnote[i].status != KEY_RELASED)
           && (partnote[i].status != KEY_OFF)) //thanks to Frank Neumann
            RelaseNotePos(i);
}

// Call NoteOn(...) with the most recent still held key as new note
// (Made for Mono/Legato).
void Part::MonoMemRenote()
{
    unsigned char mmrtempnote = monomemBack(); // Last list element.
    monomemPop(mmrtempnote); // We remove it, will be added again in NoteOn(...).
    if(Pnoteon == 0)
        RelaseNotePos(lastpos);
    else
        NoteOn(mmrtempnote, monomem[mmrtempnote].velocity,
               monomem[mmrtempnote].mkeyshift);
}

/*
 * Release note at position
 */
void Part::RelaseNotePos(int pos)
{
    for(int j = 0; j < NUM_KIT_ITEMS; ++j) {
        if(partnote[pos].kititem[j].adnote)
            partnote[pos].kititem[j].adnote->relasekey();

        if(partnote[pos].kititem[j].subnote)
            partnote[pos].kititem[j].subnote->relasekey();

        if(partnote[pos].kititem[j].padnote)
            partnote[pos].kititem[j].padnote->relasekey();
    }
    partnote[pos].status = KEY_RELASED;
}


/*
 * Kill note at position
 */
void Part::KillNotePos(int pos)
{
    partnote[pos].status = KEY_OFF;
    partnote[pos].note   = -1;
    partnote[pos].time   = 0;
    partnote[pos].itemsplaying = 0;

    for(int j = 0; j < NUM_KIT_ITEMS; ++j) {
        memory.dealloc(partnote[pos].kititem[j].adnote);
        memory.dealloc(partnote[pos].kititem[j].subnote);
        memory.dealloc(partnote[pos].kititem[j].padnote);
    }
    if(pos == ctl.portamento.noteusing) {
        ctl.portamento.noteusing = -1;
        ctl.portamento.used      = 0;
    }
}


/*
 * Set Part's key limit
 */
void Part::setkeylimit(unsigned char Pkeylimit)
{
    this->Pkeylimit = Pkeylimit;
    int keylimit = Pkeylimit;
    if(keylimit == 0)
        keylimit = POLIPHONY - 5;

    //release old keys if the number of notes>keylimit
    if(Ppolymode != 0) {
        int notecount = 0;
        for(int i = 0; i < POLIPHONY; ++i)
            if((partnote[i].status == KEY_PLAYING) || (partnote[i].status == KEY_RELASED_AND_SUSTAINED))
                notecount++;

        int oldestnotepos = -1;
        if(notecount > keylimit)   //find out the oldest note
            for(int i = 0; i < POLIPHONY; ++i) {
                int maxtime = 0;
                if(((partnote[i].status == KEY_PLAYING) || (partnote[i].status == KEY_RELASED_AND_SUSTAINED)) && (partnote[i].time > maxtime)) {
                    maxtime = partnote[i].time;
                    oldestnotepos = i;
                }
            }
        if(oldestnotepos != -1)
            RelaseNotePos(oldestnotepos);
    }
}


/*
 * Prepare all notes to be turned off
 */
void Part::AllNotesOff()
{
    killallnotes = true;
}

void Part::RunNote(unsigned int k)
{
    unsigned noteplay = 0;
    for(int item = 0; item < partnote[k].itemsplaying; ++item) {
        int sendcurrenttofx = partnote[k].kititem[item].sendtoparteffect;

        for(unsigned type = 0; type < 3; ++type) {
            //Select a note
            SynthNote **note = NULL;
            if(type == 0)
                note = &partnote[k].kititem[item].adnote;
            else if(type == 1)
                note = &partnote[k].kititem[item].subnote;
            else if(type == 2)
                note = &partnote[k].kititem[item].padnote;

            //Process if it exists
            if(!(*note))
                continue;
            noteplay++;

            float tmpoutr[synth->buffersize];
            float tmpoutl[synth->buffersize];
            (*note)->noteout(&tmpoutl[0], &tmpoutr[0]);

            if((*note)->finished())
                memory.dealloc(*note);
            for(int i = 0; i < synth->buffersize; ++i) { //add the note to part(mix)
                partfxinputl[sendcurrenttofx][i] += tmpoutl[i];
                partfxinputr[sendcurrenttofx][i] += tmpoutr[i];
            }
        }
    }

    //Kill note if there is no synth on that note
    if(!noteplay)
        KillNotePos(k);
}

/*
 * Compute Part samples and store them in the partoutl[] and partoutr[]
 */
void Part::ComputePartSmps()
{
    for(unsigned nefx = 0; nefx < NUM_PART_EFX + 1; ++nefx)
        for(int i = 0; i < synth->buffersize; ++i) {
            partfxinputl[nefx][i] = 0.0f;
            partfxinputr[nefx][i] = 0.0f;
        }

    for(unsigned k = 0; k < POLIPHONY; ++k) {
        if(partnote[k].status == KEY_OFF)
            continue;
        partnote[k].time++;
        //get the sampledata of the note and kill it if it's finished
        RunNote(k);
    }


    //Apply part's effects and mix them
    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx) {
        if(!Pefxbypass[nefx]) {
            partefx[nefx]->out(partfxinputl[nefx], partfxinputr[nefx]);
            if(Pefxroute[nefx] == 2)
                for(int i = 0; i < synth->buffersize; ++i) {
                    partfxinputl[nefx + 1][i] += partefx[nefx]->efxoutl[i];
                    partfxinputr[nefx + 1][i] += partefx[nefx]->efxoutr[i];
                }
        }
        int routeto = ((Pefxroute[nefx] == 0) ? nefx + 1 : NUM_PART_EFX);
        for(int i = 0; i < synth->buffersize; ++i) {
            partfxinputl[routeto][i] += partfxinputl[nefx][i];
            partfxinputr[routeto][i] += partfxinputr[nefx][i];
        }
    }
    for(int i = 0; i < synth->buffersize; ++i) {
        partoutl[i] = partfxinputl[NUM_PART_EFX][i];
        partoutr[i] = partfxinputr[NUM_PART_EFX][i];
    }

    if(killallnotes) {
        for(int i = 0; i < synth->buffersize; ++i) {
            float tmp = (synth->buffersize_f - i) / synth->buffersize_f;
            partoutl[i] *= tmp;
            partoutr[i] *= tmp;
        }
        for(int k = 0; k < POLIPHONY; ++k)
            KillNotePos(k);
        killallnotes = false;
        for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx)
            partefx[nefx]->cleanup();
    }
    ctl.updateportamento();
}

/*
 * Parameter control
 */
void Part::setPvolume(char Pvolume_)
{
    Pvolume = Pvolume_;
    volume  =
        dB2rap((Pvolume - 96.0f) / 96.0f * 40.0f) * ctl.expression.relvolume;
}

void Part::setPpanning(char Ppanning_)
{
    Ppanning = Ppanning_;
    panning  = limit(Ppanning / 127.0f + ctl.panning.pan, 0.0f, 1.0f);
}

/*
 * Enable or disable a kit item
 */
void Part::setkititemstatus(unsigned kititem, bool Penabled_)
{
    //nonexistent kit item and the first kit item is always enabled
    if((kititem == 0) || (kititem >= NUM_KIT_ITEMS))
        return;

    Kit &kkit = kit[kititem];

    //no need to update if
    if(kkit.Penabled == Penabled_)
        return;
    kkit.Penabled = Penabled_;

    if(!Penabled_) {
        delete kkit.adpars;
        delete kkit.subpars;
        delete kkit.padpars;
        kkit.Pname[0] = '\0';

        //Reset notes s.t. stale buffers will not get read
        for(int k = 0; k < POLIPHONY; ++k)
            KillNotePos(k);
    }
    else {
        //All parameters must be NULL in this case
        assert(!(kkit.adpars || kkit.subpars || kkit.padpars));
        kkit.adpars  = new ADnoteParameters(fft);
        kkit.subpars = new SUBnoteParameters();
        kkit.padpars = new PADnoteParameters(fft);
    }
}

void Part::add2XMLinstrument(XMLwrapper *xml)
{
    xml->beginbranch("INFO");
    xml->addparstr("name", (char *)Pname);
    xml->addparstr("author", (char *)info.Pauthor);
    xml->addparstr("comments", (char *)info.Pcomments);
    xml->addpar("type", info.Ptype);
    xml->endbranch();


    xml->beginbranch("INSTRUMENT_KIT");
    xml->addpar("kit_mode", Pkitmode);
    xml->addparbool("drum_mode", Pdrummode);

    for(int i = 0; i < NUM_KIT_ITEMS; ++i) {
        xml->beginbranch("INSTRUMENT_KIT_ITEM", i);
        xml->addparbool("enabled", kit[i].Penabled);
        if(kit[i].Penabled != 0) {
            xml->addparstr("name", (char *)kit[i].Pname);

            xml->addparbool("muted", kit[i].Pmuted);
            xml->addpar("min_key", kit[i].Pminkey);
            xml->addpar("max_key", kit[i].Pmaxkey);

            xml->addpar("send_to_instrument_effect", kit[i].Psendtoparteffect);

            xml->addparbool("add_enabled", kit[i].Padenabled);
            if(kit[i].Padenabled && kit[i].adpars) {
                xml->beginbranch("ADD_SYNTH_PARAMETERS");
                kit[i].adpars->add2XML(xml);
                xml->endbranch();
            }

            xml->addparbool("sub_enabled", kit[i].Psubenabled);
            if(kit[i].Psubenabled && kit[i].subpars) {
                xml->beginbranch("SUB_SYNTH_PARAMETERS");
                kit[i].subpars->add2XML(xml);
                xml->endbranch();
            }

            xml->addparbool("pad_enabled", kit[i].Ppadenabled);
            if(kit[i].Ppadenabled && kit[i].padpars) {
                xml->beginbranch("PAD_SYNTH_PARAMETERS");
                kit[i].padpars->add2XML(xml);
                xml->endbranch();
            }
        }
        xml->endbranch();
    }
    xml->endbranch();

    xml->beginbranch("INSTRUMENT_EFFECTS");
    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx) {
        xml->beginbranch("INSTRUMENT_EFFECT", nefx);
        xml->beginbranch("EFFECT");
        partefx[nefx]->add2XML(xml);
        xml->endbranch();

        xml->addpar("route", Pefxroute[nefx]);
        partefx[nefx]->setdryonly(Pefxroute[nefx] == 2);
        xml->addparbool("bypass", Pefxbypass[nefx]);
        xml->endbranch();
    }
    xml->endbranch();
}

void Part::add2XML(XMLwrapper *xml)
{
    //parameters
    xml->addparbool("enabled", Penabled);
    if((Penabled == 0) && (xml->minimal))
        return;

    xml->addpar("volume", Pvolume);
    xml->addpar("panning", Ppanning);

    xml->addpar("min_key", Pminkey);
    xml->addpar("max_key", Pmaxkey);
    xml->addpar("key_shift", Pkeyshift);
    xml->addpar("rcv_chn", Prcvchn);

    xml->addpar("velocity_sensing", Pvelsns);
    xml->addpar("velocity_offset", Pveloffs);

    xml->addparbool("note_on", Pnoteon);
    xml->addparbool("poly_mode", Ppolymode);
    xml->addpar("legato_mode", Plegatomode);
    xml->addpar("key_limit", Pkeylimit);

    xml->beginbranch("INSTRUMENT");
    add2XMLinstrument(xml);
    xml->endbranch();

    xml->beginbranch("CONTROLLER");
    ctl.add2XML(xml);
    xml->endbranch();
}

int Part::saveXML(const char *filename)
{
    XMLwrapper xml;

    xml.beginbranch("INSTRUMENT");
    add2XMLinstrument(&xml);
    xml.endbranch();

    int result = xml.saveXMLfile(filename);
    return result;
}

int Part::loadXMLinstrument(const char *filename)
{
    XMLwrapper xml;
    if(xml.loadXMLfile(filename) < 0) {
        return -1;
    }

    if(xml.enterbranch("INSTRUMENT") == 0)
        return -10;
    getfromXMLinstrument(&xml);
    xml.exitbranch();

    return 0;
}

void Part::applyparameters(void)
{
    applyparameters([]{return false;});
}

void Part::applyparameters(std::function<bool()> do_abort)
{
    for(int n = 0; n < NUM_KIT_ITEMS; ++n)
        if(kit[n].Ppadenabled && kit[n].padpars)
            kit[n].padpars->applyparameters(do_abort);
}

void Part::initialize_rt(void)
{
    for(int i=0; i<NUM_PART_EFX; ++i)
        partefx[i]->init();
}

void Part::kill_rt(void)
{
    for(int i=0; i<NUM_PART_EFX; ++i)
        partefx[i]->kill();
    for(int k = 0; k < POLIPHONY; ++k)
        KillNotePos(k);
}

void Part::monomemPush(char note)
{
    for(int i=0; i<256; ++i)
        if(monomemnotes[i]==note)
            return;

    for(int i=254;i>=0; --i)
        monomemnotes[i+1] = monomemnotes[i];
    monomemnotes[0] = note;
}

void Part::monomemPop(char note)
{
    int note_pos=-1;
    for(int i=0; i<256; ++i)
        if(monomemnotes[i]==note)
            note_pos = i;
    if(note_pos != -1) {
        for(int i=note_pos; i<256; ++i)
            monomemnotes[i] = monomemnotes[i+1];
        monomemnotes[255] = -1;
    }
}

char Part::monomemBack(void) const
{
    return monomemnotes[0];
}

bool Part::monomemEmpty(void) const
{
    return monomemnotes[0] == -1;
}

void Part::monomemClear(void)
{
    for(int i=0; i<256; ++i)
        monomemnotes[i] = -1;
}

void Part::getfromXMLinstrument(XMLwrapper *xml)
{
    if(xml->enterbranch("INFO")) {
        xml->getparstr("name", (char *)Pname, PART_MAX_NAME_LEN);
        xml->getparstr("author", (char *)info.Pauthor, MAX_INFO_TEXT_SIZE);
        xml->getparstr("comments", (char *)info.Pcomments, MAX_INFO_TEXT_SIZE);
        info.Ptype = xml->getpar("type", info.Ptype, 0, 16);

        xml->exitbranch();
    }

    if(xml->enterbranch("INSTRUMENT_KIT")) {
        Pkitmode  = xml->getpar127("kit_mode", Pkitmode);
        Pdrummode = xml->getparbool("drum_mode", Pdrummode);

        setkititemstatus(0, 0);
        for(int i = 0; i < NUM_KIT_ITEMS; ++i) {
            if(xml->enterbranch("INSTRUMENT_KIT_ITEM", i) == 0)
                continue;
            setkititemstatus(i, xml->getparbool("enabled", kit[i].Penabled));
            if(kit[i].Penabled == 0) {
                xml->exitbranch();
                continue;
            }

            xml->getparstr("name", (char *)kit[i].Pname, PART_MAX_NAME_LEN);

            kit[i].Pmuted  = xml->getparbool("muted", kit[i].Pmuted);
            kit[i].Pminkey = xml->getpar127("min_key", kit[i].Pminkey);
            kit[i].Pmaxkey = xml->getpar127("max_key", kit[i].Pmaxkey);

            kit[i].Psendtoparteffect = xml->getpar127(
                "send_to_instrument_effect",
                kit[i].Psendtoparteffect);

            kit[i].Padenabled = xml->getparbool("add_enabled",
                                                kit[i].Padenabled);
            if(xml->enterbranch("ADD_SYNTH_PARAMETERS")) {
                if(!kit[i].adpars)
                    kit[i].adpars = new ADnoteParameters(fft);
                kit[i].adpars->getfromXML(xml);
                xml->exitbranch();
            }

            kit[i].Psubenabled = xml->getparbool("sub_enabled",
                                                 kit[i].Psubenabled);
            if(xml->enterbranch("SUB_SYNTH_PARAMETERS")) {
                if(!kit[i].subpars)
                    kit[i].subpars = new SUBnoteParameters();
                kit[i].subpars->getfromXML(xml);
                xml->exitbranch();
            }

            kit[i].Ppadenabled = xml->getparbool("pad_enabled",
                                                 kit[i].Ppadenabled);
            if(xml->enterbranch("PAD_SYNTH_PARAMETERS")) {
                if(!kit[i].padpars)
                    kit[i].padpars = new PADnoteParameters(fft);
                kit[i].padpars->getfromXML(xml);
                xml->exitbranch();
            }

            xml->exitbranch();
        }

        xml->exitbranch();
    }


    if(xml->enterbranch("INSTRUMENT_EFFECTS")) {
        for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx) {
            if(xml->enterbranch("INSTRUMENT_EFFECT", nefx) == 0)
                continue;
            if(xml->enterbranch("EFFECT")) {
                partefx[nefx]->getfromXML(xml);
                xml->exitbranch();
            }

            Pefxroute[nefx] = xml->getpar("route",
                                          Pefxroute[nefx],
                                          0,
                                          NUM_PART_EFX);
            partefx[nefx]->setdryonly(Pefxroute[nefx] == 2);
            Pefxbypass[nefx] = xml->getparbool("bypass", Pefxbypass[nefx]);
            xml->exitbranch();
        }
        xml->exitbranch();
    }
}

void Part::getfromXML(XMLwrapper *xml)
{
    Penabled = xml->getparbool("enabled", Penabled);

    setPvolume(xml->getpar127("volume", Pvolume));
    setPpanning(xml->getpar127("panning", Ppanning));

    Pminkey   = xml->getpar127("min_key", Pminkey);
    Pmaxkey   = xml->getpar127("max_key", Pmaxkey);
    Pkeyshift = xml->getpar127("key_shift", Pkeyshift);
    Prcvchn   = xml->getpar127("rcv_chn", Prcvchn);

    Pvelsns  = xml->getpar127("velocity_sensing", Pvelsns);
    Pveloffs = xml->getpar127("velocity_offset", Pveloffs);

    Pnoteon     = xml->getparbool("note_on", Pnoteon);
    Ppolymode   = xml->getparbool("poly_mode", Ppolymode);
    Plegatomode = xml->getparbool("legato_mode", Plegatomode); //older versions
    if(!Plegatomode)
        Plegatomode = xml->getpar127("legato_mode", Plegatomode);
    Pkeylimit = xml->getpar127("key_limit", Pkeylimit);


    if(xml->enterbranch("INSTRUMENT")) {
        getfromXMLinstrument(xml);
        xml->exitbranch();
    }

    if(xml->enterbranch("CONTROLLER")) {
        ctl.getfromXML(xml);
        xml->exitbranch();
    }
}
