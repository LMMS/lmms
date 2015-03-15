//Defines the methods of communication for the GUI
//Expect this code to mutate into some sort of ugly beast that will slowly
//remove the tendrils of the UI from the RT code

class Fl_Osc_Interface;
class MiddleWare;
namespace GUI
{
typedef void *ui_handle_t;

ui_handle_t createUi(Fl_Osc_Interface *osc, void *exit);
void destroyUi(ui_handle_t);
void raiseUi(ui_handle_t, const char *);
void raiseUi(ui_handle_t, const char *, const char *, ...);
void tickUi(ui_handle_t);

Fl_Osc_Interface *genOscInterface(MiddleWare*);
};
