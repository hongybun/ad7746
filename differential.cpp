#include <Wire.h>

#define AD7746_ADDR 0x48  // Default I2C address for AD7746

// AD7746 Register Map
#define STATUS_REG        0x00
#define CAP_DATA_HIGH     0x01
#define CAP_DATA_MID      0x02
#define CAP_DATA_LOW      0x03
#define CONFIG_REG        0x07
#define CAP_SETUP_REG     0x08
#define EXC_SETUP_REG     0x09
#define CAP_DAC_A_REG     0x0A
#define CAP_DAC_B_REG     0x0B

void setup() {
  Serial.begin(9600);
  Wire.begin();

  // Wait for sensor startup
  delay(100);

  // Setup Configuration Register
  writeRegister(CONFIG_REG, 0x00);  // Default: continuous conversion, no temperature
  writeRegister(CAP_SETUP_REG, 0x20);  // Enable capacitive channel in differential mode

  // Optional: Set DAC offset if your baseline capacitance exceeds ±4pF
  writeRegister(CAP_DAC_A_REG, 0x00);  // Adjust if needed
  writeRegister(CAP_DAC_B_REG, 0x00);

  delay(100); // Let settings take effect
}

void loop() {
  if (dataReady()) {
    long capRaw = readCapacitanceRaw();
    float capPF = rawToCapPF(capRaw);
    
    Serial.print("Capacitance: ");
    Serial.print(capPF, 6);
    Serial.println(" pF");
  }

  delay(200);
}

// Check if conversion is ready
bool dataReady() {
  Wire.beginTransmission(AD7746_ADDR);
  Wire.write(STATUS_REG);
  Wire.endTransmission(false);
  Wire.requestFrom(AD7746_ADDR, 1);
  uint8_t status = Wire.read();
  return (status & 0x80) == 0;  // Bit 7 = RDY (0 when data ready)
}

// Read 24-bit raw capacitance
long readCapacitanceRaw() {
  Wire.beginTransmission(AD7746_ADDR);
  Wire.write(CAP_DATA_HIGH);
  Wire.endTransmission(false);
  Wire.requestFrom(AD7746_ADDR, 3);

  uint32_t high = Wire.read();
  uint32_t mid  = Wire.read();
  uint32_t low  = Wire.read();

  long result = ((long)high << 16) | ((long)mid << 8) | low;
  return result;
}

// Convert raw code to pF (based on ±4 pF range and 24-bit scale)
float rawToCapPF(long code) {
  return (float)code / 16777216.0 * 8.0 - 4.0;  // ±4 pF range
}

// Write one byte to a register
void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(AD7746_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}
