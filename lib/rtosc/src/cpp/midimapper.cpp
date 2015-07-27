
#include <rtosc/ports.h>
#include <cstring>
#include <algorithm>
#include <map>
#include <sstream>
#include <deque>
#include <utility>

#include <cassert>
#include <rtosc/miditable.h>

using namespace rtosc;
using std::string;
using std::get;
using std::tuple;
using std::make_tuple;

/********************
 * Helper Templates *
 ********************/

template<class T, class U>
bool has_t(const T &t, const U&u)
{return t.find(u) != t.end();}

template<class T, class U>
bool has2(const T &t, const U&u) {
    for(const U&uu:t)
        if(uu==u)
            return true;
    return false;
}
template<class T,class U>
int getInd(const T&t,const U&u){
    int i=0;
    for(const U&uu:t) {
        if(uu==u)
            return i;
        else
            i++;
    }
    return -1;
}

/***********
 * Storage *
 ***********/

bool MidiMapperStorage::handleCC(int ID, int val, write_cb write)
{
    for(int i=0; i<mapping.size(); ++i)
    {
        if(std::get<0>(mapping[i]) == ID)
        {
            bool coarse = std::get<1>(mapping[i]);
            int  ind    = std::get<2>(mapping[i]);
            if(coarse)
                values[ind] = (val<<7)|(values[ind]&0x7f);
            else
                values[ind] = val|(values[ind]&0x3f80);
            callbacks[ind](values[ind],write);
            return true;
        }
    }
    return false;
}

//TODO try to change O(n^2) algorithm to O(n)
void MidiMapperStorage::cloneValues(const MidiMapperStorage &storage)
{
    //XXX this method is SUPER error prone
    for(int i=0; i<values.size(); ++i)
        values[i] = 0;

    for(int i=0; i<mapping.size(); ++i) {
        for(int j=0; j<storage.mapping.size(); ++j) {
            if(std::get<0>(mapping[i]) == std::get<0>(storage.mapping[j])) {
                bool coarse_src = std::get<1>(storage.mapping[j]);
                int ind_src     = std::get<2>(storage.mapping[j]);

                bool coarse_dest = std::get<1>(mapping[i]);
                int ind_dest     = std::get<2>(mapping[i]);

                int val = 0;
                //Extract
                if(coarse_src)
                    val = storage.values[ind_src]>>7;
                else
                    val = storage.values[ind_src]&0x7f;

                //Blit
                if(coarse_dest)
                    values[ind_dest] = (val<<7)|(values[ind_dest]&0x7f);
                else
                    values[ind_dest] = val|(values[ind_dest]&0x3f80);
            }
        }
    }
}

MidiMapperStorage *MidiMapperStorage::clone(void)
{
    MidiMapperStorage *nstorage = new MidiMapperStorage();
    nstorage->values    = values.sized_clone();
    nstorage->mapping   = mapping.clone();
    nstorage->callbacks = callbacks.clone();
    return nstorage;
}

int MidiBijection::operator()(float x) const {
    if(mode == 0)
        return ((x-min)/(max-min))*(1<<14);
    else
        return 0;
}

float MidiBijection::operator()(int x) const {
    if(mode == 0)
        return x/((1<<14)*1.0)*(max-min)+min;
    else
        return 0;
}


/************************
 * Non realtime portion *
 ************************/
MidiMappernRT::MidiMappernRT(void)
    :storage(0),base_ports(0)
{}

void MidiMappernRT::map(const char *addr, bool coarse)
{
    for(auto x:learnQueue)
        if(x.first == addr && x.second == coarse)
            return;

    unMap(addr, coarse);
    learnQueue.push_back(std::make_pair(addr,coarse));
    char buf[1024];
    rtosc_message(buf, 1024, "/midi-add-watch","");
    rt_cb(buf);
}
        
