#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cabeceras.h"

void leer_particion(unsigned char *particion) {
    FILE *file = fopen("particion.bin", "rb");
    if (file == NULL) {
        perror("Error al abrir particion.bin");
        exit(1);
    }
    fread(particion, SIZE_BLOQUE, MAX_BLOQUES_PARTICION, file);
    fclose(file);
}

void info(EXT_SIMPLE_SUPERBLOCK *sb) {
    printf("Inodes count: %d\n", sb->s_inodes_count);
    printf("Blocks count: %d\n", sb->s_blocks_count);
    printf("Free blocks count: %d\n", sb->s_free_blocks_count);
    printf("Free inodes count: %d\n", sb->s_free_inodes_count);
    printf("First data block: %d\n", sb->s_first_data_block);
    printf("Block size: %d\n", sb->s_block_size);
}

void bytemaps(EXT_BYTE_MAPS *bm) {
    printf("Bytemap de bloques:\n");
    for (int i = 0; i < 25; i++) {
        printf("%d ", bm->bmap_bloques[i]);
    }
    printf("\nBytemap de inodos:\n");
    for (int i = 0; i < MAX_INODOS; i++) {
        printf("%d ", bm->bmap_inodos[i]);
    }
    printf("\n");
}

void dir(EXT_ENTRADA_DIR *dir_entries, EXT_SIMPLE_INODE *inodos) {
    for (int i = 0; i < 20; i++) {
        if (dir_entries[i].dir_inodo != 0xFFFF) {
            printf("Nombre: %s, Inodo: %d, Tamaño: %d\n",
                   dir_entries[i].dir_nfich, dir_entries[i].dir_inodo,
                   inodos[dir_entries[i].dir_inodo].size_fichero);
        }
    }
}

void rename_file(EXT_ENTRADA_DIR *dir_entries, char *old_name, char *new_name) {
    for (int i = 0; i < 20; i++) {
        if (strcmp(dir_entries[i].dir_nfich, old_name) == 0) {
            strcpy(dir_entries[i].dir_nfich, new_name);
            return;
        }
    }
    printf("Error: archivo no encontrado o nombre ya existente.\n");
}

void imprimir(EXT_SIMPLE_INODE *inodos, EXT_ENTRADA_DIR *dir_entries, char *filename, unsigned char *particion) {
    int inodo_index = -1;
    for (int i = 0; i < 20; i++) {
        if (strcmp(dir_entries[i].dir_nfich, filename) == 0) {
            inodo_index = dir_entries[i].dir_inodo;
            break;
        }
    }
    if (inodo_index == -1) {
        printf("Error: archivo no encontrado.\n");
        return;
    }
    for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) {
        if (inodos[inodo_index].i_nbloque[i] != 0xFFFF) {
            // Mostrar el contenido del bloque
            fwrite(&particion[inodos[inodo_index].i_nbloque[i] * SIZE_BLOQUE], SIZE_BLOQUE, 1, stdout);
        }
    }
    printf("\n");
}

void remove_file(EXT_ENTRADA_DIR *dir_entries, EXT_SIMPLE_INODE *inodos, EXT_BYTE_MAPS *bm, char *filename) {
    for (int i = 0; i < 20; i++) {
        if (strcmp(dir_entries[i].dir_nfich, filename) == 0) {
            int inodo_index = dir_entries[i].dir_inodo;
            dir_entries[i].dir_inodo = 0xFFFF;
            dir_entries[i].dir_nfich[0] = '\0';
            inodos[inodo_index].size_fichero = 0;
            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
                if (inodos[inodo_index].i_nbloque[j] != 0xFFFF) {
                    bm->bmap_bloques[inodos[inodo_index].i_nbloque[j]] = 0;
                    inodos[inodo_index].i_nbloque[j] = 0xFFFF;
                }
            }
            bm->bmap_inodos[inodo_index] = 0;
            return;
        }
    }
    printf("Error: archivo no encontrado.\n");
}

