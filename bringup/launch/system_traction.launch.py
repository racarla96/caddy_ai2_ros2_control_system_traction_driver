import yaml
from launch import LaunchContext
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import Command, FindExecutable, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():

    # Declare arguments
    declared_arguments = []

    # Default update rate
    default_update_rate = 100 # Of the controller manager
    update_rate = default_update_rate

    # Get controller configuration
    robot_controllers = PathJoinSubstitution(
        [
            FindPackageShare("caddy_ai2_ros2_control_hardware_curtis_motor_driver"),
            "config",
            "curtis_motor.yaml",
        ]
    )

    # LEER EL YAML DE CONFIGURACIÓN DEL CONTROLADOR
    # Crear un contexto de lanzamiento
    context = LaunchContext()
    # Evaluar y obtener el valor real de la sustitución
    resolved_robot_controllers_path = robot_controllers.perform(context)
    # Imprimir el valor resuelto
    # print(f"[DEBUG LAUNCH] path del YAML: {resolved_robot_controllers_path}")
    # Leer el contenido del archivo YAML
    with open(resolved_robot_controllers_path, 'r') as yaml_file:
        try:
            # Cargar el contenido del YAML
            yaml_content = yaml.safe_load(yaml_file)
            # Imprimir el contenido del YAML para depuración
            print("[DEBUG LAUNCH] YAML Content:", yaml_content) 
            # Obtener el valor de update_rate o usar 100 como predeterminado
            update_rate = yaml_content.get("controller_manager", default_update_rate).get("ros__parameters", default_update_rate).get("update_rate", default_update_rate)
        except yaml.YAMLError as exc:
            print(f"[ERROR LAUNCH] Error al cargar el YAML: {exc}")


    # Get URDF via xacro
    robot_description_content = Command(
        [
            PathJoinSubstitution([FindExecutable(name="xacro")]),
            " ",
            PathJoinSubstitution(
                [FindPackageShare("caddy_ai2_ros2_control_hardware_curtis_motor_driver"), "urdf", "curtis_motor.urdf.xacro"]
            ),
            " ",
            f"update_rate:={update_rate}",
        ]
    )

    # MOSTRAR EL CONTENIDO DEL COMANDO
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