#ifndef RTOSC_TYPED_MESSAGE_H
#define RTOSC_TYPED_MESSAGE_H
#include <rtosc/rtosc.h>
#include <type_traits>
#include <stdexcept>

namespace rtosc
{

template<class... Types> class rtMsg;

// empty tuple
template<> class rtMsg<>
{
    public:
        rtMsg(const char *arg = NULL, const char *spec=NULL, bool _=false)
            :msg(arg)
        {
            if(arg && spec && !rtosc_match_path(spec, arg))
                msg = NULL;
            (void)_;
        }

        operator bool(void){return this->msg;}

        const char *msg;
};


template<class T>
bool valid_char(char) { return false;}

template<>
bool valid_char<const char*>(char c) { return c=='s' || c=='S'; };

template<>
bool valid_char<int32_t>(char c) { return c=='i'; };

template<>
bool valid_char<float>(char c) { return c=='f'; };

template<int i>
bool validate(const char *arg)
{
    return rtosc_narguments(arg) == i;
}

template<int i, class This, class... Rest>
bool validate(const char *arg)
{
    if(!valid_char<This>(rtosc_type(arg,i)))
        return false;
    else
        return validate<i+1,Rest...>(arg);
}

//Tuple Like Template Class Definition
template<class This, class... Rest>
class rtMsg<This, Rest...>:public rtMsg<Rest...>
{
    public:
        typedef rtMsg<Rest...> T;
        rtMsg(const char *arg = NULL, const char *spec=NULL)
            :T(arg, spec, false)
        {
            if(this->msg && !validate<0,This,Rest...>(this->msg))
                this->msg = NULL;
        }

        rtMsg(const char *arg, const char *spec, bool)
            :T(arg, spec, false)
        {}
};


// tuple_element
template<size_t Index, class Tuple> struct osc_element;

// select first element
template<class This, class... Rest>
struct osc_element<0, rtMsg<This, Rest...>>
{
    typedef This type;
};

// recursive tuple_element definition
template <size_t Index, class This, class... Rest>
struct osc_element<Index, rtMsg<This, Rest...>>
: public osc_element<Index - 1, rtMsg<Rest...>>
{
};

template<class T>
T rt_get_impl(const char *msg, size_t i);

template<>
const char *rt_get_impl(const char *msg, size_t i)
{
    return rtosc_argument(msg,i).s;
}

template<>
int32_t rt_get_impl(const char *msg, size_t i)
{
    return rtosc_argument(msg,i).i;
}

// get reference to _Index element of tuple
template<size_t Index, class... Types> inline
    typename osc_element<Index, rtMsg<Types...>>::type
get(rtMsg<Types...>& Tuple)
{
    if(!Tuple.msg)
        throw std::invalid_argument("Message Does Not Match Spec");
    typedef typename std::remove_reference<typename osc_element<Index, rtMsg<Types...>>::type>::type T;
    return rt_get_impl<T>(Tuple.msg, Index);
}
};
#endif
