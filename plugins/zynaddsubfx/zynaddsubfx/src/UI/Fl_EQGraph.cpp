#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include "Fl_EQGraph.H"
#include "common.H"
#include "../Effects/EffectMgr.h"
#include "../DSP/FFTwrapper.h"
#include "../globals.h"

#include <rtosc/rtosc.h>

#define MAX_DB 30

Fl_EQGraph::Fl_EQGraph(int x,int y, int w, int h, const char *label)
    :Fl_Box(x,y,w,h,label), Fl_Osc_Widget(this)
{
    memset(num, 0, sizeof(num));
    memset(dem, 0, sizeof(dem));
    num[0] = 1;
    dem[0] = 1;
    ext = "eq-coeffs";
    oscRegister("eq-coeffs");
}

Fl_EQGraph::~Fl_EQGraph(void)
{}

void Fl_EQGraph::OSC_raw(const char *msg)
{
    memcpy(dem, rtosc_argument(msg, 0).b.data, sizeof(dem));
    memcpy(num, rtosc_argument(msg, 1).b.data, sizeof(dem));
    redraw();
}

void Fl_EQGraph::update(void)
{
    oscWrite("eq-coeffs");
}

void Fl_EQGraph::draw_freq_line(float freq, int type)
{
    fl_color(FL_GRAY);
    float freqx=getfreqpos(freq);
    switch(type){
        case 0:if (active_r()) fl_color(FL_WHITE);
                   else fl_color(205,205,205);
                   fl_line_style(FL_SOLID);
                   break;
        case 1:fl_line_style(FL_DOT);break;
        case 2:fl_line_style(FL_DASH);break;
    }; 


    if ((freqx>0.0)&&(freqx<1.0))
        fl_line(x()+(int) (freqx*w()),y(),
                x()+(int) (freqx*w()),y()+h());
}

void Fl_EQGraph::draw(void)
{
    int ox=x(),oy=y(),lx=w(),ly=h(),i;
    double iy,oiy;
    float freqx;

    if (active_r()) fl_color(fl_darker(FL_GRAY));
    else fl_color(FL_GRAY);
    fl_rectf(ox,oy,lx,ly);


    //draw the lines
    fl_color(fl_lighter(FL_GRAY));

    fl_line_style(FL_SOLID);
    fl_line(ox+2,oy+ly/2,ox+lx-2,oy+ly/2);

    freqx=getfreqpos(1000.0);
    if ((freqx>0.0)&&(freqx<1.0))
        fl_line(ox+(int) (freqx*lx),oy,
                ox+(int) (freqx*lx),oy+ly);

    for (i=1;i<10;i++) {
        if(i==1) {
            draw_freq_line(i*100.0,0);
            draw_freq_line(i*1000.0,0);
        } else 
            if (i==5) {
                draw_freq_line(i*10.0,2);
                draw_freq_line(i*100.0,2);
                draw_freq_line(i*1000.0,2);
            } else {
                draw_freq_line(i*10.0,1);
                draw_freq_line(i*100.0,1);
                draw_freq_line(i*1000.0,1);
            };
    };

    draw_freq_line(10000.0,0);
    draw_freq_line(20000.0,1);


    fl_line_style(FL_DOT);
    int GY=6;if (ly<GY*3) GY=-1;
    for (i=1;i<GY;i++){
        int tmp=(int)(ly/(float)GY*i);
        fl_line(ox+2,oy+tmp,ox+lx-2,oy+tmp);
    };


    //draw the frequency response
    if (active_r()) fl_color(FL_YELLOW);
    else fl_color(200,200,80);
    fl_line_style(FL_SOLID,2);
    //fl_color( fl_color_add_alpha( fl_color(), 127 ) );
    oiy=getresponse(ly,getfreqx(0.0));
    fl_begin_line();
    for (i=1;i<lx;i++){
        float frq=getfreqx(i/(float) lx);
        if (frq>synth->samplerate/2) break;
        iy=getresponse(ly,frq);
        if ((oiy>=0) && (oiy<ly) &&
                (iy>=0) && (iy<ly) )
            fl_vertex(ox+i,oy+ly-iy);
        oiy=iy;
    };
    fl_end_line();
    fl_line_style(FL_SOLID,0);
}

/*
 * For those not too familiar with digital filters, what is happening here is an
 * evaluation of the filter's frequency response through the evaluation of
 * H(z^{-1}) via z^{-1}=e^{j\omega}.
 * This will yield a complex result which will indicate the phase and magnitude
 * transformation of the input at the set frequency denoted by \omega
 */
double Fl_EQGraph::getresponse(int maxy,float freq) const
{
    const float angle = 2*PI*freq/synth->samplerate_f;
    std::complex<float> num_res = 0;
    std::complex<float> dem_res = 0;
         

    for(int i = 0; i < MAX_EQ_BANDS*MAX_FILTER_STAGES*2+1; ++i) {
        num_res += FFTpolar<float>(num[i], i*angle);
        dem_res += FFTpolar<float>(dem[i], i*angle);
    }

    float dbresp=20*log(abs(num_res/dem_res))/log(10);

    //rescale
    return (int) ((dbresp/MAX_DB+1.0)*maxy/2.0);
}

float Fl_EQGraph::getfreqx(float x) const
{
    if(x>1.0)
        x=1.0;
    return(20.0*pow((float)1000.0,x));
}

float Fl_EQGraph::getfreqpos(float freq) const
{
    if(freq<0.00001)
        freq=0.00001;
    return(log(freq/20.0)/log(1000.0));
}
