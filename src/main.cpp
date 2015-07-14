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

droneBumper::ultrasonicRanger rangers[6] =
{
{GPIOA, 4, GPIOA, 5, {0,0,0,0,0}, 0, 0},
{GPIOA, 8, GPIOC, 11, {0,0,0,0,0}, 0, 0},     //Set 0
{GPIOB, 2, GPIOB, 3, {0,0,0,0,0}, 0, 0},
{GPIOB, 8, GPIOB, 6, {0,0,0,0,0}, 0, 0},     //Set 1
{GPIOC, 10, GPIOA, 1, {0,0,0,0,0}, 0, 0},
{GPIOC, 12, GPIOC, 13, {0,0,0,0,0}, 0, 0}  //Set 2
};

uint32_t rangerSetSizes[3] = {2,2,2};

droneBumper::ultrasonicRanger *rangerSets[3] = {&(rangers[0]), &(rangers[2]), &(rangers[4])};
char stringBuffer[256];

//Main is void due to being embedded program, stack size set in make file
int main(void)
{
//Initialize HAL and Kernel
halInit();
chSysInit();

droneBumper::ultrasonicRangerManager myRanger(rangerSets, rangerSetSizes, 3, 38000, 50);

//Turn on serial using usart 1 (Don't connect user USB!)
sdStart(&SD1, nullptr); //Use usart 4 with default settings, then set pin modes (default serial rate defined in SERIAL_DEFAULT_BITRATE directive in halconf.h)
palSetPadMode(GPIOA, 9, PAL_MODE_ALTERNATE(1)); //TX 
palSetPadMode(GPIOA, 10, PAL_MODE_ALTERNATE(1)); //RX
palSetPadMode(GPIOA, 11, PAL_MODE_ALTERNATE(1)); //RTS
palSetPadMode(GPIOA, 12, PAL_MODE_ALTERNATE(1)); //CTS

int numberOfCharactersInString = 0;

while(true)
{
myRanger.updateRanges();

for(int i=0; i < 6; i++)
{
chprintf((BaseSequentialStream *) &SD1, "Range %d: %lu %lu %lu %lu %lu\r\n", i, rangers[i].ranges[(rangers[i].currentRangeIndex )%5], rangers[i].ranges[(rangers[i].currentRangeIndex +1)%5], rangers[i].ranges[(rangers[i].currentRangeIndex +2)%5], rangers[i].ranges[(rangers[i].currentRangeIndex +3)%5], rangers[i].ranges[(rangers[i].currentRangeIndex +4)%5]);
//sdWrite(&SD1, (const uint8_t *) stringBuffer, numberOfCharactersInString);
}


palTogglePad(GPIOC, GPIOC_LED_RED);
chThdSleepMilliseconds(500);
}
}
