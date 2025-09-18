#include "CheezPPG.h"

// 带通滤波器系数
const float BP_B0 = 0.00122714f;
const float BP_B1 = 0.00245428f;
const float BP_B2 = 0.00122714f;
const float BP_A1 = -1.8794700f;
const float BP_A2 = 0.89155200f;

CheezPPG::CheezPPG(int inputPin, int sampleRate)
    : _inputPin(inputPin), _sampleRate(sampleRate) {
  _moving_window_size = _sampleRate / 50;  // 设置移动平均窗口大小
  _smallest = _sampleRate * 60 / _hr_max;  // 计算最小可能的心跳间隔

  unsigned char i = 0;
  for (i = 20; i >= 1; i--) _peak_window_HP[(unsigned char)(i - 1)] = 0;
  for (i = 11; i >= 1; i--) _HR_buffer[(unsigned char)(i - 1)] = 0;
  for (i = 11; i >= 1; i--) _RR_buffer[(unsigned char)(i - 1)] = 0;
  for (i = 11; i >= 1; i--) _HRV_buffer[(unsigned char)(i - 1)] = 0;
}

void CheezPPG::setWearThreshold(int Threshold) { _wearThreshold = Threshold; }

bool CheezPPG::checkSampleInterval(void) {
  static unsigned long past_time;
  static long count_time;

  unsigned long current_time = micros();
  unsigned long interval_time = current_time - past_time;

  past_time = current_time;
  count_time -= interval_time;

  if (count_time < 0) {
    count_time += 1000000 / _sampleRate;
    return true;
  }
  return false;
}

void CheezPPG::ppgProcess(void) {
  _rawPPG = analogRead(_inputPin);
  _avgPPG = AverageFilter(_rawPPG);
  _filteredPPG = bandpassFilter(_avgPPG);
  statHRMAlgo(_filteredPPG);
}

void CheezPPG::statHRMAlgo(unsigned long ppgData) {
  unsigned char i;
  _moving_window_HP += ppgData;

  if (_moving_window_count > _moving_window_size) {
    _moving_window_count = 0;

    updateWindow(_peak_window_HP, _moving_window_HP, (_moving_window_size + 1));

    _moving_window_HP = 0;  // 重置移动平均累加器
    ispeak = 0;             // 重置峰值标志
    _ispeakOnset = 0;
    _ispeakPeak = 0;

    if ((_last_peak > _smallest) && (ispeak == 0)) {
      ispeak = 1;
      _ispeakOnset = 1;
      // 在21点缓冲区中寻找局部最大值（峰值）
      for (i = 10; i >= 1; i--) {
        // 检查中心点（第10个点）是否比左右各10个点都大
        if (_peak_window_HP[10] < _peak_window_HP[(unsigned int)(10 - i)]) {
          ispeak = 0;
          _ispeakOnset = 0;
        }
        if (_peak_window_HP[10] < _peak_window_HP[(unsigned int)(10 + i)]) {
          ispeak = 0;
          _ispeakOnset = 0;
        }
      }

      if (ispeak == 1) {
        // 找到局部最大值（峰值）
        _last_peakValueLED1 = findMax(_peak_window_HP);  // 获取峰值幅度
        _total_found_peak++;  // 增加总峰谷计数

        if (_total_found_peak > 10) {
          updateheart_rate(_HR_buffer, _last_peak);
          updateHRV(_RR_buffer, _last_peak);
        }
        _last_peak = 0;
        _found_peak++;
      }
    }

    // 如果距离上一个谷值的时间足够长且当前不是峰值
    if ((_last_onset > _smallest) && (ispeak == 0)) {
      // 在21点缓冲区中寻找局部最小值（谷值）
      ispeak = 1;
      _ispeakPeak = 1;
      for (i = 10; i >= 1; i--) {
        // 检查中心点是否比左右各10个点都小
        if (_peak_window_HP[10] > _peak_window_HP[(unsigned int)(10 - i)]) {
          ispeak = 0;
          _ispeakPeak = 0;
        }
        if (_peak_window_HP[10] > _peak_window_HP[(unsigned int)(10 + i)]) {
          ispeak = 0;
          _ispeakPeak = 0;
        }
      }

      // 找到局部最小值
      if (ispeak == 1) {
        _last_onsetValueLED1 = findMin(_peak_window_HP);
        _total_found_peak++;
        _found_peak++;    // 增加本次处理的峰谷计数
        _last_onset = 0;  // 重置谷值间隔计数器
      }
    }

    // 每找到4个新的峰谷后更新一次心率值
    if (_found_peak > 10) {
      _found_peak = 0;                    // 重置计数器
      _hr_temp = chooseRate(_HR_buffer);  // 从心率数组中选择有效心率值
      _hrv_temp = chooseHRV(_HRV_buffer);  // 从心率数组中选择有效心率值

      // 心率值有效性检查
      if ((_hr_temp > _hr_min) && (_hr_temp < _hr_max))
        heart_rate = _hr_temp;  // 更新最终心率值

      if ((_hrv_temp >= _hrv_min) && (_hrv_temp < _hrv_max))
        sdnn = _hrv_temp;  // 更新最终HRV(SDNN)值
    }

    detectWearStatus();
  }

  _moving_window_count++;
  _last_onset++;
  _last_peak++;
}

void CheezPPG::updateWindow(unsigned long *window, unsigned long sum,
                            unsigned char n) {
  unsigned char i;
  for (i = 20; i >= 1; i--) {
    window[i] = window[(unsigned char)(i - 1)];
  }

  if (n > 0) window[0] = (sum / n);  // 插入新的移动平均值
}

