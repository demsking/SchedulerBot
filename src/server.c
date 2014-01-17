#include "server.h"

/**
 * Global vars: définis dans le fichier common.h
 * @var pid_t __pid			PID du processus
 * @var int __shmid			ID de mémoire partagée
 * @var Anneau *__anneau		Anneau partagée dans la mémoire partagée
 * @var sem_t *__semaphore		Sémaphore de synchronisation de l'anneau
 *
 * Global vars: définis dans le fichier server.h
 * @var int msgid 			Identifiant de la file de message
 * @var pthread_t thread_id		ID du thread (coordinateur)
 * @var Produit *produits 		Liste de profils des produits
 * @var int produitsPlanifies[NB_PROD]	Nombre de produits à fabriquer
 * @var int produitsFabriques[NB_PROD]	Nombre de produits fabriqués
 * @var int stockComposants[NB_PROD]	Stock de composants = produitsPlanifies * produitsFabriques
 */

int main(int argc, char *argv[]) {
  
  if (argc != 2) {
    __raise(-1, "Usage: %s <projet>", argv[0]);
  }
  
  //
  // Initialisation
  init(argv);
  
  //
  // Démarrage du coordinateur de gestion de connexion des robots
  pthread_create(&thread_id, 0, callback_thread_coord, (void *) argv[1]);
  
  //
  // Branchement du serveur aux position ANNEAU_POS_SERV_IN et ANNEAU_POS_SERV_OUT
  __anneau->connexion[ANNEAU_POS_SERV_IN]  = __pid;
  __anneau->connexion[ANNEAU_POS_SERV_OUT] = __pid;
  printf("== Serveur connecté aux canneaux d'entrée %d et de sortie %d de l'anneau\n", ANNEAU_POS_SERV_IN, ANNEAU_POS_SERV_OUT);
  
  //
  // Armemant de SIGUSR1: exécutée chaque fois que l'anneau tourne
  signal(SIGINT, callback_sigint_server);
  signal(SIGUSR1, callback_sigusr1_anneau_tourne);
  
  //
  // Début du travail
  sigset_t mask;
  
  sigfillset(&mask);
  sigdelset(&mask, SIGINT);
  sigdelset(&mask, SIGUSR1);
  
  while (1) {
    sigsuspend(&mask);
  }
  
  return 0;
}
/**
 * Initialisation principale
 */
void init(char **argv) {
  void *anneau_addr;
  
  __pid = getpid();
  produits = init_produits();
  
  int i;
  for (i = 0; i < NB_PROD; i++) {
    stockComposants[i] = produitsPlanifies[i] * produits[i].nbComp; // Initalisation du stock de composants nécessaires
  }
  
  //
  //
  printf("== Initialisation du serveur %d...\n", (int) __pid);
  
  //
  // Mémoire partagée
  printf("==== Initialisation de la mémoire partagée\n");
  
  if ((__shmid = shmget(ftok(argv[1], ANNEAU_SHM_KEY), ANNEAU_SHM_SIZE, 0666)) == -1) {
    __raise(-3, "======== ERROR: Impossible de se connecter à la mémoire partagé. Rassurez-vous que l'anneau est en cours de fonctionnement");
  }
  // Attachement à une adresse choisie par le système
  if ((anneau_addr = shmat(__shmid, NULL, 0)) == (void *) -1) {
    __raise(-4, "======== ERROR: Attachement impossible");
  }
  printf("====== Connecté à la mémoire partagée %d\n", __shmid);
  
  // Initialisation de l'anneau
  __anneau = (Anneau *) anneau_addr;
  
  //
  // Ouverture du sémaphore
  __semaphore = sem_open(SEM_NAME, 0);
}

/**
 * Fonction de rappel SIGINT du serveur
 */
void callback_sigint_server (int s) {
  printf("==== Réception du signal SIGINT\n");
  printf("====== Interuption du processus en cours...\n");
  
  // Déconnexion de l'anneau
  __anneau->connexion[ANNEAU_POS_SERV_IN]  = 0;
  __anneau->connexion[ANNEAU_POS_SERV_OUT] = 0;
  printf("======== Serveur déconnecté de l'anneau\n");
  
  msgctl(__shmid, IPC_RMID, NULL);
  printf("======== Déconnecté du segment (IPC)\n");
  
  // Fermeture du sémaphore
  int sem_val;
  if (sem_getvalue(__semaphore, &sem_val) == 0) {
    if (sem_val == 1) {
      sem_post(__semaphore);
    }
  }
  sem_close(__semaphore);
  
  //
  // Arrêt du coordinateur
  printf("====== Interuption du coordinateur en cours...\n");
  if (pthread_cancel(thread_id) != 0) {
    printf("\n====== Échec d'interruption du coordinateur\n");
  }
  
  msgctl(msgid, IPC_RMID, NULL);
  printf("====== File de message (IPC) supprimée\n");
  
  printf("====== Libération de la mémoire\n");
  if (produits) {
    free(produits);
  }
  
  __end_process();
  exit(0);
}

/**
 * Exécutée par le thread: Coordinateur
 */
