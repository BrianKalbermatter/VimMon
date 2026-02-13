#!/bin/bash
# ============================================================
# FASE 3 - Ejercicio 3.9: Reseteo de contraseña root
# ============================================================
# OBJETIVO: Saber resetear la contraseña de root desde GRUB
# ESTO TE LO TOMAN SEGURO EN EL RHCSA - APRENDERLO DE MEMORIA
#
# === PROCEDIMIENTO PASO A PASO ===
#
# PASO 1: Reiniciar la VM
#   sudo reboot
#
# PASO 2: En el menu GRUB
#   - Presionar 'e' para editar la entrada del kernel
#   - Buscar la linea que empieza con "linux" (la linea del kernel)
#   - Ir al final de esa linea
#   - Agregar: rd.break
#   - Presionar Ctrl+X para arrancar con esos parametros
#
# PASO 3: Entras a modo de emergencia
#   - Vas a ver un prompt: switch_root:/#
#   - El filesystem real esta en /sysroot (montado en solo lectura)
#
# PASO 4: Remontar /sysroot como lectura/escritura
#   mount -o remount,rw /sysroot
#
# PASO 5: Entrar al filesystem real
#   chroot /sysroot
#
# PASO 6: Cambiar la contraseña de root
#   passwd root
#   # Ingresar nueva contraseña 2 veces
#
# PASO 7: Crear archivo para que SELinux re-etiquete
#   touch /.autorelabel
#   # ESTO ES CRITICO - sin esto SELinux puede bloquear el login
#
# PASO 8: Salir
#   exit          # Salir del chroot
#   exit          # Salir de switch_root (o reboot)
#
# PASO 9: Esperar
#   - El sistema va a reiniciar
#   - Si creaste /.autorelabel, va a tardar un rato re-etiquetando
#   - NO interrumpas este proceso
#
# PASO 10: Verificar
#   - Login como root con la nueva contraseña
#   - Debe funcionar!
#
# === PRACTICA ===
#
# Repeti este proceso MINIMO 5 veces hasta que lo hagas sin pensar.
# En el examen tenes tiempo limitado, no podes dudar en esto.
#
# Checklist rapido:
# [ ] Editar GRUB → agregar rd.break
# [ ] mount -o remount,rw /sysroot
# [ ] chroot /sysroot
# [ ] passwd root
# [ ] touch /.autorelabel
# [ ] exit, exit
# [ ] Verificar login
#
# === ERRORES COMUNES ===
#
# - Olvidar /.autorelabel → SELinux bloquea el login
# - Olvidar remontar rw → passwd falla porque es read-only
# - Escribir rd.break en la linea equivocada de GRUB
# - Interrumpir el proceso de autorelabel
#
# NOTA: Este procedimiento funciona en RHEL 7, 8 y 9
# ============================================================

echo "=== RESETEO DE CONTRASENA ROOT ==="
echo ""
echo "MEMORIZA ESTOS PASOS:"
echo ""
echo "1. Reboot → 'e' en GRUB"
echo "2. Agregar 'rd.break' al final de la linea linux"
echo "3. Ctrl+X"
echo "4. mount -o remount,rw /sysroot"
echo "5. chroot /sysroot"
echo "6. passwd root"
echo "7. touch /.autorelabel"
echo "8. exit → exit"
echo ""
echo "PRACTICA ESTO HASTA QUE LO HAGAS DE MEMORIA!"
