# CheezPPG 腕部脉搏传感器

## 简介
 
启思电子推出腕带心率传感器 Cheez.PPG，采用光电容积脉搏波检测(PhotoPlethysmoGraphy,PPG)，通过测量血液中血红蛋白随心脏跳动而对氧气吸收的变化量来测量人体心率参数。

Cheez.PPG传感器通常采集的是绿光信号，具有较好的稳定性、信号强度。该方法拥有性能稳定，适应性强等特点。模块拥有方波和脉搏波两种信号输出模式，分别用引脚引出输出信号。脉搏波将输出一个连续的心率波形，而方波将根据心率的变化输出对应的方波。
可以佩戴于手指、手腕、手臂等皮肤裸露处。另外，板载的XH2.54接口也可直接兼容Arduino EX.UNO扩展板及EX.STM32开发板，使用简单，平台兼容性好。

### 注意： 
1. **此为静态检测传感器，测量时请勿随意移动，也勿按压过紧。**
2. **本品并非专业医疗仪器，不能作为辅助配件参与诊断和治疗。**

## 产品参数 
- 输入电压：5V-DC
- 输出电压：模拟量(0 - 3.3V)、 数字量(0 / 3.3V)
- 工作电流：< 10mA
- 模块接口：XH2.54-4P杜邦线  
- 尺寸：26mm x 26.4mm 
<br/>

## 引脚说明
<br/>

![image](./img/cheezPPG_1.png)

## 使用教程

### 安装方式

* 模块可以安放在 手腕，手臂等皮肤直接接触处： 
<br/>

![image](./img/cheezPPG_2.png) 

### 接线图  
<br/>

![image](./img/cheezPPG_3.png) 

### 教程

#### 准备

- **软件** 
  * Arduino IDE 2.3
  * 安装Arduino库：管理中安装CheezPPG库 
  
  
- **硬件**
  - Arduino UNO开发板 x1
  - EX.UNO 扩展板x1（可选）
  - CheezPPG脉搏传感器 x 1 
  - XH2.54端子 转 4Pin杜邦线 x1 

- **引脚连接**  
  
|单片机引脚|传感器引脚|
|--|--|
|D2| S2(黄色线)|
|A0| S1(白色线)|
|5V| V(红色线)|
|GND| G(黑色线)|

#### 样例代码

```c_cpp
/* 
串口输出:
        波特率 115200
        采样率：125Hz 

串口输出内容(ASCII码)：
        原始数据,平滑滤波数据,带通滤波数据,心跳检测数据,心率数据,HRV(SDNN)数据
*/
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
      String((int)ppg.getRawPPG())    + "," +  // 原始数据
      String((int)ppg.getAvgPPG())    + "," +  // 平滑滤波数据
      String((int)ppg.getFilterPPG()) + "," +  // 带通滤波数据
      String((int)ppg.getPpgPeak())   + "," +  // 心跳检测数据
      String((int)ppg.getPpgHr())     + "," +  // 心率数据
      String((int)ppg.getPpgHrv())             // HRV(SDNN)数据
    );
  }   
}
```

#### 实验结果

可通过ArduinoIDE上的 " 串口绘图仪 "查看

<br/>
