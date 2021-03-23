#include <vcl.h>
#include <windows.h>

#pragma hdrstop
#pragma argsused

#include <tchar.h>

#include <stdio.h>
#include "hub.h"

extern CRITICAL_SECTION cs_index_dices;

#define QUIT_CMD "quit"

int _tmain(int argc, _TCHAR* argv[]) {
	char input[50];
	printf("[Enter \"quit\" to exit the http server]\n");

	if (!InitializeCriticalSectionAndSpinCount(&cs_index_dices, 0x00000400))
		return -1;

	DataModule1 = new TDataModule1(Application);

	do {
		printf(">>");
		scanf("%49s", input);
	}
	while (strcmp(input, QUIT_CMD));

	delete DataModule1;
	DeleteCriticalSection(&cs_index_dices);

	return 0;
}
