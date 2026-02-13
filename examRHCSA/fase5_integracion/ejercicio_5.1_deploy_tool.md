# Ejercicio 5.1: Deploy Tool - Proyecto Integrador

Proyecto completo que combina **C + Bash + RHCSA** en un sistema de deploy.

## Arquitectura

```
deploy_tool/
├── src/
│   ├── deploy_daemon.c    ← Daemon TCP en C
│   └── Makefile
├── scripts/
│   ├── deploy_client.sh   ← Cliente en Bash
│   ├── setup.sh           ← Instalador
│   └── monitor.sh         ← Monitor del daemon
└── systemd/
    └── deploy.service     ← Unit de systemd
```

## Parte 1: Daemon en C (deploy_daemon.c)

Crear un daemon que:
- Escuche en un puerto TCP (ej: 5000)
- Acepte comandos por texto:
  - `status` → devuelve uptime, disco, memoria
  - `deploy` → copia /opt/staging/* a /opt/production/
  - `rollback` → restaura backup anterior
  - `quit` → cierra la conexion
- Antes de cada deploy, hace backup de /opt/production/
- Loguea todo en /var/log/deploy_daemon.log
- Maneja multiples clientes con fork()
- Se cierra limpiamente con SIGTERM

## Parte 2: Scripts Bash

### deploy_client.sh
```bash
# Se conecta al daemon y envia comandos
# Uso: ./deploy_client.sh <host> <puerto> <comando>
# Ejemplo: ./deploy_client.sh localhost 5000 status
```

### setup.sh
```bash
# Instalador completo:
# 1. Compila el daemon con make
# 2. Copia binario a /usr/local/bin/
# 3. Crea usuario "deploy" para el servicio
# 4. Crea directorios /opt/staging y /opt/production
# 5. Instala el archivo .service en systemd
# 6. Configura firewall para el puerto
# 7. Configura SELinux
# 8. Habilita e inicia el servicio
```

### monitor.sh
```bash
# Monitorea que el daemon este corriendo
# Si se cae, lo reinicia
# Registra eventos en log
# Se ejecuta via cron cada minuto
```

## Parte 3: Configuracion RHCSA

- Crear usuario "deploy" sin login shell
- SELinux: contexto correcto para el puerto custom
- Firewall: abrir el puerto del daemon
- LVM: crear volumen logico para /opt/production
- Cron: ejecutar monitor.sh cada minuto
- Podman: containerizar todo el sistema

## Como empezar

1. Empieza por el daemon en C (la parte mas compleja)
2. Proba con `nc localhost 5000`
3. Despues crea los scripts Bash
4. Al final configura todo el entorno RHCSA
5. Subi todo a GitHub

## Bonus

- Agregar autenticacion basica (password)
- Agregar encriptacion con TLS
- Dashboard web simple que consulte al daemon
- CI/CD con GitHub Actions que compile y teste
