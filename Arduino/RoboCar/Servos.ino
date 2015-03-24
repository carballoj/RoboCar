/**
 * Servo handling functions for control and position estimation
 *
 * @author Joseph Duchesne
 */
 
// Servo lib for camera servos
#include <Servo.h>

//a few useful constants
#define SERVO_PITCH 0
#define SERVO_YAW 1

//fraction multiplier consts
#define NUMERATOR 0
#define DENOMINATOR 1

//camera servos: pitch, yaw
static int servoPins[2] = {11,10};
//for pitch, yaw what are the min/max valid values
static int servoRanges[2][2] = {{-10,100}, {-45,45}}; 
//for pitch, yaw what are the microsecond timings for those degrees
static int servoTimings[2][2] = {{710,1920}, {967,1967}}; 

//servo position estimation vars/consts
int servoPositionsOld[2] = {0}; 
int servoPositions[2] = {0}; 
int timeOfLastServoChange[2] = {0};
//0.23 sec/60deg @4.8v as per sheed, estimated 0.224 sec/60deg @5v via linear approx.
//for fast and accurate int math, I'm storing a fraction: 0: numerator, 1: denominator
static long int servoDegreesPerMillisecond[2] = {60, 224};
static int maxServoMoveTime = 700; //ms. At 0.224/60deg it would take 672ms to move 180deg.

//servo objects for writing
Servo servos[2];

/**
 * Init servo objects
 * 
 * @return void
 */
void servo_setup() {
  int i;
  
  //attach the servo objects and
  for (i=0;i<2;i++) {
    servos[i].attach(servoPins[i]);
  }
  //move the servos to their home positions (0 degrees horizontal and vertical0
  setServo('P', 0);
  setServo('Y', 0);
}

/**
 * Using the servo's estimated speed, old position, and time since last move command
 * estimate the servo's current position in degrees
 * 
 * @param int offset The servo offset (0 for pitch, 1 for yaw)
 *
 * @return int The current estimated servo position in degrees
 */
int estimateServoPosition(int offset){
  long int delta = millis()-timeOfLastServoChange[offset];
  
  //estimation cutoff to avoid wraparound errors
  if (!timeOfLastServoChange[offset] || delta>maxServoMoveTime) {
    timeOfLastServoChange[offset]=0; //skip estimation in the future
    return servoPositions[offset]; //it's clearly arrived
  }
  
  //multiply the time delta by the degrees/milli fraction to get degree delta
  long int degreeDelta = delta * servoDegreesPerMillisecond[NUMERATOR] / servoDegreesPerMillisecond[DENOMINATOR];
  
  //add or subtract the degree delta and constrain between the old and the new position values
  if (servoPositions[offset]>servoPositionsOld[offset]) { //+ direction
    return constrain(servoPositionsOld[offset]+degreeDelta, servoPositionsOld[offset], servoPositions[offset]);
  } else {  //- direction
    return constrain(servoPositionsOld[offset]-degreeDelta, servoPositions[offset], servoPositionsOld[offset]);
  }
}

/**
 * Set the servos ('P' or 'Y') to an angle
 *
 * @param char dir   The side 'P' for pitch up/down, 'Y' for yaw side to side
 * @param int  angle The angle in degrees
 *
 * @return void
 */
void setServo(char servo, int angle) {
  int microseconds = 0;
  
  int offset = SERVO_PITCH; //pitch defaults to offset 0
  if (servo=='Y') {  //yaw is offset 1
    offset = SERVO_YAW; 
  }
  
  Serial.print("#Setting servo ");
  Serial.print(servo);
  Serial.print(" to ");
  
  //clamp servo angle between the configured min/max
  angle = constrain(angle, servoRanges[offset][0], servoRanges[offset][1]);
  
  Serial.print(angle);
  Serial.print(" degrees: ");
  
  //now calculate the timing 
  microseconds = map(angle,  servoRanges[offset][0], servoRanges[offset][1], servoTimings[offset][0],  servoTimings[offset][1]);

  Serial.print(microseconds);
  Serial.println("us");
  
  servos[offset].writeMicroseconds(microseconds);
  
  //update position estimation data
  servoPositionsOld[offset] = estimateServoPosition(offset); 
  servoPositions[offset] = angle; 
  timeOfLastServoChange[offset] = millis();
}