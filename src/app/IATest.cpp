#include <Arduino.h>
#include <LilyGoWatch.h>
#include "../lunokiot_config.hpp"

#include "../UI/widgets/CanvasWidget.hpp" // for color mask
#include "IATest.hpp"

uint8_t eyeSize = 8;

IATestApplication::IATestApplication() {
    LunokIoTApplication();
/*
    /// COLOR ORDER NIGHTMARE....
    testButton[0] = new NotificationWidget(0,0,[&,this]() {
        Serial.println("@TODO IATestApplication BUTTON TEST 0");
    }, ttgo->tft->color24to16(0x79fa21)); //GRB

    testButton[1] = new NotificationWidget(50,5,[&,this]() {
        Serial.println("@TODO IATestApplication BUTTON TEST 1");
    }, ttgo->tft->color24to16(0x21fa79)); // BRG

    testButton[2] = new NotificationWidget(95,5,[&,this]() {
        Serial.println("@TODO IATestApplication BUTTON TEST 2");
    }, ttgo->tft->color24to16(0x2179fa)); // BGR

    testButton[3] = new NotificationWidget(140,5,[&,this]() {
        Serial.println("@TODO IATestApplication BUTTON TEST 3");
    }, ttgo->tft->color24to16(0x7921fa)); //GBR

    testButton[4] = new NotificationWidget(185,5,[&,this]() {
        Serial.println("@TODO IATestApplication BUTTON TEST 4");
    }, ttgo->tft->color24to16(0xfa7921)); // RGB
    */

    // Buffers to do the work :)

    eyeLensBuffer = new CanvasWidget(TFT_HEIGHT-40, TFT_WIDTH-40);
    compositeBuffer = new CanvasWidget(TFT_HEIGHT, TFT_WIDTH); // dirty buffer to do composition and push
    watchFacebackground = new CanvasWidget(TFT_HEIGHT,TFT_WIDTH); // static background (fast copy)

    accelSphere = new CanvasWidget(150, 150); // BMP info on a spin
    compositeBuffer150 = new CanvasWidget(150,150); // dirty buffer to do things with accelSphere

    eyeLensBuffer->GetCanvas()->fillCircle(eyeLensBuffer->canvas->height()/2, eyeLensBuffer->canvas->width()/2,98,ttgo->tft->color24to16(0x2a040c));//0x686890));
    eyeLensBuffer->GetCanvas()->fillCircle(eyeLensBuffer->canvas->height()/2, eyeLensBuffer->canvas->width()/2,87,CanvasWidget::MASK_COLOR);
    eyeLensBuffer->GetCanvas()->fillRect(0,0,100,200,CanvasWidget::MASK_COLOR);


    // generating back mask (circle)
    // base sphere (using transparent on center)
    compositeBuffer->GetCanvas()->fillSprite(TFT_BLACK);
    compositeBuffer->GetCanvas()->fillCircle(TFT_HEIGHT/2, TFT_WIDTH/2,80,CanvasWidget::MASK_COLOR);

    // outher circle decoration
    compositeBuffer->GetCanvas()->fillCircle(TFT_HEIGHT/2, TFT_WIDTH/2,120,ttgo->tft->color24to16(0x383838));
    compositeBuffer->GetCanvas()->fillCircle(TFT_HEIGHT/2, TFT_WIDTH/2,116,CanvasWidget::MASK_COLOR);
    // eraser ;-P
    compositeBuffer->GetCanvas()->fillRect(0,0,118,TFT_HEIGHT,CanvasWidget::MASK_COLOR);
    compositeBuffer->GetCanvas()->fillRect(132,0,108,TFT_HEIGHT,CanvasWidget::MASK_COLOR);
    compositeBuffer->GetCanvas()->fillRect(110,120,30,120,CanvasWidget::MASK_COLOR);

    watchFacebackground->GetCanvas()->fillSprite(CanvasWidget::MASK_COLOR);
    // build the circle marks
    for (int16_t sphereAngle = 0;sphereAngle<360;sphereAngle+=24) {
        compositeBuffer->GetCanvas()->pushRotated(watchFacebackground->GetCanvas(),sphereAngle,CanvasWidget::MASK_COLOR);
    }

    // upper single border circle
    compositeBuffer->GetCanvas()->drawCircle(TFT_HEIGHT/2, TFT_WIDTH/2,150, canvas->color24to16(0x383838));

    // inner circle
    compositeBuffer->GetCanvas()->fillCircle(TFT_HEIGHT/2, TFT_WIDTH/2,80, canvas->color24to16(0x383838));
    compositeBuffer->GetCanvas()->fillCircle(TFT_HEIGHT/2, TFT_WIDTH/2,75,CanvasWidget::MASK_COLOR);
    // right side circle cut
    compositeBuffer->GetCanvas()->fillRect(180,84,120,74, TFT_BLACK);
    compositeBuffer->GetCanvas()->fillCircle(TFT_WIDTH/2,TFT_HEIGHT/2,75, CanvasWidget::MASK_COLOR);

//    compositeBuffer->GetCanvas()->fillRect(120,80,120,80, TFT_BLACK);
    // right side circle cut lines
    compositeBuffer->GetCanvas()->fillRect(185,80,55,5, compositeBuffer->canvas->color24to16(0x383838));
    compositeBuffer->GetCanvas()->fillRect(185,160,55,5, compositeBuffer->canvas->color24to16(0x383838));

    // Dump to buffer
    compositeBuffer->GetCanvas()->pushRotated(watchFacebackground->GetCanvas(),0,CanvasWidget::MASK_COLOR);

    const int32_t notificationRouletteAngle=35;
    circleNotifications = new CanvasWidget(TFT_HEIGHT, TFT_WIDTH);

    //circleNotifications->canvas->setColorDepth(4);
    //circleNotifications->canvas->createPalette(default_4bit_palette,4);
    // default_4bit_palette
    //circleNotifications->canvas->createPalette(uint16_t *palette = nullptr, uint8_t colors = 16); 
    // notification pane draw

    // notif 1 test
    circleNotifications->GetCanvas()->fillCircle(TFT_WIDTH/2,38,20,canvas->color24to16(0x0));
    circleNotifications->GetCanvas()->drawCircle(TFT_WIDTH/2,38,18,canvas->color24to16(0xffffff));
    circleNotifications->GetCanvas()->fillCircle(TFT_WIDTH/2,38,16,canvas->color24to16(0x00ff00));

    compositeBuffer->GetCanvas()->fillSprite(CanvasWidget::MASK_COLOR);

    circleNotifications->GetCanvas()->pushRotated(compositeBuffer->GetCanvas(),notificationRouletteAngle,CanvasWidget::MASK_COLOR);
    circleNotifications->GetCanvas()->fillSprite(CanvasWidget::MASK_COLOR);
    compositeBuffer->GetCanvas()->pushRotated(circleNotifications->GetCanvas(),0,CanvasWidget::MASK_COLOR);
    // notif 2 test
    circleNotifications->GetCanvas()->fillCircle(TFT_WIDTH/2,38,20,canvas->color24to16(0x0));
    circleNotifications->GetCanvas()->drawCircle(TFT_WIDTH/2,38,18,canvas->color24to16(0xffffff));
    circleNotifications->GetCanvas()->fillCircle(TFT_WIDTH/2,38,16,canvas->color24to16(0x0000ff));

    compositeBuffer->GetCanvas()->fillSprite(CanvasWidget::MASK_COLOR);
    circleNotifications->GetCanvas()->pushRotated(compositeBuffer->GetCanvas(),notificationRouletteAngle,CanvasWidget::MASK_COLOR);
    circleNotifications->GetCanvas()->fillSprite(CanvasWidget::MASK_COLOR);
    compositeBuffer->GetCanvas()->pushRotated(circleNotifications->GetCanvas(),0,CanvasWidget::MASK_COLOR);

    // notif 3 test
    circleNotifications->GetCanvas()->fillCircle(TFT_WIDTH/2,38,20,canvas->color24to16(0x0));
    circleNotifications->GetCanvas()->drawCircle(TFT_WIDTH/2,38,18,canvas->color24to16(0xffffff));
    circleNotifications->GetCanvas()->fillCircle(TFT_WIDTH/2,38,16,canvas->color24to16(0xff00ff));

    compositeBuffer->GetCanvas()->fillSprite(CanvasWidget::MASK_COLOR);
    circleNotifications->GetCanvas()->pushRotated(compositeBuffer->GetCanvas(),notificationRouletteAngle,CanvasWidget::MASK_COLOR);
    circleNotifications->GetCanvas()->fillSprite(CanvasWidget::MASK_COLOR);
    compositeBuffer->GetCanvas()->pushRotated(circleNotifications->GetCanvas(),0,CanvasWidget::MASK_COLOR);

    // notif 4 test
    circleNotifications->GetCanvas()->fillCircle(TFT_WIDTH/2,38,20,canvas->color24to16(0x0));
    circleNotifications->GetCanvas()->drawCircle(TFT_WIDTH/2,38,18,canvas->color24to16(0xffffff));
    circleNotifications->GetCanvas()->fillCircle(TFT_WIDTH/2,38,16,canvas->color24to16(0xff0ffa));
    
}

