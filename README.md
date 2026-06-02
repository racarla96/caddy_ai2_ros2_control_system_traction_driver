# Caddy AI2 ROS2 Control System Traction Driver

Plugin de tipo **Controller** (`controller_interface::ControllerInterface`) para ROS2 Control que gestiona el sistema de tracción del robot Caddy AI2 mediante comunicación CAN directa con el controlador de motor Curtis 1234.

El vehículo viene equipado con un controlador Curtis 1234 (equivalente al Control System ID600 de la figura). En la imagen siguiente se muestra esquemáticamente el sistema de tracción de un carrito de golf similar al Caddy AI2.

![Sistema de tracción](doc/img/Transaxle-for-Electric-Golf-cars-5kw-1.jpg)

## Descripción

Este paquete implementa un **Controller** de ROS2 Control que sustituye a la antigua arquitectura basada en Hardware Interface. El controlador gestiona directamente la comunicación CAN con el motor Curtis sin necesidad de un hardware interface separado:

- **Motor Curtis 1234** — controlador de tracción con protocolo CAN propietario
- **Comunicación CAN** — mediante SocketCAN (Linux), adaptador Ixxat USB-to-CAN V2
- **Frecuencia CAN** — mensajes de estado a ~25 Hz desde el Curtis
- **Interfaz ROS2** — recibe consigna de velocidad vía tópico, publica estado del motor

## Arquitectura

```
┌─────────────────────────────────────────────────────────────┐
│                    ROS2 Control Manager                      │
│                    (frecuencia configurable)                  │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│           TractionDriverController (Controller)              │
│                                                              │
│  ~/reference (Float64) ──► velocidad objetivo [m/s]         │
│                                                              │
│  ~/state/velocity        ◄── velocidad [m/s]                │
│  ~/state/motor_rpm       ◄── RPM del motor                  │
│  ~/state/current_rms     ◄── corriente RMS [A]              │
│  ~/state/battery_current ◄── corriente batería [A]          │
│  ~/state/battery_voltage ◄── tensión batería [V]            │
│  ~/state/interlock       ◄── estado interlock               │
│  ~/state/on_fault        ◄── estado de fallo                │
│  ~/state/mode_auto       ◄── modo automático                │
│  ~/state/mode_manual     ◄── modo manual                    │
│  ~/state/fault_code      ◄── código de fallo                │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  • Gestión de frecuencias (ratio, multiplicidades)   │   │
│  │  • Conversión m/s ↔ throttle value                   │   │
│  │  • Offsets de lectura/escritura                      │   │
│  └──────────────────────────────────────────────────────┘   │
│                         │                                    │
│                         ▼                                    │
│  ┌──────────────────────────────────────────────────────┐   │
│  │           TractionDriver                             │   │
│  │  • process_frames(): decode 0x227, 0x1A6, 0x2A6     │   │
│  │  • create_0x226_frame(): encode throttle command     │   │
│  └──────────────────────────────────────────────────────┘   │
│                         │                                    │
│                         ▼                                    │
│  ┌──────────────────────────────────────────────────────┐   │
│  │         SocketCANInterface                           │   │
│  │  • Socket CAN RAW / Non-blocking I/O                │   │
│  └──────────────────────────────────────────────────────┘   │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
                  ┌─────────────┐
                  │   CAN Bus   │
                  │  (can/vcan) │
                  └─────────────┘
                         │
                         ▼
                  ┌─────────────┐
                  │  Curtis 1234│
                  └─────────────┘
```

## Protocolo CAN Curtis

| ID CAN | Dirección | Contenido |
|--------|-----------|-----------|
| `0x226` | TX (host → Curtis) | Throttle command (consigna de velocidad) |
| `0x227` | RX (Curtis → host) | Estado: interlock, fault, mode |
| `0x1A6` | RX | Corriente RMS, corriente batería, tensión, BDI% |
| `0x2A6` | RX | RPM motor, velocidad vehículo, temperatura |

## Componentes

### TractionDriverController
Controller principal que implementa `controller_interface::ControllerInterface`.

**Lifecycle:**
- `on_init()`: Crea el `ParamListener`.
- `on_configure()`: Instancia `SocketCANInterface` y `TractionDriver`, calcula multiplicidades, crea subscriber y publishers.
- `on_activate()`: Inicializa la interfaz CAN, envía frame de reset al Curtis y espera 5 s para su inicialización.
- `update()`: Bloque de lectura (recibe frames CAN, decodifica, publica estados) y bloque de escritura (convierte velocidad a throttle, envía frame 0x226).
- `on_deactivate()`: Envía throttle = 0 (parada segura).
- `on_cleanup()`: Destruye todos los recursos.

