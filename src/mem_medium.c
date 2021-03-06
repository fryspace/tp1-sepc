/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <stdint.h>
#include <assert.h>
#include "mem.h"
#include "mem_internals.h"
#include <stdbool.h>


unsigned int puiss2(unsigned long size) {
    unsigned int p=0;
    size = size -1; // allocation start in 0
    while(size) {  // get the largest bit
	p++;
	size >>= 1;
    }
    if (size > (1 << p))
	p++;
    return p;
}


void * division_block_recursif(unsigned int taille_block){
    /*
    Cette fonction permet de trouver un bloc et de le diviser jusqu'à ce qu'on ait une taille correct.
    */

    /* Si la taille entrée est supérieur à la taille max, on réalloue un bloc de taille max*/
    if (taille_block >= arena.medium_next_exponant + FIRST_ALLOC_MEDIUM_EXPOSANT){
        mem_realloc_medium();
        
    }

    void * ptr = arena.TZL[taille_block];
    if (ptr != NULL){ // On a trouvé un bloc à découper
        arena.TZL[taille_block] = *(void **)ptr; // On déréférence de la liste le bloc ainsi trouvé.
        return ptr; // On retourne le pointeur du bloc à découper 
    }else{
        void * block_a_decouper = division_block_recursif( taille_block + 1); // dans tous les cas un bloc à découper
        void * buddy = (void *)((uintptr_t)block_a_decouper ^ (1 << taille_block)); // On trouve l'adresse de buddy
        arena.TZL[taille_block] = buddy; //Comme il n'y avait pas de bloc de cette taille, buddy devient le seul en présence
        void ** adr_buddy=buddy;
        *adr_buddy = (void *)NULL; //Comme il est seul, on écrit rien à son adresse
        return block_a_decouper; //On récupère le bon pointeur du bloc une fois découpé
    }

}


void *
emalloc_medium(unsigned long size){
    /*
    Fonction principale appelant la précédente. Elle récupère le bloc à alloué et renvoie le pointeur sur la mémoire à écrire
    du bloc alloué en marquant les extrémitées.
    */

    //On vérifie qu'on est bien dans le cas d'une allocation médium
    assert(size < LARGEALLOC);
    assert(size > SMALLALLOC);

    size +=32; //taille entière pour retrouver les valeurs des marques
    unsigned int taille_optimale = puiss2(size);
    void * ptr = division_block_recursif(taille_optimale); //On récupère le bloc à allouer
    return mark_memarea_and_get_user_ptr(ptr, 1 << taille_optimale, MEDIUM_KIND); //On retourne le bloc marqué
}





void remonter_buddy(void * ptr, unsigned int taille_block){
    /*
    Par le même principe, on va récursivement remonter la pile des buddy 
    jusqu'à ne plus en trouver
    */

    void * buddy = (void *)((uintptr_t) ptr ^ (1 << taille_block)); //On récupère l'adresse du buddy

    // On réaliste le parcours de la liste 
    void * traceur = arena.TZL[taille_block];
    void * sent = &traceur;
    while(traceur != NULL){
        void ** suiv_traceur=traceur;
        if(*suiv_traceur == buddy){ //On  trouve le buddy
            *suiv_traceur = *(void**)buddy; //On enlève buddy de la liste et on fait pointer vers le suivant de buddy
            if(ptr > buddy){
                ptr = buddy; // On remet les blocs dans l'ordre
            }
            return remonter_buddy(ptr, taille_block + 1);//On fusionne les blocs et on recherche le prochain buddy
        }
        traceur = *suiv_traceur;//On passe au suivant
    }
    traceur = sent; //On replace le traceur au début de la liste
    //On supprime de la liste des blocks de taille donné le pointeur initiale pour libérer la mémoire.
    *(void **)ptr = arena.TZL[taille_block];
    arena.TZL[taille_block]=ptr;
}



void efree_medium(Alloc a) {
    /*
    De même, on utilise la focntion récursive précédente pour replacer le bloc
    */
   
    unsigned int size_log = puiss2((unsigned long)a.size); //On récupère la taille du block
    remonter_buddy(a.ptr, size_log);//On fusionne jusuqu'à ce qu'on ne trouve plus de buddy
}


