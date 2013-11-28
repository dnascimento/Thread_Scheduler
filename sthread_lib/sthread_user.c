/* Simplethreads Instructional Thread Package
 * 
 * sthread_user.c - Implements the sthread API using user-level threads.
 *
 * 		
 *
 * Change Log:
 * Grupo26
 * 2010-10-18 - Inicio
 * 2010-10-28 - Fim
 * 
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include <sthread.h>
#include <sthread_user.h>
#include <sthread_ctx.h>
#include <sthread_time_slice.h>
#include <sthread_user.h>
#include "queue.h"
#include "redblack.h"
#include <sthread_ctx.h>


#define dprintf if(0) printf


struct _sthread {
  sthread_ctx_t *saved_ctx;					/*Pilha de contexto */
  sthread_start_func_t start_routine_ptr;	/*Programa a correr*/
  long int wake_time;						/*Clock em que vai acordar*/
  int join_tid;								/*TID da tarefa que fizemos  join*/
  void* join_ret;							/*Se != NULL, ja fizemos join. Fica com o  *value_ptr  do join*/
  void* args;								/*Argumentos do programa*/
  int tid;          						/* meramente informativo */
  
											/**PARAMETROS NOVOS*/
  long int vruntime;						/*Tempo em processador*/
  long int exectime;						/*Tempo de execuçao do processo*/
  int priority;								/*Prioridade*/
  int nice;									/*Nice*/
    
  long int times_runned;					/*nº de tiks que o processo ja executou*/
  
  long int waittime;						/*Tempo que esteve a espera na arvore*/
  long int sleeptime;						/*Tempo que esteve em bloqueado*/
};

       
static ArvoreRB exe_thr_arvore_rb;		/* Red-black tree de threads executaveis */
static queue_t *dead_thr_list;         	/* lista de threads "mortas" */
static ArvoreRB sleep_thr_arvore_rb;	/*Red-black tree de threads em sleep*/
static queue_t * join_thr_list;			/*Lista de joins, guarda os pais que ainda tem filhos vivos e estao a espera que eles fiquem zombie*/
static queue_t * zombie_thr_list;		/*Lista de tarefas zombie, fizeram exit mas ainda nao acabaram. Onde os pais vao procurar os filhos mortos*/

static queue_t* mutex_list;
static queue_t* monitor_list;
						
static struct _sthread *active_thr;   	/* thread activa */
static int tid_gen;                   	/* gerador de tid's */

#define min_delay 5
#define CLOCK_TICK 10000				/*Periodo do time_slicer*/
static unsigned long int Clock;
		
static int mutex_id_gen = 0;			/*Gerar os id's para mutex's*/
static int monitor_id_gen = 0;			/*Gerar os id's para monitores*/

/* INTERFACE COM A ARVORE REDBLACK */
void RBInserir(ArvoreRB arvore,struct _sthread* thread){
	RBInserirNo(arvore,thread->vruntime,thread->tid,thread);
}



/*Assinaturas de funcoes auxiliares*/
void actualizarWaittime(ArvoreRB arvore_executaveis,PtNo h);
void acordarProcessos(ArvoreRB arvore_sleep,PtNo h);
void actualizarJoinTime(queue_t* join_list);
void actualizarSleepTimeLista(queue_t* lista_threads);
void actualizarMutex(queue_t* lista_mutex);
void actualizarMonitores(queue_t* lista_Monitores);
void sthread_user_dump();

/*********************************************************************/
/* Part 1: Creating and Scheduling Threads                           */
/*********************************************************************/


void sthread_user_free(struct _sthread *thread);	

void sthread_aux_start(void){						/*Auxiliar para criar uma thread, invocada em sthread_user_create*/
  splx(LOW);										/*Reactivar interrupções*/
  active_thr->start_routine_ptr(active_thr->args);	/*Torna a rotina criada como rotina activa*/
  sthread_user_exit((void*)0);
}

void sthread_user_dispatcher(void);					/*Declaracao do dispatcher, definido mais abaixo*/


/*Inicia o processo de escalonamento invocando o sthread_time_slices_init, lancando um signal periodico cujo tratamento inclui o algoritmo de despaxo*/


