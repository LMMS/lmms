#include <math.h>
#include <rtosc/miditable.h>

using namespace rtosc;

#define RTOSC_INVALID_MIDI 255
        
class rtosc::MidiTable_Impl
{
    public:
        MidiTable_Impl(unsigned len, unsigned elms)
            :len(len), elms(elms)
        {
            table = new MidiAddr[elms];
            for(unsigned i=0; i<elms; ++i) {
                table[i].ch   = RTOSC_INVALID_MIDI;
                table[i].ctl  = RTOSC_INVALID_MIDI;
                table[i].path = new char[len];
                table[i].conversion = NULL;
            }
            //TODO initialize all elms
        }
        ~MidiTable_Impl()
        {
            for(unsigned i=0; i<elms; ++i) {
                delete [] table[i].path;
            }
            delete [] table;
        }

        MidiAddr *begin(void) {return table;}
        MidiAddr *end(void) {return table + elms;}

        unsigned len;
        unsigned elms;
        MidiAddr *table;
};

//MidiAddr::MidiAddr(void)
//    :ch(RTOSC_INVALID_MIDI),ctl(RTOSC_INVALID_MIDI)
//{}

static void black_hole3 (const char *, const char *, const char *, int, int)
{}
static void black_hole2(const char *a, const char *b)
{printf("'%s' and '%s'\n", a,b);}
static void black_hole1(const char *a)
{printf("'%s'\n", a);}

#define	MAX_UNHANDLED_PATH 128

MidiTable::MidiTable(const Ports &_dispatch_root)
:dispatch_root(_dispatch_root), unhandled_ch(RTOSC_INVALID_MIDI), unhandled_ctl(RTOSC_INVALID_MIDI),
    error_cb(black_hole2), event_cb(black_hole1), modify_cb(black_hole3)
{
    impl = new MidiTable_Impl(128,128);
    unhandled_path = new char[MAX_UNHANDLED_PATH];
    memset(unhandled_path, 0, MAX_UNHANDLED_PATH);
}

MidiTable::~MidiTable()
{
       delete impl;
       delete [] unhandled_path;
}

bool MidiTable::has(uint8_t ch, uint8_t ctl) const
{
    for(auto e: *impl) {
        if(e.ch == ch && e.ctl == ctl)
            return true;
    }
    return false;
}

MidiAddr *MidiTable::get(uint8_t ch, uint8_t ctl)
{
    for(auto &e: *impl)
        if(e.ch==ch && e.ctl == ctl)
            return &e;
    return NULL;
}

const MidiAddr *MidiTable::get(uint8_t ch, uint8_t ctl) const
{
    for(auto &e:*impl)
        if(e.ch==ch && e.ctl == ctl)
            return &e;
    return NULL;
}

bool MidiTable::mash_port(MidiAddr &e, const Port &port)
{
    const char *args = strchr(port.name, ':');
    if(!args)
        return false;

    //Consider a path to be typed based upon the argument restrictors
    if(strchr(args, 'f')) {
        e.type = 'f';
        e.conversion = port.metadata;
    } else if(strchr(args, 'i'))
        e.type = 'i';
    else if(strchr(args, 'T'))
        e.type = 'T';
    else if(strchr(args, 'c'))
        e.type = 'c';
    else
        return false;
    return true;
}

void MidiTable::addElm(uint8_t ch, uint8_t ctl, const char *path)
{
    const Port *port = dispatch_root.apropos(path);

    if(!port || port->ports) {//missing or directory node
        error_cb("Bad path", path);
        return;
    }

    if(MidiAddr *e = this->get(ch,ctl)) {
        strncpy(e->path,path,impl->len);
        if(!mash_port(*e, *port)) {
            e->ch  = RTOSC_INVALID_MIDI;
            e->ctl = RTOSC_INVALID_MIDI;
            error_cb("Failed to read metadata", path);
        }
        modify_cb("REPLACE", path, e->conversion, (int) ch, (int) ctl);
        return;
    }

    for(MidiAddr &e:*impl) {
        if(e.ch == RTOSC_INVALID_MIDI) {//free spot
            e.ch  = ch;
            e.ctl = ctl;
            strncpy(e.path,path,impl->len);
            if(!mash_port(e, *port)) {
                e.ch  = RTOSC_INVALID_MIDI;
                e.ctl = RTOSC_INVALID_MIDI;
                error_cb("Failed to read metadata", path);
            }
            modify_cb("ADD", path, e.conversion, (int) ch, (int) ctl);
            return;
        }
    }
}

