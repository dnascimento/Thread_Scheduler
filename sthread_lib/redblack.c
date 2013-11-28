/* Implementacao da estrutura ordenada Arvore RedBlack */

#define sim 1
#define nao  0
#include "redblack.h"


/*Prototipos*/
void RodarEsquerda(ArvoreRB arvore,PtNo x);
void RodarDireita(ArvoreRB arvore,PtNo x);
void Assert(int assertion, char* error);

int comparar(long int A,long int B);


PtNo CriarNo(long int chave,struct _sthread* elemento,int tid,int cor,PtNo esquerdo,PtNo direito,PtNo pai){			/*Alocar um no*/
	PtNo No;
	
	if(!(cor == red || cor == black)){	/*Cor invalida*/
		printf("CriarNo: Cor invalida\n\n");
		exit(-1);
	}
	
	if((No = (PtNo) malloc(sizeof(struct NoRB))) == NULL){
		printf("CriarNo: Falhou alocacao de memoria PtNo\n\n");
		exit(-1);}
		
	No->elem = elemento; /*Guardar informacao*/	
	
	No->tid = tid;
	No->chave = chave;
	No->cor = cor;
	No->esq = esquerdo;
	No->dir = direito;
	No->pai = pai;
	
	return No;
}



void DestruirNo(PtNo* no){
	free(*no);			/*Libertar do no*/
	*no = NULL;
}


/************************************************************************/
ArvoreRB RBNovaArvore(){
	ArvoreRB novaArvore;
	PtNo nilo;
	
	if((novaArvore = (ArvoreRB) malloc(sizeof(struct ArvoreRB_t))) == NULL){
		printf("Nova Arvore: Falhou alocacao");
		exit(-1);
	}
	
	
	nilo = novaArvore->nil = CriarNo(-1,NULL,-1,black,NULL,NULL,NULL);
	nilo->pai = nilo->esq = nilo->dir = nilo;

	novaArvore->raiz = CriarNo(-1,NULL,-1,black,nilo,nilo,nilo);
	novaArvore->minimo = NULL;
	return(novaArvore);
}

	/*Devolver a raiz*/

PtNo RBRaizArvore(ArvoreRB arvore){
	return arvore->raiz->esq;
}


	/*Rodar a esquerda*/

void RodarEsquerda(ArvoreRB arvore,PtNo x){
	PtNo y;
	PtNo nil = arvore->nil;
	
	y = x->dir;
	x->dir = y->esq;
	

	if (y->esq != nil)
		y->esq->pai=x;
		 
	y->pai=x->pai;   

  
	if( x == x->pai->esq)
		x->pai->esq=y;
	else 
		x->pai->dir=y;
		
	y->esq=x;
	x->pai=y;
	
#ifdef DEBUG_ASSERT
  Assert(!arvore->nil->cor,"Nil nao vermelho a rodarEsquerda");
#endif
}

void RodarDireita(ArvoreRB arvore,PtNo y){
  PtNo x;
  PtNo nil = arvore->nil;
  
   x=y->esq;
	y->esq=x->dir;

	if (nil != x->dir)  
		x->dir->pai=y; 
  

	x->pai=y->pai;
  
  
	if( y == y->pai->esq)
		y->pai->esq=x;
	else
		y->pai->dir=x;
  
	x->dir=y;
	y->pai=x;
	
	
#ifdef DEBUG_ASSERT
  Assert(!arvore->nil->cor,"Nil nao vermelho a RodarDireita");
#endif
}




void InserirNoAux(ArvoreRB arvore,PtNo z) {
  /*  Esta funcao e chamada pelo InserirNo*/
  PtNo x,y;
  PtNo nil = arvore->nil;
  
  z->esq = z->dir = nil;	/*inicialmente o no aponta para nil*/
 
  y = arvore->raiz;
  
  x = arvore->raiz->esq;
  
  
  while( x != nil) {
    y = x;
    if (comparar(x->chave,z->chave) == 1) /* x.chave >= z.chave    */
      x = x->esq;
    else /* x,chave <= z.chave */
      x = x->dir;
  }
  
  z->pai = y;
  
  if ((y == arvore->raiz) ||			/*Se o pai for raiz ou maior que o filho*/
       (comparar(y->chave,z->chave) == 1))  /* colocar filho à esquerda */
		y->esq = z;
  else 
    y->dir=z;


#ifdef DEBUG_ASSERT
  Assert(!arvore->nil->cor,"Nil nao vermelho no InsereNoAux");
#endif
}



