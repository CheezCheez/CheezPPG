#include "CheezPPG.h"
#include <math.h>

CheezPPG::CheezPPG(int inputPin, int sampleRate) 
    : _inputPin(inputPin), _sampleRate(sampleRate) {
}

void CheezPPG::setWearThreshold(int Threshold)
{
    _wearThreshold = Threshold;
}

bool CheezPPG::checkSampleInterval() {
    static unsigned long _pastTime;  
    static long _timer;  
    unsigned long present = micros();  
    unsigned long interval = present - _pastTime;  
    _pastTime = present;  
    _timer -= interval;  
  
    if (_timer < 0) {  
        _timer += 1000000 / _sampleRate;  
        return true;   
    }  
    return false;  
}

void CheezPPG::ppgProcess() 
{
    _rawPPG = analogRead(_inputPin);

    // 平滑滤波
    _avgPPG = AverageFilter(_rawPPG);
    _filteredPPG = bandpassFilter(_avgPPG);
    _peak = detectPPGPeak(_filteredPPG);

    // 计算HR和HRV
    calculateHRHRV(_peak);

    // 检测佩戴状态
    _isWear = detectWearStatus(_filteredPPG);
}

float CheezPPG::AverageFilter(float input) {
    float avgPPG; 
    // 平滑滤波
    _avgBuffer.push(input);
    if (_avgBuffer.isFull()) {
        avgPPG = 0;
        for (int i = 0; i < _avgBuffer.numItems(); i++) {
            avgPPG += _avgBuffer.peek(i);
        }
        avgPPG /= _avgBuffer.numItems();
    }
    return avgPPG;  
}

// 带通滤波器系数 
const float BP_B0 = 0.00122714f;  
const float BP_B1 = 0.00245428f;  
const float BP_B2 = 0.00122714f;  
const float BP_A1 = -1.8794700f;  
const float BP_A2 = 0.89155200f; 
float CheezPPG::bandpassFilter(float input) 
{
    static float x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;

    float output =  BP_B0 * input + 
                    BP_B1 * x1 + 
                    BP_B2 * x2 - 
                    BP_A1 * y1 - 
                    BP_A2 * y2;    
    x2 = x1;  
    x1 = input;  
    y2 = y1;  
    y1 = output;    
    return output;
}

bool CheezPPG::detectWearStatus(int ppgValue) 
{
    bool isWear = _isWear;
    if(_wearThreshold < 0)
    { 
        isWear = 1;
    }
    else
    {
        switch(isWear) 
        {
            case 0:  // 未佩戴状态  
            {  
                if (ppgValue > _wearThreshold)   
                    _wearCount++;   
                else  
                    _wearCount = 0;   
                
                if(_wearCount >= _stableCount) {   
                    _wearCount = 0;  
                    isWear = 1;   
                }  
            }  
            break;
            
            case 1:  // 已佩戴状态  
            {  
                if (ppgValue < _wearThreshold)  
                    _wearCount++;   
                else   
                    _wearCount = 0;   
                
                if(_wearCount >= _stableCount) {   
                    _wearCount = 0;  
                    isWear = 0;  
                }  
            }  
            break;   
        } 
    } 
    return isWear;  
}

#define PPG_BUFFER_LEN 18
bool CheezPPG::detectPPGPeak(float new_sample) 
{
    bool getPeak;
    // 数据平均值和标准差的缓冲区
    static float dataBuffer[PPG_BUFFER_LEN];
    static float meanBuffer[PPG_BUFFER_LEN];
    static float stdDevBuffer[PPG_BUFFER_LEN];
    static int data_index = 0;

    // 检查峰值
    if ((new_sample - meanBuffer[data_index]) > 
    (PPG_BUFFER_LEN / 2) * stdDevBuffer[data_index]) {
        dataBuffer[data_index] = new_sample + dataBuffer[data_index];
        getPeak = 1;
    } else {
        dataBuffer[data_index] = new_sample;
        getPeak = 0;
    }

    // 计算平均值
    float sum = 0.0, mean=0.0, stdDeviation = 0.0;
    for (int i = 0; i < PPG_BUFFER_LEN; ++i) {
        sum += dataBuffer[(data_index + i) % PPG_BUFFER_LEN];
    }
    mean = sum / PPG_BUFFER_LEN;

    // 计算标准差
    for (int i = 0; i < PPG_BUFFER_LEN; ++i) {
        stdDeviation += pow(dataBuffer[(i) % PPG_BUFFER_LEN] - mean, 2);
    }
    meanBuffer[data_index] = mean;      // 更新平均缓冲区
    stdDevBuffer[data_index] = sqrt(stdDeviation / PPG_BUFFER_LEN);  // 更新标准差缓冲区
    data_index = (data_index + 1) % PPG_BUFFER_LEN;
    return getPeak;
}

void CheezPPG::calculateHRHRV(bool getPeak) 
{
    static bool _ignoreReading = false;
    static bool _firstPulseDetected = false;
    static unsigned long FirstPulseTime = 0;
    static unsigned long SecondPulseTime = 0; 

    if (getPeak && _ignoreReading == false) 
    {
        if (_firstPulseDetected == false)  {
            FirstPulseTime = millis();
            _firstPulseDetected = true;
        } else {
            SecondPulseTime = millis();
            unsigned long PulseInterval = SecondPulseTime - FirstPulseTime; 
            _rrBuffer.push(PulseInterval);                 
            FirstPulseTime = SecondPulseTime;
        }
        _ignoreReading = true;
    }
    if (!getPeak) _ignoreReading = false;  

    if (_rrBuffer.isFull()) {
        // HR
        int rrAvg = 0;
        for (int i = 0; i < _rrBuffer.numItems(); i++) {
            rrAvg += _rrBuffer.peek(i);  // 间隔总和
        }
        rrAvg = rrAvg / _rrBuffer.numItems(); 
        _hr = (1000 * 60.0 / rrAvg);  // Heat Rate = 60 / RR(单位:BPM) 
        static int lastHr = 60;
        _hr = (lastHr * 0.9 + _hr * 0.1); // 平滑心率数据
        lastHr = _hr; // 更新上次心率

        // HRV(SDNN)
        unsigned long diffSum = 0;
        for (int i = 0; i < _rrBuffer.numItems(); i++) {
            diffSum += pow(_rrBuffer.peek(i) - rrAvg, 2);
        }
        diffSum = diffSum > 90000 ? 90000 : diffSum;

        _hrv = sqrt(diffSum / (_rrBuffer.numItems() - 1));
        _rrBuffer.pop(); 
    }
}
