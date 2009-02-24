/*
  ZynAddSubFX - a software synthesizer
 
  XMLwrapper.C - XML wrapper
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

#include "XMLwrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#include "../globals.h"
#include "Util.h"

int xml_k=0;
char tabs[STACKSIZE+2];

const char *XMLwrapper_whitespace_callback(mxml_node_t *node,int where){
    const char *name=node->value.element.name;

    if ((where==MXML_WS_BEFORE_OPEN)&&(!strcmp(name,"?xml"))) return(NULL);
    if ((where==MXML_WS_BEFORE_CLOSE)&&(!strcmp(name,"string"))) return(NULL);

    if ((where==MXML_WS_BEFORE_OPEN)||(where==MXML_WS_BEFORE_CLOSE)) {
/*	const char *tmp=node->value.element.name;
	if (tmp!=NULL) {
	    if ((strstr(tmp,"par")!=tmp)&&(strstr(tmp,"string")!=tmp)) {
		printf("%s ",tmp);
		if (where==MXML_WS_BEFORE_OPEN) xml_k++;
		if (where==MXML_WS_BEFORE_CLOSE) xml_k--;
		if (xml_k>=STACKSIZE) xml_k=STACKSIZE-1;
		if (xml_k<0) xml_k=0;
		printf("%d\n",xml_k);
		printf("\n");
	    };
	    
	};
	int i=0;
	for (i=1;i<xml_k;i++) tabs[i]='\t';
	tabs[0]='\n';tabs[i+1]='\0';
	if (where==MXML_WS_BEFORE_OPEN) return(tabs);
	    else return("\n");
*/	
	return("\n");
    };
    
    return(0);
};


XMLwrapper::XMLwrapper(){
    ZERO(&parentstack,(int)sizeof(parentstack));
    ZERO(&values,(int)sizeof(values));

    minimal=true;
    stackpos=0;

    information.PADsynth_used=false;
    
    tree=mxmlNewElement(MXML_NO_PARENT,"?xml version=\"1.0\" encoding=\"UTF-8\"?");
/*  for mxml 2.1 (and older)
    tree=mxmlNewElement(MXML_NO_PARENT,"?xml"); 
    mxmlElementSetAttr(tree,"version","1.0");
    mxmlElementSetAttr(tree,"encoding","UTF-8");
*/
    
    mxml_node_t *doctype=mxmlNewElement(tree,"!DOCTYPE");
    mxmlElementSetAttr(doctype,"ZynAddSubFX-data",NULL);

    node=root=mxmlNewElement(tree,"ZynAddSubFX-data");
        
    mxmlElementSetAttr(root,"version-major","1");
    mxmlElementSetAttr(root,"version-minor","1");
    mxmlElementSetAttr(root,"ZynAddSubFX-author","Nasca Octavian Paul");

    //make the empty branch that will contain the information parameters
    info=addparams0("INFORMATION");
    
    //save zynaddsubfx specifications
    beginbranch("BASE_PARAMETERS");
	addpar("max_midi_parts",NUM_MIDI_PARTS);
	addpar("max_kit_items_per_instrument",NUM_KIT_ITEMS);

	addpar("max_system_effects",NUM_SYS_EFX);
	addpar("max_insertion_effects",NUM_INS_EFX);
	addpar("max_instrument_effects",NUM_PART_EFX);

	addpar("max_addsynth_voices",NUM_VOICES);
    endbranch();

};

XMLwrapper::~XMLwrapper(){
    if (tree!=NULL) mxmlDelete(tree);
};

