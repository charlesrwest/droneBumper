#include "ultrasonicRangerManager.hpp"

using namespace droneBumper;

/*
The external interrupt doesn't appear to have a way to pass data to it, so the class is implemented as a singleton.  Any ultrasonicRangerManager that is created adjusts this pointer to pick to it and its initialization will fail if another manager has already taken it.
*/
volatile struct ultrasonicRangerManager *rangerManagerSingleton = nullptr;

/**
This function initializes the ultrasonicRangerManager object with the given rangers and sets it to be the rangerManagerSingleton.  If one of the arguments is invalid or there is already a rangerManagerSingleton then the constructor fails, clears any memory that had been allocated and sets the state to invalid.  Notifications are handled in the thread that uses the range data.  It is assumed that external interrupts have not yet been configured.
@param inputRangerSets: a pointer to the array of arrays which holds the ranger sets
@param inputRangerSetSizes: The array holding the size of each of the ranger sets
@param inputNumberOfRangerSets: The number of ranger sets in the array of arrays
@param inputMicrosecondsBetweenSampling: Approximately how long to wait before triggering the next set to fire (typically between 25000 and 38000 microseconds)
@param inputEventBufferSize: How many external interrupt events to queue before dropping any (buffer size)
*/
ultrasonicRangerManager::ultrasonicRangerManager(struct ultrasonicRanger **inputRangerSets, uint32_t *inputRangerSetSizes, uint32_t inputNumberOfRangerSets, uint32_t inputMicrosecondsBetweenSampling, uint32_t inputEventBufferSize)
{
if(inputRangerSets == nullptr || inputRangerSetSizes == nullptr || rangerManagerSingleton != nullptr || inputMicrosecondsBetweenSampling < 1000)
{ //Input invalid or singleton already taken
isValid = false;
return;
}

numberOfRangerSets = inputNumberOfRangerSets;
rangerSetSizes = inputRangerSetSizes;
rangerSets = inputRangerSets;
currentSetToTrigger = 0;
microsecondsBetweenSampling = inputMicrosecondsBetweenSampling;
eventBufferSize = inputEventBufferSize;

numberOfRangers = 0; //Determine how many rangers there are
for(uint32_t i=0; i<numberOfRangerSets; i++)
{
numberOfRangers += rangerSetSizes[i]; //Add up the number of rangers in each set to get the total
}



//Setup the pin to ranger pointer array, determine if there are any conflicts (two pins with the same number) and set ultrasonic ranges to max and index to 0
for(uint32_t i=0; i<MAXIMUM_NUMBER_OF_EXTERNAL_INTERRUPT_PINS; i++)
{
pinNumberToRanger[i] = nullptr;
}

for(uint32_t ii=0; ii<numberOfRangerSets; ii++)
{
for(uint32_t i=0; i<rangerSetSizes[ii]; i++)
{
if(rangerSets[ii][i].echoPinNumber > MAXIMUM_NUMBER_OF_EXTERNAL_INTERRUPT_PINS)
{
isValid = false; //Ranger index is out of acceptable range
return;
}

if(pinNumberToRanger[rangerSets[ii][i].echoPinNumber] != nullptr)
{
isValid = false; //Two rangers have the same interrupt pin number
return;
}
pinNumberToRanger[rangerSets[ii][i].echoPinNumber] = &rangerSets[ii][i];

rangerSets[ii][i].currentRangeIndex = 0;
rangerSets[ii][i].echoPulseStartTime = 0;

for(uint32_t a = 0; a < ULTRASONIC_RANGE_HISTORY_SIZE; a++)
{
rangerSets[ii][i].ranges[a] = UINT32_MAX;
}
}
}

//Allocate memory for and initialize memory pool
memoryForPool = chHeapAlloc(nullptr, sizeof(struct ultrasonicRangerEvent)*eventBufferSize);
if(memoryForPool == nullptr)
{
isValid = false;
return;
}

chPoolObjectInit(&messageMemoryPool, sizeof(struct ultrasonicRangerEvent), nullptr); //Memory pool not allowed to grow after initialization
chPoolLoadArray(&messageMemoryPool, memoryForPool, eventBufferSize);

//Allocate and initialize mailbox
memoryForMailbox = (msg_t *) chHeapAlloc(nullptr, sizeof(msg_t)*eventBufferSize);
if(memoryForMailbox == nullptr)
{
chHeapFree(memoryForPool);
isValid = false;
return;
}
chMBObjectInit(&notificationInterface, memoryForMailbox, eventBufferSize);

if(initializeInterrupts() != true)
{
chHeapFree(memoryForPool);
chMBReset(&notificationInterface);
chHeapFree((void *) memoryForMailbox);
}

triggerThreadShutdownFlag = false;
triggerThread = chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(TRIGGER_THREAD_STACK_SIZE), NORMALPRIO, &triggerThreadFunction, (void *) this);

