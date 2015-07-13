#include "ch.h"
#include "hal.h"
#include "ultrasonicRangerManager.hpp"

//Ask about heap size (is it just limited by static allocations and stack size?), C++11 and THD_WA_SIZE replacement in 3.0

//Define the thread function without using a macro
/**
This function flashes on of the LEDs using the ChibiOS functions to handle timing.
@param inputUserData: A pointer to any user data to pass the function
*/
static void thread1Function(void *inputUserData)
{
while(true) 
{ //Pins referred to as "pads"
palClearPad(GPIOC, GPIOC_LED_BLUE);
chThdSleepMilliseconds(500);
palSetPad(GPIOC, GPIOC_LED_BLUE);
chThdSleepMilliseconds(500);
}

}

class testClass
{
public:
testClass()
{
test = 5;
}

void method()
{
}

int test;
};


void externalInterruptCallback(EXTDriver *inputDriver, expchannel_t inputChannel)
{
palTogglePad(GPIOC, GPIOC_LED_RED);
}

//Supports up to 16 different pin interrupts.  Each channel can be linked to the corresponding pin number (first entry -> 1) on one of the ports (A, B, C, etc) but you cannot have two pins with the same number enabled (can't have both A0 and B0).
static EXTConfig externalInterruptConfig =
{
{
{EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOA, externalInterruptCallback}, //0
{EXT_CH_MODE_DISABLED, NULL}, //1
{EXT_CH_MODE_DISABLED, NULL}, //2
{EXT_CH_MODE_DISABLED, NULL}, //3
{EXT_CH_MODE_DISABLED, NULL}, //4
{EXT_CH_MODE_DISABLED, NULL}, //5
{EXT_CH_MODE_DISABLED, NULL}, //6
{EXT_CH_MODE_DISABLED, NULL}, //7
{EXT_CH_MODE_DISABLED, NULL}, //8
{EXT_CH_MODE_DISABLED, NULL}, //9
{EXT_CH_MODE_DISABLED, NULL}, //10
{EXT_CH_MODE_DISABLED, NULL}, //11
{EXT_CH_MODE_DISABLED, NULL}, //12
{EXT_CH_MODE_DISABLED, NULL}, //13
{EXT_CH_MODE_DISABLED, NULL}, //14
{EXT_CH_MODE_DISABLED, NULL} //15
}
};

//Main is void due to being embedded program, stack size set in make file
int main(void)
{
//Initialize HAL and Kernel
halInit();
chSysInit();

//Set up pin driven interrupt
palSetPadMode(GPIOA, 0, PAL_MODE_INPUT_PULLUP);
extStart(&EXTD1, &externalInterruptConfig);

//Start thread with default heap allocator (using first NULL pointer), passing it null as the pointer argument for function
//thread_t *thread1 = chThdCreateFromHeap(NULL, THD_WA_SIZE(128), NORMALPRIO, &thread1Function, NULL);
auto *thread1 = chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(128), NORMALPRIO, &thread1Function, NULL);

/*
char memory[sizeof(testClass)];
testClass *myClass = (testClass *) memory;
new(myClass) testClass();
myClass->method();
*/

systime_t currentTime = chVTGetSystemTimeX();
//Use with chVTTimeElapsedSinceX

while(true)
{
chThdSleepMilliseconds(5000);
}
}
