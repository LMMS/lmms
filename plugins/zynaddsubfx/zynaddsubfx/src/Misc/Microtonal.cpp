/*
  ZynAddSubFX - a software synthesizer

  Microtonal.cpp - Tuning settings and microtonal capabilities
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

#include <math.h>
#include <string.h>
#include "Microtonal.h"

#define MAX_LINE_SIZE 80

Microtonal::Microtonal()
{
    Pname    = new unsigned char[MICROTONAL_MAX_NAME_LEN];
    Pcomment = new unsigned char[MICROTONAL_MAX_NAME_LEN];
    defaults();
}

void Microtonal::defaults()
{
    Pinvertupdown = 0;
    Pinvertupdowncenter = 60;
    octavesize  = 12;
    Penabled    = 0;
    PAnote      = 69;
    PAfreq      = 440.0f;
    Pscaleshift = 64;

    Pfirstkey       = 0;
    Plastkey        = 127;
    Pmiddlenote     = 60;
    Pmapsize        = 12;
    Pmappingenabled = 0;

    for(int i = 0; i < 128; ++i)
        Pmapping[i] = i;

    for(int i = 0; i < MAX_OCTAVE_SIZE; ++i) {
        octave[i].tuning = tmpoctave[i].tuning = powf(
                               2,
                               (i % octavesize
                                + 1) / 12.0f);
        octave[i].type = tmpoctave[i].type = 1;
        octave[i].x1   = tmpoctave[i].x1 = (i % octavesize + 1) * 100;
        octave[i].x2   = tmpoctave[i].x2 = 0;
    }
    octave[11].type = 2;
    octave[11].x1   = 2;
    octave[11].x2   = 1;
    for(int i = 0; i < MICROTONAL_MAX_NAME_LEN; ++i) {
        Pname[i]    = '\0';
        Pcomment[i] = '\0';
    }
    snprintf((char *) Pname, MICROTONAL_MAX_NAME_LEN, "12tET");
    snprintf((char *) Pcomment,
             MICROTONAL_MAX_NAME_LEN,
             "Equal Temperament 12 notes per octave");
    Pglobalfinedetune = 64;
}

Microtonal::~Microtonal()
{
    delete [] Pname;
    delete [] Pcomment;
}

/*
 * Get the size of the octave
 */
unsigned char Microtonal::getoctavesize() const
{
    if(Penabled != 0)
        return octavesize;
    else
        return 12;
}

/*
 * Get the frequency according the note number
 */
