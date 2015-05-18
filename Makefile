#########################################################
# File: Makefile
# # Description: A makefile for CS344.
# #########################################################

CXX = gcc

SRC1 = smallsh.c
SRCS = ${SRC1}

PROG1 = smallsh 
PROGS = ${PROG1}

default:
	${CXX} ${SRCS} -g -Wall -std=c99 -D_XOPEN_SOURCE -o ${PROG1}

clean:
	rm -rf smallsh 1 junk testdir*
