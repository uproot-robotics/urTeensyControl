#include "AccelMotor.h"
#include "SerialPacket.h"
#include "settings.h"
#include "util.h"

#include <Arduino.h>
#include <SPI.h>
#include <vector>

//#define USE_USB

#ifdef USE_USB
usb_serial_class& SerialPort = Serial;
#else
HardwareSerial& SerialPort = Serial1;
#endif

static inline void spi_setup() {
    SPI.setMOSI(PIN_MOSI);
    SPI.setMISO(PIN_MISO);
    SPI.setSCK(PIN_SCK);
    SPI.begin();
}

static inline void serial_setup() {
    SerialPort.begin(SER_BAUD_RATE);
    while (!SerialPort) {};
    SerialPort.println("");
}

int8_t get_char() {
    if (SerialPort.available() > 0) return SerialPort.read();
    return -1;
}

void blink() {
    digitalWrite(STD_LED, 1);
    delay(100);
    digitalWrite(STD_LED, 0);
    delay(100);
    digitalWrite(STD_LED, 1);
}

static inline void end_eff_on(bool on) {
    digitalWrite(PIN_RELAY, on);
}

static inline float deg_to_step(float deg) {
    return (deg / 360.0) * STEPS_PER_REV * STEP_MODE;
}

void setup() {
    // Setup comms first
    spi_setup();
    delay(1000);

    serial_setup();
    delay(1000);

    // Setup Motors
    AccelMotor sm1(PIN_M1_CS,
                   PIN_LIM1,
                   PIN_ENC1A,
                   PIN_ENC1B,
                   STEPS_PER_REV,
                   static_cast<HPSDDecayMode>(DECAY_MODE),
                   CURRENT_LIMIT_MA,
                   static_cast<HPSDStepMode>(STEP_MODE),
                   USE_ENC);
    AccelMotor sm2(PIN_M2_CS,
                   PIN_LIM2,
                   PIN_ENC2A,
                   PIN_ENC2B,
                   STEPS_PER_REV,
                   static_cast<HPSDDecayMode>(DECAY_MODE),
                   CURRENT_LIMIT_MA,
                   static_cast<HPSDStepMode>(STEP_MODE),
                   USE_ENC);
    AccelMotor sm3(PIN_M3_CS,
                   PIN_LIM3,
                   PIN_ENC3A,
                   PIN_ENC3B,
                   STEPS_PER_REV,
                   static_cast<HPSDDecayMode>(DECAY_MODE),
                   CURRENT_LIMIT_MA,
                   static_cast<HPSDStepMode>(STEP_MODE),
                   USE_ENC);

    std::vector<AccelMotor*> motors;
    motors.push_back(&sm1);
    motors.push_back(&sm2);
    motors.push_back(&sm3);

    float max_speed = deg_to_step(MAX_SPEED_DEG_S);
    float accel = deg_to_step(ACCEL_DEG_S_S);

    for (auto mtr : motors) {
        mtr->set_max_angle(MAX_ANGLE_DEG);
        mtr->setAcceleration(accel);
        mtr->setMaxSpeed(max_speed);
    }

    // Bring out of sleep
    pinMode(PIN_RELAY, OUTPUT);
    pinMode(PIN_N_SLP, OUTPUT);
    pinMode(PIN_RST, OUTPUT);
    digitalWrite(PIN_N_SLP, 1);
    digitalWrite(PIN_RST, 0);
    end_eff_on(false);

    delay(1);

    // Show alive
    pinMode(STD_LED, OUTPUT);
    digitalWrite(STD_LED, 1);

    // For serial comms
    bool msg_sent = false;
    std::vector<char> bytes;
    SerialUtils::CmdMsg* p_rx_msg = NULL;

    // Now calibrate everything
    AccelMotor::calibrate(motors);
    bool running_mtrs = false;
    SerialUtils::CmdMsg response;

    while (1) {
        if (SerialPort.available() > 0) {
            uint8_t next_byte = SerialPort.read();
            if (next_byte == SerialUtils::DELIMITER) {
                p_rx_msg = new SerialUtils::CmdMsg();
                SerialUtils::unpack(bytes, *p_rx_msg);
                bytes.clear();
            } else {
                bytes.push_back(next_byte);
            }
        }

        if (p_rx_msg) {
            switch (p_rx_msg->cmd_type) {
                case SerialUtils::CMDTYPE_MTRS:
                    for (int i = 0; i < motors.size(); i++)
                        motors[i]->set_angle(p_rx_msg->mtr_angles[i],
                                             !p_rx_msg->is_relative);
                    running_mtrs = true;
                    break;

                case SerialUtils::CMDTYPE_CAL:
                    AccelMotor::calibrate(motors);
                    break;

                case SerialUtils::CMDTYPE_ENDEFF_ON:
                    end_eff_on(true);
                    break;

                case SerialUtils::CMDTYPE_ENDEFF_OFF:
                    end_eff_on(false);
                    break;

                case SerialUtils::CMDTYPE_CONFIG:
                    for (auto mtr : motors) {
                        if (p_rx_msg->mtr_accel_deg_s_s) mtr->setAcceleration(deg_to_step(p_rx_msg->mtr_accel_deg_s_s));
                        if (p_rx_msg->mtr_speed_deg_s) mtr->setMaxSpeed(deg_to_step(p_rx_msg->mtr_speed_deg_s));
                    }
                    break;
            }

            response = *p_rx_msg;
            delete p_rx_msg;
            p_rx_msg = NULL;
            msg_sent = false;
        }

        bool done = true;
        for (auto& mtr : motors) done &= !mtr->run();

        // Let the controller know if movement completed
        if ((done || !running_mtrs) && !msg_sent) {
            response.cmd_success = true;
            std::vector<char> tx_buf;
            SerialUtils::pack(tx_buf, response);
            SerialPort.write((char*)&tx_buf[0], tx_buf.size());
            msg_sent = true;
            running_mtrs = false;
        }
    };
}

void loop() {}
