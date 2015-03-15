
class PADnoteHarmonicProfile: public Fl_Box, public Fl_Osc_Widget
{
    public:
        PADnoteHarmonicProfile(int x,int y, int w, int h, const char *label=0)
            :Fl_Box(x,y,w,h,label), smps(new float[w]), realbw(0.0f)
        {
            memset(smps, 0, w*sizeof(float));
        }

        ~PADnoteHarmonicProfile(void)
        {
            osc->removeLink(loc, (Fl_Osc_Widget*) this);
            delete[] smps;
        }

        void init(void)
        {
            Fl_Osc_Pane *og  = fetch_osc_pane(this);
            assert(og);

            loc = og->base + "profile";
            osc = og->osc;
            assert(osc);

            osc->createLink(loc, (Fl_Osc_Widget*) this);
            update();
        }

        void update(void)
        {
            osc->write(loc, "i", w());
        }
        
        void OSC_value(unsigned N, void *data, const char *name) override
        {
            assert(!strcmp(name, "profile"));
            assert(N==w()*sizeof(float));
            memcpy(smps, data, N);
            redraw();
        }

        void OSC_value(float x, const char *name) override
        {
            assert(!strcmp(name, "profile"));
            realbw = x;
            redraw();
        }

        void draw(void)
        {
            int ox=x(),oy=y(),lx=w(),ly=h();
            const bool active=active_r();
            if(!visible())
                return;

            if (damage()!=1){
                fl_color(fl_color_average(FL_BLACK,
                            FL_BACKGROUND_COLOR, 0.5 ));
                fl_rectf(ox,oy,lx,ly);
            }

            //draw the equivalent bandwidth
            if (active) fl_color(220,220,220);
            else fl_color(160,165,165);
            fl_line_style(FL_DASH);
            int rbw=(int)(realbw*(lx-1.0)/2.0);
            fl_begin_line();
            for(int i=lx/2-rbw;i<(lx/2+rbw); ++i)
                fl_vertex(ox+i,oy);
            fl_end_line();

            fl_line_style(FL_DASH);
            if(active)
                fl_color(200,200,200);
            else
                fl_color(160,160,160);

            for (int i=1;i<10;i++){
                const int kx=(int)(lx/10.0*i);
                fl_line(ox + kx, oy, ox + kx, oy + ly - 1);
            }
            for (int i=1;i<5;i++){
                const int ky=(int)(ly/5.0*i);
                fl_line(ox,oy+ly-ky,ox+lx,oy+ly-ky-1);
            }


            fl_color(120,120,120);
            fl_line_style(FL_DASH);
            fl_line(ox+lx/2,oy,ox+lx/2,oy+ly);

            //draw the graph
            fl_line_style(FL_SOLID);
            if (active)
                fl_color(180,210,240);
            else
                fl_color(150,150,155);

            fl_color(fl_color_add_alpha(fl_color(), 127));

            fl_begin_polygon();
            fl_vertex(ox, oy + h());
            for (int i=0; i<lx; ++i){
                int val=(int) ((ly-2)*smps[i]);
                fl_vertex(ox+i,oy+ly-1-val);
            }
            fl_vertex(ox + w(), oy + h());
            fl_end_polygon();


            fl_line_style(FL_DASH);
            if (active)
                fl_color(0,100,220);
            else
                fl_color(150,160,170);
            fl_line(ox+lx/2-rbw,oy,ox+lx/2-rbw,oy+ly-1);
            fl_line(ox+lx/2+rbw,oy,ox+lx/2+rbw,oy+ly-1);

            fl_line_style(0);
        }
    private:
        float *smps;
        float realbw;
}; 
