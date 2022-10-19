#include <Arduino.h>
#include <LilyGoWatch.h>
#include "../lunokiot_config.hpp"

#include "../UI/activator/ActiveRect.hpp"
#include "../UI/widgets/CanvasWidget.hpp"
#include "../UI/widgets/ButtonWidget.hpp"
#include "../UI/widgets/ButtonImageXBMWidget.hpp"

#include "Watchface.hpp"

#include "../static/img_watchface0.c"
#include "../static/img_hours_hand.c"
#include "../static/img_minutes_hand.c"
#include "../static/img_seconds_hand.c"
#include "../static/img_wifi_24.xbm"
#include "../static/img_usb_24.xbm"

#include "../static/img_daynightCycle.c"
#include "../static/img_batteryGauge106.c"
#include "../static/img_watchface0_marks.c"
#include "../static/img_watchface0_usb.c"
#include "../static/img_watchface0_backlight.c"

#include "../static/img_weather_200.c"
#include "../static/img_weather_300.c"
#include "../static/img_weather_500.c"
#include "../static/img_weather_600.c"

#include "../static/img_weather_800.c"

#include "MainMenu.hpp"

#include <WiFi.h>
//#include "../UI/UI.hpp"
#include <HTTPClient.h>
#include <Arduino_JSON.h>

double sphereRotation = 0;

int demoCycle = 0;
int demobattPC = 100;
unsigned long nextWatchFaceFullRedraw = 0;

extern TFT_eSprite *overlay;

// NTP sync data
const char *ntpServer       = "es.pool.ntp.org";
const long  gmtOffset_sec   = 3600;
//const int   daylightOffset_sec = 0; // winter
const int   daylightOffset_sec = 3600; // summer

bool ntpSyncDone = false;
bool weatherSyncDone = false;

// manipulate the angle data from UI
extern int16_t downTouchX;
extern int16_t downTouchY;

extern int batteryPercent;
extern const String openWeatherMapApiKey;

extern void AlertLedEnable(bool enabled);
int currentSec=random(0,60);
int currentMin=random(0,60);
int currentHour=random(0,24);

int weatherId = -1;
char * weatherMain = nullptr;
char * weatherDescription = nullptr;
char * weatherIcon = nullptr;
double weatherTemp = -1000;

extern const uint8_t openweatherPEM_start[] asm("_binary_asset_openweathermap_org_pem_start");
extern const uint8_t openweatherPEM_end[] asm("_binary_asset_openweathermap_org_pem_end");

char * weatherReceivedData = nullptr;

unsigned long nextRaindropMS = 0;

void DrawRainDrops() {
    if ( millis() < nextRaindropMS ) { return; }
    int32_t x = 40; //random (0,TFT_WIDTH);
    int32_t y = random (100,140);
    int32_t d = random (0,220);
    uint32_t c = random (0,0xffffff);
    //overlay->drawPixel(x,y,c);
    overlay->drawFastHLine(x,y,d,c);
    int32_t nx = random (x+d,240);
    int32_t ny = random (0,240);
    overlay->drawLine(x+d,y,nx,ny,c);

    nextRaindropMS=millis()+random(3000,15000);
}

void WatchfaceApplication::ParseWeatherData() {

    if ( nullptr == weatherReceivedData ) {
        Serial.println("Watchface: ERROR: OpenWeather JSON parsing: Empty string ''");
        return;
    }
    JSONVar myObject = JSON.parse(weatherReceivedData);
    if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Watchface: ERROR: OpenWeather JSON parsing: malformed JSON:");
        Serial.printf("%s\n",weatherReceivedData);
        return;
    }
    if (false == myObject.hasOwnProperty("weather")) {
        Serial.println("Watchface: ERROR: OpenWeather JSON parsing: property 'weather' not found");
        Serial.printf("%s\n",weatherReceivedData);
        return;
    }
    JSONVar weatherBranch = myObject["weather"][0];

    if (false == weatherBranch.hasOwnProperty("description")) {
        Serial.println("Watchface: ERROR: OpenWeather JSON parsing: property '[weather][0][description]' not found");
        Serial.printf("%s\n",weatherReceivedData);
        return;
    }
    weatherId = weatherBranch["id"];
    const char * weatherMainConst =  weatherBranch["main"];

    if ( nullptr != weatherMain ) {
        free(weatherMain);
        weatherMain = nullptr;
    }
    weatherMain = (char *)malloc(strlen(weatherMainConst)+1);
    strcpy(weatherMain, weatherMainConst);

    const char * weatherDescriptionConst = weatherBranch["description"];
    if ( nullptr != weatherDescription ) {
        free(weatherDescription);
        weatherDescription = nullptr;
    }
    weatherDescription = (char *)malloc(strlen(weatherDescriptionConst)+1);
    strcpy(weatherDescription, weatherDescriptionConst);

    const char * weatherIconConst = weatherBranch["icon"];
    if ( nullptr != weatherIcon ) {
        free(weatherIcon);
        weatherIcon = nullptr;
    }
    weatherIcon = (char *)malloc(strlen(weatherIconConst)+1);
    strcpy(weatherIcon, weatherIconConst);



    if (false == myObject.hasOwnProperty("main")) {
        Serial.println("Watchface: ERROR: OpenWeather JSON parsing: property 'main' not found");
        Serial.printf("%s\n",weatherReceivedData);
        return;
    }
    JSONVar mainBranch = myObject["main"];
    weatherTemp = mainBranch["temp"];

    weatherSyncDone = true;
}

