#include "caddy_ai2_ros2_control_system_traction_driver/traction_driver.hpp"

TractionDriver::TractionDriver(bool verbose)
:   verbose_(verbose)
{
    if (verbose_) {
        std::cout << "TractionDriver initialized with verbose mode enabled." << std::endl;
    }
}

TractionDriver::~TractionDriver() {
    if (verbose_) {
        std::cout << "TractionDriver destroyed." << std::endl;
    }
}

std::vector<bool> TractionDriver::process_frames(const std::vector<struct can_frame>& frames) {
    if (verbose_) {
        std::cout << "Processing " << frames.size() << " CAN frames." << std::endl;
    }
    std::vector<bool> results;
    for (const auto& frame : frames) {
        bool processed = false;
        switch (frame.can_id) {
            case FRAME_227:
                processed = process_0x227_frame(frame);
                break;
            case FRAME_327:
                // processed = process_0x327_frame(frame); // Sin uso
                processed = true; // For now, just mark as processed
                break;
            case FRAME_1A6:
                processed = process_0x1A6_frame(frame);
                break;
            case FRAME_2A6:
                processed = process_0x2A6_frame(frame);
                break;
            case FRAME_726:
                // processed = process_0x726_frame(frame); // Sin uso
                processed = true; // For now, just mark as processed
                break;
            default:
                if (verbose_) {
                    std::cout << "Unknown CAN frame ID: " << frame.can_id << std::endl;
                }
                processed = false;
                break;
        }
        results.push_back(processed);
    }
    return results;
}

/**
 * @brief Processes a CAN frame with ID 0x227.
 * 
 * Process the received message.
 * 
 * Values received in the CAN frame:
 * 1. CAN_Master_Commands [Byte 0]:
 *    - Bit 0: CAN_Master_interlock_state
 *    - Bit 1: CAN_Master_fault_status
 *    - Bit 2: CAN_Master_Mode_Auto
 *    - Bit 3: CAN_Master_Mode_Manual
 *    - Bits 4-7: Reserved.
 * 
 * 2. CAN_Master_fault_code [Byte 1]:
 *    - Represents the fault code sent by the master.
 * 
 * 3-8. Reserved.
 * 
 * This function extracts the `CAN_Master_Commands` and `CAN_Master_fault_code` values 
 * from the CAN frame. It updates the corresponding member variables and records the 
 * timestamp of the last update. If the frame is invalid (e.g., incorrect length), the 
 * function returns `false`.
 * 
 * @param frame The CAN frame to process.
 * @return true if the frame was successfully processed, false otherwise.
 */
bool TractionDriver::process_0x227_frame(const struct can_frame& frame) {
    if (verbose_) {
        std::cout << "Processing frame with ID 0x227." << std::endl;
    }
    if (frame.len == 8) {
        // Extract CAN_Master_Commands (Byte 0)
        uint8_t can_master_commands = frame.data[0];
        interlock_ = can_master_commands & (1 << 0);
        on_fault_ = can_master_commands & (1 << 1);
        mode_auto_ = can_master_commands & (1 << 2);
        mode_manual_ = can_master_commands & (1 << 3);

        // Extract CAN_Master_fault_code (Byte 1)
        fault_code_ = frame.data[1];

        // Update the timestamp
        last_update_0x227_frame_++;

        if (verbose_) {
            std::cout << "CAN_Master_Commands: "
                      << "interlock_state=" << interlock_
                      << ", fault_status=" << on_fault_
                      << ", mode_auto=" << mode_auto_
                      << ", mode_manual=" << mode_manual_ << std::endl;

            fault_description_ = getFaultString(fault_code_);
            std::cout << "CAN_Master_fault_code: " << static_cast<int>(fault_code_)
                      << " (" << fault_description_ << ")" << std::endl;
        }
        return true;
    }
    return false;
}

