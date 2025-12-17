import yaml
from launch import LaunchContext
from launch import LaunchDescription
from launch.substitutions import Command, FindExecutable, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from caddy_ai2_ros2_common.launch_utils import read_update_rate_from_controller_yaml

def generate_launch_description():

    # Declare arguments
    declared_arguments = []

    # Default update rate
    default_update_rate = 100 # Of the controller manager
    update_rate = default_update_rate

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

    # MOSTRAR EL CONTENIDO DEL COMANDO
    context = LaunchContext()
    resolved_robot_description_content = robot_description_content.perform(context)
    print(f"[DEBUG LAUNCH] robot_description_content: {resolved_robot_description_content}")
    print(f"[DEBUG LAUNCH] robot_description_content: {robot_description_content}")

    robot_description = {"robot_description": robot_description_content}

    # ROS2 Control node
    control_node = Node(
        package="controller_manager",
        executable="ros2_control_node",
        parameters=[robot_controllers],
        output="both",
    )

    
    robot_state_pub_node = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        output="both",
        parameters=[robot_description],
    )

    # Load robot state publisher
    joint_state_broadcaster_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["joint_state_broadcaster"],
    )

 
    # robot_controller_spawner = Node(
    #     package="controller_manager",
    #     executable="spawner",
    #     arguments=[
    #         "diffbot_base_controller",
    #         "--param-file",
    #         robot_controllers,
    #         "--controller-ros-args",
    #         "-r /diffbot_base_controller/cmd_vel:=/cmd_vel",
    #     ],
    # )

    # # Delay start of joint_state_broadcaster after `robot_controller`
    # # TODO(anyone): This is a workaround for flaky tests. Remove when fixed.
    # delay_joint_state_broadcaster_after_robot_controller_spawner = RegisterEventHandler(
    #     event_handler=OnProcessExit(
    #         target_action=robot_controller_spawner,
    #         on_exit=[joint_state_broadcaster_spawner],
    #     )
    # )

    # nodes = [
    #     control_node,
    #     robot_state_pub_node,
    #     robot_controller_spawner,
    #     delay_rviz_after_joint_state_broadcaster_spawner,
    #     delay_joint_state_broadcaster_after_robot_controller_spawner,
    # ]

    nodes = [
        control_node,
        robot_state_pub_node,
        joint_state_broadcaster_spawner,
    ]

    return LaunchDescription(declared_arguments + nodes)