bool XMLwrapper::checkfileinformation(const char *filename){
    stackpos=0;
    ZERO(&parentstack,(int)sizeof(parentstack));
    information.PADsynth_used=false;

    if (tree!=NULL) mxmlDelete(tree);tree=NULL;
    char *xmldata=doloadfile(filename);
    if (xmldata==NULL) return(-1);//the file could not be loaded or uncompressed


    char *start=strstr(xmldata,"<INFORMATION>");
    char *end=strstr(xmldata,"</INFORMATION>");

    if ((start==NULL)||(end==NULL)||(start>end)) {
	delete []xmldata;
	return(false);
    };
    end+=strlen("</INFORMATION>");
    end[0]='\0';    
    
    tree=mxmlNewElement(MXML_NO_PARENT,"?xml");
    node=root=mxmlLoadString(tree,xmldata,MXML_OPAQUE_CALLBACK);
    if (root==NULL) {
	delete []xmldata;
	mxmlDelete(tree);
	node=root=tree=NULL;
	return(false);
    };

    root=mxmlFindElement(tree,tree,"INFORMATION",NULL,NULL,MXML_DESCEND);
    push(root);

    if (root==NULL){
	delete []xmldata;
	mxmlDelete(tree);
	node=root=tree=NULL;
	return(false);
    };
    
    information.PADsynth_used=getparbool("PADsynth_used",false);

    exitbranch();
    if (tree!=NULL) mxmlDelete(tree);
    delete []xmldata;
    node=root=tree=NULL;

    return(true);
};


/* SAVE XML members */

int XMLwrapper::saveXMLfile(const char *filename){
    char *xmldata=getXMLdata();
    if (xmldata==NULL) return(-2);

    int compression=config.cfg.GzipCompression;
    
    int fnsize=strlen(filename)+100;
    char *filenamenew=new char [fnsize];
    snprintf(filenamenew,fnsize,"%s",filename);
    
    int result=dosavefile(filenamenew,compression,xmldata);
    
    delete []filenamenew;
    free(xmldata);    
    return(result);
};

char *XMLwrapper::getXMLdata(){
    xml_k=0;
    ZERO(tabs,STACKSIZE+2);
    
    mxml_node_t *oldnode=node;
    
    node=info;
    //Info storing
    addparbool("PADsynth_used",information.PADsynth_used);
    
    node=oldnode;
    char *xmldata=mxmlSaveAllocString(tree,XMLwrapper_whitespace_callback);

    return(xmldata);
};


int XMLwrapper::dosavefile(const char *filename,int compression,const char *xmldata){
    if (compression==0){
	FILE *file;
	file=fopen(filename,"w");
	if (file==NULL) return(-1);
	fputs(xmldata,file);
	fclose(file);
    } else {
	if (compression>9) compression=9;
	if (compression<1) compression=1;
	char options[10];
	snprintf(options,10,"wb%d",compression);

	gzFile gzfile;
	gzfile=gzopen(filename,options);
	if (gzfile==NULL) return(-1);
	gzputs(gzfile,xmldata);
	gzclose(gzfile);
    };
    
    return(0);
};



void XMLwrapper::addpar(const char *name,int val){
    addparams2("par","name",name,"value",int2str(val));
};

void XMLwrapper::addparreal(const char *name,REALTYPE val){
    addparams2("par_real","name",name,"value",real2str(val));
};

void XMLwrapper::addparbool(const char *name,int val){
    if (val!=0) addparams2("par_bool","name",name,"value","yes");
	else addparams2("par_bool","name",name,"value","no");
};

void XMLwrapper::addparstr(const char *name,const char *val){
    mxml_node_t *element=mxmlNewElement(node,"string");
    mxmlElementSetAttr(element,"name",name);
    mxmlNewText(element,0,val);
};


void XMLwrapper::beginbranch(const char *name){
    push(node);
    node=addparams0(name);
};

void XMLwrapper::beginbranch(const char *name,int id){
    push(node);
    node=addparams1(name,"id",int2str(id));
};

void XMLwrapper::endbranch(){
    node=pop();
};



/* LOAD XML members */

