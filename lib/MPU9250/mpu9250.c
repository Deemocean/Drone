#include "mpu9250.h"
#define SPI_PORT spi0
#define READ_BIT 0x80

static inline void cs_select(mpu9250 *imu)
{
  asm volatile("nop \n nop \n nop");
  gpio_put(imu->PIN_CS, 0); // Active low
  asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect(mpu9250 *imu)
{
  asm volatile("nop \n nop \n nop");
  gpio_put(imu->PIN_CS, 1);
  asm volatile("nop \n nop \n nop");
}

void mpu9250_setup(mpu9250 *imu, int CS_PIN)
{
  // Two byte reset. First byte register, second byte data
  // There are a load more options to set up the device in different ways that could be added here
  //assign CS Pin
  imu->PIN_CS = CS_PIN;

  uint8_t buf[] = {0x6B, 0x00};
  cs_select(imu);
  spi_write_blocking(SPI_PORT, buf, 2);
  cs_deselect(imu);
}

static void read_registers(mpu9250 *imu, uint8_t reg, uint8_t *buf, uint16_t len)
{
  // For this particular device, we send the device the register we want to read
  // first, then subsequently read from the device. The register is auto incrementing
  // so we don't need to keep sending the register we want, just the first.

  reg |= READ_BIT;
  cs_select(imu);
  spi_write_blocking(SPI_PORT, &reg, 1);
  sleep_us(10);

  spi_read_blocking(SPI_PORT, 0, buf, len);
  cs_deselect(imu);
  sleep_us(10);
}

void mpu9250_read_raw(mpu9250 *imu)
{
  uint8_t buffer[6];
  // Start reading acceleration registers from register 0x3B for 6 bytes
  read_registers(imu, 0x3B, buffer, 6);

  for (int i = 0; i < 3; i++)
  {
    imu->a_raw[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
  }

  // Now gyro data from reg 0x43 for 6 bytes
  read_registers(imu, 0x43, buffer, 6);

  for (int i = 0; i < 3; i++)
  {
    imu->w_raw[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);

  }

  // Now temperature from reg 0x41 for 2 bytes
  read_registers(imu, 0x41, buffer, 2);

  imu->temperature = buffer[0] << 8 | buffer[1];
}