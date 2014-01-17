#include "anneau.h"

/**
 * Global vars: définis dans le fichier common.h
 * @var __pid: PID du processus
 * @var __shmid: ID de mémoire partagée
 * @var *__anneau: Anneau partagée dans la mémoire partagée
 * @var *__semaphore: Sémaphore de synchronisation de l'anneau
 */

int main(int argc, char *argv[]) {
  int cadence = ANNEAU_CADENCE_DEFAULT; // Cadence définit en millisecondes
  
  if (argc < 2) {
    __raise(-1, "Usage: %s <projet [, cadence]>", argv[0]);
  }
  
  //
  // Initialisation
  init(argv);
  
  // Cadence
  if (argc > 2) {
    cadence = atoi(argv[2]);
  }
  
  // Anneau
  int i;
  Anneau ano;
  
  ano.id 	= __pid;
  
  for (i = 0; i < ANNEAU_NUM_CASES; i++) {
    ano.cases[i].num 	= i;
    ano.cases[i].type 	= VIDE;
    ano.connexion[i] 	= 0;
  }
  
  *((Anneau *) __anneau) = ano;
    
  // 
  // Démarrage
  
  printf("== Démarrage de l'anneau...\n");
  printf("==== Cadence de rotation: %d ms/tour\n", cadence);
  
  //
  info();
  
  //
  // Petite pause
  usleep(2000);
  
  //
  // Rotation
  int rotation = 1;
  while (1) {
    usleep(cadence * 1000); // Attente
    
    // La roue tourne d'un pas
    tourner();
    printf("\tRotation %5d: \n", rotation++);
    info();
    
    // Émission d'un signalS sonore pour informer les robots 
    ding();
  }
  
  __endProcess();
  return 0;
}

/**
 * Initialisation de l'anneau
 */
void init(char** argv) {
  printf("== Initialisation de l'anneau...\n");
  
  //
  // Mémoire partagée
  printf("==== Création de la mémoire partagée\n");
  
  printf("====== Création du segment\n");
  
  if ((__shmid = shmget(ftok(argv[1], ANNEAU_SHM_KEY), ANNEAU_SHM_SIZE, IPC_CREAT |  0666)) == -1) {
    __raise(-3, "======== ERROR: Impossible de créer le segment partagé.");
  }
  // Attachement à une adresse choisie par le système
  if ((__anneau = shmat(__shmid, NULL, 0)) == (void *) -1) {
    __raise(-4, "======== ERROR: Attachement impossible");
  }
  printf("====== Mémoire partagée %d créée\n", __shmid);
  
  // SIGINT
  signal (SIGINT, callback_sigint);
  
  // Création du sémaphore
  __semaphore = sem_open(SEM_NAME, O_CREAT, 0644, 1);
  
  printf("==[ Utilisez Ctrl+C stopper l'anneau ]==\n\n");
}

/**
 * Fonction de rappel SIGINT
 */
void callback_sigint (int s) {
  printf("\n==== Réception du signal SIGINT");
  printf("\n====== Interuption du processus en cours...\n");
  
  // Stop connexions
  send_signal_to_connexions(SIGINT);
  int i;
  for (i = 0; i < ANNEAU_NUM_CASES; i++) {
    if (__anneau->connexion[i] != 0) {
      waitpid(__anneau->connexion[i]);
    }
  }
  
  int sem_val;
  if (sem_getvalue(__semaphore, &sem_val) == 0) {
    if (sem_val == 0) {
      sem_wait(__semaphore);
    }
  }
  
  // Suppression IPCs
  msgctl(__shmid, IPC_RMID, NULL);
  
  // Suppression du sémaphore
  sem_post(__semaphore);
  sem_close(__semaphore);
  sem_unlink(SEM_NAME);
  
  printf("\n====== IPCs supprimés\n");
  
  __end_process();
  exit(0);
}

/**
 * Envoie le signal s aux processus connectés à l'anneau
 */
void send_signal_to_connexions(int s) {
  static int i;
  printf ("\n\tSIG(%d) sent to [ ", s);
  
  for (i = 0; i < ANNEAU_NUM_CASES; i++) {
    if (__anneau->connexion[i] != 0 && i != ANNEAU_POS_SERV_OUT) {
      kill (__anneau->connexion[i], s);
      printf("%d ", (int) __anneau->connexion[i]);
    }
  }
  
  printf ("]\n");
}


/**
 * Envoie d'un signal sonore
 */
void ding() {
  send_signal_to_connexions(SIGUSR1);
}

/**
 * Permute le contenu de 2 cases
 */
void swap(Case *c1, Case *c2) {
  static Case tmp;
  tmp = *c1;
  *c1 = *c2;
  *c2 = tmp;
}

/**
 * Tourne l'anneau d'un pas
 */
static void tourner() {
  static Case c;
  static int i;
  
  sem_wait(__semaphore);
  
  for (i = 0; i < (ANNEAU_NUM_CASES - 1); i++) {
    swap(&((*((Anneau *) __anneau)).cases[i]), &((*((Anneau *) __anneau)).cases[i + 1]));
  }
  
  sem_post(__semaphore);
}

/**
 * Affiche le contenu de l'anneau
 */
static void info() {
  static Case *c;
  static int i;
  
  // Affichage des éléments de l'anneau
  for (i = 0; i < ANNEAU_NUM_CASES; i++) {
    c = &((*((Anneau *) __anneau)).cases[i]);
    
    printf ("\t\tPosition %2d: Case[%2d] : %s\n", i, c->num, desc_case(c));
  }
  
  fflush (stdout);
}