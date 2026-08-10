# Red Hat tiene NDA estricto - Los que toman el examen firman acuerdos de no divulgación, así que los ejercicios exactos del examen real nunca se publican públicamente

Los ejercicios reales son confidenciales - No existen "exámenes pasados" oficiales disponibles

Lo que hice fue:

Tomar los objetivos del syllabus oficial de RHCSA para RHEL 9
Crear ejercicios prácticos que cubren esos objetivos
Usar el formato y estilo típico de tareas hands-on que se esperan en el examen

Los objetivos oficiales incluyen cosas como:

Understand and use essential tools
Operate running systems
Configure local storage
Create and configure file systems
Deploy, configure, and maintain systems
Manage users and groups
Manage security

Estos ejercicios que te armé cubren esos temas de forma práctica. Son representativos del tipo de tareas que verías, pero obviamente no son las preguntas exactas del examen.
Si quieres algo más "oficial", te recomendaría:

El libro de Sander van Vugt o Asghar Ghori - tienen exámenes de práctica creados por instructores certificados
Los labs de Red Hat Learning Subscription (de pago)

# En el examen RHCSA real tienes entre 15-20 tareas para completar en 2.5 a 3 horas (depende de la versión del examen, pero generalmente son 2.5 horas).

Desglose típico:

Tiempo total: 2.5 - 3 horas (150-180 minutos)
Número de tareas: 15-20 ejercicios
Tiempo por ejercicio: ~7-12 minutos en promedio

Pero ojo: No todos los ejercicios valen lo mismo ni toman el mismo tiempo:

Tareas simples (crear usuario, cambiar hostname): 2-3 minutos
Tareas medias (configurar LVM, SELinux): 5-10 minutos
Tareas complejas (troubleshooting de boot, containers con systemd): 15-20 minutos

En la práctica:

Los ejercicios que te di arriba (10 ejercicios) serían para practicar en 3-4 horas si los haces con calma
Para simular el examen real deberías intentar 12-15 ejercicios en 2.5 horas

Tips importantes:

No te atasques - Si un ejercicio te está tomando más de 15 minutos, márcalo y sigue
Valida todo - El examen se califica automáticamente reiniciando la VM
Prioriza - Algunos ejercicios valen más puntos, haz primero los que dominas
Necesitas ~70% para pasar - No tienes que completar todo perfecto


# Dominar las PAGES Como lo haria un PRO

## 1. Estructura de las Man Pages

Las man pages están divididas en secciones numeradas:

```bash
man <sección> <comando>
```

Secciones importantes:

1 - Comandos de usuario (ls, cp, mv)
5 - Formatos de archivos y convenciones (/etc/fstab, /etc/passwd)
8 - Comandos de administración (useradd, systemctl)

Ejemplos prácticos:

```bash
man passwd          # Comando passwd (sección 1)
man 5 passwd        # Formato del archivo /etc/passwd (sección 5)

man crontab         # Comando crontab
man 5 crontab       # Formato de archivo crontab
```

---

## 2. Navegación Eficiente Dentro de Man Pages

**Teclas esenciales:**

```
/palabra       - Buscar "palabra" hacia adelante
?palabra       - Buscar "palabra" hacia atrás
n              - Siguiente resultado de búsqueda
N              - Resultado anterior
g              - Ir al inicio
G              - Ir al final
q              - Salir

Espacio        - Página abajo
b              - Página arriba
```

Ejemplo práctico:

```bash
man useradd
# Presiona: /example
# Te lleva directo a la sección de EXAMPLES
```

## 3. Búsqueda Rápida de Comandos

apropos - Tu mejor amigo cuando no recuerdas el comando

```bash
# ¿Cómo se cambia contraseña?
apropos password
# Muestra: passwd, chpasswd, usermod...

# ¿Cómo gestiono volúmenes LVM?
apropos lvm
apropos "logical volume"

# ¿Cómo configuro red?
apropos network
apropos nmcli

# ¿Cómo gestiono servicios?
apropos systemd
apropos service
```

Tip pro: Si apropos no encuentra nada, actualiza la base de datos:

```bash
sudo mandb
```

## 4. Man Pages CRÍTICAS para RHCSA

Gestión de Usuarios

```bash
man useradd        # Crear usuarios
man usermod        # Modificar usuarios
man groupadd       # Crear grupos
man passwd         # Cambiar contraseñas
man chage          # Password aging
man 5 sudoers      # Configurar sudo

# Búsquedas útiles dentro:
# /example  /expire  /group  /shell
```

Permisos y ACLs

```bash
man chmod          # Cambiar permisos
man chown          # Cambiar dueño
man setfacl        # Configurar ACLs
man getfacl        # Ver ACLs

# En setfacl busca: /example
```

LVM

```bash
man pvcreate       # Physical volumes
man vgcreate       # Volume groups
man lvcreate       # Logical volumes
man lvextend       # Extender LV
man xfs_growfs     # Redimensionar XFS
man resize2fs      # Redimensionar ext4

# Busca siempre: /example /resize
```

Red (NetworkManager)

