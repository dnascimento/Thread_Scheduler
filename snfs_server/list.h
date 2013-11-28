/**

 */
#ifndef _LIST_REF_
#define _LIST_REF_

/**
 * Estrutura do nodo da lista.
 */
typedef struct sListNode
{
	unsigned ordem;
  ///Apontador para a informação do nodo.
  void *inf;
  ///Apontador para o nodo anterior.
  struct sListNode *prev;
  ///Apontador para o nodo seguinte.
  struct sListNode *next;
}SListNode;

/**
 * Definição do apontador para os nodos da lista.
 */
typedef SListNode* ListNode;

/**
 * Estrutura da lista.
 */
typedef struct sList
{
  ///Número de elementos da lista.
  int size;
  ///Apontador para o início da lista.
  ListNode first;
  ///Apontador para o fim da lista.
  ListNode last;
}SList;

/**
 * Definição da lista.
 */
typedef SList* List;

//##############################################################################

/**
 * Cria uma lista.
 * Se não for possível criar a lista devolve NULL.
 *
 * @return lista inicializada ou NULL.
 */
List newList();

/**
 * Elimina uma lista.
 *
 * @attention apenas liberta a memória referente à estrutura da lista; não
 *            liberta o espaço ocupado pelos elementos nela contidos.
 *
 * @param list lista.
 */
void listDelete(List list);

/**
 * Insere um elemento no início de uma lista.
 * Verifica se é possível inserir o elemento, devolvendo 1 caso não seja
 *   possível.
 *
 * @param list lista.
 * @param inf  endereço do elementos que queremos inserir.
 *
 * @return 0 se o elemento for inserido;\n
 *         1 se não for possível alocar memória para o novo elemento.
 */
int listInsertFst(List list,void* inf);

/**
 * Insere um elemento no fim de uma lista.
 * Verifica se é possível inserir o elemento, devolvendo 1 caso não seja
 *   possível.
 *
 * @param list lista.
 * @param inf  endereço do elemento que queremos inserir.
 *
 * @return 0 se o elemento for inserido;\n
 *         1 se não for possível alocar memória para o novo elemento.
 */
int listInsertLst(List list,void* inf);

/**
 * Insere um elemento numa determinada posição de uma lista.
 * A posição, especificada pelo argumento @a index, tem que estar entre 0 e o
 *   tamanho da lista.
 * O (n+1)-ésimo elemento e todos os seguintes avançam uma posição.
 *
 * @param list  lista.
 * @param index posição em que será inserido.
 * @param inf   endereço do elemento que queremos inserir.
 *
 * @return 0 se o elemento for inserido;\n
 *         1 se o valor de @a index não for válido;\n
 *         2 se não for possível alocar memória para o novo elemento.
 */
int listInsertAt(List list,int index,void* inf);

/**
 * Remove o primeiro elemento de uma lista.
 * Permite devolver a informação do elemento removido, caso o valor de @a inf
 *   seja diferente de NULL.
 * Se a lista estiver vazia é colocado o valor NULL em @a inf.
 *
 * @attention esta função não liberta o espaço ocupado pelo elemento removido.
 *
 * @param list lista.
 * @param inf  endereço onde é colocado o elemento removido (ou NULL).
 *
 * @return 0 se o elemento for removido;\n
 *         1 se a lista estiver vazia.
 */
int listRemoveFst(List list,void** inf);

/**
 * Remove o último elemento de uma lista.
 * Permite devolver a informação do elemento removido, caso o valor de @a inf
 *   seja diferente de NULL.
 * Se a lista estiver vazia é colocado o valor NULL em @a inf.
 *
 * @attention esta função não liberta o espaço ocupado pelo elemento removido.
 *
 * @param list lista.
 * @param inf  endereço onde é colocado o elemento removido (ou NULL).
 *
 * @return 0 se o elemento for removido;\n
 *         1 se a lista estiver vazia.
 */
int listRemoveLst(List list,void** inf);

/**
 * Remove o elemento de uma determinada posição de uma lista. 
 * Permite devolver a informação do elemento removido, caso o valor de @a inf
 *   seja diferente de NULL.
 * Se a lista estiver vazia é colocado o valor NULL em @a inf.\n
 * A posição, especificada pelo argumento @a n, tem que estar entre 0 e o
 *   tamanho da lista menos 1.
 * O n-ésimo elemento e todos os seguintes recuam uma posição.
 *
 * @attention esta função não liberta o espaço ocupado pelo elemento removido.
 *
 * @param list  lista.
 * @param index posição do elemento que queremos remover.
 * @param inf   endereço onde é colocado o elemento removido (ou NULL).
 *
 * @return 0 se o elemento for removido;\n
 *         1 se o valor de @a n não for válido.
 */
int listRemoveAt(List list,int index,void** inf);

/**
 * Verifica qual o primeiro elemento de uma lista.
 * Se a lista estiver vazia é colocado o valor NULL em @a inf.
 *
 * @attention esta função coloca em @a inf o endereço do elemento pretendido;
 *            depois de executar esta função é aconselhável fazer uma cópia da
 *            informação e passar a trabalhar com a cópia para que não haja
 *            problemas de partilha de referências.
 *
 * @param list lista.
 * @param inf  endereço onde é colocado o resultado.
 *
 * @return 0 se a lista não estiver vazia;\n
 *         1 se a lista estiver vazia.
 */
int listFst(List list,void** inf);

/**
 * Verifica qual o último elemento de uma lista.
 * Se a lista estiver vazia é colocado o valor NULL em @a inf.
 *
 * @attention esta função coloca em @a inf o endereço do elemento pretendido;
 *            depois de executar esta função é aconselhável fazer uma cópia da
 *            informação e passar a trabalhar com a cópia para que não haja
 *            problemas de partilha de referências.
 *
 * @param list lista.
 * @param inf  endereço onde é colocado o resultado.
 *
 * @return 0 se a lista não estiver vazia;\n
 *         1 se a lista estiver vazia.
 */
int listLst(List list,void** inf);

/**
 * Verifica qual o elemento numa determinada posição de uma lista.
 * A posição, especificada pelo argumento @a index, tem que estar entre 0 e o
 *   tamanho da lista menos 1. Se isto não acontecer é colocado o valor NULL em
 *   @a inf.
 *
 * @attention esta função coloca em @a inf o endereço do elemento pretendido;
 *            depois de executar esta função é aconselhável fazer uma cópia da
 *            informação e passar a trabalhar com a cópia para que não haja
 *            problemas de partilha de referências.
 *
 * @param list  lista.
 * @param index posição do elemento que procuramos.
 * @param inf   endereço onde será colocado o resultado.
 *
 * @return 0 se o valor de @a index for válido;\n
 *         1 se o valor de @a index não for válido.
 */
int listAt(List list,int index,void** inf);

/**
 * Determina o tamanho de uma lista.
 * Devolve o valor do campo @a size da lista.
 *
 * @param list lista.
 *
 * @return número de elementos da lista.
 */
int listSize(List list);


/**Inserir ordenadamente na lista segundo um inteiro*/
int inserirOrdenado(List list,void* elemento,unsigned short  ordem);



#endif
