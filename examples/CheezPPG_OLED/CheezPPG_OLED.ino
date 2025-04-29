/**
******************************************************************************
* @file   : CheezPPG_OLED.ino
* @brief  ：需安装Arduino库：CheezPPG
                            Adafruit_GFX
                            Adafruit_SSD1306
* @brief  : 输出：串口,波特率 115200
            采样率：125Hz 
            引脚连接：
                  单片机  |   传感器
                  D2      |   S2(黄色线)
                  A0      |   S1(白色线)
                  5V      |   V(红色线)
                  GND     |   G(黑色线)
 
            串口输出内容(ASCII码)：
                  PPG,心率数据,HRV(SDNN)数据
            OLED显示:
                  * 佩戴检测
                  * PPG,心率数据,HRV(SDNN)数据
******************************************************************************
* @attention
*
* Copyright (c) 2024 Cheez.
* All rights reserved.
*
* This software is licensed under terms that can be found in the LICENSE file
* in the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
******************************************************************************
*/  
#include "CheezPPG.h"   
#include "zhLib.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
    
#define INPUT_PIN A0
#define LED_PIN 11
   
bool isWear = false;          
int8_t sX = 0, x = 0, sY = 60, y = 0; 
float HR = 0, HRV = 0, bpPPG = 0; 
  
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);
CheezPPG czPPG(INPUT_PIN,125);

void setup() { 
  Serial.begin(115200); 
  pinMode(LED_PIN, OUTPUT); 

  Wire.begin();
  Wire.setClock(400000L);  // 提高I2C通信速度 
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // 清屏并显示初始信息
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);

  display.drawBitmap(20, 20, cn_mai, 16, 16, 1);
  display.drawBitmap(40, 20, cn_bo, 16, 16, 1);
  display.drawBitmap(60, 20, cn_jian, 16, 16, 1);
  display.drawBitmap(80, 20, cn_ce, 16, 16, 1);
  display.display();
  delay(1000);

  display.clearDisplay();
  display.display(); 
}

void loop() { 
  if (czPPG.checkSampleInterval()) 
  {   
    czPPG.ppgProcess(); 

    bool peak  = czPPG.getPpgPeak();
    isWear = czPPG.getPpgisWear();   
    if (isWear) digitalWrite(LED_PIN, !peak);
    static bool lastwear;
    if(lastwear!=isWear)
    {
      lastwear = isWear; 
      display.clearDisplay();
    }

    HR = czPPG.getPpgHr();
    HRV = czPPG.getPpgHrv();
    bpPPG = czPPG.getFilterPPG();

  }
  
/************************************************/
  static unsigned long lastShow = 0;
  if (millis() - lastShow > 100) 
  {
    lastShow = millis(); 

    if (!isWear) {
      HR = 0;
      HRV = 0;

      display.clearDisplay();
      display.setTextSize(1);
      display.drawBitmap(6, 20, cn_wei, 16, 16, 1);
      display.drawBitmap(22, 20, cn_jian, 16, 16, 1);
      display.drawBitmap(38, 20, cn_ce, 16, 16, 1);
      display.drawBitmap(54, 20, cn_dao, 16, 16, 1);
      display.drawBitmap(70, 20, cn_mai, 16, 16, 1);
      display.drawBitmap(86, 20, cn_bo, 16, 16, 1);
      display.setCursor(0, 40);
      display.println(F("  Not Wearing"));
      display.setCursor(110, 52);
      display.setTextSize(1);
      display.println((int)bpPPG);
      
      Serial.print(bpPPG);
      Serial.print(",");
      Serial.print(0);
      Serial.print(",");
      Serial.println(0);

    } 
    else { 
      display.setCursor(0, 5);
      display.drawBitmap(10, 5, cn_xin, 16, 16, 1);
      display.drawBitmap(30, 5, cn_lv, 16, 16, 1);
      display.setCursor(50, 8);
      display.setTextSize(1);
      display.fillRect(50, 5, 78, 16, SSD1306_BLACK);
      display.println((int)HR);

      display.setCursor(0, 25);
      display.setTextSize(2);
      display.print(F(" HRV"));
      display.setCursor(50, 28);
      display.setTextSize(1);
      display.fillRect(50, 25, 78, 16, SSD1306_BLACK);
      display.println((int)HRV); 

      // 绘制曲线
      y = 64 - map(bpPPG, 0, 1024, 0, 32);
      display.drawLine(sX, sY, x, y, WHITE);
      sX = x;
      sY = y;
      x++;
      if (x >= 127) {
        x = 0;
        sX = 0;
        display.fillRect(0, 42, 128, 22, SSD1306_BLACK);
      }

      Serial.print(bpPPG);
      Serial.print(",");
      Serial.print(HR);
      Serial.print(",");
      Serial.println(HRV);
    }
    display.display();  // 刷新显示
  } 
}
 