// PCA9685 Servo Controller
#define PWM_MODE1	0x00
#define PWM_MODE2	0x01
#define PWM_SUBADDR1	0x02
#define PWM_SUBADDR2	0x03
#define PWM_SUBADDR3	0x04
#define PWM_LED0_ON_L	0x06
#define PWM_LED0_ON_H	0x07
#define PWM_LED0_OFF_L	0x08
#define PWM_LED0_OFF_H	0x09
#define PWM_ALL_ON_L	0xFA
#define PWM_ALL_ON_H	0xFB
#define PWM_ALL_OFF_L	0xFC
#define PWM_ALL_OFF_H	0xFD
#define PWM_PRESCALE	0xFE

#define PWM_RESET	0x06
#define PWM_RESTART	0x80
#define PWM_SLEEP	0x10
#define PWM_ALLCALL	0x01
#define PWM_INVERT	0x10
#define PWM_OUTDRV	0x04

#define PWM_OFF         0
#define PWM_ON          1

#define PCA9685_MAXDEVICES  10   // Max number of devices in the PCA9685Info table
typedef struct 
{
  unsigned hDevice;      // i/o handle
  unsigned address;      // device address
  unsigned frequency;    // frequency
} PCA9685InfoStruct;

// Software reset of the PCA9685
int PCA9685Reset(int handle);

// Set the frequency (PWM_PRESCALE) and restart
int PCA9685SetFrequency(int handle, unsigned frequency);

// Setup PCA9685 with standard settings
int PCA9685Setup(int handle);

// Initialize the PCA9685 
int PCA9685Init(unsigned address, unsigned frequency);

// Write to 8 bit register
int PCA9685WriteReg8(int handle, unsigned register, unsigned data);

// Write to one of the led registers (low and high), by LED number.
// Use on = 0 for LEDx_OFF and on != 0 for LEDx_ON
int PCA9685WriteLed12(int handle, unsigned led, int on, unsigned data);

// Write a %duty to one of the led registers (low and high), by LED number.
int PCA9685WriteLedDuty(int handle, unsigned led, unsigned duty);
