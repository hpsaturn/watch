//
//    LunokWatch, a open source smartwatch software
//    Copyright (C) 2022,2023  Jordi Rubió <jordi@binarycell.org>
//    This file is part of LunokWatch.
//
// LunokWatch is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software 
// Foundation, either version 3 of the License, or (at your option) any later 
// version.
//
// LunokWatch is distributed in the hope that it will be useful, but WITHOUT 
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
// details.
//
// You should have received a copy of the GNU General Public License along with 
// LunokWatch. If not, see <https://www.gnu.org/licenses/>. 
//

#include <Arduino.h>
#include <LilyGoWatch.h>
#include "Playground1.hpp"
#include "../UI/widgets/NotificationWidget.hpp"
#include "../UI/UI.hpp"
#include "../static/img_sample_background.c"


PlaygroundApplication1::~PlaygroundApplication1() {
    if ( nullptr != testNotification ) {
        delete testNotification;
        testNotification = nullptr;
    }
    
    if ( nullptr != testNotification1 ) {
        delete testNotification1;
        testNotification1 = nullptr;
    }
    
}

PlaygroundApplication1::PlaygroundApplication1() {
    testNotification = new NotificationWidget([](void *callerObject){
        //Serial.println("test");
    },(char*)"hello this is a test");

    testNotification1 = new NotificationWidget([](void *callerObject){
        Serial.println("BBBBBBBBBBBB");
    },(char*)"BYEEEEE");

    testNotification->byteSwap = true;
    testNotification1->byteSwap = true;
    
    testNotification1->SetY(50);

    testNotification->Show();
    testNotification1->Show();
}



bool PlaygroundApplication1::Tick() {
    testNotification->Interact(touched,touchX,touchY);
    testNotification1->Interact(touched,touchX,touchY);
    static unsigned long triggerNotif = millis()+11000;
    static unsigned long triggerNotif2 = millis()+5500;

    if ( millis() > triggerNotif ) {
        testNotification->SetX(random(0,120));
        testNotification->SetY(random(0,120));
        testNotification->SetH(random(30,120));
        testNotification->SetW(random(60,120));
        testNotification->Show();
        triggerNotif=millis()+11000;
    }
    
    if ( millis() > triggerNotif2 ) {
        testNotification1->SetX(random(0,120));
        testNotification1->SetY(random(0,120));
        testNotification1->SetH(random(30,120));
        testNotification1->SetW(random(60,120));
        testNotification1->Show();
        triggerNotif2=millis()+11000;
    }
    
    if (millis() > nextRedraw ) {
        canvas->setSwapBytes(true);
        canvas->pushImage(0,0,240,240,(uint16_t *)img_sample_background.pixel_data);
        testNotification->DrawTo(canvas);
        testNotification1->DrawTo(canvas);
        nextRedraw=millis()+(1000/12);
        return true;
    }
    return false;
}
