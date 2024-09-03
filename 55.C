/****************************************************************
 * PROGRAMA:	ESCALON.C
 * OBJETIVO:	Demostrar os conceitos basicos para a construcao
 *		de um escalonadr de processos
 * AUTOR:	Fernando Branquinho
 *
 * VERSAO:	1.0 (MAR/2003)
 *			versao original
 ****************************************************************/

#include "dos.h"

/****************************************************************
 * Constantes e Variaveis Globais da Rotina ESCALONADOR
 ****************************************************************/

#define	LIMITE_CIC_ESC	10
#define TAM_PILHA 1000

int	global_ponteiroEscalonador;	/* Indice que determina posicao
					 * do ponteiro do escalonador
					 * dentro da janela
					 */

int 	global_nrot;			/* Numero de rotinas registradas */

struct {

	unsigned int pilha[TAM_PILHA]; 	/* Pilha de cada rotina */

	unsigned int reg_sp;		/* Topo da pilha da rotina */
	unsigned int reg_ss;		/* Segmento da pilha da rotina
					   que sera na area de dados (DS) */

} global_var_escalonador[LIMITE_CIC_ESC];

void interrupt

	(*global_intOriginal)(); 	/* Variavel para armazenamento
					 * do antigo endereco da rotina
					 * de interrupcao do relogio do PC
					 */
/****************************************************************
 * API com funcoes de uso geral
 ****************************************************************/
/****************************************************************
 * ROTINA:	print()
 * OBJETIVO:	Saida de string no Video em baixo nivel
 *
 * PARAM:	int = coluna onde sera' apresentada a string
 *		int = linha onde sera' apresentada a string
 *		char * = Ponteiro para string a ser apresentada
 ****************************************************************/

void print(int c,int l,char *string) {

int t;
char far *vid=(char far *)0xb8000000;

	for (t=0;string[t];t++)
		*(vid+t*2+c*2+l*160)=string[t];
}

/****************************************************************
 * ROTINA:	b_ltoa()
 * OBJETIVO:	Converte um valor longo para ASCII
 *
 * PARAM:	int = numero a ser convertido
 *		char * = Ponteiro para string de destino
 ****************************************************************/

char *b_ltoa(unsigned long numero,char *string) {
unsigned long t,g,aux;

	for (t=0;numero > 0;t++) {
		string[t]=numero%10+0x30;
		numero=numero/10;
	}
	string[t]=0;

	for (g=0;g<t/2;g++) {
		aux=string[g];
		string[g]=string[t-g-1];
		string[t-g-1]=aux;
	}

	return(string);
}
/****************************************************************
 * ROTINA:	limpa_tela()
 * OBJETIVO:	Preencher toda a tela com espacos em branco
 *
 * PARAM:	nenhum
 ****************************************************************/
void limpa_tela() {
int c,l;

	for (c=0;c<80;c++)
		for(l=0;l<25;l++)
			print(c,l," ");
}
/****************************************************************
 * Rotinas de geranciamento da multitarefa
 ****************************************************************/
/****************************************************************
 * ROTINA:	registra_rotina()
 * OBJETIVO:	Registra uma rotina para rodar em Multitarefa
 *
 * PARAM:	Endereco da Rotina
 ****************************************************************/

