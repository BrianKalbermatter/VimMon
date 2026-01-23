// Ejercicio Area Trapecio

#include <stdio.h>

int main(){
    float base_Mayor;
    float base_Menor;
    float altura;
    float area;
    FILE *archivo;
    
    printf("Escribe la base mayor\n");
    scanf("%f", &base_Mayor); // Primero %f que es lo que va a tomar y asignarle a la memoria interna. Esta se almacenara por el tiempo de ejecucion y luego se elimina
    // Si la quiero guardar luego permanentemente como hago?
    
    printf("Escribe la base menor\n");
    scanf("%f", &base_Menor); // Lo mismo aca

    printf("Escribe la Altura\n");
    scanf("%f", &altura);

    area = ((base_Mayor + base_Menor) * altura) / 2;
    printf("\nEl area es: %2.f", area);
    
    // Leer valor anterior(si existe)
    archivo = fopen("valor_area.txt", "r");
    if (archivo != NULL){
        fscanf(archivo,"%f", &area);
        fclose(archivo);
        printf("Valor anterior: %2.f\n", area);
    }

    archivo = fopen("valor_area.txt", "w");
    fprintf(archivo, "%2.f", area);
    fclose(archivo);
    
    //      Funciones                |       Que Hace
    // fopen("archivo", "r");        |  Abre para leer
    // fopen("archivo", "w");        |  Abre para escribir
    // fscanf(archivo, "%f", &var);  |  Lee del Archivo
    // fprintf(archivo, "%f", var);  |  Escribir al archivo
    // fclose(archivo);              |  Cierra el archivo
    // Cada vez que ejecute el programa, leera el valor anterior de datos.txt y lo acumulara.
    return 0;
}

