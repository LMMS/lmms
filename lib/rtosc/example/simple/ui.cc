#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Chart.H>
#include <FL/Fl_Hor_Nice_Slider.H>
#include <FL/Fl_Toggle_Button.H>
#include <err.h>
#include <rtosc.h>
#include <thread-link.h>

static bool _T(msg_t){return true;};
std::function<void(msg_t)> _blob(void(*fn)(blob_t))
{
    return [fn](msg_t msg){fn(argument(msg,0).b);};
}

void chart_update(blob_t b);
extern ThreadLink<2048,16> bToU, uToB;
Dispatch<1> dispatchU{std::make_tuple("/ui/plot", "b", _T, _blob(chart_update))};

static void shape_cb(Fl_Widget *w, void*)
{
    uToB.write("/synth/shape", "i", static_cast<Fl_Choice*>(w)->value());

    //Request plot data
    uToB.write("/synth/plot/data","i",128);
}

static void freq_cb(Fl_Widget *w, void*)
{
    uToB.write("/synth/freq", "f", static_cast<Fl_Slider*>(w)->value());
}

static void gate_cb(Fl_Widget *w, void*)
{
    uToB.write("/synth/gate", static_cast<Fl_Button*>(w)->value() ? "T" : "F");
}


Fl_Chart *chart;
void chart_update(blob_t b) {
    const int    elms = b.len/sizeof(float);
    const float *data = (const float*) b.data;
    chart->clear();
    for(int i=0; i<elms; ++i) {
        chart->add(data[i]);
    }
}

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
    uToB.write("/synth/plot/data","i",128);

    while(running) {
        while(bToU.hasNext())
            dispatchU(bToU.read());
        Fl::wait(0.01f);
    }

    stop_synth();
    return 0;
}
