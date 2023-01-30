#include <Arduino.h>
#include "LogView.hpp" // log capabilities
#include "../UI/AppTemplate.hpp"
#include "ApplicationBase.hpp"

ApplicationBase::ApplicationBase() {
    // Init here any you need
    lAppLog("Hello from ApplicationBase!\n");

    // initalize here widgets
    //mywidget=new .....

    // Remember to dump anything to canvas to get awesome splash screen
    // draw of TFT_eSPI canvas for splash here

    Tick(); // OR call this if no splash 
}

ApplicationBase::~ApplicationBase() {
    // please remove/delete/free all to avoid leaks!
    //delete mywidget;
}

bool ApplicationBase::Tick() {
    // put your interacts here:
    //mywidget->Interact(touched,touchX,touchY);

    if ( millis() > nextRefresh ) { // redraw full canvas
        canvas->fillSprite(ThCol(background)); // use theme colors
        TemplateApplication::Tick();
        // draw your interface widgets here!!!
        //mywidget->DrawTo(canvas);

        nextRefresh=millis()+(1000/8); // 8 FPS is enought for GUI
        return true;
    }
    return false;
}
