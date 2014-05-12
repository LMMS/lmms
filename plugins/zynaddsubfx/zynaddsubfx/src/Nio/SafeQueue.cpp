
template<class T>
SafeQueue<T>::SafeQueue(size_t maxlen)
    :writePtr(0), readPtr(0), bufSize(maxlen)
{
    sem_init(&w_space, PTHREAD_PROCESS_PRIVATE, maxlen - 1);
    sem_init(&r_space, PTHREAD_PROCESS_PRIVATE, 0);
    buffer = new T[maxlen];
}

template<class T>
SafeQueue<T>::~SafeQueue()
{
    sem_destroy(&w_space);
    sem_destroy(&r_space);
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
    int space = 0;
    sem_getvalue(&r_space, &space);
    return space;
}

template<class T>
unsigned int SafeQueue<T>::wSpace() const
{
    int space = 0;
    sem_getvalue(&w_space, &space);
    return space;
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
    sem_wait(&w_space); //guaranteed not to wait
    sem_post(&r_space);
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
    sem_wait(&r_space); //guaranteed not to wait
    sem_post(&w_space);
    return 0;
}

template<class T>
void SafeQueue<T>::clear()
{
    //thread unsafe
    while(!sem_trywait(&r_space))
        sem_post(&w_space);
    readPtr = writePtr;
}
