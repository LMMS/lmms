#include "EnvelopeFreeEdit.h"
#include "../Misc/Util.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <cstdlib>
#include <cassert>
#include <rtosc/rtosc.h>

EnvelopeFreeEdit::EnvelopeFreeEdit(int x,int y, int w, int h, const char *label)
:Fl_Box(x,y,w,h,label), Fl_Osc_Widget(this)
{
    pair=NULL;
    currentpoint=-1;
    cpx=0;
    lastpoint=-1;
}

void EnvelopeFreeEdit::init(void)
{
    oscRegister("Penvpoints");
    oscRegister("Penvdt");
    oscRegister("Penvval");
    oscRegister("Penvsustain");
}

void EnvelopeFreeEdit::OSC_raw(const char *msg)
{
    const char *args = rtosc_argument_string(msg);
    if(strstr(msg,"Penvpoints") && !strcmp(args, "c")) {
        Penvpoints = rtosc_argument(msg, 0).i;
    } else if(strstr(msg,"Penvdt") && !strcmp(args, "b")) {
        rtosc_blob_t b = rtosc_argument(msg, 0).b;
        assert(b.len == MAX_ENVELOPE_POINTS);
        memcpy(Penvdt, b.data, MAX_ENVELOPE_POINTS);
    } else if(strstr(msg,"Penvval") && !strcmp(args, "b")) {
        rtosc_blob_t b = rtosc_argument(msg, 0).b;
        assert(b.len == MAX_ENVELOPE_POINTS);
        memcpy(Penvval, b.data, MAX_ENVELOPE_POINTS);
    } else if(strstr(msg,"Penvsustain") && !strcmp(args, "c")) {
        Penvsustain = rtosc_argument(msg, 0).i;
    }
    redraw();
    do_callback();
}

void EnvelopeFreeEdit::setpair(Fl_Box *pair_)
{
    pair=pair_;
}

int EnvelopeFreeEdit::getpointx(int n) const
{
    const int lx=w()-10;
    int npoints=Penvpoints;

    float  sum=0;
    for(int i=1; i<npoints; ++i)
        sum+=getdt(i)+1;

    float sumbefore=0;//the sum of all points before the computed point
    for(int i=1; i<=n; ++i)
        sumbefore+=getdt(i)+1;

    return (int) (sumbefore/(float) sum*lx);
}

int EnvelopeFreeEdit::getpointy(int n) const
{
    const int ly=h()-10;

    return (1.0-Penvval[n]/127.0)*ly;
}

int EnvelopeFreeEdit::getnearest(int x,int y) const
{
    x-=5;y-=5;

    int nearestpoint=0;
    int nearestval=1000000;//a big value
    for(int i=0; i<Penvpoints; ++i){
        int distance=abs(x-getpointx(i))+abs(y-getpointy(i));
        if (distance<nearestval) {
            nearestpoint=i;
            nearestval=distance;
        }
    }

    return nearestpoint;
}

float EnvelopeFreeEdit::getdt(int i) const
{
    return EnvelopeParams::dt(Penvdt[i]);
}

void EnvelopeFreeEdit::draw(void)
{
    int ox=x(),oy=y(),lx=w(),ly=h();
    //if (env->Pfreemode==0)
    //    env->converttofree();
    const int npoints=Penvpoints;

    if (active_r()) fl_color(FL_BLACK);
    else fl_color(90,90,90);
    if (!active_r()) currentpoint=-1;

    fl_rectf(ox,oy,lx,ly);

    //Margins
    ox+=5;oy+=5;lx-=10;ly-=10;

    //draw the lines
    fl_color(FL_GRAY);

    fl_line_style(FL_SOLID);
    fl_line(ox+2,oy+ly/2,ox+lx-2,oy+ly/2);

    //draws the evelope points and lines
    Fl_Color alb=FL_WHITE;
    if (!active_r()) alb=fl_rgb_color(180,180,180);
    fl_color(alb);
    int oldxx=0,xx=0,oldyy=0,yy=getpointy(0);
    fl_rectf(ox-3,oy+yy-3,6,6);
    for (int i=1; i<npoints; ++i){
        oldxx=xx;oldyy=yy;
        xx=getpointx(i);yy=getpointy(i);
        if (i==currentpoint) fl_color(FL_RED);
        else fl_color(alb);
        fl_line(ox+oldxx,oy+oldyy,ox+xx,oy+yy);
        fl_rectf(ox+xx-3,oy+yy-3,6,6);
    }

    //draw the last moved point point (if exists)
    if (lastpoint>=0){
        fl_color(FL_CYAN);
        fl_rectf(ox+getpointx(lastpoint)-5,oy+getpointy(lastpoint)-5,10,10);
    }

    //draw the sustain position
    if(Penvsustain>0){
        fl_color(FL_YELLOW);
        xx=getpointx(Penvsustain);
        fl_line(ox+xx,oy+0,ox+xx,oy+ly);
    }

    //Show the envelope duration and the current line duration
    fl_font(FL_HELVETICA|FL_BOLD,10);
    float time=0.0;
    if (currentpoint<=0){
        fl_color(alb);
        for(int i=1; i<npoints; ++i)
            time+=getdt(i);
    } else {
        fl_color(255,0,0);
        time=getdt(currentpoint);
    }
    char tmpstr[20];
    if (time<1000.0)
        snprintf((char *)&tmpstr,20,"%.1fms",time);
    else
        snprintf((char *)&tmpstr,20,"%.2fs",time/1000.0);
    fl_draw(tmpstr,ox+lx-20,oy+ly-10,20,10,FL_ALIGN_RIGHT,NULL,0);
}

int EnvelopeFreeEdit::handle(int event)
{
    const int x_=Fl::event_x()-x();
    const int y_=Fl::event_y()-y();

    if (event==FL_PUSH) {
        currentpoint=getnearest(x_,y_);
        cpx=x_;
        cpdt=Penvdt[currentpoint];
        lastpoint=currentpoint;
        redraw();
        if (pair)
            pair->redraw();
    }

    if (event==FL_RELEASE){
        currentpoint=-1;
        redraw();
        if (pair)
            pair->redraw();
    }

    if (event==FL_DRAG && currentpoint>=0){
        int ny=limit(127-(int) (y_*127.0/h()), 0, 127);

        Penvval[currentpoint]=ny;

        const int dx=(int)((x_-cpx)*0.1);
        const int newdt=limit(cpdt+dx,0,127);

        if(currentpoint!=0)
            Penvdt[currentpoint]=newdt;
        else
            Penvdt[currentpoint]=0;

        oscWrite(to_s("Penvdt")+to_s(currentpoint), "c", newdt);
        redraw();

        if(pair)
            pair->redraw();
    }


    return 1;
}

void EnvelopeFreeEdit::update(void)
{
    oscWrite("Penvpoints");
    oscWrite("Penvdt");
    oscWrite("Penvval");
    oscWrite("Penvsustain");
}