void sthread_user_init(void) {						/*Iniciar o sistema de tarefas*/
  exe_thr_arvore_rb = RBNovaArvore();
  dead_thr_list = create_queue();					/*Criar as filas necessarias, consultar topo do documento*/
  sleep_thr_arvore_rb = RBNovaArvore();
  join_thr_list = create_queue();
  zombie_thr_list = create_queue();
  monitor_list = create_queue();
  mutex_list = create_queue();
  tid_gen = 1;							

  struct _sthread *main_thread = malloc(sizeof(struct _sthread));		/*Alocar a estrutura para a main_thread, a base*/
  main_thread->start_routine_ptr = NULL;								/*Ver comentarios do sthread_user_create */
  main_thread->args = NULL;
  main_thread->saved_ctx = sthread_new_blank_ctx();

  main_thread->join_tid = 0;
  main_thread->join_ret = NULL;
  main_thread->tid = tid_gen++;
  main_thread->vruntime = 0; 
  main_thread->exectime = 0;
  main_thread->priority = 1;
  main_thread->nice = 0;
  main_thread->wake_time = 0;
   
  main_thread->sleeptime = 0;
  
  main_thread->times_runned = 0;
   
  active_thr = main_thread; /*Thread passa a activa*/
  splx(HIGH);
  Clock = 1;
  sthread_time_slices_init(sthread_user_dispatcher,CLOCK_TICK);/*Para iniciar o time_slicer, indicando a funcao de despaxo e o periodo*/
  splx(LOW);
}


sthread_t sthread_user_create(sthread_start_func_t start_routine, void *arg, int priority)/*Criar uma thread de user*/
{
	
  struct _sthread *new_thread = (struct _sthread*)malloc(sizeof(struct _sthread));/*Cria uma estrutura sthread*/
  sthread_ctx_start_func_t func = sthread_aux_start;		/*Processo Filho*/							
  new_thread->args = arg;									/*Atribuir argumentos*/
  new_thread->start_routine_ptr = start_routine;			/*A tarefa é iniciada nesta rotina que é passada pela aplicacao*/
  new_thread->wake_time = 0;								
  new_thread->join_tid = 0;
  new_thread->join_ret = NULL;
  new_thread->saved_ctx = sthread_new_ctx(func);			/*Criar um novo contexto de funcao */
  new_thread->vruntime = RBMenorChave(exe_thr_arvore_rb);	/*O novo processo criado fica com o tempo do processo com maior prioridade*/
  new_thread->exectime = 0; 								/* Tempo execuçao começa a 0 */
  new_thread->nice = 0;
  
  if(priority > 10){										/* limita os valor da prioridade */
	new_thread->priority = 10;
	}
  else if(priority < 1){
	new_thread->priority =  1;
	}
  else{
	new_thread->priority = priority;
	}
	
	
  splx(HIGH);	/*Inibir interrupções -time_slice -SIM*/	
  
  		
  new_thread->tid = tid_gen++;									/*Aqui é atribuido um tid*/
  
  new_thread->wake_time = 0;
  new_thread->sleeptime = 0;
  new_thread->times_runned = 0;
  RBInserir(exe_thr_arvore_rb,new_thread);/*Insere thread na arvore rb dos executaveis*/
  
  splx(LOW);
 
  return new_thread;
}


