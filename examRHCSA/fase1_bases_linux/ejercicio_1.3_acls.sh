#!/bin/bash
# ============================================================
# FASE 1 - Ejercicio 1.3: ACLs (Access Control Lists)
# ============================================================
# OBJETIVO: Manejar permisos avanzados con ACLs
#
# INSTRUCCIONES (requiere haber hecho ejercicio 1.2):
# 1. Sobre /opt/proyecto:
#    - Dale permiso de lectura al usuario auditor con ACL
#    - Dale permiso de lectura+escritura al grupo desarrollo con ACL
# 2. Crea un archivo dentro como dev1
# 3. Verifica que auditor puede leerlo pero no modificarlo
# 4. Verifica que dev2 puede leerlo y modificarlo
# 5. Lista las ACLs con getfacl
# 6. Elimina la ACL de auditor y verifica que ya no puede leer
#
# COMANDOS CLAVE: setfacl, getfacl
# ============================================================

echo "=== Ejercicio 1.3 - ACLs ==="
echo ""
echo "PASOS:"
echo ""
echo "# Dar permiso de lectura a auditor"
echo "sudo setfacl -m u:auditor:r-x /opt/proyecto"
echo ""
echo "# Dar lectura+escritura al grupo desarrollo"
echo "sudo setfacl -m g:desarrollo:rwx /opt/proyecto"
echo ""
echo "# Crear archivo como dev1"
echo "sudo -u dev1 touch /opt/proyecto/archivo_test.txt"
echo "sudo -u dev1 echo 'contenido de prueba' > /opt/proyecto/archivo_test.txt"
echo ""
echo "# Verificar como auditor (solo lectura)"
echo "sudo -u auditor cat /opt/proyecto/archivo_test.txt    # OK"
echo "sudo -u auditor echo 'test' >> /opt/proyecto/archivo_test.txt  # FALLA"
echo ""
echo "# Verificar como dev2 (lectura+escritura)"
echo "sudo -u dev2 cat /opt/proyecto/archivo_test.txt    # OK"
echo "sudo -u dev2 echo 'mas datos' >> /opt/proyecto/archivo_test.txt  # OK"
echo ""
echo "# Ver ACLs"
echo "getfacl /opt/proyecto"
echo ""
echo "# Eliminar ACL de auditor"
echo "sudo setfacl -x u:auditor /opt/proyecto"
echo "getfacl /opt/proyecto"
