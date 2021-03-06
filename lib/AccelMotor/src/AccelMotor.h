/* AccelMotor.h
 *      This file contains the definition for the AccelMotor class
 *
 *      This class extends the AirSpayce AccelStepper class by
 *      overriding the step function.
 *
 *      Inherited functions of note:
 *              void setSpeed(float)         - step/s
 *              void setAcceleration(float)  - step/s^2
 *              bool run()                   - 1 step, false if done
 */

#pragma once

#include <AccelStepper.h>
#include <HighPowerStepperDriver.h>
#include <utility>
#include <vector>

/* Forward Declarations */
class Encoder;

/* Class to Control Stepper Motors -- Uses AccelStepper */
class AccelMotor : public AccelStepper {
public:
    AccelMotor(uint8_t cs_pin,
               uint8_t lim_pin,
               uint8_t encA_pin,
               uint8_t encB_pin,
               uint16_t full_steps_per_rev,
               HPSDDecayMode mode = HPSDDecayMode::AutoMixed,
               uint16_t current_lim_mA = 1000,
               HPSDStepMode step_mode = HPSDStepMode::MicroStep1,
               bool use_enc = false);  // Constructor
    ~AccelMotor();                     // Destructor

    void set_max_angle(float angle_degrees);             // set limit
    bool set_angle(float angle_degrees, bool absolute);  // set the angle

    void step(long) override;  // from accelstepper
    long distanceToGo();       // from accelstepper
    bool run();                // from accelstepper
    bool runSpeedToPosition() =
        delete;  // from accelstepper -- encoder override breaks this

    static void calibrate(
        std::vector<AccelMotor *> mts);  // step until limit hit

private:
    /* Helpers */
    inline int16_t enc_to_step(int32_t enc_val) {
        return -1 * enc_val * m_steps_per_rev / enc_cpr;
    }

    /* Static functions */
    static void limit_switch_isr(void);

    /* Members */
    HighPowerStepperDriver *m_hpsd;
    Encoder *m_enc;  // encoder

    // Limits
    float m_max_angle_deg;

    // Relationships / Ratios
    uint16_t m_steps_per_rev;  // based on step mode
    float m_deg_to_step;
    uint16_t m_step_tol;

    // Flags
    bool m_at_limit;
    bool m_no_enc;

    /* Static Members */
    static std::vector<std::pair<uint8_t, AccelMotor *> > itr_list;
    static long calibrate_time_us;

    // Encoder stuff
    static const uint32_t enc_cpr = 1024;
    static const float step_tol_deg = 1.0;  // How close to target

    // Weightings
    static const float step_weight = 0.5;
    static const float enc_weight = 0.5;
};
