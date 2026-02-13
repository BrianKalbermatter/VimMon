#!/bin/bash
# ============================================================
# FASE 3 - Ejercicio 3.1: Gestion de paquetes (dnf/rpm)
# ============================================================
# OBJETIVO: Dominar dnf y rpm para el RHCSA
#
# INSTRUCCIONES (ejecuta cada comando manualmente):
#
# 1. Buscar el paquete httpd
#    dnf search httpd
#
# 2. Ver info del paquete antes de instalar
#    dnf info httpd
#
# 3. Instalar httpd
#    sudo dnf install httpd -y
#
# 4. Listar archivos instalados por httpd
#    rpm -ql httpd
#
# 5. Que paquete provee /usr/sbin/ip ?
#    rpm -qf /usr/sbin/ip
#
# 6. Verificar paquete (archivos modificados)
#    rpm -V httpd
#
# 7. Descargar RPM sin instalar
#    dnf download nginx
#    ls *.rpm
#
# 8. Instalar RPM descargado
#    sudo rpm -ivh nginx*.rpm
#
# 9. Listar todos los paquetes instalados
#    rpm -qa | wc -l
#    rpm -qa | grep http
#
# 10. Listar repositorios habilitados
#     dnf repolist
#
# === CREAR REPOSITORIO LOCAL ===
#
# 11. Crear directorio para el repo
#     sudo mkdir -p /opt/mi_repo
#
# 12. Copiar algunos RPMs ahi
#     sudo cp *.rpm /opt/mi_repo/
#
# 13. Instalar createrepo
#     sudo dnf install createrepo_c -y
#
# 14. Generar metadata
#     sudo createrepo /opt/mi_repo
#
# 15. Crear archivo de configuracion del repo
#     sudo vi /etc/yum.repos.d/mi_repo.repo
#
#     [mi_repo]
#     name=Mi Repositorio Local
#     baseurl=file:///opt/mi_repo
#     enabled=1
#     gpgcheck=0
#
# 16. Verificar que el repo aparece
#     dnf repolist
#
# 17. Instalar desde tu repo
#     sudo dnf install --repo=mi_repo <paquete>
#
# === COMANDOS EXTRAS UTILES ===
#
# dnf history              → ver historial de transacciones
# dnf history undo <id>    → deshacer una transaccion
# dnf grouplist            → ver grupos de paquetes
# dnf groupinstall "Development Tools"  → instalar grupo
# dnf clean all            → limpiar cache
# ============================================================

echo "Este archivo es una guia de referencia."
echo "Ejecuta cada comando manualmente en tu VM RHEL 9."
