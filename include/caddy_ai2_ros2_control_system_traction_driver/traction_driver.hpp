#pragma once

#include <linux/can.h>
#include <vector>
#include <cstdint>
#include <chrono>
#include <string>
#include <cstring>
#include <iostream>

#include "caddy_ai2_ros2_common/socket_can_interface.hpp"

/*
    Description:
    This class is a wrapper for the Curtis Motor Driver.
    It is used to control the Curtis Motor Driver used as traction system
    of the Caddy AI2.
*/

using namespace std;

#define RESET_CODE	            0x4850
#define MOTOR_DIRECTION		    -1	// +1 REGULAR, -1 INVERTED

#define CURTIS_SPEED_DIVISOR	10.0
#define KMH_TO_MPS				0.277777778


/*
    When enter in communication, sends the following frames
    in the order specified below.

    There are two frames more like FRAME_226 and FRAME_726.
*/
#define FRAME_226 0x226 // Throttle command frame
#define FRAME_227 0x227 // First
#define FRAME_327 0x327 // Second - Not used, raw data all zeros all the time
#define FRAME_1A6 0x1A6 // Third
#define FRAME_2A6 0x2A6 // Fourth
#define FRAME_726 0x726 // When communication is not established
    // Motor data

class TractionDriver {
public:
    TractionDriver(bool verbose = false);
    ~TractionDriver();

    static constexpr int FRAME_226_ = FRAME_226;
    static constexpr int FRAME_227_ = FRAME_227;
    static constexpr int FRAME_327_ = FRAME_327;
    static constexpr int FRAME_1A6_ = FRAME_1A6;
    static constexpr int FRAME_2A6_ = FRAME_2A6;
    static constexpr int FRAME_726_ = FRAME_726;

    // Process CAN frames and update internal data
    std::vector<bool> process_frames(const std::vector<struct can_frame>& frames);
    
    // Prepare a throttle command frame
    bool create_0x226_frame(struct can_frame* frame, int throttle_value, bool reset = false);
    bool create_throttle_frame(struct can_frame* frame, int throttle_value, bool reset = false){
        return create_0x226_frame(frame, throttle_value, reset);
    }

    // Getters for motor data
    int16_t get_raw_desired_throttle() const { return raw_desired_throttle_; }
    double get_desired_throttle() const { return desired_throttle_; }

    bool get_interlock() const { return interlock_; }
    bool get_on_fault() const { return on_fault_; }
    bool get_mode_auto() const { return mode_auto_; }
    bool get_mode_manual() const { return mode_manual_; }
    int32_t get_fault_code() const { return fault_code_; }
    std::string get_fault_description() const { return fault_description_; }

    double get_current_rms() const { return current_rms_; }
    double get_battery_current() const { return battery_current_; }
    double get_keyswitch_voltage() const { return keyswitch_voltage_; }
    double get_bdi_percentage() const { return bdi_percentage_; }

    double get_motor_rpm() const { return motor_rpm_; }
    double get_speed() const { return speed_; }
    double get_controller_temp() const { return controller_temp_; }
    double get_motor_temp() const { return motor_temp_; }

    std::string getFaultString(uint8_t fault_code);
    
    // Get last update timestamp for any data
    uint64_t get_last_update_0x226_frame() const { return last_update_0x226_frame_; }
    uint64_t get_last_update_0x227_frame() const { return last_update_0x227_frame_; }
    // uint64_t get_last_update_0x327_frame() const { return last_update_0x327_frame_; }
    uint64_t get_last_update_0x1A6_frame() const { return last_update_0x1A6_frame_; }
    uint64_t get_last_update_0x2A6_frame() const { return last_update_0x2A6_frame_; }
    // uint64_t get_last_update_0x726_frame() const { return last_update_0x726_frame_; }

private:
    bool verbose_; // Verbose mode for debugging

    // Process specific CAN IDs
    bool process_0x227_frame(const struct can_frame& frame);
    // bool process_0x327_frame(const struct can_frame& frame);
    bool process_0x1A6_frame(const struct can_frame& frame);
    bool process_0x2A6_frame(const struct can_frame& frame);
    // bool process_0x726_frame(const struct can_frame& frame);

    // Info from frame 0x226
    int16_t raw_desired_throttle_ = 0;  // Raw desired throttle value (-32767 to 32767)
    double desired_throttle_ = 0.0f;     // Desired throttle percentage (-100% to 100%)

    // Info from frame 0x226
    bool interlock_ = false;            // Interlock state
    bool on_fault_ = false;             // Fault state
    bool mode_auto_ = false;            // Automatic mode
    bool mode_manual_ = false;          // Manual mode
    uint8_t fault_code_ = 0;            // Fault code
    std::string fault_description_ = ""; // Fault description
    
    // Info from frame 0x1A6
    double current_rms_ = 0.0f;          // Amps
    double battery_current_ = 0.0f;      // Amps
    double keyswitch_voltage_ = 0.0f;    // Volts
    double bdi_percentage_ = 0.0f;       // Percentage

    // Info from frame 0x2A6
    double motor_rpm_ = 0.0f;        // RPM
    double speed_ = 0.0f;            // m/s
    double controller_temp_ = 0.0f;  // Celsius
    double motor_temp_ = 0.0f;       // Celsius
    
    // Incremental counter to see the last data update
    uint64_t last_update_0x226_frame_ = 0;
    uint64_t last_update_0x227_frame_ = 0;
    // uint64_t last_update_0x327_frame_ = 0
    uint64_t last_update_0x1A6_frame_ = 0;
    uint64_t last_update_0x2A6_frame_ = 0;
    // uint64_t last_update_0x726_frame_ = 0;
};