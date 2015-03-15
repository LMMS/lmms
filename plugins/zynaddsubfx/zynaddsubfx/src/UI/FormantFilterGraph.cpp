#include "FormantFilterGraph.H"
#include <cmath>
#include <cstdio>
#include <cstdlib>

FormantFilterGraph::FormantFilterGraph(int x,int y, int w, int h, const char *label)
:Fl_Box(x,y,w,h,label), Fl_Osc_Widget(this)
{
    memset(Pvowels, 0, sizeof(Pvowels));
    Pnumformants = 0;
    Pstages = 0;
    Pgain = 0;
    Pcenterfreq = 0;
    Pq = 0;
    Poctavesfreq = 0;
    nvowel=NULL;
    nformant=NULL;
    graphpoints=NULL;
}

void FormantFilterGraph::init(int *nvowel_,int *nformant_)
{
    nvowel=nvowel_;
    nformant=nformant_;
    graphpoints=new float [w()];

    oscRegister("Pvowels");
    oscRegister("Pnumformants");
    oscRegister("Pstages");
    oscRegister("Pcenterfreq");
    oscRegister("Poctavesfreq");
    oscRegister("Pgain");
    oscRegister("Pq");
}

void FormantFilterGraph::OSC_value(int x, const char *loc)
{
    if(strstr(loc, "Pnumformants"))
        Pnumformants = x;
    else if(strstr(loc, "Pstages"))
        Pstages = x;
    else if(strstr(loc, "Pcenterfreq"))
        Pcenterfreq = x;
    else if(strstr(loc, "Pgain"))
        Pgain = x;
    else if(strstr(loc, "Pq"))
        Pq = x;
    else if(strstr(loc, "Poctavesfreq"))
        Poctavesfreq = x;

    redraw();
}
void FormantFilterGraph::OSC_value(unsigned x, void *v)
{
    assert(x = sizeof(Pvowels));
    memcpy(&Pvowels[0], v, x);
    redraw();
}

void FormantFilterGraph::draw_freq_line(float freq,int type)
{
    const float freqx=getfreqpos(freq);
    switch(type){
        case 0:fl_line_style(FL_SOLID);break;
        case 1:fl_line_style(FL_DOT);break;
        case 2:fl_line_style(FL_DASH);break;
    };


    if ((freqx>0.0)&&(freqx<1.0))
        fl_line(x()+(int) (freqx*w()),y(),
                x()+(int) (freqx*w()),y()+h());
}

void FormantFilterGraph::update(void)
{
    oscWrite("Pvowels");
    oscWrite("Pnumformants");
    oscWrite("Pstages");
    oscWrite("Pcenterfreq");
    oscWrite("Poctavesfreq");
    oscWrite("Pgain");
    oscWrite("Pq");
}

//TODO A good portion of this is copy/pasta from EnvelopUI's widget
//     REFACTOR!
void FormantFilterGraph::draw()
{
    const int maxdB=30;
    const int ox=x(),oy=y(),lx=w(),ly=h();

    fl_color(FL_BLACK);
    fl_rectf(ox,oy,lx,ly);


    //draw the lines
    fl_color(FL_GRAY);

    fl_line_style(FL_SOLID);
    //fl_line(ox+2,oy+ly/2,ox+lx-2,oy+ly/2);

    const float freqx = getfreqpos(1000.0);
    if ((freqx>0.0)&&(freqx<1.0))
        fl_line(ox+(int) (freqx*lx),oy,
                ox+(int) (freqx*lx),oy+ly);

    for(int i=1;i<10;i++){
        if(i==1){
            draw_freq_line(i*100.0,0);
            draw_freq_line(i*1000.0,0);
        }else
            if (i==5){
                draw_freq_line(i*100.0,2);
                draw_freq_line(i*1000.0,2);
            }else{
                draw_freq_line(i*100.0,1);
                draw_freq_line(i*1000.0,1);
            };
    };

    draw_freq_line(10000.0,0);
    draw_freq_line(20000.0,1);

    fl_line_style(FL_DOT);
    int GY=10;if (ly<GY*3) GY=-1;
    for (int i=1;i<GY;i++){
        int tmp=(int)(ly/(float)GY*i);
        fl_line(ox+2,oy+tmp,ox+lx-2,oy+tmp);
    };

    fl_color(FL_YELLOW);
    fl_font(FL_HELVETICA,10);
    if (*nformant < Pnumformants){
        draw_freq_line(getformantfreq(Pvowels[*nvowel].formants[*nformant].freq),2);

        //show some information (like current formant frequency,amplitude)
        char tmpstr[20];

        snprintf(tmpstr,20,"%.2f kHz",getformantfreq(Pvowels[*nvowel].formants[*nformant].freq)*0.001);
        fl_draw(tmpstr,ox+1,oy+1,40,12,FL_ALIGN_LEFT,NULL,0);

        snprintf(tmpstr,20,"%d dB",(int)( rap2dB(1e-9 +
                        getformantamp(Pvowels[*nvowel].formants[*nformant].amp))
                    + getgain()));
        fl_draw(tmpstr,ox+1,oy+15,40,12,FL_ALIGN_LEFT,NULL,0);

    };

    //draw the data

    fl_color(FL_RED);
    fl_line_style(FL_SOLID);

    formantfilterH(*nvowel,lx,graphpoints);

    fl_line_style( FL_SOLID, 2 );
    fl_begin_line();
    int oiy=(int) ((graphpoints[0]/maxdB+1.0)*ly/2.0);
    for(int i=1;i<lx;i++){
        double iy= ((graphpoints[i]/maxdB+1.0)*ly/2.0);
        if ((iy>=0)&&(oiy>=0)&&(iy<ly)&&(oiy<lx))
            fl_vertex(ox+i,oy+ly-iy);
        oiy=iy;
    };
    fl_end_line();
    fl_line_style(FL_SOLID,0);
}

