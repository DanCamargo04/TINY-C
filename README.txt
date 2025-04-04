Integrantes:
Cláudio Dias Alves (101403569) 
Daniel Rubio Camargo (10408823)

O compilador criado realiza tanto a análise léxica quanto sintática de um arquivo de texto contendo um código na linguagem TINY-C.

Parte léxica:

Nós construímos as funções de verificação léxica a partir de autômatos desenhados para facilitar o entendimento. Caso a entrada não condiza com o autômato, 
será mostrado um erro léxico para o usuário (no caso do erro léxico de excesso de caracteres na criação de identificadores, nós mostramos uma mensagem de erro léxico 
especial). Além das funções para reconhecer inteiros, caracteres, identificadores e comentários, nós tivemos que criar funções para agrupar caracteres que participariam 
de um mesmo átomo, como por exemplo a função ... que agrupa os dois '&' para formarem o átomo 'AND'.

Nas funções 'reconhece_id' e 'reconhece_int' será guardada a posição inicial do entrada para depois armazenar o conteúdo em um dos atributos do struct TInfoAtomo.
As funções de verificação léxica criadas no projeto são:

TInfoAtomo verifica_igual()
TInfoAtomo verifica_maior() 
TInfoAtomo verifica_menor() 
TInfoAtomo verifica_exclamacao()
TInfoAtomo verifica_ecomercial()
TInfoAtomo verifica_pipe()
TInfoAtomo reconhece_id()
TInfoAtomo reconhece_int()
TInfoAtomo reconhece_char()
TInfoAtomo reconhece_comentario_ou_divisao()

Parte sintática:

Para a parte sintática, nós apenas criamos funções de acordo com as regras de produção dadas no enunciado do projeto. 
A função inicial é a 'program' que é chamada na 'main' e cria uma árvore de derivação ao realizar chamadas recursivas das outras funções presentes nas regras de produção, 
retornando um erro caso os átomos não estejam em uma sequência válida.

As funções de verificação sintática criadas no projeto são:

void program();
void compound_stmt();
void var_decl();
void type_specifier();
void var_decl_list();
void variable_id();
void stmt();
void assig_stmt();
void cond_stmt();
void while_stmt();
void expr();
void conjunction();
void comparision();
void relation();
void sum();
void term();
void factor();