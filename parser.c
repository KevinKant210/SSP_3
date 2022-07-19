/*
	skeleton for Dr. Montagne's Systems Software project Summer 2022
	you may alter all of this code if desired, but you must provide clear
	instructions if you alter the compilation process or wish us to
	use different files during grading than the ones provided 
	(driver.c, compiler.h, vm.o/.c, or lex.o/.c)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"

#define false  1
#define true  0

lexeme *tokens;
int lIndex = 0;
instruction *code;
int cIndex = 0;
symbol *table;
int tIndex = 0;

//global level variable
int level;
//value to check if an error occured during parsing
// 0 = true, error occured 1 = false no eror
int hasError = false;

void emit(int opname, int level, int mvalue);
void addToSymbolTable(int k, char n[], int v, int l, int a, int m);
void mark();
int multipledeclarationcheck(char name[]);
int findsymbol(char name[], int kind);

void printparseerror(int err_code);
void printsymboltable();
void printassemblycode();

//helper functions
lexeme getToken(){
	lexeme curr = tokens[lIndex];
	lIndex++;
	return curr;
}

void pushToken(){
	//when looking at token did not get what we expected so we put it back on top
	lIndex--;
}

//our fun functions XD lmao rawr
void block(){
	level++;
	int currProc = tIndex-1;
	int numVars;

	lexeme currToken = getToken();

	//check to see if we are running into a const symbol
	if(currToken.type == constsym){
		constDec();
	}

	if(hasError == true ){
		return;
	}else{
		currToken = getToken();
	}



	if(currToken.type == varsym){
		
		numVars = varDec();
	}

	if(hasError == true ){
		return;
	}else{
		currToken = getToken();
	}

	if(currToken.type == procsym){
		procDec();
	}

	if(hasError == true ){
		return;
	}else{
		table[currProc].addr = cIndex;
		emit(6,0,numVars+3);
	}
	
	//at the end run statements
	statement();
	
	mark();

	level = level-1;

}

//constant declarations
void constDec(){

	lexeme currToken = getToken();

	int halt = false;

	while(halt != true){
		//identifier -> become sym -> number ->comma or semicolon
		if(currToken.type != identsym){
			//expected ident sym got something else
			printparseerror(2);
			hasError = true;
			return;
		}

		if(multipledeclarationcheck(currToken.name) != -1){
			printparseerror(18);
			hasError = true;
			return;
		}

		lexeme var = currToken;

		currToken = getToken();

		if(currToken.type != becomessym){
			printparseerror(2);
			hasError = true;
			return;
		}


		currToken = getToken();

		if(currToken.type != numbersym){
			printparseerror(2);
			hasError = true;
			return;
		}

		addToSymbolTable(1,var.name,currToken.value,-1,-1,0);


		currToken = getToken();

		if(currToken.type == semicolonsym){
			halt = true;
		}else if(currToken.type == commasym){
			currToken = getToken();
		}else if(currToken.type == identsym){

			printparseerror(13);
			hasError = true;
			return;
		}else{
			printparseerror(14);
			hasError = true;
			return;
		}
		
	}
	
	

}
//variable declarations
int varDec(){
	int numVars = 0;
	//vars will be offset by 3 in symbol table
	
	lexeme currToken = getToken();

	int halt = false;

	while(halt != true){
		// [ "var "ident {"," ident} “;"].
		if(currToken.type != identsym){
			//expected ident sym got something else
			printparseerror(3);
			hasError = true;
			return;
		}

		if(multipledeclarationcheck(currToken.name) != -1){
			printparseerror(18);
			hasError = true;
			return;
		}

		addToSymbolTable(2,currToken.name,0,level,numVars + 3,0);
		numVars++;

		currToken = getToken();

		if(currToken.type == commasym){

			currToken = getToken();

		}else if(currToken.type == semicolonsym){

			halt = true;

		}else if(currToken.type == identsym){

			printparseerror(13);
			hasError = true;
			return;
		}else{
			printparseerror(14);
			hasError = true;
			return;
		}
	}
}
//procedure declarations
void procDec(){

	lexeme currToken = getToken();

	if(currToken.type != identsym){
		printparseerror(4);
		hasError = true;
		return;
	}

	if(multipledeclarationcheck(currToken.name) != -1){
			printparseerror(18);
			hasError = true;
			return;
		}

	lexeme proc = currToken;
	currToken = getToken();

	if(currToken.type != semicolonsym){
		printparseerror(4);
		hasError = true;
		return;
	}
	//figure out what address we need to put for our procedures
	// just fill in
	addToSymbolTable(3,proc.name,0,0,0,0);
	//finish programming block!
	block();

	currToken = getToken();

	if(currToken.type != semicolonsym){
		printassemblycode(14);
		hasError = true;
		return;
	}

	// Putting a return on the code stack
	emit(2,0,0);

	return;
}

int statement(){

	lexeme currToken = getToken();

	// assignment statements (“ident := expression”)
	if(currToken.type == identsym){
		assign(currToken);
	}
	else if(currToken.type == callsym){
		call();
	}
	else if(currToken.type == beginsym){
		begin();
	}
	else if(currToken.type == ifsym){
		ifStatement();
	}
	else if(currToken.type == whilesym){
		whileStatement();
	}
	else if(currToken.type == writesym){
		writeStatement(currToken);
	}
	else if(currToken.type == readsym){
		readStatement(currToken);
	}

}

//Statement functions
void assign(lexeme identifier){

	

	int identIndex = findsymbol(identifier.name,2);

		

		if(identIndex == -1){
			printassemblycode(6);
			hasError = true;
			return;
		}

		lexeme currToken = getToken();

		if(currToken.type != becomessym){
			printassemblycode(5);
			hasError = true;
			return;
		}

		expression();

		emit(4,level-table[identIndex].level,table[identIndex].addr);
		
}
void call(){
	lexeme currToken = getToken();

	// Check to make sure the identifier matches a procedure in the symbol table
	int symIdx = getSymIdx(currToken);

	if(symIdx == -1){
		printassemblycode(19);
		hasError = true;
		return;
	}
	else if(currToken.type != procsym){
		printassemblycode(7);
		hasError = true;
		return;
	}

	// not sure if right.
	emit(5,level-table[symIdx].level,table[symIdx].addr);
}
void begin(){

	statement();
	do{
		// Need to fix
		lexeme currToken = getToken();
		if(currToken.token != semicolonsym){
			printassemblycode();
			hasError = true;
			return;
		}

		if{
			statement();
		}
	}while(currToken.type == semicolonsym)
}
void ifStatement(){

	condition();
	int jpcIdx == cIndex;

	// put a jump. addr unknown depending on the if statement outcome
	emit(7, 0,0);

	statement();

	// does jmpidx need to be global?
	int jmpIdx = 0;
	lexeme currToken = getToken();
	if(currToken.type == elsesym){
		jmpIdx = cIndex
		emit(7,0,0);
		code[jpcIdx].m = cIndex*3;
		statement();
		code[jmpIdx].m = cIndex*3;
	}
	else{
		code[jpcIdx].m = cIndex*3;
	}
}
void whileStatement(){
	int loopIdx = cIndex;
	condition();
	int jpcIdx = cIndex;
	emit(7,0,0);
	statement();
	emit(7,0,loopIdx*3);
	code[jpcIdx].m = cIndex;
}
void readStatement(lexeme identifier){

	
	int symIdx = findsymbol(identifier.name,2);
	if(symIdx == -1){
		printassemblycode(19);
		hasError = true;
		return;
	}
	emit(9, 0, 2); 
	emit(4, level - table[symIdx].level,table[symIdx].addr);

}
void writeStatement(){
	expression();
	emit(9,0,1);
}

void condition(){

	expression();

	lexeme currToken = getToken();

	// =
	if(currToken.type == eqlsym){
		expression();
		emit(2,0,7);
	}
	// !=
	else if(currToken.type == neqsym){
		expression();
		emit(2,0,8);
	}
	// <
	else if(currToken.type == lessym){
		expression();
		emit(2,0,9); 
	}
	// <=
	else if(currToken.type == leqsym){
		expression();
		emit(2,0,10);

	}
	// >
	else if(currToken.type == gtrsym){
		expression();
		emit(2,0,11);

	}
	// >=
	else if(currToken.type == geqsym){
		expression();
		emit(2,0,12);
	}
}


//used in assignment statements and write statements
void expression(){
	
	lexeme currToken = getToken();
	
	// if a var is getting assigned a negative expression
	if(currToken.type == minussym){
		term(currToken);
		//NEG
		emit(2,0,1);
	}
	// if a var is getting assigned anything else
	else{
		term();
	}

	// not sure what this does
	while (currToken.type == plussym||currToken.type == minussym){
		if(currToken.type == plussym){
			term(currToken);
			//ADD
			emit(2,0,2);
		}
		else{
			term();
			//SUB
			emit(2,0,3);
		}
	}

}

void term(lexeme currToken){
	
	factor(currToken);

	lexeme currToken = getToken();

	while(currToken.type == multsym||currToken.type == slashsym || currToken.type == modsym ){
		if(currToken.type == multsym){
			factor();
			emit(2,0,4);
		}
		else if(currToken.type == slashsym){
			factor();
			emit(2,0,5);
		}
		else if(currToken.type == modsym){
			factor();
			emit(2,0,6);
		}
		
	}

}

void factor(lexeme currToken){

	int kind;

	
	if(currToken.type == identsym){

		currToken = getToken();
		int symIdx = getSymIdx(currToken);
		if(symIdx == -1){
			printassemblycode(19);
			hasError = true;
			return;
		}
		else if(symIdx == 3){
			printassemblycode(11);
			hasError = true;
			return;
		}
		
		if(currToken.type == constsym){
			emit(1,0,table[symIdx].value);
		}
		else{
			emit(3,level-table[symIdx].level,table[symIdx].addr);
		}

	}
	// if var is being assigned a number
	else if(token == numbersym){
		emit(1,0,currToken.value);
	}
	else if(token == lparentsym){
		expression();


	}

}

// finds the token and its kind, returns the index or -1 if not found in the table
int getSymIdx(lexeme token){

	
		if(token.type == constsym){
			kind = 1;
		}
		else if(token.type == varsym ){
			kind 2;
		}
		else{
			kind 3;
		}
		
		int symIdx = findsymbol(table,kind);

		return symIdx;
}




instruction *parser_code_generator(lexeme *list)
{
	int i ;
	tokens = list;
	code = malloc(sizeof(instruction) * MAX_ARRAY_SIZE);
	table = malloc(sizeof(symbol) * MAX_ARRAY_SIZE);


	
	// Your code here. 
	// Make sure to remember to free table before returning always
	// and to free code and return NULL if there was an error
	
	//this is program from the psuedo code

	//put a jump with m = 0 on the code stack
	emit(7,0,0);
	addToSymbolTable(3, "main",0,0,0,0);
	level = -1;

	block();

	if(hasError == true){
		return;
	}
	//put halt on the code stack
	emit(9,0,3);
	
	//check to ensure there is a period at the end of program
	lexeme currToken = getToken();
	if(currToken.type != periodsym){
		printparseerror(1);
		return code;
	}
	//need to go through and fix all the CAL functions and the initial jump function
	
	code[cIndex].op = -1;
	return code;
}

// adds an instruction to the end of the code array
void emit(int opname, int level, int mvalue)
{
	code[cIndex].op = opname;
	code[cIndex].l = level;
	code[cIndex].m = mvalue;
	cIndex++;
}

// adds a symbol to the end of the symbol table
void addToSymbolTable(int k, char n[], int v, int l, int a, int m)
{
	table[tIndex].kind = k;
	strcpy(table[tIndex].name, n);
	table[tIndex].value = v;
	table[tIndex].level = l;
	table[tIndex].addr = a;
	table[tIndex].mark = m;
	tIndex++;
}

// starts at the end of the table and works backward (ignoring already
// marked entries (symbols of subprocedures)) to mark the symbols of
// the current procedure. it knows it's finished the current procedure
// by looking at level: if level is less than the current level and unmarked
// (meaning it belongs to the parent procedure) it stops
void mark()
{
	int i;
	for (i = tIndex - 1; i >= 0; i--)
	{
		if (table[i].mark == 1)
			continue;
		if (table[i].level < level)
			return;
		table[i].mark = 1;
	}
}

// does a linear pass through the symbol table looking for the symbol
// who's name is the passed argument. it's found a match when the names
// match, the entry is unmarked, and the level is equal to the current
// level. it returns the index of the match. returns -1 if there are no
// matches
int multipledeclarationcheck(char name[])
{
	int i;
	for (i = 0; i < tIndex; i++)
		if (table[i].mark == 0 && table[i].level == level && strcmp(name, table[i].name) == 0)
			return i;
	return -1;
}

// returns -1 if a symbol matching the arguments is not in the symbol table
// returns the index of the desired entry if found (maximizing the level value)
int findsymbol(char name[], int kind)
{
	int i;
	int max_idx = -1;
	int max_lvl = -1;
	for (i = 0; i < tIndex; i++)
	{
		if (table[i].mark == 0 && table[i].kind == kind && strcmp(name, table[i].name) == 0)
		{
			if (max_idx == -1 || table[i].level > max_lvl)
			{
				max_idx = i;
				max_lvl = table[i].level;
			}
		}
	}
	return max_idx;
}

void printparseerror(int err_code)
{
	switch (err_code)
	{
		case 1:
			printf("Parser Error: Program must be closed by a period\n");
			break;
		case 2:
			printf("Parser Error: Constant declarations should follow the pattern 'ident := number {, ident := number}'\n");
			break;
		case 3:
			printf("Parser Error: Variable declarations should follow the pattern 'ident {, ident}'\n");
			break;
		case 4:
			printf("Parser Error: Procedure declarations should follow the pattern 'ident ;'\n");
			break;
		case 5:
			printf("Parser Error: Variables must be assigned using :=\n");
			break;
		case 6:
			printf("Parser Error: Only variables may be assigned to or read\n");
			break;
		case 7:
			printf("Parser Error: call must be followed by a procedure identifier\n");
			break;
		case 8:
			printf("Parser Error: if must be followed by then\n");
			break;
		case 9:
			printf("Parser Error: while must be followed by do\n");
			break;
		case 10:
			printf("Parser Error: Relational operator missing from condition\n");
			break;
		case 11:
			printf("Parser Error: Arithmetic expressions may only contain arithmetic operators, numbers, parentheses, constants, and variables\n");
			break;
		case 12:
			printf("Parser Error: ( must be followed by )\n");
			break;
		case 13:
			printf("Parser Error: Multiple symbols in variable and constant declarations must be separated by commas\n");
			break;
		case 14:
			printf("Parser Error: Symbol declarations should close with a semicolon\n");
			break;
		case 15:
			printf("Parser Error: Statements within begin-end must be separated by a semicolon\n");
			break;
		case 16:
			printf("Parser Error: begin must be followed by end\n");
			break;
		case 17:
			printf("Parser Error: Bad arithmetic\n");
			break;
		case 18:
			printf("Parser Error: Confliciting symbol declarations\n");
			break;
		case 19:
			printf("Parser Error: Undeclared identifier\n");
			break;
		default:
			printf("Implementation Error: unrecognized error code\n");
			break;
	}
}

void printsymboltable()
{
	int i;
	printf("Symbol Table:\n");
	printf("Kind | Name        | Value | Level | Address | Mark\n");
	printf("---------------------------------------------------\n");
	for (i = 0; i < tIndex; i++)
		printf("%4d | %11s | %5d | %5d | %5d | %5d\n", table[i].kind, table[i].name, table[i].value, table[i].level, table[i].addr, table[i].mark); 
	printf("\n");
}

void printassemblycode()
{
	int i;
	printf("Assembly Code:\n");
	printf("Line\tOP Code\tOP Name\tL\tM\n");
	for (i = 0; i < cIndex; i++)
	{
		printf("%d\t", i);
		printf("%d\t", code[i].op);
		switch (code[i].op)
		{
			case 1:
				printf("LIT\t");
				break;
			case 2:
				switch (code[i].m)
				{
					case 0:
						printf("RTN\t");
						break;
					case 1:
						printf("NEG\t");
						break;
					case 2:
						printf("ADD\t");
						break;
					case 3:
						printf("SUB\t");
						break;
					case 4:
						printf("MUL\t");
						break;
					case 5:
						printf("DIV\t");
						break;
					case 6:
						printf("MOD\t");
						break;
					case 7:
						printf("EQL\t");
						break;
					case 8:
						printf("NEQ\t");
						break;
					case 9:
						printf("LSS\t");
						break;
					case 10:
						printf("LEQ\t");
						break;
					case 11:
						printf("GTR\t");
						break;
					case 12:
						printf("GEQ\t");
						break;
					default:
						printf("err\t");
						break;
				}
				break;
			case 3:
				printf("LOD\t");
				break;
			case 4:
				printf("STO\t");
				break;
			case 5:
				printf("CAL\t");
				break;
			case 6:
				printf("INC\t");
				break;
			case 7:
				printf("JMP\t");
				break;
			case 8:
				printf("JPC\t");
				break;
			case 9:
				printf("SYS\t");
				break;
			default:
				printf("err\t");
				break;
		}
		printf("%d\t%d\n", code[i].l, code[i].m);
	}
	printf("\n");
}