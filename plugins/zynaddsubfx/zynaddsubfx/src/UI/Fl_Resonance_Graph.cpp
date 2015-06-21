#include "Fl_Resonance_Graph.H"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Value_Output.H>
#include <rtosc/rtosc.h>

Fl_Resonance_Graph::Fl_Resonance_Graph(int x,int y, int w, int h, const char *label)
    :Fl_Box(x,y,w,h,label), Fl_Osc_Widget(this), khzvalue(NULL), dbvalue(NULL),
    oldx(-1), oldy(-1), cbwidget(NULL), applybutton(NULL), Pcenterfreq(0), Poctavesfreq(0),
    PmaxdB(0)
{
    memset(Prespoints, 64, N_RES_POINTS);
    //Get values
    oscRegister("Prespoints");
    oscRegister("Pcenterfreq");
    oscRegister("Poctavesfreq");
    oscRegister("PmaxdB");

    cbwidget=NULL;
    applybutton=NULL;
}

Fl_Resonance_Graph::~Fl_Resonance_Graph(void)
{
}

void Fl_Resonance_Graph::init(Fl_Value_Output *khzvalue_,Fl_Value_Output *dbvalue_)
{
    khzvalue=khzvalue_;
    dbvalue=dbvalue_;
    oldx=-1;
    khzval=-1;
}

void Fl_Resonance_Graph::draw_freq_line(float freq,int type)
{
    const float freqx=getfreqpos(freq);//XXX
    switch(type){
        case 0:fl_line_style(FL_SOLID);break;
        case 1:fl_line_style(FL_DOT);break;
        case 2:fl_line_style(FL_DASH);break;
    }


    if ((freqx>0.0)&&(freqx<1.0))
        fl_line(x()+(int) (freqx*w()),y(),
                x()+(int) (freqx*w()),y()+h());
}

void Fl_Resonance_Graph::draw()
{
    const int ox=x(),oy=y(),lx=w(),ly=h();

    fl_color(FL_DARK1);
    fl_rectf(ox,oy,lx,ly);


    //draw the lines
    fl_color(FL_GRAY);

    fl_line_style(FL_SOLID);
    fl_line(ox+2,oy+ly/2,ox+lx-2,oy+ly/2);


    //Draw 1kHz line
    const float freqx=getfreqpos(1000.0);//XXX
    if ((freqx>0.0)&&(freqx<1.0))
        fl_line(ox+(int) (freqx*lx),oy,
                ox+(int) (freqx*lx),oy+ly);

    //Draw other frequency lines
    for (int i=1; i<10; ++i){
        if(i==1) {
            draw_freq_line(i*100.0,0);
            draw_freq_line(i*1000.0,0);
        } else 
            if (i==5) {
                draw_freq_line(i*100.0,2);
                draw_freq_line(i*1000.0,2);
            } else {
                draw_freq_line(i*100.0,1);
                draw_freq_line(i*1000.0,1);
            }
    }

    draw_freq_line(10000.0,0);
    draw_freq_line(20000.0,1);

    //Draw dotted grid
    fl_line_style(FL_DOT);
    int GY=10;if (ly<GY*3) GY=-1;
    for (int i=1; i<GY; ++i){
        int tmp=(int)(ly/(float)GY*i);
        fl_line(ox+2,oy+tmp,ox+lx-2,oy+tmp);
    }



    //draw the data
    fl_color(FL_RED);
    fl_line_style(FL_SOLID,2);
    fl_begin_line();
    int oiy = ly*Prespoints[0]/128.0;//XXX easy
    for (int i=1; i<N_RES_POINTS; ++i){
        const int ix=(i*1.0/N_RES_POINTS*lx);
        const int iy= ly*Prespoints[i]/128.0;//XXX easy
        fl_vertex(ox+ix,oy+ly-oiy);
        oiy=iy;
    }
    fl_end_line();
    fl_line_style(FL_SOLID,0);
}

