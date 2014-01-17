#include "common.c"

/**
 * Global vars: définis dans le fichier common.h
 * @var __pid: PID du processus
 * @var __shmid: ID de mémoire partagée
 * @var *__anneau: Anneau partagée dans la mémoire partagée
 * @var *__semaphore: Sémaphore de synchronisation de l'anneau
 */

/**
 * Initialisation de l'anneau
 */
void init(char** argv) ;

/**
 * Fonction de rappel SIGINT
 */
void callback_sigint (int s);

/**
 * Envoie le signal s aux processus connectés à l'anneau
 */
void send_signal_to_connexions(int s);

/**
 * Lance un signal sonore
 */
void ding();

/**
 * Permute le contenu de 2 cases
 */
void swap(Case *c1, Case *c2);

/**
 * Tourne l'anneau d'un pas
 */
static void tourner();

/**
 * Affiche le contenu de l'anneau
 */
static void info();