if(triggerThread == nullptr)
{//Thread creation failed
extStop(&EXTD1); //disable external interrupt since configuration being deleted
chHeapFree((void *) externalInterruptConfiguration);
chHeapFree(memoryForPool);
chMBReset(&notificationInterface);
chHeapFree((void *) memoryForMailbox);
isValid = false;
return;
}

//Set clear trigger pins and set them as outputs
for(uint32_t ii=0; ii<numberOfRangerSets; ii++)
{
for(uint32_t i=0; i<rangerSetSizes[ii]; i++)
{
palClearPad(rangerSets[ii][i].triggerPinPort, rangerSets[ii][i].triggerPinNumber);
palSetPadMode(rangerSets[ii][i].triggerPinPort, rangerSets[ii][i].triggerPinNumber, PAL_MODE_OUTPUT_PUSHPULL);
}
}

rangerManagerSingleton = this;
isValid = true;
}

/**
This is a helper function that is used in the constructor to setup the interrupts
@return: True if initializing the interrupts succeeded
*/
bool ultrasonicRangerManager::initializeInterrupts()
{
//Allocate memory for external interrupt configuration
externalInterruptConfiguration =(EXTConfig *) chHeapAlloc(nullptr, sizeof(EXTConfig));
if(externalInterruptConfiguration == nullptr)
{
return false;
}

for(uint32_t i=0; i<MAXIMUM_NUMBER_OF_EXTERNAL_INTERRUPT_PINS; i++)
{
if(pinNumberToRanger[i] != nullptr)
{//Setup interrupt
uint32_t portFlagBit = 0;
auto echoPinPort = pinNumberToRanger[i]->echoPinPort;
if(echoPinPort == GPIOA)
{
portFlagBit = EXT_MODE_GPIOA;
}
else if(echoPinPort == GPIOB)
{
portFlagBit = EXT_MODE_GPIOB;
}
else if(echoPinPort == GPIOC)
{
portFlagBit = EXT_MODE_GPIOC;
}
else if(echoPinPort == GPIOD)
{
portFlagBit = EXT_MODE_GPIOD;
}
else if(echoPinPort == GPIOE)
{
portFlagBit = EXT_MODE_GPIOE;
}
else if(echoPinPort == GPIOF)
{
portFlagBit = EXT_MODE_GPIOF;
}

palSetPadMode(pinNumberToRanger[i]->echoPinPort, pinNumberToRanger[i]->echoPinNumber, PAL_MODE_INPUT_PULLUP); //Enable interrupt pin
externalInterruptConfiguration->channels[i] = {EXT_CH_MODE_BOTH_EDGES | EXT_CH_MODE_AUTOSTART | portFlagBit, echoPinInterruptCallback};
}
else
{
externalInterruptConfiguration->channels[i] = {EXT_CH_MODE_DISABLED, NULL};
}

}

//Initialize external interrupts
extStart(&EXTD1, externalInterruptConfiguration);
return true;
}

/*
This function processes any current outstanding ultrasonicRangerEvent messages and uses them to update the range reading stored in the ultrasonic ranger structures.
@return: false if there was an error
*/
bool ultrasonicRangerManager::updateRanges()
{
while(true) //If there are messages, process them
{
chSysLock(); //Establish kernel lock
if(chMBGetUsedCountI(&notificationInterface) == 0)
{
chSysUnlock(); //Unlock kernel
return true; //All messages processed without error, so return true
}
chSysUnlock(); //Unlock kernel

msg_t buffer;
if(chMBFetch(&notificationInterface, &buffer, TIME_IMMEDIATE) != MSG_OK)
{
return false;
}

struct ultrasonicRangerEvent *messagePointer = (struct ultrasonicRangerEvent *) buffer;

//Use information from the message to update the ranges
struct ultrasonicRanger *ranger = messagePointer->ranger;

if(messagePointer->transitionedHigh)
{//This is the pulse start, so just store the start time
ranger->echoPulseStartTime = messagePointer->timestamp;
}
else
{ //This is the pulse end, so compute the range value and store it
ranger->currentRangeIndex--;
if(ranger->currentRangeIndex<0)
{ //Move backwards in a modulo way so modulo increasing index values point to increasingly less recent range values
ranger->currentRangeIndex = ULTRASONIC_RANGE_HISTORY_SIZE - 1;
}

ranger->ranges[ranger->currentRangeIndex] = ST2US(messagePointer->timestamp - ranger->echoPulseStartTime);  //Store elapsed time between echo pulse start and stop in microseconds
}

//Free the memory associated with the memory
chPoolFree(&messageMemoryPool, (void *) messagePointer);
}

return true;
}


