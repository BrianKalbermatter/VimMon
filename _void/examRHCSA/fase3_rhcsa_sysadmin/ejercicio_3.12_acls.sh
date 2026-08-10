#!/usr/bin/env bash
# ============================================================
# FASE 3 - Ejercicio 3.12: ACLs (Access Control Lists)
# ============================================================
# OBJETIVO: Manejar permisos avanzados con ACLs
# Los permisos Unix normales (rwx) solo permiten definir permisos
# para UN dueno, UN grupo y "otros". ACLs te dan control granular
# por usuario o grupo individual.
#
# PREREQUISITO: filesystem con soporte ACL (ext4, xfs lo tienen)
#
# === PARTE 1: Permisos clasicos + setgid ===
#
# 1. Crear directorio colaborativo
#   mkdir -p /shared/devops
#
# 2. Configurar dueno root, grupo devops
#   chown root:devops /shared/devops
#
# 3. Permisos: dueno rwx, grupo rwx, otros nada + setgid
#   chmod 2770 /shared/devops
#   # 2 = setgid → archivos nuevos heredan el grupo devops
#   # 7 = rwx para dueno
#   # 7 = rwx para grupo
#   # 0 = nada para otros
#
# 4. Verificar
#   ls -ld /shared/devops
#   # drwxrws--- → la "s" en grupo indica setgid activo
#
# 5. Probar herencia de grupo
#   touch /shared/devops/archivo_test
#   ls -l /shared/devops/archivo_test
#   # El grupo debe ser "devops" aunque lo creo root
#   rm /shared/devops/archivo_test
#
# === PARTE 2: ACLs basicas ===
#
# 6. Crear directorio para practicar ACLs
#   mkdir -p /shared/logs
#   chown root:root /shared/logs
#   chmod 700 /shared/logs
#
# 7. Dar acceso de lectura+escritura a alice
#   setfacl -m u:alice:rw /shared/logs
#   # -m = modify
#   # u:alice:rw = usuario alice, permisos rw
#
# 8. Dar acceso de solo lectura a bob
#   setfacl -m u:bob:r /shared/logs
#
# 9. Verificar las ACLs
#   getfacl /shared/logs
#   # Vas a ver:
#   # user::rwx          ← permisos del dueno (root)
#   # user:alice:rw-      ← ACL para alice
#   # user:bob:r--        ← ACL para bob
#   # group::---          ← permisos del grupo
#   # mask::rw-           ← mascara (limita permisos efectivos)
#   # other::---          ← permisos para otros
#
# 10. Notar el "+" en ls -l
#   ls -ld /shared/logs
#   # drwx------+ ← el + indica que hay ACLs configuradas
#
# === PARTE 3: ACLs para grupos ===
#
# 11. Dar acceso rwx al grupo devops en /shared/logs
#   setfacl -m g:devops:rwx /shared/logs
#
# 12. Verificar
#   getfacl /shared/logs
#
# === PARTE 4: ACLs por defecto (herencia) ===
#
# Las ACLs por defecto se aplican automaticamente a archivos
# y subdirectorios nuevos creados dentro del directorio.
#
# 13. Configurar ACLs por defecto
#   setfacl -m d:u:alice:rw /shared/logs
#   setfacl -m d:u:bob:r /shared/logs
#   setfacl -m d:g:devops:rwx /shared/logs
#   # d: = default (herencia)
#
# 14. Verificar
#   getfacl /shared/logs
#   # Vas a ver secciones "default:" adicionales
#
# 15. Probar la herencia
#   touch /shared/logs/nuevo_archivo.txt
#   mkdir /shared/logs/subdir
#   getfacl /shared/logs/nuevo_archivo.txt
#   getfacl /shared/logs/subdir
#   # Deben tener las ACLs heredadas
#
# === PARTE 5: Eliminar ACLs ===
#
# 16. Eliminar ACL especifica de bob
#   setfacl -x u:bob /shared/logs
#   getfacl /shared/logs
#
# 17. Eliminar TODAS las ACLs
#   setfacl -b /shared/logs
#   getfacl /shared/logs
#   # Ya no hay ACLs extra
#   ls -ld /shared/logs
#   # Ya no tiene el "+"
#
# === PARTE 6: La mascara (mask) ===
#
# La mascara limita los permisos EFECTIVOS de ACLs y grupo.
# Si la mascara es r--, aunque una ACL diga rwx, el efectivo es r--.
#
# 18. Reconfigar ACLs para demostrar
#   setfacl -m u:alice:rwx /shared/logs
#   setfacl -m m::r /shared/logs
#   # m::r = mascara solo lectura
#
# 19. Verificar permisos efectivos
#   getfacl /shared/logs
#   # user:alice:rwx    #effective:r--
#   # La mascara limito a alice a solo lectura
#
# 20. Restaurar mascara
#   setfacl -m m::rwx /shared/logs
#
# === PARTE 7: Backup y restaurar ACLs ===
#
# 21. Guardar ACLs a un archivo
#   getfacl -R /shared/logs > /tmp/acls_backup.txt
#
# 22. Borrar todas las ACLs
#   setfacl -bR /shared/logs
#
# 23. Restaurar desde backup
#   setfacl --restore=/tmp/acls_backup.txt
#   getfacl /shared/logs
#
# === PARTE 8: Ejercicio integrador ===
#
# Crear /proyecto con estos requisitos:
# - Dueno: root
# - Grupo primario: devops (setgid activo)
# - alice: rwx (puede hacer todo)
# - bob: rx (puede leer y ejecutar, no escribir)
# - charlie: r (solo lectura)
# - Grupo dbadmins: rx
# - Otros: sin acceso
# - Los nuevos archivos heredan estas ACLs
#
# Solucion:
#   mkdir -p /proyecto
#   chown root:devops /proyecto
#   chmod 2770 /proyecto
#   setfacl -m u:alice:rwx /proyecto
#   setfacl -m u:bob:rx /proyecto
#   setfacl -m u:charlie:r /proyecto
#   setfacl -m g:dbadmins:rx /proyecto
#   setfacl -m d:u:alice:rwx /proyecto
#   setfacl -m d:u:bob:rx /proyecto
#   setfacl -m d:u:charlie:r /proyecto
#   setfacl -m d:g:dbadmins:rx /proyecto
#   getfacl /proyecto
#
# === RESUMEN DE COMANDOS ===
#
# setfacl -m u:USER:PERMS FILE   → agregar/modificar ACL de usuario
# setfacl -m g:GROUP:PERMS FILE  → agregar/modificar ACL de grupo
# setfacl -m d:u:USER:PERMS DIR  → ACL por defecto (herencia)
# setfacl -x u:USER FILE         → eliminar ACL especifica
# setfacl -b FILE                → eliminar TODAS las ACLs
# setfacl -R ...                 → aplicar recursivamente
# getfacl FILE                   → ver ACLs de un archivo
# getfacl -R DIR > backup        → backup de ACLs
# setfacl --restore=backup       → restaurar ACLs
#
# IMPORTANTE PARA EL EXAMEN:
# - Si ves un "+" en ls -l, hay ACLs → usa getfacl
# - ACLs por defecto (d:) son para herencia en directorios
# - La mascara limita permisos efectivos
# - chmod 2770 = setgid (herencia de grupo, no ACL)
# - setfacl y getfacl son los dos comandos clave
# ============================================================

echo "Este archivo es una guia de referencia para ACLs."
echo "Ejecuta cada comando manualmente en tu VM RHEL 9."
