/********************************/
/* Interface du serveur         */
/********************************/

#include "common.c"

/**
 * Global vars: définis dans le fichier common.h
 * @var __pid: PID du processus
 * @var __shmid: ID de mémoire partagée
 * @var *__anneau: Anneau partagée dans la mémoire partagée
 * @var *__semaphore: Sémaphore de synchronisation de l'anneau
 */

int msgid; // Identifiant de la file de message

pthread_t thread_id; // ID du thread (coordinateur)

Produit *produits; // Liste de profils des produits
int produitsPlanifies[NB_PROD] = {10, 15, 12, 8}; // Nombre de produits à fabriquer
int produitsFabriques[NB_PROD] = {0}; // Nombre de produits fabriqués
int stockComposants[NB_PROD]; // Stock de composants = produitsPlanifies * produitsFabriques

static char log[100]; // Utiliser pour info()

/**
 * Initialisation principale
 */
void init(char **argv);

/**
 * Exécutée par le thread: Coordinateur
 */
void callback_thread_coord(void *project);

/**
 * Fonction de rappel SIGINT du serveur
 */
void callback_sigint_server (int);

/**
 * Fonction de rappel SIGINT du coordinateur
 */
void callback_sigint_coord (int);

/**
 * Fonction de rappel SIGUSR1 du serveur.
 * Exécutée après que l'anneau ai fait un pas de rotation
 */
void callback_sigusr1_anneau_tourne(int);

/**
 * Fonction de rappel SIGUSR1: Connexion d'un nouveau robot
 */
QueryConnexionResponse callback_new_connexion(QueryConnexion *);

/**
 * c'est pas assez clair le nom de la fonction ? :)
 */
bool ya_til_des_robots_connectes();

/**
 * c'est pas assez clair le nom de la fonction ? :)
 */
bool puis_je_prendre_produit();

/**
 * Retourne le nombre de composants restants en stock
 */
int nb_composants_restants();

/**
 * Envoie le signal s aux robots connectés à l'anneau
 */
void send_signal_to_bots(int s);

/**
 * Affichage des informations du serveur
 */
void info();