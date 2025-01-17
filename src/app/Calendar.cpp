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
#include "LogView.hpp" // log capabilities
#include "../UI/AppTemplate.hpp"
#include "Calendar.hpp"
#include "../UI/widgets/ButtonTextWidget.hpp"

int days_in_month[]={31,28,31,30,31,30,31,31,30,31,30,31};
const char *MonthsAsChars[] = {
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"
};

int dayNumber(int day, int month, int year)  {
    static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 
                    4, 6, 2, 4 }; 
    year -= month < 3; 
    return ( year + year/4 - year/100 + 
            year/400 + t[month-1] + day) % 7; 
} 

CalendarApplication::CalendarApplication() {
    //https://www.codingunit.com/how-to-make-a-calendar-in-c
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        //lAppLog("Time: %02d:%02d:%02d %02d-%02d-%04d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, timeinfo.tm_mday, timeinfo.tm_mon, 1900 + timeinfo.tm_year);
        year = 1900 + timeinfo.tm_year;
        lAppLog("Current year: %d\n", year);
        if((year% 4) == FALSE && (year%100) != FALSE || (year%400) == FALSE) {
            // It is a leap year and February has 29 days.
            days_in_month[1] = 29;
            lAppLog("Is a leap year\n");
        }
        else {
            // It is not a leap year, so February has 28 days.
            days_in_month[1] = 28;
            lAppLog("No leap year\n");
        }
        monthDay=timeinfo.tm_mday;
        lAppLog("Current day: %d\n", monthDay);
        monthToShow=timeinfo.tm_mon; // 0 is JANUARY
        lAppLog("Current month: %d\n", monthToShow+1); // human readable 1~12
    }

    nextBtn=new ButtonTextWidget(canvas->width()-40,0,40,40,[&,this](void *nah){
        monthToShow++;
        if ( monthToShow > 11 ) { monthToShow = 11; }
    },">",ThCol(text),TFT_BLACK,false);
    lastBtn=new ButtonTextWidget(0,0,40,40,[&,this](void *nah){
        monthToShow--;
        if ( monthToShow < 0 ) { monthToShow = 0; }
    },"<",ThCol(text),TFT_BLACK,false);
    Tick(); // OR call this if no splash 
}

CalendarApplication::~CalendarApplication() {
    // please remove/delete/free all to avoid leaks!
    //delete mywidget;
}

bool CalendarApplication::Tick() {
    // put your interacts here:
    //mywidget->Interact(touched,touchX,touchY);
    TemplateApplication::btnBack->Interact(touched,touchX,touchY);
    nextBtn->Interact(touched,touchX,touchY);
    lastBtn->Interact(touched,touchX,touchY);
    int posX=0;
    int posY=0;
    const int width=32;
    const int height=32;
    const int offsetX=5;
    const int offsetY=45;
    int numWeek = 1;
    int weekday = dayNumber(1, monthToShow+1, year);
    if ( weekday == 0 ) { weekday=7; } // start on monday
    if ( touched ) {
        for(int day=1;day<=days_in_month[monthToShow];day++) {
            posX=(weekday-1)*width;
            posY=(numWeek-1)*height;
            if ( ActiveRect::InRect(touchX,touchY,offsetX+posX,offsetY+posY,width,height) ) {
                monthDay=day;
                break;
            }
            weekday++;
            if ( weekday>7) { weekday=1; numWeek++; }
        }
    }
    if ( millis() > nextRefresh ) { // redraw full canvas
        canvas->fillSprite(ThCol(background)); // use theme colors
        numWeek = 1;
        weekday = dayNumber(1, monthToShow+1, year);
        if ( weekday == 0 ) { weekday=7; } // start on monday
        for(int day=1;day<=days_in_month[monthToShow];day++) {
            //lLog("WEEK: %d DAY: %d WEEKDAY: %d\n", numWeek, day, weekday);
            posX=(weekday-1)*width;
            posY=(numWeek-1)*height;
            uint32_t color=ThCol(background_alt);
            if ( weekday > 5 ) { color=ThCol(highlight); }
            canvas->fillRoundRect(offsetX+posX,offsetY+posY,width,height,5,color);

            canvas->setTextFont(0);
            canvas->setTextSize(2);
            if ( day == monthDay ) {
                canvas->fillCircle((offsetX+(width/2))+posX,(offsetY+(width/2))+posY,width/2,ThCol(mark));
            }
            canvas->setTextColor(ThCol(text));

            canvas->setTextDatum(CC_DATUM);
            canvas->drawNumber(day,(offsetX+(width/2))+posX,(offsetY+(width/2))+posY);
            //canvas->drawString(textBuffer, posX, posY);

            weekday++;
            if ( weekday>7) { weekday=1; numWeek++; }
        }

        canvas->setTextSize(3);
        canvas->setTextDatum(TC_DATUM);
        canvas->setTextColor(ThCol(text));
        canvas->drawString(MonthsAsChars[monthToShow],canvas->width()/2,5);
        nextBtn->DrawTo(canvas);
        lastBtn->DrawTo(canvas);
        // move back button if interface need more space (below last month day)
        //lAppLog("posX: %d posY: %d\n", posX, posY);
        if ( posY >= 160) {
            TemplateApplication::btnBack->SetX(posX+width+5);
        } else {
            TemplateApplication::btnBack->SetX(0);
        }

        TemplateApplication::Tick();

        nextRefresh=millis()+(1000/8); // 8 FPS is enought for GUI
        return true;
    }
    return false;
}