### TractionDriver
Procesador de frames CAN del Curtis (no tiene estado de comunicación propio):
- `process_frames(frames)`: Decodifica frames recibidos y actualiza estado interno.
- `create_0x226_frame(frame, throttle, reset)`: Prepara el frame de comando de velocidad.
- Getters: `get_speed()`, `get_motor_rpm()`, `get_current_rms()`, `get_battery_current()`, `get_keyswitch_voltage()`, `get_interlock()`, `get_on_fault()`, `get_mode_auto()`, `get_mode_manual()`, `get_fault_code()`.

## Conversión de velocidad

```
throttle_value = round( (velocity_mps / MAX_V_MPS) * SHRT_MAX )
throttle_value = clamp(throttle_value, -SHRT_MAX, SHRT_MAX)

MAX_V_MPS = π × 0.5 m × (4300 RPM / 60) / 16 ≈ 7.03 m/s
```

## Parámetros

| Parámetro | Tipo | Por defecto | Descripción |
|-----------|------|-------------|-------------|
| `interface_name` | string | — | Nombre de la interfaz SocketCAN (ej. `can_trac_drv`) |
| `controller_manager_frequency_hz` | double | 100.0 | Frecuencia del controller manager (Hz) |
| `hardware_sample_frequency_hz` | double | 500.0 | Frecuencia de muestreo del bus CAN (Hz) |
| `read_multiplicity` | int | 1 | Ciclos del CM entre lecturas CAN |
| `write_multiplicity` | int | 10 | Ciclos del CM entre envíos de throttle |
| `read_offset` | int | 0 | Desfase inicial del contador de lectura |
| `write_offset` | int | 1 | Desfase inicial del contador de escritura |

### Ejemplo de configuración

```yaml
traction_driver_controller:
  ros__parameters:
    interface_name: can_trac_drv
    controller_manager_frequency_hz: 100.0
    hardware_sample_frequency_hz: 500.0
    read_multiplicity: 1
    write_multiplicity: 4
    read_offset: 0
    write_offset: 1
```

### Plugin (controller_manager config)

```yaml
controller_manager:
  ros__parameters:
    update_rate: 100
    traction_driver_controller:
      type: caddy_ai2_ros2_control_system_traction_driver/TractionDriverController
```

## Instalación y compilación

```bash
cd ~/ws_ros2_caddy_dev
colcon build --packages-select caddy_ai2_ros2_common caddy_ai2_ros2_control_system_traction_driver
source install/setup.bash
```

## Configuración del bus CAN

### CAN virtual (desarrollo)

```bash
sudo ./scripts/setup_vcan_trac_drv.sh
```

### CAN físico (hardware real)

```bash
sudo ./scripts/setup_can_trac_drv.sh
```

## Uso

```bash
# Enviar consigna de velocidad (m/s)
ros2 topic pub /traction_driver_controller/reference std_msgs/msg/Float64 "data: 1.5"

# Ver velocidad medida
ros2 topic echo /traction_driver_controller/state/velocity

# Ver estado de fallo
ros2 topic echo /traction_driver_controller/state/on_fault
ros2 topic echo /traction_driver_controller/state/fault_code

# Estado del controller
ros2 control list_controllers
```

## Estructura del proyecto

```
caddy_ai2_ros2_control_system_traction_driver/
├── include/
│   └── caddy_ai2_ros2_control_system_traction_driver/
│       ├── traction_driver_controller.hpp   # Controller (nuevo)
│       ├── traction_driver.hpp              # Driver CAN Curtis
│       └── visibility_control.h
├── src/
│   ├── traction_driver_controller.cpp       # Controller (nuevo)
│   ├── traction_driver_controller_parameters.yaml
│   └── traction_driver.cpp
├── scripts/
│   ├── setup_can_trac_drv.sh
│   └── setup_vcan_trac_drv.sh
├── doc/img/
├── plugin_description.xml
├── CMakeLists.txt
├── package.xml
└── README.md
```

## Troubleshooting

### CAN interface not found

```bash
ip link show   # listar interfaces disponibles
```

### Curtis no responde

1. Verificar bitrate del bus CAN (500 kbps por defecto).
2. Monitorear tráfico: `candump can_trac_drv`
3. Verificar que el Curtis esté alimentado (keyswitch ON).

### Fault en el Curtis

Ver `~/state/fault_code` y consultar el manual de fallos del Curtis 1234.

## Autores

- **Desarrollador Principal**: Rafael Carbonell Lázaro (racarla96)
- **Proyecto**: Caddy AI2 – Proyecto CERVAREC

## Licencia

Copyright (c) 2025, Rafael Carbonell Lázaro (racarla96)

Distribuido bajo la licencia **Creative Commons Attribution 4.0 International (CC BY 4.0)**.
https://creativecommons.org/licenses/by/4.0/