/**
 * @brief Processes a CAN frame with ID 0x327.
 * 
 * This function processes a CAN frame with ID 0x327. If the frame contains all zeros 
 * or unknown data, it prints the raw frame data in a clear format for debugging purposes.
 * 
 * The frame is expected to have a length of 8 bytes. If the length is invalid, the function 
 * returns `false`. Otherwise, it prints the raw data and updates the timestamp.
 * 
 * @param frame The CAN frame to process.
 * @return true if the frame was successfully processed, false otherwise.
 */
/*
bool TractionDriver::process_0x327_frame(const struct can_frame& frame) {
    if (verbose_) {
        std::cout << "Processing frame with ID 0x327." << std::endl;
    }
    if (frame.len == 8) {
        // Print the raw data of the frame for debugging
        if (verbose_) {
            std::cout << "Raw frame data (ID: 0x327): ";
            for (int i = 0; i < frame.len; ++i) {
                std::cout << "0x" << std::hex << static_cast<int>(frame.data[i]) << " ";
            }
            std::cout << std::dec << std::endl; // Switch back to decimal output
        }

        // Update the timestamp
        last_update_0x327_frame_++;

        return true;
    }
    return false;
}
*/

/**
 * @brief Processes a CAN frame with ID 0x1A6.
 * 
 * Process the received message.
 * 
 * Values received:
 * 1	Current (RMS)			LO
 * 2	Current (RMS)			HI
 * 3	Battery Current			LO
 * 4	Battery Current			HI
 * 5 	Keyswitch voltage		LO
 * 6 	Keyswitch voltage		HI
 * 7 	BDI percentage			LO
 * 8 	BDI percentage			HI
 * 
 * @param frame The CAN frame to process.
 * @return true if the frame was successfully processed, false otherwise.
 */
bool TractionDriver::process_0x1A6_frame(const struct can_frame& frame) {
    if (verbose_) {
        std::cout << "Processing frame with ID 0x1A6." << std::endl;
    }
    if (frame.len == 8) {
        // Unpack Current (RMS)
        int16_t current_rms_raw = (frame.data[1] << 8) | frame.data[0];
        current_rms_ = static_cast<double>(current_rms_raw) * 0.1f;

        // Unpack Battery Current
        int16_t battery_current_raw = (frame.data[3] << 8) | frame.data[2];
        battery_current_ = static_cast<double>(battery_current_raw) * 0.1f;

        // Unpack Keyswitch Voltage
        int16_t keyswitch_voltage_raw = (frame.data[5] << 8) | frame.data[4];
        keyswitch_voltage_ = static_cast<double>(keyswitch_voltage_raw) * 0.01f;

        // Unpack BDI Percentage
        int16_t bdi_percentage_raw = (frame.data[7] << 8) | frame.data[6];
        bdi_percentage_ = static_cast<double>(bdi_percentage_raw);

        // Update the timestamp
        last_update_0x1A6_frame_++;

        if (verbose_) {
            std::cout << "current_rms_: " << current_rms_ 
                      << ", battery_current_: " << battery_current_ 
                      << ", keyswitch_voltage_: " << keyswitch_voltage_ 
                      << ", bdi_percentage_: " << bdi_percentage_ 
                      << std::endl;
        }
        return true;
    }
    return false;
}

/**
 * @brief Processes a CAN frame with ID 0x2A6.
 * 
 * Process the received message.
 * 
 * Values received:
 * 1	Motor RPM				LO
 * 2	Motor RPM				HI
 * 3	Vehicle Speed			LO
 * 4	Vehicle Speed			HI
 * 5 	Master Controller Temp	LO
 * 6 	Master Controller Temp	HI
 * 7 	Master Motor Temp		LO
 * 8 	Master Motor Temp		HI
 * 
 * @param frame The CAN frame to process.
 * @return true if the frame was successfully processed, false otherwise.
 */
