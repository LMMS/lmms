#include "Controller.h"
#include "ControllerUI.h"

pthread_t  thr1, thr2;
Controller controller;



main()
{
    ControllerUI *controllerUI = new ControllerUI(&controller);

    Fl::run();

    delete controllerUI;
};
