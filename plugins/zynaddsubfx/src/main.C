/*
  ZynAddSubFX - a software synthesizer

  main.c  -  Main file of the synthesizer
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
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <unistd.h>
#include <pthread.h>

#ifdef OS_LINUX
#include <getopt.h>
#elif OS_WINDOIWS
#include <winbase.h>
#include <windows.h>
#endif

#include "Misc/Master.h"
#include "Misc/Util.h"
#include "Misc/Dump.h"
extern Dump dump;

#ifdef ALSAMIDIIN
#include "Input/ALSAMidiIn.h"
#endif

#ifdef OSSMIDIIN
#include "Input/OSSMidiIn.h"
#endif

#if (defined(NONEMIDIIN)||defined(VSTMIDIIN))
#include "Input/NULLMidiIn.h"
#endif

#ifdef WINMIDIIN
#include "Input/WINMidiIn.h"
#endif

#ifndef DISABLE_GUI
#include "UI/MasterUI.h"
MasterUI *ui;
#endif

pthread_t thr1,thr2,thr3,thr4;
Master *master;
int swaplr=0;//1 for left-right swapping
bool usejackit=false;

#ifdef JACKAUDIOOUT
#include "Output/JACKaudiooutput.h"
#endif

#ifdef JACK_RTAUDIOOUT
#include "Output/JACKaudiooutput.h"
#endif

#ifdef PAAUDIOOUT
#include "Output/PAaudiooutput.h"
#endif

#ifdef OSSAUDIOOUT
#include "Output/OSSaudiooutput.h"
OSSaudiooutput *audioout;
#endif

#ifdef USE_LASH
#include "Misc/LASHClient.h"
LASHClient *lash;
#endif

MidiIn *Midi;
int Pexitprogram=0;//if the UI set this to 1, the program will exit

/*
 * Try to get the realtime priority
 */
void set_realtime(){
#ifdef OS_LINUX
    sched_param sc;
    
    sc.sched_priority=50;

    //if you want get "sched_setscheduler undeclared" from compilation, you can safely remove the folowing line
    sched_setscheduler(0,SCHED_FIFO,&sc);
//    if (err==0) printf("Real-time");
#endif
};

/*
 * Midi input thread
 */
#if !(defined(WINMIDIIN)||defined(VSTMIDIIN))
void *thread1(void *arg){
    MidiCmdType cmdtype;
    unsigned char cmdchan,note,vel;
    int cmdparams[MP_MAX_BYTES];

    set_realtime();
    while (Pexitprogram==0){
	Midi->getmidicmd(cmdtype,cmdchan,cmdparams);
        note=cmdparams[0];
	vel=cmdparams[1];
	
	pthread_mutex_lock(&master->mutex);

	if ((cmdtype==MidiNoteON)&&(note!=0)) master->NoteOn(cmdchan,note,vel);
	if ((cmdtype==MidiNoteOFF)&&(note!=0)) master->NoteOff(cmdchan,note);
	if (cmdtype==MidiController) master->SetController(cmdchan,cmdparams[0],cmdparams[1]);

	pthread_mutex_unlock(&master->mutex);
    };
    
    return(0);
};
#endif

/*
 * Wave output thread (for OSS AUDIO out)
 */
#if defined(OSSAUDIOOUT)
//!(defined(JACKAUDIOOUT)||defined(JACK_RTAUDIOOUT)||defined(PAAUDIOOUT)||defined(VSTAUDIOOUT))

void *thread2(void *arg){
    REALTYPE outputl[SOUND_BUFFER_SIZE];
    REALTYPE outputr[SOUND_BUFFER_SIZE];

    set_realtime();
    while (Pexitprogram==0){
	pthread_mutex_lock(&master->mutex);
	master->AudioOut(outputl,outputr);
	pthread_mutex_unlock(&master->mutex);

#ifndef NONEAUDIOOUT
    audioout->OSSout(outputl,outputr);
#endif

/** /  	int i,x,x2;
	REALTYPE xx,xx2;
 
    	short int xsmps[SOUND_BUFFER_SIZE*2];    
	for (i=0;i<SOUND_BUFFER_SIZE;i++){//output to stdout
		xx=-outputl[i]*32767;
		xx2=-outputr[i]*32767;
		if (xx<-32768) xx=-32768;
		if (xx>32767) xx=32767;
		if (xx2<-32768) xx2=-32768;
		if (xx2>32767) xx2=32767;
		x=(short int) xx;
		x2=(short int) xx2;
		xsmps[i*2]=x;xsmps[i*2+1]=x2;
		};
		write(1,&xsmps,SOUND_BUFFER_SIZE*2*2);
		
		/ * */
	}; 
    return(0);
};
#endif

