#include "caddy_ai2_ros2_control_system_traction_driver/system_traction_hardware.hpp"

#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

#include "hardware_interface/lexical_casts.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"

namespace caddy_ai2_ros2_control_system_traction_driver
{

hardware_interface::CallbackReturn SystemTractionHardwareInterface::on_init(
  const hardware_interface::HardwareComponentInterfaceParams & params)
{
  if (hardware_interface::ActuatorInterface::on_init(params) != hardware_interface::CallbackReturn::SUCCESS)
  {
    return hardware_interface::CallbackReturn::ERROR;
  }

  // Verifica parámetros
  try {
    can_interface_name_ = info_.hardware_parameters.at("can_interface");
    update_rate_hz_ = hardware_interface::stod(info_.hardware_parameters.at("update_rate"));
  } catch (const std::exception &e) {
    RCLCPP_FATAL(get_logger(), "Faltan parámetros 'can_interface' o 'update_rate' o no son válidos: %s", e.what());
    return hardware_interface::CallbackReturn::ERROR;
  }

  RCLCPP_INFO(get_logger(), "Conversion: %.3f", max_v_mps);

  control_rate_write_loop_counts = static_cast<int>(std::floor(update_rate_hz_ / control_rate_hz_));

  RCLCPP_INFO(get_logger(), "Initializing System Traction Hardware Interface with CAN interface: %s", can_interface_name_.c_str());
  RCLCPP_INFO(get_logger(), "Update rate: %.2f Hz", update_rate_hz_);



  for (const hardware_interface::ComponentInfo & joint : info_.joints)
  {
    // 1. Verifica número y tipo de interfaces de comando
    if (joint.command_interfaces.size() != 1)
    {
      RCLCPP_FATAL(get_logger(), "Joint '%s' tiene %zu command interfaces. Se esperaba 1.", joint.name.c_str(), joint.command_interfaces.size());
      return hardware_interface::CallbackReturn::ERROR;
    }
    if (joint.command_interfaces[0].name != hardware_interface::HW_IF_VELOCITY)
    {
      RCLCPP_FATAL(get_logger(), "Joint '%s' tiene '%s' como command interface. Se esperaba 'velocity'.", joint.name.c_str(), joint.command_interfaces[0].name.c_str());
      return hardware_interface::CallbackReturn::ERROR;
    }

    // 2. Verifica número y orden de state_interfaces
    if (joint.state_interfaces.size() != expected_state_interfaces.size())
    {
      RCLCPP_FATAL(get_logger(), "Joint '%s' tiene %zu state interfaces. Se esperaban %zu.", joint.name.c_str(), joint.state_interfaces.size(), expected_state_interfaces.size());
      return hardware_interface::CallbackReturn::ERROR;
    }
    for (size_t i = 0; i < expected_state_interfaces.size(); ++i)
    {
      if (joint.state_interfaces[i].name != expected_state_interfaces[i])
      {
        RCLCPP_FATAL(get_logger(), "Joint '%s' state interface %zu es '%s'. Se esperaba '%s'.", joint.name.c_str(), i, joint.state_interfaces[i].name.c_str(), expected_state_interfaces[i].c_str());
        return hardware_interface::CallbackReturn::ERROR;
      }
    }

    RCLCPP_INFO(get_logger(), "Joint '%s' tiene la estructura y el orden de interfaces correcto.", joint.name.c_str());
  }

  // std::string joint_name = info_.joints[0].name;
  // RCLCPP_INFO(get_logger(), "Joint name: %s", joint_name.c_str());

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn SystemTractionHardwareInterface::on_configure(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  // Initialize CAN interface and Curtis driver
  can_interface_ = std::make_unique<SocketCANInterface>(can_interface_name_);
  traction_driver_ = std::make_unique<TractionDriver>();

  // reset values always when configuring hardware
  for (const auto & [name, descr] : joint_state_interfaces_)
  {
    set_state(name, 0.0);
    RCLCPP_INFO(get_logger(), "Reset state interface '%s' to 0.0", name.c_str());
  }
  for (const auto & [name, descr] : joint_command_interfaces_)
  {
    set_command(name, 0.0);
  }
  
  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn SystemTractionHardwareInterface::on_cleanup(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  


  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn SystemTractionHardwareInterface::on_activate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  // Initialize CAN interface
  if (!can_interface_->init()) {
    RCLCPP_ERROR(
      rclcpp::get_logger("CurtisMotorHardwareInterface"),
      "Failed to initialize CAN interface: %s", can_interface_name_.c_str());
    return hardware_interface::CallbackReturn::ERROR;
  }
  


  // Enviamos un mensaje de reset al controlador y esperamos a que se inicialice
  traction_driver_->create_0x226_frame(&throttle_frame_, 0, true);
  can_interface_->write(throttle_frame_);
  std::this_thread::sleep_for(std::chrono::seconds(RECOVER_TIME)); // Wait for controller to initialize

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn SystemTractionHardwareInterface::on_deactivate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  // Send zero throttle command to stop the motor
  traction_driver_->create_0x226_frame(&throttle_frame_, 0, false);
  can_interface_->write(throttle_frame_);

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn SystemTractionHardwareInterface::on_error(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  // Send zero throttle command to stop the motor
  traction_driver_->create_0x226_frame(&throttle_frame_, 0, false);
  can_interface_->write(throttle_frame_);
  
  return hardware_interface::CallbackReturn::ERROR;
}

hardware_interface::return_type SystemTractionHardwareInterface::read(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{

  // if (control_rate_write_counts_ < control_rate_write_loop_counts) {
  //   control_rate_write_counts_++;
  //   return hardware_interface::return_type::OK;  // Skip reading if not time to write
  // }
  // control_rate_write_counts_ = 1;  // Reset counter for next cycle

  // // Read CAN frames at 250Hz
  frames.clear();
  int num_frames = can_interface_->read(frames, 0);  // Non-blocking read
  
  if (num_frames > 0) {
    // Process frames and update internal data
    std::vector<bool> processed = traction_driver_->process_frames(frames);
    
    for(int i = 0; i < num_frames; ++i) {
      if (processed[i]) {
        if (frames[i].can_id == traction_driver_->FRAME_227_) {
          set_state(info_.joints[0].name + "/" + "motor_rpm", traction_driver_->get_motor_rpm());
          set_state(info_.joints[0].name + "/" + "velocity", traction_driver_->get_speed());
        }
        else if (frames[i].can_id == traction_driver_->FRAME_1A6_) {
          set_state(info_.joints[0].name + "/" + "current_rms", traction_driver_->get_current_rms());
          set_state(info_.joints[0].name + "/" + "battery_current", traction_driver_->get_battery_current());
          set_state(info_.joints[0].name + "/" + "battery_voltage", traction_driver_->get_keyswitch_voltage());
        }
        else if (frames[i].can_id == traction_driver_->FRAME_2A6_) {
          set_state(info_.joints[0].name + "/" + "interlock", static_cast<double>(traction_driver_->get_interlock()));
          set_state(info_.joints[0].name + "/" + "on_fault", static_cast<double>(traction_driver_->get_on_fault()));
          set_state(info_.joints[0].name + "/" + "mode_auto", static_cast<double>(traction_driver_->get_mode_auto()));
          set_state(info_.joints[0].name + "/" + "mode_manual", static_cast<double>(traction_driver_->get_mode_manual()));
          set_state(info_.joints[0].name + "/" + "fault_code", static_cast<double>(traction_driver_->get_fault_code()));
        }
      }
    }
  }
  
  return hardware_interface::return_type::OK;
}

hardware_interface::return_type SystemTractionHardwareInterface::write(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  if (control_rate_write_counts_ <= control_rate_write_loop_counts) {
    control_rate_write_counts_++;
    return hardware_interface::return_type::OK;  // Skip writing if not time to write
  }
  control_rate_write_counts_ = 1;  // Reset counter for next cycle
  velocity_ = get_command(info_.joints[0].name + "/" + "velocity");
  // RCLCPP_INFO(get_logger(), "Input velocity: %.2f m/s", velocity_);

  // Convert velocity to throttle value  

  // RCLCPP_INFO(get_logger(), "Relation: %.2f m/s", (velocity_ / max_v_mps));
  int throttle_value = static_cast<int>(std::round( ( (velocity_ * (100.0 / max_v_mps)) * ((double)SHRT_MAX / 100.0)) ));
  throttle_value = std::clamp(throttle_value, -SHRT_MAX, SHRT_MAX);  // Clamp to valid range

  // RCLCPP_INFO(get_logger(), "Calculated throttle value: %d", throttle_value);
  // Create throttle frame
  traction_driver_->create_0x226_frame(&throttle_frame_, throttle_value, false);
  // Write throttle command to CAN interface
  can_interface_->write(throttle_frame_);
  
  return hardware_interface::return_type::OK;
}

}  // namespace caddy_ai2_ros2_control_system_traction_driver

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(caddy_ai2_ros2_control_system_traction_driver::SystemTractionHardwareInterface, hardware_interface::ActuatorInterface)