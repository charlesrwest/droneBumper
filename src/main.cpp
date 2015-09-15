#include "ch.h"
#include "hal.h"
#include "ultrasonicRangerManager.hpp"
#include "chprintf.h"


/*
struct ultrasonicRanger
{
stm32_gpio_t *triggerPinPort;         //Port of the trigger pin is on
uint32_t triggerPinNumber;  //Number of the trigger pin in the port
stm32_gpio_t *echoPinPort;         //Port of the trigger pin is on
uint32_t echoPinNumber;  //Number of the trigger pin in the port
systime_t ranges[ULTRASONIC_RANGE_HISTORY_SIZE];//Ranger data/statistics, updated by the main loop
int32_t currentRangeIndex; //Which ranges[] entry is most recent (currentIndex + 1) ~ previous entry
systime_t echoPulseStartTime; //When the last recorded pulse start occurred (in microseconds)
bool expectingPulseReturn; //Whether a pulse has been sent that we can expect to return
};
*/

droneBumper::ultrasonicRanger rangers[9] =
{
{GPIOC, 13, GPIOF, 0, {0,0,0,0,0}, 0, 0},
{GPIOC, 14, GPIOF, 1, {0,0,0,0,0}, 0, 0},     //Set 0
//{GPIOC, 15, GPIOB, 7, {0,0,0,0,0}, 0, 0},     
//{GPIOA, 2, GPIOB, 8, {0,0,0,0,0}, 0, 0}, //Set 1
//{GPIOA, 3, GPIOB, 9, {0,0,0,0,0}, 0, 0},  
//{GPIOB, 15, GPIOB, 2, {0,0,0,0,0}, 0, 0}, //Set 2 
//{GPIOA, 8, GPIOB, 10, {0,0,0,0,0}, 0, 0}, 
//{GPIOA, 9, GPIOB, 11, {0,0,0,0,0}, 0, 0},  //Set 3 

//{GPIOA, 10, GPIOB, 13, {0,0,0,0,0}, 0, 0}  //Set 4
};

//uint32_t rangerSetSizes[5] = {2,2,2,2,1};

uint32_t rangerSetSizes[5] = {1,2,2,2,1};

//droneBumper::ultrasonicRanger *rangerSets[5] = {&(rangers[0]), &(rangers[2]), &(rangers[4]), &(rangers[6]), &(rangers[8])};

droneBumper::ultrasonicRanger *rangerSets[1] = {&(rangers[0])};

char stringBuffer[256];

//Main is void due to being embedded program, stack size set in make file
int main(void)
{
//Initialize HAL and Kernel
halInit();
chSysInit();

//droneBumper::ultrasonicRangerManager myRanger(rangerSets, rangerSetSizes, 5, 38000, 50);

droneBumper::ultrasonicRangerManager myRanger(rangerSets, rangerSetSizes, 1, 38000, 50);

//Turn on serial using usart (Don't connect user USB!)
sdStart(&SD2, nullptr); //Use usart 2 with default settings, then set pin modes (default serial rate defined in SERIAL_DEFAULT_BITRATE directive in halconf.h)
palSetPadMode(GPIOA, 0, PAL_MODE_ALTERNATE(1)); //CTS
palSetPadMode(GPIOA, 1, PAL_MODE_ALTERNATE(1)); //RTS
palSetPadMode(GPIOA, 14, PAL_MODE_ALTERNATE(1)); //TX
palSetPadMode(GPIOA, 15, PAL_MODE_ALTERNATE(3)); //RX



//Enable LED
palSetPadMode(GPIOB, 6, PAL_MODE_OUTPUT_PUSHPULL);
palClearPad(GPIOB, 6);

int numberOfCharactersInString = 0;

while(true)
{
myRanger.updateRanges();

for(int i=0; i < 2; i++)
{
chprintf((BaseSequentialStream *) &SD2, "Range %d: %lu %lu %lu %lu %lu\r\n", i, rangers[i].ranges[(rangers[i].currentRangeIndex )%5], rangers[i].ranges[(rangers[i].currentRangeIndex +1)%5], rangers[i].ranges[(rangers[i].currentRangeIndex +2)%5], rangers[i].ranges[(rangers[i].currentRangeIndex +3)%5], rangers[i].ranges[(rangers[i].currentRangeIndex +4)%5]);
//sdWrite(&SD1, (const uint8_t *) stringBuffer, numberOfCharactersInString);
}


//palTogglePad(GPIOB, 6);
chThdSleepMilliseconds(500);
}
}
