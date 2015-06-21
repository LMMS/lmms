#include "Connection.h"
#include <unistd.h>
namespace GUI {
ui_handle_t createUi(Fl_Osc_Interface*, void *exit)
{
    return 0;
}
void destroyUi(ui_handle_t)
{
}
void raiseUi(ui_handle_t, const char *)
{
}
void raiseUi(ui_handle_t, const char *, const char *, ...)
{
}
void tickUi(ui_handle_t)
{
    usleep(100000);
}
Fl_Osc_Interface *genOscInterface(MiddleWare*)
{
    return NULL;
}
};
