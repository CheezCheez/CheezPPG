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

CheezPPG::CheezPPG(int inputPin, int sampleRate, unsigned char hr_min,
                   unsigned char hr_max)
    : _inputPin(inputPin),
      _sampleRate(sampleRate),
      _hr_min(hr_min),
      _hr_max(hr_max) {
  _moving_window_size = _sampleRate / 50;  // 设置移动平均窗口大小
  _smallest = _sampleRate * 60 / _hr_max;  // 计算最小可能的心跳间隔

  unsigned char i = 0;
  for (i = 20; i >= 1; i--) _peak_window_HP[(unsigned char)(i - 1)] = 0;
  for (i = 11; i >= 1; i--) _HR_buffer[(unsigned char)(i - 1)] = 0;
  for (i = 11; i >= 1; i--) _RR_buffer[(unsigned char)(i - 1)] = 0;
  for (i = 11; i >= 1; i--) _HRV_buffer[(unsigned char)(i - 1)] = 0;
}

/**
 * @brief 定时器
 *
 * @return true
 * @return false
 */
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

/**
 * @brief PPG处理主函数
 *
 */
void CheezPPG::ppgProcess(void) {
  _rawPPG = analogRead(_inputPin);
  _avgPPG = AverageFilter(_rawPPG);
  _filteredPPG = bandpassFilter(_avgPPG);
  statHRMAlgo(_filteredPPG);
}

/**
 * @brief 心率算法主函数
 *
 * @param ppgData PPG数据
 */
