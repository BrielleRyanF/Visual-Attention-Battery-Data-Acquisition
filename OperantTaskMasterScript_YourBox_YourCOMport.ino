/*
 Operant Task Master Script
 By: Brielle Ferguson
 Task will test animals ability to remember cue location and respond 
 in the appropriate port to receive a reward.
Can be customized for training, and testing of different types based on the 
variables defined below.
Last update: 08/01/26 
*/

//here, define each of your arduino inputs and outputs. Each variable definition should correspond to which pin it's connected to on the arduino. 

//visual cue lights positioned in the reward ports (these LEDs are turned on during cue presentation)
#define leftRewardWhiteLED 7
#define rightRewardWhiteLED 11 
#define leftDistractorLED 5 //must change to available light pins
#define rightDistractorLED 9 //must change to available light pins

//sound cues (currently not called)
#define upsweepTTL 34 
#define downsweepTTL 32 
#define leftTone 34 
#define rightTone 32 

//house light to indicate trial availability and turning off for punishnent.
#define houseLight 13

//if there is an initial cue before the side cue, can define those here. 
#define GreenLED 38
#define BlueLED 40

//syringe pumps
#define leftRewardPort 44 
#define rightRewardPort 46

//IR Photodiode Receivers
#define IR_Photodioderight 2
#define IR_PhotodiodeCenter 18
#define IR_Photodiodeleft 19

//IR LEDs
#define IR_LEDright 10 //right
#define IR_LEDcenter 8 //Center
#define IR_LEDleft 6 //left

//ttl timestamp to record trial initiation in separate data acquisition software to time-lock other data with behavior
#define trialInitiationTTLFiP 32 //use for FiP
#define trialInitiationTTLEEG 30 //use for EEG 

//ttl connected to LED driver for delivering optogenetic stim
#define optoStimTTL 30 

//variables for controlling when doors open 
unsigned long midTrialstartTime;
unsigned long doorOpenTimer = 75000; //currently 75 seconds - after 75 seconds, doors open for 1 second

// create servo object to drive the servo motor which will open and close reward ports
#include <Servo.h>
int servoOpen = 00;
int servoClosed = 145;
Servo myservo;  
int servoPin = 49;

//variables that will be updated throughout the task
//ints to monitor the beam break status in each of the ports
int sensorStateleft = 0, sensorStateCenter = 0, sensorStateright = 0; 
int lastStateleft = 0, lastStateCenter = 0, lastStateright = 0;

//will keep track of the choices, trials, accuracy
int leftChoices = 0; 
int rightChoices = 0; 
int correctTrials = 0; 
int incorrectTrials = 0; 
int trialsCompleted = 0; 
int doorsOpened = 0;
String command; 
int side; 
int correctSide;
int trialModality;

//variables for perseveration tracking
int currentChoiceMade = 0;
int previousChoiceMade = 0;
int numSameSide = 0;
int persCount = 0;
int persWindow = 5;
float persScore = 0.0;
int twentyChoiceWarnings = 0;
int consecutiveSameSide = 0;
int lastStreakChoice = 0;
bool printedTwentyChoiceWarning = false;


// User can set up various session parameters. 

/*set times for various session parameters for training or testing*/ 
int cleanPortsDelay = 10000; //time (ms) ports are open to collect milk prior to the trial sequence beginning. When the arduino is reset it often sends a ttl pulse which will trigger a reward delivery before the task begins. 
int required = 200; //time (ms) mouse must hold its nose in the center poke to initiate a trial
int modalityCueLength = 1000; //length of modality cue, green or blue LED, currently task does not call this
int cueLength = 5000;//length (ms) of visual and auditory stim, can either set here (variableCue = false), or not define and use the sequence below (variableCue = true
int delayToCue = 000;//delay time (ms) between sample (visual or auditory stim) and reward port availability (if variableDelay = false sketch will use delay declared here, if variableDelay = true to will use sequence below)
int timeout = 15000; //timeout (ms) where a trial isn't available after animal makes an error
int rewardAvailable = 10000; //time (ms) animal has to consume reward until the reward ports close 
int numTrials = 50; //length of the trial sequences below  
unsigned long trialMinutes = 5; //(in minutes) after this amount of time is reached the serial monitor will display summary stats about the animal's performance. 
unsigned long totalSessionTime =  trialMinutes * 60000; //converts number of minutes into milliseconds
unsigned long startSession = millis(); //will record the current time, and compare to this after each trial to determine how much time has elapsed and whether to report summary stats
unsigned long endSession;  

/* change probability that the trial sequence will repeat */
int repeatProbability = 100;//number generated that will determine whether an incorrect choice results in a repeat of the previous trial if you did want to use a probability instead of always or never repeat. 
long repeatTrial; //random number generated that will determine whether an incorrect choice results in a repeat of the previous trial if you did want to use a probability instead of always or never repeat. 

/*define task mode and parameters for various session types */
int visualCueIntensity = 40; //0 is off and 250 is max brightness. Set to 150 for habituation, 100 for training and 15 for testing.
bool variableCue = false;//when true the cue length varies pseudorandomly from the cue length sequence, make true for 5CL or 3CL testing, false will use cueLength defined in block above   
bool variableDelay = false;//when true will use delayLength sequence, when false will use delayToCue defined in block above 
bool visualTrial = true; //if true, white LEDs serve as cues for the correct port to choose
bool auditoryTrial = false; //if true sounds (upsweep and downsweep) serve as cues for the correct port
bool distractorCues = false; //when true will use distractor sequence, and give matched or unmatched visual or auditory cues 
bool distractorVisualCues = true; //when distractorCues are true, will provide visual distractors
int visDistractorIntensity = 3; //0 is of f and 250 is max brightness. 
bool distractorAuditoryCues = false; // when distractorCues are set to true, will provide auditory distractors
int audDistractorIntensity = 5; //0 is off and 250 is max brightness. Set to 150 for habituation, 100 for training and 15 for testing.
bool matched = false; // when distractorCues set to true will update this value on each trial based on the matchSequence
bool crossmodalTrial = false; //set to true for crossmodal training and testing
bool checkConflict = false; //set to true for crossmodal testing
bool modalityVariable = false;
bool conflict = false; //if you're using crossmodal task, wif true will use this value, if false will use the conflict sequence
bool accuracy; // will update on each trial during training or testing

/* optogenetic stimulation parameters*/
bool optoStim = false; //if randomOptoStim and blockOptoStim are false, will use this value to always or never use opto stim
int pulselength = 5; //in ms
int optoFreq = 40; //in Hz 
int optoPulseDelay = 1000/optoFreq; //determine pulses per second based on optoFreq
bool optoStimPulse = false; //if optoStim is true, will send pulses using the pulseLength and optoFreq defined above
bool optoStimContinuous = true; //if optoStim is true, will use continuous opto stim
bool cueOptoStim = false; //opto during cue
bool responseOptoStim = false; //opto during response
bool delayOptoStim = false; //opto durung delay between trial initiation and the cue 
bool blockOptoStim = false; //if true will use sequence of blocks of 5 off 5 on opto stim trials
bool randomOptoStim = false; // if true will use the optoProbability defined below 
int optoProbability = 50; //if randomOptoStim = true, this represents the chance that the arduino will trigger opto stim on a given trial
long optoOn; //variable to save random number generated to determine optoStim if randomOptoStim = true

/*neuronal activity time stamps*/
bool sendTrialInitiationTTL = true; //true for sending ttl pulse to outisde data acquisition software, video, fiber photometry, open ephys (EEG, single-unit , etc.,) when a trial is initiated
int ttlLength = 200; 

//all trial, cue length, delay, modality, and opto stim sequences for various session types. Uncomment desired sequence below
/*Declare variables for all the different trial tjypes, see trialtype function below for more details on what each means*/ 
enum{training = 1, leftCue = 2, rightCue = 3, leftDMTP = 4,  rightDMTP = 5, leftDNMTP = 6, rightDNMTP = 7, cleanPorts = 8, habituation1 = 9, habituation2 = 10}; 
enum{visual = 11, auditory = 12};

/*Each sequence begins with a cleanPorts trial, as sending the sketch to the board, 
results in errant ttl pulses that trigger reward delivery. Experimenter can clean out the reward ports during cleanPortsDelay (declared above before placing animal in the chamber. */




/*TRAINING - for Habituation 2 or TI Training define correct trial sequence*/

int trialSequence[] = {habituation2}; //Habituation 2   lights flash for cueLength defined above (use 5s), doors open, and then will wait for animal to make either choice
// int trialSequence[] = {training}; //TI Training   waits for a nose poke in the center port, lights flash for cueLength defined above (use 5s), doors open and will wait for animal to make either choice


/*TRAINING - ALL OTHER TRAINING PROGRAMS USE THIS TRIAL SEQUENCE - 50 trial pseudorandom seqeunce of correct sides, designed to be counterbalanced equally for both variable cue lengths and delay lengths*/

