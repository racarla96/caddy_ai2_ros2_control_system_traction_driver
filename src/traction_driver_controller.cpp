#include "caddy_ai2_ros2_control_system_traction_driver/traction_driver_controller.hpp"

#include <climits>
#include <cmath>
#include <memory>
#include <thread>
#include <vector>

#include "rclcpp/rclcpp.hpp"

namespace caddy_ai2_ros2_control_system_traction_driver
{

// Physical constants (derived from Curtis + transaxle hardware — keep in sync with integration pkg)
static constexpr double MAX_RPM          = 4300.0;
static constexpr double GEAR_RATIO       = 16.0;
static constexpr double WHEEL_DIAMETER_M = 0.5;
static constexpr double MAX_V_MPS =
  (M_PI * WHEEL_DIAMETER_M) * ((MAX_RPM / 60.0) / GEAR_RATIO);

// Seconds to wait after sending the reset frame before the controller is ready
static constexpr int RECOVER_TIME_S = 5;

TractionDriverController::TractionDriverController()
: controller_interface::ControllerInterface()
{
}

controller_interface::CallbackReturn TractionDriverController::on_init()
{
  try
  {
    param_listener_ = std::make_shared<traction_driver_controller::ParamListener>(get_node());
  }
  catch (const std::exception & e)
  {
    fprintf(stderr, "Exception thrown during controller's init with message: %s \n", e.what());
    return controller_interface::CallbackReturn::ERROR;
  }
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn TractionDriverController::on_configure(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  try
  {
    params_ = param_listener_->get_params();

    // Instantiate CAN interface and Curtis traction driver
    can_interface_   = std::make_unique<SocketCANInterface>(params_.interface_name);
    traction_driver_ = std::make_unique<TractionDriver>();

    // Compute effective multiplicity
    frequency_ratio_ = static_cast<int>(
      params_.controller_manager_frequency_hz / params_.hardware_sample_frequency_hz);
    if (frequency_ratio_ < 1)
    {
      RCLCPP_WARN(
        get_node()->get_logger(),
        "controller_manager_frequency_hz (%.2f) < hardware_sample_frequency_hz (%.2f), using ratio=1",
        params_.controller_manager_frequency_hz, params_.hardware_sample_frequency_hz);
      frequency_ratio_ = 1;
    }
    effective_read_multiplicity_  = static_cast<int>(params_.read_multiplicity)  * frequency_ratio_;
    effective_write_multiplicity_ = static_cast<int>(params_.write_multiplicity) * frequency_ratio_;

    RCLCPP_INFO(get_node()->get_logger(), "CAN interface    : %s",  params_.interface_name.c_str());
    RCLCPP_INFO(get_node()->get_logger(), "Max velocity     : %.3f m/s", MAX_V_MPS);
    RCLCPP_INFO(get_node()->get_logger(), "Freq ratio       : %d",  frequency_ratio_);
    RCLCPP_INFO(get_node()->get_logger(), "Eff read mult    : %d",  effective_read_multiplicity_);
    RCLCPP_INFO(get_node()->get_logger(), "Eff write mult   : %d",  effective_write_multiplicity_);

    auto node = get_node();

    // Subscriber: receive target velocity [m/s] from an upstream controller or operator
    velocity_sub_ = node->create_subscription<std_msgs::msg::Float64>(
      "~/reference", rclcpp::SensorDataQoS(),
      [this](const std_msgs::msg::Float64::SharedPtr msg) {
        target_velocity_mps_.store(msg->data, std::memory_order_relaxed);
      });

    // Publishers: motor state feedback
    auto qos = rclcpp::SensorDataQoS();
    velocity_pub_        = node->create_publisher<std_msgs::msg::Float64>("~/state/velocity",        qos);
    motor_rpm_pub_       = node->create_publisher<std_msgs::msg::Float64>("~/state/motor_rpm",       qos);
    current_rms_pub_     = node->create_publisher<std_msgs::msg::Float64>("~/state/current_rms",     qos);
    battery_current_pub_ = node->create_publisher<std_msgs::msg::Float64>("~/state/battery_current", qos);
    battery_voltage_pub_ = node->create_publisher<std_msgs::msg::Float64>("~/state/battery_voltage", qos);
    interlock_pub_       = node->create_publisher<std_msgs::msg::Float64>("~/state/interlock",       qos);
    on_fault_pub_        = node->create_publisher<std_msgs::msg::Float64>("~/state/on_fault",        qos);
    mode_auto_pub_       = node->create_publisher<std_msgs::msg::Float64>("~/state/mode_auto",       qos);
    mode_manual_pub_     = node->create_publisher<std_msgs::msg::Float64>("~/state/mode_manual",     qos);
    fault_code_pub_      = node->create_publisher<std_msgs::msg::Float64>("~/state/fault_code",      qos);
  }
  catch (const std::exception & e)
  {
    fprintf(stderr, "Exception thrown during configure stage with message: %s \n", e.what());
    return controller_interface::CallbackReturn::ERROR;
  }
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::InterfaceConfiguration
TractionDriverController::command_interface_configuration() const
{
  // This controller communicates directly with the CAN hardware; no ros2_control
  // command interfaces are required.
  return {controller_interface::interface_configuration_type::NONE, {}};
}

controller_interface::InterfaceConfiguration
TractionDriverController::state_interface_configuration() const
{
  // Motor state is read directly via SocketCANInterface.
  return {controller_interface::interface_configuration_type::NONE, {}};
}

controller_interface::CallbackReturn TractionDriverController::on_activate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(get_node()->get_logger(), "Activating TractionDriverController...");

  if (!can_interface_->init())
  {
    RCLCPP_FATAL(get_node()->get_logger(),
      "Failed to initialise CAN interface '%s'", params_.interface_name.c_str());
    return controller_interface::CallbackReturn::ERROR;
  }

  // Send reset frame and wait for the Curtis controller to initialise.
  // NOTE: this is a blocking 5-second sleep inherited from the hardware interface design.
  struct can_frame reset_frame{};
  traction_driver_->create_0x226_frame(&reset_frame, 0, /*reset=*/true);
  can_interface_->write(reset_frame);
  RCLCPP_INFO(get_node()->get_logger(),
    "Reset frame sent — waiting %d s for Curtis controller to initialise...", RECOVER_TIME_S);
  std::this_thread::sleep_for(std::chrono::seconds(RECOVER_TIME_S));

  // Initialise phase counters with configured offsets
  read_counter_  = effective_read_multiplicity_  - static_cast<int>(params_.read_offset);
  write_counter_ = effective_write_multiplicity_ - static_cast<int>(params_.write_offset);

  target_velocity_mps_.store(0.0, std::memory_order_relaxed);

  RCLCPP_INFO(get_node()->get_logger(), "TractionDriverController activated");
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn TractionDriverController::on_deactivate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(get_node()->get_logger(), "Deactivating TractionDriverController — sending zero throttle...");

  if (traction_driver_ && can_interface_)
  {
    struct can_frame stop_frame{};
    traction_driver_->create_0x226_frame(&stop_frame, 0, /*reset=*/false);
    can_interface_->write(stop_frame);
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn TractionDriverController::on_cleanup(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  can_interface_.reset();
  traction_driver_.reset();
  velocity_sub_.reset();
  velocity_pub_.reset();
  motor_rpm_pub_.reset();
  current_rms_pub_.reset();
  battery_current_pub_.reset();
  battery_voltage_pub_.reset();
  interlock_pub_.reset();
  on_fault_pub_.reset();
  mode_auto_pub_.reset();
  mode_manual_pub_.reset();
  fault_code_pub_.reset();
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::return_type TractionDriverController::update(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  // --- Read path: receive CAN frames from the Curtis controller ---
  read_counter_++;
  if (read_counter_ >= effective_read_multiplicity_)
  {
    read_counter_ = 0;

    std::vector<struct can_frame> frames;
    const int num_frames = can_interface_->read(frames, 0);  // non-blocking

    if (num_frames > 0)
    {
      std::vector<bool> processed = traction_driver_->process_frames(frames);

      auto pub = [](rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr & p, double v) {
        std_msgs::msg::Float64 m;
        m.data = v;
        p->publish(m);
      };

      for (int i = 0; i < num_frames; ++i)
      {
        if (!processed[static_cast<size_t>(i)]) { continue; }

        switch (frames[static_cast<size_t>(i)].can_id)
        {
          case TractionDriver::FRAME_227_:
            pub(velocity_pub_,  traction_driver_->get_speed());
            pub(motor_rpm_pub_, traction_driver_->get_motor_rpm());
            break;
          case TractionDriver::FRAME_1A6_:
            pub(current_rms_pub_,     traction_driver_->get_current_rms());
            pub(battery_current_pub_, traction_driver_->get_battery_current());
            pub(battery_voltage_pub_, traction_driver_->get_keyswitch_voltage());
            break;
          case TractionDriver::FRAME_2A6_:
            pub(interlock_pub_,   static_cast<double>(traction_driver_->get_interlock()));
            pub(on_fault_pub_,    static_cast<double>(traction_driver_->get_on_fault()));
            pub(mode_auto_pub_,   static_cast<double>(traction_driver_->get_mode_auto()));
            pub(mode_manual_pub_, static_cast<double>(traction_driver_->get_mode_manual()));
            pub(fault_code_pub_,  static_cast<double>(traction_driver_->get_fault_code()));
            break;
          default:
            break;
        }
      }
    }
  }

  // --- Write path: send throttle command to the Curtis controller ---
  write_counter_++;
  if (write_counter_ >= effective_write_multiplicity_)
  {
    write_counter_ = 0;

    const double velocity = target_velocity_mps_.load(std::memory_order_relaxed);

    // Convert m/s → throttle value in [-SHRT_MAX, SHRT_MAX]
    int throttle_value = static_cast<int>(
      std::round((velocity * (100.0 / MAX_V_MPS)) * (static_cast<double>(SHRT_MAX) / 100.0)));
    throttle_value = std::clamp(throttle_value, -SHRT_MAX, SHRT_MAX);

    struct can_frame throttle_frame{};
    traction_driver_->create_0x226_frame(&throttle_frame, throttle_value, /*reset=*/false);
    can_interface_->write(throttle_frame);
  }

  return controller_interface::return_type::OK;
}

}  // namespace caddy_ai2_ros2_control_system_traction_driver

#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(
  caddy_ai2_ros2_control_system_traction_driver::TractionDriverController,
  controller_interface::ControllerInterface)