/*
 * User Interface thread
 */
 
 
void *thread3(void *arg){
#ifndef DISABLE_GUI
    ui->showUI();
    while (Pexitprogram==0) {
#ifdef USE_LASH
      std::string filename;
      switch (lash->checkevents(filename)) {
      case LASHClient::Save:
        ui->do_save_master(const_cast<char*>(filename.c_str()));
        lash->confirmevent(LASHClient::Save);
        break;
      case LASHClient::Restore:
        ui->do_load_master(const_cast<char*>(filename.c_str()));
        lash->confirmevent(LASHClient::Restore);
        break;
      case LASHClient::Quit:
        Pexitprogram = 1;
      default:
        break;
      }
#endif
      Fl::wait();
    }
#endif
    return(0);
};

/*
 * Sequencer thread (test)
 */
void *thread4(void *arg){
    while (Pexitprogram==0){
	int type,par1,par2,again,midichan;
	for (int ntrack=0;ntrack<NUM_MIDI_TRACKS;ntrack++){
	    if (master->seq.play==0) break;
	    do{
		again=master->seq.getevent(ntrack,&midichan,&type,&par1,&par2);
//		printf("ntrack=%d again=%d\n",ntrack,again);
		if (type>0) {
//	    printf("%d %d  %d %d %d\n",type,midichan,chan,par1,par2);

//	if (cmdtype==MidiController) master->SetController(cmdchan,cmdparams[0],cmdparams[1]);

	
	
		    pthread_mutex_lock(&master->mutex);
	    	    if (type==1){//note_on or note_off
			if (par2!=0) master->NoteOn(midichan,par1,par2);
			    else master->NoteOff(midichan,par1);
	    	    };
		    pthread_mutex_unlock(&master->mutex);
		};
	    } while (again>0);
	    
	};
//if (!realtime player) atunci fac asta
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#ifdef OS_LINUX
	usleep(1000);
#elif OS_WINDOWS
	Sleep(1);
#endif
    };
    
    return(0);
};

/*
 * Program initialisation
 */


void initprogram(){
#ifndef JACKAUDIOOUT
#ifndef JACK_RTAUDIOOUT
    fprintf(stderr,"\nSample Rate = \t\t%d\n",SAMPLE_RATE);
#endif
#endif
    fprintf(stderr,"Sound Buffer Size = \t%d samples\n",SOUND_BUFFER_SIZE);
    fprintf(stderr,"Internal latency = \t%.1f ms\n",SOUND_BUFFER_SIZE*1000.0/SAMPLE_RATE);
    fprintf(stderr,"ADsynth Oscil.Size = \t%d samples\n",OSCIL_SIZE);

    fflush(stderr);
    srand(time(NULL));
    denormalkillbuf=new REALTYPE [SOUND_BUFFER_SIZE];
    for (int i=0;i<SOUND_BUFFER_SIZE;i++) denormalkillbuf[i]=(RND-0.5)*1e-16;
    
    OscilGen::tmpsmps=new REALTYPE[OSCIL_SIZE];
    newFFTFREQS(&OscilGen::outoscilFFTfreqs,OSCIL_SIZE/2);
    
    master=new Master();
    master->swaplr=swaplr;

#if defined(JACKAUDIOOUT)
    if (usejackit) {
	bool tmp=JACKaudiooutputinit(master);
#if defined(OSSAUDIOOUT)
	if (!tmp) printf("\nUsing OSS instead.\n");
#else
	if (!tmp) exit(1);
#endif
	usejackit=tmp;
    };
#endif
#if defined(OSSAUDIOOUT)
    if (!usejackit) audioout=new OSSaudiooutput();
	else audioout=NULL;
#endif

#ifdef JACK_RTAUDIOOUT
    JACKaudiooutputinit(master);
#endif
#ifdef PAAUDIOOUT
    PAaudiooutputinit(master);
#endif 

#ifdef ALSAMIDIIN
    Midi=new ALSAMidiIn();
#endif
#ifdef OSSMIDIIN
    Midi=new OSSMidiIn();
#endif
#if (defined(NONEMIDIIN)||(defined(VSTMIDIIN)))
    Midi=new NULLMidiIn();
#endif
#ifndef DISABLE_GUI
    ui=new MasterUI(master,&Pexitprogram);
#endif
};

