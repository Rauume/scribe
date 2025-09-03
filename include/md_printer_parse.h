#include <iostream>
#include <vector>
#include <HardwareSerial.h>
#include "md4c.h"

using namespace std;

extern unsigned int parser_flags;
extern const MD_PARSER parser;
extern HardwareSerial* pPrinter;

int enterBlock(MD_BLOCKTYPE type, void *detail, void *userdata);
int leaveBlock(MD_BLOCKTYPE type, void *detail, void *userdata);
int enterSpan(MD_SPANTYPE type, void *detail, void *userdata);
int leaveSpan(MD_SPANTYPE type, void *detail, void *userdata);
int parseText(MD_TEXTTYPE type, const MD_CHAR *text, MD_SIZE size, void *userdata);
void dispatchMDParser(String text, HardwareSerial &printer);
void sendCommand(int data);
void sendText(const char *data, int size);
void setIndent(int direction);
void sendNewLine();