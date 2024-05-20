#include "Arduino.h"

#define LED1 15
#define LED2 18
#define LED3 21
#define BAUD 9600

int VAL;
void setup() {
  VAL = 0;
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  Serial1.begin(BAUD);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);
}

void loop() {
  if (Serial1.available()) {
    digitalWrite(LED1, HIGH);
  }
  delay(10);
}
