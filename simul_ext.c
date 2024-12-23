#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "cabeceras.h"

#define LONGITUD_COMANDO 100
#define MAX_FICHEROS 64
#define MAX_BLOQUES_DATOS 1024
#define MAX_BLOQUES_PARTICION 2048
#define SIZE_BLOQUE 1024

//Estas constantes hacen el código más legible y flexible al manejar valores repetitivos como límites de inodos y bloques
#define MAX_INODOS 64
#define MAX_NUMS_BLOQUES_INODO 12
#define NULL_INODO 0
#define NULL_BLOQUE 0


void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps);
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2);
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup);
int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombre);
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos);
int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo);
int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre);
int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre, FILE *fich);
int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino, FILE *fich);
void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich);
void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich);
void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich);
void GrabarDatos(EXT_DATOS *memdatos, FILE *fich);

int main() {
    char comando[LONGITUD_COMANDO];
    char orden[LONGITUD_COMANDO];
    char argumento1[LONGITUD_COMANDO];
    char argumento2[LONGITUD_COMANDO];

    int i, j;
    unsigned long int m;
    EXT_SIMPLE_SUPERBLOCK ext_superblock;
    EXT_BYTE_MAPS ext_bytemaps;
    EXT_BLQ_INODOS ext_blq_inodos;
    EXT_ENTRADA_DIR directorio[MAX_FICHEROS];
    EXT_DATOS memdatos[MAX_BLOQUES_DATOS];
    EXT_DATOS datosfich[MAX_BLOQUES_PARTICION];
    int entradadir;
    int grabardatos;
    FILE *fent;

    // Lectura del fichero completo de una sola vez
    fent = fopen("particion.bin", "r+b");
    fread(&datosfich, SIZE_BLOQUE, MAX_BLOQUES_PARTICION, fent);

    memcpy(&ext_superblock, (EXT_SIMPLE_SUPERBLOCK *)&datosfich[0], SIZE_BLOQUE);
    memcpy(&directorio, (EXT_ENTRADA_DIR *)&datosfich[3], SIZE_BLOQUE);
    memcpy(&ext_bytemaps, (EXT_BLQ_INODOS *)&datosfich[1], SIZE_BLOQUE);
    memcpy(&ext_blq_inodos, (EXT_BLQ_INODOS *)&datosfich[2], SIZE_BLOQUE);
    memcpy(&memdatos, (EXT_DATOS *)&datosfich[4], MAX_BLOQUES_DATOS * SIZE_BLOQUE);

    // Buce de tratamiento de comandos
    for (;;) {
        do {
            printf(">> ");
            fflush(stdin);
            fgets(comando, LONGITUD_COMANDO, stdin);
        } while (ComprobarComando(comando, orden, argumento1, argumento2) != 0);

         if (strcmp(orden, "dir") == 0) {
            Directorio(directorio, ext_blq_inodos);
            continue;
        }

        //Se añadió la capacidad de renombrar archivos desde el menú principal al incorporar el comando rename
        if (strcmp(orden, "rename") == 0) {
            Renombrar(directorio, ext_blq_inodos, argumento1, argumento2);
            continue;
        }
        // Escritura de metadatos en comandos rename, remove, copy
        //Eliminación del uso de punteros adicionales para GrabarByteMaps, GrabarSuperBloque y GrabarDatos, simplificando las llamadas y evitando confusión con los parámetros
        Grabarinodosydirectorio(directorio, ext_blq_inodos, fent);
        GrabarByteMaps(ext_bytemaps, fent);
        GrabarSuperBloque(ext_superblock, fent);
        if (grabardatos)
            GrabarDatos(memdatos, fent);
        grabardatos = 0;

        // Si el comando es salir se habrán escrito todos los metadatos
        // faltan los datos y cerrar
        if (strcmp(orden, "salir") == 0) {
            GrabarDatos(&memdatos, fent);
            fclose(fent);
            return 0;
        }
    }
}


void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps) {
    int i;
    printf("Inodos : ");
    for (i = 0; i < MAX_INODOS; i++) {
        printf("%d ", ext_bytemaps->bmap_inodos[i]);
    }
    printf("\nBloques : ");
    for (i = 0; i < MAX_BLOQUES_DATOS; i++) {
        printf("%d ", ext_bytemaps->bmap_bloques[i]);
    }
    printf("\n");
}

int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2) {
//Ahora se analiza el comando recibido en strcomando, dividiéndolo en tres partes: orden, argumento1, y argumento2. 
//Esto permite interpretar y validar comandos con uno o dos argumentos.
    int i = 0, j = 0;
    while (isspace(strcomando[i]) && (i < LONGITUD_COMANDO)) i++;
    while (!isspace(strcomando[i]) && (i < LONGITUD_COMANDO)) {
        orden[j] = strcomando[i];
        i++;
        j++;
    }
    orden[j] = '\0';
    while (isspace(strcomando[i]) && (i < LONGITUD_COMANDO)) i++;
    j = 0;
    while (!isspace(strcomando[i]) && (i < LONGITUD_COMANDO)) {
        argumento1[j] = strcomando[i];
        i++;
        j++;
    }
    argumento1[j] = '\0';
    while (isspace(strcomando[i]) && (i < LONGITUD_COMANDO)) i++;
    j = 0;
    while (!isspace(strcomando[i]) && (i < LONGITUD_COMANDO)) {
        argumento2[j] = strcomando[i];
        i++;
        j++;
    }
    argumento2[j] = '\0';
    return 0;
    return 0;
}

void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup) {
    // Se implementa la función para imprimir información del superbloque, como el tamaño del bloque, cantidad de inodos y bloques totales y libres. 
    // Esto ayuda a inspeccionar el estado de la partición.
    printf("Bloque %d\n", psup->s_block_size);
    printf("inodos particion = %d\n", psup->s_inodes_count);
    printf("inodos libres = %d\n", psup->s_free_inodes_count);
    printf("bloques particion = %d\n", psup->s_blocks_count);
    printf("bloques libres = %d\n", psup->s_free_blocks_count);
}

int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombre) {
    // Implementación pendiente
    return 0;
}

void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos) {
    int i, j;
    for (i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo != NULL_INODO) {
            printf("Nombre: %s, ", directorio[i].dir_nfich);
            printf("Tamaño: %d bytes, ", inodos->blq_inodos[directorio[i].dir_inodo].size_fichero);
            printf("Bloques: ");
            for (j = 0; j < MAX_NUMS_BLOQUES_INODO; j++) {
                if (inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[j] != NULL_BLOQUE) {
                    printf("%d ", inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[j]);
                }
            }
            printf("\n");
        }
    }
}

//Ahora la función Renombrar busca un archivo por su nombre y permite cambiarlo por otro. Mejora la funcionalidad del programa
int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo) {
    int i;
    for (i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombreantiguo) == 0) {
            strcpy(directorio[i].dir_nfich, nombrenuevo);
            printf("Archivo renombrado de %s a %s\n", nombreantiguo, nombrenuevo);
            return 0;
        }
    }
    printf("Archivo %s no encontrado\n", nombreantiguo);
    return -1;
    return 0;
}

int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre) {
    // Implementación pendiente
    return 0;
}

int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre, FILE *fich) {
    // Implementación pendiente
    return 0;
}

int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino, FILE *fich) {
    // Implementación pendiente
    return 0;
}

void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich) {
    // Implementación pendiente
}

void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich) {
    // Implementación pendiente
}

void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich) {
    // Implementación pendiente
}

void GrabarDatos(EXT_DATOS *memdatos, FILE *fich) {
    // Implementación pendiente
}