/*
 * Program exit
 */
void exitprogram(){
    pthread_mutex_lock(&master->mutex);
#ifdef OSSAUDIOOUT
    delete(audioout);
#endif
#ifdef JACKAUDIOOUT
    if (usejackit) JACKfinish();
#endif
#ifdef JACK_RTAUDIOOUT
    JACKfinish();
#endif
#ifdef PAAUDIOOUT
    PAfinish();
#endif

#ifndef DISABLE_GUI
    delete(ui);
#endif
    delete(Midi);
    delete(master); 

#ifdef USE_LASH
    delete(lash);
#endif

//    pthread_mutex_unlock(&master->mutex);
    delete [] denormalkillbuf;
    delete [] OscilGen::tmpsmps;
    deleteFFTFREQS(&OscilGen::outoscilFFTfreqs);

};

#ifdef OS_WINDOWS
#define ARGSIZE 100
 char winoptarguments[ARGSIZE];
 char getopt(int argc, char *argv[], const char *shortopts, int *index){
    winoptarguments[0]=0;
    char result=0;
    
    if (*index>=argc) return(-1);
    
    if (strlen(argv[*index])==2) 
	if (argv[*index][0]=='-') {
	    result=argv[*index][1];
	    if (*index+1<argc) {
		snprintf(winoptarguments,ARGSIZE,"%s",argv[*index+1]);
	    };
	};
    (*index)++;
    return(result);
 };
 int opterr=0;
#undef ARGSIZE
 
#endif

