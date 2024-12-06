#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#define SAMPLE_RATE (44100)

class set_I2S {
public:
	set_I2S(i2s_port_t i2s_num);  // 构造函数，接受 i2s_num 作为参数
	bool InitInput(i2s_mode_t mode, int bckPin, int wsPin, int dataInPin, int dataOutPin);
	size_t Read(unsigned char* data, int numData);
	size_t Write(const unsigned char* data, int numData);
	void End();

private:
	i2s_port_t i2s_num_;  // i2s_num 作为成员变量
};