#include <Arduino.h>
#include <ArduinoNvs.h>
#include <LilyGoWatch.h>
#include "Advanced.hpp"
#include "LogView.hpp"
#include "../static/img_back_32.xbm"
#include "../static/img_trash_32.xbm"
#include "../static/img_log_32.xbm"
#include "../static/img_help_32.xbm"

#include "Shutdown.hpp"

#include "Watchface.hpp"
#include "../UI/widgets/ButtonImageXBMWidget.hpp"
#include "../UI/widgets/SwitchWidget.hpp"

extern TFT_eSprite *overlay;

AdvancedSettingsApplication::~AdvancedSettingsApplication() {
    if ( nullptr != btnBack ) { delete btnBack; }
    if ( nullptr != btnErase ) { delete btnErase; }
    if ( nullptr != btnLog ) { delete btnLog; }
    if ( nullptr != btnHelp ) { delete btnHelp; }
}

AdvancedSettingsApplication::AdvancedSettingsApplication() {
    btnBack=new ButtonImageXBMWidget(5,TFT_HEIGHT-69,64,64,[&,this](){
        LaunchApplication(new WatchfaceApplication());
    },img_back_32_bits,img_back_32_height,img_back_32_width,TFT_WHITE,ttgo->tft->color24to16(0x353e45),false);
    btnErase=new ButtonImageXBMWidget(5,5,64,64,[&,this](){
        NVS.eraseAll(true);
        LaunchApplication(new ShutdownApplication(true,false));
    },img_trash_32_bits,img_trash_32_height,img_trash_32_width,TFT_WHITE,ttgo->tft->color24to16(0xff0000));
    btnLog=new ButtonImageXBMWidget(20,80,64,200,[&,this](){
        LaunchApplication(new LogViewApplication());
    },img_log_32_bits,img_log_32_height,img_log_32_width,TFT_WHITE,ttgo->tft->color24to16(0x555f68));
    btnHelp=new ButtonImageXBMWidget(TFT_WIDTH-69,5,64,64,[&,this](){
        showOverlay = (!showOverlay);
    },img_help_32_bits,img_help_32_height,img_help_32_width,TFT_WHITE,ttgo->tft->color24to16(0x353e45),false);

    overlay->fillSprite(TFT_BLACK);
    const uint8_t margin = 32;
    const uint8_t radius = 50;

    for(int32_t y=0;y<TFT_HEIGHT;y+=2) {
        for(int32_t x=0;x<TFT_WIDTH;x+=2) {
            overlay->drawPixel(x,y,TFT_TRANSPARENT);
        }
    }

    overlay->fillCircle(btnErase->x+margin,btnErase->y+margin,radius+2,TFT_WHITE);
    int16_t rx = btnErase->x+radius;
    int16_t ry = btnErase->y;
    int16_t rh = 30;
    int16_t rw = 140;
    overlay->fillRoundRect(rx,ry,rw,rh,5,TFT_WHITE);
    overlay->fillRoundRect(rx+2,ry+2,rw-4,rh-4,5,TFT_BLACK);
    overlay->fillCircle(btnErase->x+margin,btnErase->y+margin,radius,TFT_TRANSPARENT);
    overlay->setTextFont(0);
    overlay->setTextSize(2);
    overlay->setTextColor(TFT_WHITE);
    overlay->setTextDatum(TL_DATUM);
    overlay->drawString("Full erase",rx+7,ry+7);
}

bool AdvancedSettingsApplication::Tick() {
    btnBack->Interact(touched,touchX, touchY);
    btnErase->Interact(touched,touchX, touchY);
    btnLog->Interact(touched,touchX, touchY);
    btnHelp->Interact(touched,touchX, touchY);
    if (millis() > nextRedraw ) {
        canvas->fillSprite(canvas->color24to16(0x212121));
        btnBack->DrawTo(canvas);
        btnErase->DrawTo(canvas);
        btnLog->DrawTo(canvas);
        btnHelp->DrawTo(canvas);
        if ( showOverlay ) { overlay->pushRotated(canvas,0,TFT_TRANSPARENT); }
        nextRedraw=millis()+(1000/10);
        return true;
    }
    return false;
}