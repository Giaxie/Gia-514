#include <Arduino.h>

const int VOUT1_PIN = 6;  // 
const int VOUT2_PIN = 2;  // 

void setup() {
    Serial.begin(115200);
    adcAttachPin(VOUT1_PIN);  
    adcAttachPin(VOUT2_PIN);
    analogSetAttenuation(ADC_11db); 
}

void loop() {
    int adc1 = analogRead(VOUT1_PIN);
    int adc2 = analogRead(VOUT2_PIN);

    float voltage1 = (adc1 / 4095.0) * 3.3;
    float voltage2 = (adc2 / 4095.0) * 3.3;

    Serial.print("VOUT1: "); Serial.print(voltage1); Serial.println(" V");
    Serial.print("VOUT2: "); Serial.print(voltage2); Serial.println(" V");
    Serial.println("----------------------");

    delay(1000);
}