bool TractionDriver::process_0x2A6_frame(const struct can_frame& frame) {
    if (verbose_) {
        std::cout << "Processing frame with ID 0x2A6." << std::endl;
    }
    if (frame.len == 8) {
        // Extract Motor RPM
        // #TODO: Check the sign of the RPM value, maybe we need to change the sign.
        int16_t motor_rpm_raw = (frame.data[1] << 8) | frame.data[0];
        motor_rpm_ =  static_cast<double>(std::abs(motor_rpm_raw));

        // Extract Vehicle Speed
        int16_t speed_raw = (frame.data[3] << 8) | frame.data[2];
        speed_ = (static_cast<double>(speed_raw) / CURTIS_SPEED_DIVISOR) * KMH_TO_MPS;

        // Extract Master Controller Temperature
        int16_t controller_temp_raw = (frame.data[5] << 8) | frame.data[4];
        controller_temp_ = static_cast<double>(controller_temp_raw) * 0.1f;

        // Extract Master Motor Temperature
        int16_t motor_temp_raw = (frame.data[7] << 8) | frame.data[6];
        motor_temp_ = static_cast<double>(motor_temp_raw) * 0.1f;

        // Update the timestamp
        last_update_0x2A6_frame_++;

        if (verbose_) {
            std::cout << "motor_rpm_: " << motor_rpm_
                      << ", speed_: " << speed_
                      << ", controller_temp_: " << controller_temp_
                      << ", motor_temp_: " << motor_temp_ << std::endl;
        }
        return true;
    }

    return false;
}

/**
 * @brief Processes a CAN frame with ID 0x726.
 * 
 * This function processes a CAN frame with ID 0x726. If the frame contains all zeros 
 * or unknown data, it prints the raw frame data in a clear format for debugging purposes.
 * 
 * The frame is expected to have a length of 1 byte. If the length is invalid, the function 
 * returns `false`. Otherwise, it prints the raw data and updates the timestamp.
 * 
 * @param frame The CAN frame to process.
 * @return true if the frame was successfully processed, false otherwise.
 */
/*
bool TractionDriver::process_0x726_frame(const struct can_frame& frame) {
    if (verbose_) {
        std::cout << "Processing frame with ID 0x726." << std::endl;
    }
    if (frame.len == 1) {
        // Print the raw data of the frame for debugging
        if (verbose_) {
            std::cout << "Raw frame data (ID: 0x726): ";
			std::cout << "0x" << std::hex << static_cast<int>(frame.data[0]) << " ";
            std::cout << std::dec << std::endl; // Switch back to decimal output
        }

        // Update the timestamp
        last_update_0x726_frame_++;

        return true;
    }
    return false;
}
*/

bool TractionDriver::create_0x226_frame(struct can_frame* frame, int throttle_value, bool reset) {
    if (verbose_) {
        std::cout << "Sending throttle command. Throttle value: " << throttle_value << ", Reset: " << reset << std::endl;
    }
    if (frame == nullptr) {
        return false;
    }
    
    std::memset(frame, 0, sizeof(struct can_frame));
    frame->can_id = FRAME_226;
    frame->can_dlc = 8;

    for (int i = 0; i < 8; i++) {
        frame->data[i] = 0;
    }
 
    if (reset) {
        frame->data[2] = RESET_CODE & 0xFF;
        frame->data[3] = (RESET_CODE >> 8) & 0xFF;
    } else {

        frame->data[0] = throttle_value & 0xFF;
        frame->data[1] = (throttle_value >> 8) & 0xFF;
    }

    last_update_0x226_frame_++;
    
    if (verbose_) {
        std::cout << "Throttle command frame prepared." << std::endl;
    }
    return true;
}

