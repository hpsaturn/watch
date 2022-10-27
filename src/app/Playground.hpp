#ifndef __LUNOKIOT__PLAYGROUND_APP__
#define __LUNOKIOT__PLAYGROUND_APP__

#include <Arduino.h>
#include <LilyGoWatch.h>
#include "../system/Application.hpp"
#include "../UI/widgets/CanvasWidget.hpp"
#include "../UI/widgets/CanvasZWidget.hpp"

class PlaygroundApplication: public LunokIoTApplication {
    private:
        unsigned long nextRedraw=0;
    public:
        TFT_eSPI *tft = ttgo->tft;
        CanvasWidget *buffer=nullptr;
        bool alphaDirection = true;
        int16_t currentAlpha = 0;
        CanvasWidget *imageTest=nullptr;
        CanvasWidget *imageTest2=nullptr;
        CanvasWidget *imageTest3=nullptr;
        CanvasZWidget *imageDeepTest=nullptr;
        PlaygroundApplication();
        ~PlaygroundApplication();
        bool Tick();
};

#endif