void WatchfaceApplication::GetSecureNetworkWeather() {
    // @NOTE to get the PEM from remote server:
    // $ openssl s_client -connect api.openweathermap.org:443 -showcerts </dev/null | openssl x509 -outform pem > openweathermap_org.pem
    // $ echo "" | openssl s_client -showcerts -connect api.openweathermap.org:443 | sed -n "1,/Root/d; /BEGIN/,/END/p" | openssl x509 -outform PEM >api_openweathermap_org.pem
    WiFiClientSecure *client = new WiFiClientSecure;
    if (nullptr != client ) {
        client->setCACert((const char*)openweatherPEM_start);
        { // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
            HTTPClient https;
            https.setConnectTimeout(8*1000);
            https.setTimeout(8*1000);
            //https.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
            //    https://api.openweathermap.org/data/2.5/weather?q=Barcelona,ES&units=metric&APPID=b228d64ad499d78d86419fe10a131241
            String serverPath = "https://api.openweathermap.org/data/2.5/weather?q="+String("Barcelona")+"," + String("ES")  + String("&units=metric") + String("&APPID=") + String(openWeatherMapApiKey); // + String("&lang=") + String("ca");
            if (https.begin(*client, serverPath)) {
                // HTTPS
                Serial.printf("https get '%s'...\n", serverPath.c_str());
                // start connection and send HTTP header
                int httpCode = https.GET();

                // httpCode will be negative on error
                if (httpCode > 0) {
                    // HTTP header has been send and Server response header has been handled
                    Serial.printf("https get code: %d\n", httpCode);
                    // file found at server
                    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                        String payload = https.getString();
                        if ( nullptr != weatherReceivedData ) {
                            free(weatherReceivedData);
                            weatherReceivedData = nullptr;
                        }
                        weatherReceivedData = (char*)malloc(payload.length()+1);
                        strcpy(weatherReceivedData,payload.c_str());
                        Serial.printf("Received:\n%s\n",weatherReceivedData);
                        //config.alreadySync = true;
                    }
                } else {
                    Serial.printf("http get failed, error: %s\n", https.errorToString(httpCode).c_str());
                }
                https.end();
            } else {
                Serial.println("http unable to connect");
            }
        // End extra scoping block
        }
        delete client;
    } else {
        Serial.println("Unable to create WiFiClientSecure");
    }
}

void WatchfaceApplication::NTPSync(void * data) {
    while(true) {
        if ( ntpSyncDone ) {
            delay(60*60*12*1000);
            ntpSyncDone = false;
        }
        WiFi.begin();
        Serial.println("NTP: Trying to bring up WiFi...");
        size_t timeout = millis()+15000;
        while ( WL_CONNECTED != WiFi.status()) {
            if ( millis() > timeout ) { break; }
            //Serial.println("WiFi: waiting for connection...");
            delay(20);
        }
        if ( millis() > timeout ) {
            Serial.println("NTP: Unable to connect to know WiFi... delayed sync!");
            WiFi.disconnect(true);
            delay(100);
            WiFi.mode(WIFI_OFF);
            delay(80*1000);
            continue;
        }
        delay(100);
        //init and get the time
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        //timeClient.begin();
        // Set offset time in seconds to adjust for your timezone, for example:
        // GMT +1 = 3600
        // GMT +8 = 28800
        // GMT -1 = -3600
        // GMT 0 = 0
        /*
        timeClient.setTimeOffset(0);
        while(!timeClient.update()) {
            yield();
            timeClient.forceUpdate();
        }
        */
        delay(100);
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            ttgo->rtc->syncToRtc(); // WTF? magic code now works? // old: why don't work as expect? (getLocalTime to RTC)
            RTC_Date d = ttgo->rtc->getDateTime();
            if (d.year != (timeinfo.tm_year + 1900) || d.month != timeinfo.tm_mon + 1
                            || d.day !=  timeinfo.tm_mday ||  d.hour != timeinfo.tm_hour
                            || d.minute != timeinfo.tm_min) {
                Serial.printf("Write RTC Fail: '%s'\n",ttgo->rtc->formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));
            } else {
                Serial.printf("Write RTC PASS: Read RTC: '%s'\n",ttgo->rtc->formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));
            }
            Serial.println("NTP sync done!");
            //@TODO esp_event_post_to(systemEventloopHandler, SYSTEM_EVENTS, SYSTEM_EVENT_READY,nullptr, 0, LUNOKIOT_EVENT_TIME_TICKS);

            ntpSyncDone = true;
        }
        WatchfaceApplication::GetSecureNetworkWeather();

        WiFi.disconnect(true);
        delay(100);
        WiFi.mode(WIFI_OFF);

        WatchfaceApplication::ParseWeatherData();

        if ( false == ntpSyncDone ) {
            Serial.println("NTP sync: Unable to get time from network... retrying in 1 minute");
            delay(60*1000);
        } else {
            Serial.println("NTP sync: Next sync countdown T-12 hours...");
            delay(60*60*12*1000);
        }
    }
    vTaskDelete(NULL);
}

