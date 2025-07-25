#include <Wire.h>

//AD7746 definitions
#define I2C_ADDRESS     0x48 //AD7746 default I2C address

// AD7746 Register Map
#define STATUS          0x00
#define CAP_DATA_H      0x01
#define CAP_DATA_M      0x02
#define CAP_DATA_L      0x03
#define VT_DATA_H       0x04
#define VT_DATA_M       0x05
#define VT_DATA_L       0x06
#define CAP_SETUP       0x07
#define VT_SETUP        0x08
#define EXC_SETUP       0x09
#define CONFIGURATION   0x0A
#define CAP_DAC_A       0x0B
#define CAP_DAC_B       0x0C
#define CAP_OFFSET_H    0x0D
#define CAP_OFFSET_L    0x0E
#define CAP_GAIN_H      0x0F
#define CAP_GAIN_L      0x10
#define VOLT_GAIN_H     0x11
#define VOLT_GAIN_L     0x12
#define RESET           0xBF

// Setup for continuous differential measurements
void setup()
{
    // Initialize serial port and communications
    Serial.begin(9600);
    Wire.begin();

    // Reset the device on startup
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(RESET); 
    Wire.endTransmission();
    delay(1); // Delay 1 ms for AD7746 to reset all of its settings

    Serial.println("Initializing differential measurement"); // Announce startup

    // Setup Registers
    writeRegister(CAP_SETUP, 0xA0); // Enable capacitive channel in differential mode
    writeRegister(EXC_SETUP, 0x09); // Enable excitation A with voltage level at +- VDD/4
    writeRegister(CONFIGURATION, 0xA1); // Continuous conversion enabled
    
    // Set CAPDAC offset. CAPDACs are uncalibrated and can vary by +-20% between AD7746 devices
    float coarseOffset = 0; // Adjust negative offset in steps of 0.125
    byte capdac = (byte)(coarseOffset / 0.125); 
    //writeRegister(CAP_DAC_A, capdac | 0x80); // Write CAPDAC value to CAPDAC A and enable it
    writeRegister(CAP_DAC_A, capdac | 0x00); // Disable CAPDAC A
    writeRegister(CAP_DAC_B, 0x00); // Disable CAPDAC B

    // Set fine capacitance offset. Currently changing values unexpectedly, can't be bothered to figure out why. Low priority.
    /*
    //long capRaw = readCapacitanceRaw(); // Convert raw code to pF (based on 24-bit scale)
    //float fineOffset = (float)capRaw / 16777216.0 * 8.192; // Calculate uncalibrated capacitance
    float fineOffset = -0.1;
    int16_t offsetRaw = (int16_t)(-fineOffset * 65536.0);
    writeRegister(CAP_OFFSET_H, (offsetRaw >> 8) & 0xFF);
    writeRegister(CAP_OFFSET_L, offsetRaw & 0xFF);
    */

    delay(100); // Let settings take effect and wait for environment to stabilize for 100 ms
}

unsigned long startTime = millis();

// Main data collection loop
void loop() {
    if (dataReady()) {
        long capRaw = readCapacitanceRaw(); // Convert raw code to pF (based on 24-bit scale)
        float capPF = (float)capRaw / 16777216.0 * 8.192 - 4.096; // Calculate uncalibrated capacitance
        Serial.print("Time: ");
        Serial.print((millis() - startTime) / 1000.0);
        Serial.print(" seconds  ");
        Serial.print("Capacitance: ");
        Serial.print(capPF, 6);
        Serial.println(" pF");
    }
    delay(250); // Sample period in ms
}

// Write one byte to a register
void writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

// Check if conversion is ready
bool dataReady() {
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(STATUS);
    Wire.endTransmission(false);
    Wire.requestFrom(I2C_ADDRESS, 1);
    uint8_t status = Wire.read();
    return (status & 0x80) == 0;  // Bit 7 = RDY (0 when data ready)
}

// Read 24-bit raw capacitance. The 24 bits are separated into 3 8-bit registers.
long readCapacitanceRaw() {
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(CAP_DATA_H); // Double check this
    Wire.endTransmission(false);
    Wire.requestFrom(I2C_ADDRESS, 3); // Double check this

    uint32_t high = Wire.read();
    uint32_t mid  = Wire.read();
    uint32_t low  = Wire.read();

    long result = ((long)high << 16) | ((long)mid << 8) | low;
    return result;
}
