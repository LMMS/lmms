#include "VuMeter.h"
#include "Fl_Osc_Interface.h"
#define MIN_DB (-48)

class VuMasterMeter: public VuMeter
{
    public:
        VuMasterMeter(int x,int y, int w, int h, const char *label=0)
        :VuMeter(x,y,w,h,label),
        olddbl(0.0f),olddbr(0.0f),
        oldrmsdbl(0.0f),oldrmsdbr(0.0f),
        dbl(0.0f),dbr(0.0f),rmsdbl(0.0f),rmsdbr(0.0f),maxdbl(0.0f),maxdbr(0.0f),
        clipped(0),osc(NULL)
        {}

        void init(Fl_Osc_Interface *_osc)
        {
            osc = _osc;
        }

        int handle(int event)
        {
            switch(event){
                case FL_SHOW:
                    Fl::add_timeout(1.0/18.0,tick,osc);
                    break;
                case FL_HIDE:
                    Fl::remove_timeout(tick,osc);
                    break;
                case FL_PUSH:
                    osc->requestValue("/reset-vu");
                    return 1;
            }

            return 0;
        }

        static void tick(void *v)
        {
            Fl::repeat_timeout(1.0/18.0,tick,v);//18 fps
            Fl_Osc_Interface *osc = (Fl_Osc_Interface*)v;
            osc->requestValue("/get-vu");
        }

        void draw(void)
        {
            const int X = x(), Y = y(), W = w(), H = h();

#define VULENX (W-35)
#define VULENY (H/2-3)

            const int idbl    = dbl*VULENX;
            const int idbr    = dbr*VULENX;
            const int irmsdbl = rmsdbl*VULENX;
            const int irmsdbr = rmsdbr*VULENX;

            //draw the vu-meter lines
            //dB
            fl_rectf(X,Y,idbr,VULENY,0,200,255);
            fl_rectf(X,Y+H/2,idbl,VULENY,0,200,255);
            //black
            fl_rectf(X+idbr,Y,VULENX-idbr,VULENY,0,0,0);
            fl_rectf(X+idbl,Y+H/2,VULENX-idbl,VULENY,0,0,0);

            //draw the scales
            const float  tmp=VULENX*1.0/MIN_DB;
            for (int i=1;i<1-MIN_DB;i++){
                const int tx=VULENX+(int) (tmp*i);
                fl_rectf(X+tx,Y,1,VULENY+H/2,0,160,200);
                if (i%5==0) fl_rectf(X+tx,Y,1,VULENY+H/2,0,230,240);
                if (i%10==0) fl_rectf(X+tx-1,Y,2,VULENY+H/2,0,225,255);
            }

            //rms
            if (irmsdbr>2) fl_rectf(X+irmsdbr-1,Y,3,VULENY,255,255,0);
            if (irmsdbl>2) fl_rectf(X+irmsdbl-1,Y+H/2,3,VULENY,255,255,0);


            //draw the red box if clipping has occured
            if (clipped==0) fl_rectf(X+VULENX+2,Y+1,W-VULENX-3,H-4,0,0,10);
            else fl_rectf(X+VULENX+2,Y+1,W-VULENX-3,H-4,250,10,10);

            //draw the maxdB
            fl_font(FL_HELVETICA|FL_BOLD,10);
            fl_color(255,255,255);

            char tmpstr[10];
            if(maxdbl>MIN_DB-20){
                snprintf((char *)&tmpstr,10,"%ddB",(int)maxdbr);
                fl_draw(tmpstr,X+VULENX+1,Y+1,W-VULENX-1,VULENY,FL_ALIGN_RIGHT,NULL,0);
            }
            if(maxdbr>MIN_DB-20){
                snprintf((char *)&tmpstr,10,"%ddB",(int)maxdbl);
                fl_draw(tmpstr,X+VULENX+1,Y+H/2+1,W-VULENX-1,VULENY,FL_ALIGN_RIGHT,NULL,0);
            }
        }

        void update(vuData *d)
        {
            //Update properties
            dbl     = limit((MIN_DB-rap2dB(d->outpeakl))/MIN_DB);
            dbr     = limit((MIN_DB-rap2dB(d->outpeakr))/MIN_DB);
            rmsdbl  = limit((MIN_DB-rap2dB(d->rmspeakl))/MIN_DB);
            rmsdbr  = limit((MIN_DB-rap2dB(d->rmspeakr))/MIN_DB);
            maxdbl  = rap2dB(d->maxoutpeakl);
            maxdbr  = rap2dB(d->maxoutpeakr);
            clipped = d->clipped;

            //interpolate
            dbl    = 0.4 * dbl    + 0.6 * olddbl;
            dbr    = 0.4 * dbr    + 0.6 * olddbr;

            rmsdbl = 0.4 * rmsdbl + 0.6 * oldrmsdbl;
            rmsdbr = 0.4 * rmsdbr + 0.6 * oldrmsdbr;

            //only update when values appear to be different
            if(olddbl == dbl && olddbr == dbr)
                return;

            olddbl    = dbl;
            olddbr    = dbr;
            oldrmsdbl = rmsdbl;
            oldrmsdbr = rmsdbr;

            damage(FL_DAMAGE_USER1);
        }
    private:
        float olddbl,olddbr;
        float oldrmsdbl,oldrmsdbr;
        float dbl,dbr,rmsdbl,rmsdbr,maxdbl,maxdbr;
        int clipped;

        Fl_Osc_Interface *osc;
};