```bash
man nmcli          # MUY IMPORTANTE
man nmcli-examples # ¡Tiene ejemplos específicos!
man 5 nmcli        # Ejemplos de configuración

# Dentro de man nmcli:
# /example
# /static
# /connection
```

Systemd

```bash
man systemctl      # Gestionar servicios
man systemd.service # Crear service units
man systemd.timer  # Crear timers
man journalctl     # Ver logs

# Busca: /example /enable /start
```

SELinux

```bash
man semanage       # Gestión de políticas
man restorecon     # Restaurar contextos
man chcon          # Cambiar contextos
man getsebool      # Ver booleanos
man setsebool      # Configurar booleanos
man semanage-fcontext  # Contextos de archivos

# Busca: /example /httpd /port
```

Firewall

```bash
man firewall-cmd   # Firewall configuration
man firewalld      # Daemon info

# Busca: /add-service /add-port /permanent
```

Podman/Containers

```bash
man podman         # General
man podman-run     # Ejecutar containers
man podman-generate-systemd  # Generar units

# Busca: /example /publish /volume
```

Cron y Scheduling

```bash
man 5 crontab      # ¡Formato de crontab!
man crontab        # Comando crontab
man at             # Jobs únicos

# En man 5 crontab tiene la sintaxis:
# MIN HOUR DOM MON DOW
```

Archivos de Configuración

```bash
man 5 fstab        # Montaje permanente
man 5 passwd       # Formato /etc/passwd
man 5 group        # Formato /etc/group
man 5 shadow       # Formato /etc/shadow
```

## 5. Técnicas Ninja para el Examen

✅ Truco 1: Ir directo a EXAMPLES

La mayoría de man pages tienen sección de ejemplos al final:

```bash
man useradd
# Presiona: /EXAMPLE
# O: /example (case insensitive)
```

Esto te ahorra leer páginas enteras. Los ejemplos son oro.

✅ Truco 2: Usar --help primero

Muchas veces --help es más rápido que man:

```bash
nmcli connection add --help
lvextend --help
firewall-cmd --help
semanage fcontext --help
```

✅ Truco 3: Combinar apropos + man

```bash
# No sé cómo extender un LV
apropos extend | grep lv
# Te muestra: lvextend

man lvextend
# Busca: /example
```

✅ Truco 4: Tab completion es tu amigo

```bash
systemctl <TAB><TAB>        # Muestra subcomandos
nmcli connection <TAB>      # Muestra opciones
firewall-cmd --<TAB><TAB>   # Muestra flags
```

✅ Truco 5: Documentación local adicional

```bash
# Documentación instalada de paquetes
rpm -qd httpd
rpm -qd selinux-policy

# Ver docs en /usr/share/doc
ls /usr/share/doc/

# Ejemplos de systemd units
ls /usr/lib/systemd/system/
```

## 6. Práctica Efectiva

Ejercicio de Entrenamiento:
Resuelve estos SIN buscar en internet, SOLO con man pages:

¿Cómo crear un usuario con UID 2500 y shell /bin/bash?

```bash
# Pista: man useradd, busca /example o /uid
```

¿Cómo configurar IP estática con nmcli?

```bash
# Pista: man nmcli-examples
```

¿Cómo extender un LV de 1G a 2G?

```bash
# Pista: man lvextend, busca /example
```

¿Cómo hacer que httpd use puerto 8080 en SELinux?

```bash
# Pista: man semanage-port, busca /add o /http
```

¿Sintaxis de crontab para ejecutar algo cada lunes a las 3 AM?

```bash
# Pista: man 5 crontab (sección 5!)
```

---

## 7. Cheat Sheet Mental para el Examen

**Cuando no sepas algo:**

```
1. ¿Es un comando? → man <comando>
2. ¿Es un archivo de config? → man 5 <archivo>
3. ¿No recuerdo el comando? → apropos <concepto>
4. ¿Necesito sintaxis rápida? → <comando> --help
5. ¿Necesito ejemplos? → Busca /example en man page
```

## 8. Lista de Man Pages para Memorizar

Imprímete esta lista y practícala:

```bash
# Must-know
man useradd
man usermod
man passwd
man chage
man nmcli
man nmcli-examples
man systemctl
man journalctl
man firewall-cmd
man pvcreate
man vgcreate
man lvcreate
man lvextend
man xfs_growfs
man setfacl
man chmod
man semanage
man restorecon
man podman-run
man 5 crontab
man 5 fstab
man systemd.service
man systemd.timer
```

## 9. Simulacro Final

Cronométrate 5 minutos para encontrar esta info SOLO con man pages:

¿Cómo crear un grupo con GID específico?
¿Cómo montar permanentemente un filesystem XFS?
¿Cómo permitir httpd escuchar en puerto no estándar (SELinux)?
¿Cómo crear un container que arranque al boot?
¿Cómo configurar password expiration de 90 días?

Si tardas más de 5 minutos en encontrar cada respuesta, necesitas más práctica con man pages.

## Bonus: Comando Secreto

```bash
# Exportar una man page a texto para búsqueda offline
man useradd | col -b > /tmp/useradd.txt

# Luego puedes hacer:
grep -i "example" /tmp/useradd.txt
```
