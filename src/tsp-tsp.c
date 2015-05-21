#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#include "tsp-types.h"
#include "tsp-genmap.h"
#include "tsp-print.h"
#include "tsp-tsp.h"
#include "tsp-lp.h"
#include "tsp-hkbound.h"

/* dernier minimum trouvé */
int minimum;

/* résolution du problème du voyageur de commerce */
int present (int city, int hops, tsp_path_t path, uint64_t vpres)
{
  (void) hops;
  (void) path;
  return (vpres & (1<<city)) != 0;
}

void tsp (int hops, int len, uint64_t vpres, tsp_path_t path, long long int *cuts, tsp_path_t sol, int *sol_len)
{
    if (len + cutprefix[(nb_towns-hops)] >= *sol_len) {
      (*cuts)++ ;
      return;
    }

    /* calcul de l'arbre couvrant comme borne inférieure */
    if ((nb_towns - hops) > 6 &&
	lower_bound_using_hk(path, hops, len, vpres) >= *sol_len) {
      (*cuts)++;
      return;
    }


    /* un rayon de coupure à 15, pour ne pas lancer la programmation
       linéaire pour les petits arbres, plus rapide à calculer sans */
    if ((nb_towns - hops) > 22
	&& lower_bound_using_lp(path, hops, len, vpres) >= *sol_len) {
      (*cuts)++;
      return;
    }

  
    if (hops == nb_towns) {
	    int me = path [hops - 1];
	    int dist = tsp_distance[me][0]; // retourner en 0
            if ( len + dist < *sol_len ) {
		    *sol_len = len + dist;
		    memcpy(sol, path, nb_towns*sizeof(int));
		    if (!quiet)
		      print_solution (path, len+dist);
	    }
    } else {
        int me = path [hops - 1];        
        for (int i = 0; i < nb_towns; i++) {
	  if (!present (i, hops, path, vpres)) {
                path[hops] = i;
		vpres |= (1<<i);
                int dist = tsp_distance[me][i];
                tsp (hops + 1, len + dist, vpres, path, cuts, sol, sol_len);
		vpres &= (~(1<<i));
            }
        }
    }
}

