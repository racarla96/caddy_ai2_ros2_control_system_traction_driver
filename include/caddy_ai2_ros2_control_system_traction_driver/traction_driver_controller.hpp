#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "controller_interface/controller_interface.hpp"
#include "std_msgs/msg/float64.hpp"

#include "caddy_ai2_ros2_common/socket_can_interface.hpp"
#include "caddy_ai2_ros2_control_system_traction_driver/traction_driver_controller_parameters.hpp"
#include "caddy_ai2_ros2_control_system_traction_driver/traction_driver.hpp"

namespace caddy_ai2_ros2_control_system_traction_driver
{

class TractionDriverController : public controller_interface::ControllerInterface
{
public:
  TractionDriverController();

  controller_interface::CallbackReturn on_init() override;

  controller_interface::CallbackReturn on_configure(
    const rclcpp_lifecycle::State & previous_state) override;

  controller_interface::InterfaceConfiguration command_interface_configuration() const override;

  controller_interface::InterfaceConfiguration state_interface_configuration() const override;

  controller_interface::CallbackReturn on_activate(
    const rclcpp_lifecycle::State & previous_state) override;

  controller_interface::CallbackReturn on_deactivate(
    const rclcpp_lifecycle::State & previous_state) override;

  controller_interface::CallbackReturn on_cleanup(
    const rclcpp_lifecycle::State & previous_state) override;

  controller_interface::return_type update(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

protected:
  std::shared_ptr<traction_driver_controller::ParamListener> param_listener_;
  traction_driver_controller::Params params_;

  // CAN hardware
  std::unique_ptr<SocketCANInterface> can_interface_;
  std::unique_ptr<TractionDriver> traction_driver_;

  // ROS2 I/O: velocity command in [m/s], individual state publishers
  rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr velocity_sub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr velocity_pub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr motor_rpm_pub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr current_rms_pub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr battery_current_pub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr battery_voltage_pub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr interlock_pub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr on_fault_pub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr mode_auto_pub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr mode_manual_pub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr fault_code_pub_;

  // RT-safe velocity storage (written by subscription callback, read in update)
  std::atomic<double> target_velocity_mps_{0.0};

  // Multiplicity / phase counters
  int frequency_ratio_{1};
  int effective_read_multiplicity_{1};
  int effective_write_multiplicity_{1};
  int read_counter_{0};
  int write_counter_{0};
};

}  // namespace caddy_ai2_ros2_control_system_traction_driver
