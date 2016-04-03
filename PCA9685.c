#include <stdio.h>
#include <errno.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <time.h>
#include <logging.h>
#include <PCA9685.h>

PCA9685InfoStruct 	PCA9685Info[PCA9685_MAXDEVICES];  // list of PCA9685 devices
unsigned 		PCA9685InfoLen = 0;


// Get PCA9685 info from table
PCA9685InfoStruct *PCA9685GetInfo(int handle)
{
  return &PCA9685Info[handle];
}


// Software reset of the PCA9685
int PCA9685Reset(int handle)
{
  log_function("PCA9685Reset(handle: %d)...\n", handle);

  int hPCA9685 = PCA9685GetInfo(handle)->hDevice;
  int status;

  if((status = wiringPiI2CWriteReg8(hPCA9685, PWM_MODE1, PWM_SLEEP)) != 0)
  {
    log_error("ERROR.\n");
    return -1;
  }

  log_function("PCA9685Reset Done\n");
  return 0;
}


// Set the frequency (PWM_PRESCALE) and restart
int PCA9685SetFrequency(int handle, unsigned frequency)
{
  log_function("PCA9685SetFrequency(handle: %d, frequency: %d)...\n", handle, frequency);

  PCA9685InfoStruct *device;
  int hPCA9685;
  double rawPrescale;
  unsigned prescale;
  unsigned status;
  unsigned oldMode1;
  unsigned mode1;
  int waiting;

  device = PCA9685GetInfo(handle);
  hPCA9685 = device->hDevice;
  device->frequency = frequency;

  // Set PWM frequency
  rawPrescale = 25000000.0;                 // 25Hhz
  rawPrescale /= 4096.0;                    // 12-bit
  rawPrescale /= (double)frequency;         // desired frequency
  prescale = (int)((rawPrescale += 0.5) - 1);
  log_message(LOG_NORMAL, "  Raw Prescale: %f  Prescale: %d\n", rawPrescale, prescale);

  oldMode1 = wiringPiI2CReadReg8(hPCA9685, PWM_MODE1);
  log_message(LOG_NORMAL, "  Read PWM_MODE1: %02X\n", oldMode1);

  mode1 = (oldMode1 & 0x6F);  // unset RESTART and SLEEP

  // Tell device to sleep
  log_message(LOG_NORMAL, "  Set PWM_MODE1 to PWM_SLEEP: %02X\n", mode1 | PWM_SLEEP);
  if((status = wiringPiI2CWriteReg8(hPCA9685, PWM_MODE1, mode1 | PWM_SLEEP)) != 0)
  {
    log_error("  ERROR\n");
    return -1;
  }

  // Set PRESCALE
  log_message(LOG_NORMAL, "  Set PWM_PRESCALE: %02X\n", prescale);
  if((status = wiringPiI2CWriteReg8(hPCA9685, PWM_PRESCALE, prescale)) != 0)
  {
    log_error("  ERROR\n");
    return -1;
  }

  delay(1);  // wait a bit in case PWM cycle was mid-stream (some PWM channels were running)

  // clear sleep flag
  log_message(LOG_NORMAL, "  Clear sleep flag: %02X\n", mode1);
  if((status = wiringPiI2CWriteReg8(hPCA9685, PWM_MODE1, mode1)) != 0)
  {
    log_error("  ERROR\n");
    return -1;
  }

  delay(1);  // Must wait at least 500uS for oscillator to wake up

  // set reset flag (which actually finishes the reset)
  log_message(LOG_NORMAL, "  Set reset HIGH on PWM_MODE1: %02X\n", mode1 | PWM_RESTART);
  if((status = wiringPiI2CWriteReg8(hPCA9685, PWM_MODE1, mode1 | PWM_RESTART)) != 0)
  {
    log_error("  ERROR\n");
    return -1;
  }

  // Read the value of PWM_MODE1
  mode1 = wiringPiI2CReadReg8(hPCA9685, PWM_MODE1);
  log_message(LOG_NORMAL, "  Read PWM_MODE1: %02X\n", mode1);

  log_function("PCA9685SetFrequency DONE\n");
  return 0;
}



