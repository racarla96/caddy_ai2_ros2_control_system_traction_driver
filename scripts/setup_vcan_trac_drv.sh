#!/bin/bash

# setup_vcan_trac_drv.sh - Configurar interfaz CAN virtual

echo "Configurando interfaz CAN virtual..."

# Cargar módulo vcan si no está cargado
if ! lsmod | grep -q vcan_trac_drv; then
    echo "Cargando módulo vcan_trac_drv..."
    sudo modprobe vcan_trac_drv
fi

# Eliminar vcan_trac_drv si ya existe
if ip link show vcan_trac_drv &> /dev/null; then
    echo "Eliminando vcan_trac_drv existente..."
    sudo ip link delete vcan_trac_drv
fi

# Crear y activar vcan_trac_drv
echo "Creando vcan_trac_drv..."
sudo ip link add dev vcan_trac_drv type vcan
sudo ip link set up vcan_trac_drv

echo "✓ Interfaz vcan_trac_drv configurada correctamente"
ip link show vcan_trac_drv