void sthread_user_exit(void *ret) {
  splx(HIGH);
  
   int is_zombie = 1;

   // unblock threads waiting in the join list
   queue_t *tmp_queue = create_queue();   		
   while (!queue_is_empty(join_thr_list)) {						/*Verificar se algum pai já aguarda por nós*/
      struct _sthread *thread = queue_removeThread(join_thr_list);			
    
      if (thread->join_tid == active_thr->tid) {				
		  thread->join_ret = ret;								/*Encontramos o pai, vamos dizer que realizou join e vai poder continuar*/	
				
		RBInserir(exe_thr_arvore_rb, thread);		/*O pai vai continuar*/
        
         is_zombie = 0;				/*E o processo (filho) deixa de ser zombie*/
      } else {
         queue_insert(tmp_queue,thread);		/*Se não era o pai, mantem na lista*/
      }
   }
   delete_queue(join_thr_list);			/*Apagar a queue antiga e substituir pela nova*/
   join_thr_list = tmp_queue;
 
 
   if (is_zombie) {			/*Se for zombie, o pai ainda nao estava a espera, vai para a lista de zombies*/
      queue_insert(zombie_thr_list, active_thr);
   } else {
      queue_insert(dead_thr_list, active_thr);		/*Se o pai recebeu o "certificado de morte", a tarefa passa à lista de mortas*/
   }
   

	/*Se a lista de executaveis estiver vazia, vamos apagá-la*/
   if(RBArvoreVazia(exe_thr_arvore_rb)){  						/* pode acontecer se a unica thread em execucao fizer exit*/
		RBarvoreDestroy(exe_thr_arvore_rb);
    
		delete_queue(dead_thr_list);	/*Apagar a lista das tarefas mortas*/
		sthread_user_free(active_thr);	/*Libertar a thread activa*/
		dprintf("Exec rbtree is empty!\n");
		exit(0);
  }

  
   // remove from exec list
   struct _sthread *old_thr = active_thr;	/*Preservar referencia para a thread activa*/
   
   active_thr = RBExtraiMininimo(exe_thr_arvore_rb); /*Seleccionar uma nova thread*/
   sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);

   splx(LOW);
}

	
void sthread_user_dispatcher(void){
	
	++(active_thr->times_runned);														
	++(active_thr->exectime);/* Incrementa o tempo de execuçao do processo*/
	
	actualizarWaittime(exe_thr_arvore_rb,RBRaizArvore(exe_thr_arvore_rb));	/*Actualizar todos os waittime das tarefas na arvore de executaveis*/
	actualizarJoinTime(join_thr_list);			/*Actualizar o sleeptime de todos os que aguardam bloqueados em listas*/
	actualizarMutex(mutex_list);
	actualizarMonitores(monitor_list);
	
	acordarProcessos(sleep_thr_arvore_rb,RBRaizArvore(sleep_thr_arvore_rb));//mete os processo que acordaram em execuçao
	
	long tempV = active_thr->vruntime + ((active_thr->nice)+(active_thr->priority))*(active_thr->times_runned);
	if(tempV >= RBMenorChave(exe_thr_arvore_rb)){		 /*Vamos retirá-lo de execucao*/						
		sthread_user_yield();				/*faz a comutacao*/
	}
	
	
	++Clock;
	
	
	if(active_thr->times_runned < min_delay){
		return;		/*Ja executou o tempo minimo?*/

	}else {
		
		active_thr->vruntime += ((active_thr->nice)+(active_thr->priority))*(active_thr->times_runned);
		active_thr->times_runned = 0;
		
		/*Preempcao   - Verificar os processos em sleep*/
		//acordarProcessos(sleep_thr_arvore_rb,RBRaizArvore(sleep_thr_arvore_rb));
		/*Seleccionar o processo de menor vruntime para realizar a comutacao*/	
		//if(active_thr->vruntime > RBMenorChave(exe_thr_arvore_rb)){		 /*Vamos retirá-lo de execucao?*/						
		//	sthread_user_yield();				/*Verificar se e para fazer a comutacao*/
		//}
		
	}
}

void sthread_user_yield(void){
  
  splx(HIGH); 
  struct _sthread *old_thr;
   			
	old_thr = active_thr;						/*Seleccionar a tarefa actual*/						
 
	RBInserir(exe_thr_arvore_rb,old_thr);

	active_thr = RBExtraiMininimo(exe_thr_arvore_rb);	/*Colocar a thread com mais prioridade como activa*/
  
	sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);	/*Fazer a comutação*/
	splx(LOW);
}


void sthread_user_free(struct _sthread *thread)
{
  sthread_free_ctx(thread->saved_ctx);
  free(thread);
}

/*********************************************************************/
/* Part 2: Join and Sleep Primitives                                 */
/*********************************************************************/