MidiMapperStorage *MidiMappernRT::generateNewBijection(const Port &port, std::string addr)
{
    MidiBijection bi;
    bi.mode = 0;
    bi.min = atof(port.meta()["min"]);
    bi.max = atof(port.meta()["max"]);
    auto tmp = [bi,addr](int16_t x, MidiMapperStorage::write_cb cb) {
        float out = bi(x);
        char buf[1024];
        rtosc_message(buf, 1024, addr.c_str(), "f", out);
        cb(buf);
    };

    MidiMapperStorage *nstorage = new MidiMapperStorage();
    if(storage) {
        //XXX not quite
        nstorage->values    = storage->values.one_larger();
        nstorage->mapping   = storage->mapping.clone();//insert(std::make_tuple(ID, true, storage->callbacks.size()));
        nstorage->callbacks = storage->callbacks.insert(tmp);
    } else {
        nstorage->values    = nstorage->values.insert(0);
        nstorage->mapping   = nstorage->mapping.clone();//insert(std::make_tuple(ID, true, 0));
        nstorage->callbacks = nstorage->callbacks.insert(tmp);
    }
    inv_map[addr] = std::make_tuple(nstorage->callbacks.size()-1, -1,-1,bi);
    return nstorage;
}

void MidiMappernRT::addNewMapper(int ID, const Port &port, std::string addr)
{
    MidiBijection bi;
    bi.mode = 0;
    bi.min = atof(port.meta()["min"]);
    bi.max = atof(port.meta()["max"]);
    auto tmp = [bi,addr](int16_t x, MidiMapperStorage::write_cb cb) {
        float out = bi(x);
        char buf[1024];
        rtosc_message(buf, 1024, addr.c_str(), "f", out);
        cb(buf);
    };

    MidiMapperStorage *nstorage = new MidiMapperStorage();
    if(storage) {
        //XXX not quite
        nstorage->values    = storage->values.one_larger();
        nstorage->mapping   = storage->mapping.insert(std::make_tuple(ID, true, storage->callbacks.size()));
        nstorage->callbacks = storage->callbacks.insert(tmp);
    } else {
        nstorage->values    = nstorage->values.insert(0);
        nstorage->mapping   = nstorage->mapping.insert(std::make_tuple(ID, true, 0));
        nstorage->callbacks = nstorage->callbacks.insert(tmp);
    }
    storage = nstorage;
    inv_map[addr] = std::make_tuple(storage->callbacks.size()-1, ID,-1,bi);
}

void MidiMappernRT::addFineMapper(int ID, const Port &port, std::string addr)
{
    (void) port;
    //TODO asserts
    //Bijection already created
    //Coarse node already active
    //Value already allocated

    //Find mapping
    int mapped_ID = std::get<0>(inv_map[addr]);
    std::get<2>(inv_map[addr]) = ID;
    MidiMapperStorage *nstorage = new MidiMapperStorage();
    nstorage->values    = storage->values.sized_clone();
    nstorage->mapping   = storage->mapping.insert(std::make_tuple(ID, false, mapped_ID));
    nstorage->callbacks = storage->callbacks.insert(storage->callbacks[mapped_ID]);
    storage = nstorage;
}

void killMap(int ID, MidiMapperStorage &m)
{
    MidiMapperStorage::TinyVector<tuple<int, bool, int>> nmapping(m.mapping.size()-1);
    int j=0;
    for(int i=0; i<m.mapping.size(); i++)
        if(get<0>(m.mapping[i]) != ID)
            nmapping[j++] = m.mapping[i];
    assert(j == nmapping.size());
    m.mapping = nmapping;
}

void MidiMappernRT::useFreeID(int ID)
{
    if(learnQueue.empty())
        return;
    std::string addr = std::get<0>(learnQueue.front());
    bool coarse      = std::get<1>(learnQueue.front());
    
    learnQueue.pop_front();
    assert(base_ports);
    const rtosc::Port *p = base_ports->apropos(addr.c_str());
    assert(p);

    MidiMapperStorage *nstorage;
    if(inv_map.find(addr) == inv_map.end())
        nstorage = generateNewBijection(*p, addr);
    else
        nstorage = storage->clone();

    auto imap = inv_map[addr];
    int mapped_ID = std::get<0>(imap);
    nstorage->mapping = nstorage->mapping.insert(make_tuple(ID, coarse, mapped_ID));

    if(coarse) {
        if(get<1>(imap) != -1)
            killMap(get<1>(imap), *nstorage);
        inv_map[addr] = make_tuple(get<0>(imap), ID, get<2>(imap), get<3>(imap));
    } else {
        if(get<2>(imap) != -1)
            killMap(get<1>(imap), *nstorage);
        inv_map[addr] = make_tuple(get<0>(imap), get<1>(imap), ID, get<3>(imap));
    }
    storage = nstorage;

    //TODO clean up unused value and callback objects

    char buf[1024];
    rtosc_message(buf, 1024, "/midi-bind", "b", sizeof(storage), &storage);
    rt_cb(buf);
};

