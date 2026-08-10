#!/learnC/bash
# ============================================================
# FASE 3 - Ejercicio 3.13: Troubleshooting de Boot
# ============================================================
# OBJETIVO: Resolver problemas de arranque del sistema
# Complementa el 3.9 (reset root) con mas escenarios
#
# === PROBLEMA 1: fstab invalido (emergency mode) ===
#
# Este es un clasico del examen: el sistema no arranca porque
# hay una linea mala en /etc/fstab.
#
# SIMULAR EL PROBLEMA (solo en VM de practica!):
#
# 1. Agregar una linea invalida a fstab
#   echo '/dev/sdXXX /mnt/fake ext4 defaults 0 0' >> /etc/fstab
#
# 2. Reiniciar
#   reboot
#
# 3. El sistema va a caer en emergency mode
#   # Va a mostrar algo como:
#   # "Welcome to emergency mode!"
#   # "Give root password for maintenance"
#   # (or press Control-D to continue)
#
# SOLUCION:
#
# 4. Ingresar password de root
#
# 5. Remontar / como lectura-escritura
#   mount -o remount,rw /
#
# 6. Editar fstab y borrar/comentar la linea invalida
#   vi /etc/fstab
#   # Buscar la linea con /dev/sdXXX y borrarla
#   # O comentarla con # al inicio
#
# 7. Verificar que fstab esta bien
#   mount -a
#   # Si no da error, esta bien
#
# 8. Reiniciar
#   systemctl reboot
#   # O: reboot
#
# 9. El sistema debe arrancar normalmente
#
# CONSEJO: Siempre hacer mount -a ANTES de reiniciar despues
# de editar fstab, asi detectas errores sin tener que reiniciar.
#
# === PROBLEMA 2: Cambiar el target por defecto ===
#
# Los targets de systemd reemplazan a los runlevels de SysVinit:
#
# Target                    | Runlevel | Descripcion
# --------------------------|----------|---------------------------
# poweroff.target           | 0        | Apagar
# rescue.target             | 1        | Modo rescate (single user)
# multi-user.target         | 3        | Multi-usuario SIN GUI
# graphical.target          | 5        | Multi-usuario CON GUI
# reboot.target             | 6        | Reiniciar
#
# 10. Ver target actual
#   systemctl get-default
#
# 11. Cambiar a multi-user (sin GUI)
#   systemctl set-default multi-user.target
#   # Esto crea un symlink:
#   # /etc/systemd/system/default.target → multi-user.target
#
# 12. Verificar
#   systemctl get-default
#   # Debe decir: multi-user.target
#
# 13. Cambiar a graphical (con GUI)
#   systemctl set-default graphical.target
#
# 14. Cambiar target SIN reiniciar (para esta sesion)
#   systemctl isolate multi-user.target
#   # CUIDADO: esto cambia inmediatamente, cierra la GUI si tenes
#
#   systemctl isolate graphical.target
#   # Vuelve a la GUI
#
# === PROBLEMA 3: Arrancar en target especifico desde GRUB ===
#
# Si el sistema no arranca bien, podes forzar un target desde GRUB.
#
# 15. Reiniciar y en GRUB presionar 'e'
#
# 16. En la linea del kernel (linux), agregar al final:
#   systemd.unit=rescue.target
#   # Esto arranca en modo rescate (single user con root)
#
#   # O para emergency (mas basico, sin montar nada):
#   systemd.unit=emergency.target
#
# 17. Ctrl+X para arrancar
#
# 18. Desde ahi podes:
#   - Arreglar fstab
#   - Arreglar configuraciones rotas
#   - Cambiar el default target
#   - Etc.
#
# 19. Cuando termines, reiniciar:
#   systemctl reboot
#
# === PROBLEMA 4: journalctl para diagnostico ===
#
# 20. Ver logs del ultimo boot
#   journalctl -b
#
# 21. Ver logs del boot anterior (si fallo)
#   journalctl -b -1
#   # -1 = boot anterior, -2 = dos boots atras, etc.
#
# 22. Ver solo errores
#   journalctl -b -p err
#   # Prioridades: emerg, alert, crit, err, warning, notice, info, debug
#
# 23. Ver logs de un servicio especifico
#   journalctl -u httpd
#   journalctl -u sshd --since "1 hour ago"
#
# 24. Seguir logs en tiempo real
#   journalctl -f
#
# 25. Hacer el journal persistente (sobrevive reboot)
#   mkdir -p /var/log/journal
#   systemctl restart systemd-journald
#   # Ahora journalctl -b -1 funciona despues de reiniciar
#
# === PROBLEMA 5: Servicio que no arranca ===
#
# 26. Diagnosticar un servicio que falla
#   systemctl status httpd
#   # Buscar "Active: failed" y la razon
#
# 27. Ver logs detallados del servicio
#   journalctl -xeu httpd
#   # -x = info extra
#   # -e = ir al final
#   # -u = unidad especifica
#
# 28. Verificar configuracion del servicio
#   systemctl cat httpd
#   # Muestra el contenido del unit file
#
# 29. Listar dependencias
#   systemctl list-dependencies httpd
#
# 30. Verificar que el servicio esta habilitado
#   systemctl is-enabled httpd
#   # enabled = arranca con el boot
#   # disabled = no arranca automaticamente
#
# === RESUMEN DE COMANDOS ===
#
# mount -o remount,rw /             → remontar / como rw en emergency
# mount -a                          → montar todo de fstab (verificar)
# systemctl get-default             → ver target de arranque
# systemctl set-default TARGET      → cambiar target de arranque
# systemctl isolate TARGET          → cambiar target en caliente
# journalctl -b                     → logs del boot actual
# journalctl -b -1                  → logs del boot anterior
# journalctl -b -p err              → solo errores del boot
# journalctl -xeu SERVICIO          → logs detallados de un servicio
#
# GRUB:
# rd.break                          → reset root password
# systemd.unit=rescue.target        → modo rescate
# systemd.unit=emergency.target     → modo emergencia
#
# IMPORTANTE PARA EL EXAMEN:
# - Si el sistema no arranca, PRIMERO lee el error en pantalla
# - fstab roto = emergency mode → remontar rw → editar → reboot
# - Siempre hacer mount -a despues de editar fstab
# - journalctl -b -1 te muestra que paso en el boot fallido
# - Saber la diferencia entre rescue y emergency target
# ============================================================

echo "Este archivo es una guia de referencia para troubleshooting de boot."
echo "Practica ROMPIENDO y ARREGLANDO tu VM. No tengas miedo de romperla."