void copy_file(EXT_ENTRADA_DIR *dir_entries, EXT_SIMPLE_INODE *inodos, EXT_BYTE_MAPS *bm, char *src, char *dest, unsigned char *particion) {
    int src_inodo = -1, dest_inodo = -1;
    for (int i = 0; i < 20; i++) {
        if (strcmp(dir_entries[i].dir_nfich, src) == 0) {
            src_inodo = dir_entries[i].dir_inodo;
        }
        if (strcmp(dir_entries[i].dir_nfich, dest) == 0) {
            printf("Error: archivo destino ya existe.\n");
            return;
        }
        if (dir_entries[i].dir_inodo == 0xFFFF && dest_inodo == -1) {
            dest_inodo = i;
        }
    }
    if (src_inodo == -1) {
        printf("Error: archivo origen no encontrado.\n");
        return;
    }
    int new_inodo = -1;
    for (int i = 0; i < MAX_INODOS; i++) {
        if (bm->bmap_inodos[i] == 0) {
            new_inodo = i;
            break;
        }
    }
    if (new_inodo == -1) {
        printf("Error: no hay inodos libres.\n");
        return;
    }
    bm->bmap_inodos[new_inodo] = 1;
    strcpy(dir_entries[dest_inodo].dir_nfich, dest);
    dir_entries[dest_inodo].dir_inodo = new_inodo;
    inodos[new_inodo].size_fichero = inodos[src_inodo].size_fichero;
    for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) {
        if (inodos[src_inodo].i_nbloque[i] != 0xFFFF) {
            for (int j = 0; j < MAX_BLOQUES_PARTICION; j++) {
                if (bm->bmap_bloques[j] == 0) {
                    bm->bmap_bloques[j] = 1;
                    inodos[new_inodo].i_nbloque[i] = j;
                    memcpy(&particion[j * SIZE_BLOQUE], &particion[inodos[src_inodo].i_nbloque[i] * SIZE_BLOQUE], SIZE_BLOQUE);
                    break;
                }
            }
        }
    }
}

int main() {
    unsigned char particion[MAX_BLOQUES_PARTICION * SIZE_BLOQUE];
    leer_particion(particion);

    EXT_SIMPLE_SUPERBLOCK *sb = (EXT_SIMPLE_SUPERBLOCK *) particion;
    EXT_BYTE_MAPS *bm = (EXT_BYTE_MAPS *) (particion + SIZE_BLOQUE);
    EXT_SIMPLE_INODE *inodos = (EXT_SIMPLE_INODE *) (particion + 2 * SIZE_BLOQUE);
    EXT_ENTRADA_DIR *dir_entries = (EXT_ENTRADA_DIR *) (particion + 3 * SIZE_BLOQUE);

    char comando[32], arg1[32], arg2[32];

    while (1) {
        printf(">> ");
        scanf("%s", comando);

        if (strcmp(comando, "info") == 0) {
            info(sb);
        } else if (strcmp(comando, "bytemaps") == 0) {
            bytemaps(bm);
        } else if (strcmp(comando, "dir") == 0) {
            dir(dir_entries, inodos);
        } else if (strcmp(comando, "rename") == 0) {
            scanf("%s %s", arg1, arg2);
            rename_file(dir_entries, arg1, arg2);
        } else if (strcmp(comando, "imprimir") == 0) {
            scanf("%s", arg1);
            imprimir(inodos, dir_entries, arg1, particion);
        } else if (strcmp(comando, "remove") == 0) {
            scanf("%s", arg1);
            remove_file(dir_entries, inodos, bm, arg1);
        } else if (strcmp(comando, "copy") == 0) {
            scanf("%s %s", arg1, arg2);
            copy_file(dir_entries, inodos, bm, arg1, arg2, particion);
        } else if (strcmp(comando, "exit") == 0) {
            break;
        } else {
            printf("Comando no reconocido.\n");
        }
    }

    return 0;
}
