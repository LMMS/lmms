/* Calf DSP Library
 * LV2-related helper classes and functions
 *
 * Copyright (C) 2001-2008 Krzysztof Foltman
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
#ifndef CALF_LV2HELPERS_H
#define CALF_LV2HELPERS_H

#if USE_LV2

#include <calf/lv2_event.h>
#include <calf/lv2_uri_map.h>

class uri_map_access
{
public:
    /// URI map feature pointer (previously in a mixin, but polymorphic ports made it necessary for most plugins)
    LV2_URI_Map_Feature *uri_map;

    uri_map_access()
    : uri_map(NULL)
    {}

    /// Map an URI through an URI map
    uint32_t map_uri(const char *ns, const char *URI)
    {
        if (uri_map)
            return uri_map->uri_to_id(uri_map->callback_data, ns, URI);
        return 0;
    }
    /// Called on instantiation for every LV2 feature sent by a host
    void use_feature(const char *URI, void *data) {
        if (!strcmp(URI, LV2_URI_MAP_URI))
        {
            uri_map = (LV2_URI_Map_Feature *)data;
            map_uris();
        }
    }
    virtual void map_uris()
    {
    }
    virtual ~uri_map_access() {}
};
    
/// A mixin for adding the event feature and URI map to the small plugin
template<class T>
class event_mixin: public T
{
public:
    /// Event feature pointer
    LV2_Event_Feature *event_feature;
    virtual void use_feature(const char *URI, void *data) {
        if (!strcmp(URI, LV2_EVENT_URI))
        {
            event_feature = (LV2_Event_Feature *)data;
        }
        T::use_feature(URI, data);
    }
    /// Create a reference
    inline void ref_event(LV2_Event *event) { event_feature->lv2_event_ref(event_feature->callback_data, event); }
    /// Destroy a reference
    inline void unref_event(LV2_Event *event) { event_feature->lv2_event_unref(event_feature->callback_data, event); }
};

/// A mixin for adding the URI map and MIDI event type retrieval to small plugins
template<class T>
class midi_mixin: public virtual event_mixin<T>
{
public:
    /// MIDI event ID, as resolved using the URI map feature
    uint32_t midi_event_type;
    virtual void map_uris() {
        midi_event_type = this->map_uri("http://lv2plug.in/ns/ext/event", "http://lv2plug.in/ns/ext/midi#MidiEvent");
        printf("MIDI event type = %d\n", midi_event_type);
        event_mixin<T>::map_uris();
    }
};

/// A mixin for adding the URI map and MIDI event type retrieval to small plugins
template<class T>
class message_mixin: public virtual event_mixin<T>
{
public:
    /// MIDI event ID, as resolved using the URI map feature
    uint32_t message_event_type;
    virtual void map_uris() {
        message_event_type = this->map_uri("http://lv2plug.in/ns/ext/event", "http://lv2plug.in/ns/dev/msg#MessageEvent");
        printf("Message event type = %d\n", message_event_type);
        event_mixin<T>::map_uris();
    }
};

/// LV2 event structure + payload as 0-length array for easy access
struct lv2_event: public LV2_Event
{
    uint8_t data[];
    inline lv2_event &operator=(const lv2_event &src) {
        *(LV2_Event *)this = (const LV2_Event &)src;
        memcpy(data, src.data, src.size);
        return *this;
    }
    /// Returns a 64-bit timestamp for easy and inefficient comparison
    inline uint64_t timestamp() const {
        return ((uint64_t)frames << 32) | subframes;
    }
private:
    /// forbid default constructor - this object cannot be constructed, only obtained via cast from LV2_Event* (or &) to lv2_event* (or &)
    lv2_event() {}
    /// forbid copy constructor - see default constructor
    lv2_event(const lv2_event &) {}
};

/// A read-only iterator-like object for reading from event buffers
class event_port_read_iterator
{
protected:
    const LV2_Event_Buffer *buffer;
    uint32_t offset;
public:
    /// Default constructor creating a useless iterator you can assign to
    event_port_read_iterator()
    : buffer(NULL)
    , offset(0)
    {
    }
    
    /// Create an iterator based on specified buffer and index/offset values
    event_port_read_iterator(const LV2_Event_Buffer *_buffer, uint32_t _offset = 0)
    : buffer(_buffer)
    , offset(0)
    {
    }

    /// Are any data left to be read?
    inline operator bool() const {
        return offset < buffer->size;
    }
    
    /// Read pointer
    inline const lv2_event &operator*() const {
        return *(const lv2_event *)(buffer->data + offset);
    }
    /// Pointer to member
    inline const lv2_event *operator->() const {
        return &**this;
    }

    /// Move to the next element
    inline event_port_read_iterator operator++() {
        offset += ((**this).size + 19) &~7;
        return *this;
    }

    /// Move to the next element
    inline event_port_read_iterator operator++(int) {
        event_port_read_iterator old = *this;
        offset += ((**this).size + 19) &~7;
        return old;
    }
};

/// A write-only iterator-like object for writing to event buffers
class event_port_write_iterator
{
protected:
    LV2_Event_Buffer *buffer;
public:
    /// Default constructor creating a useless iterator you can assign to
    event_port_write_iterator()
    : buffer(NULL)
    {
    }
    
    /// Create a write iterator based on specified buffer and index/offset values
    event_port_write_iterator(LV2_Event_Buffer *_buffer)
    : buffer(_buffer)
    {
    }

    /// @return the remaining buffer space
    inline uint32_t space_left() const {
        return buffer->capacity - buffer->size;
    }
    /// @return write pointer
    inline lv2_event &operator*() {
        return *(lv2_event *)(buffer->data + buffer->size);
    }
    /// Pointer to member
    inline lv2_event *operator->() {
        return &**this;
    }
    /// Move to the next element after the current one has been written (must be called after each write)
    inline event_port_write_iterator operator++() {
        buffer->size += ((**this).size + 19) &~7;
        buffer->event_count ++;
        return *this;
    }
    /// Move to the next element after the current one has been written
    inline lv2_event *operator++(int) {
        lv2_event *ptr = &**this;
        buffer->size += ((**this).size + 19) &~7;
        buffer->event_count ++;
        return ptr;
    }
};

template<class Iter1, class Iter2>
class event_port_merge_iterator
{
public:
    Iter1 first;
    Iter2 second;
public:
    event_port_merge_iterator() {}
    event_port_merge_iterator(const Iter1 &_first, const Iter2 &_second)
    : first(_first)
    , second(_second)
    {
    }
    /// @retval true if any of the iterators have any data left
    inline operator bool() const {
        return ((bool)first) || ((bool)second);
    }
    inline bool select_first() const
    {
        if (!(bool)second)
            return true;
        if (!(bool)first)
            return false;
        return first->timestamp() < second->timestamp();
    }
    /// Returns the earliest of (*first, *second)
    inline const lv2_event &operator*() const {
        if (select_first())
        {
            assert((bool)first);
            return *first;
        }
        assert((bool)second);
        return *second;
    }
    /// Pointer to member
    inline const lv2_event *operator->() const {
        return &**this;
    }
    /// Prefix increment
    inline event_port_merge_iterator operator++() {
        if (select_first())
            first++;
        else
            second++;
        return *this;
    }
    /// Postfix increment
    inline event_port_merge_iterator operator++(int) {
        event_port_merge_iterator ptr = *this;
        if (select_first())
            first++;
        else
            second++;
        return ptr;
    }
};

#endif
#endif
