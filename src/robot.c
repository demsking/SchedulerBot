#include "robot.h"

/**
 * Global vars: définis dans le fichier common.h
 * @var pid_t __pid		PID du processus
 * @var int __shmid		ID de mémoire partagée
 * @var Anneau *__anneau	Anneau partagée dans la mémoire partagée
 * @var sem_t *__semaphore	Sémaphore de synchronisation de l'anneau
 */

int main(int argc, char *argv[]) {
  
  if (argc != 6) {
    __raise(-1, "Usage: %s <projet, id, ops, chaîne de produits, chaîne de produits en mode dégradés>", argv[0]);
  }
  
  //
  // Initialisation
  init(argv);
  
  //
  // Attente de connexion du coordinateur
  while (__anneau->connexion[ANNEAU_POS_SERV_OUT] == 0) {
    usleep(1000);
  }
  pid_coord = __anneau->connexion[ANNEAU_POS_SERV_OUT];
  
  //
  // Démarrage du dispositif de communication avec le serveur
  // Opérations:
  //	0: Connexion à la file de message ouverte par le coordinateur
  // 	1: Connexion au coordinateur
  // 	2: Réception de l'indice de positionnement sur l'anneau
  // 	3: Reste en communication permanente avec le coordinateur
  connect_to_coord(argv[1]);

  //
  // Chargement des signaux SIGUSR1, SIGUSR2 et SIGINT
  signal(SIGINT, callback_sigint);
  signal(SIGUSR1, callback_sigusr1_anneau_tourne);
  signal(SIGUSR2, callback_sigusr2_mode);
  
  //
  // Début du travail
  sigset_t mask;
  
  sigfillset(&mask);
  sigdelset(&mask, SIGINT);
  sigdelset(&mask, SIGUSR1);
  sigdelset(&mask, SIGUSR2);
  
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
  pid_t pid = getpid();
  int i;
  
  produits = init_produits();
  
  printf("== Initialisation du robot R%s (%d)...\n", argv[2], (int) pid);
  
  bot.id   = atoi(argv[2]);
  bot.pid  = getpid();
  bot.mode = NORMAL;
  bot.pos  = -1;
  
  sprintf(bot.ops, "%s", argv[3]);
  sprintf(bot.prods, "%s", argv[4]);
  sprintf(bot.prodsDegrades, "%s", argv[5]);
  
  for (i = 0; i < NB_PROD; i++) {
    bot.stockComposants[i] = 0;
    bot.stockProduits[i] = 0;
  }
  
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
  // Création du sémaphore
  __semaphore = sem_open(SEM_NAME, 0);
}

/**
 * Fonction de rappel SIGINT
 */
void callback_sigint(int s) {
  printf("\n==== Réception du signal SIGINT");
  printf("\n====== Interuption du processus en cours...\n");
  
  // Envoie d'un signal au coordinateur
  if (pid_coord != 0) {
    QueryConnexion query;
    
    query.type  = pid_coord;
    query.query = COORD_MSG_GOODBYE;
    query.bot   = bot;
    
    msgsnd(msgid, &query, sizeof(QueryConnexion) - sizeof(long), 0);
  }
  
  // Déconnexion de l'anneau
  if (bot.pos != -1) {
    __anneau->connexion[bot.pos] = 0;
    printf("\n====== Déconnecté de l'anneau\n");
  }
  
  // Suppression IPCs
  msgctl(__shmid, IPC_RMID, NULL);
  
  // Fermeture du sémaphore
  int sem_val;
  if (sem_getvalue(__semaphore, &sem_val) == 0) {
    if (sem_val == 1) {
      sem_post(__semaphore);
    }
  }
  sem_close(__semaphore);
  
  // Libération de l'anneau
  free(produits);
  
  // Interuption du thread
  if (pthread_cancel(thread_id) != 0) {
    printf("\n====== Échec d'interruption du thread\n");
  }
  
  __end_process();
  exit(0);
}

/**
 * Connexion au coordinateur
 *   Opérations:
 * 	0: Connexion à la file de message ouverte par le coordinateur
 * 	1: Connexion au coordinateur
 * 	2: Réception de l'indice de positionnement sur l'anneau
 */
void connect_to_coord(char *project) {
  QueryPing q;
  QueryPingResponse r;
  int query_size = sizeof(QueryPing) - sizeof(long);
  int query_response_size = sizeof(QueryPingResponse) - sizeof(long);
  
  //
  // File de message: Bus de communication entre le coordinateur et les robots
  if ((msgid = msgget(ftok(project, ANNEAU_SHM_KEY * pid_coord), 0666)) == -1) {
    __raise(1, "======== ERROR: Impossible de créer la file de message !");
  }
  printf("====== Connecté au buss coord %d\n", msgid);
  
  //
  // Connexion au coordinateur
  QueryConnexion query;
  QueryConnexionResponse response;
  int query_connexion_size = sizeof(QueryConnexion) - sizeof(long);
  int query_connexion_response_size = sizeof(QueryConnexionResponse) - sizeof(long);
  
  query.type  = pid_coord;
  query.query = COORD_MSG_HELLO;
  query.bot   = bot;
  
  msgsnd(msgid, &query, query_connexion_size, 0);
  msgrcv(msgid, &response, query_connexion_response_size, (int) bot.pid, 0);
  
  bot.pos = response.pos;
  
  //
  // Connexion à l'anneau
  __anneau->connexion[bot.pos] = bot.pid;
  printf("====== Robot %d connecté en %d\n", bot.id, bot.pos);
}

