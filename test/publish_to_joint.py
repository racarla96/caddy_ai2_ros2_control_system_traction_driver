import rclpy
from rclpy.node import Node
from std_msgs.msg import Float64MultiArray

class JointStateProfilePublisher(Node):
    def __init__(self):
        super().__init__('joint_state_profile_publisher')
        # CORREGIDO: Usar Float64MultiArray en lugar de JointState
        self.publisher_ = self.create_publisher(Float64MultiArray, '/curtis_motor_velocity_controller/commands', 10)
        
        # Configuración del perfil
        self.publish_rate_hz = 25
        self.ramp_up_time = 5.0  # 5 segundos para subir
        self.hold_time = 5.0     # 5 segundos manteniendo velocidad máxima
        self.ramp_down_time = 5.0  # 5 segundos para bajar
        self.max_velocity = 1.0  # m/s
        
        # Estado interno
        self.current_time = 0.0
        self.current_velocity = 0.0
        self.phase = "ramp_up"  # "ramp_up", "hold", "ramp_down", "stop"
        
        # Timer
        timer_period = 1.0 / self.publish_rate_hz
        self.timer = self.create_timer(timer_period, self.timer_callback)
        
        self.get_logger().info('Iniciando perfil de velocidad: 0 -> 1 -> 0 m/s')

    def timer_callback(self):
        dt = 1.0 / self.publish_rate_hz
        self.current_time += dt
        
        if self.phase == "ramp_up":
            # Subir linealmente de 0 a max_velocity en ramp_up_time segundos
            progress = min(self.current_time / self.ramp_up_time, 1.0)
            self.current_velocity = progress * self.max_velocity
            
            if self.current_time >= self.ramp_up_time:
                self.phase = "hold"
                self.current_time = 0.0  # Reset timer para la siguiente fase
                self.get_logger().info(f'Fase HOLD: manteniendo {self.max_velocity} m/s')
                
        elif self.phase == "hold":
            # Mantener velocidad máxima
            self.current_velocity = self.max_velocity
            
            if self.current_time >= self.hold_time:
                self.phase = "ramp_down"
                self.current_time = 0.0
                self.get_logger().info('Fase RAMP_DOWN: bajando a 0 m/s')
                
        elif self.phase == "ramp_down":
            # Bajar linealmente de max_velocity a 0 en ramp_down_time segundos
            progress = min(self.current_time / self.ramp_down_time, 1.0)
            self.current_velocity = self.max_velocity * (1.0 - progress)
            
            if self.current_time >= self.ramp_down_time:
                self.phase = "stop"
                self.current_velocity = 0.0
                self.get_logger().info('Perfil completado. Velocidad = 0 m/s')
                
        elif self.phase == "stop":
            # Mantener en 0
            self.current_velocity = 0.0

        # CORREGIDO: Publicar Float64MultiArray
        msg = Float64MultiArray()
        msg.data = [self.current_velocity]  # Array con un elemento para curtis_motor
        
        self.publisher_.publish(msg)
        
        # Log cada segundo aproximadamente
        if int(self.current_time * self.publish_rate_hz) % self.publish_rate_hz == 0:
            self.get_logger().info(f'Fase: {self.phase}, Velocidad: {self.current_velocity:.2f} m/s')

def main(args=None):
    rclpy.init(args=args)
    node = JointStateProfilePublisher()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()