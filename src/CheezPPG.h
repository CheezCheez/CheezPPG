/**
 * @file CheezPPG.h
 * @author Vecang @Cheez
 * @brief 
 * @version 1.1.1
 * @date 2025-09-19
 * 
 * @copyright Copyright (c) 2025
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

#ifndef _CHEEZPPG_H_
#define _CHEEZPPG_H_

#include "Arduino.h"

class CheezPPG {
 public:
  CheezPPG(int inputPin, int sampleRate);  // 构造函数，初始化输入引脚和采样率
  bool checkSampleInterval(void);  // 检查采样间隔是否到达

  void ppgProcess(void);
  void detectWearStatus();
  void setWearThreshold(int Threshold);

  bool getPpgisWear() const { return _isWear; }
  bool getPpgPeak() const { return _ispeakPeak; }
  bool getPpgOnset() const { return _ispeakOnset; }
  int getRawPPG() const { return _rawPPG; }
  int getAvgPPG() const { return _avgPPG; }
  int getFilterPPG() const { return _filteredPPG; }
  float getPpgHr() const { return heart_rate; }
  float getPpgHrv() const { return sdnn; }

 private:
  int _rawPPG = 0;
  int _avgPPG = 0;
  int _filteredPPG = 0;
  unsigned char ispeak = 0;        // 当前点是否为峰/谷的标志
  unsigned char _ispeakPeak = 0;   // 当前点是否为峰的标志
  unsigned char _ispeakOnset = 0;  // 当前点是否为谷的标志
  unsigned char heart_rate = 0;
  float sdnn = 0;

  uint32_t _inputPin;
  uint32_t _sampleRate = 0;  // 采样间隔计数器

  unsigned long _peak_window_HP[21];  // 用于存储21个点的峰值检测窗口
  unsigned char _HR_buffer[12];       // 存储最近12个心率值的数组
  unsigned char _HRV_buffer[12];      // 存储最近12个HRV(SDNN)值的数组
  float _RR_buffer[12];               // 存储最近12个RR间期的数组
  float _avg_PPG_buffer[5];           // 平滑滤波缓冲区
  unsigned long _moving_window_HP = 0;     // 移动平均累加器
  unsigned char _moving_window_count = 0;  // 移动平均计数器
  unsigned char _moving_window_size = 0;   // 移动平均窗口大小
  unsigned char _smallest = 0;  // 最小可能的心跳间隔（对应最高心率）
  unsigned char _found_peak = 0;        // 已找到的峰/谷计数
  unsigned char _total_found_peak = 0;  // 总共找到的峰/谷数
  unsigned int _last_peak = 0;          // 距离上一个峰值的采样点数
  unsigned int _last_onset = 0;         // 距离上一个谷值的采样点数

  unsigned long _last_onsetValueLED1;  // LED1信号的上一个谷值
  unsigned long _last_peakValueLED1;   // LED1信号的上一个峰值

  unsigned char _hr_temp = 0;
  unsigned char _hr_max = 220;  // 最大心率
  unsigned char _hr_min = 40;   // 最小心率

  unsigned char _hrv_temp = 0;
  unsigned char _hrv_max = 200;  // 最大HRV(SDNN)
  unsigned char _hrv_min = 0;    // 最小HRV(SDNN)

  static const int AVG_WINDOW_SIZE = 21;  // PPG 平滑滤波窗口大小
  float _avgBuffer[AVG_WINDOW_SIZE] = {0.0f};
  int _avgIndex = 0;
  int _avgCount = 0;
  float _avgSum = 0.0f;

  bool _isWear = false;
  int _wearThreshold = 25;
  static const unsigned int WEAR_COUNT_THRESHOLD = 50;
  unsigned int _wearCount = 0;

  void statHRMAlgo(unsigned long ppgData);
  void updateWindow(unsigned long *window, unsigned long sum, unsigned char n);
  unsigned char chooseRate(unsigned char *rate);
  unsigned char chooseHRV(unsigned char *rate);
  void updateheart_rate(unsigned char *rate, unsigned int last);
  void updateHRV(float *rrBuffer, unsigned int last);
  float AverageFilter(float input);
  float bandpassFilter(float input);

  unsigned long findMax(unsigned long *X);
  unsigned long findMin(unsigned long *X);
};

#endif /*_CHEEZPPG_H_*/