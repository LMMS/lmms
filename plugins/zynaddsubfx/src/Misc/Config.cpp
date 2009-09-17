/*
  ZynAddSubFX - a software synthesizer

  Config.C - Configuration file functions
  Copyright (C) 2003-2005 Nasca Octavian Paul
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
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef OS_WINDOWS
#include <windows.h>
#include <mmsystem.h>
#endif

#include "Config.h"
#include "XMLwrapper.h"

Config::Config()
{
};
void Config::init()
{
    maxstringsize=MAX_STRING_SIZE;//for ui
    //defaults
    cfg.SampleRate=44100;
    cfg.SoundBufferSize=256;
    cfg.OscilSize=1024;
    cfg.SwapStereo=0;

    cfg.LinuxOSSWaveOutDev=new char[MAX_STRING_SIZE];
    snprintf(cfg.LinuxOSSWaveOutDev,MAX_STRING_SIZE,"/dev/dsp");
    cfg.LinuxOSSSeqInDev=new char[MAX_STRING_SIZE];
    snprintf(cfg.LinuxOSSSeqInDev,MAX_STRING_SIZE,"/dev/sequencer");

    cfg.DumpFile=new char[MAX_STRING_SIZE];
    snprintf(cfg.DumpFile,MAX_STRING_SIZE,"zynaddsubfx_dump.txt");

    cfg.WindowsWaveOutId=0;
    cfg.WindowsMidiInId=0;

    cfg.BankUIAutoClose=0;
    cfg.DumpNotesToFile=0;
    cfg.DumpAppend=1;

    cfg.GzipCompression=3;

    cfg.Interpolation=0;
    cfg.CheckPADsynth=1;

    cfg.UserInterfaceMode=0;
    cfg.VirKeybLayout=1;
    winwavemax=1;
    winmidimax=1;
//try to find out how many input midi devices are there
#ifdef WINMIDIIN
    winmidimax=midiInGetNumDevs();
    if (winmidimax==0) winmidimax=1;
#endif
    winmididevices=new winmidionedevice[winmidimax];
    for (int i=0;i<winmidimax;i++) {
        winmididevices[i].name=new char[MAX_STRING_SIZE];
        for (int j=0;j<MAX_STRING_SIZE;j++) winmididevices[i].name[j]='\0';
    };


//get the midi input devices name
#ifdef WINMIDIIN
    MIDIINCAPS midiincaps;
    for (int i=0;i<winmidimax;i++) {
        if (! midiInGetDevCaps(i,&midiincaps,sizeof(MIDIINCAPS)))
            snprintf(winmididevices[i].name,MAX_STRING_SIZE,"%s",midiincaps.szPname);
    };
#endif
    for (int i=0;i<MAX_BANK_ROOT_DIRS;i++) cfg.bankRootDirList[i]=NULL;
    cfg.currentBankDir=new char[MAX_STRING_SIZE];
    sprintf(cfg.currentBankDir,"./testbnk");

    for (int i=0;i<MAX_BANK_ROOT_DIRS;i++) cfg.presetsDirList[i]=NULL;

    char filename[MAX_STRING_SIZE];
    getConfigFileName(filename,MAX_STRING_SIZE);
    readConfig(filename);

    if (cfg.bankRootDirList[0]==NULL) {
#if defined(OS_LINUX)
        //banks
        cfg.bankRootDirList[0]=new char[MAX_STRING_SIZE];
        sprintf(cfg.bankRootDirList[0],"~/banks");

        cfg.bankRootDirList[1]=new char[MAX_STRING_SIZE];
        sprintf(cfg.bankRootDirList[1],"./");

        cfg.bankRootDirList[2]=new char[MAX_STRING_SIZE];
        sprintf(cfg.bankRootDirList[2],"/usr/share/zynaddsubfx/banks");

        cfg.bankRootDirList[3]=new char[MAX_STRING_SIZE];
        sprintf(cfg.bankRootDirList[3],"/usr/local/share/zynaddsubfx/banks");

        cfg.bankRootDirList[4]=new char[MAX_STRING_SIZE];
        sprintf(cfg.bankRootDirList[4],"../banks");

        cfg.bankRootDirList[5]=new char[MAX_STRING_SIZE];
        sprintf(cfg.bankRootDirList[5],"banks");

#else
        //banks
        cfg.bankRootDirList[0]=new char[MAX_STRING_SIZE];
        sprintf(cfg.bankRootDirList[0],"./");

#ifdef VSTAUDIOOUT
        cfg.bankRootDirList[1]=new char[MAX_STRING_SIZE];
        sprintf(cfg.bankRootDirList[1],"c:/Program Files/ZynAddSubFX/banks");
#else
        cfg.bankRootDirList[1]=new char[MAX_STRING_SIZE];
        sprintf(cfg.bankRootDirList[1],"../banks");
#endif
        cfg.bankRootDirList[2]=new char[MAX_STRING_SIZE];
        sprintf(cfg.bankRootDirList[2],"banks");

#endif
    };

    if (cfg.presetsDirList[0]==NULL) {
#if defined(OS_LINUX)
        //presets
        cfg.presetsDirList[0]=new char[MAX_STRING_SIZE];
        sprintf(cfg.presetsDirList[0],"./");

        cfg.presetsDirList[1]=new char[MAX_STRING_SIZE];
        sprintf(cfg.presetsDirList[1],"../presets");

        cfg.presetsDirList[2]=new char[MAX_STRING_SIZE];
        sprintf(cfg.presetsDirList[2],"presets");

        cfg.presetsDirList[3]=new char[MAX_STRING_SIZE];
        sprintf(cfg.presetsDirList[3],"/usr/share/zynaddsubfx/presets");

        cfg.presetsDirList[4]=new char[MAX_STRING_SIZE];
        sprintf(cfg.presetsDirList[4],"/usr/local/share/zynaddsubfx/presets");

#else
        //presets
        cfg.presetsDirList[0]=new char[MAX_STRING_SIZE];
        sprintf(cfg.presetsDirList[0],"./");

#ifdef VSTAUDIOOUT
        cfg.presetsDirList[1]=new char[MAX_STRING_SIZE];
        sprintf(cfg.presetsDirList[1],"c:/Program Files/ZynAddSubFX/presets");
#else
        cfg.presetsDirList[1]=new char[MAX_STRING_SIZE];
        sprintf(cfg.presetsDirList[1],"../presets");
#endif

        cfg.presetsDirList[2]=new char[MAX_STRING_SIZE];
        sprintf(cfg.presetsDirList[2],"presets");
#endif
    };

};

Config::~Config()
{

    delete [] cfg.LinuxOSSWaveOutDev;
    delete [] cfg.LinuxOSSSeqInDev;
    delete [] cfg.DumpFile;

    for (int i=0;i<winmidimax;i++) delete [] winmididevices[i].name;
    delete [] winmididevices;
};


void Config::save()
{
    char filename[MAX_STRING_SIZE];
    getConfigFileName(filename,MAX_STRING_SIZE);
    saveConfig(filename);
};

void Config::clearbankrootdirlist()
{
    for (int i=0;i<MAX_BANK_ROOT_DIRS;i++) {
        if (cfg.bankRootDirList[i]==NULL) delete(cfg.bankRootDirList[i]);
        cfg.bankRootDirList[i]=NULL;
    };
};

void Config::clearpresetsdirlist()
{
    for (int i=0;i<MAX_BANK_ROOT_DIRS;i++) {
        if (cfg.presetsDirList[i]==NULL) delete(cfg.presetsDirList[i]);
        cfg.presetsDirList[i]=NULL;
    };
};

void Config::readConfig(const char *filename)
{
    XMLwrapper *xmlcfg=new XMLwrapper();
    if (xmlcfg->loadXMLfile(filename)<0) return;
    if (xmlcfg->enterbranch("CONFIGURATION")) {
        cfg.SampleRate=xmlcfg->getpar("sample_rate",cfg.SampleRate,4000,1024000);
        cfg.SoundBufferSize=xmlcfg->getpar("sound_buffer_size",cfg.SoundBufferSize,16,8192);
        cfg.OscilSize=xmlcfg->getpar("oscil_size",cfg.OscilSize,MAX_AD_HARMONICS*2,131072);
        cfg.SwapStereo=xmlcfg->getpar("swap_stereo",cfg.SwapStereo,0,1);
        cfg.BankUIAutoClose=xmlcfg->getpar("bank_window_auto_close",cfg.BankUIAutoClose,0,1);

        cfg.DumpNotesToFile=xmlcfg->getpar("dump_notes_to_file",cfg.DumpNotesToFile,0,1);
        cfg.DumpAppend=xmlcfg->getpar("dump_append",cfg.DumpAppend,0,1);
        xmlcfg->getparstr("dump_file",cfg.DumpFile,MAX_STRING_SIZE);

        cfg.GzipCompression=xmlcfg->getpar("gzip_compression",cfg.GzipCompression,0,9);

        xmlcfg->getparstr("bank_current",cfg.currentBankDir,MAX_STRING_SIZE);
        cfg.Interpolation=xmlcfg->getpar("interpolation",cfg.Interpolation,0,1);

        cfg.CheckPADsynth=xmlcfg->getpar("check_pad_synth",cfg.CheckPADsynth,0,1);


        cfg.UserInterfaceMode=xmlcfg->getpar("user_interface_mode",cfg.UserInterfaceMode,0,2);
        cfg.VirKeybLayout=xmlcfg->getpar("virtual_keyboard_layout",cfg.VirKeybLayout,0,10);

        //get bankroot dirs
        for (int i=0;i<MAX_BANK_ROOT_DIRS;i++) {
            if (xmlcfg->enterbranch("BANKROOT",i)) {
                cfg.bankRootDirList[i]=new char[MAX_STRING_SIZE];
                xmlcfg->getparstr("bank_root",cfg.bankRootDirList[i],MAX_STRING_SIZE);
                xmlcfg->exitbranch();
            };
        };

        //get preset root dirs
        for (int i=0;i<MAX_BANK_ROOT_DIRS;i++) {
            if (xmlcfg->enterbranch("PRESETSROOT",i)) {
                cfg.presetsDirList[i]=new char[MAX_STRING_SIZE];
                xmlcfg->getparstr("presets_root",cfg.presetsDirList[i],MAX_STRING_SIZE);
                xmlcfg->exitbranch();
            };
        };

        //linux stuff
        xmlcfg->getparstr("linux_oss_wave_out_dev",cfg.LinuxOSSWaveOutDev,MAX_STRING_SIZE);
        xmlcfg->getparstr("linux_oss_seq_in_dev",cfg.LinuxOSSSeqInDev,MAX_STRING_SIZE);

        //windows stuff
        cfg.WindowsWaveOutId=xmlcfg->getpar("windows_wave_out_id",cfg.WindowsWaveOutId,0,winwavemax);
        cfg.WindowsMidiInId=xmlcfg->getpar("windows_midi_in_id",cfg.WindowsMidiInId,0,winmidimax);

        xmlcfg->exitbranch();
    };
    delete(xmlcfg);

    cfg.OscilSize=(int) pow(2,ceil(log (cfg.OscilSize-1.0)/log(2.0)));

};

void Config::saveConfig(const char *filename)
{
    XMLwrapper *xmlcfg=new XMLwrapper();

    xmlcfg->beginbranch("CONFIGURATION");

    xmlcfg->addpar("sample_rate",cfg.SampleRate);
    xmlcfg->addpar("sound_buffer_size",cfg.SoundBufferSize);
    xmlcfg->addpar("oscil_size",cfg.OscilSize);
    xmlcfg->addpar("swap_stereo",cfg.SwapStereo);
    xmlcfg->addpar("bank_window_auto_close",cfg.BankUIAutoClose);

    xmlcfg->addpar("dump_notes_to_file",cfg.DumpNotesToFile);
    xmlcfg->addpar("dump_append",cfg.DumpAppend);
    xmlcfg->addparstr("dump_file",cfg.DumpFile);

    xmlcfg->addpar("gzip_compression",cfg.GzipCompression);

    xmlcfg->addpar("check_pad_synth",cfg.CheckPADsynth);

    xmlcfg->addparstr("bank_current",cfg.currentBankDir);

    xmlcfg->addpar("user_interface_mode",cfg.UserInterfaceMode);
    xmlcfg->addpar("virtual_keyboard_layout",cfg.VirKeybLayout);


    for (int i=0;i<MAX_BANK_ROOT_DIRS;i++) if (cfg.bankRootDirList[i]!=NULL) {
            xmlcfg->beginbranch("BANKROOT",i);
            xmlcfg->addparstr("bank_root",cfg.bankRootDirList[i]);
            xmlcfg->endbranch();
        };

    for (int i=0;i<MAX_BANK_ROOT_DIRS;i++) if (cfg.presetsDirList[i]!=NULL) {
            xmlcfg->beginbranch("PRESETSROOT",i);
            xmlcfg->addparstr("presets_root",cfg.presetsDirList[i]);
            xmlcfg->endbranch();
        };

    xmlcfg->addpar("interpolation",cfg.Interpolation);

    //linux stuff
    xmlcfg->addparstr("linux_oss_wave_out_dev",cfg.LinuxOSSWaveOutDev);
    xmlcfg->addparstr("linux_oss_seq_in_dev",cfg.LinuxOSSSeqInDev);

    //windows stuff
    xmlcfg->addpar("windows_wave_out_id",cfg.WindowsWaveOutId);
    xmlcfg->addpar("windows_midi_in_id",cfg.WindowsMidiInId);

    xmlcfg->endbranch();

    int tmp=cfg.GzipCompression;
    cfg.GzipCompression=0;
    xmlcfg->saveXMLfile(filename);
    cfg.GzipCompression=tmp;

    delete(xmlcfg);
};

void Config::getConfigFileName(char *name, int namesize)
{
    name[0]=0;
#ifdef OS_LINUX
    snprintf(name,namesize,"%s%s",getenv("HOME"),"/.zynaddsubfxXML.cfg");
#else
    snprintf(name,namesize,"%s","zynaddsubfxXML.cfg");
#endif

};

