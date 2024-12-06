---
title: ESP32使用I2S、ADC采样麦克风进行音频采集与播放
date: 2024-11-22 20:33:29
mathjax: true
categories:
- ESP32
tags:
- ADC采样
- I2S
- 音频流采集
---

<!-- toc -->

# **协议介绍**

## I2S总线规范

### 三种信号线

1. SCK串行时钟：

   对应的数字音频的每一位音频数据，SCK都有1个脉冲，SCK的频率=2×采样频率×采样位数。

2. WS字段（声道选择）：

   用于切换左右声道的数据。WS的频率=采样频率

   命令选择线表明了正在被传输的声道。

   WS为“1”表示正在传输的是左声道的数据。
   WS为“0”表示正在传输的是右声道的数据。
   WS可以在串行时钟的上升沿或者下降沿发生改变，并且WS信号不需要一定是对称的。在从属装置端，WS在时钟信号的上升沿发生改变。WS总是在最高位传输前的一个时钟周期发生改变，这样可以使从属装置得到与被传输的串行数据同步的时间，并且使接收端存储当前的命令以及为下次的命令清除空间。

3. SD串行数据：

   这个其实就是我们常说的数据端口了。

### 工作模式

I2S工作模式可以是主模式（Master Mode）或从模式（Slave Mode）。两者唯一的区别是：提供时钟信号（SCK）和帧同步信号（LRCK）的主题不同。如下图所示，一共存在三种工作模式，分别是：

 1.发射器（transmitter）为Master，接收器（receiver）为Slave，此时由发射器提供SCK和LRCK

2.接收器（receiver）为Master，发射器（transmitter）为Slave，此时由接收器提供SCK和LRCK

3.发射器（transmitter）和接收器（receiver）均为Slave，由系统中其他模块提供SCK和LRCK

