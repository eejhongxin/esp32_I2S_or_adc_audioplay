#include "set_I2S.h"


set_I2S::set_I2S(i2s_port_t i2s_num) : i2s_num_(i2s_num) {}

bool set_I2S::InitInput(i2s_mode_t mode,
                      int bckPin,
                      int wsPin,
                      int dataInPin,
                      int dataOutPin
                       )
{
  i2s_config_t i2s_config = {
    .mode = mode,
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 1024,
    .use_apll = 0,
    .tx_desc_auto_clear = true,
    .fixed_mclk = -1
  };

  i2s_pin_config_t pin_config;
  memset(&pin_config,0,sizeof(i2s_pin_config_t));
  pin_config.bck_io_num = bckPin;
  pin_config.ws_io_num = wsPin;
  pin_config.data_in_num = dataInPin;
  pin_config.data_out_num = dataOutPin;


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

size_t set_I2S::Read(unsigned char* data, int numData)
{
  size_t recvSize;
  i2s_read(i2s_num_, data, numData, &recvSize, portMAX_DELAY);
  return recvSize;
}

size_t set_I2S::Write(const unsigned char* data, int numData) {
    size_t sendSize;
    i2s_write(i2s_num_, (const void*)data, numData, &sendSize, portMAX_DELAY);
    return sendSize;
}

void set_I2S::End()
{
  i2s_driver_uninstall(i2s_num_);
}