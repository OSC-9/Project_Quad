#include <Arduino.h>

// Hardware Pin Definitions
#define SBUS_RX_PIN          16      // GPIO 16 (Hardware UART2 RX)
#define SBUS_BAUD            100000  // Standard inverted SBUS baud rate

// SBUS Protocol Frame Constants
#define SBUS_FRAME_SIZE      25
#define SBUS_HEADER          0x0F
#define SBUS_FOOTER          0x00

// Calibration Constants (Futaba / FrSky standard)
#define SBUS_RAW_MIN         173     // Raw minimum bit value (1000us)
#define SBUS_RAW_MAX         1812    // Raw maximum bit value (2000us)
#define PWM_OUT_MIN          1000    // PWM Minimum Pulse Width (us)
#define PWM_OUT_MAX          2000    // PWM Maximum Pulse Width (us)
#define STICK_DEADBAND       8       // Deadband microsecond noise threshold

// Structures
struct SBUS_Channels {
    uint16_t raw[16];        // 11-bit raw channel values (173 - 1812)
    uint16_t pwm[16];        // Calculated pulse width (1000us - 2000us)
    bool     ch17;
    bool     ch18;
    bool     frame_lost;
    bool     failsafe;
};

SBUS_Channels rx_data;

// Internal Frame Parsing Buffers
uint8_t sbus_buffer[SBUS_FRAME_SIZE];
uint8_t buffer_index = 0;

// Function Declarations
bool parse_sbus_frame(const uint8_t* buffer, SBUS_Channels* channels);
uint16_t map_raw_to_pwm(uint16_t raw_val);
uint16_t apply_deadband(uint16_t pwm_val, uint16_t center_val);

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000);

    // Initialize UART2 with Hardware Signal Inversion enabled for SBUS
    Serial2.begin(SBUS_BAUD, SERIAL_8E2, SBUS_RX_PIN, -1, true);

    Serial.println(F("[SYSTEM] SBUS Hardware Receiver Initialized on RX Pin 16"));
}

void loop() {
    // Process incoming UART bytes non-blockingly
    while (Serial2.available() > 0) {
        uint8_t in_byte = Serial2.read();

        // Sync header detection
        if (buffer_index == 0 && in_byte != SBUS_HEADER) {
            continue; // Skip out-of-sync bytes
        }

        sbus_buffer[buffer_index++] = in_byte;

        // Full 25-byte frame received
        if (buffer_index >= SBUS_FRAME_SIZE) {
            buffer_index = 0; // Reset index for next frame

            // Verify end byte and parse
            if (sbus_buffer[24] == SBUS_FOOTER || (sbus_buffer[24] & 0x0F) == 0x04) {
                if (parse_sbus_frame(sbus_buffer, &rx_data)) {
                    
                    // Display Channel 1-4 (Sticks) & Safety Status every 100ms
                    static uint32_t last_print = 0;
                    if (millis() - last_print >= 100) {
                        last_print = millis();

                        Serial.printf("[SBUS] Roll: %4dμs | Pitch: %4dμs | Thr: %4dμs | Yaw: %4dμs | Failsafe: %s\n",
                            rx_data.pwm[0],
                            rx_data.pwm[1],
                            rx_data.pwm[2],
                            rx_data.pwm[3],
                            rx_data.failsafe ? "ACTIVE (CRITICAL)" : "OK"
                        );
                    }
                }
            }
        }
    }
}

/**
 * @brief Unpacks 11-bit channel bitfields from 25-byte SBUS frame
 */
bool parse_sbus_frame(const uint8_t* buf, SBUS_Channels* ch) {
    // 11-bit Channel Decoding Bitmasking
    ch->raw[0]  = (uint16_t)((buf[1]       | buf[2]  << 8)                     & 0x07FF);
    ch->raw[1]  = (uint16_t)((buf[2]  >> 3 | buf[3]  << 5)                     & 0x07FF);
    ch->raw[2]  = (uint16_t)((buf[3]  >> 6 | buf[4]  << 2 | buf[5] << 10)      & 0x07FF);
    ch->raw[3]  = (uint16_t)((buf[5]  >> 1 | buf[6]  << 7)                     & 0x07FF);
    ch->raw[4]  = (uint16_t)((buf[6]  >> 4 | buf[7]  << 4)                     & 0x07FF);
    ch->raw[5]  = (uint16_t)((buf[7]  >> 7 | buf[8]  << 1 | buf[9] << 9)       & 0x07FF);
    ch->raw[6]  = (uint16_t)((buf[9]  >> 2 | buf[10] << 6)                     & 0x07FF);
    ch->raw[7]  = (uint16_t)((buf[10] >> 5 | buf[11] << 3)                     & 0x07FF);
    ch->raw[8]  = (uint16_t)((buf[12]      | buf[13] << 8)                     & 0x07FF);
    ch->raw[9]  = (uint16_t)((buf[13] >> 3 | buf[14] << 5)                     & 0x07FF);
    ch->raw[10] = (uint16_t)((buf[14] >> 6 | buf[15] << 2 | buf[16] << 10)     & 0x07FF);
    ch->raw[11] = (uint16_t)((buf[16] >> 1 | buf[17] << 7)                     & 0x07FF);
    ch->raw[12] = (uint16_t)((buf[17] >> 4 | buf[18] << 4)                     & 0x07FF);
    ch->raw[13] = (uint16_t)((buf[18] >> 7 | buf[19] << 1 | buf[20] << 9)      & 0x07FF);
    ch->raw[14] = (uint16_t)((buf[20] >> 2 | buf[21] << 6)                     & 0x07FF);
    ch->raw[15] = (uint16_t)((buf[21] >> 5 | buf[22] << 3)                     & 0x07FF);

    // Flags byte decoding
    ch->ch17       = (buf[23] & (1 << 0)) ? true : false;
    ch->ch18       = (buf[23] & (1 << 1)) ? true : false;
    ch->frame_lost = (buf[23] & (1 << 2)) ? true : false;
    ch->failsafe   = (buf[23] & (1 << 3)) ? true : false;

    // Convert raw bits into calibrated microsecond PWM values
    for (int i = 0; i < 16; i++) {
        uint16_t mapped_pwm = map_raw_to_pwm(ch->raw[i]);
        
        // Apply stick deadband filter for Roll, Pitch, Yaw
        if (i == 0 || i == 1 || i == 3) {
            ch->pwm[i] = apply_deadband(mapped_pwm, 1500);
        } else {
            ch->pwm[i] = mapped_pwm;
        }
    }

    return true;
}

/**
 * @brief Converts 11-bit raw integer (173..1812) to standard PWM duration (1000..2000us)
 */
uint16_t map_raw_to_pwm(uint16_t raw_val) {
    if (raw_val <= SBUS_RAW_MIN) return PWM_OUT_MIN;
    if (raw_val >= SBUS_RAW_MAX) return PWM_OUT_MAX;

    return (uint16_t)(PWM_OUT_MIN + ((float)(raw_val - SBUS_RAW_MIN) / (float)(SBUS_RAW_MAX - SBUS_RAW_MIN)) * (PWM_OUT_MAX - PWM_OUT_MIN));
}

/**
 * @brief Applies center deadband filter to stabilize noisy remote stick inputs
 */
uint16_t apply_deadband(uint16_t pwm_val, uint16_t center_val) {
    int16_t error = (int16_t)pwm_val - (int16_t)center_val;
    if (abs(error) <= STICK_DEADBAND) {
        return center_val;
    }
    return pwm_val;
}
