/* Calf DSP Library
 * Utilities
 *
 * Copyright (C) 2008 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02111-1307, USA.
 */
#ifndef __CALF_UTILS_H
#define __CALF_UTILS_H

#include <errno.h>
#include <pthread.h>
#include <map>
#include <string>

namespace calf_utils
{

/// Pthreads based mutex class
class ptmutex
{
public:
    pthread_mutex_t pm;
    
    ptmutex(int type = PTHREAD_MUTEX_RECURSIVE)
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, type);
        pthread_mutex_init(&pm, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    
    bool lock()
    {
        return pthread_mutex_lock(&pm) == 0;
    }
    
    bool trylock()
    {
        return pthread_mutex_trylock(&pm) == 0;
    }
    
    void unlock()
    {
        pthread_mutex_unlock(&pm);
    }

    ~ptmutex()
    {
        pthread_mutex_destroy(&pm);
    }
};

class ptlock_base
{
protected:
    ptmutex &mutex;
    bool locked;

    ptlock_base(ptmutex &_m)
    : mutex(_m)
    , locked(false)
    {
    }
    
public:
    bool is_locked()
    {
        return locked;
    }
    void unlock()
    {
        mutex.unlock();
        locked = false;
    }
    void unlocked()
    {
        locked = false;
    }
    ~ptlock_base()
    {
        if (locked)
            mutex.unlock();
    }
};

/// Exception-safe mutex lock
class ptlock: public ptlock_base
{
public:
    ptlock(ptmutex &_m) : ptlock_base(_m)
    {
        locked = mutex.lock();
    }
};

/// Exception-safe polling mutex lock
class pttrylock: public ptlock_base
{
public:
    pttrylock(ptmutex &_m) : ptlock_base(_m)
    {
        locked = mutex.trylock();
    }
};

/// Exception-safe temporary assignment
template<class T, class Tref = T&>
class scope_assign
{
    Tref data;
    T old_value;
public:
    scope_assign(Tref _data, T new_value)
    : data(_data), old_value(_data)
    {
        data = new_value;
    }
    ~scope_assign()
    {
        data = old_value;
    }
};

struct text_exception: public std::exception
{
    const char *text;
    std::string container;
public:
    text_exception(const std::string &t) : container(t) { text = container.c_str(); }
    virtual const char *what() const throw () { return text; }
    virtual ~text_exception() throw () {}
};

struct file_exception: public std::exception
{
    const char *text;
    std::string message, filename, container;
public:
    file_exception(const std::string &f);
    file_exception(const std::string &f, const std::string &t);
    virtual const char *what() const throw () { return text; }
    virtual ~file_exception() throw () {}
};

/// String-to-string mapping
typedef std::map<std::string, std::string> dictionary;

/// Serialize a dictonary to a string
extern std::string encode_map(const dictionary &data);
/// Deserialize a dictonary from a string
extern void decode_map(dictionary &data, const std::string &src);

/// int-to-string
extern std::string i2s(int value);

/// float-to-string
extern std::string f2s(double value);

/// float-to-string-that-doesn't-resemble-an-int
extern std::string ff2s(double value);

/// Encode a key-value pair as XML attribute
std::string to_xml_attr(const std::string &key, const std::string &value);

/// Escape a string to be used in XML file
std::string xml_escape(const std::string &src);

/// Load file from disk into a std::string blob, or throw file_exception
std::string load_file(const std::string &src);

/// Indent a string by another string (prefix each line)
std::string indent(const std::string &src, const std::string &indent);

};

#endif