bool IATestApplication::Tick() {

    static unsigned long nextRedraw = 0;
    if ( nextRedraw < millis() ) {
        canvas->fillSprite(TFT_BLACK);
        canvas->setPivot(TFT_WIDTH/2,TFT_HEIGHT/2);
        

        // static int16_t notifAngle = 0;
       //  testButton[0]->x = 120;
       //  testButton[0]->y = 140;
         //testButton[0]->DrawTo(clockSphere->GetCanvas());

        // notifAngle++;
        // if ( notifAngle > 359 ) { notifAngle = notifAngle-360; }

        
        // clockSphere->GetCanvas()->fillRect(testButton[0]->x-20,testButton[0]->y-20,45+40,45+40,MYTRANSPARENT);

        nextRedraw = millis()+(1000/3);
    }

    testButton[0]->Interact(touched,touchX,touchY);

    static unsigned long assistantRedraw = 0;
    if ( assistantRedraw < millis() ) {
        // clean the composite
        compositeBuffer150->GetCanvas()->fillSprite(CanvasWidget::MASK_COLOR);
        compositeBuffer150->GetCanvas()->setPivot(75,75);
        accelSphere->GetCanvas()->pushRotated(compositeBuffer150->GetCanvas(),0,CanvasWidget::MASK_COLOR);

        int32_t randX = 8 - random(0, 16);
        int32_t randY = 8 - random(0, 16);
        int32_t randZ = 8 - random(0, 16);

        uint16_t pcXinByte = pcX*2.55;
        uint16_t pcYinByte = pcY*2.55;
        uint16_t pcZinByte = pcZ*2.55;
        
        uint16_t colorX = canvas->alphaBlend(pcXinByte,TFT_WHITE, canvas->color24to16(0xbfbf40))+randX;
        //uint16_t colorX = canvas->alphaBlend(pcX,TFT_WHITE, canvas->color24to16(0x1c02a2))+randX;
        uint16_t colorY = canvas->alphaBlend(pcYinByte,TFT_WHITE, canvas->color24to16(0xff1649))+randY;
        
        //uint16_t colorY = canvas->alphaBlend(pcY,TFT_WHITE, canvas->color24to16(0x4266f6))+randY;
        //uint16_t colorZ = canvas->alphaBlend(pcZ,TFT_WHITE, canvas->color24to16(0xff1ecf))+randZ;
        uint16_t colorZ = canvas->alphaBlend(pcZinByte,TFT_WHITE, canvas->color24to16(0x1c72c5))+randZ;

        int32_t sizeX = (pcX*0.12); if ( sizeX < 2 ) { sizeX =1; }
        int32_t sizeY = (pcY*0.12); if ( sizeY < 2 ) { sizeY =1; }
        int32_t sizeZ = (pcZ*0.12); if ( sizeZ < 2 ) { sizeZ =1; }
        /*
        compositeBuffer150->GetCanvas()->fillCircle(75,pcX+eyeSize,sizeX,colorX);
        compositeBuffer150->GetCanvas()->fillCircle(pcY+eyeSize,75,sizeY,colorY);
        compositeBuffer150->GetCanvas()->fillCircle(75+(pcZ*0.33)+eyeSize,75+(pcZ*0.33)+eyeSize,sizeZ,colorZ);
        */
        compositeBuffer150->GetCanvas()->fillCircle(75,pcX+randX,sizeX,colorX);
        compositeBuffer150->GetCanvas()->fillCircle(75,pcY+randY,sizeY,colorY);
        compositeBuffer150->GetCanvas()->fillCircle(75,pcZ+randZ,sizeZ,colorZ);
        int32_t fakeHeight = random(0, 100);
        //int32_t dcolor = random(0, 16536);
        int32_t dsize = random(1, 4);
        compositeBuffer150->GetCanvas()->fillCircle(75,fakeHeight,dsize,canvas->color24to16(0x8840bf)+randZ);

        /*
        int32_t drandX = random(0, 100);
        int32_t drandY = random(0, 100);
        int32_t dsize = random(1, 4);
        int32_t dcolor = random(0, 16536);
        compositeBuffer150->GetCanvas()->fillCircle(drandX,drandY,dsize,canvas->color24to16(0x8840bf)+randZ);
        */

        // cleanup final buffer
        accelSphere->GetCanvas()->fillSprite(CanvasWidget::MASK_COLOR);
        compositeBuffer150->GetCanvas()->setPivot(75,75);
        compositeBuffer150->GetCanvas()->pushRotated(accelSphere->GetCanvas(),3,CanvasWidget::MASK_COLOR);
        compositeBuffer150->GetCanvas()->setPivot(75,75);
/*
        static int16_t accelSphereAngle = 0;
        accelSphereAngle++;
        accelSphereAngle = accelSphereAngle % 360;
*/
        assistantRedraw = millis()+(1000/10);
        return true;
    }


    
    testButton[1]->Interact(touched,touchX,touchY);
    testButton[2]->Interact(touched,touchX,touchY);
    testButton[3]->Interact(touched,touchX,touchY);
    testButton[4]->Interact(touched,touchX,touchY);

    // watch whole clock together
    canvas->fillSprite(TFT_BLACK);
    static int16_t eyeBufferAngle = 0;
    eyeBufferAngle++;
    eyeBufferAngle = eyeBufferAngle % 360;
    eyeLensBuffer->GetCanvas()->pushRotated(canvas,360-eyeBufferAngle,CanvasWidget::MASK_COLOR);

    accelSphere->GetCanvas()->pushRotated(canvas,0,CanvasWidget::MASK_COLOR); // BMA

    watchFacebackground->GetCanvas()->pushRotated(canvas,0,CanvasWidget::MASK_COLOR); // background

    // notifications
    static int16_t notificationsAngle = 0;
    notificationsAngle+=6;
    notificationsAngle = notificationsAngle % 360;  


    // Bounding box parameters
    int16_t min_x;
    int16_t min_y;
    int16_t max_x;
    int16_t max_y;
    
/*
    testButton[0]->x = 0;
    testButton[0]->y = 0;
    testButton[0]->canvas->setPivot( 0,0);

    testButton[0]->canvas->pushRotated(circleNotifications->GetCanvas(),0,MYTRANSPARENT);
     */




///caca    testButton[0]->canvas->pushSprite(circleNotifications->canvas,10,10,MYTRANSPARENT);




//    testButton[0]->x = min_x;
 //   testButton[0]->y = min_y;
    // Get the bounding box of this rotated source Sprite

/*
    if ( circleNotifications->canvas->getRotatedBounds(testButton[0]->canvas,notificationsAngle, &min_x, &min_y, &max_x, &max_y) ) {
        //Serial.printf("getRotatedBounds: minX: %d minY: %d maxX: %d maxY: %d\n",min_x,min_y,max_x,max_y);
        
        //testButton[0]->canvas->setPivot(min_x,min_y);
         //testButton[0]->x = min_x;
         //testButton[0]->y = min_y;

    }
*/
       //testButton[0]->canvas->pushRotated(circleNotifications->canvas,MYTRANSPARENT);

        testButton[0]->DrawTo(circleNotifications->GetCanvas());

 // /////  testButton[0]->canvas->pushRotated(circleNotifications->GetCanvas(),0,MYTRANSPARENT);

  //circleNotifications->GetCanvas()->pushRotated(canvas,notificationsAngle,MYTRANSPARENT);




//circleNotifications->GetCanvas()->pushSprite(0,0);
    //int16_t anchorX = accelSphere->GetCanvas()->getPivotX();
    //int16_t anchorY = accelSphere->GetCanvas()->getPivotY();
    //Serial.println("@TODO use anchor to get the eye the impression of watch you");




    //canvas->fillCircle(120,120,eyeSize, TFT_BLACK);
    //canvas->fillCircle(120,120,eyeSize+(eyeSize/5), TFT_BLACK);
    canvas->fillCircle(120,120,eyeSize, TFT_WHITE); // use to hide the rotation glitches
    return true;

}