float Microtonal::getnotefreq(int note, int keyshift) const
{
    // in this function will appears many times things like this:
    // var=(a+b*100)%b
    // I had written this way because if I use var=a%b gives unwanted results when a<0
    // This is the same with divisions.

    if((Pinvertupdown != 0) && ((Pmappingenabled == 0) || (Penabled == 0)))
        note = (int) Pinvertupdowncenter * 2 - note;

    //compute global fine detune
    float globalfinedetunerap = powf(2.0f,
                                     (Pglobalfinedetune - 64.0f) / 1200.0f);       //-64.0f .. 63.0f cents

    if(Penabled == 0)
        return powf(2.0f,
                    (note - PAnote
                     + keyshift) / 12.0f) * PAfreq * globalfinedetunerap;                     //12tET

    int scaleshift =
        ((int)Pscaleshift - 64 + (int) octavesize * 100) % octavesize;

    //compute the keyshift
    float rap_keyshift = 1.0f;
    if(keyshift != 0) {
        int kskey = (keyshift + (int)octavesize * 100) % octavesize;
        int ksoct = (keyshift + (int)octavesize * 100) / octavesize - 100;
        rap_keyshift  = (kskey == 0) ? (1.0f) : (octave[kskey - 1].tuning);
        rap_keyshift *= powf(octave[octavesize - 1].tuning, ksoct);
    }

    //if the mapping is enabled
    if(Pmappingenabled != 0) {
        if((note < Pfirstkey) || (note > Plastkey))
            return -1.0f;
        //Compute how many mapped keys are from middle note to reference note
        //and find out the proportion between the freq. of middle note and "A" note
        int tmp = PAnote - Pmiddlenote, minus = 0;
        if(tmp < 0) {
            tmp   = -tmp;
            minus = 1;
        }
        int deltanote = 0;
        for(int i = 0; i < tmp; ++i)
            if(Pmapping[i % Pmapsize] >= 0)
                deltanote++;
        float rap_anote_middlenote =
            (deltanote ==
             0) ? (1.0f) : (octave[(deltanote - 1) % octavesize].tuning);
        if(deltanote != 0)
            rap_anote_middlenote *=
                powf(octave[octavesize - 1].tuning,
                     (deltanote - 1) / octavesize);
        if(minus != 0)
            rap_anote_middlenote = 1.0f / rap_anote_middlenote;

        //Convert from note (midi) to degree (note from the tunning)
        int degoct =
            (note - (int)Pmiddlenote + (int) Pmapsize
             * 200) / (int)Pmapsize - 200;
        int degkey = (note - Pmiddlenote + (int)Pmapsize * 100) % Pmapsize;
        degkey = Pmapping[degkey];
        if(degkey < 0)
            return -1.0f;           //this key is not mapped

        //invert the keyboard upside-down if it is asked for
        //TODO: do the right way by using Pinvertupdowncenter
        if(Pinvertupdown != 0) {
            degkey = octavesize - degkey - 1;
            degoct = -degoct;
        }
        //compute the frequency of the note
        degkey  = degkey + scaleshift;
        degoct += degkey / octavesize;
        degkey %= octavesize;

        float freq = (degkey == 0) ? (1.0f) : octave[degkey - 1].tuning;
        freq *= powf(octave[octavesize - 1].tuning, degoct);
        freq *= PAfreq / rap_anote_middlenote;
        freq *= globalfinedetunerap;
        if(scaleshift != 0)
            freq /= octave[scaleshift - 1].tuning;
        return freq * rap_keyshift;
    }
    else {  //if the mapping is disabled
        int nt    = note - PAnote + scaleshift;
        int ntkey = (nt + (int)octavesize * 100) % octavesize;
        int ntoct = (nt - ntkey) / octavesize;

        float oct  = octave[octavesize - 1].tuning;
        float freq =
            octave[(ntkey + octavesize - 1) % octavesize].tuning * powf(oct,
                                                                        ntoct)
            * PAfreq;
        if(ntkey == 0)
            freq /= oct;
        if(scaleshift != 0)
            freq /= octave[scaleshift - 1].tuning;
//	fprintf(stderr,"note=%d freq=%.3f cents=%d\n",note,freq,(int)floor(logf(freq/PAfreq)/logf(2.0f)*1200.0f+0.5f));
        freq *= globalfinedetunerap;
        return freq * rap_keyshift;
    }
}

bool Microtonal::operator==(const Microtonal &micro) const
{
    return !(*this != micro);
}

bool Microtonal::operator!=(const Microtonal &micro) const
{
    //A simple macro to test equality MiCRotonal EQuals (not the perfect
    //approach, but good enough)
#define MCREQ(x) if(x != micro.x) \
        return true

    //for floats
#define FMCREQ(x) if(!((x < micro.x + 0.0001f) && (x > micro.x - 0.0001f))) \
        return true

    MCREQ(Pinvertupdown);
    MCREQ(Pinvertupdowncenter);
    MCREQ(octavesize);
    MCREQ(Penabled);
    MCREQ(PAnote);
    FMCREQ(PAfreq);
    MCREQ(Pscaleshift);

    MCREQ(Pfirstkey);
    MCREQ(Plastkey);
    MCREQ(Pmiddlenote);
    MCREQ(Pmapsize);
    MCREQ(Pmappingenabled);

    for(int i = 0; i < 128; ++i)
        MCREQ(Pmapping[i]);

    for(int i = 0; i < octavesize; ++i) {
        FMCREQ(octave[i].tuning);
        MCREQ(octave[i].type);
        MCREQ(octave[i].x1);
        MCREQ(octave[i].x2);
    }
    if(strcmp((const char *)this->Pname, (const char *)micro.Pname))
        return true;
    if(strcmp((const char *)this->Pcomment, (const char *)micro.Pcomment))
        return true;
    MCREQ(Pglobalfinedetune);
    return false;

    //undefine macros, as they are no longer needed
#undef MCREQ
#undef FMCREQ
}


/*
 * Convert a line to tunings; returns -1 if it ok
 */