// int trialSequence[]  ={rightCue, leftCue, leftCue, rightCue, rightCue, leftCue, leftCue, rightCue, leftCue, rightCue, leftCue, rightCue, leftCue, leftCue,
// leftCue, rightCue, rightCue, rightCue, leftCue, rightCue, leftCue, rightCue, leftCue, leftCue, leftCue, rightCue, rightCue, rightCue, leftCue, rightCue, 
// rightCue, leftCue, rightCue, leftCue, rightCue, leftCue, rightCue, leftCue, rightCue, leftCue, leftCue, leftCue, rightCue, rightCue, leftCue, rightCue, 
// rightCue, leftCue, leftCue, rightCue};




/*for 4CL TESTING define the trial sequence and cue length sequence*/

/*DAY 1 - pseudorandom seqeunce of 104 correct sides, designed ot be counterbalanced equally for variableCue testing with 4 cue lengths*/
//Trial sequence for - 4CL DAY 1

// int trialSequence[] = {3,  3,  3,  2, 2, 2,  3,  2,  3,  2,  3,  3,  2,  3,  2,  2,  2,  2,  3,  3, 2,  3,  2,  3,  3,  3,  2,  3,  2,  2,
//  2,  3,  2,  2,  3,  3,  2,  3,  2,  3,  3,  2,  2,  3,  3,  2,  2,  3, 2, 2,  3,  2,  3,  2,  3,  3,  3,  2, 2, 2,  3,  3,  3,  2,  2,
//  3,  2,  3,  3, 2, 3,  2,  3,  2,  3,  2,  2,  3,  3,  2,  3,  2,  2,  2,  3,  3,  2,  3,  2,  3,  2,  2,  2,  3,  3,  3,  2,  2,  3,
//  2,  3,  3,  2,  3};

//*DAY 1 - Pseudorandom sequence of 104 cue lengths to be used for the variableCue version of task with four cue lengths 2000, 1000, 500, and 50*/ 
//Cue length sequence for - 4CL DAY 1

// int cueLengthSequence[] = {2000,  50,  500,  2000, 50,  500,  1000, 1000, 500,  2000, 50,  2000, 500,  1000, 1000, 50, 500, 2000, 2000,
// 500,  50,  1000, 1000, 50,  2000, 1000, 2000, 500,  500,  1000, 50,  50,  1000, 2000, 50,  2000, 500,  500, 50,  1000, 1000, 50,  500,  500,
// 50,  2000, 1000, 2000, 1000, 50,  500,  2000, 50,  500,  2000, 1000, 50,  2000, 50,  500, 1000, 2000, 500,  1000, 500,  500,  1000, 1000, 
// 2000, 2000, 50,  50,  1000, 50,  50,  2000, 1000, 500,  2000, 500,  50,  2000, 1000, 50,  500,  2000, 500,  1000, 50,  500,  500,  1000, 
// 2000, 50,  2000, 1000, 2000, 1000, 2000, 500, 500, 1000, 50, 1000};


/*DAY 2 - pseudorandom seqeunce of 104 correct sides, designed ot be counterbalanced equally for variableCue testing with 4 cue lengths*/
//Trial sequence for - 4CL DAY 2

// int trialSequence[] = {3, 2, 2, 3, 2, 3, 3, 2, 3, 2, 3, 3, 2, 2, 3, 2, 3, 3, 3, 2, 2, 2, 3, 2, 2, 3, 3, 3, 2, 3, 2, 2, 2, 2, 2,
// 3, 3, 3, 3, 2, 3, 2, 3, 3, 2, 2, 3, 2, 3, 3, 3, 2, 2, 3, 2, 2, 2, 2, 3, 3, 3, 2, 2, 3, 2, 3, 3, 2, 2, 3, 2, 3, 3, 3, 2, 3, 2, 2, 2, 3, 2, 
// 2, 3, 2, 3, 3, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 3, 2, 2, 2, 3, 3, 2};

//*DAY 2 - Pseudorandom sequence of 104 cue lengths to be used for the variableCue version of task with four cue lengths 2000, 1000, 500, and 50*/ 
//Cue length sequence for - 4CL DAY 2


// int cueLengthSequence[] = {500, 500, 1000,  1000,  2000,  2000,  50, 50, 1000,  2000,  50, 2000,  50, 1000,  500, 500, 2000,  500, 1000,500, 2000,  50, 50, 1000,  1000,  500, 2000,  50, 500, 1000,  2000,  50, 500, 2000,  50, 500, 50, 2000,  1000,  1000,1000,  2000,  2000, 
// 50, 50, 1000,  500, 500, 1000,  50, 2000,  1000,  2000,  500, 50, 500, 1000,  2000,  50, 2000,  500, 500, 50,1000,  50, 2000,  500, 1000, 
// 500, 1000,  2000,  50, 2000,  50, 500, 1000,  2000,  50, 1000,  500, 1000,  500, 1000,  2000,  2000,  50, 500, 50, 2000,  50, 1000,  2000,  
// 500, 500, 50, 1000,  1000,  500, 1000,  500, 2000,  2000,  50, 1000};

/*DAY 3 - pseudorandom seqeunce of 104 correct sides, designed ot be counterbalanced equally for variableCue testing with 4 cue lengths*/
//Trial sequence for - 4CL DAY 3

// int trialSequence[] = {2, 3, 2, 2, 3, 2, 3, 3, 2, 3, 2, 3, 2, 2, 3, 3, 2, 3, 3, 3, 3, 2, 2, 2, 2, 3, 2, 2, 3, 3, 3,
// 2, 3, 2, 3, 2, 2, 3, 3, 2, 3, 2, 3, 3, 3, 2, 2, 2, 3, 2, 3, 2, 3, 2, 2, 3, 3, 3, 2, 3, 3, 2, 2, 2, 2, 3, 2, 3, 3, 2, 2, 3, 2, 3,
// 3, 3, 2, 2, 2, 3, 3, 2, 3, 3, 2, 3, 2, 2, 3, 3, 2, 2, 3, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 3};

//*DAY 3 - Pseudorandom sequence of 104 cue lengths to be used for the variableCue version of task with four cue lengths 2000, 1000, 500, and 50*/ 
//Cue length sequence for - 4CL DAY 3

int cueLengthSequence[] ={50,  2000,  1000,  500, 500, 2000,  50,  1000,  50,  500, 1000, 50,  2000,  500, 2000,  1000,  2000,  2000, 50,  500, 1000,  500, 1000,  50,  1000,  2000,
2000,  500, 1000,  50,  500, 50,  1000,  1000,  500, 500, 50,  2000,  50,  2000,  1000,  500, 2000,  500, 50,  1000,  2000,  50,  2000,  1000, 
50,  50,  1000,  500, 2000,  500, 50,  500, 2000,  1000,  2000,  500, 1000,  50,  500, 2000,  2000,  50,  500, 50,  1000,  1000,  500, 2000,
50 , 500, 50,  1000,  2000,  1000,  50,  50,  1000,  500, 1000,  2000,  2000,  500, 500, 50,  1000,  50,  2000,  500, 1000,  2000,  50,  500,
1000,  500, 2000,  50,  2000,  1000,};




/*for variableDelay TESTING (VDST) define the trial sequence and cue length sequence*/
//*VDST - variable delay to sample task side sequences (trialSequence) and delay lenghth pairs (variableDelayLengthSequence) for each day of vdst testing

//*DAY 1 - Pseudorandom sequence of correct sides to be used for the variableDelay version of the task with three cue lenghts - 3, 4, and 5s */
//Trial sequence for - VDST DAY 1

// int trialSequence[] = {3,2,2,3,3,2,2,2,2,3,3,3,3,2,2,3,2,3,2,2,3,3,2,3,3,2,2,2,3,3,3,3,2,2,3,2,2,2,2,3,3,3,2,3,2,3,3,2,3,3,2,3,2,2,3,2,3,2,3,
// 2,2,3,2,3,2,3,2,3,2,3,3,2,3,2,3,2,3,2,3,2,3,2,2,3,3,2,2,3,2,3,2,3,2,2,3,3};

//DAY 1 - Pseudorandom sequence of delay lengths to be used for the variableDelay version of the task with three cue lenghts - 3, 4, and 5s */
//Variable delay sequence for - VDST DAY 1

// int variableDelayLengthSequence[] = {5000,4000,5000,3000,4000,3000,5000,4000,3000,3000,4000,5000,4000,3000,4000,3000,5000,5000,4000,5000,3000,5000,3000,4000,4000,
// 3000,4000,5000,5000,3000,3000,4000,3000,4000,5000,5000,4000,5000,3000,3000,4000,5000,4000,3000,3000,5000,4000,5000,3000,4000,4000,5000,3000,5000,4000,3000,5000,
// 5000,3000,4000,4000,5000,3000,4000,5000,3000,4000,5000,3000,3000,4000,5000,3000,3000,5000,5000,4000,4000,5000,4000,3000,5000,3000,4000,4000,3000,4000,3000,5000,
// 5000,5000,5000,3000,4000,4000,3000};


//*DAY 2 - Pseudorandom sequence of correct sides to be used for the variableDelay version of the task with three cue lenghts - 3, 4, and 5s */
//Trial sequence for - VDST DAY 2

