#include "CheezPPG.h"
 
#define INPUT_PIN A0    // PPG引脚
#define SAMPLE_RATE 125 // 采样率

CheezPPG ppg(INPUT_PIN,SAMPLE_RATE);  

void setup() 
{
  Serial.begin(115200); 
  ppg.setWearThreshold(80); // 设置佩戴阈值，不需要佩戴检测可设置为-1
}

void loop() 
{   
  if (ppg.checkSampleInterval()) 
  {  
    ppg.ppgProcess();
    Serial.println(
      String((int)ppg.getRawPPG()) + "," + 
      String((int)ppg.getAvgPPG()) + "," + 
      String((int)ppg.getFilterPPG()) + "," +  
      String((int)ppg.getPpgPeak()) + "," + 
      String((int)ppg.getPpgHr())+ "," + 
      String((int)ppg.getPpgHrv()) 
    );
  }   
}

