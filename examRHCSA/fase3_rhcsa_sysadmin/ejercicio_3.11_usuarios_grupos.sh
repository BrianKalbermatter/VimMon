#!/bin/bash
# ============================================================
# FASE 3 - Ejercicio 3.11: Usuarios, Grupos y Sudo avanzado
# ============================================================
# OBJETIVO: Dominar gestion de usuarios, grupos, chage y sudoers
#
# === PARTE 1: Grupos con GID especifico ===
#
# 1. Crear grupo devops con GID 2000
#   groupadd -g 2000 devops
#
# 2. Crear grupo dbadmins con GID 3000
#   groupadd -g 3000 dbadmins
#
# 3. Verificar
#   getent group devops
#   getent group dbadmins
#   # Tambien podes ver en /etc/group:
#   grep -E 'devops|dbadmins' /etc/group
#
# === PARTE 2: Crear usuarios con opciones especificas ===
#
# 4. Crear alice con UID 1500 y grupo secundario devops
#   useradd -u 1500 -G devops alice
#
# 5. Crear bob con grupo secundario devops
#   useradd -G devops bob
#
# 6. Crear charlie sin grupo secundario extra
#   useradd charlie
#
# 7. Crear usuario de servicio (sin login, sin home)
#   useradd -s /sbin/nologin -M svc_app
#
# 8. Asignar passwords
#   echo "alice:password123" | chpasswd
#   echo "bob:password123" | chpasswd
#   echo "charlie:password123" | chpasswd
#
# 9. Verificar
#   id alice       # uid=1500, groups=devops
#   id bob         # groups=devops
#   id charlie     # solo su grupo primario
#   id svc_app     # shell /sbin/nologin
#
# === PARTE 3: Modificar usuarios existentes ===
#
# 10. Agregar charlie al grupo devops despues de creado
#   usermod -aG devops charlie
#   # IMPORTANTE: -a es "append", sin -a REEMPLAZA los grupos secundarios
#
# 11. Agregar bob tambien a dbadmins
#   usermod -aG dbadmins bob
#   id bob   # debe mostrar devops Y dbadmins
#
# 12. Cambiar shell de charlie
#   usermod -s /bin/zsh charlie
#   grep charlie /etc/passwd
#
# === PARTE 4: chage - Politicas de password ===
#
# 13. Forzar a charlie a cambiar password en proximo login
#   chage -d 0 charlie
#   # -d 0 pone "last password change" en epoch 0 (1 enero 1970)
#   # El sistema lo interpreta como "password expirado"
#
# 14. Verificar
#   chage -l charlie
#   # "Last password change" debe decir: password must be changed
#
# 15. Configurar que la cuenta de bob expire el 31 dic 2025
#   chage -E 2025-12-31 bob
#
# 16. Configurar politica de password para alice:
#   # - Minimo 7 dias entre cambios de password
#   # - Maximo 90 dias antes de que expire
#   # - Aviso 14 dias antes de que expire
#   chage -m 7 -M 90 -W 14 alice
#
# 17. Verificar todas las politicas
#   chage -l alice
#   chage -l bob
#   chage -l charlie
#
# === PARTE 5: Sudoers avanzado ===
#
# REGLA DE ORO: NUNCA editar /etc/sudoers directamente
# Siempre usar archivos en /etc/sudoers.d/
#
# 18. alice puede ejecutar TODO sin password
#   echo "alice ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/alice
#   chmod 440 /etc/sudoers.d/alice
#
# 19. Grupo devops puede reiniciar httpd sin password
#   echo "%devops ALL=(ALL) NOPASSWD: /usr/bin/systemctl restart httpd" > /etc/sudoers.d/devops
#   chmod 440 /etc/sudoers.d/devops
#
# 20. Grupo dbadmins puede ejecutar comandos de postgres sin password
#   echo "%dbadmins ALL=(ALL) NOPASSWD: /usr/bin/systemctl restart postgresql, /usr/bin/systemctl status postgresql" > /etc/sudoers.d/dbadmins
#   chmod 440 /etc/sudoers.d/dbadmins
#
# 21. Verificar sintaxis (MUY IMPORTANTE)
#   visudo -cf /etc/sudoers.d/alice
#   visudo -cf /etc/sudoers.d/devops
#   visudo -cf /etc/sudoers.d/dbadmins
#   # Debe decir "parsed OK"
#
# 22. Probar
#   sudo -u alice sudo whoami        # debe decir root
#   sudo -u bob sudo systemctl restart httpd  # debe funcionar
#
# === PARTE 6: Bloquear y eliminar usuarios ===
#
# 23. Bloquear cuenta de charlie (no puede hacer login)
#   usermod -L charlie
#   # Esto pone un ! delante del hash del password en /etc/shadow
#
# 24. Desbloquear
#   usermod -U charlie
#
# 25. Eliminar usuario svc_app con su home
#   userdel -r svc_app
#
# === RESUMEN DE COMANDOS ===
#
# groupadd -g GID nombre       → crear grupo con GID especifico
# useradd -u UID -G grupo usr  → crear usuario con UID y grupo secundario
# usermod -aG grupo usuario    → agregar grupo secundario (sin -a reemplaza!)
# chage -d 0 usuario           → forzar cambio de password
# chage -E YYYY-MM-DD usuario  → fecha de expiracion de cuenta
# chage -m MIN -M MAX -W WARN  → politica de password
# chage -l usuario             → ver politica actual
# /etc/sudoers.d/              → archivos de sudo (chmod 440!)
# visudo -cf archivo           → verificar sintaxis de sudoers
#
# IMPORTANTE PARA EL EXAMEN:
# - usermod -aG (con -a!) para agregar grupo sin perder los otros
# - chage -d 0 para forzar cambio de password
# - Sudoers siempre en /etc/sudoers.d/ con chmod 440
# - Siempre verificar con visudo -cf
# ============================================================

echo "Este archivo es una guia de referencia para usuarios y grupos."
echo "Ejecuta cada comando manualmente en tu VM RHEL 9."
