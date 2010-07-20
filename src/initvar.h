/*------------------------------------------------------------------------------|
 |                      E N V I O N N E M E N T   C A N A D A                   |
 |                      -------------------------------------                   |
 | Centre Meteorologique Canadien                                               |
 | 2121 route transcanadienne                                                   |
 | Dorval , Quebec                                                              |
 | H9P 1J3                                                                      |
 |                                                                              |
 |                                                                              |
 | Projet   :  Projet d'affichage image sur le web                              |
 | Fichier  :  initvar.h                                                        |
 | Auteur   :  Francois Patrick Andre                                           |
 | Date     :  23 oct 2001                                                      |
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
#ifndef INITVAR_H
#define INITVAR_H

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  SetChaine        ()                                              |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (23 oct 2001)                           |
|                                                                                  |
|  Description :  Initialise une chaine de caractere                               |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern void SetChaine   (char * , char ) ; 

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  GetChaine        ()                                              |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (23 oct 2001)                           |
|                                                                                  |
|  Description :  retourne une chaine de caractere                                 |
\*--------------------------------------------------------------------------------*/
extern char *GetChaine   (char ) ;
/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  SetInteger       ()                                              |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (23 oct 2001)                           |
|                                                                                  |
|  Description :  Initialise un entier                                             |
\*--------------------------------------------------------------------------------*/
extern void SetInteger   (int , char ) ;
/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  GetInteger       ()                                              |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (23 oct 2001)                           |
|                                                                                  |
|  Description :  Retourne un entier                                               |
\*--------------------------------------------------------------------------------*/
extern int  GetInteger   (char ) ;

#endif

