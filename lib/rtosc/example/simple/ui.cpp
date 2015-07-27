#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Chart.H>
#include <FL/Fl_Hor_Nice_Slider.H>
#include <FL/Fl_Toggle_Button.H>
#include <err.h>
#include <rtosc/rtosc.h>

char gui_osc_buf[2048];
void dsp_message(const char *msg);
#define message(path, type, ...) do { \
    if(rtosc_message(gui_osc_buf, 2048, path, type, ##__VA_ARGS__)) \
        dsp_message(gui_osc_buf); \
    else \
        warnx("Message to %s is too long...", path);\
} while(0)



static void shape_cb(Fl_Widget *w, void*)
{
    message("/synth/shape", "i", static_cast<Fl_Choice*>(w)->value());

    //Request plot data
    message("/synth/plot/data", "i", 128);
}

static void freq_cb(Fl_Widget *w, void*)
{
    message("/synth/freq", "f", static_cast<Fl_Slider*>(w)->value());
}

static void gate_cb(Fl_Widget *w, void*)
{
    message("/synth/gate", static_cast<Fl_Button*>(w)->value() ? "T" : "F");
}
Fl_Chart *chart;
void gui_dispatch(const char *msg)
{
    const int    elms = rtosc_argument(msg,0).b.len;
    const float *data = (const float*) rtosc_argument(msg,0).b.data;
    chart->clear();
    for(int i=0; i<elms; ++i)
        chart->add(data[i]);
}

void gui_ev(void);
void start_synth(void);
void stop_synth(void);
bool running = true;
int main()
{
    Fl_Window *window = new Fl_Double_Window(400, 400, "window");
    chart = new Fl_Chart(10, 10, 380, 300);
    Fl_Choice *choice = new Fl_Choice(100, 320, 100, 20, "wave shape");
    Fl_Hor_Nice_Slider *slider = new Fl_Hor_Nice_Slider(100, 340, 100, 20, "frequency");
    Fl_Toggle_Button   *button = new Fl_Toggle_Button(220, 320, 150, 70, "Gate");

    //Config
    choice->add("sinusoidal");
    choice->add("sawtooth");
    choice->add("rectangular");

    slider->minimum(0.0);
    slider->maximum(440.0);

    //callbacks
    window->callback([](Fl_Widget*, void*){running=false;});
    choice->callback(shape_cb);
    slider->callback(freq_cb);
    button->callback(gate_cb);

    window->resizable(chart);

    window->show();
    start_synth();

    //Request plot data
    message("/synth/plot/data", "i", 128);

    while(running) {
        gui_ev();
        Fl::wait(0.01f);
    }

    stop_synth();
    return 0;
}
