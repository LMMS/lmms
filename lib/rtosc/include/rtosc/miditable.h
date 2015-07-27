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
#ifndef RTOSC_MIDITABLE_H
#define RTOSC_MIDITABLE_H

#include <rtosc/ports.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <sstream>
#include <deque>
#include <utility>
#include <cassert>

namespace rtosc {
/**
 * Module Overview
 *
 * Actions:
 *  - Add a mapping {coarse/fine}    [nRT]
 *  - Delete a mapping {coarse/fine} [nRT]
 *  - Transform mapping value based on passive observation [nRT]
 *  - Find unused CC numbers  [RT]
 *  - Transform CC into event {coarse/fine} [RT]
 */

class MidiMapperStorage
{
    public:
        //Almost immutable short vector class
        template<class T>
        class TinyVector {
            int n;
            T  *t;
            public:
            TinyVector(void):n(0),t(0){}
            TinyVector(int i):n(i),t(new T[i]){}
            T&operator[](int i)       {assert(i>=0 && i<n);return t[i];}
            T operator[](int i) const {assert(i>=0 && i<n);return t[i];}

            TinyVector insert(const T &t_)
            {TinyVector next(n+1); for(int i=0;i<n; ++i) next.t[i]=t[i]; next.t[n] = t_;return std::move(next);}
            TinyVector one_larger(void)
            {TinyVector next(n+1); for(int i=0;i<n + 1; ++i) next.t[i]=0; return std::move(next);}
            TinyVector sized_clone(void)
            {TinyVector next(n); for(int i=0;i<n; ++i) next.t[i]=0; return std::move(next);}
            TinyVector clone(void)
            {TinyVector next(n); for(int i=0;i<n; ++i) next.t[i]=t[i]; return std::move(next);}
            int size(void) const{return n;}
        };

        typedef std::function<void(const char*)> write_cb;
        typedef std::function<void(int16_t,write_cb)> callback_t;
        //RT Read Only
        TinyVector<std::tuple<int, bool, int>> mapping;//CC->{coarse, val-cb offset}
        TinyVector<callback_t> callbacks;
        //RT RW
        TinyVector<int> values;

        bool handleCC(int ID, int val, write_cb write);

        //TODO try to change O(n^2) algorithm to O(n)
        void cloneValues(const MidiMapperStorage &storage);

        MidiMapperStorage *clone(void);
};

struct MidiBijection
{
    int mode;//0:linear,1:log
    float min;
    float max;
    int operator()(float x) const;
    float operator()(int x) const;
};

#include <cassert>
class MidiMappernRT
{
    public:
        MidiMappernRT(void);
        void map(const char *addr, bool coarse = true);

        MidiMapperStorage *generateNewBijection(const Port &port, std::string);

        void addNewMapper(int ID, const Port &port, std::string addr);

        void addFineMapper(int ID, const Port &port, std::string addr);

        void useFreeID(int ID);

        void unMap(const char *addr, bool coarse);

        void delMapping(int ID, bool coarse, const char *addr);
        void replaceMapping(int, bool, const char *);

        std::map<std::string, std::string> getMidiMappingStrings(void);

        //unclear if this should be be here as a helper or not
        std::string getMappedString(std::string addr);

        MidiBijection getBijection(std::string s);
        
        void snoop(const char *msg);

        void apply_high(int v, int ID);
        void apply_low(int v, int ID);
        void apply_midi(int val, int ID);

        void setBounds(const char *str, float low, float high);

        std::tuple<float,float,float,float> getBounds(const char *str);

        bool has(std::string addr);
        bool hasPending(std::string addr);
        bool hasCoarse(std::string addr);
        bool hasFine(std::string addr);
        bool hasCoarsePending(std::string addr);
        bool hasFinePending(std::string addr);
        int getCoarse(std::string addr);
        int getFine(std::string addr);

        //(Location, Coarse, Fine, Bijection)
        std::map<std::string, std::tuple<int, int, int, MidiBijection>> inv_map;
        std::deque<std::pair<std::string,bool>> learnQueue;
        std::function<void(const char *)> rt_cb;
        MidiMapperStorage *storage;
        const Ports *base_ports;
};

class MidiMapperRT
{
    public:
        MidiMapperRT(void);
        void setBackendCb(std::function<void(const char*)> cb);
        void setFrontendCb(std::function<void(const char*)> cb);
        void handleCC(int ID, int val);
        void addWatch(void);
        void remWatch(void);
        Port addWatchPort(void);
        Port removeWatchPort(void);
        Port bindPort(void);

        //Fixed upper bounded size set of integer IDs
        class PendingQueue
        {
            public:
                PendingQueue()
                    :pos_r(0), pos_w(0), size(0)
                {
                    for(int i=0; i<32; ++i)
                        vals[i] = -1;
                }
                void insert(int x)
                {
                    if(has(x) || size > 31)
                        return;
                    vals[pos_w] = x;
                    size++;
                    pos_w = (pos_w+1)%32;
                }
                void pop(void)
                {
                    if(size == 0)
                        return;
                    size--;
                    vals[pos_r] = -1;
                    pos_r = (1+pos_r)%32;
                }
                bool has(int x)
                {
                    for(int i=0; i<32; ++i)
                        if(vals[i] == x)
                            return true;
                    return false;
                }
                int vals[32];
                int pos_r;
                int pos_w;
                int size;

        };


        /***************
         * Member Data *
         ***************/
        PendingQueue pending;
        MidiMapperStorage *storage;
        unsigned watchSize;
        std::function<void(const char*)> backend;
        std::function<void(const char*)> frontend;
};

struct MidiAddr
{
    //The midi values that map to the specified action
    uint8_t ch, ctl;

    //The type of the event 'f', 'i', 'T', 'c'
    char  type;
    //The path of the event
    char *path;
    //The conversion function for 'f' types
    const char *conversion;
};


/**
 * Table of midi mappings - Deprecated
 *
 */
class MidiTable
{
    public:

        const Ports  &dispatch_root;
        short  unhandled_ch;
        short  unhandled_ctl;
        char  *unhandled_path;

        void (*error_cb)(const char *, const char *);
        void (*event_cb)(const char *);
        void (*modify_cb)(const char *, const char *, const char *, int, int);

        MidiTable(const Ports &_dispatch_root);
        ~MidiTable();

        bool has(uint8_t ch, uint8_t ctl) const;

        MidiAddr *get(uint8_t ch, uint8_t ctl);

        const MidiAddr *get(uint8_t ch, uint8_t ctl) const;

        bool mash_port(MidiAddr &e, const Port &port);

        void addElm(uint8_t ch, uint8_t ctl, const char *path);

        void check_learn(void);

        void learn(const char *s);

        void clear_entry(const char *s);

        void process(uint8_t ch, uint8_t ctl, uint8_t val);

        Port learnPort(void);
        Port unlearnPort(void);
        Port registerPort(void);

        //TODO generalize to an addScalingFunction() system
        static float translate(uint8_t val, const char *meta);

    private:
        class MidiTable_Impl *impl;
};

};
#endif