void MidiTable::check_learn(void)
{
    if(unhandled_ctl == RTOSC_INVALID_MIDI || unhandled_path[0] == '\0')
        return;
    addElm(unhandled_ch, unhandled_ctl, unhandled_path);
    unhandled_ch = unhandled_ctl = RTOSC_INVALID_MIDI;
    memset(unhandled_path, 0, MAX_UNHANDLED_PATH);
}

void MidiTable::learn(const char *s)
{
    if(strlen(s) > impl->len) {
        error_cb("String too long", s);
        return;
    }
    clear_entry(s);
    strncpy(unhandled_path, s, MAX_UNHANDLED_PATH);
    unhandled_path[MAX_UNHANDLED_PATH-1] = '\0';
    check_learn();
}

void MidiTable::clear_entry(const char *s)
{
    for(unsigned i=0; i<impl->elms; ++i) {
        if(!strcmp(impl->table[i].path, s)) {
            //Invalidate
            impl->table[i].ch  = RTOSC_INVALID_MIDI;
            impl->table[i].ctl = RTOSC_INVALID_MIDI;
            modify_cb("DEL", s, "", -1, -1);
            break;
        }
    }
}

void MidiTable::process(uint8_t ch, uint8_t ctl, uint8_t val)
{
    const MidiAddr *addr = get(ch,ctl);
    if(!addr) {
        unhandled_ctl = ctl;
        unhandled_ch  = ch;
        check_learn();
        return;
    }

    char buffer[1024];
    switch(addr->type)
    {
        case 'f':
            rtosc_message(buffer, 1024, addr->path,
                    "f", translate(val,addr->conversion));
            break;
        case 'i':
            rtosc_message(buffer, 1024, addr->path,
                    "i", val);
            break;
        case 'T':
            rtosc_message(buffer, 1024, addr->path,
                    (val<64 ? "F" : "T"));
            break;
        case 'c':
            rtosc_message(buffer, 1024, addr->path,
                    "c", val);
    }

    event_cb(buffer);
}

Port MidiTable::learnPort(void)
{
    return Port{"learn:s", "", 0, [this](msg_t m, RtData&){
            this->learn(rtosc_argument(m,0).s);
            }};

}

Port MidiTable::unlearnPort(void)
{
    return Port{"unlearn:s", "", 0, [this](msg_t m, RtData&){
            this->clear_entry(rtosc_argument(m,0).s);
            }};

}

Port MidiTable::registerPort(void)
{
    return Port{"register:iis","", 0, [this](msg_t m,RtData&){
            const char *pos = rtosc_argument(m,2).s;
            while(*pos) putchar(*pos++);
            this->addElm(rtosc_argument(m,0).i,rtosc_argument(m,1).i,rtosc_argument(m,2).s);}};
}

//TODO generalize to an addScalingFunction() system
float MidiTable::translate(uint8_t val, const char *meta_)
{
    //Allow for middle value to be set
    //TODO consider the centered trait for this op
    float x = val!=64.0 ? val/127.0 : 0.5;

    Port::MetaContainer meta(meta_);

    if(!meta["min"] || !meta["max"] || !meta["scale"]) {
        fprintf(stderr, "failed to get properties\n");
        return 0.0f;
    }

    const float min   = atof(meta["min"]);
    const float max   = atof(meta["max"]);
    const char *scale = meta["scale"];

    if(!strcmp(scale,"linear"))
        return x*(max-min)+min;
    else if(!strcmp(scale,"logarithmic")) {
        const float b = log(min);
        const float a = log(max)-b;
        return expf(a*x+b);
    }

    return 0.0f;
}