void WatchfaceApplication::FreeRTOSEventReceived(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    //Serial.printf("AWWW YEAHHHH event:'%s' id: '%d' \n",base, id);
    WatchfaceApplication *self = reinterpret_cast<WatchfaceApplication*>(handler_args);
    if ( WIFI_EVENT == base) {
        if ( WIFI_EVENT_STA_START == id ) { self->wifiEnabled=true; }
        else if ( WIFI_EVENT_STA_STOP == id ) { self->wifiEnabled=false; }
    }
}

WatchfaceApplication::WatchfaceApplication() {
    watchFaceCanvas = new CanvasWidget(TFT_HEIGHT,TFT_WIDTH);
    watchFaceCanvas->canvas->fillSprite(TFT_BLACK);

    backgroundCanvas = new CanvasWidget(TFT_HEIGHT,TFT_WIDTH);
    backgroundCanvas->canvas->setSwapBytes(true);
    backgroundCanvas->canvas->pushImage(0,0,img_watchface0.width,img_watchface0.height, (uint16_t *)img_watchface0.pixel_data);
    backgroundCanvas->canvas->setSwapBytes(false);

    hourHandCanvas = new CanvasWidget(img_hours_hand.height,img_hours_hand.width);
    hourHandCanvas->canvas->pushImage(0,0,img_hours_hand.width,img_hours_hand.height, (uint16_t *)img_hours_hand.pixel_data);
    hourHandCanvas->canvas->setPivot(12,81);

    minuteHandCanvas = new CanvasWidget(img_minutes_hand.height,img_minutes_hand.width);
    minuteHandCanvas->canvas->pushImage(0,0,img_minutes_hand.width,img_minutes_hand.height, (uint16_t *)img_minutes_hand.pixel_data);
    minuteHandCanvas->canvas->setPivot(12,107);

    backlightCanvas = new CanvasWidget(img_watchface0_backlight.height,img_watchface0_backlight.width);
    backlightCanvas->canvas->setSwapBytes(true);
    backlightCanvas->canvas->pushImage(0,0,img_watchface0_backlight.width,img_watchface0_backlight.height, (uint16_t *)img_watchface0_backlight.pixel_data);
    backlightCanvas->canvas->setSwapBytes(false);


    marksCanvas = new CanvasWidget(TFT_HEIGHT,TFT_WIDTH);
    marksCanvas->canvas->fillSprite(CanvasWidget::MASK_COLOR);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, WatchfaceApplication::FreeRTOSEventReceived,this);

    xTaskCreate(WatchfaceApplication::NTPSync, "", LUNOKIOT_TASK_PROVISIONINGSTACK_SIZE,nullptr, uxTaskPriorityGet(NULL), &NTPTaskHandler);

    bottomRightButton = new ActiveRect(172,172,70,50,[&, this]() {
        LaunchApplication(new MainMenuApplication());
    });

    nextWatchFaceFullRedraw = 0;

}
WatchfaceApplication::~WatchfaceApplication() {
    Serial.printf("WatchfaceApplication: %p DELETE\n",this);
    if ( nullptr != watchFaceCanvas ) {
        delete watchFaceCanvas;
        watchFaceCanvas = nullptr;
    }
    if ( nullptr != bottomRightButton ) {
        delete bottomRightButton;
        bottomRightButton = nullptr;
    }
    if( NTPTaskHandler != NULL ) { vTaskDelete( NTPTaskHandler ); }
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, WatchfaceApplication::FreeRTOSEventReceived);
    if ( nullptr != backlightCanvas ) {
        delete backlightCanvas;
        backlightCanvas = nullptr;
    }
    if ( nullptr != minuteHandCanvas ) {
        delete minuteHandCanvas;
        minuteHandCanvas = nullptr;
    }
    if ( nullptr != hourHandCanvas ) {
        delete hourHandCanvas;
        hourHandCanvas = nullptr;
    }
    if ( nullptr != backgroundCanvas ) {
        delete backgroundCanvas;
        backgroundCanvas = nullptr;
    }
    if ( nullptr != weatherReceivedData ) {
        free(weatherReceivedData);
        weatherReceivedData = nullptr;
    }
    if ( nullptr != weatherMain) {
        free(weatherMain);
        weatherMain = nullptr;
    }
    if ( nullptr != weatherDescription) {
        free(weatherDescription);
        weatherDescription = nullptr;
    }
    if ( nullptr != weatherIcon) {
        free(weatherIcon);
        weatherIcon = nullptr;
    }
    if ( nullptr != hourClockHandCache ) {
        hourClockHandCache->deleteSprite();
        delete hourClockHandCache;
        hourClockHandCache = nullptr;
    }
    if ( nullptr != minuteClockHandCache ) {
        minuteClockHandCache->deleteSprite();
        delete minuteClockHandCache;
        minuteClockHandCache = nullptr;
    }
}

