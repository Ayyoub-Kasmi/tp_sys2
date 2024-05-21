#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global Variable for the operating system (Linux = 1, MacOS = 2)
int os_number = 1;

// gpt_header struct (512 bytes)
struct {
  unsigned char part1[24];

  unsigned char LBA_address[8];
  unsigned char LBA_address_copy[8];
  unsigned char P_adrlba_Donnee[8];
  unsigned char D_adrlba_Donnee[8];
  unsigned char GUIDE[16];
  unsigned char adrlba_TP[8];
  unsigned char NB_Entree_TP[4];
  unsigned char Taille_Entree_TP[4];

  unsigned char part3[424];

} gpt_header;

struct {
  struct {
    unsigned char TypeGuide[16];
    unsigned char GuideU[16];
    unsigned char P_adrlba[8];
    unsigned char D_adrlba[8];
    unsigned char Attributs[8];

    unsigned char Nom_Partition[72];

  } EntreeTable[4];

} sector;

struct {
  unsigned char partie1[11];
  struct {
    unsigned char partie1[2];
    unsigned char NbSecRes[2];
    unsigned char NbFat;
    unsigned char partie2[5];
    unsigned char NbSecFat[2];
    unsigned char partie3[12];
  } Bios;
  unsigned char padding [8];
  unsigned char firstClusterNumber;
  unsigned char partie2[468];
} boot_sector;

void list_os() {
  // Supported options: Linux and MacOS
  printf("1. Linux\n");
  printf("2. MacOS\n");
}

// print the list of available disks
void list_disks() {
  // look for files that start with 'sd' or 'disk' and have a length of 3
  struct dirent *de;
  DIR *dr = opendir("/dev");
  if (dr == NULL) {
    printf("(error) '/dev' could not be opened\n");
    return;
  }

  char *name;
  bool disks_found = false;
  while ((de = readdir(dr)) != NULL) {
    if (de->d_name[0] == 's' && de->d_name[1] == 'd' &&
        strlen(de->d_name) == 3) {
      disks_found = true;
      printf("%s\n", de->d_name);
    }
  }
  closedir(dr);
  if (!disks_found) {
    printf("(error) No disk in this machine\n");
  }
  return;
}

// Procédure d'affichage des informations de disque
void print_disk_information(char *nom_disque) {
    // 1. open the disk file from /dev
  FILE *disque;
  char path[255] = "";
  int read_count;
  strcat(path, "/dev/");
  strcat(path, nom_disque);
  printf("(info) full disk path: %s\n", path);

  disque = fopen(path, "rb");
  printf("(info) disk pointer: %p\n", disque);
  if (disque == NULL) {
    printf("\n(error) 'disk %s' is not opened\n", path);
    exit(1);
  }

    // 2. Read the first 512 bytes (i.e the GPT header) from the file descriptor
  int err = fseek(disque, 512, SEEK_SET);

  if (err != 0) {
    printf( "\n (error) %d le deplacement n'a pas été effectuer sur le disque \n", err);
    exit(1);
  }

  read_count = fread(&gpt_header, 512, 1, disque);

  if (read_count <= 0) {
    printf("\n (error) read error n = %d", read_count);
    exit(1);
  }

    // 3. Divide the 512 bytes into the GPT header fields
  int adr = *(int *)&(gpt_header.adrlba_TP);
  printf("\n ***** Adresse LBA de la table des partitions %d \n", adr);

  // int adrDebut = *(int *)&(gpt_header.P_adrlba_Donnee);
  // printf("\n ***** Adresse début LBA de la zonne données  %d \n", adrDebut);

  // int adrFIN = *(int *)&(gpt_header.D_adrlba_Donnee);
  // printf("\n ***** Adresse fin LBA de la zonne données %d \n", adrFIN);
}