// int trialSequence[] = {2,2,3,3,2,3,3,2,2,3,3,2,3,3,2,2,2,3,2,3,2,3,3,2,2,3,3,2,3,2,2,2,3,3,2,3,3,2,2,3,3,2,3,2,2,3,2,3,2,3,3,3,2,2,3,3,2,2,2,3,3,3,3,2,2,
// 2,3,2,2,3,3,2,3,2,2,3,2,3,3,2,3,2,2,3,3,2,3,2,2,3,2,3,2,3,3,2};

//DAY 2 - Pseudorandom sequence of delay lengths to be used for the variableDelay version of the task with three cue lenghts - 3, 4, and 5s */
//Variable delay sequence for - VDST DAY 2

int variableDelayLengthSequence[] = {3000,4000,3000,5000,5000,4000,3000,4000,3000,5000,4000,5000,3000,5000,3000,4000,5000,4000,3000,3000,5000,4000,5000,4000,3000,
3000,5000,4000,4000,5000,5000,4000,5000,4000,3000,3000,3000,3000,5000,5000,4000,4000,4000,5000,4000,3000,3000,5000,3000,5000,3000,4000,5000,4000,3000,4000,5000
,3000,4000,5000,5000,4000,3000,3000,4000,5000,3000,4000,5000,5000,4000,3000,5000,5000,3000,4000,4000,3000,3000,4000,4000,3000,5000,5000,4000,5000,3000,4000,3000,
5000,5000,5000,3000,3000,4000,4000};




// DT TRAINING vis intensity sequence -- keep the distractor intensity the same
//Vis intensity for DT training

// int visIntensitySequence[] = {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
// 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
// 5,5,5,5,5,5,5,5,5,5,5,5,5,5};


/*for DT TESTING, define the trial sequence, vis intensity sequence (for distractor), and match sequence (side of distractor) below DAY MUST BE CONSISTENT ACROSS THE 3*/

/*DAY 1 - Pseudorandom sequence of correct sides to be used for the distractorCues version of the task with three intensities - 0, 15, and 45 */
//Trial sequence for DT DAY 1

// int trialSequence[] = {2,2,3,2,3,3,2,2,3,3,3,2,3,2,2,3,3,2,2,2,3,2,2,2,3,3,2,3,2,3,3,3,2,2,3,3,3,2,3,2,2,2,3,3,3,2,2,3,3,3,2,2,3,3,3,2,3,3,2,
// 2,3,2,2,2,3,2,3,3,3,2,2,3,3,2,2,2,3,2,2,3,2,2,2,3,2,3,2,2,3,3,2,3,3,3,2,3,2,2,3,2,3,2,3,2,3,2,2,3,2,3,3,3};

//*DAY 1 - Pseudorandom sequence of visual cue intensities*/
//max 45, starting at 0
//Vis intensity sequence for DT DAY 1

// int visIntensitySequence[] = {45,0,15,45,5,0,15,5,45,0,5,15,45,5,0,15,0,5,45,5,15,0,0,45,5,15,15,45,15,45,5,0,5,45,45,0,45,45,0,15,5,15,5,
// 15,5,0,0,15,0,15,15,15,45,5,45,5,5,15,0,45,0,5,0,45,5,45,5,45,15,5,0,15,45,15,15,5,0,0,45,0,5,45,5,0,0,5,0,45,0,45,15,15,15,5,15,45,5,45,
// 5,0,45,15,0,15,5,45,5,15,0,0,15,45};

//*DAY 1 - Pseudorandom match sequence to be paired with side sequence directly above*/
//Match sequence for DT DAY 1 AND FOR DT TRAINING

// bool matchSequence[] = {1,0,0,0,1,0,0,1,0,1,0,1,1,0,1,1,0,1,0,0,1,0,1,1,0,0,0,1,1,0,1,1,0,1,1,0,0,0,1,0,1,1,1,0,0,1,0,1,0,0,0,1,0,1,1,1,0,
// 1,1,0,1,0,0,1,0,0,1,0,1,1,1,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,0,1,1,0,0,1,0,1,1,1,0,0,1,1,0,0,1,1,0,1,1,1,0,0,0,1};


//*DAY 2 - Pseudorandom sequence of correct sides to be used for the distractorCues version of the task with three intensities - 0, 15, and 45*/
//Trial sequence for DT DAY 2

// int trialSequence[] = {2,2,3,2,3,2,2,3,3,2,3,3,3,2,2,3,2,2,3,3,2,2,3,2,3,2,2,3,2,3,3,3,2,2,2,3,2,2,2,3,3,2,3,3,3,2,3,3,3,2,3,2,2,3,2,2,3,2,
// 2,3,3,2,3,3,2,3,2,3,2,3,2,2,3,3,2,3,3,2,2,3,3,3,2,2,3,2,2,3,2,3,3,3,2,2,3,2,3,3,2,2,3,3,3,2,3,3,2,2,3,2,2,2};

//*DAY 2 - Pseudorandom sequence of visual cue intensities*/
//max 45, starting at 0
//Vis intensity sequence for DT DAY 2

// int visIntensitySequence[] = {45,0,15,45,5,0,15,5,45,0,5,15,45,5,0,15,0,5,45,5,15,0,0,45,5,15,15,45,15,45,5,0,5,45,45,0,45,45,0,15,5,15,5,
// 15,5,0,0,15,0,15,15,15,45,5,45,5,5,15,0,45,0,5,0,45,5,45,5,45,15,5,0,15,45,15,15,5,0,0,45,0,5,45,5,0,0,5,0,45,0,45,15,15,15,5,15,45,5,45,
// 5,0,45,15,0,15,5,45,5,15,0,0,15,45};

//*DAY 2 - Pseudorandom match sequence to be paired with side sequence directly above*/
//Match sequence for DT DAY 2

// bool matchSequence[] = {0,1,0,1,1,0,0,0,1,1,0,0,1,1,0,1,0,1,1,1,0,1,1,0,0,1,1,0,0,0,1,0,0,0,1,0,1,1,1,0,0,0,1,1,1,0,1,0,1,0,0,0,1,1,0,0,1,1,
// 1,0,0,1,0,1,1,0,1,1,0,0,1,0,1,0,0,1,0,1,0,1,1,0,0,0,1,1,0,1,0,1,0,0,1,1,0,1,1,1,0,1,0,1,0,0,1,0,1,0,0,1,0,1};


//*DAY 3 - Pseudorandom sequence of correct sides to be used for the distractorCues version of the task with three intensities - 0, 15, and 45*/
//max 45
//Trial sequence for DT DAY 3

//  int trialSequence[] = {3,2,3,3,2,3,2,2,2,3,3,2,3,2,2,3,2,2,3,3,3,2,3,2,3,2,3,2,3,2,2,3,3,2,2,3,2,3,3,2,2,3,3,2,2,3,3,2,2,3,2,2,3,2,3,3,2,3,2,2,
//  3,3,2,3,3,2,2,3,3,2,3,2,2,2,3,3,2,3,3,2,3,3,2,2,3,2,2,2,3,2,3,3,3,2,2,3,3,2,3,2,3,3,3,2,2,2,3,3,3,2,2,2};

//*DAY 3 - Pseudorandom sequence of visual cue intensities*/
//max 45, starting at 0
//Vis intensity sequence for DT DAY 3

 int visIntensitySequence[] = {5,5,5,45,0,45,15,0,5,0,0,15,15,45,45,15,45,45,5,45,45,5,5,0,15,0,0,15,15,15,5,0,15,0,45,45,45,5,5,15,5,0,15,5,0,
 45,0,15,15,15,5,15,45,0,45,5,0,0,45,45,5,15,5,0,45,15,45,15,0,0,0,45,5,0,15,5,5,45,5,15,15,0,0,45,45,45,0,5,15,15,5,5,0,5,15,45,5,15,0,45,15,
 45,5,15,5,5,0,45,15,0,45,0};

//*DAY 3 - Pseudorandom match sequence to be paired with side sequence directly above*/
//Match sequence for DT DAY 3

 bool matchSequence[] = {0,1,1,0,1,1,1,0,0,0,1,0,0,1,0,1,1,0,1,0,1,1,0,1,1,0,0,1,0,0,0,1,0,0,0,1,1,0,1,0,1,0,1,0,1,0,1,1,0,1,0,1,0,0,1,0,1,0,1,
 0,1,0,1,1,0,1,1,0,1,0,0,0,1,1,1,0,0,1,1,0,0,1,0,0,0,1,1,0,1,1,1,0,0,1,0,1,0,0,1,1,1,0,1,1,1,0,0,1,0,1,0,0};

 int audIntensitySequence[] = {10};




/*for DT-VDST TESTING, define the trial sequence, vis intensity sequence (for distractor), and match sequence (side of distractor) below DAY MUST BE CONSISTENT ACROSS THE 3*/
/*DAY 1 - Pseudorandom sequence of correct sides*/
//Trial sequence for DT-VDST DAY 1