void WatchfaceApplication::PrepareDataLayer() {

    int32_t posX = 40;
    int32_t posY = 170;

    // connectivity notifications
    if ( wifiEnabled ) {
        marksCanvas->canvas->fillCircle(posX, posY, 5, TFT_GREEN);
        marksCanvas->canvas->drawXBitmap(posX+10,posY-12,img_wifi_24_bits, img_wifi_24_width, img_wifi_24_height, TFT_WHITE);
    }
    if ( vbusPresent ) {
        posX=55;
        posY=190;
        uint32_t color = TFT_DARKGREY;
        if ( ttgo->power->isChargeing() ) {
            color = TFT_GREEN;
        }
        marksCanvas->canvas->fillCircle(posX, posY, 5, color);
        marksCanvas->canvas->drawXBitmap(posX+10,posY-12,img_usb_24_bits, img_usb_24_width, img_usb_24_height, TFT_WHITE);
    }


    char textBuffer[64] = { 0 };
    marksCanvas->canvas->setTextFont(0);
    marksCanvas->canvas->setTextSize(2);
    marksCanvas->canvas->setTextDatum(TL_DATUM);
    marksCanvas->canvas->setTextWrap(false,false);

    // weekday/day/month/year
    if ( ntpSyncDone ) {
        sprintf(textBuffer,"%02d/%02d", timeinfo.tm_mday,timeinfo.tm_mon+1);
        posX = 150;
        posY = 94;
        marksCanvas->canvas->setTextColor(TFT_BLACK);
        marksCanvas->canvas->drawString(textBuffer, posX+2, posY+2);
        marksCanvas->canvas->drawString(textBuffer, posX-2, posY-2);
        marksCanvas->canvas->setTextColor(TFT_WHITE);
        marksCanvas->canvas->drawString(textBuffer, posX, posY);
    }
    if ( weatherSyncDone ) { // tilt with the arm makes date grows and glow


        /*
        uint8_t colBase = pcX*2.55;            
        if (degX < 195 ) { colBase = 33; }
        else if (degX > 260 ) { colBase = 255; }


        //Serial.printf("degX: %f, degY: %f, degZ: %f\n", degX,degY,degZ);
        Serial.printf("degX: %f\n", degX);
        if (degX > 260 ) {  overlay->setTextSize(4); }
        else if (degX > 230 ) {  overlay->setTextSize(3); }
        else { overlay->setTextSize(2); }


        overlay->setTextDatum(BL_DATUM);

        if (degX > 260 ) {  overlay->setTextSize(3); }
        else if (degX > 230 ) {  overlay->setTextSize(2); }
        else { overlay->setTextSize(1); }
        sprintf(textBuffer,"%04d", 1900+timeinfo.tm_year);
        posY=112;
        overlay->drawString(textBuffer, posX,posY);
        */

        // temperature
        if ( -1000 != weatherTemp ) {
            sprintf(textBuffer,"%2.1fc", weatherTemp);
            posX = 150;
            posY = 74;
            marksCanvas->canvas->setTextColor(TFT_WHITE);
            marksCanvas->canvas->drawString(textBuffer,posX, posY);//, 185, 96);
        }
    }
    // steps
    uint32_t activityColor = TFT_WHITE;
    bool doingSomething = false;
    posX = 40;
    posY = 74;
    if ( vbusPresent ) {
        activityColor = TFT_DARKGREY;
    } else if ( nullptr != currentActivity ) {
        if ( 0 == strcmp("BMA423_USER_WALKING",currentActivity)) {
            activityColor = TFT_GREEN;
            doingSomething=true;
        } else if ( 0 == strcmp("BMA423_USER_RUNNING",currentActivity)) {
            activityColor = TFT_YELLOW;
            doingSomething=true;
        }
        if ( doingSomething ) {
            marksCanvas->canvas->fillCircle(posX-10, posY+6, 5, activityColor );
        }
    }
    marksCanvas->canvas->setTextColor(activityColor);
    marksCanvas->canvas->setTextSize(2);
    marksCanvas->canvas->setTextDatum(TL_DATUM);
    sprintf(textBuffer,"%d", stepCount);
    marksCanvas->canvas->drawString(textBuffer, posX, posY);

    // batt pc
    if ( batteryPercent > -1 ) {
        uint32_t battColor = TFT_DARKGREY;
        posX = 40;
        posY = 150;
        bool battActivity= false; 
        if ( vbusPresent ) {
            battColor = TFT_GREEN;
            battActivity = true;
        } else if ( batteryPercent < 10 ) {
                battActivity = true;
                battColor = TFT_RED;
        } else if ( batteryPercent < 35 ) { battColor = TFT_YELLOW; }
        //else if ( batteryPercent > 70 ) { battColor = TFT_GREEN; }
        marksCanvas->canvas->setTextColor(battColor);
        int battFiltered = batteryPercent;
        if ( battFiltered > 99 ) { battFiltered=99; }
        sprintf(textBuffer,"%2d%%", battFiltered);
        marksCanvas->canvas->setTextDatum(CL_DATUM);
        marksCanvas->canvas->drawString(textBuffer, posX, posY);
        marksCanvas->canvas->fillCircle(posX-10, posY-1, 5, battColor); // Alarm: red dot

        /*
        if ( batteryPercent < lastBattPC ) { 
            unsigned long timeLastBatteryStepdown = millis()-lastBattPCTimeMs;
            remainingBattTimeS = (timeLastBatteryStepdown*batteryPercent)/1000;
        }
        if ( false == vbusPresent ) {
            uint16_t s = remainingBattTimeS % 60;
            uint16_t t = (remainingBattTimeS - s)/60;
            uint16_t m = t % 60;
            t = (t - m)/60;
            uint16_t h = t;

            if ( remainingBattTimeS/60 > 60 ) {
                sprintf(textBuffer,"%02dh %02dm",h , m);
            } else {
                sprintf(textBuffer,"%02dm %02ds", m, s);
            }
            lastBattPCTimeMs = millis();
            dataLayer->setTextColor(TFT_WHITE);
            dataLayer->setTextDatum(TL_DATUM);
            dataLayer->setTextSize(1);
            dataLayer->drawString(textBuffer, 36,162);
        }

        */
    }

    if ( nullptr != weatherMain ) {
        marksCanvas->canvas->setTextFont(0);
        marksCanvas->canvas->setTextSize(2);
        marksCanvas->canvas->setTextDatum(CC_DATUM);
        marksCanvas->canvas->setTextWrap(false,false);
        marksCanvas->canvas->setTextColor(TFT_WHITE);
        marksCanvas->canvas->drawString(weatherMain, 120,40);
    }
    
    if ( nullptr != weatherDescription ) {
        marksCanvas->canvas->setTextFont(0);
        marksCanvas->canvas->setTextSize(1);
        marksCanvas->canvas->setTextDatum(CC_DATUM);
        marksCanvas->canvas->setTextWrap(false,false);
        marksCanvas->canvas->setTextColor(TFT_BLACK);
        marksCanvas->canvas->drawString(weatherDescription, 122,62);
        marksCanvas->canvas->setTextColor(TFT_WHITE);
        marksCanvas->canvas->drawString(weatherDescription, 120,60);
    }
}





