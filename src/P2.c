#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/binary_info.h"
#include "../lib/MPU9250/mpu9250.h"

#define PIN_MISO 4
#define PIN_CS 5
#define PIN_SCK 6
#define PIN_MOSI 7
#define CLK_FREQ 100*1000

static mpu9250 imu;

int main()
{
  stdio_init_all();


  spi_init(SPI_PORT, CLK_FREQ);
  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
  // Make the SPI pins available to picotool
  //bi_decl(bi_3pins_with_func(PIN_MISO, PIN_MOSI, PIN_SCK, GPIO_FUNC_SPI));

  // Chip select is active-low, so we'll initialise it to a driven-high state
  gpio_init(PIN_CS);
  gpio_set_dir(PIN_CS, GPIO_OUT);
  gpio_put(PIN_CS, 1);
  // Make the CS pin available to picotool
  //bi_decl(bi_1pin_with_name(PIN_CS, "SPI CS"));

  mpu9250_setup(&imu,PIN_CS);

  // See if SPI is working - interrograte the device for its I2C ID number, should be 0x71
  // uint8_t id;
  // read_registers(&imu, 0x75, &id, 1);
  // printf("I2C address is 0x%x\n", id);


  while (1)
  {
    mpu9250_read_raw(&imu);

    // These are the raw numbers from the chip, so will need tweaking to be really useful.
    // See the datasheet for more information
    printf("Acc. X = %d, Y = %d, Z = %d \n", imu.a_raw[0], imu.a_raw[1], imu.a_raw[2]);
    printf("Gyro. X = %d, Y = %d, Z = %d \n", imu.w_raw[0], imu.w_raw[1], imu.w_raw[2]);
    // Temperature is simple so use the datasheet calculation to get deg C.
    // Note this is chip temperature.
    //printf("Temp. = %f\n", (imu.temperature / 340.0) + 36.53);


    sleep_ms(10);
  }
}