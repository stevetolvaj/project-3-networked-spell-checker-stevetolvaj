# Steve Tolvaj, CIS 3207-001, Operating Systems Project 3 - Networked Spell Checker.
all: spell_checker client

spell_checker: spell_checker.c spell_checker.h
	gcc -pthread -Wall -Werror spell_checker.c -o spell_checker

client: client.c spell_checker.h
	gcc -Wall -Werror client.c -o client 