/*O pai chama o join para recolher os filhos*/
int sthread_user_join(sthread_t thread, void **value_ptr)
{
	
/*Suspende a execucao da thread que chamou a funcao até que a thread alvo termine,
 * a menos que a tarefa alvo já tenha terminado. No retorno com sucesso de um pthread_join() 
 * chama o valor nao nulo de value_ptr arg, valor passado para o pthread_exit() pela tarefa terminada
 * é disponibilizado na localizacao referenciada por value_ptr.
 * Quando a pthread_join() retorna com sucesso, a thread alvo é terminada. 
 * O resultado de multiplas e simultaneas chamadas ao pthread_join() para a mesma thread alvo é indefinido.
 * Se a thread que está chamando pthrad_join() for cancelada, entao a thread alvo não será retirada
 * 
 * Se tiver sucesso, o ptread_join() retorna zero.Caso contrário, retorna um numero de erro que indica o erro
 *O pai vai procurar os filhos. Vai ver aos zombies, se o encontrar, mata-o para deadlist.
 * Se nao, vai procurá-lo por todas as listas. Se encontrar, altera o seu join_ret,
 *  sai de execucao e fica na join_thr_list a aguardar que os filhos acabem*/
    
   splx(HIGH);
   // checks if the thread to wait is zombie
   int found = 0;
   queue_t *tmp_queue = create_queue(); 
   
   while (!queue_is_empty(zombie_thr_list)) {						/*Vamos ler os zombies*/
      struct _sthread *zthread = queue_removeThread(zombie_thr_list);
      if (thread->tid == zthread->tid) {					/*Se a thread actual esta em zombie*/
         *value_ptr = thread->join_ret;					/*Realizar o join*/
         queue_insert(dead_thr_list,zthread);			/*Coloca-la na lista de tarefas mortas*/
         found = 1;		/*Encontramos, vamos retornar com sucesso*/
      } else {
         queue_insert(tmp_queue,zthread);			/*Se nao a queremos, fica na lista*/
      }
   }
   delete_queue(zombie_thr_list);		/*Substituir a lista*/
   zombie_thr_list = tmp_queue;
  
   if (found) {
       splx(LOW);
       return 0;	/*Encontramos, vamos retornar com sucesso*/
   }

   
   // search active queue
   if (active_thr->tid == thread->tid) {		/*Join com actual?*/
      found = 1;
   }

   // search na lista de executaveis    
   if((RBSearchTID(exe_thr_arvore_rb,thread->tid)) == 1)	/*Vai procurar o TID da thread na arvore de executaveis, 1 se encontrou,0 se nao*/
			found = 1;

   // search na lista de sleep
    if((RBSearchTID(sleep_thr_arvore_rb,thread->tid)) == 1);	/*Procurar nas tarefas adormecidas*/
			found = 1;

   
   queue_element_t *qe = NULL;	/*Ponteiro usado nas listas*/	

   // search na lista de join
   qe = join_thr_list->first;
   while (!found && qe != NULL) {
      if (((struct _sthread*)qe->conteudo)->tid == thread->tid) {
         found = 1;
      }
      qe = qe->next;
   }

   // if found blocks until thread ends
   if (!found) {
      splx(LOW);
      return -1;			/*Não encontrou a thread para fazer join*/
   } 
   else {								/*Encontramos o TID numa das listas ou na propria thread*/
      active_thr->join_tid = thread->tid; 	/*Vamos colocar o tid da encontrada no join tid da actual*/
      
      struct _sthread *old_thr = active_thr;
      queue_insert(join_thr_list, old_thr);
      
      active_thr = RBExtraiMininimo(exe_thr_arvore_rb);
      sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
  
      *value_ptr = thread->join_ret;
   }
   
   splx(LOW);
   return 0;
}


/* minimum sleep time of 1 clocktick.
  1 clock tick, value 10 000 = 10 ms */

int sthread_user_sleep(int time){
   splx(HIGH);
   
   long int num_ticks = time / CLOCK_TICK;
   
   if (num_ticks == 0) {
      splx(LOW);
      
      return 0;
	}
	active_thr->wake_time = Clock + num_ticks;
	
	RBInserirNo(sleep_thr_arvore_rb,active_thr->wake_time,active_thr->tid,active_thr); /*Vamos colocar na lista das tarefas adormecidas e carregar outra thread*/
   
   sthread_t old_thr = active_thr;
   
   if(RBArvoreVazia(exe_thr_arvore_rb)){
		dprintf("ARVORE VAZIA sleep 441\n");
		exit(-1);
	}
   active_thr = RBExtraiMininimo(exe_thr_arvore_rb);  
   
   sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
   
   splx(LOW);
   return 0;
}

/* --------------------------------------------------------------------------*
 * Synchronization Primitives                                                *
 * ------------------------------------------------------------------------- */

/*
 * Mutex implementation
 */

struct _sthread_mutex
{
	int id_monitor;				/*Monitor a que esta associado*/
	int id;						/*E atribuido um identificador a cada mutex*/
	lock_t l;
	struct _sthread *thr;
	
