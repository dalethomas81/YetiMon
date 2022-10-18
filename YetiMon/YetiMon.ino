
const int analogInPin = A2;
int sensorValue = 0;
float voltageValue = 0.0;
const float upperLimit = 11.00;
const float lowerLimit = 10.28; // 10.41v is about 15% | 10.28v is 10%
// note: we see about 300mV increase with 600w power supply on.

#define AVG_VOLTAGE_SIZE 100
float voltageReadings[AVG_VOLTAGE_SIZE] = {0.0};
int voltageArrayIndex = 0;
float totalVoltage = 0.0;
float averageVoltage = 0.0;

const int outputPin1 =  15;
int outputPin1State = LOW;

const long limitDelay = 60000; // how long to be under/over the limits
unsigned long previousLimitMillis = 0;

const long sampleInterval = 100;
unsigned long previousSampleMillis = 0;

const long printInterval = 1000;
unsigned long previousPrintMillis = 0;
  
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  pinMode(outputPin1, OUTPUT);
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousSampleMillis >= sampleInterval) {
    previousSampleMillis = currentMillis;
    
    sensorValue = analogRead(analogInPin);
    // sensorValue * scaled-input-to-controller * scaled-input-to-voltage-divider
    //voltageValue = sensorValue * (5.0 / 1023.0) * (15.0 / 5.0);
    voltageValue = sensorValue * (3.3 / 4095.0) * (15.0 / 2.4);

    // smoothing
    voltageReadings[voltageArrayIndex] = voltageValue;
    if (voltageArrayIndex >= AVG_VOLTAGE_SIZE - 1){
      voltageArrayIndex = 0;
    } else {
      voltageArrayIndex++;
    }
    totalVoltage = 0.0;
    for (int i = 0; i <= AVG_VOLTAGE_SIZE - 1; i++){
      totalVoltage = totalVoltage + voltageReadings[i];
    }
    averageVoltage = totalVoltage / AVG_VOLTAGE_SIZE;

  }

  if (averageVoltage < lowerLimit){
    if (currentMillis - previousLimitMillis >= limitDelay) {
      outputPin1State = HIGH;
    }
  } else if (averageVoltage > upperLimit) {
    if (currentMillis - previousLimitMillis >= limitDelay) {
      outputPin1State = LOW;
    }
  } else {
    previousLimitMillis = currentMillis;
  }
  
  digitalWrite(outputPin1, outputPin1State);
  
  if (currentMillis - previousPrintMillis >= printInterval) {
    previousPrintMillis = currentMillis;
    Serial.print("sensor=");
    Serial.print(sensorValue);  
    Serial.print(" voltage=");
    Serial.print(voltageValue);  
    Serial.print(" | avg=");
    Serial.print(averageVoltage); 
    Serial.print(" | output1=");
    Serial.print(outputPin1State);
    Serial.println();
  }
  
}
