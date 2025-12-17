import yaml
from launch import LaunchDescription
from launch.substitutions import Command, FindExecutable, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from caddy_ai2_ros2_common.launch_utils import read_update_rate_from_controller_yaml

def generate_launch_description():

    # Get controller configuration
    robot_controllers = PathJoinSubstitution(
        [
            FindPackageShare("caddy_ai2_ros2_control_system_traction_driver"),
            "bringup",
            "config",
            "system_traction.yaml",
        ]
    )

    update_rate = read_update_rate_from_controller_yaml(robot_controllers)

    # Get URDF via xacro
    robot_description_content = Command(
        [
            PathJoinSubstitution([FindExecutable(name="xacro")]),
            " ",
            PathJoinSubstitution(
                [FindPackageShare("caddy_ai2_ros2_control_system_traction_driver"), "description", "urdf", "system_traction.urdf.xacro"]
            ),
            " ",
            f"update_rate:={update_rate}",
        ]
    )

    robot_description = {"robot_description": robot_description_content}

    # ROS2 Control node with namespace
    control_node = Node(
        package="controller_manager",
        executable="ros2_control_node",
        namespace="traction",
        parameters=[
            robot_description,
            robot_controllers
        ],
        output="both",
    )

    # Robot State Publisher with namespace
    robot_state_pub_node = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        namespace="traction",
        output="both",
        parameters=[robot_description],
    )

    # Spawners with namespace
    joint_state_broadcaster_spawner = Node(
        package="controller_manager",
        executable="spawner",
        namespace="traction",
        arguments=["joint_state_broadcaster"],
        output="screen",
    )

    velocity_controller_spawner = Node(
        package="controller_manager",
        executable="spawner",
        namespace="traction",
        arguments=["system_traction_velocity_controller"],
        output="screen",
    )

    nodes = [
        control_node,
        robot_state_pub_node,
        joint_state_broadcaster_spawner,
        velocity_controller_spawner,
    ]

    return LaunchDescription(nodes)