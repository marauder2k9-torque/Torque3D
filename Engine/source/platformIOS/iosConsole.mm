//
//  iosConsole.m
//  TestProject
//
//  Created by ctrlintelligence on 29/07/2023.
//

#include "platformIOS/iosConsole.h"

#include "platform/input/event.h"
#include "core/util/journal/process.h"
#include "console/engineAPI.h"
#include "core/strings/stringFunctions.h"

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <cstdarg>
#include <fcntl.h>

IOSConsole *iosConsole = NULL;

DefineEngineFunction(enableWinConsole, void, (bool _enable),,"enableWinConsole(bool);")
{
   if(iosConsole)
   {
      iosConsole->enable(_enable);
      iosConsole->enableInput(_enable);
   }
}

void IOSConsole::create()
{
   if(iosConsole == NULL)
      iosConsole = new IOSConsole();
}

void IOSConsole::destroy()
{
   if(iosConsole && iosConsole->isEnabled())
      iosConsole->enable(false);
   
   delete iosConsole;
   
   iosConsole = NULL;
}

void IOSConsole::enable(bool enabled)
{
   if(enabled && !iosConsoleEnabled)
   {
      iosConsoleEnabled = true;
      
      printf("%s", Con::getVariable("Con::Prompt"));
   }
   else if(!enabled && iosConsoleEnabled)
   {
      iosConsoleEnabled = false;
      printf("Deactivating Console.");
   }
}

void IOSConsole::enableInput(bool enabled)
{
   iosConsoleInputEnabled = enabled;
}

bool IOSConsole::isEnabled()
{
   if(iosConsole)
      return iosConsole->iosConsoleEnabled;
   
   return false;
}

static void iosConsoleConsumer(U32, const char* line)
{
   iosConsole->processConsoleLine(line);
}

IOSConsole::IOSConsole()
{
   for(S32 i = 0; i < MAX_CMDS; i ++)
   {
      rgCmds[i][0] = '\0';
   }
   
   inbuf[0] = 0x00;
   
   iCmdIndex = 0;
   inpos = 0;
   
   iosConsoleEnabled = false;
   iosConsoleInputEnabled = false;
   lineOutput = false;
   Con::addConsumer(iosConsoleConsumer);
}

IOSConsole::~IOSConsole()
{
   Con::removeConsumer(iosConsoleConsumer);
   
   if(iosConsoleEnabled)
      enable(false);
   
   
}

void IOSConsole::processConsoleLine(const char *consoleLine)
{
   if(iosConsoleEnabled)
   {
      if(lineOutput)
         printf("%s\n", consoleLine);
      else
      {
         printf("%c[2k", 27);
         printf("%c%s\n%s%s", '\r', consoleLine, Con::getVariable("Con::Prompt"), inbuf);
      }
   }
}

void IOSConsole::printf(const char *s, ...)
{
   static const int BufSize = 4096;
   static char buffer[BufSize];
   
   va_list args;
   va_start(args, s);
   
   vsnprintf(buffer, BufSize, s, args);
   
   va_end(args);
   
   char *pos = buffer;
   while (*pos) {
      if(*pos == '\t')
         *pos = '^';
      
      pos++;
   }
   
   Con::stripColorChars(buffer);
   //write(stdout, buffer, strlen(buffer));
   
   fflush(stdout);
}

void IOSConsole::process()
{
   
}
