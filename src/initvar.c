/*------------------------------------------------------------------------------|
 |                      E N V I O N N E M E N T   C A N A D A                   |
 |                      -------------------------------------                   |
 | Centre Meteorologique Canadien                                               |
 | 2121 route transcanadienne                                                   |
 | Dorval , Quebec                                                              |
 | H9P 1J3                                                                      |
 |                                                                              |
 |                                                                              |
 | Projet   :  Projet ocmi_unification                                          |
 | Fichier  :  initvar.c                                                        |
 | Auteur   :  Francois Patrick Andre                                           |
 | Date     :  7 juillet  2004                                                  |
 |                                                                              |
 |------------------------------------------------------------------------------|
 | Description : module qui gere les variables globales .                       |
 |                                                                              |
 |------------------------------------------------------------------------------|
 | Modification   :                                                             |
 |                                                                              |
 |    Nom         :                                                             |
 |    Date        :                                                             |
 |    Description :                                                             |
 -------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

int socks = 0;

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  SetInteger       ()                                              |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (23 oct 2001)                           |
|                                                                                  |
|  Description :  Initialise un entier                                             |
|                                                                                  |
|  Parametre(s): car    : Caractere indiquant l'entier a initialise                |
|                entier : l'entier                                                 |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/

void SetInteger   (int entier , char car) {

  switch  (car) {
     case 's' :  if (socks) socks = 0;
                   socks  = entier      ; break ;
  }
}
/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  GetInteger       ()                                              |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (23 oct 2001)                           |
|                                                                                  |
|  Description :  Retourne un entier                                               |
|                                                                                  |
|  Parametre(s):  car : Le caratere indiquant l'entier  a retourner                |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/

int  GetInteger   (char car) {
 int  entier ;

  switch  (car) {
     case 's' :  entier = socks ; break ;

  }
  return entier ;
}

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  GetChaine        ()                                              |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (23 oct 2001)                           |
|                                                                                  |
|  Description :  retourne une chaine de caractere                                 |
|                                                                                  |
|  Parametre(s):  type : la chaine a retourner                                     |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/

char *GetChaine   (char car) {
  char *Rep     = "/logs";
  char *Firtmp  = "/logs/firtmp";
  char *Firsin  = "/logs/firsin";
  char *chaine  = NULL;
  
  switch  (car) {
     case   'R'  : chaine = Rep        ; break ;
     case   'T'  : chaine = Firtmp     ; break ;
     case   'F'  : chaine = Firsin     ; break ;
  }
  return chaine ;
} 