#ifndef VSTAUDIOOUT
int main(int argc, char *argv[]){

#ifdef USE_LASH
    lash = new LASHClient(&argc, &argv);
#endif

    config.init();
    dump.startnow();
    int noui=0;
#ifdef JACKAUDIOOUT
    usejackit=true;//use jack by default
#endif
    fprintf(stderr,"%s","\nZynAddSubFX - Copyright (c) 2002-2008 Nasca Octavian Paul and others\n");
    fprintf(stderr,"Compiled: %s %s\n",__DATE__,__TIME__);
    fprintf(stderr,"%s","This program is free software (GNU GPL v.2 or later) and \n    it comes with ABSOLUTELY NO WARRANTY.\n\n");
#ifdef OS_LINUX
    if (argc==1) fprintf(stderr,"%s","Try 'zynaddsubfx --help' for command-line options.\n");
#else
    if (argc==1) fprintf(stderr,"%s","Try 'zynaddsubfx -h' for command-line options.\n");
#endif
    /* Get the settings from the Config*/
    SAMPLE_RATE=config.cfg.SampleRate;	
    SOUND_BUFFER_SIZE=config.cfg.SoundBufferSize;
    OSCIL_SIZE=config.cfg.OscilSize;
    swaplr=config.cfg.SwapStereo;
    
    /* Parse command-line options */
#ifdef OS_LINUX
    struct option opts[]={
	{"load",2,NULL,'l'},
	{"load-instrument",2,NULL,'L'},
	{"sample-rate",2,NULL,'r'},
	{"buffer-size",2,NULL,'b'},
	{"oscil-size",2,NULL,'o'},
	{"dump",2,NULL,'D'},
	{"swap",2,NULL,'S'},
	{"no-gui",2,NULL,'U'},
	{"not-use-jack",2,NULL,'A'},
	{"dummy",2,NULL,'Y'},
	{"help",2,NULL,'h'},
	{0,0,0,0}
    };
#endif
    opterr=0;
    int option_index=0,opt,exitwithhelp=0;
    
    char loadfile[1001];ZERO(loadfile,1001);
    char loadinstrument[1001];ZERO(loadinstrument,1001);
    
    while (1){
#ifdef OS_LINUX
	opt=getopt_long(argc,argv,"l:L:r:b:o:hSDUAY",opts,&option_index);
	char *optarguments=optarg;
#else
	opt=getopt(argc,argv,"l:L:r:b:o:hSDUAY",&option_index);
	char *optarguments=&winoptarguments[0];
#endif 

	if (opt==-1) break;

	int tmp;
	switch(opt){
	    case 'h':exitwithhelp=1;
		     break;
	    case 'Y':/* this command a dummy command (has NO effect) 
		        and is used because I need for NSIS installer 
			(NSIS sometimes forces a command line for a 
			program, even if I don't need that; eg. when 
			I want to add a icon to a shortcut.
		      */
		     break;
	    case 'U':noui=1;
		     break;
	    case 'A':
#ifdef JACKAUDIOOUT
#ifdef OSSAUDIOOUT
		     usejackit=false;
#endif
#endif
		     break;
	    case 'l':tmp=0;
	             if (optarguments!=NULL) {
		        snprintf(loadfile,1000,"%s",optarguments);
		     };
		     break;
	    case 'L':tmp=0;
	             if (optarguments!=NULL) {
		        snprintf(loadinstrument,1000,"%s",optarguments);
		     };
		     break;
	    case 'r':tmp=0;
	             if (optarguments!=NULL) tmp=atoi(optarguments);
		     if (tmp>=4000) {
		        SAMPLE_RATE=tmp;
		     } else {
		        fprintf(stderr,"ERROR:Incorrect sample rate  %s .\n",optarguments);
			exit(1);
		     };
		     break;
	    case 'b':tmp=0;
	             if (optarguments!=NULL) tmp=atoi(optarguments);
		     if (tmp>=2) {
		        SOUND_BUFFER_SIZE=tmp;
		     } else {
		        fprintf(stderr,"ERROR:Incorrect buffer size  %s .\n",optarguments);
			exit(1);
		     };
		     break;
	    case 'o':tmp=0;
	             if (optarguments!=NULL) tmp=atoi(optarguments);
		     OSCIL_SIZE=tmp;
	    	    if (OSCIL_SIZE<MAX_AD_HARMONICS*2) OSCIL_SIZE=MAX_AD_HARMONICS*2;
		     OSCIL_SIZE=(int) pow(2,ceil(log (OSCIL_SIZE-1.0)/log(2.0)));
		     if (tmp!=OSCIL_SIZE) fprintf(stderr,"\nOSCIL_SIZE is wrong (must me 2^n) or too small. Adjusting to %d.\n",OSCIL_SIZE);
		     break;
	    case 'S':swaplr=1;
		     break;
	    case 'D':dump.startnow();
		     break;
	    case '?':fprintf(stderr,"%s","ERROR:Bad option or parameter.\n\n");
	             exitwithhelp=1;
		     break;
	};
    };
    
    if (exitwithhelp!=0) {
	fprintf(stderr,"%s","Usage: zynaddsubfx [OPTION]\n\n");
	fprintf(stderr,"%s","  -h , --help \t\t\t\t display command-line help and exit\n");
	fprintf(stderr,"%s","  -l file, --load=FILE\t\t\t loads a .xmz file\n");
	fprintf(stderr,"%s","  -L file, --load-instrument=FILE\t\t loads a .xiz file\n");
	fprintf(stderr,"%s","  -r SR, --sample-rate=SR\t\t set the sample rate SR\n");
	fprintf(stderr,"%s","  -b BS, --buffer-size=SR\t\t set the buffer size (granularity)\n");
	fprintf(stderr,"%s","  -o OS, --oscil-size=OS\t\t set the ADsynth oscil. size\n");
	fprintf(stderr,"%s","  -S , --swap\t\t\t\t swap Left <--> Right\n");
	fprintf(stderr,"%s","  -D , --dump\t\t\t\t Dumps midi note ON/OFF commands\n");
	fprintf(stderr,"%s","  -U , --no-gui\t\t\t\t Run ZynAddSubFX without user interface\n");
#ifdef JACKAUDIOOUT
#ifdef OSSAUDIOOUT
	fprintf(stderr,"%s","  -A , --not-use-jack\t\t\t Use OSS/ALSA instead of JACK\n");
#endif
#endif
#ifdef OS_WINDOWS
	fprintf(stderr,"%s","\nWARNING: On Windows systems, only short comandline parameters works.\n");
	fprintf(stderr,"%s","  eg. instead '--buffer-size=512' use '-b 512'\n");
#endif
	fprintf(stderr,"%s","\n\n");
	return(0);
    };
    
    //---------
    
    initprogram();

#ifdef USE_LASH
#ifdef ALSAMIDIIN
    ALSAMidiIn* alsamidi = dynamic_cast<ALSAMidiIn*>(Midi);
    if (alsamidi)
      lash->setalsaid(alsamidi->getalsaid());
#endif
#ifdef JACKAUDIOOUT
    lash->setjackname(JACKgetname());
#endif
#endif

    if (strlen(loadfile)>1){
        int tmp=master->loadXML(loadfile);
	if (tmp<0) {
	    fprintf(stderr,"ERROR:Could not load master file  %s .\n",loadfile);
	    exit(1);
	} else {
	    master->applyparameters();
#ifndef DISABLE_GUI
	    if (noui==0) ui->refresh_master_ui();
#endif
	    printf("Master file loaded.\n");
	};
    };

    if (strlen(loadinstrument)>1){
	int loadtopart=0;
        int tmp=master->part[loadtopart]->loadXMLinstrument(loadinstrument);
	if (tmp<0) {
	    fprintf(stderr,"ERROR:Could not load instrument file  %s .\n",loadinstrument);
	    exit(1);
	} else {
	    master->part[loadtopart]->applyparameters();
#ifndef DISABLE_GUI
	    if (noui==0) ui->refresh_master_ui();
#endif
	    printf("Instrument file loaded.\n");
	};
    };
    

#if !(defined(NONEMIDIIN)||defined(WINMIDIIN)||defined(VSTMIDIIN))
    pthread_create(&thr1,NULL,thread1,NULL);
#endif

#ifdef OSSAUDIOOUT
//!(defined(JACKAUDIOOUT)||defined(JACK_RTAUDIOOUT)||defined(PAAUDIOOUT)||defined(VSTAUDIOOUT))
    if (!usejackit) pthread_create(&thr2,NULL,thread2,NULL);
#endif

/*It is not working and I don't know why
//drop the suid-root permisions
#if !(defined(JACKAUDIOOUT)||defined(PAAUDIOOUT)||defined(VSTAUDIOOUT)|| (defined (WINMIDIIN)) )
      setuid(getuid());
      seteuid(getuid());
//      setreuid(getuid(),getuid());
//      setregid(getuid(),getuid());
#endif
*/
    if (noui==0) pthread_create(&thr3,NULL,thread3,NULL);

    pthread_create(&thr4,NULL,thread4,NULL);
#ifdef WINMIDIIN
	InitWinMidi(master);
#endif

    while (Pexitprogram==0){ 
#ifdef OS_LINUX
    usleep(100000);
#elif OS_WINDOWS
    Sleep(100);
#endif
    };    
    
#ifdef WINMIDIIN
	StopWinMidi();
#endif

    exitprogram();
    return(0);
};


