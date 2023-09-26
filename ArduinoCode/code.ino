#include <Arduino.h>
#include <avr/power.h>
#include <Arduino_FreeRTOS.h>
// macros
#define PF0   A5
#define PF1   A4
#define PF4   A3
#define PF5   A2
#define PF6   A1
#define PF7   A0
#define DIGIT_1   13  // PC7
#define RED_LED   5   // PC6
#define DIGIT_4   10  // PB6
#define DATA      9   // PB5 
#define LATCH     8   // PB4 
#define BUZZER    6   // PD7
#define DIGIT_2   12  // PD6
#define GREEN_LED 4   // PD4
#define PD3       1   // TX
#define PD2       0   // RX
#define BUTTON_1  2   // PD1 Start/Stop Timer and Stop Buzzer 
#define BUTTON_2  3   // PD0 Inc Timer
#define DIGIT_3   11  // PB7

//7-Seg Display Variables
unsigned char gTable[]=
{0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c
,0x39,0x5e,0x79,0x71,0x00};

//Timer Variables
#define DEFAULT_COUNT 30 //s
int  gCount = DEFAULT_COUNT;
char gBuzzerFlag = 0;
unsigned char gTimerRunning = 0;
//define task handles
TaskHandle_t TaskClockTimer_Handler;

void setup() {

  if(F_CPU == 16000000) clock_prescale_set(clock_div_1);

   pinMode(RED_LED, OUTPUT);
   pinMode(GREEN_LED, OUTPUT);
   //LEDs
   digitalWrite(RED_LED, HIGH);
   digitalWrite(GREEN_LED, HIGH);
   pinMode(BUTTON_1, INPUT);
   pinMode(BUTTON_2, INPUT);
   pinMode(BUZZER, OUTPUT);
   //default buzzer 
   digitalWrite(BUZZER,LOW);
   //7-Seg Display
   pinMode(DIGIT_1, OUTPUT);
   pinMode(DIGIT_2, OUTPUT);
   pinMode(DIGIT_3, OUTPUT);
   pinMode(DIGIT_4, OUTPUT);
   //Shift Register Pins
   pinMode(LATCH, OUTPUT);
   pinMode(CLOCK, OUTPUT);
   pinMode(DATA, OUTPUT);
   dispOff();

  //7-Seg Display Task
  xTaskCreate(TaskDisplay, "Display", 128,NULL,0,NULL);
  //Clock timer Task
  xTaskCreate(TaskClockTimer, "ClockTimer", 128, NULL, 0,&TaskClockTimer_Handler);
  stopTaskClockTimer();
  //Buzzer Task
  xTaskCreate(TaskBuzzer,"Buzzer",128, NULL, 0,NULL );
  //Read Button 2 Task
  xTaskCreate(TaskReadButton2,"ReadButton2", 128,NULL, 0,NULL);
  //Read Button 1 Task
  xTaskCreate(TaskReadButton1,"ReadButton1",128,  NULL, 0,NULL);
}

void loop() {}
/*
7-Seg Display Task
*/
void TaskDisplay(void * pvParameters) {
  (void) pvParameters;
  byte current_digit;
  for (;;) {
    dispOff();
    switch (current_digit){
      case 1: //0x:xx
        display( int((gCount/60) / 10) % 6, 0 );   // prepare to display digit 1 (most left)
        digitalWrite(DIGIT_1, LOW);  // turn on digit 1
        break;
      case 2: //x0:xx
        display( int(gCount / 60) % 10, 1 );   // prepare to display digit 2
        digitalWrite(DIGIT_2, LOW);     // turn on digit 2
        break;
      case 3: //xx:0x
        display( (gCount / 10) % 6, 0 );   // prepare to display digit 3
        digitalWrite(DIGIT_3, LOW);    // turn on digit 3
        break;
      case 4: //xx:x0
        display(gCount % 10, 0); // prepare to display digit 4 (most right)
        digitalWrite(DIGIT_4, LOW);  // turn on digit 4
    }
    current_digit = (current_digit % 4) + 1;
    vTaskDelay( 5 / portTICK_PERIOD_MS );
  }
}

/*
Clock Timer Task
*/
void TaskClockTimer(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    gCount--;
    if(gCount == 0) { 
      gBuzzerFlag = 1;
      gTimerRunning = 0;
      stopTaskClockTimer();
    }
    vTaskDelay( 1000 / portTICK_PERIOD_MS );
  }
}

/* 
Buzzer Task 
*/
void TaskBuzzer(void *pvParameters) {
  (void) pvParameters;
  unsigned char state = 0;
  for (;;) {
    if(gBuzzerFlag == 1) {      
      digitalWrite(RED_LED, HIGH);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(BUZZER, HIGH);
      vTaskDelay( 250 / portTICK_PERIOD_MS );
      digitalWrite(BUZZER, LOW);
      vTaskDelay( 250 / portTICK_PERIOD_MS );
    }
    else
      digitalWrite(BUZZER, LOW);
    vTaskDelay(1);
  }
}
/*
Read Button 2 Task
*/
void TaskReadButton2(void *pvParameters) {
  (void) pvParameters;
  unsigned int buttonState = 0; //reading push button status flag
  for (;;) { 
    //Read Button 2
    buttonState = digitalRead(BUTTON_2);
    if(buttonState == 1) {
      vTaskDelay( 250 / portTICK_PERIOD_MS );
      gCount++;
    }
    vTaskDelay(1);
  }
}
/*
Read Button 1 Task
*/
void TaskReadButton1(void *pvParameters) {
  (void) pvParameters;
  unsigned int buttonState = 0;
  for (;;)
  { 
    buttonState = digitalRead(BUTTON_1);
    if(buttonState == 1) {
      vTaskDelay( 250 / portTICK_PERIOD_MS );
      if(gTimerRunning == 0) {
        gTimerRunning = 1;
        if(gCount == 0) {
          gCount = DEFAULT_COUNT;
        }
        if(gBuzzerFlag == 1) {
          gBuzzerFlag = 0;
          digitalWrite(RED_LED, HIGH);
          digitalWrite(GREEN_LED, HIGH);
        }
        else {
          startTaskClockTimer();
          digitalWrite(RED_LED, LOW);
          digitalWrite(GREEN_LED, HIGH);
        }
      }
      else {
        stopTaskClockTimer();
        gTimerRunning = 0;
        digitalWrite(RED_LED, HIGH);
        digitalWrite(GREEN_LED, HIGH);
      }
    }
    vTaskDelay(1);
  }
}
void stopClockTimer() {
  vTaskSuspend(TaskClockTimer_Handler);
}
void startClockTimer() {
  vTaskResume(TaskClockTimer_Handler);
}
void dispOff() {
   digitalWrite(DIGIT_1, HIGH);
   digitalWrite(DIGIT_2, HIGH);
   digitalWrite(DIGIT_3, HIGH);
   digitalWrite(DIGIT_4, HIGH);
}
void display(unsigned char num, unsigned char dp) {
  digitalWrite(LATCH, LOW);
  shiftOut(DATA, CLOCK, MSBFIRST, gTable[num] | (dp<<7));
  digitalWrite(LATCH, HIGH);
}