/**
******************************************************************************
* @file   : ppg.ino
* @brief  ：需安装Arduino库 CheezPPG
* @brief  : 输出：串口,波特率 115200
            采样率：125Hz
            引脚连接：
                  单片机  |   传感器
                  D2      |   S2(黄色线)
                  A0      |   S1(白色线)
                  5V      |   V(红色线)
                  GND     |   G(黑色线)

            串口输出内容(ASCII码)：
                  原始数据,平滑滤波数据,带通滤波数据,心跳检测数据,心率数据,HRV(SDNN)
            LED显示：
                  检测到脉搏信号闪烁一次
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

#define INPUT_PIN A0      // PPG引脚
#define LED_PIN 11        // LED引脚
#define BAUD_RATE 115200  // 串口波特率

#define SAMPLE_RATE 125  // 采样率

CheezPPG ppg(INPUT_PIN, SAMPLE_RATE);  // 采样率 125Hz

bool ppg_Peak;
int raw_PPG, avg_PPG, filter_PPG;
float hr, hrv;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  if (ppg.checkSampleInterval()) {
    ppg.ppgProcess();

    raw_PPG    =     ppg.getRawPPG();
    avg_PPG    =     ppg.getAvgPPG();
    filter_PPG =  ppg.getFilterPPG();
    ppg_Peak   =    ppg.getPpgPeak();
    hr         = (int)ppg.getPpgHr();
    hrv        =(int)ppg.getPpgHrv();

    if (!ppg.getPpgisWear()) {
      ppg_Peak = hr = hrv = 0;
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, !ppg.getPpgPeak());
    }

    Serial.println(String(raw_PPG) + "," +     // 原始数据
                   String(avg_PPG) + "," +     // 平滑滤波数据
                   String(filter_PPG) + "," +  // 带通滤波数据
                   String(ppg_Peak) + "," +    // 心跳检测数据
                   String(hr) + "," +          // 心率数据
                   String(hrv)                 // HRV(SDNN)数据
    );
  }
}