void callback_thread_coord(void *project) {
  //
  // File de message: Bus de communication entre le coordinateur et les robots
  if ((msgid = msgget(ftok((char*) project, ANNEAU_SHM_KEY * __pid), IPC_CREAT | IPC_EXCL | 0666)) == -1) {
    __raise(1, "======== ERROR: Impossible de créer la file de message !");
  }
  printf("====== File coord créée avec l'identificateur %d\n", msgid);
  
  //
  // 
  QueryConnexion q;
  QueryConnexionResponse r;
  int query_connexion_size = sizeof(QueryConnexion) - sizeof(long);
  int query_connexion_response_size = sizeof(QueryConnexionResponse) - sizeof(long);
  
  int i;
  
  //
  // Écoute
  printf("====== Cordinateur à l'écoute...\n");
  
  while (1) {
    msgrcv(msgid, &q, query_connexion_size, __pid, 0);
    
    switch (q.query) {
      case COORD_MSG_HELLO:
	printf("======> Connexion du robot R%d %d...\n", q.bot.id, (int) q.bot.pid);
	
	r = callback_new_connexion(&q);
	
	msgsnd(msgid, &r, query_connexion_response_size, 0);
	break;
	
      case COORD_MSG_GOODBYE:
	printf("<====== Déconnexion du robot R%d (%d)...\n", q.bot.id, (int) q.bot.pid);
	break;
    }
  }
  
  pthread_exit(NULL);
}

/**
 * Fonction de rappel SIGUSR1 du serveur.
 * Exécutée après que l'anneau ai fait un pas de rotation
 */
void callback_sigusr1_anneau_tourne(int s) {
  static Case *case_in;
  static Case *case_out;
  static int j = 0;
  static int i;
  
  sem_wait(__semaphore);
  
  case_in  = &(__anneau->cases[ANNEAU_POS_SERV_IN]);
  case_out = &(__anneau->cases[ANNEAU_POS_SERV_OUT]);
  
  sprintf(log, " ");
  
  // Si la case IN contient un produit
  if (case_in->type == PRODUIT) {
    // Si la fabrication de ce produit est terminée
    if ((__anneau->cases[ANNEAU_POS_SERV_IN]).p.etat == -1) {
      // Je le stocke
      produitsFabriques[ctoi(case_in->p.num) - 1]++;
      produitsPlanifies[ctoi(case_in->p.num) - 1]--;
      sprintf(log, " Stock de %s", desc_case(case_in));
      case_in->type = VIDE;
    }
  }
  
  // Distribution
  if (ya_til_des_robots_connectes()) {
    if (case_out->type == VIDE) {
      if (nb_composants_restants() > 0) {
	if (nb_cases_vides() > 3) {
	  distri:
	  if (j == NB_PROD) {
	    j = 0;
	  }
	  if (stockComposants[j] == 0) {
	    j++;
	    goto distri;
	  }
	  
	  case_out->c.num = itoc(j + 1);
	  case_out->type = COMPOSANT;
	  stockComposants[j]--;
	  j++;
	  
	  strcat(log, " Distribution de ");
	  strcat(log, desc_composant(&case_out->c));
	}
      } else {
	strcat(log, " Stock épuisé");
      }
    }
  }
  
  sem_post(__semaphore);
  
  info();
}

/**
 * Fonction de rappel SIGUSR1: Connexion d'un nouveau robot
 */
QueryConnexionResponse callback_new_connexion(QueryConnexion *q) {
  static i;
  static int iter = (int) (ANNEAU_NUM_CASES / NB_ROBOTS);
  
  for (i = 0; i < ANNEAU_NUM_CASES; i += iter) {
    if (__anneau->connexion[i] == 0) {
      break;
    }
  }
  
  if (i > ANNEAU_NUM_CASES) {
    for (i = ANNEAU_NUM_CASES; i >= 0; i--) {
      if (__anneau->connexion[i] == 0) {
	break;
      }
    }
  }
  
  QueryConnexionResponse r;
  r.type = q->bot.pid;
  r.pos	 = i;
  
  return r;
}

/**
 * c'est pas assez clair le nom de la fonction ? :)
 */
bool ya_til_des_robots_connectes() {
  static int i;
  for (i = 0; i < NB_ROBOTS; i++) {
    if (__anneau->connexion[i] != 0 && __anneau->connexion[i] != __pid) {
      return true;
    }
  }
  return false;
}

/**
 * c'est pas assez clair le nom de la fonction ? :)
 */
bool puis_je_prendre_produit() {
  // Si le produit est terminée
  if ((__anneau->cases[ANNEAU_POS_SERV_IN]).p.etat == -1) {
    return true;
  }
  return false;
}

/**
 * Retourne le nombre de composants restants en stock
 */
int nb_composants_restants() {
  static int i, num;
  for (i = 0, num = 0; i < NB_PROD; i++) {
    num += stockComposants[i];
  }
  return num;
}

/**
 * Affichage des informations du serveur
 */
void info() {
  printf("⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯[Server]⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯\n");
  printf("             PID : %d\n", (int) __pid);
  
  printf("⎬⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯[état in/out]⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎨\n");
  printf("              IN : %s\n", desc_case(&(__anneau->cases[ANNEAU_POS_SERV_IN])));
  printf("             OUT : %s\n\n", desc_case(&(__anneau->cases[ANNEAU_POS_SERV_OUT])));
  
  printf("⎬⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯[stats]⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎨\n\n");
  printf("                      1   2   3   4\n");
  printf("  Stock composants : %2d  %2d  %2d  %2d\n", stockComposants[0], stockComposants[1], stockComposants[2], stockComposants[3]);
  printf(" Produits planifés : %2d  %2d  %2d  %2d\n", produitsPlanifies[0], produitsPlanifies[1], produitsPlanifies[2], produitsPlanifies[3]);
  printf("Produits fabriqués : %2d  %2d  %2d  %2d\n\n\n", produitsFabriques[0], produitsFabriques[1], produitsFabriques[2], produitsFabriques[3]);
  printf("   %s\n\n\n", log);
}


