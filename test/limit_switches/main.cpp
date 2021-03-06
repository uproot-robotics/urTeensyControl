#include "util.h"
#include "SerialPacket.h"

#include <Arduino.h>

#define DEBUG

static inline void serial_setup()
{
    Serial.begin(SER_BAUD_RATE);
    while (!Serial){};
#ifdef DEBUG    
    Serial.println("Starting...");
#endif
}

void ISR1(void) {
    Serial.println("In 1");
}
void ISR2(void) {
    Serial.println("In 2");
}
void ISR3(void) {
    Serial.println("In 3");
}

void setup() 
{
    serial_setup();
    pinMode(PIN_LIM1, INPUT);
    pinMode(PIN_LIM2, INPUT);
    pinMode(PIN_LIM3, INPUT);

    delay(1);
    
    pinMode(STD_LED, OUTPUT);
    digitalWrite(STD_LED, 1);

    attachInterrupt(PIN_LIM1, ISR1, FALLING);
    attachInterrupt(PIN_LIM2, ISR2, FALLING);
    attachInterrupt(PIN_LIM3, ISR3, FALLING);

    SerialUtils::CmdMsg tosend = {0};
    tosend.motors_done = true;
    std::vector<char> tx_buf;
    SerialUtils::pack(tx_buf, tosend);
    Serial.write((char *)&tx_buf[0], tx_buf.size());
    
    SerialUtils::CmdMsg rx = {0};
    SerialUtils::unpack(tx_buf, rx);
    Serial.println(rx.motors_done);
    
    while(1) {
//        Serial.println(digitalRead(PIN_LIM1));
    };
}

void loop() {}
