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

class IOSConsole {
   bool iosConsoleEnabled;
   bool iosConsoleInputEnabled;
   bool inBackground;
   bool lineOutput;
   
   char inbuf[512];
   S32 inpos;
   
   char curTabComplete[512];
   S32 tabCompleteStart;
   char rgCmds[MAX_CMDS][512];
   S32 iCmdIndex;
   
   void printf(const char*s, ...);
public:
   IOSConsole();
   virtual ~IOSConsole();
   void process();
   void enable(bool);
   void enableInput(bool enabled);
   void processConsoleLine(const char *consoleLine);
   
   static void create();
   static void destroy();
   static bool isEnabled();
};

extern IOSConsole *iosConsole;

#endif /* iosConsole_h */
