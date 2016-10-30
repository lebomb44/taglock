/*******************************************************************
    Copyright (C) 2009 FreakLabs
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.

    Originally written by Christopher Wang aka Akiba.
    Please post support questions to the FreakLabs forum.

*******************************************************************/
/*!
    \file Cmd.c

    This implements a simple command line interface for the Arduino so that
    its possible to execute individual functions within the sketch. 
*/
/**************************************************************************/
#include <avr/pgmspace.h>
#include "Arduino.h"
#include "HardwareSerial.h"
#include "SerialPrint.h"
#include "Cmd.h"

// command line message buffer and pointer
static uint8_t msg[MAX_MSG_SIZE];
static uint8_t *msg_ptr;

// linked list for command table
static cmd_t *cmd_tbl_list, *cmd_tbl;

// text strings for command prompt (stored in flash)
const char cmd_prompt[] PROGMEM = "> ";
const char cmd_unrecog[] PROGMEM = "Command not recognized. Try 'help' to list commands";

/**************************************************************************/
/*!
    Generate the main command prompt
*/
/**************************************************************************/
void cmd_display()
{
    char buf[100];

    //Serial.print(micros());
    //strcpy_P(buf, cmd_prompt);
    //Serial.print(buf);

    //Serial.print("> ");
    SerialPrint::put_string("> ");

    fflush(stdout);
}

/**************************************************************************/
/*!
    Parse the command line. This function tokenizes the command input, then
    searches for the command table entry associated with the commmand. Once found,
    it will jump to the corresponding function.
*/
/**************************************************************************/
void cmd_parse(char *cmd)
{
    uint8_t argc, i = 0;
    char *argv[MAX_ARG] = {NULL};
    char buf[50] = {0};
    cmd_t *cmd_entry = NULL;

    fflush(stdout);

    // parse the command line statement and break it up into space-delimited
    // strings. the array of strings will be saved in the argv array.
    argv[i] = strtok(cmd, " ");
    do
    {
        argv[++i] = strtok(NULL, " ");
    } while ((i < (MAX_MSG_SIZE/2)) && (argv[i] != NULL));

    if(0 == argv[0])
    {
        cmd_display();
        return;
    }
    
    // save off the number of arguments for the particular command.
    argc = i;

    // parse the command table for valid command. used argv[0] which is the
    // actual command name typed in at the prompt
    for (cmd_entry = cmd_tbl; cmd_entry != NULL; cmd_entry = cmd_entry->next)
    {
        if (!strcmp(argv[0], cmd_entry->cmd))
        {
            cmd_entry->func(argc, argv);
            cmd_display();
            return;
        }
    }

    // command not recognized. print message and re-generate prompt.
    strcpy_P(buf, cmd_unrecog);
    //Serial.println(buf);
    SerialPrint::put_string(buf);
    SerialPrint::put_string("\n\r");
    cmd_display();
}

/**************************************************************************/
/*!
    This function processes the individual characters typed into the command
    prompt. It saves them off into the message buffer unless its a "backspace"
    or "enter" key. 
*/
/**************************************************************************/
void cmd_handler()
{
    char c = Serial.read();

    switch (c)
    {
    case '\r':
    case '\n':
        // terminate the msg and reset the msg ptr. then send
        // it to the handler for processing.
        *msg_ptr = '\0';
        //Serial.println();
	SerialPrint::put_string("\n\r");
        cmd_parse((char *)msg);
        msg_ptr = msg;
        break;
    
    case '\b':
        // backspace 
        //Serial.print(c);
        SerialPrint::put_char(c);
        if (msg_ptr > msg)
        {
            msg_ptr--;
        }
        break;
    
    default:
        // normal character entered. add it to the buffer
        //Serial.print(c);
      	SerialPrint::put_char(c);
        *msg_ptr++ = c;
        break;
    }
}

/**************************************************************************/
/*!
    This function should be set inside the main loop. It needs to be called
    constantly to check if there is any available input at the command prompt.
*/
/**************************************************************************/
void cmdPoll()
{
    while (Serial.available())
    {
        cmd_handler();
    }
}

/**************************************************************************/
/*!
    Initialize the command line interface. This sets the terminal speed and
    and initializes things. 
*/
/**************************************************************************/
void cmdInit(void)
{
    // init the msg ptr
    msg_ptr = msg;

    // init the command table
    cmd_tbl_list = NULL;
}


/**************************************************************************/
/*!
    Add a command to the command table. The commands should be added in
    at the setup() portion of the sketch. 
*/
/**************************************************************************/
void cmdAdd(const char *name, const char *desc, void (*func)(int argc, char **argv))
{
    if(20 <= strnlen(name, 20))
    {
        return;
    }
    if(50 <= strnlen(desc, 50))
    {
        return;
    }

    // alloc memory for command struct
    cmd_tbl = (cmd_t *)malloc(sizeof(cmd_t));

    // alloc memory for command name
    char *cmd_name = (char *)malloc(strlen(name)+1);
    // copy command name
    strcpy(cmd_name, name);
    // terminate the command name
    cmd_name[strlen(name)] = '\0';

    // alloc memory for command description
    char *cmd_desc = (char *)malloc(strlen(desc)+1);
    // copy command description
    strcpy(cmd_desc, desc);
    // terminate the command description
    cmd_desc[strlen(desc)] = '\0';

    // fill out structure
    cmd_tbl->cmd = cmd_name;
    cmd_tbl->desc = cmd_desc;
    cmd_tbl->func = func;
    cmd_tbl->next = cmd_tbl_list;
    cmd_tbl_list = cmd_tbl;
}

void cmdStart(void)
{
    cmd_display();
}

/**************************************************************************/
/*!
    List all commands registered in the table. 
*/
/**************************************************************************/
void cmdList(void)
{
    cmd_t *cmd_entry = NULL;
    for (cmd_entry = cmd_tbl; cmd_entry != NULL; cmd_entry = cmd_entry->next)
    {
      //Serial.print(cmd_entry->cmd);
      SerialPrint::put_string(cmd_entry->cmd);
      //Serial.print(" : ");
      SerialPrint::put_string(" : ");
      //Serial.println(cmd_entry->desc);
      SerialPrint::put_string(cmd_entry->desc);
      SerialPrint::put_string("\n\r");
    }
}

/**************************************************************************/
/*!
    Convert a string to a number. The base must be specified, ie: "32" is a
    different value in base 10 (decimal) and base 16 (hexadecimal).
*/
/**************************************************************************/
uint32_t cmdStr2Num(char *str, uint8_t base)
{
    return strtol(str, NULL, base);
}