// Calculer l'adresse LBA de cluster racine
int cluster_address(int adr, FILE *disque) {

  printf("\n\n\t\t******************* l'adresse LBA de cluster racine ******************* \n");

  if (disque == NULL) {
    printf("\n (error) le disque n'est ouvert \n");
    exit(0);
  }

  printf("\n adr %d \n", adr);
  int adrD = 512 * adr;

  int err = fseek(disque, adrD, SEEK_SET);

  if (err != 0) {
    printf( "\n (error) %d le deplacement n'a pas été effectuer sur le disque \n", err);
    exit(0);
  }

  int read_count = fread(&boot_sector, 512, 1, disque);

  if (read_count <= 0) {
    printf("\n (error) de lecture n = %d", read_count);
    exit(0);
  }

  // char bios=*(char *) &(secteurBoot.Bios);

  int NbrSecteurRes = *(int *)&(boot_sector.Bios.NbSecRes);

  int NbrFat = *(int *)&(boot_sector.Bios.NbFat);
  int NbSecFat = *(int *)&(boot_sector.Bios.NbSecFat);

//  printf("\n Adresse LBA premier cluster : %d \n ", AdrCluster);
    return adr + NbrSecteurRes + (NbrFat * NbSecFat);
}

// Procédure d'affichage des informations de partitions
void afficher_info_Partition(char *nom_disque) {
  FILE *disque;
  char path[255] = "";
  int read_count;
  strcat(path, "/dev/");
  strcat(path, nom_disque);

  disque = fopen(path, "rb");
  if (disque == NULL) {
    printf("\n (error) le disque n'est ouvert \n");
    exit(0);
  }

  int err = fseek(disque, 512 * 2, SEEK_SET);

  if (err != 0) {
    printf(
        "\n (error) %d le deplacement n'a pas été effectuer sur le disque \n",
        err);
    exit(0);
  }

  read_count = fread(&sector, 512, 1, disque);

  if (read_count <= 0) {
    printf("\n (error) de lecture n = %d", read_count);
    exit(0);
  }

  printf("\n\n\t\t******************* Table de partition du disque "
         "******************* \n");

  printf("\n|-----------|----------------|----------------|----------------|---"
         "------------|");
  printf("\n| Partition |    Taille(secteur)   |  Taille KO       | Premier cluster |  "
         "LBA premier cluster|");
  printf("\n|-----------|----------------|----------------|----------------|---"
         "------------|");

    int x = 1024;
    for (int i = 0; i < 4; i++) {

        int adrDebutPartition = *(int *) &(sector.EntreeTable[i].P_adrlba);
        // printf("\n première adresse LBA de la partition %d \n",adrDebutPartition);

        int adrFinPartition = *(int *) &(sector.EntreeTable[i].D_adrlba);
        // printf("\n dernière adresse LBA de la partition  %d \n",adrFinPartition);

        int taille_sectors = adrFinPartition - adrDebutPartition;
        // printf("\n La taille de la partition en nombre de secteur  %d
        // \n",taille);
        double taille_KO, y;
        y = (double) taille_sectors * 512;
        taille_KO = y / x;

        int clusterLBAAddress = cluster_address(adrDebutPartition, disque);

        // printf( "\n| sdb%d      |   x'%8d'  |   x'%8d'  |    %8d    |     %8.3f     |", i, adrDebutPartition, adrFinPartition, taille, taille_KO);
        printf("\n| %s%d      |   x'%9d'  |    %9.3f    |     %9d     |     %9d     |", nom_disque, i, taille_sectors, taille_KO, boot_sector.firstClusterNumber, clusterLBAAddress);
        printf("\n|-----------|----------------|----------------|---------------|");
    }

}

int main() {

  printf("--------- Tell us your operating system ---------\n");
  list_os();
  printf("=> Enter the number of the operating system: ");
  scanf("%d", &os_number);

  // Print the list of available disks
  list_disks();
  // lire
  printf("--------- Choose a disk ---------\n");
  char disk[10];
  printf("=> Enter the name of one of the physical disks printed above: ");
  scanf("%9s", disk);

  print_disk_information(disk);

  afficher_info_Partition(disk);
}