int XMLwrapper::loadXMLfile(const char *filename){
    if (tree!=NULL) mxmlDelete(tree);
    tree=NULL;

    ZERO(&parentstack,(int)sizeof(parentstack));
    ZERO(&values,(int)sizeof(values));

    stackpos=0;

    const char *xmldata=doloadfile(filename);    
    if (xmldata==NULL) return(-1);//the file could not be loaded or uncompressed
    
    root=tree=mxmlLoadString(NULL,xmldata,MXML_OPAQUE_CALLBACK);

    delete []xmldata;

    if (tree==NULL) return(-2);//this is not XML
    
    
    node=root=mxmlFindElement(tree,tree,"ZynAddSubFX-data",NULL,NULL,MXML_DESCEND);
    if (root==NULL) return(-3);//the XML doesnt embbed zynaddsubfx data
    push(root);

    values.xml_version.major=str2int(mxmlElementGetAttr(root,"version-major"));
    values.xml_version.minor=str2int(mxmlElementGetAttr(root,"version-minor"));

    return(0);
};


char *XMLwrapper::doloadfile(const char *filename){
    char *xmldata=NULL;
    int filesize=-1;
    
    //try get filesize as gzip data (first)
    gzFile gzfile=gzopen(filename,"rb");
    if (gzfile!=NULL){//this is a gzip file 
	// first check it's size
	while(!gzeof(gzfile)) {
	    gzseek (gzfile,1024*1024,SEEK_CUR);
	    if (gztell(gzfile)>10000000) {
		gzclose(gzfile);
		goto notgzip;//the file is too big
	    };
	};
	filesize=gztell(gzfile);

	//rewind the file and load the data
	xmldata=new char[filesize+1];
	ZERO(xmldata,filesize+1);

	gzrewind(gzfile);
	gzread(gzfile,xmldata,filesize);
	
	gzclose(gzfile);
	return (xmldata);
    } else {//this is not a gzip file
	notgzip:    
	FILE *file=fopen(filename,"rb");
	if (file==NULL) return(NULL);
	fseek(file,0,SEEK_END);
	filesize=ftell(file);

	xmldata=new char [filesize+1];
	ZERO(xmldata,filesize+1);
	
	rewind(file);
	fread(xmldata,filesize,1,file);
	
	fclose(file);
	return(xmldata);
    }; 
};

bool XMLwrapper::putXMLdata(const char *xmldata){
    if (tree!=NULL) mxmlDelete(tree);
    tree=NULL;

    ZERO(&parentstack,(int)sizeof(parentstack));
    ZERO(&values,(int)sizeof(values));

    stackpos=0;

    if (xmldata==NULL) return (false);
    
    root=tree=mxmlLoadString(NULL,xmldata,MXML_OPAQUE_CALLBACK);

    if (tree==NULL) return(false);
    
    node=root=mxmlFindElement(tree,tree,"ZynAddSubFX-data",NULL,NULL,MXML_DESCEND);
    if (root==NULL) return (false);;
    push(root);

    return(true);
};



int XMLwrapper::enterbranch(const char *name){
    node=mxmlFindElement(peek(),peek(),name,NULL,NULL,MXML_DESCEND_FIRST);
    if (node==NULL) return(0);

    push(node);
    return(1);
};

int XMLwrapper::enterbranch(const char *name,int id){
    snprintf(tmpstr,TMPSTR_SIZE,"%d",id);
    node=mxmlFindElement(peek(),peek(),name,"id",tmpstr,MXML_DESCEND_FIRST);
    if (node==NULL) return(0);

    push(node);
    return(1);
};


void XMLwrapper::exitbranch(){
    pop();
};


int XMLwrapper::getbranchid(int min, int max){
    int id=str2int(mxmlElementGetAttr(node,"id"));
    if ((min==0)&&(max==0)) return(id);
    
    if (id<min) id=min;
	else if (id>max) id=max;

    return(id);
};

int XMLwrapper::getpar(const char *name,int defaultpar,int min,int max){
    node=mxmlFindElement(peek(),peek(),"par","name",name,MXML_DESCEND_FIRST);
    if (node==NULL) return(defaultpar);

    const char *strval=mxmlElementGetAttr(node,"value");
    if (strval==NULL) return(defaultpar);
    
    int val=str2int(strval);
    if (val<min) val=min;
	else if (val>max) val=max;
    
    return(val);
};

