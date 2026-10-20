#include <Arduino.h>
#include <HX711.h>

// HX711 circuit wiring
const int DOUT_PIN = 6;
const int SCK_PIN = 5;

HX711 hx;

String readInput();
void test_weight();
void calibration();
void pre_defined_calibration();

void setup() {
    Serial.begin(115200);    
    hx.begin(DOUT_PIN, SCK_PIN);

    delay(3000);
}

void loop() {
    Serial.printf("Choose one: \n[0] Calibration\n[1] Test weight\n[2] Use pre-defined Calibrations\n");
    int choice = readInput().toInt();

    if (choice == 0){
        // Calibration
        calibration();
    }
    else if (choice == 1) {
        // Test weight
        test_weight();
    }
    else if (choice == 2){
        // Set pre difined calibrations
        pre_defined_calibration();
    }
    delay(100);
}

/**
 * @brief Reads input from Serial.
 * 
 * @return String of Serial input.
 */
String readInput() {
    while (true) {
        if (Serial.available()) {
            String input = Serial.readString();
            input.trim();

            return input;
        }
        delay(100);
    }
}

void test_weight(){
    Serial.printf("Enter how many measurments that should be taken: ");
    int n = readInput().toInt();

    Serial.println("Wait 10s for stabilization.");
    delay(10000);

    float measurment;
    float tot = 0.0;
    for (int i = 0; i < n; i++){
        measurment = hx.get_units(10);
        Serial.println(measurment);

        tot += measurment;
        delay(500);
    }

    Serial.print("\nAverage = ");
    Serial.println(tot/(float)n);
}

void pre_defined_calibration(){
    // float scale = -396.19039917;
    // long offset = 58586;

    float scale = -396.06430054;
    long offset = 58711;

    hx.set_scale();
    hx.tare(20);
    hx.set_scale(scale);
}


void calibration() {
    Serial.println("--------- START CALIBRATION -----------");
    Serial.println("Remove any items from the scale.");
    Serial.println("Press Enter when ready.");
    readInput();
    // Take zero-measurement
    long offset = hx.read_average(30);
    hx.set_offset(offset);

    Serial.println("\nPlace an item of known weight on the scale.");
    Serial.println("Press Enter when ready.");
    readInput();
    // Take reference measurment
    delay(5000); // Let it settle
    long measured_value = hx.read_average(30) - hx.get_offset();

    Serial.println("Take of the weight.");
    Serial.println("Enter the item's weight in grams:");
    float item_weight = readInput().toFloat();

    // Calculate scale fraction
    float scale = (float)measured_value / item_weight;
    hx.set_scale(scale);

    Serial.print("Scale: ");
    Serial.println(scale, 8);
    Serial.print("Offset: ");
    Serial.println(hx.get_offset());

    Serial.println("\nCalibration complete!");
}