#else 

#include "Output/VSTaudiooutput.h"

#define main main_plugin
extern "C" __declspec(dllexport) AEffect *main_plugin(audioMasterCallback audioMaster);

int instances=-1;

AEffect *main (audioMasterCallback audioMaster){
//    if (audioMaster(0,audioMasterVersion,0,0,0,0)!=0) {
//	return(0);
//    };

    if (instances==-1){
	Midi=new NULLMidiIn();
	denormalkillbuf=new REALTYPE [SOUND_BUFFER_SIZE];
	for (int i=0;i<SOUND_BUFFER_SIZE;i++) denormalkillbuf[i]=(RND-0.5)*1e-16;
	instances=0;
    };

    if (instances!=0) return(0);//don't allow multiple instances

    AudioEffect *sintetizator=new VSTSynth(audioMaster);

    return sintetizator->getAeffect();
};

void* hInstance;
BOOL WINAPI DllMain (HINSTANCE hInst,DWORD dwReason,LPVOID lpvReserved){
    hInstance=hInst;
    return(1);
};

void *thread(void *arg){
    VSTSynth *vs=(VSTSynth *) arg;

/*    FILE *a=fopen("aaaa1","a");
    fprintf(a,"%lx %lx %lx -i=%d\n",vs,0,vs->vmaster,instances);
    fflush(a);fclose(a);
*/

    vs->ui=new MasterUI(vs->vmaster,&vs->Pexitprogram);

/*    a=fopen("aaaa1","a");
    fprintf(a,"%lx %lx %lx\n",vs,vs->ui->master,vs->vmaster);
    fflush(a);fclose(a);
*/

    vs->ui->showUI();

/*    a=fopen("aaaa1","a");
    fprintf(a,"%lx %lx %lx\n",vs,vs->ui,vs->vmaster);
    fflush(a);fclose(a);
*/

    while (vs->Pexitprogram==0) Fl::wait(0.01);

    delete(vs->ui);
    Fl::wait(0.01);

/*    a=fopen("aaaa1","a");
    fprintf(a,"EXIT\n");
    fflush(a);fclose(a);
*/

    
    pthread_exit(0);
    return(0);
};

