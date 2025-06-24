# caddy_ai2_ros2_control_hardware_curtis_motor_driver

Frecuencia de los mensajes de control a 25 Hz

# Como configurar el uso de CAN en Linux
Con los comandos
lsusb
sudo dmesg | grep -i can
Podemos ver los dispositivos Can conectados

Con 
ip link show
Podemos ver si tiene la interfaz de red lista para ser configurada

# Crear una interfaz virtual para testear el driver

sudo ip link add dev vcan0 type vcan

# Necesario Agregar tu usuario al grupo netdev

sudo usermod -aG netdev $USER

Después, cierra la sesión y vuelve a entrar para que los cambios tengan efecto.

# Instalación de dependencias
### libsocketcan
sudo apt-get install libsocketcan* -y
### libsocketcanpp (is a socketcan wrapper library for C++)
git clone https://github.com/SimonCahill/libsockcanpp.git
cd libsockcanpp
mkdir build && cd build
cmake ..
make -j
sudo make install
# can-utils
git clone https://github.com/linux-can/can-utils.git
cd can-utils
make
sudo make install
# Ixxat USB-to-CAN V2 compact
sudo apt install linux-headers-$(uname -r)
sudo apt install --reinstall build-essential
ls /usr/src/linux-headers-$(uname -r)
cd drivers/ix_usb_can_2.0.520-REL
make all
sudo make install


# Enlaces
### libsocketcan

### libsocketcanpp (is a socketcan wrapper library for C++)
https://github.com/SimonCahill/libsockcanpp

# can-utils
https://github.com/linux-can/can-utils

# Ixxat USB-to-CAN V2 compact
https://www.hms-networks.com/p/1-01-0281-12001-ixxat-usb-to-can-v2-compact?tab=tab-support

## Configuración en /etc/sudoers (Recomendado)

Para evitar tener que ejecutar el programa como root, puedes configurar el archivo /etc/sudoers para permitir que un usuario específico ejecute el comando ip link sin necesidad de contraseña.

    Abre el archivo /etc/sudoers con sudo visudo.

    Añade una línea como la siguiente:

    usuario ALL=(ALL) NOPASSWD: /sbin/ip

    Reemplaza usuario con el nombre del usuario que ejecutará el programa.

    Advertencia: Modificar /etc/sudoers incorrectamente puede bloquear el acceso al sistema. Usa visudo para evitar errores de sintaxis.
    
### **Guía para asignar el nombre personalizado al adaptador USB-CAN**

1. Dale permisos de ejecución:

   ```bash
   sudo chmod +x asignar_can_motor_drv.sh
   sudo chmod +x asignar_can_steer_drv.sh
   ```
2. Ejecútalo:

   ```bash
   sudo ./asignar_can_motor_drv.sh
   sudo ./asignar_can_steer_drv.sh
   ```
3. Verifica

    Después de desconectar y reconectar el adaptador, verifica que el nombre se ha asignado correctamente:
    ```bash
    ip link show
    ```

    Deberías ver tu interfaz como `can_motor_drv` o `can_steer_drv`.


ros2 control load_controller --set-state active curtis_motor_velocity_controller


# Enviar mensaje

sudo ip link add dev vcan_motor_drv type vcan
sudo ip link set up vcan_motor_drv

ros2 topic pub -r 25  /curtis_motor_velocity_controller/commands std_msgs/msg/Float64 "{data: 1.0}"

# TODOs
- [ ] Include on launch file the interface as parameter to easy change it