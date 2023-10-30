#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/binary_info.h"

#define SPI_PORT spi0
#define READ_BIT 0x80

typedef struct mpu9250
{
  int16_t w_raw[3];
  int16_t a_raw[3];
  int16_t temperature;

  int PIN_CS;

} mpu9250;

void mpu9250_setup(mpu9250 *imu, int CS_PIN);
void mpu9250_read_raw(mpu9250 *imu);