	queue_t* queue;				/*Lista de bloqueados no mutex*/
};

sthread_mutex_t sthread_user_mutex_init()
{
  sthread_mutex_t lock;

  if(!(lock = malloc(sizeof(struct _sthread_mutex)))){
    dprintf("Error in creating mutex\n");
    return 0;
  }

  /* mutex initialization */
  lock->l=0;
  lock->thr = NULL;
  lock->queue = create_queue();	
  lock->id_monitor = -1;					/*Comeca por considerar que nao tem monitor associado*/
  lock->id = mutex_id_gen++;				/*Atribuir Id ao mutex*/
  queue_insert(mutex_list,lock);			/*Colocar o mutex na lista de mutex*/
 
  return lock;
}

void sthread_user_mutex_free(sthread_mutex_t lock)		/*Apagar o mutex*/
{
	int encontrado = 0;
	int lockid = lock->id;
	struct _sthread_mutex *mutextemp;
	
	 queue_t *tmp_queue = create_queue();
	
	while(!queue_is_empty(mutex_list)){
		mutextemp = queue_removeMutex(mutex_list);
		if(lockid == mutextemp->id)
			encontrado = 1;			/*Encontramos o mutex a retirar da lista,nao e inserido na nova*/
		else
			queue_insert(tmp_queue,mutextemp);
		}
		
   delete_queue(mutex_list);	
   mutex_list = tmp_queue;
	
  delete_queue(lock->queue);
  free(lock);
}


void sthread_user_mutex_lock(sthread_mutex_t lock)		/*Trancar o mutex*/
{
	while(atomic_test_and_set(&(lock->l))) {}
	
  if(lock->thr == NULL){			/*Se livre, coloca a thread actual como a thread bloqueada*/
    lock->thr = active_thr;			
    atomic_clear(&(lock->l));
  } else {
	queue_insert(lock->queue, active_thr);	/*Guardar a thread numa lista para aguardar*/
    
    atomic_clear(&(lock->l));

    splx(HIGH);
    
    struct _sthread *old_thr;			
    old_thr = active_thr;
    
    active_thr = RBExtraiMininimo(exe_thr_arvore_rb);
    sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
    splx(LOW);
  }
}

void sthread_user_mutex_unlock(sthread_mutex_t lock)		/*Desbloquear o mutex*/
{
  if(lock->thr!=active_thr){
    dprintf("unlock without lock!\n");
    return;
  }

  while(atomic_test_and_set(&(lock->l))) {}


if(queue_is_empty(lock->queue)){		/*Se a fila está vazia, vamos colocar a NULL*/
    lock->thr = NULL;
  } else {
    lock->thr = queue_remove(lock->queue);		/*Vamos remover a tarefa que estava bloqueada */
	
	long tempr = (active_thr->vruntime)+(((active_thr->nice)+(active_thr->priority))*active_thr->times_runned); 
   splx(HIGH);
   if(tempr > lock->thr->vruntime){		 /*Vamos retirá-lo de execucao?*/
		
		dprintf("RETIRAR do mutex   !!! loked : %ld   active:%ld     \n",lock->thr->vruntime,tempr);
		active_thr->vruntime += (((active_thr->nice)+(active_thr->priority))*active_thr->times_runned);
		active_thr->times_runned = 0;

		struct _sthread *old_thr = active_thr;						/*Seleccionar a tarefa actual*/										 
		RBInserir(exe_thr_arvore_rb,active_thr);
		active_thr = lock->thr;
		sthread_switch(old_thr->saved_ctx, lock->thr->saved_ctx);	/*Fazer a comutação*/
		
   }
   else{
		RBInserir(exe_thr_arvore_rb,lock->thr); /*e coloca-la na three de executaveis,sera executada quando o despacho a escolher*/  
	}
	splx(LOW);
  }

  atomic_clear(&(lock->l));
}

/*
 * Monitor implementation
 */

struct _sthread_mon {
	int id;
 	sthread_mutex_t mutex;
	queue_t* queue;
};