// int trialSequence[] = {2,3,3,3,2,3,2,2,3,3,2,2,2,3,3,2,3,2,3,3,3,2,2,2,3,2,3,3,2,2,3,2,2,3,3,2,2,3,3,2,2,3,2,2,2,3,3,3,2,2,3,3,3,2,3,2,3,3,
// 2,3,2,2,2,3,3,2,2,3,2,3,2,3,2,2,3,2,2,3,3,3};

/*DAY 1 - Pseudorandom sequence of visual cue intensities*/
//alternating 0 and 5 
//Vis intensity sequence for DT-VDST DAY 1 (AND FOR DT-VDST TRAINING (confirming with brielle, could also be the 5,5,5 sequence from DT TRAINING))

// int visIntensitySequence[] = {0,0,0,5,5,5,0,5,5,0,0,5,0,5,0,5,5,5,0,0,5,0,5,0,5,0,0,5,5,0,0,5,5,0,5,0,5,0,5,0,5,5,0,0,5,0,5,0,0,5,5,5,0,0,0,
// 5,5,5,0,0,5,5,0,0,5,0,0,0,5,0,5,5,5,0,5,5,0,0,0,5};

//*DAY 1 - Pseudorandom match sequence to be paired with side sequence directly above*/
//Match sequence for DT-VDST DAY 1 AND FOR DT-VDST TRAINING

// bool matchSequence[] = {0,0,1,1,1,0,1,0,1,0,1,0,0,0,1,1,0,1,0,1,1,0,0,1,1,1,0,0,1,0,1,0,0,0,1,0,1,1,0,1,1,0,0,1,0,1,1,0,1,1,1,0,1,0,0,0,0,1,
// 0,1,0,1,1,0,1,0,1,0,1,1,0,0,1,1,1,0,0,0,1,0};


/*DAY 2 - Pseudorandom sequence of correct sides*/
//Trial sequence for DT-VDST DAY 2

// int trialSequence[] = {3,2,2,3,2,2,3,3,2,3,3,2,3,2,3,2,2,3,2,2,3,3,2,3,3,2,3,3,2,2,3,2,2,3,3,3,2,2,2,3,2,3,3,3,2,3,2,2,3,3,2,3,2,3,2,2,3,2,
// 3,2,3,3,2,2,3,2,3,2,3,2,2,3,3,2,2,3,2,3,2,3};

//*DAY 2 - Pseudorandom sequence of visual cue intensities*/
//alternating 0 and 5 
//Vis intensity sequence for DT-VDST DAY 2

// int visIntensitySequence[] = {0,5,0,5,0,5,0,5,0,0,5,0,5,5,0,5,5,5,0,5,0,5,0,0,0,0,5,5,5,0,0,5,5,5,5,0,0,5,0,0,0,5,0,5,0,0,5,5,0,0,5,5,0,5,5,
// 0,0,0,5,0,0,5,5,5,0,5,5,0,5,5,0,0,0,0,5,0,5,5,0,5};

//*DAY 2 - Pseudorandom match sequence to be paired with side sequence directly above*/
//Match sequence for DT-VDST DAY 2

// bool matchSequence[] = {1,1,0,0,1,0,0,1,1,1,0,0,1,0,0,1,0,1,0,1,0,0,1,1,0,1,1,0,1,0,1,0,0,1,0,0,1,1,0,1,1,1,0,0,0,1,1,0,0,1,1,0,0,1,0,1,0,1,
// 0,0,1,1,1,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,0,1};




/*SCL (single cue lenth) CONTROL EXPERIMENT TO BE RUN AT END*/

/*Trial sequence contains single cue length trial blocks - 13 blocks of 8 pseudorandom sides*/
//Trial sequence for BOTH DAYS of SCL

// int trialSequence[] = {3, 2, 3, 2, 2, 3, 2, 3, 3, 2, 3, 3, 2, 2, 2, 2, 3, 3, 2, 2, 2, 3, 3, 2, 3, 3, 2, 2, 2, 3, 3, 3, 2, 3, 3, 2, 2, 2, 3, 2, 2, 3, 3, 2, 3, 3, 2, 
// 2, 3, 3, 2, 3, 2, 3, 2, 3, 3, 2, 2, 3, 3, 2, 2, 2, 3, 2, 2, 2, 3, 3, 3, 3, 2, 3, 2, 3, 2, 3, 2, 3, 3, 2, 3, 2, 2, 3, 2, 2, 3, 2, 3, 2, 3, 3, 2, 3, 2, 3, 2, 3, 2, 2, 3};

/*The corresponding cue length sequence, descending from 2000, 1000, 500, and 50 in blocks of 8*/
//Cue length sequence for BOTH DAYS of SCL 

// int cueLengthSequence[] = {2000, 2000,  2000,  2000,  2000,  2000,  2000,  2000,  1000,  1000,  1000,  1000,  1000,  1000,  1000,  1000,  500, 500,
// 500, 500, 500, 500, 500, 500, 50,  50,  50,  50,  50,  50,  50,  50, 2000, 2000,  2000,  2000,  2000,  2000,  2000,  2000,  1000,  1000,  1000,  1000,  
// 1000,  1000,  1000,  1000,  500, 500, 500, 500, 500, 500, 500, 500, 50,  50,  50,  50,  50,  50,  50,  50, 2000, 2000,  2000,  2000,  2000,  2000,
// 2000,  2000,  1000,  1000,  1000,  1000,  1000,  1000,  1000,  1000,  500, 500,500, 500, 500, 500, 500, 500, 50,  50,  50,  50,  50,  50,  50,  50, };




int trialModalitySequence[] = {1};

/*Blocks of 5 off 5 on trials for optogenetic stim, will be used if blockOptoStim = true*/
bool optoStimSequence[] = {false, false, false, false, false, true, true, true, true, true, false, false, false, false, false, true, true, true, true, true, false, false, false, false, false, 
true, true, true, true, true, false, false, false, false, false, true, true, true, true, true, false, false, false, false, false, true, true, true, true, true};

/*Blocks for training in crossmodal task if conflict is set to true*/
bool conflictSequence[] = {true, false, true, true, true, false, false, false, false, false,true, true, true, true, true, false, false, false, false, false, true, true, true, true, true, false, false, false, false, false,
true, true, true, true, true, false, false, false, false, false, true, true, true, true, true, false, false, false, false, false}; 



/*initialize all pins defined above as inputs, outputs, and turn them on if on throughout the task. Will initialize the serial monitor to stream all data to arduino serial monitor or matlab*/
void setup() {
  myservo.attach(servoPin);  // attaches the servo on pin number declared above to the servo object
  closePokes();
  pinMode(leftRewardPort, OUTPUT); 
  pinMode(rightRewardPort, OUTPUT); 
  pinMode(houseLight, OUTPUT);
  pinMode(GreenLED, OUTPUT); 
  pinMode(BlueLED, OUTPUT); 
  pinMode(leftRewardWhiteLED, OUTPUT);
  pinMode(rightRewardWhiteLED, OUTPUT);
  pinMode(leftDistractorLED, OUTPUT);
  pinMode(rightDistractorLED, OUTPUT);
  pinMode(upsweepTTL, OUTPUT);
  pinMode(downsweepTTL, OUTPUT); 
  digitalWrite(upsweepTTL, HIGH); 
  digitalWrite(downsweepTTL, HIGH); 
  pinMode(leftTone, OUTPUT);
  pinMode(rightTone, OUTPUT); 
  pinMode(IR_Photodioderight, INPUT);  
  pinMode(IR_PhotodiodeCenter, INPUT);  
  pinMode(IR_Photodiodeleft, INPUT); 
  digitalWrite(IR_Photodioderight, HIGH);   
  digitalWrite(IR_PhotodiodeCenter, HIGH);
  digitalWrite(IR_Photodiodeleft, HIGH); 
  pinMode(IR_LEDright, OUTPUT);  
  pinMode(IR_LEDcenter, OUTPUT); 
  pinMode(IR_LEDleft, OUTPUT); 
  digitalWrite(IR_LEDright, HIGH);   
  digitalWrite(IR_LEDcenter, HIGH);
  digitalWrite(IR_LEDleft, HIGH);
  pinMode(trialInitiationTTLFiP,OUTPUT);
  pinMode(trialInitiationTTLEEG,OUTPUT);
  digitalWrite(trialInitiationTTLFiP,LOW);
   digitalWrite(trialInitiationTTLEEG,LOW);
  pinMode(optoStimTTL,OUTPUT);
  digitalWrite(optoStimTTL,LOW); 
  randomSeed(analogRead(0));
  Serial.begin(115200); //turns on the serial monitor 
  Serial.println("Begin Session");
  Serial.println("Clean Ports"); 
  myservo.write(servoOpen); 
  intertrialInterval(cleanPortsDelay); //gives user time to clean out ports
  myservo.write(servoClosed);
}

//below, all functions are defined that will be used throughout the task to be called during the main function loop

enum {left = 1, right = 2, both = 3};

