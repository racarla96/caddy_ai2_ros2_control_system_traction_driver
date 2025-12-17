caddy_ai2_ros2ction_driver/bringup/launch/system_traction.launch.import os
import yaml
from launch import LaunchDescription
from launch.substitutions import Command, FindExecutable, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory

# Función para cargar YAML de forma segura
def load_yaml(package_name, file_path):
    full_path = os.path.join(get_package_share_directory(package_name), file_path)
    with open(full_path, 'r') as f:
        return yaml.safe_load(f)

def generate_launch_description():
    # --- Configuración del sistema de tracción ---
    traction_pkg = "caddy_ai2_ros2_control_system_traction_driver"
    traction_config_rel_path = os.path.join("bringup", "config", "system_traction.yaml")
    traction_urdf_rel_path = os.path.join("description", "urdf", "system_traction.urdf.xacro")

    # Cargar el YAML de configuración
    traction_yaml_params = load_yaml(traction_pkg, traction_config_rel_path)
    
    # Extraer update_rate del YAML para el xacro (si existe, si no, usa 100)
    update_rate = traction_yaml_params.get("controller_manager", {}).get("ros__parameters", {}).get("update_rate", 100)

    # Generar el contenido del URDF con xacro
    robot_description_content = Command(
        [
            PathJoinSubstitution([FindExecutable(name="xacro")]),
            " ",
            PathJoinSubstitution([FindPackageShare(traction_pkg), traction_urdf_rel_path]),
            " ",
            f"update_rate:={update_rate}",
        ]
    )

    # --- Nodos ---

    # 1. ros2_control_node (el corazón del control)
    control_node = Node(
        package="controller_manager",
        executable="ros2_control_node",
        namespace="traction", # Usamos namespace para buena práctica
        parameters=[
            {"robot_description": robot_description_content}, # Pasa el URDF
            traction_yaml_params # Pasa los parámetros del YAML
        ],
        output="screen",
        emulate_tty=True # Para ver logs en la consola
    )

    # 2. robot_state_publisher (necesario para que ros2_control_node se inicialice)
    robot_state_pub_node = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        namespace="traction", # Mismo namespace
        output="screen",
        parameters=[{"robot_description": robot_description_content}],
    )

    # 3. Spawner para joint_state_broadcaster
    joint_state_broadcaster_spawner = Node(
        package="controller_manager",
        executable="spawner",
        namespace="traction", # Mismo namespace
        arguments=["joint_state_broadcaster"],
        output="screen",
    )

    # 4. Spawner para system_traction_velocity_controller
    velocity_controller_spawner = Node(
        package="controller_manager",
        executable="spawner",
        namespace="traction", # Mismo namespace
        arguments=["system_traction_velocity_controller"],
        output="screen",
    )

    # --- Retornar la descripción del launch ---
    return LaunchDescription([
        control_node,
        robot_state_pub_node,
        joint_state_broadcaster_spawner,
        velocity_controller_spawner,
    ])