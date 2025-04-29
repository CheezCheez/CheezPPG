#include "CheezPPG.h"  
#include <math.h>  

CheezPPG::CheezPPG(uint8_t ppgPin, unsigned long sampleRate) : 
    _ppgPin(ppgPin),   
    _sampleRate(sampleRate),    
    _stableCount(180),  
    _ignoreReading(false),  
    _firstPulseDetected(false)
{   
    for(int i = 0; i < PPG_BUFFER_LEN; i++) {  
        dataBuffer[i] = 0;  
        meanBuffer[i] = 0;  
        stdDevBuffer[i] = 0;  
    }  
     
    rrBuffer.init();  
    avgBuffer.init();  
    setWearThreshold();
}  

void CheezPPG::setWearThreshold(int wearThreshold)
{
  if(wearThreshold<0)
    enableWearCheck = 0;
  else 
  { 
    enableWearCheck = 1;
    _wearThreshold =  wearThreshold;
  }
}

bool CheezPPG::checkSampleInterval(void)   
{  
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
   
float CheezPPG::AverageFilter(float input)   
{  
    float avgValue = 0;  
    avgBuffer.push(input);  

    if (avgBuffer.numItems() > 0) {  
        float sum = 0;  
        for (int i = 0; i < avgBuffer.numItems(); i++) {  
            float value;  
            avgBuffer.peek(i, value);  
            sum += value;  
        }  
        avgValue = sum / avgBuffer.numItems();  
    }  
    
    _avgPPG = avgValue;  
    return avgValue;  
}  
   
float CheezPPG::bandpassFilter(float input)   
{    
    float output =  BP_B0 * input + 
                    BP_B1 * x1 + 
                    BP_B2 * x2 - 
                    BP_A1 * y1 - 
                    BP_A2 * y2;   
   
    x2 = x1;  
    x1 = input;  
    y2 = y1;  
    y1 = output;   
    _filteredPPG = output;
    return output;  
}   

uint8_t CheezPPG::detectWearStatus(int ppgValue)   
{  
    switch(_isWear)   
    {  
        case 0:  // 未佩戴状态  
        {  
            if (ppgValue > _wearThreshold)   
                 _wearCount++;   
            else  
                _wearCount = 0;   
            
            if(_wearCount >= _stableCount)  
            {   
                _wearCount = 0;  
                _isWear = 1;   
            }  
        }  
        break;  
        
        case 1:  // 已佩戴状态  
        {  
            if (ppgValue < _wearThreshold)  
                _wearCount++;   
            else   
                _wearCount = 0;   
            
            if(_wearCount >= _stableCount)  
            {   
                _wearCount = 0;  
                _isWear = 0;  
            }  
        }  
        break;   
    }  
    return _isWear;  
}  

uint8_t CheezPPG::detectPPGPeak(float newSample)  
{  	   
    // 检查峰值  
    if ((newSample - meanBuffer[dataIndex]) >   
        (PPG_BUFFER_LEN / 2.0f) * stdDevBuffer[dataIndex])  
    {  
        dataBuffer[dataIndex] = newSample + dataBuffer[dataIndex];  
        _peak = 1;  
    }  
    else  
    {  
        dataBuffer[dataIndex] = newSample;  
        _peak = 0;  
    }   

    // 计算平均值  
    float sum = 0.0, mean, standardDeviation = 0.0;  
    for (int i = 0; i < PPG_BUFFER_LEN; ++i)  
    {  
        sum += dataBuffer[(dataIndex + i) % PPG_BUFFER_LEN];  
    }  
    mean = sum / PPG_BUFFER_LEN;  

    // 计算标准差  
    for (int i = 0; i < PPG_BUFFER_LEN; ++i)  
    {  
        standardDeviation += pow(dataBuffer[i] - mean, 2);  
    }  
    
    // 更新平均值和标准差缓冲区  
    meanBuffer[dataIndex] = mean;  
    stdDevBuffer[dataIndex] = sqrt(standardDeviation / PPG_BUFFER_LEN);  

    // 更新索引  
    dataIndex = (dataIndex + 1) % PPG_BUFFER_LEN;  
   
    return _peak;  
}  

void CheezPPG::calculateHRHRV(void)  
{   
    if (_peak && !_ignoreReading)   
    {  
        if (!_firstPulseDetected)   
        {   
            _firstPulseTime = millis();  
            _firstPulseDetected = true;  
        }   
        else  
        {  
            _secondPulseTime = millis();  
            unsigned long pulseInterval = _secondPulseTime - _firstPulseTime;  
            rrBuffer.push(pulseInterval);   
            _firstPulseTime = _secondPulseTime;  
        }  
        _ignoreReading = true;  
    }   
    _ignoreReading = _peak ? _ignoreReading : false;  
   
    if (rrBuffer.isFull())   
    {  
        // 计算平均RR间隔  
        float rrAvg = 0;  
        int itemCount = rrBuffer.numItems();  
        for (int i = 0; i < itemCount; i++)   
        {  
            int rrValue;  
            rrBuffer.peek(i, rrValue);  
            rrAvg += rrValue;  
        }  
        rrAvg /= itemCount;  

        // 计算心率  
        float hr = (1000 * 60.0 / rrAvg);  
        hr = (hr > 160) ? 160 : ((hr < 50) ? 50 : hr);  

        // 计算HRV (SDNN)  
        float diffSum = 0;  
        for (int i = 0; i < itemCount; i++)  
        {  
            int rrValue;  
            rrBuffer.peek(i, rrValue);  
            float diff = rrValue - rrAvg;  
            diffSum += diff * diff;  
        }  
        diffSum = (diffSum > 90000) ? 90000 : diffSum;  
        float hrv = sqrt(diffSum / (itemCount - 1));  

        // 从环形缓冲区移除最旧的元素  
        int dummy;  
        rrBuffer.pop(dummy);  

        // 更新滤波器的HR和HRV  
        _rrAvg = rrAvg;   
        _hr = hr;  
        _hrv = hrv;   
    }  
}  

void CheezPPG::ppgProcess()  
{   
  _rawPPG = analogRead(_ppgPin);  
  float averageFiltered = AverageFilter(_rawPPG);     
  float bandpassFiltered = bandpassFilter(averageFiltered);  

  uint8_t ppgPeak = detectPPGPeak(bandpassFiltered);   
  uint8_t ppgState = detectWearStatus(bandpassFiltered);   
  calculateHRHRV();  

  if(!ppgState && enableWearCheck)  
  {  
      _rrAvg = 0;  
      _hr = 0;  
      _hrv = 0;   
      _peak = 0;  
      _filteredPPG = 0;  
  }  
}  