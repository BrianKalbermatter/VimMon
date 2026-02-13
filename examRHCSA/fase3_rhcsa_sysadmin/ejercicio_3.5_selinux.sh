#!/bin/bash
# ============================================================
# FASE 3 - Ejercicio 3.5: SELinux
# ============================================================
# OBJETIVO: Entender y resolver problemas de SELinux
# (tema MUY importante en el RHCSA)
#
# === CONCEPTOS ===
# SELinux agrega una capa extra de seguridad sobre los permisos Unix.
# Cada archivo y proceso tiene un "contexto" SELinux.
# Si el contexto no coincide, el acceso se deniega.
#
# === EJERCICIO PASO A PASO ===
#
# 1. Verificar modo de SELinux
#   getenforce               # Debe decir "Enforcing"
#   sestatus                  # Info completa
#
# 2. Instalar httpd
#   sudo dnf install httpd -y
#   sudo systemctl start httpd
#   sudo systemctl enable httpd
#
# 3. Cambiar DocumentRoot a /opt/mi_web
#   sudo mkdir -p /opt/mi_web
#   echo "<h1>Hola RHCSA</h1>" | sudo tee /opt/mi_web/index.html
#
#   # Editar configuracion de Apache
#   sudo vi /etc/httpd/conf/httpd.conf
#   # Cambiar DocumentRoot "/var/www/html" por DocumentRoot "/opt/mi_web"
#   # Cambiar <Directory "/var/www/html"> por <Directory "/opt/mi_web">
#
#   sudo systemctl restart httpd
#
# 4. Intentar acceder (VA A FALLAR)
#   curl http://localhost
#   # Error 403 Forbidden
#
# 5. Diagnosticar el problema
#   # Ver contexto actual
#   ls -Z /opt/mi_web/
#   # Veras algo como: unconfined_u:object_r:usr_t:s0
#   # Deberia ser: system_u:object_r:httpd_sys_content_t:s0
#
#   ls -Z /var/www/html/
#   # Este tiene el contexto correcto: httpd_sys_content_t
#
#   # Ver logs de SELinux
#   sudo ausearch -m AVC --raw | tail -5
#   # O si tenes setroubleshoot:
#   sudo sealert -a /var/log/audit/audit.log | head -30
#
# 6. Corregir el contexto
#   # Definir el contexto correcto para /opt/mi_web
#   sudo semanage fcontext -a -t httpd_sys_content_t "/opt/mi_web(/.*)?"
#
#   # Aplicar el contexto
#   sudo restorecon -Rv /opt/mi_web/
#
#   # Verificar
#   ls -Z /opt/mi_web/
#   # Ahora debe decir httpd_sys_content_t
#
# 7. Probar de nuevo
#   curl http://localhost
#   # Ahora debe funcionar!
#
# 8. Cambiar puerto de httpd a 8888
#   sudo vi /etc/httpd/conf/httpd.conf
#   # Cambiar Listen 80 por Listen 8888
#
#   sudo systemctl restart httpd
#   # VA A FALLAR porque SELinux no permite el puerto 8888
#
# 9. Agregar puerto en SELinux
#   # Ver puertos permitidos para http
#   sudo semanage port -l | grep http
#
#   # Agregar puerto 8888
#   sudo semanage port -a -t http_port_t -p tcp 8888
#
#   # Reiniciar
#   sudo systemctl restart httpd
#
#   # No olvidar el firewall!
#   sudo firewall-cmd --add-port=8888/tcp --permanent
#   sudo firewall-cmd --reload
#
# 10. Verificar todo
#   curl http://localhost:8888
#   # Debe funcionar!
#
# === BOOLEANS DE SELINUX ===
#
# Los booleans son switches on/off para politicas SELinux
#
# Ver todos los booleans de httpd
#   getsebool -a | grep httpd
#
# Ejemplo: permitir que httpd se conecte a la red
#   sudo setsebool -P httpd_can_network_connect on
#
# El flag -P hace el cambio persistente
#
# === RESUMEN DE COMANDOS ===
#
# getenforce / setenforce          → ver/cambiar modo
# sestatus                          → estado completo
# ls -Z                             → ver contexto de archivos
# ps -Z                             → ver contexto de procesos
# semanage fcontext -a -t TYPE PATH → definir contexto
# restorecon -Rv PATH               → aplicar contexto
# semanage port -a -t TYPE -p PROTO PORT → agregar puerto
# getsebool / setsebool             → manejar booleans
# ausearch -m AVC                   → buscar denegaciones
# sealert                           → diagnostico detallado
#
# IMPORTANTE PARA EL EXAMEN:
# - SELinux SIEMPRE debe estar en enforcing
# - Si algo no funciona y los permisos Unix estan bien, es SELinux
# - Siempre verificar con ls -Z y ausearch
# ============================================================

echo "Este archivo es una guia de referencia para SELinux."
echo "PRACTICA MUCHO - SELinux es tema seguro del RHCSA."
