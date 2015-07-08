#include "ch.h"
#include "hal.h"


//Allocate memory for thread stack.
static THD_WORKING_AREA(thread1WorkingArea, 128);

//Define the thread function without using a macro
/**
This function flashes on of the LEDs using the ChibiOS functions to handle timing.
@param inputUserData: A pointer to any user data to pass the function
*/
static void thread1Function(void *inputUserData)
{
while(true)
{
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

//Start thread, passing it null as the pointer argument
chThdCreateStatic(thread1WorkingArea, sizeof(thread1WorkingArea), NORMALPRIO, &thread1Function, NULL);

testClass myClass;
myClass.method();

systime_t currentTime = chVTGetSystemTimeX();
//Use with chVTTimeElapsedSinceX

while(true)
{
chThdSleepMilliseconds(5000);
}
}
