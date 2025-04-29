/**
 * @file CheezPPG.h
 * @author Vecang @Cheez
 * @brief 
 * @version 0.1
 * @date 2025-04-29
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
#ifndef __CheezPPG_H__  
#define __CheezPPG_H__  
   
#include <Arduino.h>  
#include <stdint.h>  
#include "RingBuffer.h"  

#define PPG_BUFFER_LEN 18      // 峰值检测缓冲区大小  
   
class CheezPPG  
{  
  public:  
    CheezPPG(  
        uint8_t inputPin = A0,   
        unsigned long sampleRate = 125    
    );   
    ~CheezPPG() {};   
  
    bool checkSampleInterval(void);  
    void ppgProcess();  
    void setWearThreshold(int wearThreshold =80);

    int getRawPPG(void) { return _rawPPG; }  
    int getAvgPPG(void) { return _avgPPG; }   
    float getFilterPPG(void) { return _filteredPPG; }  
    uint8_t getPpgPeak(void) { return _peak; }  
    float getPpgRr(void) { return _rrAvg; }   
    float getPpgHr(void) { return _hr; }  
    float getPpgHrv(void) { return _hrv; } 
    uint8_t getPpgisWear(void) { return _isWear; } 

  private:  
  
    uint8_t _ppgPin;  
    unsigned long _sampleRate;   

    RingBuffer<int, 5> rrBuffer;   
    RingBuffer<float, 20> avgBuffer;   
    
    // 佩戴检测部分  
    uint8_t _isWear;        
    bool enableWearCheck;  
    int _wearCount;           
    int _wearThreshold;      
    int _stableCount;         

    // 峰值检测部分  
    float dataBuffer[PPG_BUFFER_LEN];  
    float meanBuffer[PPG_BUFFER_LEN];  
    float stdDevBuffer[PPG_BUFFER_LEN];  
    int dataIndex;  
    uint8_t _peak;   

    // HR/HRV部分  
    bool _ignoreReading;  
    bool _firstPulseDetected;  
    unsigned long _firstPulseTime;  
    unsigned long _secondPulseTime;  

    // 信号及结果  
    int _rawPPG;   
    float _avgPPG; 
    float _filteredPPG;   
    float _rrAvg;  
    float _hr;  
    float _hrv;  
    
    // 带通滤波器状态变量  
    float x1;  
    float x2;  
    float y1;  
    float y2;  
    
    // 时间变量  
    unsigned long _pastTime;  
    long _timer;  

    // 带通滤波器系数  
    static constexpr float BP_B0 = 0.00122714f;  
    static constexpr float BP_B1 = 0.00245428f;  
    static constexpr float BP_B2 = 0.00122714f;  
    static constexpr float BP_A1 = -1.8794700f;  
    static constexpr float BP_A2 = 0.89155200f;  

    
    float AverageFilter(float input);  
    float bandpassFilter(float input);   
    uint8_t detectWearStatus(int ppgValue);  
    uint8_t detectPPGPeak(float newSample);   
    void calculateHRHRV(void);   
};  

#endif // __CheezPPG_H__  