void MidiMappernRT::unMap(const char *addr, bool coarse)
{
    printf("Unmapping('%s',%d)\n",addr,coarse);
    if(inv_map.find(addr) == inv_map.end())
        return;
    auto imap = inv_map[addr];

    int kill_id = -1;
    if(coarse) {
        kill_id = get<1>(imap);
        inv_map[addr] = make_tuple(get<0>(imap), -1, get<2>(imap), get<3>(imap));
    } else {
        kill_id = get<2>(imap);
        inv_map[addr] = make_tuple(get<0>(imap), get<1>(imap), -1,  get<3>(imap));
    }

    if(kill_id == -1)
        return;



    MidiMapperStorage *nstorage = storage->clone();
    killMap(kill_id, *nstorage);
    storage = nstorage;

    //TODO clean up unused value and callback objects

    char buf[1024];
    rtosc_message(buf, 1024, "/midi-bind", "b", sizeof(storage), &storage);
    rt_cb(buf);
}

void MidiMappernRT::delMapping(int ID, bool coarse, const char *addr){
    (void) ID;
    (void) coarse;
    (void) addr;
};
void MidiMappernRT::replaceMapping(int, bool, const char *){};



std::map<std::string, std::string> MidiMappernRT::getMidiMappingStrings(void)
{
    std::map<std::string, std::string> result;
    for(auto s:inv_map)
        result[s.first] = getMappedString(s.first);
    char ID = 'A';
    for(auto s:learnQueue)
    {
        if(s.second == false)
            result[s.first] += std::string(":")+ID++;
        else
            result[s.first] = ID++;
    }
    return result;
}

//unclear if this should be be here as a helper or not
std::string MidiMappernRT::getMappedString(std::string addr)
{
    std::stringstream out;
    //find coarse
    if(has_t(inv_map,addr)) {
        if(std::get<1>(inv_map[addr]) != -1)
            out << std::get<1>(inv_map[addr]);
    }else if(has2(learnQueue, make_pair(addr,true)))
        out << getInd(learnQueue,std::make_pair(addr,true));
    //find Fine
    if(has_t(inv_map,addr)) {
        if(std::get<2>(inv_map[addr]) != -1)
            out << ":" << std::get<2>(inv_map[addr]);
    } else if(has2(learnQueue, make_pair(addr,false)))
        out << getInd(learnQueue,std::make_pair(addr,false));

    return out.str();
}

MidiBijection MidiMappernRT::getBijection(std::string s)
{
    return std::get<3>(inv_map[s]);
}

void MidiMappernRT::snoop(const char *msg)
{
    if(inv_map.find(msg) != inv_map.end())
    {
        auto apple = inv_map[msg];
        MidiBijection bi = getBijection(msg);
        float value = 0;
        std::string args = rtosc_argument_string(msg);
        if(args == "f")
            value = rtosc_argument(msg, 0).f;
        else if(args == "i")
            value = rtosc_argument(msg, 0).i;
        else if(args == "T")
            value = 1.0;
        else if(args == "F")
            value = 0.0;
        else
            return;

        int new_midi = bi(value);
        //printf("--------------------------------------------\n");
        //printf("msg = '%s'\n", msg);
        //printf("--------------------------------------------\n");
        //printf("new midi value: %f->'%x'\n", value, new_midi);
        if(std::get<1>(apple) != -1)
            apply_high(new_midi,std::get<1>(apple));
        if(std::get<2>(apple) != -1)
            apply_low(new_midi,std::get<2>(apple));
    }
};

void MidiMappernRT::apply_high(int v, int ID) { apply_midi(v>>7,ID); }
void MidiMappernRT::apply_low(int v, int ID)  { apply_midi(0x7f&v,ID);}
void MidiMappernRT::apply_midi(int val, int ID)
{
    char buf[1024];
    rtosc_message(buf,1024,"/virtual_midi_cc","ii",val,ID);
    rt_cb(buf);
}

