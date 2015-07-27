#include <deque>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <ctime>
#include <rtosc/rtosc.h>
#include <rtosc/undo-history.h>

using std::pair;
using std::make_pair;

namespace rtosc {
class UndoHistoryImpl
{
    public:
        UndoHistoryImpl(void)
            :max_history_size(20)
        {}
        std::deque<pair<time_t, const char *>> history;
        long history_pos;
        unsigned max_history_size;//XXX Expose this via a public API
        std::function<void(const char*)> cb;

        void rewind(const char *msg);
        void replay(const char *msg);
        bool mergeEvent(time_t t, const char *msg, char *buf, size_t N);
};

UndoHistory::UndoHistory(void)
{
    impl = new UndoHistoryImpl;
    impl->history_pos  = 0;
}

void UndoHistory::recordEvent(const char *msg)
{
    //TODO Properly account for when you have traveled back in time.
    //while this could result in another branch of history, the simple method
    //would be to kill off any future redos when new history is recorded
    if(impl->history.size() != (unsigned) impl->history_pos) {
        impl->history.resize(impl->history_pos);
    }

    size_t len = rtosc_message_length(msg, -1);
    char *data = new char[len];
    time_t now = time(NULL);
    //printf("now = '%ld'\n", now);
    if(!impl->mergeEvent(now, msg, data, len)) {
        memcpy(data, msg, len);
        impl->history.push_back(make_pair(now, data));
        impl->history_pos++;
        if(impl->history.size() > impl->max_history_size)
        {
            delete[] impl->history[0].second;
            impl->history.pop_front();
            impl->history_pos--;
        }
    }

}

void UndoHistory::showHistory(void) const
{
    int i = 0;
    for(auto s : impl->history)
        printf("#%d type: %s dest: %s arguments: %s\n", i++,
                s.second, rtosc_argument(s.second, 0).s, rtosc_argument_string(s.second));
}

static char tmp[256];
void UndoHistoryImpl::rewind(const char *msg)
{
    memset(tmp, 0, sizeof(tmp));
    printf("rewind('%s')\n", msg);
    rtosc_arg_t arg = rtosc_argument(msg,1);
    rtosc_amessage(tmp, 256, rtosc_argument(msg,0).s,
            rtosc_argument_string(msg)+2,
            &arg);
    cb(tmp);
}

void UndoHistoryImpl::replay(const char *msg)
{
    printf("replay...'%s'\n", msg);
    rtosc_arg_t arg = rtosc_argument(msg,2);
    printf("replay address: '%s'\n", rtosc_argument(msg, 0).s);
    int len = rtosc_amessage(tmp, 256, rtosc_argument(msg,0).s,
            rtosc_argument_string(msg)+2,
            &arg);
    
    if(len)
        cb(tmp);
}

const char *getUndoAddress(const char *msg)
{
    return rtosc_argument(msg,0).s;
}

bool UndoHistoryImpl::mergeEvent(time_t now, const char *msg, char *buf, size_t N)
{
    if(history_pos == 0)
        return false;
    for(int i=history_pos-1; i>=0; --i) {
        if(difftime(now, history[i].first) > 2)
            break;
        if(!strcmp(getUndoAddress(msg),
                    getUndoAddress(history[i].second)))
        {
            //We can splice events together, merging them into one event
            rtosc_arg_t args[3];
            args[0] = rtosc_argument(msg, 0);
            args[1] = rtosc_argument(history[i].second,1);
            args[2] = rtosc_argument(msg, 2);

            rtosc_amessage(buf, N, msg, rtosc_argument_string(msg), args);

            delete [] history[i].second;
            history[i].second = buf;
            history[i].first = now;
            return true;
        }
    }
    return false;
}



void UndoHistory::seekHistory(int distance)
{
    //TODO print out the events that would need to take place to get to the
    //final destination
    
    //TODO limit the distance to be to applicable sizes
    //ie ones that do not exceed the known history/future
    long dest = impl->history_pos + distance;
    if(dest < 0)
        distance -= dest;
    if(dest > (long) impl->history.size())
        distance  = impl->history.size() - impl->history_pos;
    if(!distance)
        return;
    
    printf("distance == '%d'\n", distance);
    printf("history_pos == '%ld'\n", impl->history_pos);
    //TODO account for traveling back in time
    if(distance<0)
        while(distance++)
            impl->rewind(impl->history[--impl->history_pos].second);
    else
        while(distance--)
            impl->replay(impl->history[impl->history_pos++].second);
}

unsigned UndoHistory::getPos(void) const
{
    return impl->history_pos;
}

const char *UndoHistory::getHistory(int i) const
{
    return impl->history[i].second;
}

size_t UndoHistory::size() const
{
    return impl->history.size();
}

void UndoHistory::setCallback(std::function<void(const char*)> cb)
{
    impl->cb = cb;
}
};