int Fl_Resonance_Graph::handle(int event)
{
    int x_=Fl::event_x()-x();
    int y_=Fl::event_y()-y();
    if((x_>=0)&&(x_<w()) && (y_>=0)&&(y_<h())){
        khzvalue->value(getfreqx(x_*1.0/w())/1000.0);//XXX
        dbvalue->value((1.0-y_*2.0/h())*PmaxdB);//XXX
    }

    if((event==FL_PUSH)||(event==FL_DRAG)){
        const bool leftbutton = Fl::event_button() == FL_LEFT_MOUSE;

        if (x_<0) x_=0;if (y_<0) y_=0;
        if (x_>=w()) x_=w();if (y_>=h()-1) y_=h()-1;

        if ((oldx<0)||(oldx==x_)){
            int sn=(int)(x_*1.0/w()*N_RES_POINTS);
            int sp=127-(int)(y_*1.0/h()*127);
            if(leftbutton)
                setPoint(sn,sp);
                //oscWrite("setpoint", "ii", sn, sp);//respar->setpoint(sn,sp);//XXX easy
            else
                setPoint(sn,sp);
                //oscWrite("setpoint", "ii", sn, 64);//respar->setpoint(sn,64);//XXX easy
        } else {
            int x1=oldx;
            int x2=x_;
            int y1=oldy;
            int y2=y_;
            if (oldx>x_){
                x1=x_;y1=y_;
                x2=oldx;y2=oldy;
            }
            for (int i=0;i<x2-x1;i++){
                int sn=(int)((i+x1)*1.0/w()*N_RES_POINTS);
                float yy=(y2-y1)*1.0/(x2-x1)*i;
                int sp=127-(int)((y1+yy)/h()*127);
                if(leftbutton) //respar->setpoint(sn,sp);//XXX easy
                    setPoint(sn, sp);
                    //oscWrite("setpoint", "ii", sn, sp);
                else //respar->setpoint(sn,64);//XXX easy
                    setPoint(sn, sp);
                    //oscWrite("setpoint", "ii", sn, sp);
            }
        }

        oldx=x_;oldy=y_;
        redraw();
    }

    if(event==FL_RELEASE) {
        oldx=-1;
        if(cbwidget) {
            cbwidget->do_callback();
            if(applybutton) {
                applybutton->color(FL_RED);
                applybutton->redraw();
            }
        }
    }

    return 1;
}

void Fl_Resonance_Graph::setcbwidget(Fl_Widget *cbwidget,Fl_Widget *applybutton)
{
    this->cbwidget=cbwidget;
    this->applybutton=applybutton;
}

void Fl_Resonance_Graph::update(void)
{
    oscWrite("Prespoints");
    oscWrite("Pcenterfreq");
    oscWrite("Poctavesfreq");
    oscWrite("PmaxdB");
}
        
void Fl_Resonance_Graph::OSC_raw(const char *msg)
{
    //TODO check the types (OSC regex)
    if(strstr(msg, "Prespoints")) {
        rtosc_blob_t arg = rtosc_argument(msg, 0).b;
        assert(arg.len == N_RES_POINTS);
        memcpy(Prespoints, arg.data, N_RES_POINTS);
    } else if(strstr(msg, "Pcenterfreq"))
        Pcenterfreq = rtosc_argument(msg, 0).i;
    else if(strstr(msg, "Poctavesfreq"))
        Poctavesfreq = rtosc_argument(msg, 0).i;
    else if(strstr(msg, "PmaxdB"))
        PmaxdB = rtosc_argument(msg, 0).i;
    else
        puts("I got an unknown message...");

    redraw();
}

float Fl_Resonance_Graph::getfreqx(float x) const
{
    const float octf = powf(2.0f, getoctavesfreq());
    return getcenterfreq() / sqrt(octf) * powf(octf, limit(x, 0.0f, 1.0f));
}

/*
 * Get the x coordinate from frequency (used by the UI)
 */
float Fl_Resonance_Graph::getfreqpos(float freq) const
{
    return (logf(freq) - logf(getfreqx(0.0f))) / logf(2.0f) / getoctavesfreq();
}

/*
 * Get the center frequency of the resonance graph
 */
float Fl_Resonance_Graph::getcenterfreq() const
{
    return 10000.0f * powf(10, -(1.0f - Pcenterfreq / 127.0f) * 2.0f);
}

/*
 * Get the number of octave that the resonance functions applies to
 */
float Fl_Resonance_Graph::getoctavesfreq() const
{
    return 0.25f + 10.0f * Poctavesfreq / 127.0f;
}

void Fl_Resonance_Graph::setPoint(int idx, int val)
{
    Prespoints[idx] = val;
    oscWrite(std::string("Prespoints")+to_s(idx), "c", val);
    redraw();
}