sthread_mon_t sthread_user_monitor_init()		/*Iniciar o monitor*/
{
  sthread_mon_t mon;
  if(!(mon = malloc(sizeof(struct _sthread_mon)))){
    dprintf("Error creating monitor\n");
    return 0;
  }
	mon->id = monitor_id_gen++;					/*Atribuir um id ao monitor que criamos*/
	mon->mutex = sthread_user_mutex_init();
	mon->mutex->id_monitor = mon->id;			/*Mutex, eu sou o teu monitor*/
	mon->queue = create_queue();
	queue_insert(monitor_list, mon);			/*Guardar o monitor na lista de monitores*/
	return mon;
}

void sthread_user_monitor_free(sthread_mon_t mon)		/*Libertar um monitor*/
{
  struct _sthread_mon* monitortemp;
   queue_t* tmp_queue = create_queue();
   int monId = mon->id;
   int encontrado = 0;
   
  sthread_user_mutex_free(mon->mutex);
  delete_queue(mon->queue);
  
	while(!queue_is_empty(monitor_list)){
		monitortemp = queue_removeMonitor(mutex_list);
		if(monId == monitortemp->id)
			encontrado = 1;			/*Encontramos o monitor a retirar da lista,nao e inserido na nova*/
		else
			queue_insert(tmp_queue,monitortemp);
		}
		
   delete_queue(monitor_list);	
	monitor_list = tmp_queue;
  
  free(mon);
}

void sthread_user_monitor_enter(sthread_mon_t mon)		/*Entrar no monitor*/
{
  sthread_user_mutex_lock(mon->mutex);		/*Garantir que apenas entra 1 de cada vez*/
}

void sthread_user_monitor_exit(sthread_mon_t mon)	/*Sair do monitor*/
{
  sthread_user_mutex_unlock(mon->mutex);			/*Desbloqueia o mutex*/
}

void sthread_user_monitor_wait(sthread_mon_t mon)		/*Fazer wait no monitor,bloquear-se*/
{
  struct _sthread *temp;

  if(mon->mutex->thr != active_thr){						/*Se alguem fez wait sem ser a thread que entrou, assinala erro*/
    dprintf("monitor wait called outside monitor\n");
    return;
  }

  /* inserts thread in queue of blocked threads */
	temp = active_thr;
  
	queue_insert(mon->queue, temp);/*A thread bloqueia-se na lista*/			
	
  /* exits mutual exclusion region */
  sthread_user_mutex_unlock(mon->mutex);		/*So permite inserir uma thread na lista de cada vez*/

  splx(HIGH);
  struct _sthread *old_thr;
  old_thr = active_thr;
   
  active_thr = RBExtraiMininimo(exe_thr_arvore_rb);
  sthread_switch(old_thr->saved_ctx, active_thr->saved_ctx);
  splx(LOW);
}

void sthread_user_monitor_signal(sthread_mon_t mon)		/*Assinalar monitor,libertar 1 processo*/
{
  struct _sthread *temp;

  if(mon->mutex->thr != active_thr){
    dprintf("monitor signal called outside monitor\n");
    return;
  }

  while(atomic_test_and_set(&(mon->mutex->l))) {}
  
  if(!queue_is_empty(mon->queue)){
    /* changes blocking queue for thread */
    temp = queue_remove(mon->queue);		/*Assinala passando da fila do monitor para a do mutex*/
    queue_insert(mon->mutex->queue, temp);
  }
  atomic_clear(&(mon->mutex->l));
}

void sthread_user_monitor_signalall(sthread_mon_t mon)
{
  struct _sthread *temp;

  if(mon->mutex->thr != active_thr){
   dprintf("monitor signalall called outside monitor\n");
    return;
  }

  while(atomic_test_and_set(&(mon->mutex->l))) {}
  while(!queue_is_empty(mon->queue)){
    /* changes blocking queue for thread */
    temp = queue_remove(mon->queue);
    queue_insert(mon->mutex->queue, temp);		/*Vao todas para a queue do mutex*/
  }
  atomic_clear(&(mon->mutex->l));
}
   

/* The following functions are dummies to 
 * highlight the fact that pthreads do not
 * include monitors.
 */

sthread_mon_t sthread_dummy_monitor_init()
{
   dprintf("WARNING: pthreads do not include monitors!\n");
   return NULL;
}


