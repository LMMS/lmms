/*
  ZynAddSubFX - a software synthesizer

  Bank.h - Instrument Bank
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

#include "Bank.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "Config.h"

#define INSTRUMENT_EXTENSION ".xiz"

//if this file exists into a directory, this make the directory to be considered as a bank, even if it not contains a instrument file
#define FORCE_BANK_DIR_FILE ".bankdir"

Bank::Bank()
{


    ZERO(defaultinsname,PART_MAX_NAME_LEN);
    snprintf(defaultinsname,PART_MAX_NAME_LEN,"%s"," ");

    for (int i=0;i<BANK_SIZE;i++) {
        ins[i].used=false;
        ins[i].filename=NULL;
        ins[i].info.PADsynth_used=false;
    };
    dirname=NULL;
    clearbank();



    for (int i=0;i<MAX_NUM_BANKS;i++) {
        banks[i].dir=NULL;
        banks[i].name=NULL;
    };

    bankfiletitle=dirname;

    loadbank(config.cfg.currentBankDir);

};

Bank::~Bank()
{
    for (int i=0;i<MAX_NUM_BANKS;i++) {
        if (banks[i].dir!=NULL) delete []banks[i].dir;
        if (banks[i].name!=NULL) delete []banks[i].name;
    };

    clearbank();
};

/*
 * Get the name of an instrument from the bank
 */
char *Bank::getname (unsigned int ninstrument)
{
    if (emptyslot(ninstrument)) return (defaultinsname);
    return (ins[ninstrument].name);
};

/*
 * Get the numbered name of an instrument from the bank
 */
char *Bank::getnamenumbered (unsigned int ninstrument)
{
    if (emptyslot(ninstrument)) return (defaultinsname);
    snprintf(tmpinsname[ninstrument],PART_MAX_NAME_LEN+15,"%d. %s",ninstrument+1,getname(ninstrument));
    return(tmpinsname[ninstrument]);
};

/*
 * Changes the name of an instrument (and the filename)
 */
void Bank::setname(unsigned int ninstrument,const char *newname,int newslot)
{
    if (emptyslot(ninstrument)) return;

    char newfilename[1000+1],tmpfilename[100+1];

    ZERO(newfilename,1001);
    ZERO(tmpfilename,101);
    if (newslot>=0) snprintf(tmpfilename,100,"%4d-%s",newslot+1,newname);
    else snprintf(tmpfilename,100,"%4d-%s",ninstrument+1,newname);

    //add the zeroes at the start of filename
    for (int i=0;i<4;i++) if (tmpfilename[i]==' ') tmpfilename[i]='0';

    //make the filenames legal
    for (int i=0;i<(int) strlen(tmpfilename);i++) {
        char c=tmpfilename[i];
        if ((c>='0')&&(c<='9')) continue;
        if ((c>='A')&&(c<='Z')) continue;
        if ((c>='a')&&(c<='z')) continue;
        if ((c=='-')||(c==' ')) continue;

        tmpfilename[i]='_';
    };

    snprintf(newfilename,1000,"%s/%s.xiz",dirname,tmpfilename);

//    printf("rename %s -> %s\n",ins[ninstrument].filename,newfilename);//////////////

    rename(ins[ninstrument].filename,newfilename);
    if (ins[ninstrument].filename) delete []ins[ninstrument].filename;
    ins[ninstrument].filename=new char[strlen(newfilename)+5];
    snprintf(ins[ninstrument].filename,strlen(newfilename)+1,"%s",newfilename);
    snprintf(ins[ninstrument].name,PART_MAX_NAME_LEN,"%s",&tmpfilename[5]);

};

/*
 * Check if there is no instrument on a slot from the bank
 */
int Bank::emptyslot(unsigned int ninstrument)
{
    if (ninstrument>=BANK_SIZE) return (1);
    if (ins[ninstrument].filename==NULL) return(1);

    if (ins[ninstrument].used) return (0);
    else return(1);
};

/*
 * Removes the instrument from the bank
 */
void Bank::clearslot(unsigned int ninstrument)
{
    if (emptyslot(ninstrument)) return;

//    printf("remove  %s  \n",ins[ninstrument].filename);////////////////////////


    remove(ins[ninstrument].filename);
    deletefrombank(ninstrument);
};

/*
 * Save the instrument to a slot
 */
