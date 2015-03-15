/**
 * Extract Presets from realtime data
 */

#include "../Params/PresetsStore.h"

#include "../Misc/Master.h"
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"
#include "../Effects/EffectMgr.h"
#include "../Synth/OscilGen.h"
#include "../Synth/Resonance.h"
#include "../Params/ADnoteParameters.h"
#include "../Params/EnvelopeParams.h"
#include "../Params/FilterParams.h"
#include "../Params/LFOParams.h"
#include "../Params/PADnoteParameters.h"
#include "../Params/Presets.h"
#include "../Params/PresetsArray.h"
#include "../Params/PresetsStore.h"
#include "../Params/SUBnoteParameters.h"
#include "../Misc/MiddleWare.h"
#include "PresetExtractor.h"
#include <string>
using std::string;


/*****************************************************************************
 *                     Implementation Methods                                *
 *****************************************************************************/
static string clip;

class Capture:public rtosc::RtData
{
    public:
        Capture(void *obj_)
        {
            matches = 0;
            memset(locbuf, 0, sizeof(locbuf));
            loc      = locbuf;
            loc_size = sizeof(locbuf);
            obj      = obj_;
        }

        virtual void reply(const char *path, const char *args, ...)
        {
            printf("reply(%p)(%s)(%s)...\n", msgbuf, path, args);
            //printf("size is %d\n", sizeof(msgbuf));
            va_list va;
            va_start(va,args);
            char *buffer = msgbuf;
            rtosc_vmessage(buffer,sizeof(msgbuf),path,args,va);
            va_end(va);
        }
        char msgbuf[1024];
        char locbuf[1024];
};

template <class T>
T capture(Master *m, std::string url);

template <>
void *capture(Master *m, std::string url)
{
    Capture c(m);
    char query[1024];
    rtosc_message(query, 1024, url.c_str(), "");
    Master::ports.dispatch(query+1,c);
    if(rtosc_message_length(c.msgbuf, sizeof(c.msgbuf))) {
        if(rtosc_type(c.msgbuf, 0) == 'b' &&
                rtosc_argument(c.msgbuf, 0).b.len == sizeof(void*))
            return *(void**)rtosc_argument(c.msgbuf,0).b.data;
    }

    return NULL;
}

template<class T>
std::string doCopy(MiddleWare &mw, string url)
{
    XMLwrapper xml;
    mw.doReadOnlyOp([&xml, url,&mw](){
        Master *m = mw.spawnMaster();
        //Get the pointer
        T *t = (T*)capture<void*>(m, url+"self");
        //Extract Via mxml
        t->add2XML(&xml);
    });

    return xml.getXMLdata();
}

template<class T, typename... Ts>
void doPaste(MiddleWare &mw, string url, string data, Ts&&... args)
{
    (void) data;
    if(clip.length() < 20)
        return;

    //Generate a new object
    T *t = new T(std::forward<Ts>(args)...);
    XMLwrapper xml;
    xml.putXMLdata(clip.data());
    t->getfromXML(&xml);

    //Send the pointer
    string path = url+"paste";
    char buffer[1024];
    rtosc_message(buffer, 1024, path.c_str(), "b", sizeof(void*), &t);
    if(!Master::ports.apropos(path.c_str()))
        fprintf(stderr, "Warning: Missing Paste URL: '%s'\n", path.c_str());
    printf("Sending info to '%s'\n", buffer);
    mw.transmitMsg(buffer);

    //Let the pointer be reclaimed later
}

/*
 * Dispatch to class specific operators
 *
 * Oscilgen and PADnoteParameters have mixed RT/non-RT parameters and require
 * extra handling.
 * See MiddleWare.cpp for these specifics
 */
