/*-----------------*/
/* Processus Robot */
/*-----------------*/

#include "common.c"

/**
 * Global vars: définis dans le fichier common.h
 * @var pid_t __pid		PID du processus
 * @var int __shmid		ID de mémoire partagée
 * @var Anneau *__anneau	Anneau partagée dans la mémoire partagée
 * @var sem_t *__semaphore	Sémaphore de synchronisation de l'anneau
 */

Robot bot; // Représente le processus robot

pthread_t thread_id;

Produit *produits;
Produit produitsStock[NB_PROD];

int msgid;

pid_t pid_coord; // PID du coordinateur (SERVER)

/**
 * Vars de log: pour info()
 */
static char log[255];
static char log_curr_pos[50];
static char log_in[50];
static char log_out[50];

/**
 * Initialisation principale
 */
void init(char **projet);

/**
 * Fonction de rappel SIGINT
 */
void callback_sigint(int s);

/**
 * Fonction de rappel SIGINT
 */
void callback_sigint (int s);

/**
 * Fonction de rappel SIGUSR1 du robot.
 * Exécutée après que l'anneau ai fait un pas de rotation
 */
void callback_sigusr1_anneau_tourne(int s);

/**
 * Fonction de rappel SIGUSR2: Permet de basculer au mode dégradé/normal
 */
void callback_sigusr2_mode (int s);

/**
 * Fonction de rappel SIGUSR3: PIND
 */
void callback_sigusr3_ping(int s);

/**
 * Connexion au coordinateur
 *   Opérations:
 * 	0: Connexion à la file de message ouverte par le coordinateur
 * 	1: Connexion au coordinateur
 * 	2: Réception de l'indice de positionnement sur l'anneau
 */
void connect_to_coord(char *project);

/**
 * Définit si le caractère val existe dans la chaîne array
 */
static bool has(char *array, char val);

/**
 * c'est pas assez clair le nom de la fonction ? :)
 */
bool puis_je_prendre_composant();

/**
 * c'est pas assez clair le nom de la fonction ? :)
 */
bool puis_je_prendre_produit();

/**
 * c'est pas assez clair le nom de la fonction ? :)
 */
Composant prendre_composant();

/**
 * c'est pas assez clair le nom de la fonction ? :)
 */
Produit prendre_produit();

/**
 * Affichage d'infos sur le robot
 */
void info();