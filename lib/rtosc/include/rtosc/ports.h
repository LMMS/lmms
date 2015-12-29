/*
 * Copyright (c) 2012 Mark McCurry
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef RTOSC_PORTS
#define RTOSC_PORTS

#include <vector>
#include <functional>
#include <initializer_list>
#include <rtosc/rtosc.h>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <string>

namespace rtosc {

//First define all types
typedef const char *msg_t;

struct Port;
struct Ports;

struct RtData
{
    RtData(void);

    char *loc;
    size_t loc_size;
    void *obj;
    int  matches;
    const Port *port;

    virtual void reply(const char *path, const char *args, ...);
    virtual void reply(const char *msg);
    virtual void broadcast(const char *path, const char *args, ...);
    virtual void broadcast(const char *msg);
};


/**
 * Port in rtosc dispatching hierarchy
 */
struct Port {
    const char  *name;    //< Pattern for messages to match
    const char  *metadata;//< Statically accessable data about port
    const Ports *ports;   //< Pointer to further ports
    std::function<void(msg_t, RtData&)> cb;//< Callback for matching functions

    class MetaIterator
    {
        public:
            MetaIterator(const char *str);

            //A bit odd to return yourself, but it seems to work for this
            //context
            const MetaIterator& operator*(void) const {return *this;}
            const MetaIterator* operator->(void) const {return this;}
            bool operator==(MetaIterator a) {return title == a.title;}
            bool operator!=(MetaIterator a) {return title != a.title;}
            MetaIterator& operator++(void);

            const char *title;
            const char *value;
    };

    class MetaContainer
    {
        public:
            MetaContainer(const char *str_);

            MetaIterator begin(void) const;
            MetaIterator end(void) const;

            MetaIterator find(const char *str) const;
            size_t length(void) const;
            const char *operator[](const char *str) const;

            const char *str_ptr;
    };

    MetaContainer meta(void) const
    {
        if(metadata && *metadata == ':')
            return MetaContainer(metadata+1);
        else
            return MetaContainer(metadata);
    }
};

/**
 * Ports - a dispatchable collection of Port entries
 *
 * This structure makes it somewhat easier to perform actions on collections of
 * port entries and it is responsible for the dispatching of OSC messages to
 * their respective ports.
 * That said, it is a very simple structure, which uses a stl container to store
 * all data in a simple dispatch table.
 * All methods post-initialization are RT safe (assuming callbacks are RT safe)
 */
struct Ports
{
    std::vector<Port> ports;


    typedef std::vector<Port>::const_iterator itr_t;

    /**Forwards to builtin container*/
    itr_t begin() const {return ports.begin();}

    /**Forwards to builtin container*/
    itr_t end() const {return ports.end();}

    /**Forwards to builtin container*/
    size_t size() const {return ports.size();}

    /**Forwards to builtin container*/
    const Port &operator[](unsigned i) const {return ports[i];}

    Ports(std::initializer_list<Port> l);
    ~Ports(void);

    Ports(const Ports&) = delete;

    /**
     * Dispatches message to all matching ports.
     * This uses simple pattern matching available in rtosc::match.
     *
     * @param m a valid OSC message
     * @param d The RtData object shall contain a path buffer (or null), the length of
     *          the buffer, a pointer to data.
     */
    void dispatch(const char *m, RtData &d) const;

    /**
     * Retrieve local port by name
     * TODO implement full matching
     */
    const Port *operator[](const char *name) const;


    /**
     * Find the best match for a given path
     *
     * @parameter path partial OSC path
     * @returns first path prefixed by the argument
     *
     * Example usage:
     * @code
     *    Ports p = {{"foo",0,0,dummy_method},
     *               {"flam",0,0,dummy_method},
     *               {"bar",0,0,dummy_method}};
     *    p.apropos("/b")->name;//bar
     *    p.apropos("/f")->name;//foo
     *    p.apropos("/fl")->name;//flam
     *    p.apropos("/gg");//NULL
     * @endcode
     */
    const Port *apropos(const char *path) const;

    private:
    //Performance hacks
    class Port_Matcher *impl;
    unsigned elms;
};


/*********************
 * Port walking code *
 *********************/
//typedef std::function<void(const Port*,const char*)> port_walker_t;
typedef void(*port_walker_t)(const Port*,const char*,void*);

void walk_ports(const Ports *base,
        char          *name_buffer,
        size_t         buffer_size,
        void          *data,
        port_walker_t  walker);

/*********************
 * Port Dumping code *
 *********************/

struct OscDocFormatter
{
    const Ports *p;
    std::string prog_name;
    std::string uri;
    std::string doc_origin;
    std::string author_first;
    std::string author_last;
    //TODO extend this some more
};

std::ostream &operator<<(std::ostream &o, OscDocFormatter &formatter);
};
#endif