// Setup PCA9685 with standard settings
int PCA9685Setup(int handle)
{
  log_function("PCA9685Setup(handle: %d)...\n", handle);

  unsigned data;
  double prescale;

  PCA9685InfoStruct *device;
  device = PCA9685GetInfo(handle);
  int hPCA9685 = device->hDevice;

  // Perform reset sequence

  // Tell all channels to stop
  if((data = wiringPiI2CWriteReg8(hPCA9685, PWM_ALL_OFF_L, 0x0)) != 0)
    return -1;
  if((data = wiringPiI2CWriteReg8(hPCA9685, PWM_ALL_OFF_H, 0x0)) != 0)
    return -1;

  // Set MODE2 to OUTDRV (Totem pole)
  if((data = wiringPiI2CWriteReg8(hPCA9685, PWM_MODE2, PWM_OUTDRV)) != 0)
    return -1;

  // Set MODE1 to enable ALLCALL 
  if((data = wiringPiI2CWriteReg8(hPCA9685, PWM_MODE1, PWM_ALLCALL)) != 0)
    return -1;

  // Wait for oscillator
  delay(5);

  // Set PWM frequency (which also does the rest of the reset)
  PCA9685SetFrequency(handle, device->frequency);

  log_function("PCA9685Setup DONE\n");
}


// Initialize the PCA9685 
int PCA9685Init(unsigned address, unsigned frequency)
{
  log_function("PCA9685Init(address: %02X, frequency: %d)...\n", address, frequency);

  int hPCA9685;

  // Connect to PCA9685 PWM module using I2C
  hPCA9685 = wiringPiI2CSetup(address);
  if (hPCA9685 == -1)
  {
    fprintf(stderr, "WiringPiI2C failed to initialize!\n");
    fprintf(stderr, "  errno: %d - %s\n", errno, strerror(errno));
    return -1;
  }

  // Add this device to PCA9685Info 
  int i;
  for (i=0; i < PCA9685InfoLen; i++)
  {
    // Check if this record in the table is unused
    if (PCA9685Info[i].hDevice == -1)
      break;
  }
  if ( i >= PCA9685_MAXDEVICES)
  {
    log_error("Maximum number of devices exceeded.\n");
    errno = EMFILE;
    return -1;
  }

  log_message(LOG_NORMAL, "Adding PCA9685 at address: %02x (i/o handle: %d) to row %d\n", address, hPCA9685, i);

  PCA9685Info[i].hDevice = hPCA9685;
  PCA9685Info[i].address = address;
  PCA9685Info[i].frequency = frequency;

  // Do a software reset
  if(PCA9685Reset(i) == -1)
    return -1;

  PCA9685Setup(i);

  log_function("PCA9685Init DONE\n");
  return i;
}

// Write to 8 bit register
int PCA9685WriteReg8(int handle, unsigned reg, unsigned data)
{
  log_function("PCA9685WriteReg8(handle: %d, register: %02X, data: %02X)...\n", handle, reg, data);

  int hPCA9685 = PCA9685GetInfo(handle)->hDevice;

  // Write the data
  if(wiringPiI2CWriteReg8(hPCA9685, reg, data & 0xFF) != 0)
    return -1;

  log_function("PCA9685WriteReg8 DONE\n");
  return 0;
}


// Write to one of the led registers (low and high), by LED number.
// Use on = 0 for LEDx_OFF and on != 0 for LEDx_ON
int PCA9685WriteLed12(int handle, unsigned led, int on, unsigned data)
{
  log_function("PCA9685WriteLed12(handle: %d, led: %02d, on: %s, data: %002X)...\n", handle, led, on ? "ON" : "OFF", data);

  // calculate register from led and on/off
  unsigned reg;
  if (on)
    reg = PWM_LED0_ON_L + (4 * led);
  else
    reg = PWM_LED0_OFF_L + (4 * led);

  // Write first 12 bits of data (low order 8 bits, and high order 4)
  if(PCA9685WriteReg8(handle, reg, data & 0xFF) != 0)
    return -1;
  if(PCA9685WriteReg8(handle, reg + 1, (data & 0xF00) >> 8) != 0)
    return -1;

  log_function("PCA9685WriteLed12 DONE\n");
  return 0;
}


// Turn led on for duty (where duty is between 0 and 4096).
int PCA9685WriteLedDuty(int handle, unsigned led, unsigned duty)
{
  log_function("PCA9685WriteLedDuty(handle: %d, led: %02d, duty: %04d)...\n", handle, led, duty);

  // Write cycle to off registers
  if(PCA9685WriteLed12(handle, led, PWM_ON, 0x000) != 0)
    return -1;
  if(PCA9685WriteLed12(handle, led, PWM_OFF, duty) != 0)
    return -1;

  log_function("PCA9685WriteLedDuty DONE\n");
  return 0;
}

