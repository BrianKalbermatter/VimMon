#!/bin/bash
# ============================================================
# FASE 2 - Ejercicio 2.6: Menu interactivo de administracion
# ============================================================
# OBJETIVO: Practicar menus, submenus, case, funciones
#
# REQUISITOS:
# Menu con opciones de administracion del sistema
# Cada opcion ejecuta la tarea y vuelve al menu
# ============================================================

# Colores
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m'

info_sistema() {
    echo ""
    echo -e "${CYAN}=== INFORMACION DEL SISTEMA ===${NC}"
    echo "Hostname : $(hostname)"
    echo "Kernel   : $(uname -r)"
    echo "Uptime   : $(uptime -p 2>/dev/null || uptime)"
    echo "IP       : $(hostname -I 2>/dev/null | awk '{print $1}')"
    echo "Fecha    : $(date)"
    echo ""
}

gestionar_usuarios() {
    while true; do
        echo ""
        echo -e "${CYAN}=== GESTION DE USUARIOS ===${NC}"
        echo "  a) Listar usuarios"
        echo "  b) Crear usuario"
        echo "  c) Eliminar usuario"
        echo "  d) Volver al menu principal"
        echo ""
        read -p "Opcion: " sub_opt

        case $sub_opt in
            a)
                echo ""
                echo "Usuarios del sistema:"
                awk -F: '$3 >= 1000 {print "  " $1 " (UID:" $3 ")"}' /etc/passwd
                ;;
            b)
                read -p "Nombre del nuevo usuario: " new_user
                if [ -n "$new_user" ]; then
                    sudo useradd "$new_user" 2>/dev/null && \
                        echo -e "${GREEN}Usuario '$new_user' creado${NC}" || \
                        echo "Error al crear usuario"
                fi
                ;;
            c)
                read -p "Nombre del usuario a eliminar: " del_user
                if [ -n "$del_user" ]; then
                    read -p "Seguro? Esto eliminara el usuario y su home (s/n): " confirm
                    if [ "$confirm" = "s" ]; then
                        sudo userdel -r "$del_user" 2>/dev/null && \
                            echo -e "${GREEN}Usuario '$del_user' eliminado${NC}" || \
                            echo "Error al eliminar usuario"
                    fi
                fi
                ;;
            d) return ;;
            *) echo "Opcion invalida" ;;
        esac
    done
}

uso_disco() {
    echo ""
    echo -e "${CYAN}=== USO DE DISCO ===${NC}"
    df -h | awk 'NR==1 || /^\/dev/'
    echo ""
}

procesos_activos() {
    echo ""
    echo -e "${CYAN}=== TOP 10 PROCESOS (por CPU) ===${NC}"
    ps aux --sort=-%cpu | head -11
    echo ""
}

gestionar_servicios() {
    read -p "Nombre del servicio: " servicio
    if [ -z "$servicio" ]; then
        echo "Nombre vacio"
        return
    fi

    echo ""
    echo "  1) start"
    echo "  2) stop"
    echo "  3) restart"
    echo "  4) status"
    read -p "Accion: " accion

    case $accion in
        1) sudo systemctl start "$servicio" && echo "Iniciado" ;;
        2) sudo systemctl stop "$servicio" && echo "Detenido" ;;
        3) sudo systemctl restart "$servicio" && echo "Reiniciado" ;;
        4) systemctl status "$servicio" ;;
        *) echo "Accion invalida" ;;
    esac
}

conexiones_red() {
    echo ""
    echo -e "${CYAN}=== CONEXIONES DE RED ===${NC}"
    echo ""
    echo "--- Interfaces ---"
    ip -br addr
    echo ""
    echo "--- Puertos escuchando ---"
    ss -tlnp 2>/dev/null || netstat -tlnp 2>/dev/null
    echo ""
}

# Menu principal
while true; do
    echo ""
    echo -e "${GREEN}=========================================${NC}"
    echo -e "${GREEN}         ADMIN MENU${NC}"
    echo -e "${GREEN}=========================================${NC}"
    echo "  1) Ver informacion del sistema"
    echo "  2) Gestionar usuarios"
    echo "  3) Ver uso de disco"
    echo "  4) Ver procesos activos"
    echo "  5) Gestionar servicios"
    echo "  6) Ver conexiones de red"
    echo "  7) Salir"
    echo -e "${GREEN}=========================================${NC}"
    echo ""
    read -p "Selecciona una opcion [1-7]: " opcion

    case $opcion in
        1) info_sistema ;;
        2) gestionar_usuarios ;;
        3) uso_disco ;;
        4) procesos_activos ;;
        5) gestionar_servicios ;;
        6) conexiones_red ;;
        7)
            echo -e "${GREEN}Hasta luego!${NC}"
            exit 0
            ;;
        *)
            echo -e "${YELLOW}Opcion invalida. Intenta de nuevo.${NC}"
            ;;
    esac
done