/**
 * Fonction de rappel SIGUSR2: Permet de basculer au mode dégradé/normal
 */
void callback_sigusr2_mode (int s) {
  bot.mode = bot.mode == NORMAL ? DEGRADE : NORMAL;
  info();
  return;
}

/**
 * Définit si le caractère val existe dans la chaîne array
 */
static bool has(char *array, char val) {
  static int i;
  
  for (i = 0; array[i]; i++) {
    if (array[i] == val) {
      return true;
    }
  }
  return false;
}

/**
 * c'est pas assez clair le nom de la fonction ? :)
 */
bool puis_je_prendre_composant() {
  // Si je peux travailler sur le produit correspondant
  if (has(bot.mode == NORMAL ? bot.prods : bot.prodsDegrades, (__anneau->cases[bot.pos]).c.num)) {
    static int i;
    i = ctoi((__anneau->cases[bot.pos]).c.num) - 1;
    
    // Si j'ai de la place pour stocker ce composant
    if (bot.stockComposants[i] < 3) {
      // S'il manque n-1 composants pour initaliser le produit
      if (bot.stockComposants[i] == (produits[i].nbComp - 1)) {
	// S'il y a de place pour stocker le futur produit
	if (bot.stockProduits[i] == 0) {
	  // Alors oui, je peux prendre ce composant
	  return true;
	}
      } else {
	return true;
      }
    }
  }
  return false;
}

/**
 * c'est pas assez clair le nom de la fonction ? :)
 */
bool puis_je_prendre_produit() {
  // Si je fonctionne en mode NORMAL
  if (bot.mode == NORMAL) {
    // Si le produit nécessite une opération op (en production)
    if ((__anneau->cases[bot.pos]).p.etat >= 0) {
      // Si j'ai de la place pour stocker ce produit
      if (bot.stockProduits[ctoi((__anneau->cases[bot.pos]).p.num) - 1] == 0) {
	// Si je peux travailler sur ce produit
	if (has(bot.prods, (__anneau->cases[bot.pos]).p.num)) {
	  // Si je peux effectuer l'opération nécessaire (op)
	  if (bot.ops[0] == (__anneau->cases[bot.pos]).p.ops[(__anneau->cases[bot.pos]).p.etat]) {
	    // Alors oui, je peux prendre ce produit
	    return true;
	  }
	}
      }
    }
  } else { // Mode Dégradé
    // Si le produit nécessite une opération op (en production)
    if ((__anneau->cases[bot.pos]).p.etat >= 0) {
      // Si j'ai de la place pour stocker ce produit
      if (bot.stockProduits[ctoi((__anneau->cases[bot.pos]).p.num) - 1] == 0) {
	// Si je peux travailler sur ce produit
	if (has(bot.prodsDegrades, (__anneau->cases[bot.pos]).p.num)) {
	  // Si je peux effectuer l'opération nécessaire (op)
	  if (has(bot.ops, (__anneau->cases[bot.pos]).p.ops[(__anneau->cases[bot.pos]).p.etat])) {
	    // Alors oui, je peux prendre ce produit
	    return true;
	  }
	}
      }
    }
  }
  return false;
}

/**
 * c'est pas assez clair le nom de la fonction ? :)
 */
Composant prendre_composant() {
  static Composant c;
  
  c = (__anneau->cases[bot.pos]).c;
  
  (__anneau->cases[bot.pos]).type = VIDE;
  (__anneau->cases[bot.pos]).c.num = 0;
  
  return c;
}

/**
 * c'est pas assez clair le nom de la fonction ? :)
 */
Produit prendre_produit() {
  static Produit p;
  
  p = (__anneau->cases[bot.pos]).p;
  
  (__anneau->cases[bot.pos]).type = VIDE;
  (__anneau->cases[bot.pos]).p.num = 0;
  
  return p;
}

