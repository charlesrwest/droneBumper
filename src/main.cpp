#include "ch.h"
#include "hal.h"

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

//Main is void due to being embedded program, stack size set in make file
int main(void)
{
//Initialize HAL and Kernel
halInit();
chSysInit();

//Start thread with default heap allocator (using first NULL pointer), passing it null as the pointer argument for function
//thread_t *thread1 = chThdCreateFromHeap(NULL, THD_WA_SIZE(128), NORMALPRIO, &thread1Function, NULL);
auto *thread1 = chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(128), NORMALPRIO, &thread1Function, NULL);

testClass myClass;
myClass.method();

systime_t currentTime = chVTGetSystemTimeX();
//Use with chVTTimeElapsedSinceX

while(true)
{
chThdSleepMilliseconds(5000);
}
}
