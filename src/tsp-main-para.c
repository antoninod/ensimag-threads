#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <assert.h>
#include <complex.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#include "tsp-types.h"
#include "tsp-job.h"
#include "tsp-genmap.h"
#include "tsp-print.h"
#include "tsp-tsp.h"
#include "tsp-lp.h"
#include "tsp-hkbound.h"

void *lancement_tsp(void* arg);

pthread_mutex_t mutex_queue;
pthread_mutex_t mutex_minimum;



/* macro de mesure de temps, retourne une valeur en nanosecondes */
#define TIME_DIFF(t1, t2) \
  ((t2.tv_sec - t1.tv_sec) * 1000000000ll + (long long int) (t2.tv_nsec - t1.tv_nsec))

#define NB_LECTURE_MAX 5


/* tableau des distances */
tsp_distance_matrix_t tsp_distance ={};

/** Param�tres **/

/* nombre de villes */
int nb_towns=10;
/* graine */
long int myseed= 0;
/* nombre de threads */
int nb_threads=1;

/* affichage SVG */
bool affiche_sol= false;
bool affiche_progress=false;
bool quiet=false;
    
//a prot�ger
tsp_path_t sol;
long long int cuts = 0;
struct tsp_queue q;



static void generate_tsp_jobs (struct tsp_queue *q, int hops, int len, uint64_t vpres, tsp_path_t path, long long int *cuts, tsp_path_t sol, int *sol_len, int depth)
{
    if (len >= minimum) {
        (*cuts)++ ;
        return;
    }
    
    if (hops == depth) {
        /* On enregistre du travail � faire plus tard... */
      add_job (q, path, hops, len, vpres);
    } else {
        int me = path [hops - 1];        
        for (int i = 0; i < nb_towns; i++) {
	  if (!present (i, hops, path, vpres)) {
                path[hops] = i;
		vpres |= (1<<i);
                int dist = tsp_distance[me][i];
                generate_tsp_jobs (q, hops + 1, len + dist, vpres, path, cuts, sol, sol_len, depth);
		vpres &= (~(1<<i));
            }
        }
    }
}

static void usage(const char *name) {
  fprintf (stderr, "Usage: %s [-s] <ncities> <seed> <nthreads>\n", name);
  exit (-1);
}

int main (int argc, char **argv)
{
    int sol_len;
    unsigned long long perf;
    tsp_path_t path;
    uint64_t vpres=0;

    struct timespec t1, t2;

    /* lire les arguments */
    int opt;
    while ((opt = getopt(argc, argv, "spq")) != -1) {
      switch (opt) {
      case 's':
	affiche_sol = true;
	break;
      case 'p':
	affiche_progress = true;
	break;
      case 'q':
	quiet = true;
	break;
      default:
	usage(argv[0]);
	break;
      }
    }

    if (optind != argc-3)
      usage(argv[0]);

    nb_towns = atoi(argv[optind]);
    myseed = atol(argv[optind+1]);
    nb_threads = atoi(argv[optind+2]);
    assert(nb_towns > 0);
    assert(nb_threads > 0);
   
    minimum = INT_MAX;
      
    /* generer la carte et la matrice de distance */
    if (! quiet)
      fprintf (stderr, "ncities = %3d\n", nb_towns);
    genmap ();

    init_queue (&q);

    clock_gettime (CLOCK_REALTIME, &t1);

    memset (path, -1, MAX_TOWNS * sizeof (int));
    path[0] = 0;
    vpres=1;

    /* mettre les travaux dans la file d'attente */
    generate_tsp_jobs (&q, 1, 0, vpres, path, &cuts, sol, & sol_len, 3);
    no_more_jobs (&q);
   
   
    /* calculer chacun des travaux */



    //mise du tableau a +infini
 //   memset (solution, -1, MAX_TOWNS * sizeof (int));

 //   solution[0] = 0;


    pthread_mutex_init(&mutex_queue,NULL);
    pthread_mutex_init(&mutex_minimum,NULL);

    pthread_t* tableau_thread = (pthread_t*)malloc(nb_threads * sizeof(pthread_t));

    for(int i =0; i < nb_threads; i++)
    {
                pthread_create(&(tableau_thread[i]),NULL,lancement_tsp, NULL);
    }

    for(int i =0; i < nb_threads; i++)
    {
            pthread_join(tableau_thread[i], NULL);
    }

    sol_len = minimum;

    clock_gettime (CLOCK_REALTIME, &t2);

    if (affiche_sol)
      print_solution_svg (sol, sol_len);

    perf = TIME_DIFF (t1,t2);
    printf("<!-- # = %d seed = %ld len = %d threads = %d time = %lld.%03lld ms ( %lld coupures ) -->\n",
	   nb_towns, myseed, sol_len, nb_threads,
	   perf/1000000ll, perf%1000000ll, cuts);

    return 0 ;
}

void *lancement_tsp(void* arg)
{
        tsp_path_t meilleure_sol_local;
        tsp_path_t solution;


        int mini_local = minimum;
        long long int cuts_local = 0;

        int nb_lecture = 0;

        while (1)
        {
                int hops = 0, len = 0;
                uint64_t vpres = 1;

                //DEBUT SECTION CRITIQUE
                pthread_mutex_lock(&mutex_queue);

                if(empty_queue(&q))
                {
                        pthread_mutex_unlock(&mutex_queue);        
                        break;
                }
                        

                                
                get_job(&q, solution, &hops, &len, &vpres);

                pthread_mutex_unlock(&mutex_queue);
                //FIN SECTION CRITIQUE

	        // le noeud est moins bon que la solution courante
	        if (mini_local < INT_MAX
	                && (nb_towns - hops) > 10
	                && ( (lower_bound_using_hk(solution, hops, len, vpres)) >= mini_local
		                || (lower_bound_using_lp(solution, hops, len, vpres)) >= mini_local)
	        ) continue;

	        tsp (hops, len, vpres, solution, &cuts_local, meilleure_sol_local, &mini_local);
                nb_lecture++;

               /* if(nb_lecture >= NB_LECTURE_MAX)
                {*/
                        pthread_mutex_lock(&mutex_minimum);

                        if(mini_local < minimum)
                        {
                                minimum = mini_local;
                                //memcpy(sol, meilleure_sol_local, nb_towns*sizeof(int));
                        }

                        mini_local = minimum;
                        //memcpy(meilleure_sol_local, sol, nb_towns*sizeof(int));

                        pthread_mutex_unlock(&mutex_minimum);

                        nb_lecture =0;
               /* }*/
        }

        pthread_mutex_lock(&mutex_minimum);

        if(mini_local <= minimum)
        {
            minimum = mini_local;
            memcpy(sol, meilleure_sol_local, nb_towns*sizeof(int));
        }
        
        cuts += cuts_local;

        pthread_mutex_unlock(&mutex_minimum);

        return NULL;
         
}
