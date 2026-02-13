/*
 * ============================================================
 * FASE 4 - Ejercicio 4.2: Lista enlazada simple
 * ============================================================
 * OBJETIVO: Dominar punteros, malloc/free y estructuras de datos
 *
 * REQUISITOS:
 * 1. Struct nodo con: int dato, struct nodo *siguiente
 * 2. Funciones: insertar_inicio, insertar_final, buscar,
 *    eliminar, imprimir, liberar
 * 3. Sin memory leaks (verificar con valgrind)
 * 4. Menu interactivo
 *
 * COMPILAR: gcc -Wall -Wextra -g -o lista ejercicio_4.2_lista_enlazada.c
 * VALGRIND: valgrind --leak-check=full ./lista
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>

/* Estructura del nodo */
typedef struct nodo {
    int dato;
    struct nodo *siguiente;
} Nodo;

/* Insertar al inicio de la lista */
Nodo *insertar_inicio(Nodo *cabeza, int valor) {
    Nodo *nuevo = malloc(sizeof(Nodo));
    if (!nuevo) {
        perror("malloc");
        exit(1);
    }
    nuevo->dato = valor;
    nuevo->siguiente = cabeza;
    return nuevo;
}

/* Insertar al final de la lista */
Nodo *insertar_final(Nodo *cabeza, int valor) {
    Nodo *nuevo = malloc(sizeof(Nodo));
    if (!nuevo) {
        perror("malloc");
        exit(1);
    }
    nuevo->dato = valor;
    nuevo->siguiente = NULL;

    if (!cabeza)
        return nuevo;

    Nodo *actual = cabeza;
    while (actual->siguiente)
        actual = actual->siguiente;

    actual->siguiente = nuevo;
    return cabeza;
}

/* Buscar un valor en la lista */
Nodo *buscar(Nodo *cabeza, int valor) {
    Nodo *actual = cabeza;
    int pos = 0;

    while (actual) {
        if (actual->dato == valor) {
            printf("Encontrado: %d en posicion %d\n", valor, pos);
            return actual;
        }
        actual = actual->siguiente;
        pos++;
    }

    printf("No encontrado: %d\n", valor);
    return NULL;
}

/* Eliminar primera ocurrencia de un valor */
Nodo *eliminar(Nodo *cabeza, int valor) {
    if (!cabeza) {
        printf("Lista vacia\n");
        return NULL;
    }

    /* Si es el primer nodo */
    if (cabeza->dato == valor) {
        Nodo *temp = cabeza->siguiente;
        free(cabeza);
        printf("Eliminado: %d\n", valor);
        return temp;
    }

    /* Buscar en el resto */
    Nodo *actual = cabeza;
    while (actual->siguiente) {
        if (actual->siguiente->dato == valor) {
            Nodo *temp = actual->siguiente;
            actual->siguiente = temp->siguiente;
            free(temp);
            printf("Eliminado: %d\n", valor);
            return cabeza;
        }
        actual = actual->siguiente;
    }

    printf("No encontrado: %d\n", valor);
    return cabeza;
}

/* Imprimir toda la lista */
void imprimir_lista(Nodo *cabeza) {
    if (!cabeza) {
        printf("Lista vacia\n");
        return;
    }

    printf("Lista: ");
    Nodo *actual = cabeza;
    int count = 0;

    while (actual) {
        printf("[%d]", actual->dato);
        if (actual->siguiente)
            printf(" -> ");
        actual = actual->siguiente;
        count++;
    }
    printf(" (total: %d)\n", count);
}

/* Liberar toda la lista */
void liberar_lista(Nodo *cabeza) {
    Nodo *actual = cabeza;
    while (actual) {
        Nodo *temp = actual;
        actual = actual->siguiente;
        free(temp);
    }
}

/* Menu interactivo */
void menu(void) {
    printf("\n=== LISTA ENLAZADA ===\n");
    printf("1) Insertar al inicio\n");
    printf("2) Insertar al final\n");
    printf("3) Buscar\n");
    printf("4) Eliminar\n");
    printf("5) Imprimir lista\n");
    printf("6) Salir\n");
    printf("Opcion: ");
}

int main(void) {
    Nodo *lista = NULL;
    int opcion, valor;

    while (1) {
        menu();

        if (scanf("%d", &opcion) != 1) {
            /* Limpiar buffer si no es numero */
            while (getchar() != '\n');
            printf("Ingresa un numero\n");
            continue;
        }

        switch (opcion) {
        case 1:
            printf("Valor: ");
            scanf("%d", &valor);
            lista = insertar_inicio(lista, valor);
            imprimir_lista(lista);
            break;
        case 2:
            printf("Valor: ");
            scanf("%d", &valor);
            lista = insertar_final(lista, valor);
            imprimir_lista(lista);
            break;
        case 3:
            printf("Buscar: ");
            scanf("%d", &valor);
            buscar(lista, valor);
            break;
        case 4:
            printf("Eliminar: ");
            scanf("%d", &valor);
            lista = eliminar(lista, valor);
            imprimir_lista(lista);
            break;
        case 5:
            imprimir_lista(lista);
            break;
        case 6:
            liberar_lista(lista);
            printf("Lista liberada. Hasta luego!\n");
            return 0;
        default:
            printf("Opcion invalida\n");
        }
    }

    return 0;
}