FormantFilterGraph::~FormantFilterGraph(void)
{
    delete [] graphpoints;
}

/*
 * Parameter control
 */
float FormantFilterGraph::getgain()
{
    return (Pgain / 64.0f - 1.0f) * 30.0f; //-30..30dB
}

float FormantFilterGraph::getq()
{
    return expf(powf((float) Pq / 127.0f, 2) * logf(1000.0f)) - 0.9f;
}

/*
 * Get the center frequency of the formant's graph
 */
float FormantFilterGraph::getcenterfreq()
{
    return 10000.0f * powf(10, -(1.0f - Pcenterfreq / 127.0f) * 2.0f);
}

/*
 * Get the number of octave that the formant functions applies to
 */
float FormantFilterGraph::getoctavesfreq()
{
    return 0.25f + 10.0f * Poctavesfreq / 127.0f;
}

/*
 * Get the frequency from x, where x is [0..1]
 */
float FormantFilterGraph::getfreqx(float x)
{
    if(x > 1.0f)
        x = 1.0f;
    float octf = powf(2.0f, getoctavesfreq());
    return getcenterfreq() / sqrt(octf) * powf(octf, x);
}

/*
 * Get the x coordinate from frequency (used by the UI)
 */
float FormantFilterGraph::getfreqpos(float freq)
{
    return (logf(freq) - logf(getfreqx(0.0f))) / logf(2.0f) / getoctavesfreq();
}


/*
 * Get the freq. response of the formant filter
 */
void FormantFilterGraph::formantfilterH(int nvowel, int nfreqs, float *freqs)
{
    float c[3], d[3];

    for(int i = 0; i < nfreqs; ++i)
        freqs[i] = 0.0f;

    //for each formant...
    for(int nformant = 0; nformant < Pnumformants; ++nformant) {
        //compute formant parameters(frequency,amplitude,etc.)
        const float filter_freq = getformantfreq(Pvowels[nvowel].formants[nformant].freq);
        float filter_q    = getformantq(Pvowels[nvowel].formants[nformant].q) * getq();
        if(Pstages > 0)
            filter_q = (filter_q > 1.0f ? powf(filter_q, 1.0f / (Pstages + 1)) : filter_q);

        const float filter_amp = getformantamp(Pvowels[nvowel].formants[nformant].amp);

        //printf("NFORMANT %d\n", nformant);
        //printf("CHARACTERISTICS: FREQ %f Q %f AMP %f\n", filter_freq, filter_q, filter_amp);


        if(filter_freq <= (synth->samplerate / 2 - 100.0f)) {
            const float omega = 2 * PI * filter_freq / synth->samplerate_f;
            const float sn    = sinf(omega);
            const float cs    = cosf(omega);
            const float alpha = sn / (2 * filter_q);
            const float tmp   = 1 + alpha;
            c[0] = alpha / tmp *sqrt(filter_q + 1);
            c[1] = 0;
            c[2] = -alpha / tmp *sqrt(filter_q + 1);
            d[1] = -2 * cs / tmp * (-1);
            d[2] = (1 - alpha) / tmp * (-1);
        }
        else
            continue;


        for(int i = 0; i < nfreqs; ++i) {
            const float freq = getfreqx(i / (float) nfreqs);

            //Discard frequencies above nyquist rate
            if(freq > synth->samplerate / 2) {
                for(int tmp = i; tmp < nfreqs; ++tmp)
                    freqs[tmp] = 0.0f;
                break;
            }

            //Convert to normalized frequency
            const float fr = freq / synth->samplerate * PI * 2.0f;

            //Evaluate Complex domain ratio
            float x  = c[0], y = 0.0f;
            for(int n = 1; n < 3; ++n) {
                x += cosf(n * fr) * c[n];
                y -= sinf(n * fr) * c[n];
            }
            float h = x * x + y * y;
            x = 1.0f;
            y = 0.0f;
            for(int n = 1; n < 3; ++n) {
                x -= cosf(n * fr) * d[n];
                y += sinf(n * fr) * d[n];
            }
            h = h / (x * x + y * y);

            freqs[i] += powf(h, (Pstages + 1.0f) / 2.0f) * filter_amp;
        }
    }

    //Convert to logarithmic data ignoring points that are too small
    for(int i = 0; i < nfreqs; ++i) {
        if(freqs[i] > 0.000000001f)
            freqs[i] = rap2dB(freqs[i]) + getgain();
        else
            freqs[i] = -90.0f;
    }
}

/*
 * Transforms a parameter to the real value
 */
float FormantFilterGraph::getformantfreq(unsigned char freq)
{
    return getfreqx(freq / 127.0f);
}

float FormantFilterGraph::getformantamp(unsigned char amp)
{
    return powf(0.1f, (1.0f - amp / 127.0f) * 4.0f);
}

float FormantFilterGraph::getformantq(unsigned char q)
{
    return powf(25.0f, (q - 32.0f) / 64.0f);
}
