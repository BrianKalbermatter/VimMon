#!/bin/bash
# ============================================================
# FASE 1 - Ejercicio 1.2: Permisos de archivos
# ============================================================
# OBJETIVO: Dominar permisos, usuarios, grupos y bits especiales
#
# INSTRUCCIONES:
# 1. Crea 3 usuarios: dev1, dev2, auditor
# 2. Crea 2 grupos: desarrollo, auditoria
# 3. Asigna dev1 y dev2 al grupo desarrollo
# 4. Asigna auditor al grupo auditoria
# 5. Crea el directorio /opt/proyecto
# 6. Configura permisos para que:
#    - dev1 y dev2 puedan leer, escribir y ejecutar
#    - auditor solo pueda leer
#    - el resto no tenga acceso
# 7. Pon sticky bit en /opt/proyecto
# 8. Verifica los permisos con ls -la y getfacl
#
# COMANDOS CLAVE: useradd, groupadd, usermod, chmod, chown, chgrp
# ============================================================

echo "=== Ejercicio 1.2 - Permisos ==="
echo ""
echo "PASOS:"
echo ""
echo "# Crear usuarios y grupos"
echo "sudo useradd dev1"
echo "sudo useradd dev2"
echo "sudo useradd auditor"
echo "sudo groupadd desarrollo"
echo "sudo groupadd auditoria"
echo ""
echo "# Asignar usuarios a grupos"
echo "sudo usermod -aG desarrollo dev1"
echo "sudo usermod -aG desarrollo dev2"
echo "sudo usermod -aG auditoria auditor"
echo ""
echo "# Crear directorio y configurar permisos"
echo "sudo mkdir -p /opt/proyecto"
echo "sudo chown root:desarrollo /opt/proyecto"
echo "sudo chmod 770 /opt/proyecto"
echo ""
echo "# Sticky bit"
echo "sudo chmod +t /opt/proyecto"
echo ""
echo "# Verificar"
echo "ls -la /opt/proyecto"
echo "getfacl /opt/proyecto"
echo ""
echo "NOTA: Para que auditor solo lea, usa ACLs (ejercicio 1.3)"