unsigned char CheezPPG::chooseRate(unsigned char *rate) {
  // 去除最高和最低值后计算平均心率
  unsigned char max_val, min_val, count;
  unsigned int total;

  max_val = rate[0];  // 初始化最大值
  min_val = rate[0];  // 初始化最小值
  total = 0;          // 初始化累加和
  count = 0;          // 初始化有效数据计数

  // 遍历心率数组
  for (unsigned char i = 0; i < 7; i++) {
    unsigned char current_rate = rate[i];

    if (current_rate > 0) {
      max_val = (current_rate > max_val) ? current_rate : max_val;
      min_val = (current_rate < min_val) ? current_rate : min_val;

      total += current_rate;  // 累加值
      count++;                // 计数增加
    }
  }

  // 计算平均值（去除最高和最低值）
  if (count >= 3) {
    total = ((total - max_val - min_val) + ((count - 2) / 2)) /
            (count - 2);  // 四舍五入
  } else if (count > 0) {
    total = (total + count / 2) / count;
  }

  return total;  // 返回处理后的心率值
}

unsigned char CheezPPG::chooseHRV(unsigned char *rate) {
  // 去除最高和最低值后计算平均HRV(SDNN)
  unsigned char max_val, min_val, count;
  unsigned int total;

  max_val = rate[0];  // 初始化最大值
  min_val = rate[0];  // 初始化最小值
  total = 0;          // 初始化累加和
  count = 0;          // 初始化有效数据计数

  // 遍历HRV数组
  for (unsigned char i = 0; i < 7; i++) {
    unsigned char current_rate = rate[i];

    if (current_rate > 0) {
      max_val = (current_rate > max_val) ? current_rate : max_val;
      min_val = (current_rate < min_val) ? current_rate : min_val;

      total += current_rate;  // 累加值
      count++;                // 计数增加
    }
  }

  // 计算平均值（去除最高和最低值）
  if (count >= 3) {
    total = ((total - max_val - min_val) + ((count - 2) / 2)) /
            (count - 2);  // 四舍五入
  } else if (count > 0) {
    total = (total + count / 2) / count;
  }

  return total;  // 返回处理后的HRV(SDNN)值
}

void CheezPPG::updateheart_rate(unsigned char *rate, unsigned int last) {
  unsigned char i;
  i = 60 * _sampleRate / last;  // 计算心率值（BPM）

  // 心率值有效性检查
  if ((i > _hr_min) && (i < _hr_max)) {
    for (i = 11; i >= 1; i--) {
      rate[i] = rate[(unsigned char)(i - 1)];
    }
    rate[0] = 60 * _sampleRate / last;
  }
}

void CheezPPG::updateHRV(float *rrBuffer, unsigned int last) {
  unsigned char i;
  for (i = 11; i >= 1; i--) {
    rrBuffer[i] = rrBuffer[(unsigned char)(i - 1)];
    _HRV_buffer[i] = _HRV_buffer[(unsigned char)(i - 1)];
  }
  rrBuffer[0] = _sampleRate * 1000 / last;  // ms

  // 计算SDNN
  float meanRR = 0;
  for (i = 0; i < 12; i++) {
    meanRR += rrBuffer[i];
  }
  meanRR /= 12;

  float sdnn_temp = 0;
  for (i = 0; i < 12; i++) {
    sdnn_temp += (rrBuffer[i] - meanRR) * (rrBuffer[i] - meanRR);
  }
  sdnn_temp = sqrt(sdnn_temp / 12);
  _HRV_buffer[0] = sdnn_temp;
}

unsigned long CheezPPG::findMax(unsigned long *X) {
  unsigned long max_val = X[8];
  for (unsigned char i = 9; i <= 12; i++) {
    if (max_val < X[i]) max_val = X[i];  // 更新最大值
  }
  return max_val;
}

unsigned long CheezPPG::findMin(unsigned long *X) {
  unsigned long min_val = X[8];
  for (unsigned char i = 12; i >= 9; i--) {
    if (min_val > X[i]) min_val = X[i];  // 更新最小值
  }
  return min_val;
}

float CheezPPG::AverageFilter(float input) {
  float avgPPG = 0.0f;

  // 如果缓冲尚未填满，直接累加；否则减去被覆盖的旧值再累加新值
  if (_avgCount < AVG_WINDOW_SIZE) {
    _avgSum += input;
    _avgBuffer[_avgIndex] = input;
    ++_avgCount;
  } else {
    // 覆盖位置上的旧值
    _avgSum = _avgSum - _avgBuffer[_avgIndex] + input;
    _avgBuffer[_avgIndex] = input;
  }

  // 前进写指针
  _avgIndex = (_avgIndex + 1) % AVG_WINDOW_SIZE;

  // 仅在缓冲已填满时返回完整窗口平均（与原 isFull 语义一致）
  if (_avgCount == AVG_WINDOW_SIZE) {
    avgPPG = _avgSum / AVG_WINDOW_SIZE;
  }

  return avgPPG;
}

float CheezPPG::bandpassFilter(float input) {
  static float x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;

  float output =
      BP_B0 * input + BP_B1 * x1 + BP_B2 * x2 - BP_A1 * y1 - BP_A2 * y2;
  x2 = x1;
  x1 = input;
  y2 = y1;
  y1 = output;
  return output;
}

void CheezPPG::detectWearStatus() {
  long onset_peak_diff = _last_peakValueLED1 - _last_onsetValueLED1;

  if (onset_peak_diff > _wearThreshold) {
    if (_wearCount < WEAR_COUNT_THRESHOLD) ++_wearCount;
  } else {
    _wearCount = 0;
  }
  _isWear = (_wearCount >= WEAR_COUNT_THRESHOLD);
}