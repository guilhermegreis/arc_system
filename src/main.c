#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat32.h"
#include "output.h"

void usage() {
    printf("Uso do sistema de arquivos FAT32:\n");
    printf("  ./programa ls <imagem_fat>\n");
    printf("  ./programa cp <origem> <destino> <imagem_fat>\n");
    printf("  ./programa mv <origem> <destino> <imagem_fat>\n");
    printf("  ./programa rm <caminho> <imagem_fat>\n");
    printf("  ./programa cat <caminho> <imagem_fat>\n");
    printf("\n");
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printf("Nenhum argumento fornecido. Iniciando menu interativo...\n");

        FILE *fp = fopen("fat32.img", "rb+");
        if (!fp) {
            perror("Erro ao abrir a imagem FAT32");
            return 1;
        }

        struct fat_bpb bpb;
        rfat(fp, &bpb);

        int option;
        char source[100], dest[100];

        do {
            menu();
            scanf("%d", &option);
            switch (option) {
                case 1: ls(fp, &bpb); break;
                case 2:
                    printf("Caminho do arquivo de origem: ");
                    scanf("%s", source);
                    printf("Caminho de destino: ");
                    scanf("%s", dest);
                    cp(fp, source, dest, &bpb);
                    break;
                case 3:
                    printf("Arquivo de origem: ");
                    scanf("%s", source);
                    printf("Destino: ");
                    scanf("%s", dest);
                    mv(fp, source, dest, &bpb);
                    break;
                case 4:
                    printf("Arquivo a remover: ");
                    scanf("%s", source);
                    rm(fp, source, &bpb);
                    break;
                case 5:
                    printf("Arquivo para exibir: ");
                    scanf("%s", source);
                    cat(fp, source, &bpb);
                    break;
                case 0: printf("Encerrando o programa.\n"); break;
                default: printf("Opção inválida.\n");
            }
        } while (option != 0);
        fclose(fp);
    } else if (argc >= 3) {
    } else {
        usage();
    }

    return 0;
}
