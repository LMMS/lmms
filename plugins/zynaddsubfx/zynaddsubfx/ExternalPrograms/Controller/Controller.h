#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <alsa/asoundlib.h>

extern pthread_mutex_t mutex;
extern int Pexitprogram;

class Controller
{
    public:
        Controller();
        ~Controller();
        void send(int npar, float xval);
        //parameters
        unsigned char Pchout;
        struct {
            unsigned char mode; //0=off,1=ctl,2=RPN,3=NRPN
            unsigned char val1, val2;
            struct {
                unsigned char par;
            } ctl;
            struct {
                unsigned char cpar, fpar, cval;
            } nrpn;
        } pars[6];
    private:
        void sendcontroller(int par, unsigned char val);
        void sendnrpn(int npar, unsigned char val);

        snd_seq_t *midi_out;
};

#endif