void CheezPPG::statHRMAlgo(unsigned long ppgData) {
  unsigned char i;
  _moving_window_HP += ppgData;

  if (_moving_window_count > _moving_window_size) {
    _moving_window_count = 0;

    updateWindow(_peak_window_HP, _moving_window_HP, (_moving_window_size + 1));

    _moving_window_HP = 0;
    ispeak = 0;
    _ispeakOnset = 0;
    _ispeakPeak = 0;

    // 峰值
    if ((_last_peak > _smallest) && (ispeak == 0)) {
      ispeak = 1;
      _ispeakOnset = 1;
      for (i = PEAK_WINDOW_HALF_SIZE; i >= 1; i--) {
        if (_peak_window_HP[PEAK_WINDOW_HALF_SIZE] <
            _peak_window_HP[(unsigned int)(PEAK_WINDOW_HALF_SIZE - i)]) {
          ispeak = 0;
          _ispeakOnset = 0;
        }
        if (_peak_window_HP[PEAK_WINDOW_HALF_SIZE] <
            _peak_window_HP[(unsigned int)(PEAK_WINDOW_HALF_SIZE + i)]) {
          ispeak = 0;
          _ispeakOnset = 0;
        }
      }

      if (ispeak == 1) {
        _last_peakValueLED1 = findMax(_peak_window_HP);
        _total_found_peak++;

        if (_total_found_peak > 10) {
          updateheart_rate(_HR_buffer, _last_peak);
          updateHRV(_RR_buffer, _last_peak);
        }
        _last_peak = 0;
        _found_peak++;
      }
    }

    // 谷值
    if ((_last_onset > _smallest) && (ispeak == 0)) {
      ispeak = 1;
      _ispeakPeak = 1;
      for (i = PEAK_WINDOW_HALF_SIZE; i >= 1; i--) {
        if (_peak_window_HP[PEAK_WINDOW_HALF_SIZE] >
            _peak_window_HP[(unsigned int)(PEAK_WINDOW_HALF_SIZE - i)]) {
          ispeak = 0;
          _ispeakPeak = 0;
        }
        if (_peak_window_HP[PEAK_WINDOW_HALF_SIZE] >
            _peak_window_HP[(unsigned int)(PEAK_WINDOW_HALF_SIZE + i)]) {
          ispeak = 0;
          _ispeakPeak = 0;
        }
      }

      if (ispeak == 1) {
        _last_onsetValueLED1 = findMin(_peak_window_HP);

        _total_found_peak++;
        _found_peak++;
        _last_onset = 0;
      }
    }

    if (_found_peak > 8) {
      _found_peak = 0;
      _hr_temp = chooseRate(_HR_buffer);
      _hrv_temp = chooseHRV(_HRV_buffer);

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

/**
 * @brief 更新移动平均窗口
 *
 * @param window 移动平均窗口数组
 * @param sum 当前累加值
 * @param n 当前累加值的样本数
 */
void CheezPPG::updateWindow(unsigned long *window, unsigned long sum,
                            unsigned char n) {
  unsigned char i;
  for (i = 20; i >= 1; i--) {
    window[i] = window[(unsigned char)(i - 1)];
  }

  if (n > 0) window[0] = (sum / n);  // 插入新的移动平均值
}

/**
 * @brief 选择心率值，去除最高和最低值后计算平均心率
 *
 * @param rate 心率数组
 * @return unsigned char  处理后的心率值
 */
unsigned char CheezPPG::chooseRate(unsigned char *rate) {
  unsigned char max_val, min_val, count;
  unsigned int total;

  max_val = rate[0];
  min_val = rate[0];
  total = 0;
  count = 0;

  for (unsigned char i = 0; i < 7; i++) {
    unsigned char current_rate = rate[i];
    if (current_rate > 0) {
      max_val = (current_rate > max_val) ? current_rate : max_val;
      min_val = (current_rate < min_val) ? current_rate : min_val;
      total += current_rate;
      count++;
    }
  }

  if (count >= 3) {
    total = ((total - max_val - min_val) + ((count - 2) / 2)) / (count - 2);
  } else if (count > 0) {
    total = (total + count / 2) / count;
  }

  return total;
}

/**
 * @brief 选择HRV(SDNN)值，去除最高和最低值后计算平均HRV
 *
 * @param rate  HRV数组
 * @return unsigned char  处理后的HRV(SDNN)值
 */
unsigned char CheezPPG::chooseHRV(unsigned char *rate) {
  unsigned char max_val, min_val, count;
  unsigned int total;

  max_val = rate[0];
  min_val = rate[0];
  total = 0;
  count = 0;

  for (unsigned char i = 0; i < 7; i++) {
    unsigned char current_rate = rate[i];

    if (current_rate > 0) {
      max_val = (current_rate > max_val) ? current_rate : max_val;
      min_val = (current_rate < min_val) ? current_rate : min_val;
      total += current_rate;
      count++;
    }
  }
  if (count >= 3) {
    total = ((total - max_val - min_val) + ((count - 2) / 2)) / (count - 2);
  } else if (count > 0) {
    total = (total + count / 2) / count;
  }
  return total;
}

/**
 * @brief 更新心率值数组
 *
 * @param rate 心率数组
 * @param last 距离上一个峰值的采样点数
 */
void CheezPPG::updateheart_rate(unsigned char *rate, unsigned int last) {
  unsigned char i;
  i = 60 * _sampleRate / last;

  if ((i > _hr_min) && (i < _hr_max)) {
    for (i = 11; i >= 1; i--) {
      rate[i] = rate[(unsigned char)(i - 1)];
    }
    rate[0] = 60 * _sampleRate / last;
  }
}

/**
 * @brief 更新HRV(SDNN)值数组
 *
 * @param rrBuffer RR间期数组
 * @param last 距离上一个峰值的采样点数
 */
void CheezPPG::updateHRV(float *rrBuffer, unsigned int last) {
  unsigned char i;
  for (i = 11; i >= 1; i--) {
    rrBuffer[i] = rrBuffer[(unsigned char)(i - 1)];
    _HRV_buffer[i] = _HRV_buffer[(unsigned char)(i - 1)];
  }
  rrBuffer[0] = _sampleRate * 1000 / last;  // ms

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

/**
 * @brief 在数组中间位置找到最大值
 *
 * @param X 输入数组
 * @return unsigned long 最大值
 */
unsigned long CheezPPG::findMax(unsigned long *X) {
  unsigned long max_val = X[(PEAK_WINDOW_HALF_SIZE - 2)];
  for (unsigned char i = (PEAK_WINDOW_HALF_SIZE - 1);
       i <= (PEAK_WINDOW_HALF_SIZE + 2); i++) {
    if (max_val < X[i]) max_val = X[i];
  }
  return max_val;
}

/**
 * @brief 在数组中间位置找到最小值
 *
 * @param X 输入数组
 * @return unsigned long 最小值
 */
unsigned long CheezPPG::findMin(unsigned long *X) {
  unsigned long min_val = X[(PEAK_WINDOW_HALF_SIZE - 2)];
  for (unsigned char i = (PEAK_WINDOW_HALF_SIZE + 2);
       i >= (PEAK_WINDOW_HALF_SIZE - 1); i--) {
    if (min_val > X[i]) min_val = X[i];
  }
  return min_val;
}

/**
 * @brief 平滑滤波函数，使用简单移动平均滤波
 *
 * @param input 输入数据
 * @return float 平滑滤波后的数据
 */
float CheezPPG::AverageFilter(float input) {
  float avgPPG = 0.0f;

  if (_avgCount < AVG_WINDOW_SIZE) {
    _avgSum += input;
    _avgBuffer[_avgIndex] = input;
    ++_avgCount;
  } else {
    _avgSum = _avgSum - _avgBuffer[_avgIndex] + input;
    _avgBuffer[_avgIndex] = input;
  }

  _avgIndex = (_avgIndex + 1) % AVG_WINDOW_SIZE;

  if (_avgCount == AVG_WINDOW_SIZE) {
    avgPPG = _avgSum / AVG_WINDOW_SIZE;
  }

  return avgPPG;
}

/**
 * @brief 带通滤波函数，使用二阶IIR带通滤波器
 *
 * @param input 输入数据
 * @return float 带通滤波后的数据
 */
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

/**
 * @brief 设置佩戴检测阈值
 *
 * @param Threshold 阈值
 */
void CheezPPG::setWearThreshold(int Threshold) { _wearThreshold = Threshold; }

/**
 * @brief 检测佩戴状态
 *
 */
void CheezPPG::detectWearStatus() {
  long onset_peak_diff = _last_peakValueLED1 - _last_onsetValueLED1;

  if (onset_peak_diff > _wearThreshold) {
    if (_wearCount < WEAR_COUNT_THRESHOLD) ++_wearCount;
  } else {
    _wearCount = 0;
  }
  _isWear = (_wearCount >= WEAR_COUNT_THRESHOLD);
}