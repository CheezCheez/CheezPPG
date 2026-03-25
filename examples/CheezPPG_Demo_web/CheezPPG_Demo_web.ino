/**
******************************************************************************
* @file   : CheezPPG_Demo.ino
* @brief  ：需安装Arduino库 CheezPPG
* @brief  : 输出：串口,波特率 115200
            采样率：125Hz
            引脚连接：
                  单片机  |   传感器
                  D2      |   S2(黄色线)
                  A0      |   S1(白色线)
                  5V      |   V(红色线)
                  GND     |   G(黑色线)

            串口输出内容(HEX码)：
                  CCCC 01 数据长度，原始数据,心跳检测数据,心率数据,HRV(SDNN)数据
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

#define INPUT_PIN A0      // S 信号输入(白色) 
#define BAUD_RATE 115200  // 串口波特率

#define OUT_DATA_NUM                   4   // 输出 数据个数
#define OUT_DATA_LEN   (OUT_DATA_NUM * 2)  // 输出 数据位数
#define BUFFER_LEN     (OUT_DATA_LEN + 4)  // 输出 总的位数

byte buffer[BUFFER_LEN] = { 0xCC, 0xCC, 0x01 };
int sendVal[OUT_DATA_NUM];



CheezPPG ppg(INPUT_PIN, 125);

void setup() {
  Serial.begin(BAUD_RATE); 
  buffer[3] = OUT_DATA_LEN;
}

void loop() {
  if (ppg.checkSampleInterval()) 
  {
    ppg.ppgProcess();

    /*************************************************************/
    bool ppg_Peak;
    int raw_PPG, avg_PPG, filter_PPG;
    float hr, hrv;

    raw_PPG = ppg.getRawPPG(); 
    ppg_Peak = ppg.getPpgPeak();
    hr = (int)ppg.getPpgHr();
    hrv = (int)ppg.getPpgHrv();

    sendVal[0] = raw_PPG; 
    sendVal[1] = ppg_Peak;
    sendVal[2] = hr;
    sendVal[3] = hrv;
 
    /*************************************************************/
    for (int i = 0; i < OUT_DATA_NUM; i++) 
    {
      buffer[4 + i * 2] = (byte)((sendVal[i] >> 8) & 0xFF);
      buffer[4 + i * 2 + 1] = (byte)((sendVal[i] >> 0) & 0xFF);
    }
    Serial.write(buffer, BUFFER_LEN);
  }
}
