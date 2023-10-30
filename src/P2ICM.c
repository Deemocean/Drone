#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "../lib/ICM_20948/src/ICM_20948.h"

#define PIN_MISO 4
#define PIN_CS 5
#define PIN_SCK 6
#define PIN_MOSI 7

#define SPI_PORT spi0
#define SPI_CLK 1000000


// These are the interface functions that you would define for your system. They can use either I2C or SPI,

ICM_20948_Status_e my_write_spi(uint8_t reg, uint8_t *data, uint32_t len, void *user);
ICM_20948_Status_e my_read_spi(uint8_t reg, uint8_t *buff, uint32_t len, void *user);

// You declare a "Serial Interface" (serif) type and give it the pointers to your interface functions
const ICM_20948_Serif_t mySerif = {
    my_write_spi, // write
    my_read_spi,  // read
    NULL,         // this pointer is passed into your functions when they are called.
};

// Now declare the structure that represents the ICM.
ICM_20948_Device_t myICM;

static inline void cs_select()
{
  asm volatile("nop \n nop \n nop");
  gpio_put(PIN_CS, 0); // Active low
  asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect()
{
  asm volatile("nop \n nop \n nop");
  gpio_put(PIN_CS, 1);
  asm volatile("nop \n nop \n nop");
}


ICM_20948_Status_e my_read_spi(uint8_t reg, uint8_t *buff, uint32_t len, void *user)
{
  // if (user == NULL)
  // {
  //   return ICM_20948_Stat_ParamErr;
  // }

  uint8_t sign_reg = ((reg & 0x7F) | 0x80);

  cs_select();


  spi_write_blocking(SPI_PORT, &sign_reg , 1);
  spi_read_blocking(SPI_PORT, 0, buff, len);


  cs_deselect();

  return ICM_20948_Stat_Ok;
}

ICM_20948_Status_e my_write_spi(uint8_t reg, uint8_t *data, uint32_t len, void *user)
{
  // if (user == NULL)
  // {
  //   return ICM_20948_Stat_ParamErr;
  // }

  uint8_t sign_reg = ((reg & 0x7F) | 0x00);
  cs_select();


  spi_write_blocking(SPI_PORT, &sign_reg, 1);
  spi_write_blocking(SPI_PORT, data, len);

  cs_deselect();


  return ICM_20948_Stat_Ok;
}


void printPaddedInt16b(int16_t val)
{
  if (val > 0)
  {
    printf(" ");
    if (val < 10000)
    {
      printf("0");
    }
    if (val < 1000)
    {
      printf("0");
    }
    if (val < 100)
    {
      printf("0");
    }
    if (val < 10)
    {
      printf("0");
    }
  }
  else
  {
    printf("-");
    if (abs(val) < 10000)
    {
      printf("0");
    }
    if (abs(val) < 1000)
    {
      printf("0");
    }
    if (abs(val) < 100)
    {
      printf("0");
    }
    if (abs(val) < 10)
    {
      printf("0");
    }
  }
  printf("%d", abs(val));
}

void printRawAGMT(ICM_20948_AGMT_t agmt)
{
  printf("RAW. Acc [ ");
  printPaddedInt16b(agmt.acc.axes.x);
  printf(", ");
  printPaddedInt16b(agmt.acc.axes.y);
  printf(", ");
  printPaddedInt16b(agmt.acc.axes.z);
  printf(" ], Gyr [ ");
  printPaddedInt16b(agmt.gyr.axes.x);
  printf(", ");
  printPaddedInt16b(agmt.gyr.axes.y);
  printf(", ");
  printPaddedInt16b(agmt.gyr.axes.z);
  printf(" ], Mag [ ");
  printPaddedInt16b(agmt.mag.axes.x);
  printf(", ");
  printPaddedInt16b(agmt.mag.axes.y);
  printf(", ");
  printPaddedInt16b(agmt.mag.axes.z);
  printf(" ], Tmp [ ");
  printPaddedInt16b(agmt.tmp.val);
  printf(" ]");
  printf("\n");
}

float getAccMG(int16_t raw, uint8_t fss)
{
  switch (fss)
  {
  case 0:
    return (((float)raw) / 16.384);
    break;
  case 1:
    return (((float)raw) / 8.192);
    break;
  case 2:
    return (((float)raw) / 4.096);
    break;
  case 3:
    return (((float)raw) / 2.048);
    break;
  default:
    return 0;
    break;
  }
}

float getGyrDPS(int16_t raw, uint8_t fss)
{
  switch (fss)
  {
  case 0:
    return (((float)raw) / 131);
    break;
  case 1:
    return (((float)raw) / 65.5);
    break;
  case 2:
    return (((float)raw) / 32.8);
    break;
  case 3:
    return (((float)raw) / 16.4);
    break;
  default:
    return 0;
    break;
  }
}

float getMagUT(int16_t raw)
{
  return (((float)raw) * 0.15);
}

float getTmpC(int16_t raw)
{
  return (((float)raw) / 333.87);
}

int main()
{
  stdio_init_all();

  printf("Reading raw data from registers via SPI...\n");

  // use SPI0 at 0.1MHz.
  spi_init(SPI_PORT, SPI_CLK);
  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

  // Chip select is active-low, so we'll initialise it to a driven-high state
  gpio_init(PIN_CS);
  gpio_set_dir(PIN_CS, GPIO_OUT);
  gpio_put(PIN_CS, 1);



  // Initialize myICM
  ICM_20948_init_struct(&myICM);

  // Link the serif
  ICM_20948_link_serif(&myICM, &mySerif);

  while (ICM_20948_check_id(&myICM) != ICM_20948_Stat_Ok)
  {
    printf("whoami does not match. Halting...\n");
    sleep_ms(1000);
  }

  ICM_20948_Status_e stat = ICM_20948_Stat_Err;
  uint8_t whoami = 0x00;
  while ((stat != ICM_20948_Stat_Ok) || (whoami != ICM_20948_WHOAMI))
  {
    whoami = 0x00;
    stat = ICM_20948_get_who_am_i(&myICM, &whoami);
    printf("whoami does not match (0x");
    printf("%x", whoami);
    printf("). Halting...");
    printf("\n");
    sleep_ms(500);
  }

  // // Here we are doing a SW reset to make sure the device starts in a known state
  ICM_20948_sw_reset(&myICM);
  sleep_ms(1000);

  // Set Gyro and Accelerometer to a particular sample mode
  ICM_20948_set_sample_mode(&myICM, (ICM_20948_InternalSensorID_bm)(ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), ICM_20948_Sample_Mode_Continuous); // optiona: ICM_20948_Sample_Mode_Continuous. ICM_20948_Sample_Mode_Cycled
  sleep_ms(1000);
  // Set full scale ranges for both acc and gyr
  ICM_20948_fss_t myfss;
  myfss.a = gpm2;   // (ICM_20948_ACCEL_CONFIG_FS_SEL_e)
  myfss.g = dps250; // (ICM_20948_GYRO_CONFIG_1_FS_SEL_e)
  ICM_20948_set_full_scale(&myICM, (ICM_20948_InternalSensorID_bm)(ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), myfss);
  sleep_ms(1000);
  // // Set up DLPF configuration
  ICM_20948_dlpcfg_t myDLPcfg;
  myDLPcfg.a = acc_d473bw_n499bw;
  myDLPcfg.g = gyr_d361bw4_n376bw5;
  ICM_20948_set_dlpf_cfg(&myICM, (ICM_20948_InternalSensorID_bm)(ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), myDLPcfg);
  sleep_ms(1000);
  // // Choose whether or not to use DLPF
  ICM_20948_enable_dlpf(&myICM, ICM_20948_Internal_Acc, false);
  ICM_20948_enable_dlpf(&myICM, ICM_20948_Internal_Gyr, false);

  // // Now wake the sensor up
  ICM_20948_sleep(&myICM, false);
  ICM_20948_low_power(&myICM, false);

  while (true)
  {

    sleep_ms(500);

    ICM_20948_AGMT_t agmt = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0}};

    if (ICM_20948_get_agmt(&myICM, &agmt) == ICM_20948_Stat_Ok)
    {
      //printf("wx: %f, wy: %f, wz: %f \n", getGyrDPS(agmt.gyr.axes.x, myfss.g), getGyrDPS(agmt.gyr.axes.y, myfss.g), getGyrDPS(agmt.gyr.axes.z, myfss.g));

      printRawAGMT(agmt);
    }
    else
    {
      printf("Uh oh");
    }


  }
}