/**
This function triggers each of the ranger sets sequentially.
@param inputUltrasonicRangerManager: A pointer to the ultrasonic ranger manager which contains the rangers to trigger
*/
void droneBumper::triggerThreadFunction(void *inputUltrasonicRangerManager)
{
while(true)
{
ultrasonicRangerManager *rangerManager = (ultrasonicRangerManager *) inputUltrasonicRangerManager;
struct ultrasonicRanger *currentSet = rangerManager->rangerSets[rangerManager->currentSetToTrigger];
int currentSetSize = rangerManager->rangerSetSizes[rangerManager->currentSetToTrigger];

if(rangerManager->triggerThreadShutdownFlag)
{
return; //Time for thread to shut down
}

//Trigger set
for(int i=0; i < currentSetSize; i++)
{
palSetPad(currentSet[i].triggerPinPort, currentSet[i].triggerPinNumber);
} 
chThdSleepMicroseconds(TRIGGER_ON_TIME); //Hold high for 1 millisecond
for(int i=0; i < currentSetSize; i++)
{
if(currentSet[i].triggerPinPort == GPIOA && currentSet[i].triggerPinNumber == 4)
{
palClearPad(GPIOC, GPIOC_LED_BLUE);
}
palClearPad(currentSet[i].triggerPinPort, currentSet[i].triggerPinNumber);
} 

//Increment to point to next set
rangerManager->currentSetToTrigger = (rangerManager->currentSetToTrigger+1) % rangerManager->numberOfRangerSets;

//Sleep until it is time to trigger the next set
chThdSleepMicroseconds(rangerManager->microsecondsBetweenSampling-TRIGGER_ON_TIME);
}
}

/*
This function is registered as the callback for external rising and falling event interrupts.  When it is called, it records the time, allocates memory  from the manager's thread pool for a message, initializes the message and sends a mailbox message with a pointer to the message.  The function checking the mailbox handles freeing the allocated memory.
@param inputDriver: The external interrupt driver
@param inputChannel: The channel associated with the current interrupt
*/
void droneBumper::echoPinInterruptCallback(EXTDriver *inputDriver, expchannel_t inputChannel)
{
(void ) inputDriver; //Get rid of warnings

palTogglePad(GPIOC, GPIOC_LED_BLUE);

//Get current time
systime_t eventTime = chVTGetSystemTimeX(); //Get time when this interrupt occurred


if(rangerManagerSingleton == nullptr || inputChannel >= MAXIMUM_NUMBER_OF_EXTERNAL_INTERRUPT_PINS)
{
return; //No ranger manager or invalid channel, so can't do anything
}



//Allocate memory for message
chSysLockFromISR(); //Establish kernel lock
struct ultrasonicRangerEvent *message = (struct ultrasonicRangerEvent *) chPoolAllocI((memory_pool_t *) &(rangerManagerSingleton->messageMemoryPool));
if(message == nullptr)
{
chSysUnlockFromISR(); //Release kernel lock
return; //Couldn't allocate memory for message, so can't do anything
}
chSysUnlockFromISR(); //Release kernel lock

//Initialize message
message->timestamp = eventTime;
message->ranger = rangerManagerSingleton->pinNumberToRanger[inputChannel];
message->transitionedHigh = palReadPad(message->ranger->echoPinPort, message->ranger->echoPinNumber); //Set true if high, false if low 

//Send pointer to message
chSysLockFromISR(); //Establish kernel lock
if(chMBPostI((mailbox_t *) &(rangerManagerSingleton->notificationInterface), (msg_t) message) != MSG_OK)
{
//Couldn't post message, so deallocate memory that was allocated for it
chPoolFreeI((memory_pool_t *) &(rangerManagerSingleton->messageMemoryPool), message);
//Don't forget to unlock if you return here
}
chSysUnlockFromISR(); //Release kernel lock


}



/**
This function cleans up the class unless isValid is set to false (in which case it does nothing).  Specifically, it shuts down the interrupts, stops the thread, releases the memory for the thread, mailbox and memory pool and sets the singleton pointer to nullptr.
*/
ultrasonicRangerManager::~ultrasonicRangerManager()
{
if(isValid == true)
{
extStop(&EXTD1); //disable external interrupt since configuration being deleted

//Need to signal thread to return and free the associated memory
triggerThreadShutdownFlag = true;
chThdWait(triggerThread); //Free memory allocated for thread

chHeapFree((void *) externalInterruptConfiguration);
chHeapFree(memoryForPool);
chMBReset(&notificationInterface);
chHeapFree((void *) memoryForMailbox);
}
}

