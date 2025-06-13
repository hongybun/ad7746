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

// Setup for continuous single-ended measurements

void setup()
{
    

    Serial.begin(9600);
    Wire.begin();
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(RESET);  // Reset the device on startup
    Wire.endTransmission();
    delay(1);                       // Delay needed for AD7746 to reset all of its settings (200 us)

    Serial.println("Initializing"); // Announce startup

    // Setup Registers
    writeRegister(CONFIGURATION, 0xA1); // Continuous conversion, no temperature, CAPDAC A enabled
    writeRegister(CAP_SETUP, 0x80);     // Enable capacitive channel in single-ended mode
    writeRegister(EXC_SETUP, 0x09);     // Enable excitation A, with voltage level at +- VDD/4

    // Set CAPDAC offset
    float coarseOffset = 4.625;                 // -4.625 pF offset
    byte capdac = (byte)(coarseOffset / 0.125); 
    writeRegister(CAP_DAC_A, capdac | 0x80);    // Write CAPDAC value to CAPDAC A and enable it
    writeRegister(CAP_DAC_B, 0x00);             // Disable CAPDAC B

    // Set fine capacitance offset
    /*
    //long capRaw = readCapacitanceRaw();                       // Convert raw code to pF (based on 24-bit scale)
    //float fineOffset = (float)capRaw / 16777216.0 * 8.192;    // Calculate uncalibrated capacitance
    float fineOffset = -0.1;
    int16_t offsetRaw = (int16_t)(-fineOffset * 65536.0);
    writeRegister(CAP_OFFSET_H, (offsetRaw >> 8) & 0xFF);
    writeRegister(CAP_OFFSET_L, offsetRaw & 0xFF);
    */

    delay(100); // Let settings take effect and wait for environment to stabilize
}

unsigned long startTime = millis();

void loop() {
    if (dataReady()) {
        long capRaw = readCapacitanceRaw(); // Convert raw code to pF (based on 24-bit scale)
        float capPF = (float)capRaw / 16777216.0 * 8.192; // Calculate uncalibrated capacitance
        Serial.print("Time: ");
        Serial.print((millis() - startTime) / 1000.0);
        Serial.print(" Capacitance: ");
        Serial.print(capPF, 4);
        Serial.println(" pF");
    }

    delay(200);
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

// Read 24-bit raw capacitance
long readCapacitanceRaw() {
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(CAP_DATA_H);
    Wire.endTransmission(false);
    Wire.requestFrom(I2C_ADDRESS, 3);

    uint32_t high = Wire.read();
    uint32_t mid  = Wire.read();
    uint32_t low  = Wire.read();

    long result = ((long)high << 16) | ((long)mid << 8) | low;
    return result;
}
