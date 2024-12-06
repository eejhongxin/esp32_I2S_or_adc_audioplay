
//------------------------------------------------------------------------------------------------------------------------
// Includes
    #include "driver/i2s.h"                       // Library of I2S routines, comes with ESP32 standard install
    #include "set_I2S.h"
    #include "set_I2S_adc.h"
//------------------------------------------------------------------------------------------------------------------------
  //扬声器
  #define speaker_bck 12
  #define speaker_ws 13
  #define speaker_data 14
  //麦克风
  #define micro_bck 21
  #define micro_ws 22
  #define micro_data 32

  #define micro_channel ADC1_CHANNEL_4
//------------------------------------------------------------------------------------------------------------------------
    //扬声器I2S模式配置
    static const i2s_port_t speaker_i2s_num = I2S_NUM_0;  // I2S通道0
    static const i2s_mode_t speaker_mode =(i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX); //发送模式

    //麦克风I2S配置
    static const i2s_port_t micro_i2s_num = I2S_NUM_1;
    static const i2s_mode_t micro_mode=(i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);

    
//------------------------------------------------------------------------------------------------------------------------
// I2S configuration structures
  set_I2S speaker_I2S(speaker_i2s_num);
  set_I2S micro_I2S(micro_i2s_num);
  set_I2S_adc micro_I2S_adc;

//------------------------------------------------------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
 if (!speaker_I2S.InitInput(speaker_mode,speaker_bck, speaker_ws, I2S_PIN_NO_CHANGE,speaker_data))  // GPIO 35
  {
    Serial.println("init speaker_i2s error");
    return;
  }
 if (!micro_I2S.InitInput(micro_mode,micro_bck, micro_ws, micro_data,I2S_PIN_NO_CHANGE))  // GPIO 35
  {
    Serial.println("init micro_i2s error");
    return;
  }
  if (!micro_I2S_adc.InitAdcInput(micro_channel))  // GPIO 35
  {
    Serial.println("init micro_i2s_adc error");
    return;
  }
  //  i2s_set_sample_rates(speaker_i2s_num, WavHeader.SampleRate);
}

void loop() {
    size_t bytes_read;
    int16_t data[256]; // 数据缓冲区

    // 从麦克风读取数据
    i2s_read(I2S_NUM_1, &data, sizeof(data), &bytes_read, portMAX_DELAY);

    // // 打印读取的数据
    // Serial.print("Bytes read: ");
    // Serial.println(bytes_read);

    // for (size_t i = 0; i < bytes_read / sizeof(int16_t); i++) {
    //     // Serial.print("Data[");
    //     // Serial.print(i);
    //     // Serial.print("]: ");
    //     Serial.println(data[i]);
    // }

    // 将数据写入扬声器
    i2s_write(I2S_NUM_0, &data, sizeof(data), &bytes_read, portMAX_DELAY);
}




