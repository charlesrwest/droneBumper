#include "blinkAnimator.hpp"

using namespace droneBumper;

/**
This funciton initializes the blink animator to affect the given pin.  isValid will be set to true if the contruction succeeds.
@param inputLEDPinPort: The port the LED is on
@param inputLEDPinNumber: The port pin the LED is on
*/
blinkAnimator::blinkAnimator(stm32_gpio_t *inputLEDPinPort, uint32_t inputLEDPinNumber)
{
isValid = false;

if(inputLEDPinPort == nullptr)
{ //Invalid port
return;
}

LEDPinPort = inputLEDPinPort;
LEDPinNumber = inputLEDPinNumber;

//Set pin as output
palClearPad(LEDPinPort, LEDPinNumber);
palSetPadMode(LEDPinPort, LEDPinNumber, PAL_MODE_OUTPUT_PUSHPULL);

//Initialize mailbox
chMBObjectInit(&notificationInterface, memoryForMailbox, BLINK_ANIMATOR_MAILBOX_SIZE);

//Create thread for running animations
blinkAnimationThread = chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(BLINK_ANIMATOR_THREAD_STACK_SIZE), NORMALPRIO, &blinkThreadFunction, (void *) this);


isValid = true;
}

/**
This function triggers a LED animation if there isn't one running already.
@param inputNumberOfTimesToBlink: How many times to go on,wait-off,wait
@param inputLockedPeriod: How many blink equivalent periods for the animator to stay locked
@param inputBlinkPeriod: The period of a blink in milliseconds
@return: True if the animation was added and the animator wasn't busy, false otherwise
*/
bool blinkAnimator::startBlinkAnimation(uint32_t inputNumberOfTimesToBlink, uint32_t inputLockedPeriod, uint32_t inputBlinkPeriod)
{
if(animationIsRunning || !isValid)
{
return false;
}

animationIsRunning = true; //Prevent other calls
numberOfTimesToBlink = inputNumberOfTimesToBlink;
lockedPeriod = inputLockedPeriod;
blinkPeriod = inputBlinkPeriod;
currentBlinkCount = 0;
LEDIsOn = false;

//palTogglePad(LEDPinPort, LEDPinNumber);

//Attempt to wakeup thread if it is sleeping
if(chMBPost(&notificationInterface, (msg_t) 0, TIME_IMMEDIATE) != MSG_OK)
{
animationIsRunning = false; //Failed
return false;
}

return true; //Animation is running
}

/**
This function asks the associated thread to shut down and waits for it to do so.
*/
blinkAnimator::~blinkAnimator()
{
if(isValid == true)
{
shutdownFlag = true;

//Attempt to wakeup the thread if it is sleeping so that it can shut down
chMBPost(&notificationInterface, (msg_t) 0, TIME_IMMEDIATE);

chThdWait(blinkAnimationThread); //Free memory allocated for thread
}
}

/**
This function manages the LED animations associated with an LED animator object.
@param inputBlinkAnimator: A pointer to the blinkAnimator that owns the LED pin
*/
void droneBumper::blinkThreadFunction(void *inputBlinkAnimator)
{
blinkAnimator *blinker = (blinkAnimator *) inputBlinkAnimator;

while(!blinker->shutdownFlag)
{
msg_t buffer; //See if there is an animation to run
if(chMBFetch(&(blinker->notificationInterface), &buffer, TIME_INFINITE) != MSG_OK)
{
blinker->isValid = false;
return;
}

//palSetPad(blinker->LEDPinPort, blinker->LEDPinNumber);
//palTogglePad(blinker->LEDPinPort, blinker->LEDPinNumber);

//There is an animation!
while(blinker->currentBlinkCount < blinker->lockedPeriod)
{

if(blinker->currentBlinkCount < blinker->numberOfTimesToBlink && !blinker->LEDIsOn)
{
palSetPad(blinker->LEDPinPort, blinker->LEDPinNumber);
blinker->LEDIsOn = true;
}
else if(blinker->currentBlinkCount < blinker->numberOfTimesToBlink && blinker->LEDIsOn)
{
palClearPad(blinker->LEDPinPort, blinker->LEDPinNumber);
blinker->LEDIsOn = false;
}

if(!blinker->LEDIsOn)
{
blinker->currentBlinkCount++;
}

//TODO: Finish this function
chThdSleepMilliseconds(blinker->blinkPeriod/2);
} //While

blinker->animationIsRunning = false;
} //While

}
