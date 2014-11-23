
template<class T>
SafeQueue<T>::SafeQueue(size_t maxlen)
    :writePtr(0), readPtr(0), bufSize(maxlen)
{
    w_space.init(PTHREAD_PROCESS_PRIVATE, maxlen - 1);
    r_space.init(PTHREAD_PROCESS_PRIVATE, 0);
    buffer = new T[maxlen];
}

template<class T>
SafeQueue<T>::~SafeQueue()
{
    delete [] buffer;
}

template<class T>
unsigned int SafeQueue<T>::size() const
{
    return rSpace();
}

template<class T>
unsigned int SafeQueue<T>::rSpace() const
{
    return r_space.getvalue();
}

template<class T>
unsigned int SafeQueue<T>::wSpace() const
{
    return w_space.getvalue();
}

template<class T>
int SafeQueue<T>::push(const T &in)
{
    if(!wSpace())
        return -1;

    //ok, there is space to write
    size_t w = (writePtr + 1) % bufSize;
    buffer[w] = in;
    writePtr  = w;

    //adjust ranges
    w_space.wait(); //guaranteed not to wait
    r_space.post();
    return 0;
}

template<class T>
int SafeQueue<T>::peak(T &out) const
{
    if(!rSpace())
        return -1;

    //ok, there is space to read
    size_t r = (readPtr + 1) % bufSize;
    out     = buffer[r];

    return 0;
}

template<class T>
int SafeQueue<T>::pop(T &out)
{
    if(!rSpace())
        return -1;

    //ok, there is space to read
    size_t r = (readPtr + 1) % bufSize;
    out     = buffer[r];
    readPtr = r;

    //adjust ranges
    r_space.wait(); //guaranteed not to wait
    w_space.post();
    return 0;
}

template<class T>
void SafeQueue<T>::clear()
{
    //thread unsafe
    while(!r_space.trywait())
        w_space.post();
    readPtr = writePtr;
}