void Bank::savetoslot(unsigned int ninstrument,Part *part)
{
    clearslot(ninstrument);

    const int maxfilename=200;
    char tmpfilename[maxfilename+20];
    ZERO(tmpfilename,maxfilename+20);

    snprintf(tmpfilename,maxfilename,"%4d-%s",ninstrument+1,(char *)part->Pname);

    //add the zeroes at the start of filename
    for (int i=0;i<4;i++) if (tmpfilename[i]==' ') tmpfilename[i]='0';

    //make the filenames legal
    for (int i=0;i<(int)strlen(tmpfilename);i++) {
        char c=tmpfilename[i];
        if ((c>='0')&&(c<='9')) continue;
        if ((c>='A')&&(c<='Z')) continue;
        if ((c>='a')&&(c<='z')) continue;
        if ((c=='-')||(c==' ')) continue;

        tmpfilename[i]='_';
    };

    strncat(tmpfilename,".xiz",maxfilename+10);

    int fnsize=strlen(dirname)+strlen(tmpfilename)+10;
    char *filename=new char[fnsize+4];
    ZERO(filename,fnsize+2);

    snprintf(filename,fnsize,"%s/%s",dirname,tmpfilename);

    remove(filename);
    part->saveXML(filename);
    addtobank(ninstrument,tmpfilename,(char *) part->Pname);

    delete[]filename;
};

/*
 * Loads the instrument from the bank
 */
void Bank::loadfromslot(unsigned int ninstrument,Part *part)
{
    if (emptyslot(ninstrument)) return;

    part->defaultsinstrument();

//    printf("load:  %s\n",ins[ninstrument].filename);

    part->loadXMLinstrument(ins[ninstrument].filename);

};


/*
 * Makes current a bank directory
 */
int Bank::loadbank(const char *bankdirname)
{
    DIR *dir=opendir(bankdirname);
    clearbank();

    if (dir==NULL) return(-1);

    if (dirname!=NULL) delete[]dirname;
    dirname=new char[strlen(bankdirname)+1];
    snprintf(dirname,strlen(bankdirname)+1,"%s",bankdirname);

    bankfiletitle=dirname;

    // printf("loadbank %s/\n",bankdirname);
    struct dirent *fn;

    while ((fn=readdir(dir))) {
        const char *filename= fn->d_name;

        //sa verific daca e si extensia dorita
        if (strstr(filename,INSTRUMENT_EXTENSION)==NULL) continue;

        //verify if the name is like this NNNN-name (where N is a digit)
        int no=0;
        unsigned int startname=0;

        for (unsigned int i=0;i<4;i++) {
            if (strlen(filename)<=i) break;

            if ((filename[i]>='0')&&(filename[i]<='9')) {
                no=no*10+(filename[i]-'0');
                startname++;
            };
        };


        if ((startname+1)<strlen(filename)) startname++;//to take out the "-"

        char name[PART_MAX_NAME_LEN+1];
        ZERO(name,PART_MAX_NAME_LEN+1);
        snprintf(name,PART_MAX_NAME_LEN,"%s",filename);

        //remove the file extension
        for (int i=strlen(name)-1;i>=2;i--) {
            if (name[i]=='.') {
                name[i]='\0';
                break;
            };
        };

        if (no!=0) {//the instrument position in the bank is found
            addtobank(no-1,filename,&name[startname]);
        } else {
            addtobank(-1,filename,name);
        };

    };


    closedir(dir);

    if (dirname!=NULL) {
        sprintf(config.cfg.currentBankDir,"%s",dirname);
    };

    return(0);
};

/*
 * Makes a new bank, put it on a file and makes it current bank
 */