void callback_sigusr1_anneau_tourne(int s) {
  static Composant c;
  static Produit p;  
  static int i;
  static int j = 0;
  
  sprintf(log_curr_pos, "%s", desc_case(&(__anneau->cases[bot.pos])));
  
  sem_wait(__semaphore);
  
  switch ((__anneau->cases[bot.pos]).type) {
    case COMPOSANT:
      sprintf(log_out, " ");
      
      if (puis_je_prendre_composant()) {
	c = prendre_composant();
	
	sprintf(log_in, "%s", desc_composant(&c));
	
	i = ctoi(c.num) - 1;
	p = produits[i];
	
	bot.stockComposants[i]++;
	
	if (bot.stockComposants[i] == p.nbComp) {
	  // Nombre de composants nécessaires atteint
	  bot.stockComposants[i] = 0;
	  
	  // Si je peux réaliser la première opération sur le produit
	  if (has(bot.ops, p.ops[p.etat])) {
	    // J'effectue l'opération
	    sprintf(log, "    opération %c sur P%c", p.ops[p.etat], p.num);
	    p.etat++; // Prochaine étape
	  } else {
	    sprintf(log, "       P%c initialisé", p.num);
	  }
	  
	  bot.stockProduits[i]++;
	  
	  info();
	  
	  produitsStock[i] = p;
	} else {
	  sprintf(log, " ");
	  info();
	}
      } else {
	sprintf(log_in, " ");
	sprintf(log, " ");
	info();
      }
      break;
      
    case PRODUIT:
      p = (__anneau->cases[bot.pos]).p;
      
      if (puis_je_prendre_produit()) {
	// Je peux réaliser l'opération p.ops[p.etat] nécessaire
	
	p = prendre_produit(); // Je prends le produit
	
	sprintf(log_in, "%s", log_curr_pos);
	
	// J'effectue l'opération
	p.etat++; // Prochaine opération
	
	if (p.etat == strlen(p.ops)) {
	  sprintf(log, "opération %c sur P%c => P%c terminé", p.ops[p.etat], p.num, p.num);
	  
	  p.etat = -1; // Produit terminée
	} else {
	  sprintf(log, "    opération %c sur P%c [%d]", p.ops[p.etat], p.num, p.etat);
	}
	
	info();
	
	i = ctoi(p.num) - 1;
	
	produitsStock[i] = p;
	bot.stockProduits[i]++;	
      } else {
	sprintf(log_in, " ");
	info();
      }
      break;
      
    case VIDE:
      if (nb_cases_vides() == ANNEAU_NUM_CASES) {
	for (i = 0; i < NB_PROD; i++) {
	  if ((bot.stockComposants[i] == 1 && produits[i].nbComp > 1) || (bot.stockComposants[i] == 2 && produits[i].nbComp == 3)) {
	    // Je remets le composant i sur l'anneau
	    (__anneau->cases[bot.pos]).c.num = itoc(i + 1);
	    (__anneau->cases[bot.pos]).type = COMPOSANT;
	    bot.stockComposants[i]--;
	    
	    sprintf(log, " pose le C%d sur l'anneau", i+1);
	    break;
	  }
	}
      }
      
      if ((__anneau->cases[bot.pos]).type == VIDE) {
	while (j < NB_PROD) {
	  if (bot.stockProduits[j] == 1) {
	    // je pose le produit j sur la case
	    p = produitsStock[j];
	    
	    sprintf(log, "  pose P%c sur la case %d", p.num, (__anneau->cases[bot.pos]).num);
	    
	    (__anneau->cases[bot.pos]).p = p;
	    (__anneau->cases[bot.pos]).type = PRODUIT;
	    
	    bot.stockProduits[j] = 0;
	    
	    sprintf(log_in, " ");
	    sprintf(log_out, "%s", desc_produit(&p));
	    
	    j++;
	    break;
	  }
	  j++;
	}
	
	if (j >= NB_PROD) {
	  j = 0;
	}
      }
      
      info();
      break;
  }
  
  sem_post(__semaphore);
  
  return;
}

/**
 * Affichage des informations du serveur
 */
void info() {
  static char prods[NB_PROD];
  
  strncpy(prods, bot.prods, NB_PROD);
  
  printf("⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯\n");
  printf("        Robot : %d\n", bot.id);
  printf("          PID : %d\n", (int) bot.pid);
  printf("         Mode : %s\n", (bot.mode == NORMAL ? "NORMAL " : "DÉGRADÉ"));
  printf("   Opérations : N[%c] D[%s]\n", bot.ops[0], bot.ops);
  printf("     Capacité : N[%s] D[%s]\n", prods, bot.prodsDegrades);
  printf("     Position : %d\n\n", bot.pos);
  
  printf("⎬⎯⎯⎯⎯⎯⎯⎯⎯[état in/out]⎯⎯⎯⎯⎯⎯⎯⎯⎨\n");
  printf("       POS %2d : %s\n", bot.pos, log_curr_pos);
  printf("           IN : %s\n", log_in);
  printf("          OUT : %s\n\n", log_out);
  
  printf("⎬⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯[stock]⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎨\n\n");
  printf("                1  2  3  4\n");
  printf("    Composant : %d  %d  %d  %d\n", bot.stockComposants[0], bot.stockComposants[1], bot.stockComposants[2], bot.stockComposants[3]);
  printf("      Produit : %d  %d  %d  %d\n", bot.stockProduits[0], bot.stockProduits[1], bot.stockProduits[2], bot.stockProduits[3]);
  printf("\n");
  
  printf("⎬⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯[usine]⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎨\n");
  printf("  %s\n\n\n", log);
  
  fflush(stdout);
  
  sprintf(log_in, " ");
  sprintf(log_out, " ");
  sprintf(log, " ");
}