void updatePerseveration(int choice) {
    currentChoiceMade = choice;

    // Track true consecutive-choice streak
    if (choice == lastStreakChoice) {
        consecutiveSameSide++;
    } else {
        consecutiveSameSide = 1;
        lastStreakChoice = choice;
        printedTwentyChoiceWarning = false;
    }

    if (consecutiveSameSide == 20 && !printedTwentyChoiceWarning) {
        twentyChoiceWarnings++;
        printedTwentyChoiceWarning = true;
    }

        // First choice for perseveration calculation
        if (previousChoiceMade == 0) {
            previousChoiceMade = currentChoiceMade;
            return;
        }

    // Sliding-window perseveration calculation
    if (currentChoiceMade == previousChoiceMade) {
        numSameSide++;
    } else {
        numSameSide = 0;
    }

    if (numSameSide == persWindow) {
        persCount++;
        numSameSide = persWindow - 1;
    }

    previousChoiceMade = currentChoiceMade;

    if (trialsCompleted > 0) {
        persScore = (float)persCount / trialsCompleted;
    }
}

/*delay between trials*/
void intratrialInterval (int delayLength) {
  delay(delayLength);
}

/*delay during trials*/
void intertrialInterval (int delayLength) {
  delay(delayLength);
}

/*array to be used in switch case statements for everything where function passes in a side or both sides*/

/*will dispense a reward to int side (left,right, or both) by sending a 1s ttl pulse to a syringe pump*/
void dispenseReward (int side) {
  switch (side) {
    case left: 
      Serial.println("Dispensing reward left");
      digitalWrite(leftRewardPort, HIGH); //left reward delivery by activating left syringe pump 
      delay(1000);  
      digitalWrite(leftRewardPort, LOW);
    break; 
    case right: 
      Serial.println("Dispensing reward right");
      digitalWrite(rightRewardPort, HIGH); //right reward delivery by activating left syringe pump 
      delay(1000);  
      digitalWrite(rightRewardPort, LOW);
    break; 
    case both: 
      Serial.println("Dispensing reward to both ports");
      digitalWrite(leftRewardPort, HIGH); //left reward delivery by activating left syringe pump 
      digitalWrite(rightRewardPort, HIGH); //right reward delivery by activating left syringe pump
      delay(1000);   
      digitalWrite(leftRewardPort, LOW);
      digitalWrite(rightRewardPort, LOW);
    break;
    
  }
}

/*use to send commands to change operant task parameters during a session, can only be used if viewing session through arduino's serial monitor, not in MATLAB*/
void executeUserCommand(String command) {
        if(command.equals("dispense reward left")){
           Serial.print("Executing User Command - "); 
           dispenseReward(left);  
        }
        else if (command.equals("dispense reward right")){
          Serial.print("Executing User Command - "); 
          dispenseReward(right); 
        } 
        else if (command.equals("deactivate right reward port")){
           Serial.println("Executing User Command - Deactivating Right Reward Port"); 
           digitalWrite(IR_LEDright, LOW);   
        }
        else if (command.equals("deactivate left reward port")){
           Serial.println("Executing User Command - Deactivating Left Reward Port"); 
           digitalWrite(IR_LEDleft, LOW);    
        }
        else if (command.equals("activate right reward port")){
           Serial.println("Executing User Command - Activating Right Reward Port"); 
           digitalWrite(IR_LEDright, HIGH);  
        }
        else if (command.equals("activate left reward port")){
           Serial.println("Executing User Command - Activating Left Reward Port"); 
           digitalWrite(IR_LEDleft, HIGH);   
        }
        else if (command.equals("open reward ports")){
           Serial.println("Executing User Command - Opening Reward Ports"); 
           myservo.write(servoOpen);  
        }
        else if (command.equals("close reward ports")){
           Serial.println("Executing User Command - Closing Reward Ports"); 
           closePokes();    
        }
        else if (command.equals("repeat if incorrect")){
           Serial.println("Executing User Command - Repeat Probability = 100%"); 
           repeatProbability = 100;    
        }
        else if (command.equals("do not repeat if incorrect")){
           Serial.println("Executing User Command - Repeat Probability = 0"); 
           repeatProbability = 0;    
        }
        else if (command.equals("no opto stim")){
           Serial.println("Optogenetic Stim turned off"); 
           randomOptoStim = false; 
           blockOptoStim = false;   
           optoStim = false;  
        }
         else if (command.equals("optoFreq = 10")){
           Serial.println("Executing User Command - Updating Opto frequency to 10 Hz "); 
           optoFreq = 10;
           optoPulseDelay = 1000/optoFreq;
        }
         else if (command.equals("optoFreq = 20")){
           Serial.println("Executing User Command - Updating Opto frequency to 20 Hz "); 
           optoFreq = 20;
           optoPulseDelay = 1000/optoFreq;
        }
         else if (command.equals("optoFreq = 40")){
           Serial.println("Executing User Command - Updating Opto frequency to 40 Hz "); 
           optoFreq = 40;
           optoPulseDelay = 1000/optoFreq;
        }     
         else if (command.equals("variableCue = false")){
           Serial.println("Executing User Command - Cue length now constant "); 
           variableCue = false;
        }     
           else if (command.equals("variableDelay = false")){
           Serial.println("Executing User Command - Cue length now constant "); 
           variableDelay = false;
        }  
        else{
            Serial.println("Invalid command");
        }  
}

/*will monitor the state of all ports and continously report those states, until the delay
"int d" has passed. */
void checkBeamBreakForDelay(unsigned long d) {
  unsigned long startTime = millis(); 
  unsigned long endTime = startTime; 
  while ((endTime - startTime) <= d) 
    { 
      sensorStateleft = digitalRead(IR_Photodiodeleft);
      sensorStateCenter = digitalRead(IR_PhotodiodeCenter);
      sensorStateright = digitalRead(IR_Photodioderight);

      if (sensorStateleft && !lastStateleft) {
        Serial.println("Left Port Entry");

        leftChoices = leftChoices + 1; 
      }
      lastStateleft = sensorStateleft;

      if (sensorStateCenter && !lastStateCenter) {
        Serial.println("Center Port Entry");
      }
      lastStateCenter = sensorStateCenter;

      if (sensorStateright && !lastStateright) {
        Serial.println("Right Port Entry");

        rightChoices = rightChoices + 1; 
      }
      lastStateright = sensorStateright;
      endTime = millis(); 
   }
}


/* Continuously monitors the state of each port for entry, if entry is either left or right, 
will report entry, and then report the status of all ports for 20 seconds while animal can freely
collect rewards. If entry is center, will report and keep looping until a left or right choice is made.*/ 
void checkBeamBreakForChoice() { 
  bool beamBreakStatus = false; 
  while (beamBreakStatus == false) 
    {
      sensorStateleft = digitalRead(IR_Photodiodeleft);
      sensorStateCenter = digitalRead(IR_PhotodiodeCenter);
      sensorStateright = digitalRead(IR_Photodioderight);

      if (sensorStateleft && !lastStateleft) {
        Serial.println("Left Port Entry First");
        checkBeamBreakForDelay(10000); 
        beamBreakStatus = true; 
        leftChoices = leftChoices + 1; 
      }
      lastStateleft = sensorStateleft;

     
      if (sensorStateright && !lastStateright) {
        Serial.println("Right Port Entry First");
        checkBeamBreakForDelay(10000); 
        beamBreakStatus = true; 
        rightChoices = rightChoices + 1; 
      }
      lastStateright = sensorStateright;

      if (sensorStateCenter && !lastStateCenter) {
        Serial.println("Center Port Entry");
      }
      lastStateCenter = sensorStateCenter;
    }
}

/*continuously montors the state of all ports until the a choice is made,
then will exit the loop. Once choice is made, will dispense reward into the port that was entered. */
void waitAnyChoice() {
  bool choiceMade = false; 
  while (choiceMade == false) 
    {
      sensorStateleft = digitalRead(IR_Photodiodeleft);
      sensorStateCenter = digitalRead(IR_PhotodiodeCenter);
      sensorStateright = digitalRead(IR_Photodioderight);

      if (sensorStateleft && !lastStateleft) {
        Serial.print("Left Port Entry - ");
        choiceMade = true;
        updatePerseveration(left);
        leftChoices = leftChoices + 1; 
        dispenseReward(left); 
        intratrialInterval(15000);
      }
      lastStateleft = sensorStateleft;
     
      if (sensorStateright && !lastStateright) {
        Serial.print("Right Port Entry - ");
        updatePerseveration(right);
        choiceMade = true; 
        rightChoices = rightChoices + 1; 
        dispenseReward(right);
        intertrialInterval(15000);
      }
      lastStateright = sensorStateright;

      if (sensorStateCenter && !lastStateCenter) {
        Serial.println("Center Port Entry");
      }
      lastStateCenter = sensorStateCenter;
    }
} 

