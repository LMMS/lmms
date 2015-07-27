#ifndef RTOSC_UNDO_H
#define RTOSC_UNDO_H
#include <functional>

namespace rtosc
{
/**
 * Known event types:
 * /undo_change /path/location old-data new-data
 */
class UndoHistory
{
    //TODO think about the consequences of largish loads
    public:
        UndoHistory(void);

        //Records any undoable event
        void recordEvent(const char *msg);

        //Prints out a history
        void showHistory(void) const;

        //Seek to another point in history relative to the current one
        //Negative values mean undo, positive values mean redo
        void seekHistory(int distance);

        unsigned getPos(void) const;
        const char *getHistory(int i) const;
        size_t size(void) const;

        void setCallback(std::function<void(const char*)> cb);
    private:
        class UndoHistoryImpl *impl;
};
};
#endif
