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
#include "RingBuffer.h"

class CheezPPG 
{
public:
    CheezPPG(int inputPin, int sampleRate);
    
    void setWearThreshold(int Threshold = 80);

    bool checkSampleInterval();
    void ppgProcess(); 
    bool getPpgisWear() const { return _isWear; }
    bool getPpgPeak() const{ return _peak; } 
    int getRawPPG()const { return _rawPPG; } 
    int getAvgPPG()const { return _avgPPG; }
    int getFilterPPG()const { return _filteredPPG; }
    float getPpgHr()const{ return _hr; }
    float getPpgHrv()const{return _hrv; } 

private:
    int8_t _inputPin;
    int _sampleRate;
    int _wearThreshold = 80;
    static const int _stableCount = 180;
 
    bool _peak = false;
    bool _isWear = false;
    int _wearCount = 0;
    int _rawPPG = 0;
    int  _avgPPG = 0;
    int  _filteredPPG = 0;
    float _hr = 0, _hrv = 0;
 
    RingBuffer<int, 4> _rrBuffer;
    RingBuffer<int, 8> _avgBuffer; 

    void calculateHRHRV(bool getPeak);
    float AverageFilter(float input);
    float bandpassFilter(float input);
    bool detectWearStatus(int ppgValue);
    bool detectPPGPeak(float new_sample);
};

#endif // CHEEZ_PPG_H
