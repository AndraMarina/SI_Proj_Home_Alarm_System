// C++ code
#include<Keypad.h>
#include<LiquidCrystal_I2C.h>
#include<Servo.h>
#include "base64.hpp"

// ----Pins---- 
//Camera Pins
int CAMERA_1_SENSOR = A0;
int CAMERA_2_SENSOR = A1;

//I2C LCD Display Pin & Init
// SDA = A4
// SCL = A5
int I2C_ADDRESS = 0x27;
LiquidCrystal_I2C lcd(I2C_ADDRESS, 16, 2);

// Keypad Pins & Init
const byte rows = 4;
const byte cols = 4;

char hexkeypad[rows][cols] = { {'1', '2', '3', 'A'},
                               {'4', '5', '6', 'B'},
                               {'7', '8', '9', 'C'},
                               {'*', '0', '#', 'D'}
                             };

byte rowspins[rows] = {2,3,4,5};
byte colspins[cols] = {6,7,8,9};

Keypad kpd = Keypad(makeKeymap(hexkeypad), rowspins, colspins, rows, cols);

//Servomotor Pin & Init
int SERVO_PIN = 10;
Servo door_servo;

//LED Pins
int LED1 = 12;
int LED2 = 11;

//Buzzer pin
int BUZZER = 13;


//----Global Variables----
bool enter_pin = false;
int pin_len_count = 0;
unsigned char pin_str[5];

int new_alarm_state = -1;
int init_alarm_state = -1;


// Initialize Input and Outpus Pins and Objects
void setup()
{
  //Serial Monitor Setup
  Serial.begin(9600);

  //Sensors Setup
  pinMode(CAMERA_1_SENSOR, INPUT);
  pinMode(CAMERA_2_SENSOR, INPUT);

  //LCD Setup
  lcd.init();
  lcd.backlight();

  //Keypad Setup
  for(byte i = 2; i < 10; i ++){
  	pinMode(i, INPUT);
  }
  
  //Servo setup
  door_servo.attach(SERVO_PIN);

  //Led Setup
  pinMode(LED1, INPUT);
  pinMode(LED2, INPUT);
}

// Turn led ON and OFF with a small delay
void blink_led(int led_pin){
	digitalWrite(led_pin, HIGH);
  delay(10);
  digitalWrite(led_pin, LOW);
  delay(10);                      
}

//Read and return the state of the sensor in 1 camera
int detect_movement_in_camera(int cameraPin){
  int CAMERA_SENSOR_STATE = 0;
	int CAMERA_SENSOR_READ = analogRead(cameraPin);
  CAMERA_SENSOR_STATE = map(CAMERA_SENSOR_READ, 0, 1023, 0, 255);
  	
  //Serial.print("Camera Sensor State: ");
  //Serial.println(CAMERA_SENSOR_STATE);
  delay(20);
  return CAMERA_SENSOR_STATE;
}

// Enable or disable acoustic, light and display signals based on alarm state
void alarm_system_state(bool show_lcd, bool silent){

  int camera1State = detect_movement_in_camera(A0);
  int camera2State = detect_movement_in_camera(A1);
  
  	
  if(camera1State < 10 && camera2State < 10){
   	if(show_lcd == true){
    	lcd.clear();
    	lcd.print("Zone libere!");
    }
  } 
  else {
    if(camera1State > 10 && camera2State > 10){
    	Serial.println("Alerta camera 1 & 2");	
      if(show_lcd == true){
        lcd.clear();
        lcd.print("Alerta camera 1");
        lcd.setCursor(0,1);
        lcd.print("Alerta camera 1");
      }
    
      if(silent == false){
       	blink_led(LED1);
       	blink_led(LED2);
      	tone(BUZZER, 200, 100);
  		  delay(200);
       	tone(BUZZER, 500, 300);
  		  delay(400);
      }
    } 
    else {
      if(camera1State > 10){
        Serial.println("Alerta camera 1");
        if(show_lcd == true){
          lcd.clear();
          lcd.print("Alerta camera 1");
        }
        
        if(silent == false){
          blink_led(LED1);
          tone(BUZZER, 200, 100);
          delay(200);
        }
      }
      
      if(camera2State > 10){
        Serial.println("Alerta camera 2");
        if(show_lcd == true){
          lcd.clear();
          lcd.print("Alerta camera 2");
        }
        
        if(silent == false){
          blink_led(LED2);
          tone(BUZZER, 500, 300);
          delay(400);
        }
      }
    }
  }
    delay(10);
}

// Engage Servomotor to close the door
void close_door(){
  for(int position = 1; position <= 179; position++){
  	door_servo.write(position);
   	delay(20);
  }
}

// Engage Servomotor to open the door
void open_door(){
  for(int position = 179; position >= 1; position--){
  	door_servo.write(position);
   	delay(20);
  }
}

// Arm/disarm alarm based on keyboard input
void arm_alarm(bool arm){
  bool match = true;
  unsigned char arm_key[9] = "MTIzNAAA";
  unsigned char disarm_key[9] = "NDMyMQAA";
  unsigned char pin_key[9];
  unsigned char encoded_pin[9];
  char start_key;
	  
  if(arm == true){
    for(int i = 0; i < 9; i++){
    	pin_key[i] = arm_key[i];
    }
    start_key = '*';
  } 
  else {
    for(int i = 0; i < 9; i++){
  		pin_key[i] = disarm_key[i];
    }
    start_key = '#';
  }
  
  char key = kpd.getKey();
  if (key != NO_KEY){
    if(key == start_key){
    	enter_pin = true;
      lcd.clear();
      lcd.print("Enter IP:");
    } 
    else {	
      if(enter_pin == true){
    		lcd.print(key);
        pin_str[pin_len_count] = (char)key;
        delay(200);
        pin_len_count++;
          
        if (pin_len_count==4){
          pin_str[pin_len_count] = '\0';
          lcd.clear();
      		lcd.print("PIN Entered!");
          enter_pin = false;
              	
          pin_len_count=0;             
          encode_base64(pin_str, 6, encoded_pin);

          for(int i=0; i<9; i++){
            if(encoded_pin[i] != pin_key[i]){
              match = false;
              lcd.clear();
              lcd.print("Wrong PIN!");
              delay(300);
            }
          }
          
          if(arm == true && match == true){
            new_alarm_state = match;
          } 
          else {
            if(match == true)
             	new_alarm_state = -1;
          }
          Serial.println();
        }      
      }
    }
  }
  
  if(new_alarm_state == -1)
    alarm_system_state(false ,true);

  if(new_alarm_state == 1){
    if(enter_pin == false)
    	alarm_system_state(true ,false);
    else
    	alarm_system_state(false ,false);
  }
}

void loop()
{
  // Check if alarm arming begins since it's unarmed
  if(init_alarm_state == -1)
  	arm_alarm(true);

  // Check if alarm disarming beging since it's armed
  if(init_alarm_state == 1){
    arm_alarm(false);
  }
  	
  delay(200);

  // For changes between previous and current state
  if(init_alarm_state != new_alarm_state){
    
    // Print disarm message on LCD Display and open the door through the servomotor
    if(new_alarm_state == -1){
      lcd.clear();
      lcd.print("Alarm disharmed!");
      open_door();
    }

    // Print arm message on LCD Display and close the door through the servomotor
    if(new_alarm_state == 1){
      lcd.clear();
      lcd.print("Alarm armed!");
      close_door();
    }	

    // Reset init state with latest one 
    init_alarm_state = new_alarm_state;

  }
}