#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/stat.h>   // For mode constants

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>	// Pour la fonction __raise()

#include <fcntl.h>      // For O_* constants
#include <semaphore.h>

#define ANNEAU_SHM_KEY		1266
#define ANNEAU_SHM_SIZE		65536

#define ANNEAU_NUM_CASES	16
#define ANNEAU_CADENCE_DEFAULT  2000
#define ANNEAU_POS_SERV_OUT	0
#define ANNEAU_POS_SERV_IN	ANNEAU_NUM_CASES - 1

#define COORD_MSG_HELLO		1
#define COORD_MSG_GOODBYE	2
#define COORD_MSG_INFO		3
#define COORD_MSG_PING		4

#define NB_ROBOTS		6
#define NB_OPS			NB_ROBOTS
#define NB_PROD			4

#define SEM_NAME		"/semaphore_anneau"

#define bool int
#define true 1
#define false 0


/**************/
/* Structures */
/**************/

typedef enum {
  DEGRADE = 0,
  NORMAL = 1
} Mode;

typedef struct {
  int id;
  pid_t pid;
  int pos;
  Mode mode;
  char ops[NB_OPS];
  char prods[NB_PROD];
  char prodsDegrades[NB_PROD];
  int stockComposants[NB_PROD];
  int stockProduits[NB_PROD];
} Robot;

/**
 * Structure Composant
 */
typedef struct {
  char num;
} Composant;

/**
 * Structure Produit
 */
typedef struct {
  char num;// Type du composant nécessaire: correspond au numéro du composant
  int nbComp;	// Nombre de composants nécessaires
  char ops[NB_OPS + 1];	// Tableau de bits d'opérations nécessaires
  int etat;	// Numéro de l'opération en attente. varie entre {1..6}. -1 = Produit terminé
} Produit;

/**
 * Différents types de contenants
 */
typedef enum {
  VIDE,
  COMPOSANT,
  PRODUIT
} TypeContenant;

/**
 * Structure Case
 */
typedef struct {
  int num;
  Composant c;
  Produit p;
  TypeContenant type;
} Case;

typedef struct {
  int id;
  Case cases[ANNEAU_NUM_CASES];
  pid_t connexion[ANNEAU_NUM_CASES];
} Anneau;

/**
 * Structure SignalSonore: Utilisée pour l'envoie de la requête sur la file de message
 */
typedef struct {
  long  type;
} SignalSonore;


/**
 * Structure QueryConnexion: Utilisée pour l'envoie de la requête pour 
 */
typedef struct {
  long  type;
  Robot bot;
  int query;
} QueryConnexion;

/**
 * Structure QueryConnexion: Utilisée pour l'envoie de la requête pour 
 */
typedef struct {
  long  type;
  int pos;
} QueryConnexionResponse;

/**
 * Structure QueryConnexion: Utilisée pour l'envoie de la requête pour 
 */
typedef struct {
  long  type;
  int query;
} QueryPing;

/**
 * Structure QueryConnexion: Utilisée pour l'envoie de la requête pour 
 */
typedef struct {
  long  type;
  pid_t pid_bot;
  int query;
} QueryPingResponse;

// // // // // // // // //
// Global shared vars   //
// // // // // // // // // 

pid_t __pid;		// PID du processus
int __shmid;
Anneau *__anneau;		// Ressource critique
sem_t *__semaphore;	// Sémaphore de synchronisation de l'anneau

// // // // // // // //
// Shared functions  //
// // // // // // // // 

/**
 * Affiche la description d'un composant
 */
char* desc_composant(Composant *c);

/**
 * Affiche la description d'un produit
 */
char* desc_produit(Produit *p);

/**
 * Affiche la description d'une case
 */
char* desc_case(Case *c);

int nb_cases_vides();

Produit* init_produits();

/**
 * Marque la fin de l'exécution
 */
void __end_process();

/**
 * Levée d'une exception
 */
int __raise (int exitCode, const char *format, ...);

#endif