unsigned long nextLedTimeMS = 0;
bool desiredLedState = false;

bool NotificationDrawCallback(int x, int y, int cx, int cy, double angle, int step, void * payload) {
    if ( angle > 90 ) { return true; }
    if ( 1 == (int(angle) % 12) ) {
        //TFT_eSprite *watchFaceCanvas = (TFT_eSprite *)payload;
        CanvasWidget *watchFaceCanvas = (CanvasWidget *)payload;
        watchFaceCanvas->canvas->fillCircle(x,y,3,TFT_DARKGREY);
    }
    return true;
}

bool SecondsCallback(int x, int y, int cx, int cy, double angle, int step, void * payload) {
    if ( (currentSec*6) == int(angle)) {
        //TFT_eSprite *watchFaceCanvas = (TFT_eSprite *)payload;
        CanvasWidget *watchFaceCanvas = (CanvasWidget *)payload;
        watchFaceCanvas->canvas->drawLine(x,y,cx,cy, TFT_RED);
        watchFaceCanvas->canvas->fillCircle(x,y,5,TFT_RED);
        watchFaceCanvas->canvas->fillCircle(cx,cy,10,TFT_RED); // need a red cap
        return false;
    }
    return true;
}

bool WatchfaceApplication::Tick() {
    bottomRightButton->Interact(touched, touchX, touchY);


/*
    // draw turnable sphere counter
    if ( ( touched ) && ( false == lastTouched )) {
        // dirty HACK manipulating the angle/distance calculated from UI
        downTouchX = 120;
        downTouchY = 120;
        subsTouchDragAngle = touchDragAngle;
    } else if ( ( touched ) && ( lastTouched )) {
        sphereRotation = touchDragAngle-subsTouchDragAngle;
    } else if ( ( false == touched ) && ( lastTouched )) {
        const int divisor = 6; // 60 seconds
        sphereRotation=int(sphereRotation/divisor)*divisor;
        subsTouchDragAngle = 0;
    }
    lastSphereRotation = sphereRotation;
    lastTouched = touched;
*/
    // LED control
    if (millis() > nextLedTimeMS ) {
        desiredLedState=(!desiredLedState);
        uint32_t color = canvas->color24to16(0xff0000); // RED
        if ( vbusPresent ) {
            color = canvas->color24to16(0x98c2c2); // WHITE-BLUE
            if ( desiredLedState ) {
                nextLedTimeMS=millis()+1200;
            } else {
                nextLedTimeMS=millis()+500;
            }
        } else {
            color = canvas->color24to16(0x6C0000); // dark red
            if ( desiredLedState ) {
                nextLedTimeMS=millis()+1000;
            } else {
                nextLedTimeMS=millis()+3000;
            }
        }
        AlertLedEnable(desiredLedState, color);
       // return true;

        /*
        uint16_t *dataLed = (uint16_t*)ps_calloc(12*12,sizeof(uint16_t));
        overlay->readRect(120-6,22-6,12,12,dataLed);
        for(int i=0;i<12*12;i++) {
            Serial.printf("%2x ", dataLed[i]);
        }
        Serial.println("");
        ttgo->tft->pushRect(120-6,22-6,12,12,dataLed);
        free(dataLed);

        overlay->pushRotated(watchFaceCanvas->canvas,0,CanvasWidget::MASK_COLOR);
        */
    }

    /*
        Serial.printf("ANGLE: %.3lf\n", touchDragAngle);
        Serial.printf("Distance: %.3lf\n", touchDragDistance);
        Serial.printf("VECTOR: X: %d Y: %d\n", touchDragVectorX,touchDragVectorY);
    */
    if ( millis() > nextWatchFaceFullRedraw ) {
        unsigned long delta = millis();
        // clean the buffer
        watchFaceCanvas->canvas->fillSprite(TFT_BLACK);

        const float ROTATION = (360/24);
        // already NTP sync done?
        //if ( ntpSyncDone ) {
            // get current timedate
            time_t now;
            struct tm * tmpTime;
            time(&now);
            tmpTime = localtime(&now);
            memcpy(&timeinfo,tmpTime, sizeof(struct tm));
            cycleHour = ROTATION*(timeinfo.tm_hour);
            currentSec=timeinfo.tm_sec;
            currentMin=timeinfo.tm_min;
            currentHour=timeinfo.tm_hour;

            /*
            if (getLocalTime(&timeinfo,10)) {
                cycleHour = ROTATION*(timeinfo.tm_hour);
                currentSec=timeinfo.tm_sec;
                currentMin=timeinfo.tm_min;
                currentHour=timeinfo.tm_hour;
            }*/

        //}



        /* else {
            // default advertisement time 10:10 (happy clock!)
            currentSec=37;
            currentMin=9;
            currentHour=10;
            cycleHour=(cycleHour+4)%360;
        } */
        // daynight decorative sphere
        /*
        if ( lastCycleHour != cycleHour ) {
            // clear te cache
           // dayNightCacheCanvas->canvas->fillSprite(CanvasWidget::MASK_COLOR);
            dayNightCacheCanvas->canvas->setPivot(0,0);
            //dayNightCanvas->canvas->pushRotated(dayNightCacheCanvas->canvas,cycleHour,CanvasWidget::MASK_COLOR);
        }
        dayNightCacheCanvas->canvas->setPivot(0,0);
        //dayNightCacheCanvas->canvas->pushRotated(watchFaceCanvas->canvas,0,CanvasWidget::MASK_COLOR);
        */




        
        /*
        // battery
        if ( -1 != batteryPercent) {
            //demobattPC--;
            //if ( demobattPC < 0 ) { demobattPC = 99; }
            //if ( demobattPC > 99 ) { demobattPC = 0; }
            //batteryPercent = demobattPC;
            int16_t batDeg = ((batteryPercent*225)/100);
            //Serial.printf("BATT: %d%% DEG: %d\n", batteryPercent, batDeg);

            // fake battery gauge
            if ( batteryPercent != lastBattPC ) { 
                batteryCacheCanvas->canvas->fillSprite(CanvasWidget::MASK_COLOR);
                batteryCacheCanvas->canvas->setPivot(38,-21);

                batteryCanvas->canvas->pushRotated(batteryCacheCanvas->canvas,batDeg, CanvasWidget::MASK_COLOR);
            }
            batteryCacheCanvas->canvas->setPivot(38,-21);
            batteryCacheCanvas->canvas->pushRotated(watchFaceCanvas->canvas,0, CanvasWidget::MASK_COLOR);
        }

            */

        // the sphere (eyecandy)
        // @TODO this can be tunned by the user
        if ( ( currentHour > 17 ) || ( currentHour < 9 ))  {
           backlightCanvas->canvas->pushRotated(watchFaceCanvas->canvas,0);
        } else {
           backgroundCanvas->canvas->pushRotated(watchFaceCanvas->canvas,0);
        }

        // weather icon
        //watchFaceCanvas->canvas->fillRect(120 - (img_weather_200.width/2),52,80,46,TFT_BLACK);
        if ( -1 != weatherId ) {
            if ( ( 200 <= weatherId ) && ( 300 > weatherId ) ) {
                watchFaceCanvas->canvas->pushImage(120 - (img_weather_200.width/2),52,img_weather_200.width,img_weather_200.height, (uint16_t *)img_weather_200.pixel_data);
            } else if ( ( 300 <= weatherId ) && ( 400 > weatherId ) ) {
                watchFaceCanvas->canvas->pushImage(120 - (img_weather_300.width/2),52,img_weather_300.width,img_weather_300.height, (uint16_t *)img_weather_300.pixel_data);
            } else if ( ( 500 <= weatherId ) && ( 600 > weatherId ) ) {
                watchFaceCanvas->canvas->pushImage(120 - (img_weather_500.width/2),52,img_weather_500.width,img_weather_500.height, (uint16_t *)img_weather_500.pixel_data);
            } else if ( ( 600 <= weatherId ) && ( 700 > weatherId ) ) {
                watchFaceCanvas->canvas->pushImage(120 - (img_weather_600.width/2),52,img_weather_600.width,img_weather_600.height, (uint16_t *)img_weather_600.pixel_data);
            } else if ( ( 700 <= weatherId ) && ( 800 > weatherId ) ) {
                Serial.println("@TODO Watchface: openweather 700 condition code");
                //watchFaceCanvas->canvas->pushImage(120 - (img_weather_800.width/2) ,52,img_weather_800.width,img_weather_800.height, (uint16_t *)img_weather_800.pixel_data);
               watchFaceCanvas->canvas->pushImage(120 - (img_weather_800.width/2),52,img_weather_800.width,img_weather_800.height, (uint16_t *)img_weather_800.pixel_data);
                //watchFaceCanvas->canvas->pushImage(144,52,img_weather_600.width,img_weather_600.height, (uint16_t *)img_weather_600.pixel_data);
            } else if ( ( 800 <= weatherId ) && ( 900 > weatherId ) ) {
                watchFaceCanvas->canvas->pushImage(120 - (img_weather_800.width/2) ,52,img_weather_800.width,img_weather_800.height, (uint16_t *)img_weather_800.pixel_data);
            }
        }


        unsigned long localtimeMeasure= millis();
        

        // DRAW THE NEDDLES Or HANDS
        watchFaceCanvas->canvas->fillCircle(120,120,20,TFT_BLACK);

        if ( lastHour!=currentHour ) {
            // get new size
            int16_t min_x ;
            int16_t min_y;
            int16_t max_x;
            int16_t max_y;
            hourHandCanvas->canvas->getRotatedBounds(currentHour*30,
                    img_hours_hand.width,img_hours_hand.height,12,98,&min_x,&min_y,&max_x,&max_y);
            //Serial.printf("min_x: %d min_y: %d max_x: %d max_y: %d\n", min_x, min_y, max_x, max_y);
            if ( nullptr != hourClockHandCache ) {
                hourClockHandCache->deleteSprite();
                delete hourClockHandCache;
                hourClockHandCache = nullptr;
            }
            // create sprite
            hourClockHandCache = new TFT_eSprite(ttgo->tft);
            hourClockHandCache->createSprite(abs(min_x)+max_x,abs(min_y)+max_y);
            hourClockHandCache->setColorDepth(16);
            hourClockHandCache->setPivot(abs(min_x),abs(min_y));
            // copy & rotate
            hourClockHandCache->fillSprite(CanvasWidget::MASK_COLOR);
            hourHandCanvas->canvas->pushRotated(hourClockHandCache,currentHour*30,CanvasWidget::MASK_COLOR);
        }
        hourClockHandCache->pushRotated(watchFaceCanvas->canvas,0,CanvasWidget::MASK_COLOR);

        if ( lastMin!=currentMin ) {
            // get new size
            int16_t min_x ;
            int16_t min_y;
            int16_t max_x;
            int16_t max_y;
            minuteHandCanvas->canvas->getRotatedBounds(currentMin*6, img_minutes_hand.width,img_minutes_hand.height,12,102,&min_x,&min_y,&max_x,&max_y);
            //Serial.printf("min_x: %d min_y: %d max_x: %d max_y: %d\n", min_x, min_y, max_x, max_y);
            if ( nullptr != minuteClockHandCache ) {
                minuteClockHandCache->deleteSprite();
                delete minuteClockHandCache;
                minuteClockHandCache = nullptr;
            }
            // create sprite
            minuteClockHandCache = new TFT_eSprite(ttgo->tft);
            minuteClockHandCache->createSprite(abs(min_x)+max_x,abs(min_y)+max_y);
            minuteClockHandCache->setColorDepth(16);
            minuteClockHandCache->setPivot(abs(min_x),abs(min_y));
            // copy & rotate
            minuteClockHandCache->fillSprite(CanvasWidget::MASK_COLOR);
            minuteHandCanvas->canvas->pushRotated(minuteClockHandCache,currentMin*6,CanvasWidget::MASK_COLOR);
        }
        minuteClockHandCache->pushRotated(watchFaceCanvas->canvas,0,CanvasWidget::MASK_COLOR);            //minuteHandCacheCanvas->canvas->fillSprite(CanvasWidget::MASK_COLOR);

        /* SECONDS HAND
        if ( lastSec!=currentSec ) {
            // get new size
            int16_t min_x ;
            int16_t min_y;
            int16_t max_x;
            int16_t max_y;
            secondHandCanvas->canvas->getRotatedBounds(currentSec*6, img_seconds_hand.width,img_seconds_hand.height,7,106,&min_x,&min_y,&max_x,&max_y);
            if ( nullptr != secondClockHandCache ) {
                secondClockHandCache->deleteSprite();
                delete secondClockHandCache;
                secondClockHandCache = nullptr;
            }
            // create sprite
            secondClockHandCache = new TFT_eSprite(ttgo->tft);
            secondClockHandCache->createSprite(abs(min_x)+max_x,abs(min_y)+max_y);
            secondClockHandCache->setColorDepth(16);
            secondClockHandCache->setPivot(abs(min_x),abs(min_y));
            // copy & rotate
            secondClockHandCache->fillSprite(CanvasWidget::MASK_COLOR);
            secondHandCanvas->canvas->pushRotated(secondClockHandCache,currentSec*6,CanvasWidget::MASK_COLOR);
        }
        secondClockHandCache->pushRotated(watchFaceCanvas->canvas,0,CanvasWidget::MASK_COLOR);            //minuteHandCacheCanvas->canvas->fillSprite(CanvasWidget::MASK_COLOR);
        */
        DescribeCircle(120, 120, 80, SecondsCallback, (void *)marksCanvas);

        PrepareDataLayer();


        DescribeCircle(120, 120, 110, NotificationDrawCallback, (void *)marksCanvas);

        marksCanvas->DrawTo(watchFaceCanvas->canvas);
        marksCanvas->canvas->fillSprite(CanvasWidget::MASK_COLOR);

        watchFaceCanvas->DrawTo(canvas);
        //localtimeMeasure
        unsigned long deltaMS = millis()-localtimeMeasure;
        //Serial.printf("CLOCK ROTATION TIME: %d ms\n", deltaMS);


        /*
        if ( sphereRotation != lastSphereRotation ) {
            // copy & rotate
            marksCanvasCache->fillSprite(CanvasWidget::MASK_COLOR);
            marksCanvas->canvas->pushRotated(marksCanvasCache,sphereRotation,CanvasWidget::MASK_COLOR);
        }
        marksCanvasCache->pushRotated(watchFaceCanvas->canvas,0,CanvasWidget::MASK_COLOR);

        // drop fresh screen to the UI
        
        overlay->pushRotated(canvas,0,CanvasWidget::MASK_COLOR);

        
        //Serial.println("FULL REDRAW");    
                    */


        // DRAW THE INFORMATION

        // dump to output ( see code beyond the clock needles)
        /*
        if (degX > 260 ) {  dataLayer->pushRotated(overlay,0,TFT_TRANSPARENT); }
        else {*/
            //dataLayer->pushRotated(watchFaceCanvas->canvas,0,CanvasWidget::MASK_COLOR);
        //}



        /*
        // batt calculated remaining time
        static char battRemains[128] = { 0 };
        if (ttgo->power->isBatteryConnect()) {
                if ( battPC < lastBattPC ) { 
                    unsigned long timePC = millis()-lastBattPCTimeMs;
                    unsigned long remainingBattTimeS = (timePC*battPC)/1000;

                    uint16_t s = remainingBattTimeS % 60;
                    uint16_t t = (remainingBattTimeS - s)/60;
                    uint16_t m = t % 60;
                    t = (t - m)/60;
                    uint16_t h = t;

                    if ( remainingBattTimeS/60 > 60 ) {
                        sprintf(battRemains,"%02dh %02dm",h , m);
                    } else {
                        sprintf(battRemains,"%02dm %02ds", m, s);
                    }
                    lastBattPCTimeMs = millis();
                }
        } else {
            // demo mode
            sprintf(battRemains,"%02dm %02ds", random(0,60), random(0,60));
        }
        watchFaceCanvas->canvas->setTextFont(0);
        watchFaceCanvas->canvas->setTextSize(1);
        watchFaceCanvas->canvas->setTextDatum(TL_DATUM);
        watchFaceCanvas->canvas->setTextColor(TFT_WHITE);
        watchFaceCanvas->canvas->drawString(battRemains, 36,162);
        */

        lastCycleHour = cycleHour;
        lastBattPC  = batteryPercent; 

        lastSec=currentSec;
        lastMin=currentMin;
        lastHour=currentHour;


        nextWatchFaceFullRedraw = millis()+(1000);
        return true;
    }
    return false;
}









