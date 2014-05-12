//Copyright (c) 2002-2003 Nasca Octavian Paul
//License: GNU GPL 2

#include <pthread.h>
#include "Spliter.h"
#include "SpliterUI.h"

pthread_t thr1, thr2;
Spliter   spliter;

void *thread1(void *arg) {
    Fl::run();
    return 0;
}
void *thread2(void *arg) {
    while(Pexitprogram == 0)
        spliter.midievents();
    return 0;
}


main()
{
    Pexitprogram = 0;
    SpliterUI *spliterUI = new SpliterUI(&spliter);

    pthread_mutex_init(&mutex, NULL);
    pthread_create(&thr1, NULL, thread1, NULL);
    pthread_create(&thr2, NULL, thread2, NULL);

    while(Pexitprogram == 0) {
        usleep(100000);
    }

    pthread_mutex_destroy(&mutex);
    delete spliterUI;
};
