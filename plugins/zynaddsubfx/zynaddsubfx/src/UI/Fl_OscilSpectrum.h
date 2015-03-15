#include <cassert>

//consider merging with Fl_Oscilloscope
class Fl_OscilSpectrum : public Fl_Box, Fl_Osc_Widget
{
    public:
        Fl_OscilSpectrum(int x,int y, int w, int h, const char *label=0)
            :Fl_Box(x,y,w,h,label), nsamples(0), spc(NULL)
        {}

        ~Fl_OscilSpectrum(void)
        {
            delete[] spc;
            osc->removeLink(loc, (Fl_Osc_Widget*) this);
        }

        void init(bool base_spectrum_p)
        {
            Fl_Osc_Pane *og  = fetch_osc_pane(this);
            assert(og);

            loc = og->base + (base_spectrum_p ? "base-spectrum": "spectrum");
            osc = og->osc;
            assert(osc);

            osc->createLink(loc, (Fl_Osc_Widget*) this);
            update();
        }

        void update(void)
        {
            osc->requestValue(loc);
        }

        virtual void OSC_value(unsigned N, void *data) override
        {
            assert(!(N%4));
            const size_t new_samples = N / 4;

            //resize buffer if needed
            if(new_samples != nsamples) {
                delete [] spc;
                spc = new float[new_samples];
                nsamples = new_samples;
            }

            memcpy(spc, data, N);

            //normalize
            float max=0;
            for (unsigned i=0; i<nsamples; i++){
                float x=fabs(spc[i]);
                if (max<x) max=x;
            }
            if (max<0.000001) max=1.0;
            max=max*1.05;

            for(unsigned i=0; i<nsamples; ++i)
                spc[i]/=max;

            //Get widget to redraw new data
            redraw();
        }

        void draw(void)
        {
            const int ox=x(),oy=y(),lx=w(),ly=h();
            const int maxdb=60;//must be multiple of 10
            const int GX=2;
            int n=lx/GX-1;
            if (n>synth->oscilsize/2)
                n=synth->oscilsize/2;
                
            fl_rectf(ox,oy,lx,ly,0,0,0);

            //draw
            if(this->active_r()) 
                fl_color(0,0,255);
            else 
                fl_color(this->parent()->color());
            fl_line_style(FL_DOT);

            for(int i=1; i<maxdb/10; i++){
                const int ky=((int)((float)i*ly*10.0/maxdb)/2)*2;
                fl_line(ox,oy+ky-1,ox+lx-2,oy+ky-1);
            }

            for(int i=2; i<n; i++){
                int tmp=i*GX-2;
                if(i%10==1)
                    fl_line_style(0);
                else
                    fl_line_style(FL_DOT);
                fl_line(ox+tmp,oy+2,ox+tmp,oy+ly-2);
            }

            if (this->active_r())
                fl_color(0,255,0);
            else
                fl_color(this->parent()->color());
            fl_line_style(0);

            if(!spc)
                return;
            //draws the spectrum
            for(int i=0; i<n; i++){
                int tmp=i*GX+2;
                float x=spc[i];

                if (x>dB2rap(-maxdb)) x=rap2dB(x)/maxdb+1;
                else x=0;

                int val=(int) ((ly-2)*x);
                if (val>0) fl_line(ox+tmp,oy+ly-2-val,ox+tmp,oy+ly-2);
            }
        }
    private:

        size_t nsamples;
        float *spc;
}; 