void registra_rotina(void (*rot)()) {
int t;
int sp_main,ss_main;

	global_nrot++;			/*Incremento o numero de rotinas
					  cadastradas */

	//Salvo endereco da pilha original

	sp_main=_SP;
	ss_main=_SS;

	//Defino endereco da pilha da nova rotina

	_SS=_DS;
	_SP=(int)(global_var_escalonador[global_nrot].pilha)+TAM_PILHA*2;


	//Carrego valores iniciais na pilha da nova rotina

	asm pushf;				//Carrego flags

	_SI=FP_SEG((void far *)rot);
	asm push SI;				//Carrego CS da rotina

	_SI=FP_OFF((void far *)rot);
	asm push SI;				//Carrego IP da rotina

	asm push AX;				//Carrego AX

	asm push BX;				//Carrego BX

	asm push CX;				//Carrego CX

	asm push DX;				//Carrego DX

	asm push ES;				//Carrego ES

	asm push DS;				//Carrego DS

	asm push SI;				//Carrego SI

	asm push DI;				//Carrego DI

	asm push BP;				//Carrego BP


	//Salvo topo da pilha da nova rotina
	global_var_escalonador[global_nrot].reg_sp=_SP;

	//Salvo segmento da pilha da nova rotina
	global_var_escalonador[global_nrot].reg_ss=_SS;

	//Recupero endereco da pilha original
	_SP=sp_main;
	_SS=ss_main;
}

/****************************************************************
 * ROTINA:	escalonador()
 * OBJETIVO:	Rodar em multitarefa, todas as rotinas registradas
 *
 * PARAM:	nenhum
 ****************************************************************/

void interrupt escalonador(void)
{
int atual;	//Indice da rotina que esta' sendo executada
int prox;	//Indice da proxima rotina a ser executada

	atual=global_ponteiroEscalonador;

	global_ponteiroEscalonador++;

	if (global_ponteiroEscalonador > global_nrot)
		global_ponteiroEscalonador=0;

	prox=global_ponteiroEscalonador;

	/* Inverto posicao da pilha */

		global_var_escalonador[atual].reg_sp=_SP;
		global_var_escalonador[atual].reg_ss=_SS;

		_SS=global_var_escalonador[prox].reg_ss;
		_SP=global_var_escalonador[prox].reg_sp;

	outportb(0x20,0x20);
}

/****************************************************************
 * ROTINA:	inicializa_multitarefa()
 * OBJETIVO:	Preparar o ambiente para execucao do escalonador
 *
 * PARAM:	nenhum
 ****************************************************************/

void inicializa_multitarefa(void)
{
	global_intOriginal=getvect(8);
	setvect(8,escalonador);
	outportb(0x40, 0xa9);
	outportb(0x40,4);
}

/****************************************************************
 * ROTINA:	finaliza_multitarefa()
 * OBJETIVO:	Restaurar condicao existente antes da utilizacao
 * 		do escalonador.
 *
 * PARAM:	nenhum
 ****************************************************************/

void finaliza_multitarefa(void)
{
	setvect(8,global_intOriginal);
}

/****************************************************************
 * Rotinas para execucao em multitarefa
 ****************************************************************/

void contador1() {
long int c=0;
char strCont[50];

	while(1) {
		c++;
		if (c>5000000)
			c=0;

		b_ltoa(c,strCont);
		print(10,10,strCont);
	}
}

void contador2() {
	long int c = 0;
	char strCont[50];
	while (1) {
	c++;
	if (c> 5000000) c =0;
	b_ltoa(c, strCont);
	print(20,10, strCont);
	}
}

void contador3() {
	long int c = 0;
	char strCont[50];
	while (1) {
	c++;
	if (c> 5000000) c = 0;
	b_ltoa(c, strCont);
	print(30,10, strCont);
	}
}
/****************************************************************
 * ROTINA:	main()
 * OBJETIVO:	Preparar e iniciar o processo de multitarefa
 *		Finalizar a execucao do programa
 *
 * PARAM:	nenhum
 ****************************************************************/

void main() {
char ch=0;
	clrscr();
	global_nrot=0;

	//Registro das rotinas

	registra_rotina(contador1);
	registra_rotina(contador2);
        registra_rotina(contador3);

	limpa_tela();
	print(10,0,"Guilherme Siqueira - 227421");
	print(10,1,"Lucas Bruder - 215907");

	//Inicio da multitarefa

	inicializa_multitarefa();

	while(ch!='@') {

		ch=0;

		if (kbhit())
			ch=getch();

	}


	finaliza_multitarefa();

}

