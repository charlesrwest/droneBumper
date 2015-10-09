#pragma once

#include "ch.h"
#include "hal.h"
#include<stdint.h>


namespace droneBumper
{

const int BLINK_ANIMATOR_THREAD_STACK_SIZE = 128;
const int BLINK_ANIMATOR_MAILBOX_SIZE  = 1;

/**
This class permits simple LED animations to be performed. This object has a thread-safe interface to ask the LED to be blinked a certain number of times during a fraction of a time period.  It ignores further commands until the animation is completed.
*/
class blinkAnimator
{
public:
/**
This funciton initializes the blink animator to affect the given pin.  isValid will be set to true if the contruction succeeds.
@param inputLEDPinPort: The port the LED is on
@param inputLEDPinNumber: The port pin the LED is on
*/
blinkAnimator(stm32_gpio_t *inputLEDPinPort, uint32_t inputLEDPinNumber);

/**
This function triggers a LED animation if there isn't one running already.
@param inputNumberOfTimesToBlink: How many times to go on,wait-off,wait
@param inputLockedPeriod: How many blink equivalent periods for the animator to stay locked
@param inputBlinkPeriod: The period of a blink in milliseconds
@return: True if the animation was added and the animator wasn't busy, false otherwise
*/
bool startBlinkAnimation(uint32_t inputNumberOfTimesToBlink, uint32_t inputLockedPeriod, uint32_t inputBlinkPeriod);

/**
This function asks the associated thread to shut down and waits for it to do so.
*/
~blinkAnimator();

friend void blinkThreadFunction(void *);

bool isValid; //Flag to indicate if constructor succeeded
int numberOfTimesToBlink = 0;
int lockedPeriod = 0; //Number of times the object could blink before it stops ignoring commands
int blinkPeriod = 0; //Period of one blink in milliseconds

thread_t *blinkAnimationThread;
bool shutdownFlag = false; //Set true if it is time for the associated thread to shut down
int currentBlinkCount = 0; //How many blinks have been completed
bool LEDIsOn = false; //True if the LED is currently triggered by the blink animator

bool animationIsRunning = false; //True if the thread is currently running an animation
msg_t memoryForMailbox[BLINK_ANIMATOR_MAILBOX_SIZE];
mailbox_t notificationInterface;

stm32_gpio_t *LEDPinPort;         //Port of the LED pin is on
uint32_t LEDPinNumber;  //Number of the LED pin in the port
};

/**
This function manages the LED animations associated with an LED animator object.
@param inputBlinkAnimator: A pointer to the blinkAnimator that owns the LED pin
*/
void blinkThreadFunction(void *inputBlinkAnimator);


}