int Microtonal::linetotunings(unsigned int nline, const char *line)
{
    int   x1 = -1, x2 = -1, type = -1;
    float x  = -1.0f, tmp, tuning = 1.0f;
    if(strstr(line, "/") == NULL) {
        if(strstr(line, ".") == NULL) { // M case (M=M/1)
            sscanf(line, "%d", &x1);
            x2   = 1;
            type = 2; //division
        }
        else {  // float number case
            sscanf(line, "%f", &x);
            if(x < 0.000001f)
                return 1;
            type = 1; //float type(cents)
        }
    }
    else {  // M/N case
        sscanf(line, "%d/%d", &x1, &x2);
        if((x1 < 0) || (x2 < 0))
            return 1;
        if(x2 == 0)
            x2 = 1;
        type = 2; //division
    }

    if(x1 <= 0)
        x1 = 1;     //not allow zero frequency sounds (consider 0 as 1)

    //convert to float if the number are too big
    if((type == 2)
       && ((x1 > (128 * 128 * 128 - 1)) || (x2 > (128 * 128 * 128 - 1)))) {
        type = 1;
        x    = ((float) x1) / x2;
    }
    switch(type) {
        case 1:
            x1     = (int) floor(x);
            tmp    = fmod(x, 1.0f);
            x2     = (int) (floor(tmp * 1e6));
            tuning = powf(2.0f, x / 1200.0f);
            break;
        case 2:
            x      = ((float)x1) / x2;
            tuning = x;
            break;
    }

    tmpoctave[nline].tuning = tuning;
    tmpoctave[nline].type   = type;
    tmpoctave[nline].x1     = x1;
    tmpoctave[nline].x2     = x2;

    return -1; //ok
}

/*
 * Convert the text to tunnings
 */
int Microtonal::texttotunings(const char *text)
{
    unsigned int i, k = 0, nl = 0;
    char *lin;
    lin = new char[MAX_LINE_SIZE + 1];
    while(k < strlen(text)) {
        for(i = 0; i < MAX_LINE_SIZE; ++i) {
            lin[i] = text[k++];
            if(lin[i] < 0x20)
                break;
        }
        lin[i] = '\0';
        if(strlen(lin) == 0)
            continue;
        int err = linetotunings(nl, lin);
        if(err != -1) {
            delete [] lin;
            return nl; //Parse error
        }
        nl++;
    }
    delete [] lin;
    if(nl > MAX_OCTAVE_SIZE)
        nl = MAX_OCTAVE_SIZE;
    if(nl == 0)
        return -2;        //the input is empty
    octavesize = nl;
    for(i = 0; i < octavesize; ++i) {
        octave[i].tuning = tmpoctave[i].tuning;
        octave[i].type   = tmpoctave[i].type;
        octave[i].x1     = tmpoctave[i].x1;
        octave[i].x2     = tmpoctave[i].x2;
    }
    return -1; //ok
}

/*
 * Convert the text to mapping
 */
void Microtonal::texttomapping(const char *text)
{
    unsigned int i, k = 0;
    char *lin;
    lin = new char[MAX_LINE_SIZE + 1];
    for(i = 0; i < 128; ++i)
        Pmapping[i] = -1;
    int tx = 0;
    while(k < strlen(text)) {
        for(i = 0; i < MAX_LINE_SIZE; ++i) {
            lin[i] = text[k++];
            if(lin[i] < 0x20)
                break;
        }
        lin[i] = '\0';
        if(strlen(lin) == 0)
            continue;

        int tmp = 0;
        if(sscanf(lin, "%d", &tmp) == 0)
            tmp = -1;
        if(tmp < -1)
            tmp = -1;
        Pmapping[tx] = tmp;

        if((tx++) > 127)
            break;
    }
    delete [] lin;

    if(tx == 0)
        tx = 1;
    Pmapsize = tx;
}

/*
 * Convert tunning to text line
 */
void Microtonal::tuningtoline(int n, char *line, int maxn)
{
    if((n > octavesize) || (n > MAX_OCTAVE_SIZE)) {
        line[0] = '\0';
        return;
    }
    if(octave[n].type == 1)
        snprintf(line, maxn, "%d.%06d", octave[n].x1, octave[n].x2);
    if(octave[n].type == 2)
        snprintf(line, maxn, "%d/%d", octave[n].x1, octave[n].x2);
}


int Microtonal::loadline(FILE *file, char *line)
{
    do {
        if(fgets(line, 500, file) == 0)
            return 1;
    } while(line[0] == '!');
    return 0;
}
/*
 * Loads the tunnings from a scl file
 */
