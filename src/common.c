#include "common.h"

/**
 * Global vars: définis dans le fichier common.h
 * @var pid_t __pid			PID du processus
 * @var int __shmid			ID de mémoire partagée
 * @var Anneau *__anneau		Anneau partagée dans la mémoire partagée
 * @var sem_t *__semaphore		Sémaphore de synchronisation de l'anneau
 */

// // // // // // // //
// Shared functions  //
// // // // // // // // 

// Integer to Character
char itoc(int i) {
  static char c[1];
  sprintf(c, "%d", i);
  return c[0];
}

// Integer to Character
int ctoi(char c) {
  static char s[1];
  sprintf(s, "%c", c);
  return atoi(s);
}

/**
 * Description d'un composant
 */
char* desc_composant(Composant *c) {
  static char *str;
  
  str = malloc(255 * sizeof(char));
  sprintf(str, "C%c", c->num);
  return str;
}

/**
 * Description d'un produit
 */
char* desc_produit(Produit *p) {
  static char *str;
  
  str = malloc(100*sizeof(char));
  
  if (p->etat == -1) {
    sprintf(str, "P%c terminé", p->num);
  } else {
    sprintf(str, "P%c attente Op%c", p->num, p->ops[p->etat]);
  }
  return str;
}

/**
 * Description d'une case
 */
char* desc_case(Case *c) {
  if (c->type == VIDE) {
    return "VIDE";
  }
  return (c->type == COMPOSANT) ? desc_composant(&(c->c)) : desc_produit(&(c->p));
}

int nb_cases_vides() {
  static int i, num;
  num = 0;
  
  for (i = 0; i < ANNEAU_NUM_CASES; i++) {
    if (__anneau->cases[i].type == VIDE) {
      num++;
    }
  }
  
  return num;
}

Produit* init_produits() {
  Produit *produits = (Produit *) malloc(sizeof(Produit) * NB_PROD);
  
  produits[0].num    = '1';
  produits[0].nbComp = 3;
  sprintf(produits[0].ops, "1235");
  produits[0].etat   = 0;
  
  produits[1].num    = '2';
  produits[1].nbComp = 3;
  sprintf(produits[1].ops, "2416");
  produits[1].etat   = 0;
  
  produits[2].num    = '3';
  produits[2].nbComp = 1;
  sprintf(produits[2].ops, "13513");
  produits[2].etat   = 0;
  
  produits[3].num    = '4';
  produits[3].nbComp = 2;
  sprintf(produits[3].ops, "461");
  produits[3].etat   = 0;
  
  return produits;
}

/**
 * Marque la fin de l'exécution
 */
void __end_process() {
  printf("== Fin de l'exécution ==\n");
  printf("========================\n\n");
}

/**
 * Levée d'une exception
 */
int __raise (int exitCode, const char *format, ...) {
  va_list arg;
  int done;

  va_start (arg, format);
  done = vfprintf (stderr, format, arg);
  va_end (arg);

  fprintf(stderr, "\n==\n== Exécution avortée avec le code d'erreur %d\n", exitCode);
  __end_process();
  
  exit(exitCode);
  return done;
}