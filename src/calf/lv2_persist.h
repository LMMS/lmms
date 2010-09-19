// This file is in the public domain.
// Written by Leonard Ritter <paniq@paniq.org>

#ifndef __LV2_PERSIST_H__
#define __LV2_PERSIST_H__

#ifdef __cplusplus
extern "C" {
#endif
    
/*
    Indicates that the plugin supports persistence, which means
    being able to save and restore internal state, allowing hosts to
    save configuration with a project, or to clone a plugin, including
    internal state.
    
    The plugin should always expose this feature as optional.
*/
#define LV2_PERSIST_URI "http://paniq.org/lv2/persist"

   
/*
    Causes the host to store a binary blob in a map.
    
    A callback provided by the host to LV2_Persist.save(). 'callback_data'
    must be the callback_data argument passed to save(). 'key' is a private
    string or URI under which the data is to be stored. 'value' points to
    the binary blob to be stored. 'size' is the size of the binary blob in
    bytes.
    
    The host must store a copy of the blob under the provided key in a map 
    until returning from the save() call.
    
    A size of 0 indicates that value points to a zero-terminated string.
    This is a convenience function which requires the host to calculate the
    length as strlen(value)+1.
*/
typedef void (*LV2_Persist_Store_Function)(void *callback_data, const char *key, 
                                           const void *value, size_t size);

/*
    Causes the host to retrieve a binary blob from the map.
    
    A callback provided by the host to LV2_Persist.restore(). 'callback_data'
    must be the callback_data argument passed to restore(). 'key' is a private
    string or UI under which a blob has been previously stored.
    
    When the blob could be successfully retrieved, retrieve() must return
    a pointer to the blob and set 'size' to the length of value in bytes.
    
    'size' may be NULL. In this case, no return of the blob size is required.
    This is a convenience function to retrieve zero-terminated strings.
    
    The returned value must remain valid until restore() returns. The plugin
    is required to make a copy if it needs to continue working with the
    data. It must not attempt to access a retrieved blob pointer outside
    of the restore context.
*/
typedef const void *(*LV2_Persist_Retrieve_Function)(void *callback_data, 
                                const char *key, size_t *size);

/*
    When the plugins extension_data is called with argument LV2_PERSIST_URI,
    the plugin is expected to return an LV2_Persist structure, which remains
    valid indefinitely.
    
    The host can use the exposed function pointers to save and restore
    the state of a plugin to a map of string keys to binary blobs at any
    time.
    
    The usual application would be to save the plugins state when the
    project document is to be saved, and to restore the state when
    a project document has been loaded. Other applications are possible.
    
    Blob maps are meant to be only compatible between instances of the
    same plugin. However, should a future extension require persistent
    data to follow an URI key naming scheme, this restriction no longer
    applies.
*/
struct LV2_Persist {
    /*
        Causes the plugin to save state data which it wants to preserve
        across plugin lifetime using a store callback provided by
        the host.
    
        'instance' is the instance handle of the plugin. 'callback_data'
        is an opaque pointer to host data, e.g. the map or file where
        the blobs are to be stored, it should be passed to 'store',
        which is a host-supplied function to store a blob. For more
        information, see LV2_Persist_Store_Function.
    
        The map on which save() operates must always be empty before
        the first call to store(). The plugin is expected to store all
        blobs of interests.
    
        The callback data pointer and store function may not be used
        beyond the scope of save().
    */
    void (*save)(LV2_Handle instance, 
        LV2_Persist_Store_Function store, void *callback_data);
    
    /*
        Causes the plugin to restore state data using a retrieve callback
        provided by the host.
        
        'instance' is the instance handle of the plugin. 'callback_data'
        is an opaque pointer to host data, e.g. the map or file where
        the blobs are to be retrieved from; it should be passed to
        'retrieve', which is a host-supplied function to retrieve blobs.
        For more information, see LV2_Persist_Retrieve_Function.

        The map on which restore() operates must contain values stored
        by a plugin instance of the same class, or be empty.

        The plugin must gracefully fall back to a default value
        when a blob can not be retrieved. This allows the host to reset
        the plugin state with an empty map.
        
        The callback data pointer and store function may not be used
        beyond the scope of save().
    */
    void (*restore)(LV2_Handle instance, 
        LV2_Persist_Retrieve_Function retrieve, void *callback_data);
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __LV2_PERSIST_H__ */