void sthread_dummy_monitor_free(sthread_mon_t mon)
{
   dprintf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_enter(sthread_mon_t mon)
{
   dprintf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_exit(sthread_mon_t mon)
{
   dprintf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_wait(sthread_mon_t mon)
{
   dprintf("WARNING: pthreads do not include monitors!\n");
}


void sthread_dummy_monitor_signal(sthread_mon_t mon)
{
   dprintf("WARNING: pthreads do not include monitors!\n");
}

void sthread_dummy_monitor_signalall(sthread_mon_t mon)
{
   dprintf("WARNING: pthreads do not include monitors!\n");
}

/* Dumps das Threads*/

void ImprimirThread(struct _sthread* thread){
		printf("id: %d ",thread->tid);
		printf("priority: %d ",thread->priority);
		printf("vruntime: %ld ", thread->vruntime);
		printf("runtime: %ld ", thread->exectime);
		printf("sleeptime: %ld ", thread->sleeptime);
		printf("waittime: %ld ", thread->waittime);
		if(thread->wake_time > Clock){
			printf("timetounlock: %ld ",
			(thread->wake_time-Clock)*CLOCK_TICK);
		}
		printf("\n\n");
}

void ImprimirDadosRB(ArvoreRB arvore,PtNo h){		/*Imprimir por ordem crescente de chave*/
	if(h == arvore->nil)	/*Se é vazia*/
		return;
	ImprimirDadosRB(arvore,h->esq);
		
		ImprimirThread(h->elem);
	
	ImprimirDadosRB(arvore,h->dir);
}

void ImprimirDadosLista(queue_t * queue){
	queue_element_t* ptr;
	struct _sthread* thread;
	if(queue == NULL)
		return;
	
	ptr = (queue_element_t *) queue_firstElem(queue);	/*Acede ao 1º elemento da lista*/
	while(ptr !=NULL){
		thread = (struct _sthread*) ptr->conteudo;
		ImprimirThread(thread);
		ptr = ptr->next;
	}
}

void sthread_user_dump(){
	splx(HIGH);	/*Não queremos ser interrompidos durante o dump porque tornaria os valores de relogio irreais*/
	queue_element_t* pont;	/*Ponteiro para percorrer a queue*/
	queue_t* tempQueue;
	struct _sthread_mutex *mutextemp;
	struct _sthread_mon *montemp;
	
	printf("\n=== dump start ===\n");
	printf("active thread \n");
	
	if(active_thr == NULL)
		printf("Nenhuma thread activa");
	else{
		ImprimirThread(active_thr);
		printf("\n");
	}
	
	printf(">>>> RB-Tree <<<<\n");
	ImprimirDadosRB(exe_thr_arvore_rb,RBRaizArvore(exe_thr_arvore_rb));	/*Imprime a arvore por orderm crescente de chave*/
	printf("\n");
	
	printf(">>>> SleepList <<<<\n");					/*Ordem de tempo crescente por desbloquear*/
	ImprimirDadosRB(sleep_thr_arvore_rb,RBRaizArvore(sleep_thr_arvore_rb));
	printf("\n");
	
	printf(">>>> BlockedList <<<<\n");
	/*Ordem decrescente de tempo de bloqueada, a ultima e a ultima que se bloqueou.*/
	for(pont = monitor_list->first; pont!= NULL;pont = pont->next){	//Percorrer os processos presos na arvore de cada monitor
		montemp = (struct _sthread_mon *) pont->conteudo;
		tempQueue = montemp->queue;
		printf("Monitor: %d \n",montemp->id);
		if(queue_firstElem(tempQueue) == NULL)
			printf("----Livre----\n");
		else{
			ImprimirDadosLista(tempQueue);
		}
	}
	printf("\n");
	
	
	
	for(pont = mutex_list->first; pont!= NULL; pont = pont->next){	/*Percorrer toda a lista de mutexs*/
		mutextemp = (struct _sthread_mutex *) pont->conteudo;	/*Aceder a arvore do mutex do no*/
		tempQueue = mutextemp->queue;/*Imprimir os dados das thread bloqueadas no mutex do monitor*/
		if(mutextemp->id_monitor != -1){/*O mutex esta associado a um monitor?*/
			printf("Mutex: %d / Monitor: %d\n", mutextemp->id, mutextemp->id_monitor);	
			if(queue_firstElem(tempQueue) == NULL)
				printf("----Livre----\n");
			else{
				ImprimirDadosLista(tempQueue);	/*Imprimir os dados das thread bloqueadas no mutex*/
			}	
		}
		else{
			printf("Mutex: %d\n",mutextemp->id);
			if(queue_firstElem(tempQueue) == NULL)
				printf("----Livre----\n");
			else{
				ImprimirDadosLista(tempQueue);	/*Imprimir os dados das thread bloqueadas no mutex*/
			}
		}
	}
	
	printf("\n=== Dump End ===\n");
	splx(LOW);
}


int sthread_nice(int nice){
	
	if(nice > 10){    /* limita os valor do nice */
		active_thr->nice = 10;
	}
	else if(nice < 1){
		active_thr->nice = 1;
		}
	else{
		active_thr->nice = nice;
	}
	
	return (active_thr->priority)+(active_thr->nice);
}




void actualizarWaittime(ArvoreRB arvore_executaveis,PtNo h){					/*Actualiza automaticamente o waitime*/
	if(h == arvore_executaveis->nil)	/*Quando vazia*/
		return;
	
	actualizarWaittime(arvore_executaveis,h->esq);
	
	(h->elem->waittime)++;	
	
	actualizarWaittime(arvore_executaveis,h->dir);
}

void acordarProcessos(ArvoreRB arvore_sleep,PtNo h){					/*Actualiza automaticamente o sleeptime*/
	struct _sthread *thread;
	if(h == arvore_sleep->nil)	/*Quando vazia*/
		return;
	
	acordarProcessos(arvore_sleep,h->esq);
	
	thread = h->elem;	
	
	if(thread->wake_time <= Clock){	/*Ha algum processo para acordar, temos de ter em conta os 5pulsos minimos (usamos o <=)*/
		
		thread->wake_time = 0; /*Reset ao "despertador"*/
		
		RBInserir(exe_thr_arvore_rb,thread); /*e coloca-la na three de executaveis,sera executada quando o despacho a escolher*/  
					
		h->elem = NULL;			/*Quebrar a ligacao entre o no da arvore sleep e a thread, caso contrario ao remover o no iria destruir a thread que esta nos executaveis*/
		
		RBRemoverNo(arvore_sleep,h);			/*Tira-lo da arvore de sleep*/
	
	}
	else{
		thread->sleeptime++;					/*Vai dormir +1 clock*/
	}
	acordarProcessos(arvore_sleep,h->dir);
}


void actualizarJoinTime(queue_t* join_list){
	actualizarSleepTimeLista(join_list);
}
	

void actualizarSleepTimeLista(queue_t* lista_threads){
	queue_element_t* ptr;
	if(lista_threads == NULL)
		return;
	
	ptr = (queue_element_t *) queue_firstElem(lista_threads);	/*Acede ao 1º elemento da lista*/
	while(ptr !=NULL){
		((struct _sthread*) ptr->conteudo)->sleeptime++;
		ptr = ptr->next;
	}
}
	

void actualizarMutex(queue_t* lista_mutex){
	queue_element_t* pont;	/*Ponteiro para percorrer a queue*/
	queue_t* lista_do_mutex;
	struct _sthread_mutex *mutextemp;
	
	if(lista_mutex == NULL)
		return;	
	
	for(pont = lista_mutex->first; pont!= NULL; pont = pont->next){	/*Percorrer toda a lista de mutexs*/
		mutextemp = (struct _sthread_mutex *) pont->conteudo;	/*Aceder ao mutex do no*/
		if(mutextemp->thr != NULL)
			mutextemp->thr->sleeptime++;
			
		lista_do_mutex = mutextemp->queue;
		actualizarSleepTimeLista(lista_do_mutex);
	}
}



void actualizarMonitores(queue_t* lista_Monitores){
	queue_element_t* ptr_monitores;
	queue_t *lista_bloqueadas;
	struct _sthread_mon* monitortemp;
	if(lista_Monitores == NULL)
		return;
	
	
	
	for(ptr_monitores = lista_Monitores->first; ptr_monitores!= NULL; ptr_monitores = ptr_monitores->next){	/*Percorrer toda a lista de monitores*/
		monitortemp = (struct _sthread_mon *) ptr_monitores->conteudo;	/*Aceder ao monitor do no*/			
		lista_bloqueadas = monitortemp->queue;
		actualizarSleepTimeLista(lista_bloqueadas);
	}
}
	