/*continuously monitors the state of all ports until the correct choice is made, will dispense reward and
then will exit the loop. If incorrect choice is made will report the choice and that
it was incorrect, and keep waiting for the correct choice. If the correct choice was made, 
will report the choice, and that it was correct, dispense reward and then exit the loop.*/
void waitCorrectChoice(int correctChoice) {
  bool correctChoiceMade = false; 
  bool beamBreakleft = false;
  bool beamBreakright = false;
  while (correctChoiceMade == false) 
    {
      sensorStateleft = digitalRead(IR_Photodiodeleft);
      sensorStateCenter = digitalRead(IR_PhotodiodeCenter);
      sensorStateright = digitalRead(IR_Photodioderight);

      if (sensorStateleft && !lastStateleft) {
        Serial.print("Left Port Entry");
        beamBreakleft = true;
        updatePerseveration(left);
        leftChoices = leftChoices + 1; 
        if(beamBreakleft == true && correctChoice == left){
          Serial.println(" - Correct");
          dispenseReward(left); 
          if (printedTwentyChoiceWarning && consecutiveSameSide == 20) {
            Serial.print("WARNING: Same side chosen 20 times in a row: ");
            if (currentChoiceMade == left)
                Serial.println("LEFT");
            else
                Serial.println("RIGHT");
            }
          correctChoiceMade = true; 
          correctTrials = correctTrials + 1; 
          intertrialInterval(rewardAvailable);
          closePokes(); 
        }
        else {
          Serial.println(" - Incorrect"); 
            if (printedTwentyChoiceWarning && consecutiveSameSide == 20) {
             Serial.print("WARNING: Same side chosen 20 times in a row: ");
              if (currentChoiceMade == left)
                Serial.println("LEFT");
               else
               Serial.println("RIGHT");
        }
         if (visualTrial) {
         flashLED(correctChoice); 
        }
         if (auditoryTrial) {
         playSound(correctChoice); 
        } 
          incorrectTrials = incorrectTrials + 1; 
        }
      }
      lastStateleft = sensorStateleft;
     
      if (sensorStateright && !lastStateright) {
        Serial.print("Right Port Entry");
        beamBreakright = true; 
        updatePerseveration(right);
        rightChoices = rightChoices + 1; 
        if(beamBreakright == true && correctChoice == right) {        
          Serial.println(" - Correct");
          dispenseReward(right); 
          correctChoiceMade = true; 
          correctTrials = correctTrials + 1; 
          intertrialInterval(rewardAvailable);
          closePokes(); 
        }
        else {
          Serial.println(" - Incorrect");
        if (visualTrial) {
         flashLED(correctChoice); 
        }
         if (auditoryTrial) {
         playSound(correctChoice); 
        } 
          incorrectTrials = incorrectTrials + 1; 
        }
      }
      lastStateright = sensorStateright;

      if (sensorStateCenter && !lastStateCenter) {
        Serial.println("Center Port Entry");
      }
      lastStateCenter = sensorStateCenter;
    }
} 

/*continuously monitors the state of all ports until the a choice is made, then will exit
the loop. If correct, wil dispense the reward, and exit the loop. If the incorrect choice 
is made, will report the incorrect choice, and then exit the loop.*/
 
void checkCorrectChoice(int correctChoice) {
  bool choiceMade = false; 
  bool beamBreakleft = false;
  bool beamBreakright = false;
  while (choiceMade == false) 
    {
      sensorStateleft = digitalRead(IR_Photodiodeleft);
      sensorStateCenter = digitalRead(IR_PhotodiodeCenter);
      sensorStateright = digitalRead(IR_Photodioderight);

      if (sensorStateleft && !lastStateleft) {
        Serial.print("Left Reward Port Entry");
        updatePerseveration(left);
        beamBreakleft = true;
        leftChoices = leftChoices + 1;
        if(beamBreakleft == true && correctChoice == left){
          Serial.println(" - Correct  ");
          accuracy = true;  
          choiceMade = true; 
          correctTrials = correctTrials + 1; 
          dispenseReward(left);
          if (optoStim) {
            if (responseOptoStim) {
              sendOptoStimForDuration(rewardAvailable);
            }
            if (!responseOptoStim) {
              intertrialInterval(rewardAvailable);
            }
          }
          if (!optoStim) {
          intertrialInterval(rewardAvailable);
          }
          closePokes();                                                                                                       
        }
        else {
          Serial.println(" - Incorrect  "); 
          accuracy = false; 
          choiceMade = true;
          incorrectTrials = incorrectTrials + 1; 
          closePokes(); 
          digitalWrite(houseLight, LOW); 
          digitalWrite(GreenLED, LOW); 
          digitalWrite(BlueLED, LOW); 
          intertrialInterval(timeout);
        }
      }
      lastStateleft = sensorStateleft;
     
      if (sensorStateright && !lastStateright) {
        Serial.print("Right Reward Port Entry");
        updatePerseveration(right);
        beamBreakright = true; 

        rightChoices = rightChoices + 1; 

        if(beamBreakright == true && correctChoice == right) {        
          Serial.println(" - Correct  ");
          accuracy = true; 
          choiceMade = true;
          correctTrials = correctTrials + 1; 
          dispenseReward(right); 
          if (optoStim) {
            if (responseOptoStim) {
              sendOptoStimForDuration(rewardAvailable);
            }
            if (!responseOptoStim) {
              intertrialInterval(rewardAvailable);
            }
          }
          if (!optoStim) {
          intertrialInterval(rewardAvailable);
          }          

          closePokes(); 
        }
        else {
          Serial.println(" - Incorrect  ");
          accuracy = false; 
          choiceMade = true;
          incorrectTrials = incorrectTrials + 1; 
          closePokes();
          digitalWrite(houseLight, LOW); 
          digitalWrite(GreenLED, LOW);
          digitalWrite(BlueLED,LOW);  
          intertrialInterval(timeout);
        }
      }
      lastStateright = sensorStateright;

      if (sensorStateCenter && !lastStateCenter) {
        Serial.println("Center Initiation Port Entry");
      }
      lastStateCenter = sensorStateCenter;
    }
} 

/*will send a stim at optoFreq (defined above) for a specified delay, "d", at the optoStimFrequency and pulselength set above.*/ 
void sendOptoStimForDuration(unsigned long d) {
  unsigned long startTime = millis(); 
  unsigned long endTime = startTime; 
  if (optoStim){
  if (optoStimPulse) {
  while ((endTime - startTime) <= d) 
    { 
      digitalWrite(optoStimTTL, HIGH);
      delay(pulselength);//length of the pulse is 5ms
      digitalWrite(optoStimTTL, LOW);
      delay(optoPulseDelay - pulselength); //length of the delay till the next pulse is 25 ms - the pulse length to get a 40 Hz frequency. 100 to get a 10 Hz frequency
      endTime = millis(); 
   }
   Serial.println("Optogenetic Stim provided");
  }
    if (optoStimContinuous) {
     
      digitalWrite(optoStimTTL, HIGH);
      delay(d);
      digitalWrite(optoStimTTL, LOW);
   
   Serial.println("Optogenetic Stim provided");
  }
  
}
  if (!optoStim) {
    delay(d);
  }
}

/*Will wait for a poke in the center nose port, and then will send a trigger TTL to mark trialInitiation (sendTrialInitiationTTL = true),
or send trigger for optoStim (optoStim = true). Will also call the visual cue, auditory cue, or crossmodal cue */
void waitTrialInitiation (int side) { 
  bool centerBeamBreakStatus = false; 
  bool beamBreakleft = false;
  bool beamBreakright = false;
  midTrialstartTime = millis();
  
  while (centerBeamBreakStatus == false) 
  {
    sensorStateleft = digitalRead(IR_Photodiodeleft); 
    sensorStateright = digitalRead(IR_Photodioderight);
    sensorStateCenter = digitalRead(IR_PhotodiodeCenter);

      if ((millis() - midTrialstartTime) >= doorOpenTimer) {
        myservo.write(servoOpen); 
        delay(1000);
        closePokes(); 
        Serial.println("No Trial Initiated - Opening Doors");
        doorsOpened = doorsOpened + 1;
        midTrialstartTime = millis();
        }
        
      if (sensorStateleft && !lastStateleft) {
        Serial.println("Left Port Obstruction Detected - Opening Doors");  
        myservo.write(servoOpen); 
        delay(5000);
        closePokes();     
      }
      lastStateleft = sensorStateleft;

     
      if (sensorStateright && !lastStateright) {
        Serial.println("Right Port Obstruction Detected - Opening Doors");  
        myservo.write(servoOpen); 
        delay(5000);
        closePokes();   
      }
      lastStateright = sensorStateright;

    if (sensorStateCenter && !lastStateCenter) {       
      delay(required);
      sensorStateCenter = digitalRead(IR_PhotodiodeCenter); 
      if (sensorStateCenter) {
      centerBeamBreakStatus = true; 
      Serial.print("Center Port Entry - Trial Initiated ");
      Serial.print(trialsCompleted);
      Serial.print("   20-Warnings: ");
      Serial.println(twentyChoiceWarnings);

      if (sendTrialInitiationTTL) {
      //send TTL pulse to fiber photometry data acquisition system  
      digitalWrite(trialInitiationTTLFiP, HIGH);
      digitalWrite(trialInitiationTTLEEG, HIGH);
      delay(ttlLength);  
      digitalWrite(trialInitiationTTLFiP, LOW);
      digitalWrite(trialInitiationTTLEEG, LOW);
      }

      if (optoStim) {
        if (delayOptoStim) {
          sendOptoStimForDuration(delayToCue);
        }
        else {
          delay(delayToCue);
        }
      }
      else {
        delay(delayToCue);
      }
  
      if (!conflict) {
        if (visualTrial) {
           flashLED(side); 
        }
        if (auditoryTrial) {
           playSound(side); 
        }
      }
      if (conflict) {
          playCrossmodalCue(trialModality,side); 
      }
              
      }      
    }
    lastStateCenter = sensorStateCenter;


    //check for user input
    if(Serial.available()){
         command = Serial.readStringUntil('\n');
         executeUserCommand(command); 
    }
  }
} 


