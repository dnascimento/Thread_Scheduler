/*Redblack.h*/
//typedef struct _sthread *sthread_t;

#include <stdio.h>
#include <stdlib.h>

#define TELEM 2
#define black 0
#define red 1



typedef struct NoRB{
	long int chave;		//Criterio de ordenacao. E definido na criacao do no e terá de ter em conta o Item em causa
	struct _sthread* elem;		//Poderá alterar o tipo a ordenar nesta estrutura
	int tid;				//Guardar o tid do processo actual
	
	
	int cor;		//cor pode ser r (red) ou b (black)
	struct NoRB* pai; 	//Mantemos um ponteiro para o pai, custa memoria mas torna-se mais eficiente
	struct NoRB*  esq;
	struct NoRB*  dir;

}*PtNo;


typedef struct ArvoreRB_t{		/*Estrutura que contem a raiz de uma arvore RB e o menor no nela contida*/
	PtNo raiz;
	PtNo nil;
	PtNo minimo;
}*ArvoreRB;


/*Funcoes da arvore redblack*/

/*Criar/Modificar*/

ArvoreRB RBNovaArvore();			//Cria uma arvore
void RBInserirNo(ArvoreRB arvore,long int chav,int tid,struct _sthread* elemento);
void RBRemoverNo(ArvoreRB arvore, PtNo z);//Remove o nó apontado
void RBAlterarChave(ArvoreRB arvore,PtNo no,long int novaChave);//Alterar a chave de "no" para "novaChave"
void RBarvoreDestroy(ArvoreRB arvore); /*Destruir a arvore*/

/*Pesquisa*/
int RBSearchTID(ArvoreRB arvore,int tid);		/*Procurar uma thread na arvore com o mesmo TID da thread recebida*/
long int RBMenorChave(ArvoreRB arvore);	//Devolve o valor da menor chave
/*Acesso*/

struct _sthread * RBExtraiMininimo(ArvoreRB arvore);				/*Devolve a tarefa do no com menor chave*/


PtNo RBRaizArvore(ArvoreRB arvore);	//Devolve a raiz da arvore
PtNo RBMinimoArvore(ArvoreRB arvore);	//Devolve um ponteiro para o menor no,
long int RBChaveNo(PtNo no);			//Devolve a chave do no
struct _sthread *	RBConteudoNo(PtNo no);		//Devolve ponteiro do no

/*Display/Informacao*/
void RBImprimirArvore(ArvoreRB arvore);	//Imprime um esquema da arvore						
int RBArvoreVazia(ArvoreRB arvore);	//Arvore vazia?#define sim 1 /nao  0
