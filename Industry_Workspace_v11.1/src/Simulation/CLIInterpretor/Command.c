#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Ws2tcpip.h>
#include <windows.h>
#include <stdbool.h>
#include <signal.h>
#include "main.h"
#include "CLI.h"
#include "CLIInterface.h"

ptrCOMMANDARRAY get_commandArray(char *text)
{

	int count = 0;
	int index = 0;
	char **words = NULL;
	char word[BUFSIZ];
	memset(word, 0, BUFSIZ);

	ptrCOMMANDARRAY array = calloc(1, sizeof* array);
	array->originalCommand = _strdup(text);

	while (*text)
	{
		if (*text == ' ')
		{
			count++;
			index = 0;
			if (words)
				words = realloc(words, count * sizeof* words);
			else
				words = calloc(1, sizeof* words);
			words[count - 1] = _strdup(word);
			memset(word, 0, BUFSIZ);
		}
		else
		{
			word[index] = *text;
			index++;
		}
		text++;
	}

	count++;
	index = 0;
	if (words)
		words = realloc(words, count * sizeof* words);
	else
		words = calloc(1, sizeof* words);
	words[count - 1] = _strdup(word);
	memset(word, 0, BUFSIZ);

	array->length = count;
	array->commands = words;

	return array;
}

void free_commandArray(ptrCOMMANDARRAY c)
{
	for (int i = 0; i < c->length; i++)
		free(c->commands[i]);
	free(c->commands);
	free(c->originalCommand);
	free(c);
}

_declspec(dllexport) ptrCOMMANDARRAY remove_first_word_from_commandArray(ptrCOMMANDARRAY c)
{
	int i;
	for (i = 0; i < c->length - 1; i++)
	{
		c->commands[i] = _strdup(c->commands[i + 1]);
	}
	free(c->commands[c->length - 1]);
	c->commands[c->length - 1] = NULL;
	c->length--;
	return c;
}