std::string TractionDriver::getFaultString(uint8_t fault_code) {

	switch(fault_code){
		case 38:
			return "MAIN_CONTACTOR_WELDED";
		break;
		case 39:
			return "MAIN_CONTACTOR_DID_NOT_CLOSE";
		break;
		case 45:
			return "POT_LOW_OVERCURRENT";
		break;
		case 42:
			return "THROTTLE_WIPER_LOW ";
		break;
		case 41:
			return "THROTTLE_WIPER_HIGH ";
		break;
		case 44:
			return "POT2_WIPER_LOW ";
		break;
		case 43:
			return "POT2_WIPER_HIGH";
		break;
		case 46:
			return "EEPROM_FAILURE";
		break;
		case 47:
			return "HPD_SEQUENCING";
		break;
		case 17:
			return "SEVERE_UNDERVOLTAGE";
		break;
		case 18:
			return "SEVERE_OVERVOLTAGE";
		break;		
		case 23:
			return "UNDERVOLTAGE_CUTBACK";
		break;
		case 24:
			return "OVERVOLTAGE_CUTBACK";
		break;
		case 21:
			return "NOT_KNOWDN";
		break;
		case 22:
			return "CONTROLLER_OVERTEMP_CUTBACK";
		break;
		case 15:
			return "CONTROLLER_SEVERE_UNDERTEMP";
		break;
		case 16:
			return "CONTROLLER_SEVERE_OVERTEMP";
		break;
		case 33:
			return "COIL3_DRIVER_OPEN_SHORT";
		break;
		case 34:
			return "COIL4_DRIVER_OPEN_SHORT";
		break;
		case 35:
			return "PD_OPEN_SHORT";
		break;
		case 31:
			return "MAIN_OPEN_SHORT";
		break;
		case 32:
			return "EMBRAKE_OPEN_SHORT";
		break;
		case 14:
			return "PRECHARGE_FAILED";
		break;
		case 26:
			return "DIGITAL_OUT_6_OVERCURRENT";
		break;
		case 27:
			return "DIGITAL_OUT_7_OVERCURRENT";
		break;
		case 12:
			return "DIGITAL_OUT_6_OVERCURRENT";
		break;				
		case 13:
			return "DIGITAL_OUT_6_OVERCURRENT";
		break;
		case 28:
			return "MOTOR_TEMP_HOT_CUTBACK";
		break;		
		case 49:
			return "PARAMETER_CHANGE";
		break;
		case 37:
			return "MOTOR_OPEN";
		break;			
		
		/* USER FAULTS */		
		case 51:
			return "SLAVE_TIMEOUT_FAULT";
		break;
		case 52:
			return "CUSTOM_HPD_FAULT";
		break;
		case 53:
			return "SEQUENCING_FAULT";
		break;
		case 54:
			return "SLAVE_FAULTED";
		break;
		case 55:
			return "COMPUTER_TIMEOUT_FAULT";
		break;
		/* /USER FAULTS */
		
		case 69:
			return "EXTERNAL_SUPPLY_OUT_OF_RANGE";
		break;
		case 29:
			return "MOTOR_TEMP_SENSOR ";
		break;				
		case 68:
			return "VCL_RUN_TIME_ERROR";
		break;
		case 25:
			return "EXTERNAL_5V";
		break;		
		case 71:
			return "OS_GENERAL";
		break;
		case 72:
			return "PDO_TIMEOUT";
		break;			
		case 36:
			return "ENCODER";
		break;
		case 73:
			return "STALL_DETECTED ";
		break;				
		case 89:
			return "MOTOR_TYPE_ERROR";
		break;		
		case 87:
			return "MOTOR_CHARACTERIZATION";
		break;
		case 97:
			return "PUMP_HARDWARE";
		break;		
		case 91:
			return "VCL_OS_MISMATCH";
		break;
		case 92:
			return "EM_BRAKE_FAILED_TO_SET";
		break;				
		case 93:
			return "ENCODER_LOS";
		break;
		case 94:
			return "EMER_REV_TIMEOUT";
		break;		
		case 75:
			return "DUAL_SEVERE";
		break;
		case 74:
			return "FAULT_ON_OTHER_TRACTION_CONTROLLER ";
		break;			
		case 98:
			return "ILLEGAL_MODEL_NUMBER";
		break;				
		case 95:
			return "PUMP_OVERCURRENT";
		break;
		case 96:
			return "PUMP_BDI";
		break;
		case 99:
			return "DUALMOTOR_PARAMETER_MISMATCH";
		break;			
		default:
			return "UNKNOWN FAULT";
		break;
	}
}