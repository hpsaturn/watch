#include <Arduino.h>
#include <LilyGoWatch.h>
#include "GraphWidget.hpp"


GraphWidget::GraphWidget(int16_t h, int16_t w, int64_t minValue, int64_t maxValue, uint32_t markColor, uint32_t backgroundColor, uint32_t outColor)
                : CanvasWidget(h,w), minValue(minValue),maxValue(maxValue),
                markColor(markColor),backgroundColor(backgroundColor), outColor(outColor) {
    graph = new CanvasWidget(h,w);
    graph->canvas->fillSprite(backgroundColor);
}

bool GraphWidget::PushValue(int64_t value) {
    graph->canvas->scroll(1,0);
    if ( value > maxValue ) {
        //Serial.printf("UI: GraphWidget: %p Pushed value '%lld' is out of bounds > %lld\n", this, value, maxValue);
        graph->canvas->drawFastVLine(0,0,canvas->height(),outColor);
        return false;
    } else if ( value < minValue ) {
        //Serial.printf("UI: GraphWidget: %p Pushed value '%lld' is out of bounds < %lld\n", this, value, minValue);
        graph->canvas->drawFastVLine(0,0,canvas->height(),outColor);
        return false;
    }

    uint32_t pcValue = 0;
    if ( maxValue == canvas->height() ) {
        pcValue = value;
    } else {
        pcValue = value * canvas->height() /maxValue;
    }
    //Serial.printf("pc: %d val: %lld max: %lld canvas: %d\n",pcValue,value,maxValue,canvas->height());
    // bottom to top
    graph->canvas->drawFastVLine(0, 0, canvas->height(), backgroundColor);
    graph->canvas->drawFastVLine(0,canvas->height()-pcValue,pcValue,markColor);
    lastValue = value;
    return true;
}


GraphWidget::~GraphWidget() {
    graph->canvas->deleteSprite();
    delete graph;
}

void GraphWidget::DrawTo(TFT_eSprite * endCanvas, int16_t x, int16_t y) {
    canvas->fillSprite(backgroundColor);
    graph->DrawTo(canvas);
    CanvasWidget::DrawTo(endCanvas,x,y);
}