/*send TTL pulse to arduino to flash LED for cueLength (defined above), can also trigger opto stim (if optoStim = true) 
during the cue (cueOptoStim = true), delay (delayOptoStim = true), or response (responseOptoStim = true) if designated above */
void flashLED (int visualCue) {
  switch (visualCue)  {
    case left: //left port reward cue 
      analogWrite(leftRewardWhiteLED, visualCueIntensity);
      if (optoStim) {
        if (distractorCues) {
          if (distractorVisualCues) {
            if (matched){
              analogWrite(leftDistractorLED, visDistractorIntensity);
            }
            if (!matched){
              analogWrite(rightDistractorLED, visDistractorIntensity);
            }
          }
           if (distractorAuditoryCues) {
            if (matched){
              analogWrite(leftTone, audDistractorIntensity);
            }
            if (!matched){
              analogWrite(rightTone, audDistractorIntensity);
            }
          }
        }

        if (cueOptoStim) {
        sendOptoStimForDuration(cueLength);
        }
        if (responseOptoStim) {
          delay(cueLength); 
        }
        if (delayOptoStim) {
          delay(cueLength);
        }
        analogWrite(leftDistractorLED, 0);
        analogWrite(rightDistractorLED, 0);
        analogWrite(leftTone, 0);
        analogWrite(rightTone, 0);
      }
      if (!optoStim) {
        if (distractorCues) {
          if (distractorVisualCues) {
            if (matched){
              analogWrite(leftDistractorLED, visDistractorIntensity);
            }
            if (!matched){
              analogWrite(rightDistractorLED, visDistractorIntensity);
            }
          }
           if (distractorAuditoryCues) {
            if (matched){
              analogWrite(leftTone, audDistractorIntensity);
            }
            if (!matched){
              analogWrite(rightTone, audDistractorIntensity);
            }
          }
        delay(cueLength); 
        analogWrite(leftDistractorLED, 0);
        analogWrite(rightDistractorLED, 0);
        analogWrite(leftTone, 0);
        analogWrite(rightTone, 0);
        }
        if (!distractorCues){
        delay(cueLength); 
        }
      } 
      analogWrite(leftRewardWhiteLED, 0);
    break;
    case right: //right port reward cue 
      analogWrite(rightRewardWhiteLED, visualCueIntensity);
      if (optoStim) {
        if (distractorCues) {
          if (distractorVisualCues) {
            if (matched){
              analogWrite(rightDistractorLED, visDistractorIntensity);
            }
            if (!matched){
              analogWrite(leftDistractorLED, visDistractorIntensity);
            }
          }
           if (distractorAuditoryCues) {
            if (matched){
              analogWrite(rightTone, audDistractorIntensity);
            }
            if (!matched){
              analogWrite(leftTone, audDistractorIntensity);
            }
          }
        }
      }
      if (!optoStim) {
        if (distractorCues) {
          if (distractorVisualCues) {
            if (matched){
              analogWrite(rightDistractorLED, visDistractorIntensity);
            }
            if (!matched){
              analogWrite(leftDistractorLED, visDistractorIntensity);
            }
          }
           if (distractorAuditoryCues) {
            if (matched){
              analogWrite(rightTone, audDistractorIntensity);
            }
            if (!matched){
              analogWrite(leftTone, audDistractorIntensity);
            }
          }
        delay(cueLength); 
        analogWrite(leftDistractorLED, 0);
        analogWrite(rightDistractorLED, 0);
        analogWrite(leftTone, 0);
        analogWrite(rightTone, 0);
        }
        if (!distractorCues){
        delay(cueLength); 
        }
      }  
      analogWrite(rightRewardWhiteLED, 0);
    break;  
    case both: //simultaneous flash of both LEDs
      analogWrite(rightRewardWhiteLED, visualCueIntensity);
      analogWrite(leftRewardWhiteLED, visualCueIntensity);
      delay(cueLength);  
      analogWrite(rightRewardWhiteLED, 0);
      analogWrite(leftRewardWhiteLED, 0);
  }  
 }


/*if running a crossmodal session (crossmodal = true), will flash Green LED to indicate the visual cue, 
and the Blue LED to indicate the auditory cue */
void playModalityCue (int modality) {
  switch (modality)  {
    case visual: //left port reward cue 
//    Serial.println("Flashing Green LED"); 
    digitalWrite(GreenLED, HIGH);
    digitalWrite(BlueLED, LOW); 
    break;
    case auditory: //right port reward cue 
//    Serial.println("Flashing Blue LED"); 
      digitalWrite(BlueLED, HIGH);
      digitalWrite(GreenLED, LOW); 
    break;  
  }  
}
/*variables for the upsweep and downswep*/
enum{downsweep = 1, upsweep = 2};

/*Will send a pulse to an adafruit soundcard to play an upsweep or downsweep, currently set up to play the sound cue twice  */
void playSound (int soundCue) {
  switch (soundCue)  {
    case upsweep: //right port reward cue 
      digitalWrite(upsweepTTL, LOW);
      delay(300);  
      digitalWrite(upsweepTTL, HIGH);
      delay(300);  
      digitalWrite(upsweepTTL, LOW);
      delay(300);
      digitalWrite(upsweepTTL, HIGH);
//      delay(300);  
//      digitalWrite(upsweepTTL, LOW);
//      delay(300);
//      digitalWrite(upsweepTTL, HIGH);
    break;
    case downsweep: //left port reward cue 
      digitalWrite(downsweepTTL, LOW);
      delay(300);  
      digitalWrite(downsweepTTL, HIGH);
      delay(300);  
      digitalWrite(downsweepTTL, LOW);
      delay(300);  
      digitalWrite(downsweepTTL, HIGH);
//      delay(300);  
//      digitalWrite(downsweepTTL, LOW);
//      delay(300);  
//      digitalWrite(downsweepTTL, HIGH);      
    break;  
    case both:          
      digitalWrite(downsweepTTL, LOW);
      delay(300);  
      digitalWrite(downsweepTTL, HIGH);
      delay(300);  
      digitalWrite(upsweepTTL, LOW);
      delay(300);
      digitalWrite(upsweepTTL, HIGH);
    break; 
  }  
}

/*Will play the correct cue, based on the trialModality and correctSide, if visual and left, will flash left on the right
and play the sound cue for the right and vice versa */
void playCrossmodalCue (int trialModality, int correctSide ) {
   switch (trialModality)  {
    case visual: //will play the coordinating visual cue and conflicting auditory cue 
      if (correctSide == left) {
      digitalWrite(leftRewardWhiteLED, HIGH);
      playSound(right); 
      delay(cueLength - 300);  //to account for the length of the playSound() function 
      digitalWrite(leftRewardWhiteLED, LOW);       
      Serial.println("Flashing LED and playing sound"); 
      }
      if (correctSide == right) {
      digitalWrite(rightRewardWhiteLED, HIGH);
      playSound(left); 
      delay(cueLength - 300);  //to account for the length of the playSound() function 
      digitalWrite(rightRewardWhiteLED, LOW);
      Serial.println("Flashing LED and playing sound");           
      }
    break;
    case auditory: //will play the coordinating auditory cue and conflicting visual cue 
      if (correctSide == left) {
      digitalWrite(rightRewardWhiteLED, HIGH);
      playSound(left); 
      delay(cueLength - 300);  //to account for the length of the playSound() function 
      digitalWrite(rightRewardWhiteLED, LOW); 
      Serial.println("Flashing LED and playing sound");        
      }
      if (correctSide == right) {
      digitalWrite(leftRewardWhiteLED, HIGH);
      playSound(right); 
      delay(cueLength - 300);  //to account for the length of the playSound() function 
      digitalWrite(leftRewardWhiteLED, LOW);     
      Serial.println("Flashing LED and playing sound");      
      }
    break;  
  } 
}

/*close nose pokes by movng down ten units on the servo until the doors reach the closed position*/
void closePokes() {
  for (int i = 0; i <= servoClosed; i += 10) {
    myservo.write(servoOpen + i);
    delay(200);
  }
  myservo.write(servoClosed);
  delay(15);
}