int Microtonal::loadscl(const char *filename)
{
    FILE *file = fopen(filename, "r");
    char  tmp[500];
    fseek(file, 0, SEEK_SET);
    //loads the short description
    if(loadline(file, &tmp[0]) != 0)
        return 2;
    for(int i = 0; i < 500; ++i)
        if(tmp[i] < 32)
            tmp[i] = 0;
    snprintf((char *) Pname, MICROTONAL_MAX_NAME_LEN, "%s", tmp);
    snprintf((char *) Pcomment, MICROTONAL_MAX_NAME_LEN, "%s", tmp);
    //loads the number of the notes
    if(loadline(file, &tmp[0]) != 0)
        return 2;
    int nnotes = MAX_OCTAVE_SIZE;
    sscanf(&tmp[0], "%d", &nnotes);
    if(nnotes > MAX_OCTAVE_SIZE)
        return 2;
    //load the tunnings
    for(int nline = 0; nline < nnotes; ++nline) {
        if(loadline(file, &tmp[0]) != 0)
            return 2;
        linetotunings(nline, &tmp[0]);
    }
    fclose(file);

    octavesize = nnotes;
    for(int i = 0; i < octavesize; ++i) {
        octave[i].tuning = tmpoctave[i].tuning;
        octave[i].type   = tmpoctave[i].type;
        octave[i].x1     = tmpoctave[i].x1;
        octave[i].x2     = tmpoctave[i].x2;
    }

    return 0;
}

/*
 * Loads the mapping from a kbm file
 */
int Microtonal::loadkbm(const char *filename)
{
    FILE *file = fopen(filename, "r");
    int   x;
    char  tmp[500];

    fseek(file, 0, SEEK_SET);
    //loads the mapsize
    if(loadline(file, &tmp[0]) != 0)
        return 2;
    if(sscanf(&tmp[0], "%d", &x) == 0)
        return 2;
    if(x < 1)
        x = 0;
    if(x > 127)
        x = 127;     //just in case...
    Pmapsize = x;
    //loads first MIDI note to retune
    if(loadline(file, &tmp[0]) != 0)
        return 2;
    if(sscanf(&tmp[0], "%d", &x) == 0)
        return 2;
    if(x < 1)
        x = 0;
    if(x > 127)
        x = 127;     //just in case...
    Pfirstkey = x;
    //loads last MIDI note to retune
    if(loadline(file, &tmp[0]) != 0)
        return 2;
    if(sscanf(&tmp[0], "%d", &x) == 0)
        return 2;
    if(x < 1)
        x = 0;
    if(x > 127)
        x = 127;     //just in case...
    Plastkey = x;
    //loads last the middle note where scale fro scale degree=0
    if(loadline(file, &tmp[0]) != 0)
        return 2;
    if(sscanf(&tmp[0], "%d", &x) == 0)
        return 2;
    if(x < 1)
        x = 0;
    if(x > 127)
        x = 127;     //just in case...
    Pmiddlenote = x;
    //loads the reference note
    if(loadline(file, &tmp[0]) != 0)
        return 2;
    if(sscanf(&tmp[0], "%d", &x) == 0)
        return 2;
    if(x < 1)
        x = 0;
    if(x > 127)
        x = 127;     //just in case...
    PAnote = x;
    //loads the reference freq.
    if(loadline(file, &tmp[0]) != 0)
        return 2;
    float tmpPAfreq = 440.0f;
    if(sscanf(&tmp[0], "%f", &tmpPAfreq) == 0)
        return 2;
    PAfreq = tmpPAfreq;

    //the scale degree(which is the octave) is not loaded, it is obtained by the tunnings with getoctavesize() method
    if(loadline(file, &tmp[0]) != 0)
        return 2;

    //load the mappings
    if(Pmapsize != 0) {
        for(int nline = 0; nline < Pmapsize; ++nline) {
            if(loadline(file, &tmp[0]) != 0)
                return 2;
            if(sscanf(&tmp[0], "%d", &x) == 0)
                x = -1;
            Pmapping[nline] = x;
        }
        Pmappingenabled = 1;
    }
    else {
        Pmappingenabled = 0;
        Pmapping[0]     = 0;
        Pmapsize = 1;
    }
    fclose(file);

    return 0;
}