int XMLwrapper::getpar127(const char *name,int defaultpar){
    return(getpar(name,defaultpar,0,127));
};

int XMLwrapper::getparbool(const char *name,int defaultpar){
    node=mxmlFindElement(peek(),peek(),"par_bool","name",name,MXML_DESCEND_FIRST);
    if (node==NULL) return(defaultpar);

    const char *strval=mxmlElementGetAttr(node,"value");
    if (strval==NULL) return(defaultpar);
    
    if ((strval[0]=='Y')||(strval[0]=='y')) return(1);
	else return(0);
};

void XMLwrapper::getparstr(const char *name,char *par,int maxstrlen){
    ZERO(par,maxstrlen);
    node=mxmlFindElement(peek(),peek(),"string","name",name,MXML_DESCEND_FIRST);
    
    if (node==NULL) return;
    if (node->child==NULL) return;
    if (node->child->type!=MXML_OPAQUE) return;
    
    snprintf(par,maxstrlen,"%s",node->child->value.element.name);
    
};

REALTYPE XMLwrapper::getparreal(const char *name,REALTYPE defaultpar){
    node=mxmlFindElement(peek(),peek(),"par_real","name",name,MXML_DESCEND_FIRST);
    if (node==NULL) return(defaultpar);

    const char *strval=mxmlElementGetAttr(node,"value");
    if (strval==NULL) return(defaultpar);
    
    return(str2real(strval));
};

REALTYPE XMLwrapper::getparreal(const char *name,REALTYPE defaultpar,REALTYPE min,REALTYPE max){
    REALTYPE result=getparreal(name,defaultpar);
    
    if (result<min) result=min;
	else if (result>max) result=max;
    return(result);
};


/** Private members **/

char *XMLwrapper::int2str(int x){
    snprintf(tmpstr,TMPSTR_SIZE,"%d",x);
    return(tmpstr);
};

char *XMLwrapper::real2str(REALTYPE x){
    snprintf(tmpstr,TMPSTR_SIZE,"%g",x);
    return(tmpstr);
};

int XMLwrapper::str2int(const char *str){
    if (str==NULL) return(0);
    int result=strtol(str,NULL,10);
    return(result);
};

REALTYPE XMLwrapper::str2real(const char *str){
    if (str==NULL) return(0.0);
    REALTYPE result=strtod(str,NULL);
    return(result);
};


mxml_node_t *XMLwrapper::addparams0(const char *name){
    mxml_node_t *element=mxmlNewElement(node,name);
    return(element);
};

mxml_node_t *XMLwrapper::addparams1(const char *name,const char *par1,const char *val1){
    mxml_node_t *element=mxmlNewElement(node,name);
    mxmlElementSetAttr(element,par1,val1);
    return(element);
};

mxml_node_t *XMLwrapper::addparams2(const char *name,const char *par1,const char *val1,const char *par2, const char *val2){
    mxml_node_t *element=mxmlNewElement(node,name);
    mxmlElementSetAttr(element,par1,val1);
    mxmlElementSetAttr(element,par2,val2);
    return(element);
};




void XMLwrapper::push(mxml_node_t *node){
    if (stackpos>=STACKSIZE-1) {
	printf("BUG!: XMLwrapper::push() - full parentstack\n");
	return;
    };
    stackpos++;
    parentstack[stackpos]=node;
    
//    printf("push %d - %s\n",stackpos,node->value.element.name);
    
};
mxml_node_t *XMLwrapper::pop(){
    if (stackpos<=0) {
	printf("BUG!: XMLwrapper::pop() - empty parentstack\n");
	return (root);
    };
    mxml_node_t *node=parentstack[stackpos];
    parentstack[stackpos]=NULL;

//    printf("pop %d - %s\n",stackpos,node->value.element.name);

    stackpos--;
    return(node);
};

mxml_node_t *XMLwrapper::peek(){
    if (stackpos<=0) {
	printf("BUG!: XMLwrapper::peek() - empty parentstack\n");
	return (root);
    };
    return(parentstack[stackpos]);
};



