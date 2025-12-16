#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>

#include "hardware_interface/handle.hpp"
#include "hardware_interface/hardware_info.hpp"
#include "hardware_interface/actuator_interface.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "rclcpp/clock.hpp"
#include "rclcpp/duration.hpp"
#include "rclcpp/macros.hpp"
#include "rclcpp/time.hpp"
#include "rclcpp_lifecycle/node_interfaces/lifecycle_node_interface.hpp"
#include "rclcpp_lifecycle/state.hpp"

#include "caddy_ai2_ros2_common/socket_can_interface.hpp"
#include "caddy_ai2_ros2_control_system_traction_driver/traction_driver.hpp"

#define MAX_RPM 4300.0
#define GEAR_RATIO 16.0
#define WHEEL_DIAMETER_M 0.5 // m
#define MAX_V_MPS (M_PI * WHEEL_DIAMETER_M) * ((MAX_RPM / 60.0) / GEAR_RATIO)
#define RECOVER_TIME 5 // secs
#define WATCHDOG_TIMEOUT 0.2 // secs

namespace caddy_ai2_ros2_control_system_traction_driver
{
class SystemTractionHardwareInterface : public hardware_interface::ActuatorInterface
{
public:
  RCLCPP_SHARED_PTR_DEFINITIONS(SystemTractionHardwareInterface)

  hardware_interface::CallbackReturn on_init(const hardware_interface::HardwareComponentInterfaceParams& params) override;

  hardware_interface::CallbackReturn on_configure(const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::CallbackReturn on_cleanup(const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::CallbackReturn on_activate(const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::CallbackReturn on_deactivate(const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::CallbackReturn on_error(const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::return_type read(const rclcpp::Time & time, const rclcpp::Duration & period) override;

  hardware_interface::return_type write(const rclcpp::Time & time, const rclcpp::Duration & period) override;

private:
  // Estructura esperada de state_interfaces
  const std::vector<std::string> expected_state_interfaces = {
    "velocity",
    "motor_rpm",
    "current_rms",
    "battery_current",
    "battery_voltage",
    "interlock",
    "on_fault",
    "mode_auto",
    "mode_manual",
    "fault_code",
  };

  // Parameters
  std::string can_interface_name_;
  bool verbose_;

  // Max rate for reading and writing data 25 Hz
  double update_rate_hz_; // Rate of the controller manager for read and write operations
  double control_rate_hz_ = 25; // Rate of the control loop for sending commands
  int control_rate_write_counts_; // Number of write operations per control loop iteration
  int control_rate_write_loop_counts; // Counter for control rate (update_rate_hz_ / control_rate_hz_)
  
  double max_v_mps = MAX_V_MPS; // Maximum velocity in m/s
  double velocity_; // Current velocity in m/s
  struct can_frame throttle_frame_; // Frame for sending throttle commands
  std::vector<struct can_frame> frames;

  // Hardware
  std::unique_ptr<SocketCANInterface> can_interface_;
  std::unique_ptr<TractionDriver> traction_driver_;
};

}  // namespace caddy_ai2_ros2_control_system_traction_driver