void RBInserirNo(ArvoreRB arvore,long int chav,int tid,struct _sthread* elemento){
	PtNo y,x,novoNo;
	long int chave = chav;
	
	x = CriarNo(chave,elemento,tid,red,NULL,NULL,NULL);

	InserirNoAux(arvore,x);
	
	novoNo = x;		/*Se quisermos retornar o no*/
	
	while(x->pai->cor == red) { 			/*Enquanto o pai tiver cor vermelha*/
		if (x->pai == x->pai->pai->esq) {
			y = x->pai->pai->dir;
			
			if (y->cor == red) {
				x->pai->cor = black;
				y->cor = black;
				x->pai->pai->cor = red;
				x = x->pai->pai;
			}
			else {
				if (x == x->pai->dir) {
					x = x->pai;
					RodarEsquerda(arvore,x);
				}
			
				x->pai->cor = black;
				x->pai->pai->cor = red;
				RodarDireita(arvore,x->pai->pai);
			} 
      	
    } 
    else { 					/* se x->pai == x->pai->pai->dir */
		y = x->pai->pai->esq;
		
		if (y->cor == red) {
			x->pai->cor = black;
			y->cor = black;
			x->pai->pai->cor = red;
			x = x->pai->pai;
		} 
      else {
			if (x == x->pai->esq) {
				x = x->pai;
				RodarDireita(arvore,x);
			}
			
		x->pai->cor = black;
		x->pai->pai->cor = red;
		RodarEsquerda(arvore,x->pai->pai);
      } 
    }
  }
  
  arvore->raiz->esq->cor = black;
  
  /*Condicoes para actualizar o minimo:
   * 	nao exitir = NULL
   * 	ser menor ou o minimo estar no nill*/
  
  if(arvore->minimo == NULL || (comparar(arvore->minimo->chave,novoNo->chave) == 1))/*minimo>chave*/
	arvore->minimo = novoNo;
	

#ifdef DEBUG_ASSERT
  Assert(!(arvore->nil->cor == red),"Nil vermelho no InserirNo");
  Assert(!arvore->raiz->cor == red,"Raiz vermelho no InserirNo");
#endif
}

	/*Sucessor arvore*/
 
PtNo Sucessor(ArvoreRB arvore,PtNo x) { 
  PtNo y;
  PtNo nil=arvore->nil;
  PtNo raiz=arvore->raiz;

  if (nil != (y = x->dir)) { /* assignment to y is intentional */
    while(y->esq != nil) { /* returns the minium of the dir subarvore of x */
      y=y->esq;
    }
    return(y);
  } else {
    y=x->pai;
    while(x == y->dir) { /* sentinel used instead of checking for nil */
      x=y;
      y=y->pai;
    }
    if (y == raiz) return(nil);
    return(y);
  }
}
	 
	 
	/*Predecessor*/

PtNo Predecessor(ArvoreRB arvore, PtNo x) {
	PtNo y;
	PtNo nil=arvore->nil;
	PtNo raiz=arvore->raiz;

	if (nil != (y = x->esq)) {
		while(y->dir != nil) { /* maximo da subarvore esquerda*/
			y=y->dir;
		}
		return(y);
    } 
    
	else {
		y = x->pai;
		while(x == y->esq) { 
			if (y == raiz) 
				return(nil); 
	
			x =y ;
			y = y->pai;
		}
   return(y);
  }
}

void DestroiArvoreAux(ArvoreRB arvore, PtNo x) {
	/*Invocada pelo DestroiArvore*/
  PtNo nil = arvore->nil;
  if (x != nil) {
    DestroiArvoreAux(arvore,x->esq);
    DestroiArvoreAux(arvore,x->dir);
    DestruirNo(&x);
  }
}

