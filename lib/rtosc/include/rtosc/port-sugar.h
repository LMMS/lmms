/*
 * Copyright (c) 2013 Mark McCurry
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

#ifndef RTOSC_PORT_SUGAR
#define RTOSC_PORT_SUGAR

//Hack to workaround old incomplete decltype implementations
#ifdef __GNUC__
#if    __GNUC__ < 4 ||  (__GNUC__ == 4 && __GNUC_MINOR__ <= 7)
template<typename T>
struct rtosc_hack_decltype_t
{
    typedef T type;
};

#define decltype(expr) rtosc_hack_decltype_t<decltype(expr)>::type
#endif
#endif

//General macro utilities
#define STRINGIFY2(a) #a
#define STRINGIFY(a) STRINGIFY2(a)

//Helper for documenting varargs
#define IMPL(_1,_2,_3,_4,_5,_6,_7,_8,_9, N, ...) N
#define LAST_IMP(...) IMPL(__VA_ARGS__,9,8,7,6,5,4,3,2,1,0,0,0,0)
#define DOC_IMP9(a,b,c,d,e,f,g,h,i) a b c d e f g h rDoc(i)
#define DOC_IMP8(a,b,c,d,e,f,g,h)   a b c d e f g rDoc(h)
#define DOC_IMP7(a,b,c,d,e,f,g)     a b c d e f rDoc(g)
#define DOC_IMP6(a,b,c,d,e,f)       a b c d e rDoc(f)
#define DOC_IMP5(a,b,c,d,e)         a b c d rDoc(e)
#define DOC_IMP4(a,b,c,d)           a b c rDoc(d)
#define DOC_IMP3(a,b,c)             a b rDoc(c)
#define DOC_IMP2(a,b)               a rDoc(b)
#define DOC_IMP1(a)                 rDoc(a)
#define DOC_IMP0() YOU_MUST_DOCUMENT_YOUR_PORTS
#define DOC_IMP(count, ...) DOC_IMP ##count(__VA_ARGS__)
#define DOC_I(count, ...) DOC_IMP(count,__VA_ARGS__)
#define DOC(...) DOC_I(LAST_IMP(__VA_ARGS__), __VA_ARGS__)

//XXX Currently unused macro
#define MAC_EACH_0(mac, x, ...) INSUFFICIENT_ARGUMENTS_PROVIDED_TO_MAC_EACH
#define MAC_EACH_1(mac, x, ...) mac(x)
#define MAC_EACH_2(mac, x, ...) mac(x) MAC_EACH_1(mac, __VA_ARGS__)
#define MAC_EACH_3(mac, x, ...) mac(x) MAC_EACH_2(mac, __VA_ARGS__)
#define MAC_EACH_4(mac, x, ...) mac(x) MAC_EACH_3(mac, __VA_ARGS__)
#define MAC_EACH_5(mac, x, ...) mac(x) MAC_EACH_4(mac, __VA_ARGS__)
#define MAC_EACH_6(mac, x, ...) mac(x) MAC_EACH_5(mac, __VA_ARGS__)
#define MAC_EACH_7(mac, x, ...) mac(x) MAC_EACH_6(mac, __VA_ARGS__)
#define MAC_EACH_8(mac, x, ...) mac(x) MAC_EACH_7(mac, __VA_ARGS__)
#define MAC_EACH_9(mac, x, ...) mac(x) MAC_EACH_8(mac, __VA_ARGS__)

#define MAC_EACH_IMP(mac, count, ...) MAC_EACH_ ##count(mac,__VA_ARGS__)
#define MAC_EACH_I(mac, count, ...) MAC_EACH_IMP(mac, count, __VA_ARGS__)
#define MAC_EACH(mac, ...) MAC_EACH_I(mac, LAST_IMP(__VA_ARGS__), __VA_ARGS__)

#define OPTIONS_IMP9(a,b,c,d,e,f,g,h,i) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e) rOpt(5,f) rOpt(6,g) \
    rOpt(7,h) rOpt(8,i)
#define OPTIONS_IMP8(a,b,c,d,e,f,g,h) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e) rOpt(5,f) rOpt(6,g) \
    rOpt(7,h)
#define OPTIONS_IMP7(a,b,c,d,e,f,g) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e) rOpt(5,f) rOpt(6,g)
#define OPTIONS_IMP6(a,b,c,d,e,f) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e) rOpt(5,f)
#define OPTIONS_IMP5(a,b,c,d,e) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d) rOpt(4,e)
#define OPTIONS_IMP4(a,b,c,d) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c) rOpt(3,d)
#define OPTIONS_IMP3(a,b,c) \
    rOpt(0,a) rOpt(1,b) rOpt(2,c)
#define OPTIONS_IMP2(a,b) \
    rOpt(0,a) rOpt(1,b)
#define OPTIONS_IMP1(a) \
    rOpt(0,a)
#define OPTIONS_IMP0() YOU_MUST_PROVIDE_OPTIONS
#define OPTIONS_IMP(count, ...) OPTIONS_IMP ##count(__VA_ARGS__)
#define OPTIONS_I(count, ...) OPTIONS_IMP(count, __VA_ARGS__)
#define OPTIONS(...) OPTIONS_I(LAST_IMP(__VA_ARGS__), __VA_ARGS__)

//Additional Change Callback (after parameters have been changed)
//This can be used to queue up interpolation or parameter regen
#define rChangeCb

//Normal parameters
#define rParam(name, ...) \
  {STRINGIFY(name) "::c",  rProp(parameter) rMap(min, 0) rMap(max, 127) DOC(__VA_ARGS__), NULL, rParamCb(name)}
#define rParamF(name, ...) \
  {STRINGIFY(name) "::f",  rProp(parameter) DOC(__VA_ARGS__), NULL, rParamFCb(name)}
#define rParamI(name, ...) \
  {STRINGIFY(name) "::i",  rProp(parameter) DOC(__VA_ARGS__), NULL, rParamICb(name)}
#define rToggle(name, ...) \
  {STRINGIFY(name) "::T:F",rProp(parameter) DOC(__VA_ARGS__), NULL, rToggleCb(name)}
#define rOption(name, ...) \
  {STRINGIFY(name) "::i:c",rProp(parameter) DOC(__VA_ARGS__), NULL, rOptionCb(name)}

//Array operators
#define rArrayF(name, length, ...) \
{STRINGIFY(name) "#" STRINGIFY(length) "::f", rProp(parameter) DOC(__VA_ARGS__), NULL, rArrayFCb(name)}
#define rArray(name, length, ...) \
{STRINGIFY(name) "#" STRINGIFY(length) "::c", rProp(parameter) DOC(__VA_ARGS__), NULL, rArrayCb(name)}
#define rArrayT(name, length, ...) \
{STRINGIFY(name) "#" STRINGIFY(length) "::T:F", rProp(parameter) DOC(__VA_ARGS__), NULL, rArrayTCb(name)}
#define rArrayI(name, length, ...) \
{STRINGIFY(name) "#" STRINGIFY(length) "::i", rProp(parameter) DOC(__VA_ARGS__), NULL, rArrayICb(name)}


//Method callback Actions
#define rAction(name, ...) \
{STRINGIFY(name) ":", DOC(__VA_ARGS__), NULL, rActionCb(name)}
#define rActioni(name, ...) \
{STRINGIFY(name) ":i", DOC(__VA_ARGS__), NULL, rActioniCb(name)}


//Alias operators
#define rParams(name, length, ...) \
rArray(name, length, __VA_ARGS__), \
{STRINGIFY(name) ":", rProp(alias), NULL, rParamsCb(name, length)}


template<class T> constexpr T spice(T*t) {return *t;}

//Recursion [two ports in one for pointer manipulation]
#define rRecur(name, ...) \
    {STRINGIFY(name) "/", DOC(__VA_ARGS__), &decltype(rObject::name)::ports, rRecurCb(name)}, \
    {STRINGIFY(name) ":", rProp(internal), NULL, rRecurPtrCb(name)}

#define rRecurp(name, ...) \
    {STRINGIFY(name) "/", DOC(__VA_ARGS__), \
        &decltype(spice(rObject::name))::ports, \
        rRecurpCb(name)}

#define rRecurs(name, length, ...) \
    {STRINGIFY(name) "#" STRINGIFY(length)"/", DOC(__VA_ARGS__), \
        &decltype(spice(&rObject::name[0]))::ports, \
        rRecursCb(name, length)}

//Technically this is a pointer pointer method...
#define rRecursp(name, length, ...) \
    {STRINGIFY(name)"#" STRINGIFY(length) "/", DOC(__VA_ARGS__), \
        &decltype(spice(rObject::name[0]))::ports, \
        rRecurspCb(name)}

//{STRINGIFY(name) ":", rProp(internal), NULL, rRecurPtrCb(name)}

//Misc
#define rDummy(name, ...) {STRINIFY(name), rProp(dummy), NULL, [](msg_t, rtosc::RtData &){}}
#define rString(name, len, ...) \
    {STRINGIFY(name) "::s", rMap(length, len) DOC(__VA_ARGS__), NULL, rStringCb(name,len)}

//General property operators
#define rMap(name, value) ":" STRINGIFY(name) "\0=" STRINGIFY(value) "\0"
#define rProp(name)       ":" STRINGIFY(name) "\0"

//Scaling property
#define rLinear(min_, max_) rMap(min, min_) rMap(max, max_) rMap(scale, linear)
#define rLog(min_, max_) rMap(min, min_) rMap(max, max_) rMap(scale, logarithmic)

//Special values
#define rSpecial(doc) ":special\0" STRINGIFY(doc) "\0"
#define rCentered ":centered\0"

//Misc properties
#define rDoc(doc) ":documentation\0=" doc "\0"
#define rOpt(numeric,symbolic) rMap(map numeric, symbolic)
#define rOptions(...) OPTIONS(__VA_ARGS__)


//Callback Implementations
#define rBOIL_BEGIN [](const char *msg, rtosc::RtData &data) { \
        (void) msg; (void) data; \
        rObject *obj = (rObject*) data.obj;(void) obj; \
        const char *args = rtosc_argument_string(msg); (void) args;\
        const char *loc = data.loc; (void) loc;\
        auto prop = data.port->meta(); (void) prop;

#define rBOIL_END }

#define rLIMIT(var, convert) \
    if(prop["min"] && var < (decltype(var))convert(prop["min"])) \
        var = convert(prop["min"]);\
    if(prop["max"] && var > (decltype(var))convert(prop["max"])) \
        var = convert(prop["max"]);

#define rTYPE(n) decltype(obj->n)

#define rAPPLY(n,t) if(obj->n != var) data.reply("undo_change", "s" #t #t, data.loc, obj->n, var); obj->n = var;

#define rParamCb(name) rBOIL_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "c", obj->name); \
        } else { \
            rTYPE(name) var = rtosc_argument(msg, 0).i; \
            rLIMIT(var, atoi) \
            rAPPLY(name, c) \
            data.broadcast(loc, "c", obj->name);\
            rChangeCb \
        } rBOIL_END

#define rParamFCb(name) rBOIL_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "f", obj->name); \
        } else { \
            rTYPE(name) var = rtosc_argument(msg, 0).f; \
            rLIMIT(var, atof) \
            rAPPLY(name, f) \
            data.broadcast(loc, "f", obj->name);\
            rChangeCb \
        } rBOIL_END

#define rParamICb(name) rBOIL_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "i", obj->name); \
        } else { \
            rTYPE(name) var = rtosc_argument(msg, 0).i; \
            rLIMIT(var, atoi) \
            rAPPLY(name, i) \
            data.broadcast(loc, "i", obj->name);\
            rChangeCb \
        } rBOIL_END

//TODO finish me (include string mapper action?)
#define rOptionCb(name) rBOIL_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "i", obj->name); \
        } else { \
            rTYPE(name) var = rtosc_argument(msg, 0).i; \
            rLIMIT(var, atoi) \
            rAPPLY(name, i) \
            data.broadcast(loc, rtosc_argument_string(msg), obj->name);\
            rChangeCb \
        } rBOIL_END

#define rToggleCb(name) rBOIL_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, obj->name ? "T" : "F"); \
        } else { \
            if(obj->name != rtosc_argument(msg, 0).T) { \
                data.broadcast(loc, args);\
                obj->name = rtosc_argument(msg, 0).T; \
                rChangeCb \
            } \
        } rBOIL_END

#define SNIP \
    while(*msg && *msg!='/') ++msg; \
    msg = *msg ? msg+1 : msg;

#define rRecurCb(name) rBOIL_BEGIN \
    data.obj = &obj->name; \
    SNIP \
    decltype(obj->name)::ports.dispatch(msg, data); \
    rBOIL_END

#define rRecurPtrCb(name) rBOIL_BEGIN \
    void *ptr = &obj->name; \
    data.reply(loc, "b", sizeof(void*), &ptr); \
    rBOIL_END

#define rRecurpCb(name) rBOIL_BEGIN \
    if(obj->name == NULL) return; \
    data.obj = obj->name; \
    SNIP \
    decltype(spice(rObject::name))::ports.dispatch(msg, data); \
    rBOIL_END

#define rRecursCb(name, length) rBOILS_BEGIN \
    data.obj = &obj->name[idx]; \
    SNIP \
    decltype(spice(rObject::name))::ports.dispatch(msg, data); \
    rBOILS_END

#define rRecurspCb(name) rBOILS_BEGIN \
    data.obj = obj->name[idx]; \
    SNIP \
    decltype(spice(rObject::name[0]))::ports.dispatch(msg, data); \
    rBOILS_END

#define rActionCb(name) rBOIL_BEGIN obj->name(); rBOIL_END
#define rActioniCb(name) rBOIL_BEGIN \
    obj->name(rtosc_argument(msg,0).i); rBOIL_END

//Array ops

#define rBOILS_BEGIN rBOIL_BEGIN \
            const char *mm = msg; \
            while(*mm && !isdigit(*mm)) ++mm; \
            unsigned idx = atoi(mm);

#define rBOILS_END rBOIL_END


#define rArrayCb(name) rBOILS_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "c", obj->name[idx]); \
        } else { \
            char var = rtosc_argument(msg, 0).i; \
            rLIMIT(var, atoi) \
            rAPPLY(name[idx], c) \
            data.broadcast(loc, "c", obj->name[idx]);\
            rChangeCb \
        } rBOILS_END

#define rArrayFCb(name) rBOILS_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "f", obj->name[idx]); \
        } else { \
            float var = rtosc_argument(msg, 0).f; \
            rLIMIT(var, atof) \
            rAPPLY(name[idx], f) \
            data.broadcast(loc, "f", obj->name[idx]);\
        } rBOILS_END

#define rArrayTCb(name) rBOILS_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, obj->name[idx] ? "T" : "F"); \
        } else { \
            if(obj->name[idx] != rtosc_argument(msg, 0).T) { \
                data.broadcast(loc, args);\
                rChangeCb \
            } \
            obj->name[idx] = rtosc_argument(msg, 0).T; \
        } rBOILS_END

#define rArrayICb(name) rBOILS_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "i", obj->name[idx]); \
        } else { \
            char var = rtosc_argument(msg, 0).i; \
            rLIMIT(var, atoi) \
            rAPPLY(name[idx], i) \
            data.broadcast(loc, "i", obj->name[idx]);\
            rChangeCb \
        } rBOILS_END

#define rParamsCb(name, length) rBOIL_BEGIN \
    data.reply(loc, "b", length, obj->name); rBOIL_END

#define rStringCb(name, length) rBOIL_BEGIN \
        if(!strcmp("", args)) {\
            data.reply(loc, "s", obj->name); \
        } else { \
            strncpy(obj->name, rtosc_argument(msg, 0).s, length); \
            data.broadcast(loc, "s", obj->name);\
            rChangeCb \
        } rBOIL_END


#endif
