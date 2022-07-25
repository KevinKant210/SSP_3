/*
	Kevin Kant and Caitlin Fabian
	PL/0 Parser
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"

#define false 1
#define true 0

lexeme *tokens;
int lIndex = 0;
instruction *code;
int cIndex = 0;
symbol *table;
int tIndex = 0;

// global level variable
int level;
// value to check if an error occured during parsing
//  0 = true, error occured 1 = false no eror
int hasError = false;

void emit(int opname, int level, int mvalue);
void addToSymbolTable(int k, char n[], int v, int l, int a, int m);
void mark();
int multipledeclarationcheck(char name[]);
int findsymbol(char name[], int kind);

void printparseerror(int err_code);
void printsymboltable();
void printassemblycode();

void constDec();
int varDec();
void procDec();
void statement();

void assign(lexeme identifier);
void call();
void begin();
void ifStatement();
void whileStatement();
void readStatement(lexeme identifier);
void writeStatement();

void condition();
void expression();
void term(lexeme currToken);
void factor(lexeme factToken);

// helper functions

// function to grab next token on the list
lexeme getToken()
{

	lexeme curr = tokens[lIndex];
	lIndex++;

	return curr;
}

// function to push a token back onto the list
void pushToken()
{
	// when looking at token did not get what we expected so we put it back on top
	lIndex = lIndex - 1;
}

// our functions for the grammar
void block()
{
	// obligatory check to ensure we didnt enter this function with an error
	if (hasError == true)
	{
		return;
	}

	// declaration of block vars
	level = level + 1;
	int currProcIndex = tIndex - 1;
	int numVars = 0;

	lexeme currToken = getToken();

	// check to see if we are running into a const symbol

	while (currToken.type == constsym)
	{
		constDec();
		currToken = getToken();
	}

	// if an error came back after declaration we return
	if (hasError == true)
	{
		return;
	}

	// declare variables
	while (currToken.type == varsym)
	{
		numVars += varDec();
		currToken = getToken();
	}

	// if an error came back return

	if (hasError == true)
	{
		return;
	}

	// declare our procedures
	while (currToken.type == procsym)
	{
		procDec();
		currToken = getToken();
	}

	// if an error came back return

	if (hasError == true)
	{
		return;
	}

	// we know that we are done with declaration and running into statements
	// so we store the addres of the current procdure here and then emit the INC code

	table[currProcIndex].addr = cIndex;
	emit(6, 0, numVars + 3);

	// we can now parse the code segment

	statement(currToken);

	if (hasError == true)
	{
		return;
	}

	// once done parsing the code we need to mark all the variables on this procedures level before we return
	mark();

	// decrement global level variable then return
	level = level - 1;

	return;
}

// constant declarations
void constDec()
{
	if (hasError == true)
	{
		return;
	}

	// grab the next token and begin declaration loop.
	lexeme currToken = getToken();

	int halt = false;

	while (halt != true)
	{
		// identifier -> become sym -> number ->comma or semicolon
		if (currToken.type != identsym)
		{
			// expected ident sym got something else
			printparseerror(2);
			hasError = true;
			return;
		}

		if (multipledeclarationcheck(currToken.name) != -1)
		{
			// error when we have mult declarations
			printparseerror(18);
			hasError = true;
			return;
		}

		// if no errors we can go about saving the const
		lexeme var = currToken;

		// check to ensure becomes is there
		currToken = getToken();

		if (currToken.type != becomessym)
		{
			printparseerror(2);
			hasError = true;
			return;
		}

		// check to ensure number is there
		currToken = getToken();

		if (currToken.type != numbersym)
		{
			printparseerror(2);
			hasError = true;
			return;
		}

		// const dec has no errors so add it to the symbol table
		addToSymbolTable(1, var.name, currToken.value, level, 0, 0);

		// check if there are more symbols to dec or if we are done
		currToken = getToken();

		if (currToken.type == semicolonsym)
		{
			halt = true;
		}
		else if (currToken.type == commasym)
		{
			currToken = getToken();
		}
		else if (currToken.type == identsym)
		{
			printparseerror(13);
			hasError = true;
			return;
		}
		else
		{
			// printf("hi 1");
			printparseerror(14);
			hasError = true;
			return;
		}
	}

	return;
}
// variable declarations
int varDec()
{
	if (hasError == true)
	{
		return 0;
	}
	int numVars = 0;
	// vars will be offset by 3 in symbol table

	lexeme currToken = getToken();

	int halt = false;

	while (halt != true)
	{
		// [ "var "ident {"," ident} “;"].
		if (currToken.type != identsym)
		{
			// expected ident sym got something else
			printparseerror(3);
			hasError = true;
			return numVars;
		}

		if (multipledeclarationcheck(currToken.name) != -1)
		{

			printparseerror(18);
			hasError = true;
			return numVars;
		}

		// var was declared properly so add it to symbol table
		addToSymbolTable(2, currToken.name, 0, level, numVars + 3, 0);
		// keep track of how many variables have been declared in this procedure
		numVars++;

		currToken = getToken();

		// logic to see if we are done declaring / have an error

		if (currToken.type == commasym)
		{

			currToken = getToken();
		}
		else if (currToken.type == semicolonsym)
		{

			halt = true;
		}
		else if (currToken.type == identsym)
		{

			printparseerror(13);
			hasError = true;
			return numVars;
		}
		else
		{

			printparseerror(14);
			hasError = true;
			return numVars;
		}
	}

	return numVars;
}
// procedure declarations
void procDec()
{
	if (hasError == true)
	{
		return;
	}

	lexeme currToken = getToken();

	if (currToken.type != identsym)
	{
		printparseerror(4);
		hasError = true;
		return;
	}

	if (multipledeclarationcheck(currToken.name) != -1)
	{
		printparseerror(18);
		hasError = true;
		return;
	}

	lexeme proc = currToken;
	currToken = getToken();

	if (currToken.type != semicolonsym)
	{
		printparseerror(4);
		hasError = true;
		return;
	}
	// figure out what address we need to put for our procedures later

	addToSymbolTable(3, proc.name, 0, level, 0, 0);

	block();

	if (hasError == true)
	{
		return;
	}

	currToken = getToken();

	if (currToken.type != semicolonsym)
	{

		printparseerror(14);
		hasError = true;
		return;
	}

	// Putting a return on the code stack
	emit(2, 0, 0);

	return;
}

void statement(lexeme curr)
{
	if (hasError == true)
	{
		return;
	}

	lexeme currToken = curr;

	// assignment statements (“ident := expression”)
	if (currToken.type == identsym)
	{
		assign(currToken);
	}
	else if (currToken.type == callsym)
	{
		call();
	}
	else if (currToken.type == beginsym)
	{
		begin();
	}
	else if (currToken.type == ifsym)
	{
		ifStatement();
	}
	else if (currToken.type == whilesym)
	{
		whileStatement();
	}
	else if (currToken.type == writesym)
	{

		writeStatement();
	}
	else if (currToken.type == readsym)
	{
		currToken = getToken();
		readStatement(currToken);
	}
	else
	{
		// ran into unknown symbol return it to the list and return
		pushToken();
	}

	return;
}

// Statement functions
void assign(lexeme identifier)
{
	if (hasError == true)
	{
		return;
	}

	// check to see if the variable exists
	int identIndex = findsymbol(identifier.name, 2);

	if (identIndex == -1)
	{
		// it exists but under a different kind
		if (findsymbol(identifier.name, 3) != -1 || findsymbol(identifier.name, 1) != 1)
		{
			printparseerror(6);
			hasError = true;
			return;
		}
		// it doesnt exist at all
		printparseerror(19);
		hasError = true;
		return;
	}

	lexeme currToken = getToken();

	// check for grammar errors

	if (currToken.type != becomessym)
	{
		printparseerror(5);
		hasError = true;
		return;
	}

	expression();

	if (hasError == true)
	{
		return;
	}
	// we then put a store on code stack
	emit(4, level - table[identIndex].level, table[identIndex].addr);
}

void call()
{
	if (hasError == true)
	{
		return;
	}

	lexeme currToken = getToken();

	// Check to make sure the identifier matches a procedure in the symbol table
	int symIdx = findsymbol(currToken.name, 3);

	if (symIdx == -1)
	{
		// found a match but its of the incorrect type
		if (findsymbol(currToken.name, 1) != -1 || findsymbol(currToken.name, 2) != -1 || currToken.type != identsym)
		{
			printparseerror(7);
			hasError = true;
			return;
		}
		// didnt find anything
		printparseerror(19);
		hasError = true;
		return;
	}

	emit(5, level - table[symIdx].level, symIdx);
}

void begin()
{
	if (hasError == true)
	{
		return;
	}

	lexeme currToken;

	// statement loop while we have semicolons
	do
	{
		currToken = getToken();
		statement(currToken);

		if (hasError == true)
		{
			return;
		}

		currToken = getToken();
	} while (currToken.type == semicolonsym);

	if (hasError == true)
	{
		return;
	}

	if (currToken.type != endsym)
	{

		switch (currToken.type)
		{
		case identsym:
			printparseerror(15);
			break;

		case readsym:
			printparseerror(15);
			break;

		case writesym:
			printparseerror(15);
			break;

		case beginsym:
			printparseerror(15);
			break;

		case callsym:
			printparseerror(15);
			break;

		case ifsym:
			printparseerror(15);
			break;

		case whilesym:
			printparseerror(15);
			break;

		default:

			printparseerror(16);
		}

		hasError = true;

		return;
	}

	return;
}

void ifStatement()
{
	if (hasError == true)
	{
		return;
	}

	condition();

	if (hasError == true)
	{
		return;
	}

	int jpcIdx = cIndex;

	lexeme then = getToken();

	// If there is no then after the if ifsym
	if (then.type != thensym)
	{
		printparseerror(8);
		hasError = true;
		return;
	}

	// putting a jmpC here to tell where to jump if the condition is true
	emit(8, 0, 0);
	lexeme currToken = getToken();
	statement(currToken);

	if (hasError == true)
	{
		return;
	}

	int jmpIdx = cIndex;
	currToken = getToken();

	if (currToken.type == elsesym)
	{
		jmpIdx = cIndex;
		// putting a jmp here for when the condition is false

		emit(7, 0, 0);
		code[jpcIdx].m = cIndex * 3;

		currToken = getToken();
		statement(currToken);

		if (hasError == true)
		{
			return;
		}

		code[jmpIdx].m = cIndex * 3;
	}
	else
	{
		// there is no else
		pushToken();
		code[jpcIdx].m = cIndex * 3;
	}
}

void whileStatement()
{

	if (hasError == true)
	{
		return;
	}

	int loopIdx = cIndex;

	condition();

	if (hasError == true)
	{
		return;
	}

	lexeme dostatement = getToken();
	if (dostatement.type != dosym)
	{
		printparseerror(9);
		hasError = true;
		return;
	}

	// jmp on condition is false we go to after loop
	int jpcIdx = cIndex;
	emit(8, 0, 0);

	lexeme currToken = getToken();
	statement(currToken);

	if (hasError == true)
	{
		return;
	}

	emit(7, 0, loopIdx * 3);

	// in psuedo code no times 3 but i think its an error
	code[jpcIdx].m = cIndex * 3;
}

void readStatement(lexeme identifier)
{

	int symIdx = findsymbol(identifier.name, 2);

	if (symIdx == -1)
	{
		// check if it exists under the wrong symbol
		if (findsymbol(identifier.name, 1) != -1 || findsymbol(identifier.name, 2) != -1 || identifier.type != identsym)
		{
			printparseerror(6);
			hasError = true;
			return;
		}
		else
		{
			printparseerror(19);
			hasError = true;
			return;
		}
	}

	emit(9, 0, 2);
	emit(4, level - table[symIdx].level, table[symIdx].addr);

	return;
}

void writeStatement()
{
	expression();
	emit(9, 0, 1);
}

void condition()
{

	expression();

	lexeme currToken = getToken();

	// =
	if (currToken.type == eqlsym)
	{
		expression();
		emit(2, 0, 7);
	}
	// !=
	else if (currToken.type == neqsym)
	{
		expression();
		emit(2, 0, 8);
	}
	// <
	else if (currToken.type == lessym)
	{
		expression();
		emit(2, 0, 9);
	}
	// <=
	else if (currToken.type == leqsym)
	{
		expression();
		emit(2, 0, 10);
	}
	// >
	else if (currToken.type == gtrsym)
	{
		expression();
		emit(2, 0, 11);
	}
	// >=
	else if (currToken.type == geqsym)
	{
		expression();
		emit(2, 0, 12);
	}
	else
	{
		printparseerror(10);
		hasError = true;
		return;
	}
}

// used in assignment statements and write statements
void expression()
{
	//["-"] term { ("+"|"-") term}.

	lexeme currToken = getToken();

	// if a var is getting assigned a negative expression
	if (currToken.type == minussym)
	{
		currToken = getToken();
		term(currToken);
		// NEG
		emit(2, 0, 1);
	}
	else
	{
		// if a var is getting assigned anything else

		term(currToken);
	}

	currToken = getToken();

	while (currToken.type == plussym || currToken.type == minussym)
	{
		if (currToken.type == plussym)
		{
			currToken = getToken();
			term(currToken);
			// ADD
			emit(2, 0, 2);
		}
		else
		{
			currToken = getToken();
			term(currToken);
			// SUB
			emit(2, 0, 3);
		}

		currToken = getToken();
	}

	if (hasError == true)
	{
		return;
	}

	// bad arithmetic error check
	if (currToken.type == identsym || currToken.type == numbersym || currToken.type == lparentsym)
	{
		printparseerror(17);
		hasError = true;
		return;
	}

	pushToken();
	return;
}

void term(lexeme currToken)
{

	factor(currToken);

	currToken = getToken();

	while (currToken.type == multsym || currToken.type == slashsym || currToken.type == modsym)
	{
		if (currToken.type == multsym)
		{
			currToken = getToken();
			factor(currToken);
			emit(2, 0, 4);
		}
		else if (currToken.type == slashsym)
		{
			currToken = getToken();
			factor(currToken);
			emit(2, 0, 5);
		}
		else if (currToken.type == modsym)
		{
			currToken = getToken();
			factor(currToken);
			emit(2, 0, 6);
		}

		currToken = getToken();
	}

	pushToken();

	return;
}

void factor(lexeme factToken)
{

	if (factToken.type == identsym)
	{

		int symIdx = findsymbol(factToken.name, 2);

		if (symIdx == -1)
		{
			symIdx = findsymbol(factToken.name, 1);
			if (symIdx == -1)
			{
				// not a variable or constant
				symIdx = findsymbol(factToken.name, 3);

				if (symIdx == -1)
				{
					printparseerror(19);
					hasError = true;
					return;
				}
				else
				{

					printparseerror(11);
					hasError = true;
					return;
				}
			}
		}

		if (table[symIdx].kind == 1)
		{
			// then the identifier is a constant
			emit(1, 0, table[symIdx].value);
		}
		else if (table[symIdx].kind == 2)
		{
			// then the identifier is a variable
			emit(3, level - table[symIdx].level, table[symIdx].addr);
		}

		return;
	}
	// if var is being assigned a number
	else if (factToken.type == numbersym)
	{
		emit(1, 0, factToken.value);
		return;
	}
	else if (factToken.type == lparentsym)
	{
		expression();

		if (hasError == true)
		{
			return;
		}

		factToken = getToken();
		if (factToken.type != rparentsym)
		{
			printparseerror(12);
			hasError = true;
			return;
		}
	}
	else
	{

		printparseerror(11);
		hasError = true;
		return;
	}
}

instruction *parser_code_generator(lexeme *list)
{
	int i;
	tokens = list;
	code = malloc(sizeof(instruction) * MAX_ARRAY_SIZE);
	table = malloc(sizeof(symbol) * MAX_ARRAY_SIZE);

	// Your code here.
	// Make sure to remember to free table before returning always
	// and to free code and return NULL if there was an error

	// this is program from the psuedo code

	// put a jump with m = 0 on the code stack
	emit(7, 0, 0);
	addToSymbolTable(3, "main", 0, 0, 0, 0);
	level = -1;

	block();

	if (hasError == true)
	{
		free(code);
		free(table);
		return NULL;
	}
	// put halt on the code stack
	emit(9, 0, 3);

	// check to ensure there is a period at the end of program

	// case where there is no symbol at the end period included
	if (sizeof(list) == sizeof(lexeme) * lIndex)
	{

		printparseerror(1);
		hasError = true;
	}
	else
	{ // case where there is a symbol and its not a period
		lexeme currToken = getToken();
		if (currToken.type != periodsym)
		{

			printparseerror(1);
			hasError = true;
		}
	}

	if (hasError == true)
	{
		free(code);
		free(table);
		return NULL;
	}
	// need to go through and fix all the CAL functions and the initial jump function

	// fixing main jump location

	code[0].m = table[0].addr * 3;

	// fixing calls

	for (int i = 0; i < cIndex; i++)
	{
		if (code[i].op == 5)
		{
			code[i].m = table[code[i].m].addr * 3;
		}
	}

	code[cIndex].op = -1;
	printsymboltable();
	printassemblycode();
	free(table);
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