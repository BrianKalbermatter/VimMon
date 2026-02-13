#!/bin/bash
# ============================================================
# FASE 3 - Ejercicio 3.4: Redes y Firewall
# ============================================================
# OBJETIVO: Configurar red y firewall con nmcli y firewall-cmd
#
# === REDES ===
#
# 1. Mostrar interfaces
#   nmcli device status
#   ip addr show
#   ip -br addr
#
# 2. Ver conexiones configuradas
#   nmcli connection show
#
# 3. Configurar IP estatica
#   nmcli connection modify "enp0s3" \
#     ipv4.addresses 192.168.122.100/24 \
#     ipv4.gateway 192.168.122.1 \
#     ipv4.dns "8.8.8.8 8.8.4.4" \
#     ipv4.method manual
#
# 4. Aplicar cambios
#   nmcli connection down "enp0s3"
#   nmcli connection up "enp0s3"
#
# 5. Verificar
#   ip addr show enp0s3
#   ping -c 3 192.168.122.1
#   ping -c 3 8.8.8.8
#
# 6. Configurar hostname
#   sudo hostnamectl set-hostname servidor-rhcsa.lab.local
#   hostname
#   cat /etc/hostname
#
# 7. Configurar /etc/hosts
#   sudo vi /etc/hosts
#   # Agregar:
#   192.168.122.100  servidor-rhcsa.lab.local  servidor-rhcsa
#
# === FIREWALL ===
#
# 8. Verificar estado del firewall
#   sudo systemctl status firewalld
#   sudo firewall-cmd --state
#
# 9. Ver zona por defecto y reglas
#   firewall-cmd --get-default-zone
#   firewall-cmd --list-all
#
# 10. Agregar servicios
#   sudo firewall-cmd --add-service=http --permanent
#   sudo firewall-cmd --add-service=https --permanent
#
# 11. Abrir puerto custom
#   sudo firewall-cmd --add-port=8080/tcp --permanent
#
# 12. Aplicar cambios
#   sudo firewall-cmd --reload
#
# 13. Verificar
#   sudo firewall-cmd --list-all
#   sudo firewall-cmd --list-services
#   sudo firewall-cmd --list-ports
#
# 14. Bloquear IP con rich rules
#   sudo firewall-cmd --add-rich-rule='rule family="ipv4" source address="192.168.122.200" reject' --permanent
#   sudo firewall-cmd --reload
#
# 15. Listar rich rules
#   sudo firewall-cmd --list-rich-rules
#
# 16. Eliminar regla
#   sudo firewall-cmd --remove-rich-rule='rule family="ipv4" source address="192.168.122.200" reject' --permanent
#   sudo firewall-cmd --reload
#
# === EXTRAS UTILES ===
#
# Ver rutas
#   ip route show
#
# Ver puertos en uso
#   ss -tlnp
#
# DNS lookup
#   dig google.com
#   nslookup google.com
#
# Traceroute
#   traceroute 8.8.8.8
#
# IMPORTANTE PARA EL EXAMEN:
# - Siempre usar --permanent para que sobreviva al reboot
# - Siempre hacer --reload despues de cambios permanentes
# - nmcli es el comando principal para redes en RHEL 9
# ============================================================

echo "Este archivo es una guia de referencia para redes y firewall."
echo "Practica en tu VM RHEL 9."
