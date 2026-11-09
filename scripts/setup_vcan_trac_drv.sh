#!/bin/bash

################################################################################
# Script de configuración de interfaz virtual CAN para sistema de tracción
# Proyecto: Caddy AI2 ROS2 Control System Traction Driver
# Autor: Rafael Carbonell Lázaro (racarla96)
# Licencia: CC BY 4.0
################################################################################

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

INTERFACE_NAME="vcan_trac_drv"

print_header() {
    echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}  Configuración de interfaz virtual CAN (vcan) para tracción${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
}

print_info()    { echo -e "${GREEN}[INFO]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARN]${NC} $1"; }
print_error()   { echo -e "${RED}[ERROR]${NC} $1"; }
print_step()    { echo -e "\n${BLUE}▶${NC} $1"; }

################################################################################
# Verificar módulo vcan
################################################################################

check_vcan_module() {
    print_step "Verificando módulo vcan..."

    if lsmod | grep -q "^vcan"; then
        print_info "Módulo vcan ya cargado."
    else
        print_info "Cargando módulo vcan..."
        sudo modprobe vcan
        print_info "✓ Módulo vcan cargado."
    fi
}

################################################################################
# Crear y levantar la interfaz
################################################################################

setup_interface() {
    print_step "Configurando interfaz $INTERFACE_NAME..."

    if ip link show "$INTERFACE_NAME" &>/dev/null; then
        print_warning "La interfaz $INTERFACE_NAME ya existe. Eliminando..."
        sudo ip link delete "$INTERFACE_NAME"
    fi

    sudo ip link add dev "$INTERFACE_NAME" type vcan
    sudo ip link set up "$INTERFACE_NAME"
    print_info "✓ Interfaz $INTERFACE_NAME creada y levantada."
}

################################################################################
# Persistencia opcional mediante systemd-tmpfiles / rc.local
################################################################################

print_persistence_info() {
    print_step "Persistencia (opcional)"

    echo -e "\n${YELLOW}La interfaz vcan NO persiste tras reinicios. Para hacerla persistente:${NC}"
    echo ""
    echo "  Opción 1 — systemd (recomendado):"
    echo "    Crea /etc/systemd/system/vcan-trac-drv.service con:"
    echo ""
    echo "    [Unit]"
    echo "    Description=Virtual CAN traction driver interface"
    echo "    After=network.target"
    echo ""
    echo "    [Service]"
    echo "    Type=oneshot"
    echo "    RemainAfterExit=yes"
    echo "    ExecStart=/bin/bash -c 'modprobe vcan && ip link add dev $INTERFACE_NAME type vcan && ip link set up $INTERFACE_NAME'"
    echo "    ExecStop=/sbin/ip link delete $INTERFACE_NAME"
    echo ""
    echo "    [Install]"
    echo "    WantedBy=multi-user.target"
    echo ""
    echo "    Luego: sudo systemctl enable --now vcan-trac-drv.service"
    echo ""
    echo "  Opción 2 — ejecutar este script en cada sesión de desarrollo:"
    echo "    $(realpath "$0")"
}

################################################################################
# Información de uso
################################################################################

print_usage_info() {
    print_step "Información de uso"

    echo -e "\n${YELLOW}Comandos útiles:${NC}"
    echo "  ip link show $INTERFACE_NAME"
    echo "  candump $INTERFACE_NAME"
    echo "  cansend $INTERFACE_NAME 123#DEADBEEF"
    echo "  ip -s link show $INTERFACE_NAME"
    echo ""
    echo "  # Eliminar la interfaz"
    echo "  sudo ip link delete $INTERFACE_NAME"
    echo ""
    echo -e "${YELLOW}Integración con ROS2 Control:${NC}"
    echo "  Edita el parámetro interface_name en el fichero de hardware ros2_control"
    echo "  y cámbialo a: $INTERFACE_NAME"
}

################################################################################
# Main
################################################################################

main() {
    print_header

    if [[ $EUID -eq 0 ]]; then
        print_warning "No ejecutes este script como root. Se solicitará sudo cuando sea necesario."
        exit 1
    fi

    if ! command -v ip &>/dev/null; then
        print_error "'ip' no encontrado. Instala: sudo apt install iproute2"
        exit 1
    fi

    check_vcan_module
    setup_interface
    print_persistence_info
    print_usage_info

    echo -e "\n${GREEN}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}  ✓ Interfaz $INTERFACE_NAME lista para usar${NC}"
    echo -e "${GREEN}═══════════════════════════════════════════════════════════════${NC}\n"
}

main