//Parts of the VSTSynth class
VSTSynth::VSTSynth (audioMasterCallback audioMaster):AudioEffectX(audioMaster,1,0){
    instances++;

  if (audioMaster){
    setNumInputs(0);
    setNumOutputs(2);
    setUniqueID('ZASF');
    canProcessReplacing();
//    hasVu(false);
//    hasClip(false);

    isSynth(true);

    programsAreChunks(true);
    
  };


    SAMPLE_RATE=config.cfg.SampleRate;	
    SOUND_BUFFER_SIZE=config.cfg.SoundBufferSize;
    OSCIL_SIZE=config.cfg.OscilSize;
    swaplr=config.cfg.SwapStereo;
    this->Pexitprogram=0;
    
    this->vmaster=new Master();
    this->vmaster->swaplr=swaplr;
    

//    FILE *a=fopen("aaaa0","a");
//    fprintf(a,"%lx %lx %lx\n",this,this->ui,this->ui->masterwindow);
//    fflush(a);fclose(a);

    pthread_create(&this->thr,NULL,thread,this);

//    suspend();

};



VSTSynth::~VSTSynth(){
    this->Pexitprogram=1;
    
    Sleep(200);//wait the thread to finish
    
//    pthread_mutex_lock(&vmaster->mutex);


    delete(this->vmaster); 

    instances--;
};

long VSTSynth::processEvents(VstEvents *events){
    for (int i=0;i<events->numEvents;i++){

      //debug stuff
//      FILE *a=fopen("events","a");
//      fprintf(a,"%lx\n",events->events[i]->type);
//      fflush(a);fclose(a);

	if ((events->events[i])->type != kVstMidiType) continue;
	VstMidiEvent *ev= (VstMidiEvent*) events->events[i];
	unsigned char *data= (unsigned char *)ev->midiData;
	int status=data[0]/16;
	int cmdchan=data[0]&0x0f;
	int cntl;	

	pthread_mutex_lock(&vmaster->mutex);
	switch(status){
	    case 0x8:vmaster->NoteOff(cmdchan,data[1]&0x7f);
		 break;
	    case 0x9:if (data[2]==0) vmaster->NoteOff(cmdchan,data[1]&0x7f);
		      else vmaster->NoteOn(cmdchan,data[1]&0x7f,data[2]&0x7f);
		 break;
	    case 0xB: cntl=Midi->getcontroller(data[1]&0x7f);
		      vmaster->SetController(cmdchan,cntl,data[2]&0x7f);
		 break;
	    case 0xE: vmaster->SetController(cmdchan,C_pitchwheel,data[1]+data[2]*(long int) 128-8192);
		 break;
	};
	pthread_mutex_unlock(&vmaster->mutex);
	
    };

return(1);
};

long VSTSynth::getChunk(void** data,bool isPreset){
    int size=0;
    size=vmaster->getalldata((char **)data);
    return((long)size);
};

long VSTSynth::setChunk(void *data,long size,bool isPreset){
    vmaster->putalldata((char*)data,size);
    return(0);
};
#endif

