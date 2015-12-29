#include <rtosc/rtosc.h>
#include <ctype.h>
#include <stdlib.h>

static bool rtosc_match_number(const char **pattern, const char **msg)
{
    //Verify both hold digits
    if(!isdigit(**pattern) || !isdigit(**msg))
        return false;

    //Read in both numeric values
    unsigned max = atoi(*pattern);
    unsigned val = atoi(*msg);

    ////Advance pointers
    while(isdigit(**pattern))++*pattern;
    while(isdigit(**msg))++*msg;

    //Match iff msg number is strictly less than pattern
    return val < max;
}

const char *rtosc_match_path(const char *pattern, const char *msg)
{
    while(1) {
        //Check for special characters
        if(*pattern == ':' && !*msg)
            return pattern;
        else if(*pattern == '/' && *msg == '/')
            return ++pattern;
        else if(*pattern == '#') {
            ++pattern;
            if(!rtosc_match_number(&pattern, &msg))
                return NULL;
        } else if((*pattern == *msg)) { //verbatim compare
            if(*msg)
                ++pattern, ++msg;
            else
                return pattern;
        } else
            return NULL;
    }
}

//Match the arg string or fail
static bool rtosc_match_args(const char *pattern, const char *msg)
{
    //match anything if now arg restriction is present (ie the ':')
    if(*pattern++ != ':')
        return true;

    const char *arg_str = rtosc_argument_string(msg);
    bool      arg_match = *pattern || *pattern == *arg_str;

    while(*pattern && *pattern != ':')
        arg_match &= (*pattern++==*arg_str++);

    if(*pattern==':') {
        if(arg_match && !*arg_str)
            return true;
        else
            return rtosc_match_args(pattern, msg); //retry
    }

    return arg_match;
}

bool rtosc_match(const char *pattern, const char *msg)
{
    const char *arg_pattern = rtosc_match_path(pattern, msg);
    if(!arg_pattern)
        return false;
    else if(*arg_pattern == ':')
        return rtosc_match_args(arg_pattern, msg);
    return true;
}
