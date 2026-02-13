#!/bin/bash
# ============================================================
# FASE 3 - Ejercicio 3.3: Almacenamiento LVM
# ============================================================
# OBJETIVO: Dominar LVM (tema CLAVE del RHCSA)
#
# PREREQUISITO: Agregar un disco de 2GB a tu VM
# En QEMU: -drive file=disco2.qcow2,format=qcow2
# En VirtualBox: Settings > Storage > Add Hard Disk
#
# === INSTRUCCIONES ===
#
# PASO 1: Verificar el nuevo disco
#   lsblk
#   # Deberia aparecer como /dev/sdb o /dev/vdb
#
# PASO 2: Crear particiones (2 de 1GB)
#   sudo fdisk /dev/sdb
#   # n → nueva particion → +1G → tipo 8e (Linux LVM)
#   # Repetir para la segunda
#   # w → escribir y salir
#
#   lsblk  # Verificar: sdb1 y sdb2
#
# PASO 3: Crear Physical Volumes (PV)
#   sudo pvcreate /dev/sdb1 /dev/sdb2
#   sudo pvs          # Listar PVs
#   sudo pvdisplay    # Detalle de PVs
#
# PASO 4: Crear Volume Group (VG)
#   sudo vgcreate vg_datos /dev/sdb1 /dev/sdb2
#   sudo vgs          # Listar VGs
#   sudo vgdisplay    # Detalle
#
# PASO 5: Crear Logical Volumes (LV)
#   sudo lvcreate -n lv_web -L 800M vg_datos
#   sudo lvcreate -n lv_db -L 500M vg_datos
#   sudo lvs          # Listar LVs
#
# PASO 6: Crear filesystems
#   sudo mkfs.xfs /dev/vg_datos/lv_web
#   sudo mkfs.ext4 /dev/vg_datos/lv_db
#
# PASO 7: Montar
#   sudo mkdir -p /mnt/web /mnt/db
#   sudo mount /dev/vg_datos/lv_web /mnt/web
#   sudo mount /dev/vg_datos/lv_db /mnt/db
#   df -h  # Verificar
#
# PASO 8: Hacer persistente en /etc/fstab
#   # Obtener UUID
#   blkid /dev/vg_datos/lv_web
#   blkid /dev/vg_datos/lv_db
#
#   # Agregar a /etc/fstab:
#   /dev/vg_datos/lv_web  /mnt/web  xfs   defaults  0 0
#   /dev/vg_datos/lv_db   /mnt/db   ext4  defaults  0 0
#
#   # Verificar sin reiniciar:
#   sudo umount /mnt/web /mnt/db
#   sudo mount -a
#   df -h
#
# PASO 9: Reiniciar y verificar
#   sudo reboot
#   df -h  # Deben estar montados
#
# PASO 10: Extender lv_web a 1.2GB SIN PERDER DATOS
#   # Poner datos de prueba primero:
#   echo "datos importantes" | sudo tee /mnt/web/test.txt
#
#   # Extender:
#   sudo lvextend -L 1.2G /dev/vg_datos/lv_web
#
#   # Extender el filesystem (XFS):
#   sudo xfs_growfs /mnt/web
#
#   # Para ext4 seria:
#   # sudo resize2fs /dev/vg_datos/lv_db
#
#   # Verificar:
#   df -h /mnt/web
#   lvs
#   cat /mnt/web/test.txt  # Los datos siguen ahi
#
# === RESUMEN DE COMANDOS LVM ===
#
# PV: pvcreate, pvs, pvdisplay, pvremove
# VG: vgcreate, vgs, vgdisplay, vgextend, vgremove
# LV: lvcreate, lvs, lvdisplay, lvextend, lvreduce, lvremove
#
# IMPORTANTE PARA EL EXAMEN:
# - Siempre verificar con lvs, vgs, pvs despues de cada paso
# - Recordar extender el filesystem DESPUES de extender el LV
# - XFS solo se puede extender (no reducir)
# - ext4 se puede extender y reducir
# ============================================================

echo "Este archivo es una guia de referencia para LVM."
echo "Ejecuta cada comando paso a paso en tu VM con un disco extra."
echo ""
echo "PRACTICA ESTO MUCHAS VECES - es tema fijo del RHCSA."