void RBarvoreDestroy(ArvoreRB arvore) {
  DestroiArvoreAux(arvore,arvore->raiz->esq);
  free(arvore->raiz);
  free(arvore->nil);
  free(arvore);
}


void RepararEliminado(ArvoreRB arvore, PtNo x) {
  PtNo raiz=arvore->raiz->esq;
  PtNo w;

  while( (!x->cor) && (raiz != x)) {
    if (x == x->pai->esq) {
      w=x->pai->dir;
      if (w->cor) {
	w->cor=0;
	x->pai->cor=1;
	RodarEsquerda(arvore,x->pai);
	w=x->pai->dir;
      }
      if ( (!w->dir->cor) && (!w->esq->cor) ) { 
	w->cor=1;
	x=x->pai;
      } else {
	if (!w->dir->cor) {
	  w->esq->cor=0;
	  w->cor=1;
	  RodarDireita(arvore,w);
	  w=x->pai->dir;
	}
	w->cor=x->pai->cor;
	x->pai->cor=0;
	w->dir->cor=0;
	RodarEsquerda(arvore,x->pai);
	x=raiz; /* this is to exit while loop */
      }
    } else { /* the code below is has esq and dir switched from above */
      w=x->pai->esq;
      if (w->cor) {
	w->cor=0;
	x->pai->cor=1;
	RodarDireita(arvore,x->pai);
	w=x->pai->esq;
      }
      if ( (!w->dir->cor) && (!w->esq->cor) ) { 
	w->cor=1;
	x=x->pai;
      } else {
	if (!w->esq->cor) {
	  w->dir->cor=0;
	  w->cor=1;
		RodarEsquerda(arvore,w);
	  w=x->pai->esq;
	}
	w->cor=x->pai->cor;
	x->pai->cor=0;
	w->esq->cor=0;
	RodarDireita(arvore,x->pai);
	x=raiz; /* this is to exit while loop */
      }
    }
  }
  x->cor=0;

#ifdef DEBUG_ASSERT
  Assert(!arvore->nil->cor,"nil not black in RBDeleteFixUp");
#endif
}



void RBRemoverNo(ArvoreRB arvore, PtNo z){
  PtNo x,y;
  PtNo  nil=arvore->nil;
  PtNo  raiz=arvore->raiz;
  
	
  y= ((z->esq == nil) || (z->dir == nil)) ? z : Sucessor(arvore,z);
  x= (y->esq == nil) ? y->dir : y->esq;
  if (raiz == (x->pai = y->pai)) { /* assignment of y->p to x->p is intentional */
    raiz->esq=x;
  } else {
    if (y == y->pai->esq) {
      y->pai->esq=x;
    } else {
      y->pai->dir=x;
    }
  }
  if (y != z) { /* y should not be nil in this case */

#ifdef DEBUG_ASSERT
    Assert( (y!=arvore->nil),"y is nil in RBDelete\n");
#endif
    /* y is the node to splice out and x is its child */

    if (!(y->cor)) RepararEliminado(arvore,x);
  
    free(z->elem);
    
    y->esq = z->esq;
    y->dir = z->dir;
    y->pai = z->pai;
    y->cor = z->cor;
    z->esq->pai = z->dir->pai=y;
    
    if (z == z->pai->esq) {
      z->pai->esq=y; 
    } else {
      z->pai->dir=y;
    }
	if(arvore->minimo == z){	/*se vamos apagar o no minimo, actualizamos*/
		if(z->dir != nil)		/*Filho direito e o menor de todos*/
			arvore->minimo = z->dir;
		else /*Se nao tiver filho dir*/
			arvore->minimo = z->pai;
	}   
					 
    DestruirNo(&z);
  } else {
	free(y->elem);

    if (!(y->cor)) RepararEliminado(arvore,x);
	
	if(arvore->minimo == y){	/*se vamos apagar o no minimo, actualizamos*/
		if(y->dir != nil)		/*Filho direito e o menor de todos*/
			arvore->minimo = y->dir;
		else 
			arvore->minimo = y->pai;
	}   
											
	
    DestruirNo(&y);
  }
  
#ifdef DEBUG_ASSERT
  Assert(!arvore->nil->cor,"nil not black in RBDelete");
#endif
}


	


	/*Imprimir a arvore*/