int Bank::newbank(const char *newbankdirname)
{
    int result;
    char tmpfilename[MAX_STRING_SIZE];
    char bankdir[MAX_STRING_SIZE];
    snprintf(bankdir,MAX_STRING_SIZE,"%s",config.cfg.bankRootDirList[0]);

    if (((bankdir[strlen(bankdir)-1])!='/')&&((bankdir[strlen(bankdir)-1])!='\\')) {
        strncat(bankdir,"/",MAX_STRING_SIZE);
    };
    strncat(bankdir,newbankdirname,MAX_STRING_SIZE);
#ifdef OS_WINDOWS
    result=mkdir(bankdir);
#else
    result=mkdir(bankdir,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
    if (result<0) return(-1);

    snprintf(tmpfilename,MAX_STRING_SIZE,"%s/%s",bankdir,FORCE_BANK_DIR_FILE);
//    printf("%s\n",tmpfilename);
    FILE *tmpfile=fopen(tmpfilename,"w+");
    fclose(tmpfile);

    return(loadbank(bankdir));
};

/*
 * Check if the bank is locked (i.e. the file opened was readonly)
 */
int Bank::locked()
{
    return(dirname==NULL);
};

/*
 * Swaps a slot with another
 */
void Bank::swapslot(unsigned int n1, unsigned int n2)
{
    if ((n1==n2)||(locked())) return;
    if (emptyslot(n1)&&(emptyslot(n2))) return;
    if (emptyslot(n1)) {//change n1 to n2 in order to make
        int tmp=n2;
        n2=n1;
        n1=tmp;
    };

    if (emptyslot(n2)) {//this is just a movement from slot1 to slot2
        setname(n1,getname(n1),n2);
        ins[n2]=ins[n1];
        ins[n1].used=false;
        ins[n1].name[0]='\0';
        ins[n1].filename=NULL;
        ins[n1].info.PADsynth_used=0;
    } else {//if both slots are used
        if (strcmp(ins[n1].name,ins[n2].name)==0) {//change the name of the second instrument if the name are equal
            strncat(ins[n2].name,"2",PART_MAX_NAME_LEN);
        };
        setname(n1,getname(n1),n2);
        setname(n2,getname(n2),n1);
        ins_t tmp;
        tmp.used=true;
        strcpy(tmp.name,ins[n2].name);
        char *tmpfilename=ins[n2].filename;
        bool padsynth_used=ins[n2].info.PADsynth_used;

        ins[n2]=ins[n1];
        strcpy(ins[n1].name,tmp.name);
        ins[n1].filename=tmpfilename;
        ins[n1].info.PADsynth_used=padsynth_used;
    };

};


//a helper function that compares 2 banks[] arrays
int Bank_compar(const void *a,const void *b)
{
    struct Bank::bankstruct *bank1= (Bank::bankstruct *)a;
    struct Bank::bankstruct *bank2= (Bank::bankstruct *)b;
    if (((bank1->name)==NULL)||((bank2->name)==NULL)) return(0);

    int result=strcasecmp(bank1->name,bank2->name);
    return(result<0);
};


/*
 * Re-scan for directories containing instrument banks
 */

void Bank::rescanforbanks()
{
    for (int i=0;i<MAX_NUM_BANKS;i++) {
        if (banks[i].dir!=NULL) delete []banks[i].dir;
        if (banks[i].name!=NULL) delete []banks[i].name;
        banks[i].dir=NULL;
        banks[i].name=NULL;
    };

    for (int i=0;i<MAX_BANK_ROOT_DIRS;i++) if (config.cfg.bankRootDirList[i]!=NULL) scanrootdir(config.cfg.bankRootDirList[i]);

    //sort the banks
    for (int j=0;j<MAX_NUM_BANKS-1;j++) {
        for (int i=j+1;i<MAX_NUM_BANKS;i++) {
            if (Bank_compar(&banks[i],&banks[j])) {
                char *tmpname=banks[i].name;
                char *tmpdir=banks[i].dir;

                banks[i].name=banks[j].name;
                banks[i].dir=banks[j].dir;

                banks[j].name=tmpname;
                banks[j].dir=tmpdir;

            };
        };
    };

    //remove duplicate bank names
    int dupl=0;
    for (int j=0;j<MAX_NUM_BANKS-1;j++) {
        for (int i=j+1;i<MAX_NUM_BANKS;i++) {
            if ((banks[i].name==NULL)||(banks[j].name==NULL)) continue;
            if (strcmp(banks[i].name,banks[j].name)==0) {//add a [1] to the first bankname and [n] to others
                char *tmpname=banks[i].name;
                banks[i].name=new char[strlen(tmpname)+100];
                sprintf(banks[i].name,"%s[%d]",tmpname,dupl+2);
                delete[]tmpname;

                if (dupl==0) {
                    char *tmpname=banks[j].name;
                    banks[j].name=new char[strlen(tmpname)+100];
                    sprintf(banks[j].name,"%s[1]",tmpname);
                    delete[]tmpname;
                };

                dupl++;
            } else dupl=0;
        };
    };

};



// private stuff

void Bank::scanrootdir(char *rootdir)
{
//    printf("Scanning root dir:%s\n",rootdir);
    DIR *dir=opendir(rootdir);
    if (dir==NULL) return;

    const int maxdirsize=1000;
    struct {
        char dir[maxdirsize];
        char name[maxdirsize];
    }bank;

    const char *separator="/";
    if (strlen(rootdir)) {
        char tmp=rootdir[strlen(rootdir)-1];
        if ((tmp=='/') || (tmp=='\\')) separator="";
    };

    struct dirent *fn;
    while ((fn=readdir(dir))) {
        const char *dirname=fn->d_name;
        if (dirname[0]=='.') continue;

        snprintf(bank.dir,maxdirsize,"%s%s%s/",rootdir,separator,dirname);
        snprintf(bank.name,maxdirsize,"%s",dirname);
        //find out if the directory contains at least 1 instrument
        bool isbank=false;

        DIR *d=opendir(bank.dir);
        if (d==NULL) continue;

        struct dirent *fname;

        while ((fname=readdir(d))) {
            if ((strstr(fname->d_name,INSTRUMENT_EXTENSION)!=NULL)||
                    (strstr(fname->d_name,FORCE_BANK_DIR_FILE)!=NULL)) {
                isbank=true;
                break;//aici as putea pune in loc de break un update la un counter care imi arata nr. de instrumente din bank
            };
        };

        closedir(d);

        if (isbank) {
            int pos=-1;
            for (int i=1;i<MAX_NUM_BANKS;i++) {	//banks[0] e liber intotdeauna
                if (banks[i].name==NULL) {
                    pos=i;
                    break;
                };
            };

            if (pos>=0) {
                banks[pos].name=new char[maxdirsize];
                banks[pos].dir=new char[maxdirsize];
                snprintf(banks[pos].name,maxdirsize,"%s",bank.name);
                snprintf(banks[pos].dir,maxdirsize,"%s",bank.dir);
            };

        };

    };

    closedir(dir);

};

void Bank::clearbank()
{
    for (int i=0;i<BANK_SIZE;i++) deletefrombank(i);
    if (dirname!=NULL) delete[]dirname;
    bankfiletitle=NULL;
    dirname=NULL;
};

int Bank::addtobank(int pos, const char *filename, const char* name)
{
    if ((pos>=0)&&(pos<BANK_SIZE)) {
        if (ins[pos].used) pos=-1;//force it to find a new free position
    } else if (pos>=BANK_SIZE) pos=-1;


    if (pos<0) {//find a free position
        for (int i=BANK_SIZE-1;i>=0;i--)
            if (!ins[i].used) {
                pos=i;
                break;
            };

    };

    if (pos<0) return (-1);//the bank is full

    // printf("%s   %d\n",filename,pos);

    deletefrombank(pos);

    ins[pos].used=true;
    snprintf(ins[pos].name,PART_MAX_NAME_LEN,"%s",name);

    snprintf(tmpinsname[pos],PART_MAX_NAME_LEN+10," ");

    int len=strlen(filename)+1+strlen(dirname);
    ins[pos].filename=new char[len+2];
    ins[pos].filename[len+1]=0;
    snprintf(ins[pos].filename,len+1,"%s/%s",dirname,filename);

    //see if PADsynth is used
    if (config.cfg.CheckPADsynth) {
        XMLwrapper *xml=new XMLwrapper();
        xml->loadXMLfile(ins[pos].filename);

        ins[pos].info.PADsynth_used=xml->hasPadSynth();
        delete xml;
    } else ins[pos].info.PADsynth_used=false;

    return(0);
};

bool Bank::isPADsynth_used(unsigned int ninstrument)
{
    if (config.cfg.CheckPADsynth==0) return(0);
    else return(ins[ninstrument].info.PADsynth_used);
};


void Bank::deletefrombank(int pos)
{
    if ((pos<0)||(pos>=BANK_SIZE)) return;
    ins[pos].used=false;
    ZERO(ins[pos].name,PART_MAX_NAME_LEN+1);
    if (ins[pos].filename!=NULL) {
        delete []ins[pos].filename;
        ins[pos].filename=NULL;
    };

    ZERO(tmpinsname[pos],PART_MAX_NAME_LEN+20);

};

