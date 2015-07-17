#include <ros.h>
#include <std_msgs/UInt16.h>
#include <std_msgs/Int16.h>

struct linearActuatorObj{
  int A_pin;
  int A_history[3];
  int A_raw;
  int A_position;
  int A_lastPosition;
  int A_lastGoodPosition;
  float A_erraticPercent;
  boolean A_isErratic;
  boolean A_isFaulted;
  int A_lowLimit;
  int A_highLimit;
  float A_maxRateofChange;
  unsigned long A_currentSampleTimeMS;
  unsigned long A_lastSampleTimeMS;
  unsigned long A_lastGoodSampleTimeMS; 
  boolean M_Enable;
  int M_DIRPin;
  int M_PWMPin;
  int M_error;
  int M_Command;
  int M_PWMCommand;
  int M_deadband;
};

struct linearActuatorObj *linearActuatorObjectCreate(int Apin, float maxRateofChange, int DIRPin, int PWMPin, int lowLimit, int highLimit,int deadband){
  
  struct linearActuatorObj* obj;
  obj = (struct linearActuatorObj *)malloc(sizeof(struct linearActuatorObj));
  
  obj->A_pin = Apin;
  obj->A_maxRateofChange = maxRateofChange;
  obj->M_DIRPin = DIRPin;
  obj->M_PWMPin = PWMPin;
  obj->A_lowLimit = lowLimit;
  obj->A_highLimit = highLimit;
  obj->M_deadband = deadband;
  obj->M_Command = 400;
  
  // initialize Motor Pins
  pinMode(obj->M_DIRPin, OUTPUT);
  pinMode(obj->M_PWMPin, OUTPUT);
  digitalWrite(obj->M_DIRPin,LOW);
  digitalWrite(obj->M_PWMPin,LOW);
  
  return obj;
}
  
void linearActuatorObjectDestroy(struct linearActuatorObj *who){
  free(who);
}

// Functions ********************************************************

void sampleandFilter(struct linearActuatorObj *Act){

  Act->A_raw = analogRead(Act->A_pin);
  Act->A_currentSampleTimeMS = millis();
  Act->A_history[2] = Act->A_history[1];
  Act->A_history[1] = Act->A_history[0];
  Act->A_history[0] = Act->A_raw;
  
  int middle;
  if        ((Act->A_history[0] <= Act->A_history[1]) && (Act->A_history[0] <= Act->A_history[2])){
    middle = (Act->A_history[1] <= Act->A_history[2]) ?   Act->A_history[1] :  Act->A_history[2];
  }else if  ((Act->A_history[1] <= Act->A_history[0]) && (Act->A_history[1] <= Act->A_history[2])){
    middle = (Act->A_history[0] <= Act->A_history[2]) ?   Act->A_history[0] :  Act->A_history[2];
  }else {
    middle = (Act->A_history[0] <= Act->A_history[1]) ?   Act->A_history[0] :  Act->A_history[1];
  }
  
  Act->A_position = middle;
 
} //*******************************************************************

void sensorValidityCheck(struct linearActuatorObj *Act){
  
  // check for high rate of change
  float err = Act->A_position - Act->A_lastPosition;
  float period = Act->A_currentSampleTimeMS - Act->A_lastSampleTimeMS;
  float thisRate = abs( err / period );
  float addition = (thisRate > Act->A_maxRateofChange) ? 0.2 : -0.2 ;
  Act->A_erraticPercent = Act->A_erraticPercent + addition;
  
  if(Act->A_erraticPercent > 2.0){Act->A_erraticPercent = 2.0;} // burn off windup
  if(Act->A_erraticPercent < 0.0){Act->A_erraticPercent = 0.0;}
  
  if(Act->A_erraticPercent > 1.0){Act->A_isErratic = true;}   // set diagnostic
  if(Act->A_erraticPercent = 0.0){Act->A_isErratic = false;}
  
  // check for high/low faults
  
  if(Act->A_position > Act->A_highLimit){ Act->A_isFaulted = true;}
  else if(Act->A_position < Act->A_lowLimit){ Act->A_isFaulted = true;}
  else{ Act->A_isFaulted = false;}

} //*******************************************************************

