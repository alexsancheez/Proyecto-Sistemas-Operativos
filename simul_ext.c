#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include "cabeceras.h"

#define LONGITUD_COMANDO 100
#define MAX_FICHEROS 64
#define MAX_BLOQUES_DATOS 1024
#define MAX_BLOQUES_PARTICION 2048
#define SIZE_BLOQUE 1024
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
    
    // Bucle de tratamiento de comandos
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
        if (strcmp(orden, "rename") == 0) {
            Renombrar(directorio, ext_blq_inodos, argumento1, argumento2);
            continue;
        }
        if (strcmp(orden, "imprimir") == 0) {
            Imprimir(directorio, ext_blq_inodos, memdatos, argumento1);
            continue;
        }
        if (strcmp(orden, "remove") == 0) {
            Borrar(directorio, ext_blq_inodos, ext_bytemaps, &ext_superblock, argumento1, fent);
            continue;
        }
        ...
        // Escritura de metadatos en comandos rename, remove, copy     
        Grabarinodosydirectorio(directorio, ext_blq_inodos, fent);
        GrabarByteMaps(ext_bytemaps, fent);
        GrabarSuperBloque(ext_superblock, fent);
        if (grabardatos)
            GrabarDatos(memdatos, fent);
        grabardatos = 0;
        // Si el comando es salir se habrán escrito todos los metadatos
        // faltan los datos y cerrar
        if (strcmp(orden, "salir") == 0) {
            GrabarDatos(memdatos, fent);
            fclose(fent);
            return 0;
        }
    }
}

int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2) {
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
}

void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup) {
    printf("Bloque %d\n", psup->s_block_size);
    printf("inodos particion = %d\n", psup->s_inodes_count);
    printf("inodos libres = %d\n", psup->s_free_inodes_count);
    printf("bloques particion = %d\n", psup->s_blocks_count);
    printf("bloques libres = %d\n", psup->s_free_blocks_count);
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
}

int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre) {
    int i, j, inodo;
    inodo = BuscaFich(directorio, inodos, nombre);
    if (inodo == -1) {
        printf("Archivo %s no encontrado\n", nombre);
        return -1;
    }
    for (i = 0; i < MAX_NUMS_BLOQUES_INODO; i++) {
        if (inodos->blq_inodos[inodo].i_nbloque[i] != NULL_BLOQUE) {
            for (j = 0; j < SIZE_BLOQUE; j++) {
                printf("%c", memdatos[inodos->blq_inodos[inodo].i_nbloque[i]].dato[j]);
            }
        }
    }
    printf("\n");
    return 0;
}

int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre, FILE *fich) {
    int i, j, inodo;
    inodo = BuscaFich(directorio, inodos, nombre);
    if (inodo == -1) {
        printf("Archivo %s no encontrado\n", nombre);
        return -1;
    }
    // Liberar bloques del archivo
    for (i = 0; i < MAX_NUMS_BLOQUES_INODO; i++) {
        if (inodos->blq_inodos[inodo].i_nbloque[i] != NULL_BLOQUE) {
            ext_bytemaps->bmap_bloques[inodos->blq_inodos[inodo].i_nbloque[i]] = 0;
            inodos->blq_inodos[inodo].i_nbloque[i] = NULL_BLOQUE;
        }
    }
    // Liberar inodo
    ext_bytemaps->bmap_inodos[inodo] = 0;
    strcpy(directorio[inodo].dir_nfich, "");
    directorio[inodo].dir_inodo = NULL_INODO;
    // Actualizar superbloque
    ext_superblock->s_free_inodes_count++;
    ext_superblock->s_free_blocks_count++;
    printf("Archivo %s borrado\n", nombre);
    return 0;
}