void MidiMappernRT::setBounds(const char *str, float low, float high)
{
    if(inv_map.find(str) == inv_map.end())
        return;
    string addr  = str;
    auto imap    = inv_map[str];
    auto newBi   = MidiBijection{0,low,high};
    inv_map[str] = make_tuple(get<0>(imap),get<1>(imap),get<2>(imap),newBi);
    MidiMapperStorage *nstorage = storage->clone();

    nstorage->callbacks[get<0>(imap)] = [newBi,addr](int16_t x, MidiMapperStorage::write_cb cb) {
        float out = newBi(x);
        char buf[1024];
        rtosc_message(buf, 1024, addr.c_str(), "f", out);
        cb(buf);
    };

    storage = nstorage;
    
    char buf[1024];
    rtosc_message(buf, 1024, "/midi-bind", "b", sizeof(storage), &storage);
    rt_cb(buf);
}

std::tuple<float,float,float,float> MidiMappernRT::getBounds(const char *str)
{
    const rtosc::Port *p = base_ports->apropos(str);
    assert(p);
    float min_val = atof(p->meta()["min"]);
    float max_val = atof(p->meta()["max"]);
    if(inv_map.find(str) != inv_map.end()) {
        auto elm = std::get<3>(inv_map[str]);
        return std::make_tuple(min_val, max_val,elm.min,elm.max);
    }
    return std::make_tuple(min_val, max_val,-1.0f,-1.0f);
}

bool MidiMappernRT::has(std::string addr)
{
    return inv_map.find(addr) != inv_map.end();
}

bool MidiMappernRT::hasPending(std::string addr)
{
    for(auto s:learnQueue)
        if(s.first == addr)
            return true;
    return false;
}

bool MidiMappernRT::hasCoarse(std::string addr)
{
    if(!has(addr))
        return false;
    auto e = inv_map[addr];
    return std::get<1>(e) != -1;
}

bool MidiMappernRT::hasFine(std::string addr)
{
    if(!has(addr))
        return false;
    auto e = inv_map[addr];
    return std::get<2>(e) != -1;
}

bool MidiMappernRT::hasCoarsePending(std::string addr)
{
    for(auto s:learnQueue)
        if(s.first == addr && s.second)
            return true;
    return false;
}

bool MidiMappernRT::hasFinePending(std::string addr)
{
    for(auto s:learnQueue)
        if(s.first == addr && !s.second)
            return true;
    return false;
}

int MidiMappernRT::getCoarse(std::string addr)
{
    if(!has(addr))
        return -1;
    auto e = inv_map[addr];
    return std::get<1>(e);
}

int MidiMappernRT::getFine(std::string addr)
{
    if(!has(addr))
        return -1;
    auto e = inv_map[addr];
    return std::get<2>(e);
}

/*****************
 * Realtime code *
 *****************/

MidiMapperRT::MidiMapperRT(void)
:storage(NULL), watchSize(0)
{}
void MidiMapperRT::setBackendCb(std::function<void(const char*)> cb) {backend = cb;}
void MidiMapperRT::setFrontendCb(std::function<void(const char*)> cb) {frontend = cb;}
void MidiMapperRT::handleCC(int ID, int val) { 
    if((!storage || !storage->handleCC(ID, val, backend)) && !pending.has(ID) && watchSize) {
        watchSize--;
        pending.insert(ID);
        char msg[1024];
        rtosc_message(msg, 1024, "/midi-use-CC", "i", ID);
        frontend(msg);
    }
}
void MidiMapperRT::addWatch(void) {watchSize++;}
void MidiMapperRT::remWatch(void) {if(watchSize) watchSize--;}
Port MidiMapperRT::addWatchPort(void) {
    return Port{"midi-add-watch","",0, [this](msg_t, RtData&) {
        this->addWatch();
    }};
}
Port MidiMapperRT::removeWatchPort(void) {
    return Port{"midi-remove-watch","",0, [this](msg_t, RtData&) {
        this->remWatch();
    }};
}
Port MidiMapperRT::bindPort(void) {
    return Port{"midi-bind:b","",0, [this](msg_t msg, RtData&) {
        pending.pop();
        MidiMapperStorage *nstorage =
            *(MidiMapperStorage**)rtosc_argument(msg,0).b.data;
        if(storage) {
            nstorage->cloneValues(*storage);
            storage = nstorage;
        } else
            storage = nstorage;
        //TODO memory deallocation
    }};
}