/*conv3rt the trial types to strings that can be displayed in the serial monitor*/
String eventToString (int e){
  switch (e) {
    case training: return "training";
    case leftCue: return "Left Cue";
    case rightCue: return "Right Cue";
    case leftDMTP: return "Left DMTP";
    case rightDMTP: return "Right DMTP";
    case leftDNMTP: return "Left DNMTP"; 
    case rightDNMTP: return "Right DNMTP";   
    case cleanPorts: return "Clean Ports"; 
    case habituation1: return "Habituation - Phase 1";
    case habituation2: return "Habituation - Phase 2"; 
    case auditory: return "Auditory"; 
    case visual: return "Visual"; 
  }
}

/*chain of events for each trial type*/
void executeTrial(int trialType) {
  switch (trialType) {
    case cleanPorts:
    //ports open for 10 seconds so experimenter can clean ports       
      myservo.write(servoOpen); 
       intertrialInterval(20000); 
      closePokes();
    break; 
    case habituation1:/*paint the reward ports with milk and let them consume milk from all three for 10 minutes. Doors will open and close 
       and the reward will be dispensed every 2 minutes */ 
      myservo.write(servoOpen); 
      checkBeamBreakForDelay(120000); 
      closePokes();
      intratrialInterval(5000); 
      myservo.write(servoOpen); 
      dispenseReward(both); 
      checkBeamBreakForDelay(120000); 
      closePokes();
      intratrialInterval(5000); 
      myservo.write(servoOpen); 
      dispenseReward(both);   
      checkBeamBreakForDelay(120000); 
      closePokes();
      intratrialInterval(5000); 
      myservo.write(servoOpen); 
      dispenseReward(both);   
      checkBeamBreakForDelay(120000);
      closePokes();
    
    break; 
    case habituation2: //will play cues for both sides, wait for animal to make a choice, dispense reward in the choice port, and start a new trial after 5 s
      myservo.write(servoOpen); 
      if (visualTrial) {
           flashLED(both); 
        }
        if (auditoryTrial) {
           playSound(both); 
        }
      waitAnyChoice();//will dispense reward after choice
      closePokes();
      intertrialInterval(5000);
    break; 
    case training:
    //animal pokes nose in center port to initiate trial, both lights flash, reward is available in both sides after a nose poke       
      waitTrialInitiation(both); 
      myservo.write(servoOpen); 
      waitAnyChoice();
      closePokes();
      intertrialInterval(5000);
    break;       
    case leftCue: 
      correctSide = left;
      waitTrialInitiation(left); 
      myservo.write(servoOpen); 
      checkCorrectChoice(left);
    break; 
    case rightCue: 
      correctSide = right;
      waitTrialInitiation(right); 
      myservo.write(servoOpen); 
      checkCorrectChoice(right); 
     break; 
     case leftDMTP: //for DMTP and DNMTP, do we want for the mouse to have to place its nose back in the initiation port to get the go cue to make the next choice? 
      waitTrialInitiation(left);
      flashLED(left); 
      intertrialInterval(500); 
      myservo.write(servoOpen); 
      waitCorrectChoice(left);
      dispenseReward(left); 
      intertrialInterval(10000);
      flashLED(both); 
      checkCorrectChoice(left);
      closePokes();
      intratrialInterval(5000);
     break; 
     case rightDMTP:
      waitTrialInitiation(right);
      flashLED(right); 
      intertrialInterval(500); 
      myservo.write(servoOpen); 
      waitCorrectChoice(right);
      dispenseReward(right); 
      intertrialInterval(10000);
      flashLED(both); 
      checkCorrectChoice(left);
      closePokes();   
      intratrialInterval(5000); 
     break; 
     case leftDNMTP:
      waitTrialInitiation(left);
      flashLED(left); 
      intertrialInterval(500); 
      myservo.write(servoOpen); 
      waitCorrectChoice(left);
      dispenseReward(left); 
      intertrialInterval(10000);
      flashLED(both); 
      checkCorrectChoice(right);
      closePokes(); 
      intratrialInterval(5000);   
     break; 
     case rightDNMTP: 
      waitTrialInitiation(right);
      flashLED(right); 
      intertrialInterval(500); 
      myservo.write(servoOpen); 
      waitCorrectChoice(right);
      dispenseReward(right); 
      intertrialInterval(10000);
      flashLED(both); 
      checkCorrectChoice(left);
      closePokes(); 
      intratrialInterval(5000);     
     break;         
  }
}

/*after sessionTime (defined above) will display summary stats through the serial monitor*/
void displaySummaryStats () {
    Serial.println("Performance Summary:"); 
    Serial.print("Left Choices: ");
    Serial.println(leftChoices);
    Serial.print("Right Choices: ");
    Serial.println(rightChoices); 
    if (trialsCompleted > 0) {
        float accuracy = (float)correctTrials / trialsCompleted;
        Serial.print("Accuracy: ");
        Serial.println(accuracy, 3);   // prints 0.900
        Serial.print("Perseveration score: ");
        Serial.println(persScore, 3);
    }
    Serial.print("Doors Opened: ");
    Serial.println(doorsOpened);

}


/*main loop that will execute the desired trial sequence using the parameters defined above*/
void loop() { 

   for (int i = 0; i < numTrials; i++){

                if(Serial.available()){
                    command = Serial.readStringUntil('\n');
                    executeUserCommand(command); 
                }
                
                Serial.print("Trial Available: "); 
                String trialString = eventToString(trialSequence[i]); 

                if (modalityVariable){
                trialModality = trialModalitySequence[i];
                if (trialModality == visual) {
                  visualTrial = true; 
                  auditoryTrial = false; }
                if (trialModality == auditory){
                  auditoryTrial = true; 
                  visualTrial = false;                
                }
                playModalityCue(trialModality);
                }

//              Crossmodal version of the task                
                if (checkConflict) {
                conflict = conflictSequence[i];
                if (conflict) {
                Serial.print("Conflict "); 
                }
                if (!conflict) {
                  Serial.print("No conflict ");
                }
                }


                //if not doing the crossmodal and want to keep the modality light and use as house light to indicate trial availabity and to turn 
                //off during a timeout 

                  if (visualTrial) {
                    digitalWrite(houseLight, HIGH); 
                    digitalWrite(GreenLED, HIGH);
                    digitalWrite(BlueLED, LOW);
                  }
                  if (auditoryTrial) {
                    digitalWrite(houseLight, HIGH);
                    digitalWrite(BlueLED, HIGH); 
                    digitalWrite(GreenLED, LOW);
                  }
                  
                if (modalityVariable) {
                //print out whether trial is visual or auditory
                String trialModalityString = eventToString(trialModality);  
                Serial.print(trialString);
                Serial.print(" - ");              
                Serial.println(trialModalityString); 
                }

                //will print out trial string for training sessions
                if (!variableCue && !variableDelay && !crossmodalTrial && !modalityVariable && !distractorCues ) {
                 Serial.println(trialString);  
                }

//              variableCue version of the task 
                if (variableCue) {
                Serial.print(trialString);
                Serial.print(" Length - "); 
                cueLength = cueLengthSequence[i];
                Serial.println(cueLength); 
                }

//              variableDelay version of the task 
                if (variableDelay) {
                Serial.print(trialString);
                Serial.print(" Delay - "); 
                delayToCue = variableDelayLengthSequence[i];
                Serial.println(delayToCue);
                }
                if (blockOptoStim) {
                 optoStim = optoStimSequence[i];
  
                }

                if (randomOptoStim) {
                  optoOn = random(100);
                  //Serial.println(optoOn);
                  if (optoOn < optoProbability) {
                    optoStim = true;
                  }
                  else {
                    optoStim = false; 
                  }
                }
 //             distractor version of task 
                if (distractorCues){
                  matched = matchSequence[i];
                  visDistractorIntensity = visIntensitySequence[i];
                  audDistractorIntensity = audIntensitySequence[i];
                Serial.print(trialString);
                Serial.print(" Length - "); 
                Serial.print(cueLength); 
                if  (distractorVisualCues) {
                  Serial.print(" - Vis");
                  if (matched) {
                    Serial.print(" Matched ");
                    }
                   if (!matched) {
                    Serial.print(" Unmatched ");
                    }
                  Serial.print("Dist Intensity - ");    
                  Serial.println(visDistractorIntensity);
                }   
                if (distractorAuditoryCues) {
                  Serial.print(" - Aud");
                  if (matched) {
                    Serial.print(" Matched ");
                    }
                   if (!matched) {
                    Serial.print(" Unmatched ");
                    }
                  Serial.print("Dist Intensity - ");    
                  Serial.println(audDistractorIntensity);
                }         
                }
                
                executeTrial(trialSequence[i]);     
                   
                
                if (!accuracy) {
                  repeatTrial =  random(100); 
                  if (repeatTrial < repeatProbability) {
                    i = i - 1; 
                    Serial.println("Will repeat previous trial");
                  }               
                }

                endSession = millis(); 
                trialsCompleted = trialsCompleted + 1; 

                 if (trialsCompleted > 100) {
                          Serial.println("100 Trials Completed"); 
                 } 
                
                 if ((endSession - startSession) > totalSessionTime) {
                          displaySummaryStats(); 
                          startSession = millis();
                 }              
          }
}
