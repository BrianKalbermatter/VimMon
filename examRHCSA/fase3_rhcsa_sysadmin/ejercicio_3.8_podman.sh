#!/bin/bash
# ============================================================
# FASE 3 - Ejercicio 3.8: Containers con Podman
# ============================================================
# OBJETIVO: Manejar containers con Podman (no Docker) para RHCSA
#
# === BASICO ===
#
# 1. Instalar podman
#   sudo dnf install podman -y
#
# 2. Buscar imagen
#   podman search nginx
#
# 3. Descargar imagen
#   podman pull docker.io/library/nginx
#
# 4. Listar imagenes
#   podman images
#
# 5. Ejecutar container
#   podman run -d \
#     --name web_server \
#     -p 8080:80 \
#     -v /opt/mi_web:/usr/share/nginx/html:Z \
#     nginx
#
#   # El flag :Z es para SELinux (ajusta contexto automaticamente)
#
# 6. Verificar
#   podman ps
#   curl http://localhost:8080
#
# 7. Ver logs del container
#   podman logs web_server
#
# 8. Entrar al container
#   podman exec -it web_server /bin/bash
#
# 9. Detener y eliminar
#   podman stop web_server
#   podman rm web_server
#
# === CREAR IMAGEN PROPIA ===
#
# 10. Crear directorio del proyecto
#   mkdir -p ~/mi_container
#   cd ~/mi_container
#
# 11. Crear el script
#   cat << 'PYEOF' > hello.py
#   #!/usr/bin/env python3
#   import datetime
#   print(f"Hola desde container! Fecha: {datetime.datetime.now()}")
#   PYEOF
#
# 12. Crear Containerfile
#   cat << 'CEOF' > Containerfile
#   FROM registry.access.redhat.com/ubi9/ubi-minimal
#   RUN microdnf install python3 -y && microdnf clean all
#   COPY hello.py /app/hello.py
#   RUN chmod +x /app/hello.py
#   CMD ["python3", "/app/hello.py"]
#   CEOF
#
# 13. Construir imagen
#   podman build -t mi_app:v1 .
#
# 14. Ejecutar
#   podman run --name mi_app mi_app:v1
#
# 15. Listar todas las imagenes
#   podman images
#
# === CONTAINER COMO SERVICIO SYSTEMD ===
#
# 16. Crear el container primero
#   podman run -d --name web_service -p 8080:80 nginx
#
# 17. Generar archivo de servicio systemd
#   mkdir -p ~/.config/systemd/user/
#   podman generate systemd --name web_service --files --new
#   mv container-web_service.service ~/.config/systemd/user/
#
# 18. Detener y eliminar el container original
#   podman stop web_service
#   podman rm web_service
#
# 19. Habilitar el servicio (como usuario, no root)
#   systemctl --user daemon-reload
#   systemctl --user enable container-web_service.service
#   systemctl --user start container-web_service.service
#
# 20. Verificar
#   systemctl --user status container-web_service.service
#   podman ps
#   curl http://localhost:8080
#
# 21. Para que funcione sin estar logueado
#   loginctl enable-linger $USER
#
# === COMANDOS UTILES ===
#
# podman ps -a              → ver todos los containers (incluso detenidos)
# podman inspect <nombre>   → detalles del container
# podman port <nombre>      → ver mapeo de puertos
# podman top <nombre>       → procesos dentro del container
# podman stats              → uso de recursos
# podman system prune       → limpiar todo lo no usado
#
# IMPORTANTE PARA EL EXAMEN:
# - RHCSA usa Podman, NO Docker
# - Saber crear containers con -p (puertos) y -v (volumenes)
# - Saber crear un Containerfile basico
# - Saber generar servicio systemd desde un container
# - El flag :Z en volumenes es para SELinux
# ============================================================

echo "Este archivo es una guia de referencia para Podman."
echo "Practica creando y manejando containers en tu VM RHEL 9."