void doClassPaste(std::string type, MiddleWare &mw, string url, string data)
{
    if(type == "EnvelopeParams")
        doPaste<EnvelopeParams>(mw, url, data);
    else if(type == "LFOParams")
        doPaste<LFOParams>(mw, url, data);
    else if(type == "FilterParams")
        doPaste<FilterParams>(mw, url, data);
    else if(type == "ADnoteParameters")
        doPaste<ADnoteParameters>(mw, url, data, (FFTwrapper*)NULL);
    else if(type == "PADnoteParameters")
        doPaste<PADnoteParameters>(mw, url, data, (FFTwrapper*)NULL);
    else if(type == "SUBnoteParameters")
        doPaste<SUBnoteParameters>(mw, url, data);
    else if(type == "OscilGen")
        doPaste<OscilGen>(mw, url, data, (FFTwrapper*)NULL, (Resonance*)NULL);
    else if(type == "Resonance")
        doPaste<Resonance>(mw, url, data);
    else if(type == "EffectMgr")
        doPaste<EffectMgr>(mw, url, data, DummyAlloc, false);
}

std::string doClassCopy(std::string type, MiddleWare &mw, string url)
{
    if(type == "EnvelopeParams")
        return doCopy<EnvelopeParams>(mw, url);
    else if(type == "LFOParams")
        return doCopy<LFOParams>(mw, url);
    else if(type == "FilterParams")
        return doCopy<FilterParams>(mw, url);
    else if(type == "ADnoteParameters")
        return doCopy<ADnoteParameters>(mw, url);
    else if(type == "PADnoteParameters")
        return doCopy<PADnoteParameters>(mw, url);
    else if(type == "SUBnoteParameters")
        return doCopy<SUBnoteParameters>(mw, url);
    else if(type == "OscilGen")
        return doCopy<OscilGen>(mw, url);
    else if(type == "Resonance")
        return doCopy<Resonance>(mw, url);
    else if(type == "EffectMgr")
        doCopy<EffectMgr>(mw, url);
    return "UNDEF";
}

std::string getUrlType(std::string url)
{
    assert(!url.empty());
    printf("Searching for '%s'\n", (url+"self").c_str());
    auto self = Master::ports.apropos((url+"self").c_str());
    if(!self)
        fprintf(stderr, "Warning: URL Metadata Not Found For '%s'\n", url.c_str());

    if(self)
        return self->meta()["class"];
    else
        return "";
}

void doClassArrayPaste(std::string type, MiddleWare &mw, string url, string data, int idx)
{
    if(type == "ADnoteVoiceParam")
        ;
    else if(type == "FilterParams")
        ;
}

std::string doClassArrayCopy(std::string type, MiddleWare &mw, string url, int idx)
{
    if(type == "ADnoteVoiceParam")
        return "UNDEF";
    else if(type == "FilterParams")
        return "UNDEF";
    return "UNDEF";
}


/*****************************************************************************
 *                            API Stubs                                      *
 *****************************************************************************/

Clipboard clipboardCopy(MiddleWare &mw, string url)
{
    //Identify The Self Type of the Object
    string type = getUrlType(url);
    printf("Copying a '%s' object", type.c_str());

    //Copy The Object
    string data = doClassCopy(type, mw, url);
    printf("Object Information '%s'\n", data.c_str());

    return {type, data};
}

void clipBoardPaste(const char *url, Clipboard clip)
{
    (void) url;
    (void) clip;
}

MiddleWare *middlewarepointer;
void presetCopy(std::string url, std::string name)
{
    (void) name;
    clip = doClassCopy(getUrlType(url), *middlewarepointer, url);
    printf("PresetCopy()\n");
    printf("clip = ``%s''\n", clip.c_str());
}
void presetPaste(std::string url, std::string name)
{
    (void) name;
    doClassPaste(getUrlType(url), *middlewarepointer, url, clip);
    printf("PresetPaste()\n");
}
void presetPaste(std::string url, int)
{
    doClassPaste(getUrlType(url), *middlewarepointer, url, clip);
    printf("PresetPaste()\n");
}
void presetDelete(int)
{
    printf("PresetDelete()\n");
}
void presetRescan()
{
    printf("PresetRescan()\n");
}
std::string presetClipboardType()
{
    printf("PresetClipboardType()\n");
    return "dummy";
}
bool presetCheckClipboardType()
{
    printf("PresetCheckClipboardType()\n");
    return true;
}