void setDiagnostic(struct linearActuatorObj *Act){
  if(Act->A_isErratic || Act->A_isFaulted){
    // TO DO Send ROS Signal here TO DO
    Act->M_Enable = false;
    
    Act->A_lastPosition = Act->A_position;
    Act->A_position = Act->A_lastGoodPosition;
    
    }
} //*******************************************************************

void updateAnalogParameters(struct linearActuatorObj *Act){
 // if(
} //*******************************************************************

void updateMotor(struct linearActuatorObj *Act){
 // if(
} //*******************************************************************

// ROS parameters***********************************************

ros::NodeHandle  nh;

std_msgs::UInt16 val1SM, val2SM;
std_msgs::UInt16 cmd1SM, cmd2SM;
std_msgs::UInt16 diagSM;

ros::Publisher linValActual1_pub("linValActual1", &val1SM);
ros::Publisher linValActual2_pub("linValActual2", &val2SM);

ros::Publisher linValCmdEcho1_pub("linValCmdEcho1", &cmd1SM);
ros::Publisher linValCmdEcho2_pub("linValcmdEcho2", &cmd2SM);

ros::Publisher arduinoDiagnostic_pub("arduinoDiagnostic_pub", &diagSM);

int linValCmdFromROS1 = 400;
int linValCmdFromROS2 = 400;

void linValCmd1_Cb( const std_msgs::UInt16& msg){
  if(msg.data < 160){
    linValCmdFromROS1 = 160;
  }else{
    linValCmdFromROS1 = msg.data;
  }
}

void linValCmd2_Cb( const std_msgs::UInt16& msg){
  if(msg.data < 160){
    linValCmdFromROS2 = 160;
  }else{
    linValCmdFromROS2 = msg.data;
  }
}

ros:: Subscriber<std_msgs::UInt16> sub1("linValCmd1",linValCmd1_Cb);
ros:: Subscriber<std_msgs::UInt16> sub2("linValCmd2",linValCmd2_Cb);



void setup(){
  
  nh.initNode();
  
  nh.advertise(linValActual1_pub);
  nh.advertise(linValActual2_pub);

  nh.advertise(linValCmdEcho1_pub);
  nh.advertise(linValCmdEcho2_pub);

  nh.advertise(arduinoDiagnostic_pub);

  nh.subscribe(sub1);
  nh.subscribe(sub2);
}

// Populate objects                  *linearActuatorObjectCreate(  Apin, maxRateofChange, DIRPin, PWMPin, lowLimit, highLimit, deadband)*/
struct linearActuatorObj *linAct1  = linearActuatorObjectCreate(  0,    500.0,            5,      6,        150,      1050,     5);
struct linearActuatorObj *linAct2  = linearActuatorObjectCreate(  0,    500.0,            9,      10,       150,      1050,     5);
    
void loop(){

  sampleandFilter(linAct1);
  sampleandFilter(linAct2);
  
  sensorValidityCheck(linAct1);
  sensorValidityCheck(linAct2);
  
  setDiagnostic(linAct1);
  setDiagnostic(linAct2);
  
  updateAnalogParameters(linAct1);
  updateAnalogParameters(linAct2);
  
  updateMotor(linAct1);
  updateMotor(linAct2);
  
  // update and Send ROS Parameters
  
  val1SM.data = linAct1->A_position;
  val2SM.data = linAct2->A_position;
  cmd1SM.data = linAct1->M_Command;
  cmd2SM.data = linAct2->M_Command;
  
  linValActual1_pub.publish( &val1SM );
  linValActual2_pub.publish( &val2SM );
  
  linValCmdEcho1_pub.publish( &cmd1SM);
  linValCmdEcho2_pub.publish( &cmd2SM);
      
    
  nh.spinOnce();
  delay(10);
}