void Microtonal::add2XML(XMLwrapper *xml) const
{
    xml->addparstr("name", (char *) Pname);
    xml->addparstr("comment", (char *) Pcomment);

    xml->addparbool("invert_up_down", Pinvertupdown);
    xml->addpar("invert_up_down_center", Pinvertupdowncenter);

    xml->addparbool("enabled", Penabled);
    xml->addpar("global_fine_detune", Pglobalfinedetune);

    xml->addpar("a_note", PAnote);
    xml->addparreal("a_freq", PAfreq);

    if((Penabled == 0) && (xml->minimal))
        return;

    xml->beginbranch("SCALE");
    xml->addpar("scale_shift", Pscaleshift);
    xml->addpar("first_key", Pfirstkey);
    xml->addpar("last_key", Plastkey);
    xml->addpar("middle_note", Pmiddlenote);

    xml->beginbranch("OCTAVE");
    xml->addpar("octave_size", octavesize);
    for(int i = 0; i < octavesize; ++i) {
        xml->beginbranch("DEGREE", i);
        if(octave[i].type == 1)
            xml->addparreal("cents", octave[i].tuning);
        ;
        if(octave[i].type == 2) {
            xml->addpar("numerator", octave[i].x1);
            xml->addpar("denominator", octave[i].x2);
        }
        xml->endbranch();
    }
    xml->endbranch();

    xml->beginbranch("KEYBOARD_MAPPING");
    xml->addpar("map_size", Pmapsize);
    xml->addpar("mapping_enabled", Pmappingenabled);
    for(int i = 0; i < Pmapsize; ++i) {
        xml->beginbranch("KEYMAP", i);
        xml->addpar("degree", Pmapping[i]);
        xml->endbranch();
    }

    xml->endbranch();
    xml->endbranch();
}

void Microtonal::getfromXML(XMLwrapper *xml)
{
    xml->getparstr("name", (char *) Pname, MICROTONAL_MAX_NAME_LEN);
    xml->getparstr("comment", (char *) Pcomment, MICROTONAL_MAX_NAME_LEN);

    Pinvertupdown = xml->getparbool("invert_up_down", Pinvertupdown);
    Pinvertupdowncenter = xml->getpar127("invert_up_down_center",
                                         Pinvertupdowncenter);

    Penabled = xml->getparbool("enabled", Penabled);
    Pglobalfinedetune = xml->getpar127("global_fine_detune", Pglobalfinedetune);

    PAnote = xml->getpar127("a_note", PAnote);
    PAfreq = xml->getparreal("a_freq", PAfreq, 1.0f, 10000.0f);

    if(xml->enterbranch("SCALE")) {
        Pscaleshift = xml->getpar127("scale_shift", Pscaleshift);
        Pfirstkey   = xml->getpar127("first_key", Pfirstkey);
        Plastkey    = xml->getpar127("last_key", Plastkey);
        Pmiddlenote = xml->getpar127("middle_note", Pmiddlenote);

        if(xml->enterbranch("OCTAVE")) {
            octavesize = xml->getpar127("octave_size", octavesize);
            for(int i = 0; i < octavesize; ++i) {
                if(xml->enterbranch("DEGREE", i) == 0)
                    continue;
                octave[i].x2     = 0;
                octave[i].tuning = xml->getparreal("cents", octave[i].tuning);
                octave[i].x1     = xml->getpar127("numerator", octave[i].x1);
                octave[i].x2     = xml->getpar127("denominator", octave[i].x2);

                if(octave[i].x2 != 0)
                    octave[i].type = 2;
                else {
                    octave[i].type = 1;
                    //populate fields for display
                    float x = logf(octave[i].tuning) / LOG_2 * 1200.0f;
                    octave[i].x1 = (int) floor(x);
                    octave[i].x2 = (int) (floor(fmodf(x, 1.0f) * 1e6));
                }


                xml->exitbranch();
            }
            xml->exitbranch();
        }

        if(xml->enterbranch("KEYBOARD_MAPPING")) {
            Pmapsize = xml->getpar127("map_size", Pmapsize);
            Pmappingenabled = xml->getpar127("mapping_enabled", Pmappingenabled);
            for(int i = 0; i < Pmapsize; ++i) {
                if(xml->enterbranch("KEYMAP", i) == 0)
                    continue;
                Pmapping[i] = xml->getpar127("degree", Pmapping[i]);
                xml->exitbranch();
            }
            xml->exitbranch();
        }
        xml->exitbranch();
    }
}



int Microtonal::saveXML(const char *filename) const
{
    XMLwrapper *xml = new XMLwrapper();

    xml->beginbranch("MICROTONAL");
    add2XML(xml);
    xml->endbranch();

    int result = xml->saveXMLfile(filename);
    delete (xml);
    return result;
}

int Microtonal::loadXML(const char *filename)
{
    XMLwrapper *xml = new XMLwrapper();
    if(xml->loadXMLfile(filename) < 0) {
        delete (xml);
        return -1;
    }

    if(xml->enterbranch("MICROTONAL") == 0)
        return -10;
    getfromXML(xml);
    xml->exitbranch();

    delete (xml);
    return 0;
}
