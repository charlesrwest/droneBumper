#include "ch.h"
#include "hal.h"
#include "ultrasonicRangerManager.hpp"
#include "chprintf.h"
#include "blinkAnimator.hpp"

using namespace droneBumper;

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

//Stubs so that it can compile (shouldn't ever be called)


droneBumper::ultrasonicRanger rangers[9] =
{
{GPIOC, 13, GPIOF, 0, {0,0,0,0,0}, 0, 0},
{GPIOC, 14, GPIOF, 1, {0,0,0,0,0}, 0, 0},     //Set 0
{GPIOC, 15, GPIOB, 7, {0,0,0,0,0}, 0, 0},     
{GPIOA, 2, GPIOB, 8, {0,0,0,0,0}, 0, 0}, //Set 1
{GPIOA, 3, GPIOB, 9, {0,0,0,0,0}, 0, 0},  
{GPIOB, 15, GPIOB, 2, {0,0,0,0,0}, 0, 0}, //Set 2 
//{GPIOA, 8, GPIOB, 10, {0,0,0,0,0}, 0, 0}, 
//{GPIOA, 9, GPIOB, 11, {0,0,0,0,0}, 0, 0},  //Set 3 

//{GPIOA, 10, GPIOB, 13, {0,0,0,0,0}, 0, 0}  //Set 4
};

//uint32_t rangerSetSizes[5] = {2,2,2,2,1};

uint32_t rangerSetSizes[5] = {2,2,2,2,1};

//droneBumper::ultrasonicRanger *rangerSets[5] = {&(rangers[0]), &(rangers[2]), &(rangers[4]), &(rangers[6]), &(rangers[8])};

droneBumper::ultrasonicRanger *rangerSets[3] = {&(rangers[0]), &(rangers[2]), &(rangers[4])};

unsigned char stringBuffer[256] = "testString\r\n";

//Main is void due to being embedded program, stack size set in make file
int main(void)
{
//Initialize HAL and Kernel
halInit();
chSysInit();

//droneBumper::ultrasonicRangerManager myRanger(rangerSets, rangerSetSizes, 5, 38000, 50);

droneBumper::ultrasonicRangerManager myRanger(rangerSets, rangerSetSizes, 3, 38000, 50);

//Turn on serial using usart (Don't connect user USB!)
sdStart(&SD2, nullptr); //Use usart 2 with default settings, then set pin modes (default serial rate defined in SERIAL_DEFAULT_BITRATE directive in halconf.h)
palSetPadMode(GPIOA, 0, PAL_MODE_ALTERNATE(1)); //CTS
palSetPadMode(GPIOA, 1, PAL_MODE_ALTERNATE(1)); //RTS
palSetPadMode(GPIOA, 14, PAL_MODE_ALTERNATE(1)); //TX
palSetPadMode(GPIOA, 15, PAL_MODE_ALTERNATE(1)); //RX



//Enable LED
palSetPadMode(GPIOB, 6, PAL_MODE_OUTPUT_PUSHPULL);
palClearPad(GPIOB, 6);

int numberOfCharactersInString = 12;
int numberOfCharactersRead = 0;

blinkAnimator myBlinker(GPIOB, 6);



while(true)
{
myRanger.updateRanges();

for(int i=0; i < 1; i++)
{
//chprintf((BaseSequentialStream *) &SD2, "Range %d: %lu %lu %lu %lu %lu\r\n", i, rangers[i].ranges[(rangers[i].currentRangeIndex )%5], rangers[i].ranges[(rangers[i].currentRangeIndex +1)%5], rangers[i].ranges[(rangers[i].currentRangeIndex +2)%5], rangers[i].ranges[(rangers[i].currentRangeIndex +3)%5], rangers[i].ranges[(rangers[i].currentRangeIndex +4)%5]);
//sdWrite(&SD1, (const uint8_t *) stringBuffer, numberOfCharactersInString);
}



int rangerAverages[10];
for(int i=0; i<10; i++)
{
rangerAverages[i] = 0;
}


for(int a=0; a<6; a++)
{
for(int i=0; i<2; i++)
{
rangerAverages[a] += rangers[a].ranges[(rangers[a].currentRangeIndex + i)%5];
}

rangerAverages[a] = rangerAverages[a]/2;
}

//4800 is about a meter

for(int i=0; i<6; i++)
{

if(rangerAverages[i] < 4800)
{
myBlinker.startBlinkAnimation(i+1, 10, 1000);
break;
}
}

//myBlinker.startBlinkAnimation(2, 10, 1000);

//numberOfCharactersInString = iqReadTimeout(&((SerialDriver *) &SD2)->iqueue, (unsigned char *) stringBuffer, 8, 100000);

//Read from usart
//numberOfCharactersInString = sdReadTimeout(&SD2, stringBuffer, 8, 100000);

//Echo it back
//sdWrite(&SD2, stringBuffer, 11);

/*
numberOfCharactersRead =  sdReadTimeout( &SD2, stringBuffer, 256, 100);

if(numberOfCharactersRead > 0)
{
chSequentialStreamWrite((BaseSequentialStream *) &SD2, stringBuffer, numberOfCharactersRead);
}
*/



//chSequentialStreamPut((BaseSequentialStream *) &SD2, 'a');


//chprintf((BaseSequentialStream *) &SD2, "%d\r\n", numberOfCharactersRead);


/*
if(numberOfCharactersRead > 0)
{
palTogglePad(GPIOB, 6); //Toggle if something was read
}
*/

chThdSleepMilliseconds(5);
}
}


