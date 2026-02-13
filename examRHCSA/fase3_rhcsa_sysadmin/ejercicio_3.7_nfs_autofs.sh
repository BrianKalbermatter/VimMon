#!/bin/bash
# ============================================================
# FASE 3 - Ejercicio 3.7: NFS y Autofs
# ============================================================
# OBJETIVO: Compartir y montar filesystems por red
# PREREQUISITO: 2 VMs (servidor y cliente) en la misma red
#
# === EN EL SERVIDOR (192.168.122.10) ===
#
# 1. Instalar NFS
#   sudo dnf install nfs-utils -y
#
# 2. Crear directorios a compartir
#   sudo mkdir -p /exports/datos
#   sudo mkdir -p /exports/home_remoto
#   echo "archivo de prueba" | sudo tee /exports/datos/test.txt
#   echo "home remoto" | sudo tee /exports/home_remoto/readme.txt
#
# 3. Configurar /etc/exports
#   sudo vi /etc/exports
#
#   /exports/datos         192.168.122.0/24(rw,sync,no_root_squash)
#   /exports/home_remoto   192.168.122.0/24(ro,sync)
#
# 4. Aplicar configuracion
#   sudo exportfs -rav
#
# 5. Iniciar NFS
#   sudo systemctl enable --now nfs-server
#
# 6. Abrir firewall
#   sudo firewall-cmd --add-service=nfs --permanent
#   sudo firewall-cmd --add-service=mountd --permanent
#   sudo firewall-cmd --add-service=rpc-bind --permanent
#   sudo firewall-cmd --reload
#
# 7. Verificar
#   showmount -e localhost
#
# === EN EL CLIENTE (192.168.122.20) ===
#
# 8. Instalar NFS utils
#   sudo dnf install nfs-utils -y
#
# 9. Verificar que ve los shares del servidor
#   showmount -e 192.168.122.10
#
# 10. Montar manualmente
#   sudo mkdir -p /mnt/datos
#   sudo mount 192.168.122.10:/exports/datos /mnt/datos
#   ls /mnt/datos
#   cat /mnt/datos/test.txt
#
# 11. Montar persistente en /etc/fstab
#   # Agregar a /etc/fstab:
#   192.168.122.10:/exports/datos  /mnt/datos  nfs  defaults  0 0
#
# === AUTOFS (montaje automatico) ===
#
# 12. Instalar autofs
#   sudo dnf install autofs -y
#
# 13. Configurar master map
#   sudo vi /etc/auto.master.d/remote.autofs
#
#   /remote  /etc/auto.remote
#
# 14. Crear el mapa
#   sudo vi /etc/auto.remote
#
#   datos   -rw,sync  192.168.122.10:/exports/datos
#   home    -ro,sync  192.168.122.10:/exports/home_remoto
#
# 15. Iniciar autofs
#   sudo systemctl enable --now autofs
#
# 16. Verificar (no montar manualmente, solo acceder!)
#   ls /remote/           # Puede estar vacio, es normal
#   cd /remote/datos      # Se monta automaticamente!
#   ls                    # Veras los archivos
#   cat test.txt
#
#   cd /remote/home       # Se monta automaticamente!
#   ls
#
# 17. Esperar 5 minutos sin acceder y verificar
#   cd /tmp
#   # Esperar...
#   mount | grep remote   # Ya no deberia estar montado
#   cd /remote/datos      # Se monta de nuevo automaticamente
#
# IMPORTANTE PARA EL EXAMEN:
# - Autofs es tema casi seguro del RHCSA
# - El directorio /remote NO debe existir antes, autofs lo crea
# - Verificar siempre con: systemctl status autofs
# - Si no funciona: journalctl -u autofs
# ============================================================

echo "Este archivo es una guia de referencia para NFS y Autofs."
echo "Necesitas 2 VMs para practicar este ejercicio."