![image-20241126192005435](https://blog-images-1325348240.cos.ap-nanjing.myqcloud.com/undefinedimage-20241126192005435.png)

## ADC（Analog-to-Digital Converter）通信协议规范

adc采样大家应该都不陌生，就是模拟信号转换成数字信号的转换器，这个用途在于采集单片机ADC引脚信号，因为本质上我们的音频信号就是由一个个电压信号组成的，如果采集到了相应的ADC引脚信号，那么我们也就可以相应的复现出音频信号。

### 常规ADC信号采样在音频领域存在的瑕疵

但是如果大家尝试直接用analogread（我这里以arduino里面读取电压的方式为例）或者更为底层的去读取引脚的电压值，如果直接使用DAC引脚输出读取到的电压值时会发现声音非常卡完全不像一个音频信号。以我下面这段代码为例。

```
void setup() {
  Serial.begin(115200);
}

// the loop routine runs over and over again forever:
void loop() {
  float startT=micros();
  float sensorValue = analogRead(A0);
  float voltage = sensorValue * (5.0 / 1023.0);
  float endT=micros();
  Serial.printf("voltage:%f,time:%f\n",sensorValue,endT-startT);
}
```

这就是很简单的一段读取引脚电压的程序，最后的输出结果如下，

```
voltage:0.738025,time:128.000000
voltage:0.943304,time:128.000000
voltage:0.000000,time:144.000000
voltage:0.000000,time:136.000000
voltage:0.000000,time:120.000000
```

两个读取之间的间隔基本都在100us左右，那么它的更新频率就是1000HZ左右
$$
\frac{1}{100*10^{-6}}= 1000HZ
$$
但是我们正常的音频信号频率最低也都在8000HZ朝上，常见的都是16000HZ和44100HZ明显是远远满足不了需求的这就是为什么音频会听起来不正常的原因。

（当然也可以不适用Arduino自带的读取电压函数，调用更底层的ADC采样方法使用adc1_get_raw直接进行采样，这里我就不详细举例了，就信号的更新频率来说虽然快了一点，但是仍然满足不了音频信号的更新需求）

## 采用I2S传输协议优化ADC采样

解决方案就是采用I2S传输协议去优化ADC采样。

之前的**常规ADC**采样的流程是：

ADC先进行采样，然后将采样后的数据存储到**RAM**中，然后CPU从RAM中提取数据，最后对数据进行处理从而得到相应的电压数据，在这个过程中ADC采样和CPU从RAM中获取数据这两个部分是**不能同时进行**的，

![image-20241126202646688](https://blog-images-1325348240.cos.ap-nanjing.myqcloud.com/undefinedimage-20241126202646688.png)

但是采用I2S传输协议优化ADC采样的流程是：

首先通过I2S协议采集麦克风的数据，然后将这份数据存储到DMA存储器中，通过DMA临时存储器来和RAM进行数据传递，这样就避免了RAM堵塞时ADC采样无法进行的问题，从而大大提高了ADC采样的效率

> 这个DMA存储器可以理解成为一个临时存储站用来存储麦克风采集的数据（如果想要具体介绍，esp-idf官网有较为详细的介绍[堆内存分配 - ESP32 - — ES	P-IDF 编程指南 v5.2.3 文档](https://docs.espressif.com/projects/esp-idf/zh_CN/stable/esp32/api-reference/system/mem_alloc.html)）

![image-20241126203151787](https://blog-images-1325348240.cos.ap-nanjing.myqcloud.com/undefinedimage-20241126203151787.png)

# 系统搭建

## 硬件部分

整个系统由esp32-wroom32作为主控芯片，麦克风用的是inmp441（i2s麦克风）和max9814（adc采样麦克风）两种，音频增益芯片用的是max98357（I2S输出）和8002A功放芯片（DAC输出），喇叭市面上随便挑一个就好了

![image-20241206204650961](https://blog-images-1325348240.cos.ap-nanjing.myqcloud.com/undefinedimage-20241206204650961.png)

引脚接线如下：

ADC采样的引脚比较简单我就不说了哈哈哈哈

**INMP441引脚**

VDD——3.3V

GND，L/R——GND

SD——GPIO32

WS——GPIO22

SCK——GPIO21

**MAX98357引脚**

VIN——3.3V

GND——GND

DIN——GPIO14

BCLK——GPIO12

LRC——GPIO13

****

## 软件部分

### I2S协议配置

i2s的参数配置

```C
i2s_config_t i2s_config = {
    .mode = mode,//设置I2S的模式，为主机或者从机模式，发送或者接收模式
    .sample_rate = SAMPLE_RATE,//采样率设置
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,//采样的比特数
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,//I2S的通道设置
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,//存储空间数量
    .dma_buf_len = 1024,//存储空间长度
    .use_apll = 0,
    .tx_desc_auto_clear = true,
    .fixed_mclk = -1
  };
```

i2s的引脚配置

```C
  i2s_pin_config_t pin_config;
  memset(&pin_config,0,sizeof(i2s_pin_config_t));
  pin_config.bck_io_num = bckPin;//时钟引脚
  pin_config.ws_io_num = wsPin;//声道设置
  pin_config.data_in_num = dataInPin;//数据输入引脚（麦克风设置引脚）
  pin_config.data_out_num = dataOutPin;//数据输出引脚（扬声器设置引脚）
```

i2s的引脚和参数驱动配置

```C
  if(ESP_OK!=i2s_driver_install(i2s_num_, &i2s_config, 0, NULL))
  {
    Serial.println("install i2s driver failed");
    return false;
  }
  if(ESP_OK!=i2s_set_pin(i2s_num_, &pin_config))
  {
    Serial.println("i2s set pin failed");
    return false;
  }
  return true;
}
```

### ADC模式下的I2S配置

i2s参数配置

```c
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),//跟之前代码主要不同的就是要设置ADC的模式
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = BPS,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 16,
    .dma_buf_len = 128,
    .use_apll = false
  };
```

i2s驱动配置

```C
if(ESP_OK!=i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL))
  {
    Serial.println("install i2s driver failed");
    return false;
  }
 i2s_set_pin(I2S_NUM_0, NULL);
```

ADC配置

这里需要注意，如果你的项目中有WI-FIi功能的话建议不要使用ADC2的采样通道，因为ADC2采样通道和芯片中的WI-FI冲突了，

![image-20241206203219502](https://blog-images-1325348240.cos.ap-nanjing.myqcloud.com/undefinedimage-20241206203219502.png)具体可以参考esp32的官方文档[esp32_datasheet_cn.pdf](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_cn.pdf)

```C
  i2s_set_adc_mode(ADC_UNIT_1, adcChannel);//设置ADC采样的通道为1，
  adc1_config_channel_atten(adcChannel, ADC_ATTEN_DB_11);
  i2s_adc_enable(I2S_NUM_0);
```

主程序这里太多了就不放出来了参考[eejhongxin/esp32_I2S_or_adc_audioplay](https://github.com/eejhongxin/esp32_I2S_or_adc_audioplay)

>
>参考资料：
>
>[32 ESP32之使用I2S实现录音功能 （INMP411,MAX4466介绍）- 基于Arduino_哔哩哔哩_bilibili](https://www.bilibili.com/video/BV1xA411Q76y/?spm_id_from=333.337.search-card.all.click&vd_source=3cb0bf4d59cb1d9d4ad468ab211ce85f)
>
>https://www.youtube.com/watch?v=pPh3_ciEmzs

