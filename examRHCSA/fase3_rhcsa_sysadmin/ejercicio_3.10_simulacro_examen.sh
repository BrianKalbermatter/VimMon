#!/bin/bash
# ============================================================
# FASE 3 - Ejercicio 3.10: SIMULACRO DE EXAMEN RHCSA
# ============================================================
# TIEMPO: 2 horas 30 minutos
# REGLAS: VM limpia RHEL 9, sin consultar nada
#
# Si podes completar todo sin ayuda, estas listo para el examen.
# ============================================================
#
# TAREA 1: Configuracion basica
# - Configurar hostname: exam.lab.local
# - Configurar IP estatica: 192.168.122.50/24
# - Gateway: 192.168.122.1
# - DNS: 8.8.8.8
#
# TAREA 2: Repositorio
# - Configurar el repositorio de paquetes de RHEL
# - (Usa el ISO montado o un repo online)
# - Verificar con: dnf repolist
#
# TAREA 3: SELinux
# - Verificar que SELinux esta en modo enforcing
# - Si no lo esta, configurarlo en enforcing
# - Verificar con: getenforce
#
# TAREA 4: Usuarios y grupos
# - Crear grupo "admins"
# - Crear usuario "admin1" con UID 1100 y grupo primario "admins"
# - Crear usuario "dev1" con shell /bin/bash
# - Crear usuario "nologin1" con shell /sbin/nologin
# - Configurar password "redhat" para todos
#
# TAREA 5: Sudo
# - Configurar que admin1 pueda hacer sudo sin contraseña
# - Verificar con: sudo -u admin1 sudo whoami
#
# TAREA 6: Directorio colaborativo
# - Crear /datos/compartido
# - Grupo propietario: admins
# - Activar SGID
# - Permisos: rwxrwx--- (770)
# - Verificar que archivos creados dentro heredan el grupo admins
#
# TAREA 7: Almacenamiento LVM
# - Agregar un disco de 2GB a la VM
# - Crear un VG llamado "vg_exam"
# - Crear un LV llamado "lv_data" de 500MB con filesystem XFS
# - Montarlo persistente en /mnt/datos (sobrevive reboot)
#
# TAREA 8: Cron
# - Crear un cron job para root que haga backup de /etc
#   a las 2:00 AM todos los dias
# - El backup debe ir a /tmp/backup_etc.tar.gz
#
# TAREA 9: Apache con puerto custom
# - Instalar httpd
# - Cambiar el puerto a 82
# - Crear un index.html con contenido "Exam Server"
# - Abrir el puerto 82 en firewall
# - Configurar SELinux para permitir el puerto 82
# - Verificar: curl http://localhost:82
#
# TAREA 10: Autofs
# - Configurar autofs para montar automaticamente
#   un share NFS de un servidor (si tenes 2 VMs)
# - Si solo tenes 1 VM, configura NFS local y autofs
#
# TAREA 11: Containers
# - Descargar la imagen de httpd
# - Ejecutar un container con:
#   - Nombre: web_exam
#   - Puerto: 8888:80
# - Crear un servicio systemd para el container
# - Verificar: curl http://localhost:8888
#
# TAREA 12: Extender LVM
# - Extender lv_data de 500MB a 800MB
# - Extender el filesystem XFS sin perder datos
# - Verificar con: df -h /mnt/datos y lvs
#
# TAREA 13: Reset de contraseña root
# - Reiniciar y resetear la contraseña de root usando rd.break
# - Verificar login con nueva contraseña
#
# ============================================================
# PUNTUACION:
# Cada tarea vale puntos. Necesitas 70% para aprobar.
# En el examen real son tareas similares a estas.
#
# CUANDO TERMINES:
# - Reiniciar la VM
# - Verificar que TODO sigue funcionando despues del reboot:
#   * LVM montado
#   * httpd corriendo en puerto 82
#   * Container como servicio activo
#   * cron configurado
#   * Permisos y usuarios correctos
#
# Si algo no sobrevive el reboot, NO aprobas esa tarea.
# ============================================================

echo "=== SIMULACRO EXAMEN RHCSA ==="
echo ""
echo "TIEMPO: 2 horas 30 minutos"
echo ""
echo "REGLAS:"
echo "- VM limpia RHEL 9"
echo "- Sin consultar internet, libros ni apuntes"
echo "- Todo debe sobrevivir un reboot"
echo ""
echo "Tenes 13 tareas. Lee el archivo para ver los detalles."
echo "Si completaste todo sin ayuda, estas listo para el RHCSA."
echo ""
echo "BUENA SUERTE!"
