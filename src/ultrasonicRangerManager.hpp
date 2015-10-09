#pragma once

#include "ch.h"
#include "hal.h"
#include<stdint.h>

namespace droneBumper
{

#define ULTRASONIC_RANGE_HISTORY_SIZE 5 //How many past/present range entries to keep in memory for analysis
#define MAXIMUM_NUMBER_OF_EXTERNAL_INTERRUPT_PINS 16
#define TRIGGER_ON_TIME 1000 //How long to hold the trigger pin high to start a pulse 
#define TRIGGER_THREAD_STACK_SIZE 128


class ultrasonicRangerManager; //Defined below



/**
This represents the information associated with a particular ranger, such as what is its trigger and echo pins. It should be declared volatile when it is being used.
*/
struct ultrasonicRanger
{
stm32_gpio_t *triggerPinPort;         //Port of the trigger pin is on
uint32_t triggerPinNumber;  //Number of the trigger pin in the port
stm32_gpio_t *echoPinPort;         //Port of the trigger pin is on
uint32_t echoPinNumber;  //Number of the trigger pin in the port
systime_t ranges[ULTRASONIC_RANGE_HISTORY_SIZE];//Ranger data/statistics, updated by the main loop
int32_t currentRangeIndex; //Which ranges[] entry is most recent (currentIndex + 1) ~ previous entry
systime_t echoPulseStartTime; //When the last recorded pulse start occurred (in microseconds)
};


/**
The purpose of this C style class is to manage the timing of ultrasonic rangers and allow a dynamic number to be registered.  It allows sets of rangers to be registered so that they are fired at the same time and creates the associated callbacks for the given pins.
*/
class ultrasonicRangerManager
{
public:
/**
This function initializes the ultrasonicRangerManager object with the given rangers and sets it to be the rangerManagerSingleton.  If one of the arguments is invalid or there is already a rangerManagerSingleton then the constructor fails, clears any memory that had been allocated and sets the state to invalid.  Notifications are handled in the thread that uses the range data.  It is assumed that external interrupts have not yet been configured.
@param inputRangerSets: a pointer to the array of arrays which holds the ranger sets
@param inputRangerSetSizes: The array holding the size of each of the ranger sets
@param inputNumberOfRangerSets: The number of ranger sets in the array of arrays
@param inputMicrosecondsBetweenSampling: Approximately how long to wait before triggering the next set to fire (typically between 25000 and 38000 microseconds)
@param inputEventBufferSize: How many external interrupt events to queue before dropping any (buffer size)
*/
ultrasonicRangerManager(struct ultrasonicRanger **inputRangerSets, uint32_t *inputRangerSetSizes, uint32_t inputNumberOfRangerSets, uint32_t inputMicrosecondsBetweenSampling, uint32_t inputEventBufferSize);

/*
This function processes any current outstanding ultrasonicRangerEvent messages and uses them to update the range reading stored in the ultrasonic ranger structures.
@return: false if there was an error
*/
bool updateRanges();

/**
This function cleans up the class unless isValid is set to false (in which case it does nothing).  Specifically, it shuts down the interrupts, stops the thread, releases the memory for the thread, mailbox and memory pool and sets the singleton pointer to nullptr.
*/
~ultrasonicRangerManager();

bool isValid; //Flag to indicate if constructor succeeded
msg_t *memoryForMailbox;
mailbox_t notificationInterface; //The interface that informs the waiting thread that a echo pin event has occurred and passes a pointer to the event message
void *memoryForPool;
memory_pool_t messageMemoryPool; //The memory pool that is used to allocated/delete memory for event messages
thread_t *triggerThread; //A chibios thread that is used to trigger the ultrasonics.  The thread is created, started, stopped and destroyed by this class.
bool triggerThreadShutdownFlag;
uint32_t microsecondsBetweenSampling;  //How long to wait between sending trigger pulses

friend void triggerThreadFunction(void *);
friend void echoPinInterruptCallback(EXTDriver *inputDriver, expchannel_t inputChannel);

protected:
/**
This is a helper function that is used in the constructor to setup the interrupts
@return: True if initializing the interrupts succeeded
*/
bool initializeInterrupts();

EXTConfig *externalInterruptConfiguration;
uint32_t numberOfRangerSets; //How many ranger sets there are
uint32_t numberOfRangers; //The total number of rangers
struct ultrasonicRanger *pinNumberToRanger[MAXIMUM_NUMBER_OF_EXTERNAL_INTERRUPT_PINS]; //An array of pointers which converts from the pin number (regardless of port) to the corresponding ultrasonic ranger object (NULL if not found)
struct ultrasonicRanger **rangerSets; //The array of array pointers for the ultrasonic rangers
uint32_t *rangerSetSizes; //The size of each ranger set (number of rangers)
uint32_t currentSetToTrigger; //Which set of rangers to trigger next
uint32_t eventBufferSize;
};

struct ultrasonicRangerEvent
{
struct ultrasonicRanger *ranger; //The ultrasonic ranger associated with the event
bool transitionedHigh; //True if there was an edge transition high, false if transition low
systime_t timestamp; //The clock time when the event was recorded
};

/**
This function triggers each of the ranger sets sequentially.
@param inputUltrasonicRangerManager: A pointer to the ultrasonic ranger manager which contains the rangers to trigger
*/
void triggerThreadFunction(void *inputUltrasonicRangerManager);

/*
This function is registered as the callback for external rising and falling event interrupts.  When it is called, it records the time, allocates memory  from the manager's thread pool for a message, initializes the message and sends a mailbox message with a pointer to the message.  The function checking the mailbox handles freeing the allocated memory.
@param inputDriver: The external interrupt driver
@param inputChannel: The channel associated with the current interrupt
*/
void echoPinInterruptCallback(EXTDriver *inputDriver, expchannel_t inputChannel);


}