void ImprimirArvoreAux(PtNo no,PtNo nil, int identacao){
	int i;
	if(no == NULL)
		return;
	if(no->dir != nil)
		ImprimirArvoreAux(no->dir,nil,(identacao + 6));
		
	for(i=0; i<identacao; i++)
		printf(" ");
		
	if(no->cor == black)
		printf("%ld \n",no->chave);
		
	if(no->cor != black)
		printf("< %ld > \n",no->chave);
	if(no->esq != nil){
		ImprimirArvoreAux(no->esq,nil,(identacao + 6));
	}
}
	
void RBImprimirArvore(ArvoreRB arvore){
	if(arvore->raiz->esq == arvore->nil)
		printf("Arvore Vazia\n");
	else
		ImprimirArvoreAux(arvore->raiz->esq,arvore->nil,0);
}

	/*Pesquisa na arvore pela chave q*/
  
PtNo PesquisaRB(ArvoreRB arvore,long int q) {
	PtNo x = arvore->raiz->esq;
	PtNo nil = arvore->nil;
	
	int compVal;
	
	if (x == nil) return(0);
		compVal = comparar(x->chave,q);
	while(0 != compVal) {/*assignemnt*/
		if (1 == compVal)
			x=x->esq;
		else
		x=x->dir;
		if ( x == nil) 
			return(0);
		
		compVal = comparar(x->chave,q);
	}
	return(x);
}

long int RBChaveNo(PtNo no){
	return no->chave;
}

struct _sthread*	RBConteudoNo(PtNo no){
	return no->elem;
}

PtNo RBMinimoArvore(ArvoreRB arvore){
	return arvore->minimo;
}


int RBArvoreVazia(ArvoreRB arvore){
	if(arvore->raiz->esq  == arvore->nil)
		return sim;
	else
		return nao;
}


struct _sthread * RBExtraiMininimo(ArvoreRB arvore){
	PtNo minimo = RBMinimoArvore(arvore);	//Devolve um ponteiro para o menor no
	struct _sthread * thread;
	if(minimo != NULL){
		thread = minimo->elem;	//Extrair a thread
		minimo->elem = NULL;
		RBRemoverNo(arvore, minimo);//Remove o nó da thread
		return thread;
	}
	else
		return NULL;
}



int RBSearchTIDAUX(PtNo no,PtNo nil,int TID){
	if(no == NULL || no == nil)
		return 0;
		
	if(no->tid == TID)
		return 1;	/*Nem procura mais*/
		
	return (RBSearchTIDAUX(no->esq,nil,TID) || RBSearchTIDAUX(no->dir,nil,TID));	/*Percorre todos os nos*/
}

/*Procurar uma thread com o mesmo TID, retornar 1 se encontrar,0 se nao encontrar*/
int RBSearchTID(ArvoreRB arvore,int TID){
	PtNo raiz = arvore->raiz->esq;
	PtNo nil = arvore->nil;
	
	return RBSearchTIDAUX(raiz,nil,TID);
}


/*Devolve o valor da menor chave da arvore*/
long int RBMenorChave(ArvoreRB arvore){
	PtNo noMinimo = RBMinimoArvore(arvore);
	if(noMinimo == NULL)
		return 0;
	
	return noMinimo->chave;
}

/*Sendo a remocao e a insersao tao rapidos, torna-se mais rapido mover
 *  o conteudo para um novo no com uma nova chave e remover o no antigo*/
void RBAlterarChave(ArvoreRB arvore,PtNo no,long int novaChave){
	struct _sthread* elemento =  no->elem;		/*vamos guardar uma referencia para o conteudo do no a eliminar*/
	no->elem = NULL;					 /*apagar a estrutura referenciada*/
	RBRemoverNo(arvore,no);
	RBInserirNo(arvore,novaChave,no->tid,elemento);
}

int comparar(long int A,long int B){
	if(A > B || A == -1)
		return 1;
	if(A == B)
		return 0;
	return -1;
}

void Assert(int assertion, char* error) {
  if(!assertion) {
    printf("Assertion Failed: %s\n",error);
    exit(-1);
  }
}






