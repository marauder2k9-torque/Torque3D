//
//  iosConsole.h
//  TestProject
//
//  Created by ctrlintelligence on 29/07/2023.
//

#ifndef iosConsole_h
#define iosConsole_h

#define MAX_CMDS 10

#ifndef _CONSOLE_H_
#include "console/console.h"
#endif

#ifndef _EVENT_H_
#include "platform/input/event.h"
#endif

#include <termios.h>

class IOSConsole {
   bool iosConsoleEnabled;
   bool iosConsoleInputEnabled;
   bool inBackground;
   
   int stdOut;
   int stdIn;
   int stdErr;
   char inbuf[512];
   S32 inpos;
   bool lineOutput;
   char curTabComplete[512];
   S32 tabCompleteStart;
   char rgCmds[MAX_CMDS][512];
   S32 iCmdIndex;
   
   struct termios *originalTermState;
   
   void printf(const char*s, ...);
public:
   IOSConsole();
   virtual ~IOSConsole();
   void enable(bool);
   void enableInput(bool enabled);
   void processConsoleLine(const char *consoleLine);
   void process();
   static void create();
   static void destroy();
   static bool isEnabled();
   void resetTerminal();
};

extern IOSConsole *iosConsole;

#endif /* iosConsole_h */
