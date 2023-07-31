//
//  iosConsole.m
//  TestProject
//
//  Created by ctrlintelligence on 29/07/2023.
//

#include "platformIOS/iosConsole.h"
#include "platformIOS/iosUtils.h"
#include "platform/input/event.h"
#include "core/util/journal/process.h"
#include "console/engineAPI.h"
#include "core/util/rawData.h"
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
   for (S32 iIndex = 0; iIndex < MAX_CMDS; iIndex ++)
      rgCmds[iIndex][0] = '\0';

   stdOut = dup(1);
   stdIn  = dup(0);
   stdErr = dup(2);

   // Ensure in buffer is null terminated after allocation
   inbuf[0] = 0x00;

   iCmdIndex = 0;
   iosConsoleEnabled = false;
   iosConsoleInputEnabled = false;
   Con::addConsumer(iosConsoleConsumer);
   inpos = 0;
   lineOutput = false;
   inBackground = false;
   originalTermState = NULL;
   
   Process::notify(this, &IOSConsole::process, PROCESS_LAST_ORDER);
}

IOSConsole::~IOSConsole()
{
   Con::removeConsumer(iosConsoleConsumer);

   if (iosConsoleEnabled)
      enable(false);

   if (originalTermState != NULL)
   {
      delete originalTermState;
      originalTermState = NULL;
   }
}

void IOSConsole::printf(const char *s, ...)
{
   // Get the line into a buffer.
   static const int BufSize = 4096;
   static char buffer[BufSize];
   va_list args;
   va_start(args, s);
   vsnprintf(buffer, BufSize, s, args);
   va_end(args);
   // Replace tabs with carats, like the "real" console does.
   char *pos = buffer;
   while (*pos) {
      if (*pos == '\t') {
         *pos = '^';
      }
      pos++;
   }
   // Axe the color characters.
   Con::stripColorChars(buffer);
   // Print it.
   write(stdOut, buffer, strlen(buffer));
   fflush(stdout);
}

void IOSConsole::processConsoleLine(const char *consoleLine)
{
   if(iosConsoleEnabled)
   {
      if(lineOutput)
         printf("%s\n", consoleLine);
      else
      {
          // Clear current line before outputting the console line. This prevents prompt text from overflowing onto normal output.
          printf("%c[2K", 27);
          printf("%c%s\n%s%s", '\r', consoleLine, Con::getVariable("Con::Prompt"), inbuf);
      }
   }
}

void IOSConsole::process()
{
   if(iosConsoleEnabled)
   {
      //U16 key;
      char typedData[64];  // damn, if you can type this fast... :-D

      if (IOSUtils->inBackground())
         // we don't have the terminal
         inBackground = true;
      else
      {
         // if we were in the background, reset the terminal
         if (inBackground)
            resetTerminal();
         inBackground = false;
      }

      // see if stdIn has any input waiting
      // mojo for select call
      fd_set rfds;
      struct timeval tv;

      FD_ZERO(&rfds);
      FD_SET(stdIn, &rfds);
      // don't wait at all in select
      tv.tv_sec = 0;
      tv.tv_usec = 0;

      int numEvents = select(stdIn+1, &rfds, NULL, NULL, &tv);
      if (numEvents <= 0)
         // no data available
         return;

      numEvents = read(stdIn, typedData, 64);
      if (numEvents == -1)
         return;

      // TODO LINUX, when debug in qtCreator some times we get false console inputs.
      if( !iosConsoleEnabled )
         return;

      typedData[numEvents] = '\0';
      if (numEvents > 0)
      {
        char outbuf[512];
        S32 outpos = 0;

        for (int i = 0; i < numEvents; i++)
        {
         switch(typedData[i])
         {
            case 8:
            case 127:
            /* backspace */
               if (inpos > 0)
               {
                  outbuf[outpos++] = '\b';
                  outbuf[outpos++] = ' ';
                  outbuf[outpos++] = '\b';
                  inpos--;
               }
               break;

            // XXX Don't know if we can detect shift-TAB.  So, only handling
            // TAB for now.

            case '\t':
                        // In the output buffer, we're going to have to erase the current line (in case
                        // we're cycling through various completions) and write out the whole input
                        // buffer, so (inpos * 3) + complen <= 512.  Should be OK.  The input buffer is
                        // also 512 chars long so that constraint will also be fine for the input buffer.
                        {
                           // Erase the current line.
                           U32 i;
                           for (i = 0; i < inpos; i++) {
                              outbuf[outpos++] = '\b';
                              outbuf[outpos++] = ' ';
                              outbuf[outpos++] = '\b';
                           }
                           // Modify the input buffer with the completion.
                           U32 maxlen = 512 - (inpos * 3);
                           inpos = Con::tabComplete(inbuf, inpos, maxlen, true);
                           // Copy the input buffer to the output.
                           for (i = 0; i < inpos; i++) {
                              outbuf[outpos++] = inbuf[i];
                           }
                        }
                        break;

            case '\n':
            case '\r':
            /* new line */
                  outbuf[outpos++] = '\n';

                  inbuf[inpos] = 0;
                  outbuf[outpos] = 0;
                  printf("%s", outbuf);

                  S32 eventSize;
                  eventSize = 1;

                  {
                     RawData rd;
                     rd.size = inpos + 1;
                     rd.data = (S8*) inbuf;
                     
                     Con::smConsoleInput.trigger(rd);
                  }
                  
                  // If we've gone off the end of our array, wrap
                  // back to the beginning
                  if (iCmdIndex >= MAX_CMDS)
                      iCmdIndex %= MAX_CMDS;

                  // Put the new command into the array
                  strcpy(rgCmds[iCmdIndex ++], inbuf);

                  printf("%s", Con::getVariable("Con::Prompt"));
                  inpos = outpos = 0;
                  inbuf[0] = 0x00; // Ensure inbuf is NULL terminated after sending out command
                  break;
            case 27:
               // JMQTODO: are these magic numbers keyboard map specific?
               if (typedData[i+1] == 91 || typedData[i+1] == 79)
               {
                  i += 2;
                  // an arrow key was pressed.
                  switch(typedData[i])
                  {
                     case 'A':
                        /* up arrow */
                        // Go to the previous command in the cyclic array
                        if ((-- iCmdIndex) < 0)
                           iCmdIndex = MAX_CMDS - 1;

                        // If this command isn't empty ...
                        if (rgCmds[iCmdIndex][0] != '\0')
                        {
                           // Obliterate current displayed text
                           for (S32 i = outpos = 0; i < inpos; i ++)
                           {
                              outbuf[outpos++] = '\b';
                              outbuf[outpos++] = ' ';
                              outbuf[outpos++] = '\b';
                           }

                           // Copy command into command and display buffers
                           for (inpos = 0; inpos < (S32)strlen(rgCmds[iCmdIndex]); inpos++, outpos++)
                           {
                              outbuf[outpos] = rgCmds[iCmdIndex][inpos];
                              inbuf [inpos ] = rgCmds[iCmdIndex][inpos];
                           }
                        }
                        // If previous is empty, stay on current command
                        else if ((++ iCmdIndex) >= MAX_CMDS)
                        {
                           iCmdIndex = 0;
                        }
                        break;
                     case 'B':
                        /* down arrow */
                        // Go to the next command in the command array, if
                        // it isn't empty
                        if (rgCmds[iCmdIndex][0] != '\0' && (++ iCmdIndex) >= MAX_CMDS)
                           iCmdIndex = 0;

                        // Obliterate current displayed text
                        for (S32 i = outpos = 0; i < inpos; i ++)
                        {
                           outbuf[outpos++] = '\b';
                           outbuf[outpos++] = ' ';
                           outbuf[outpos++] = '\b';
                        }

                        // Copy command into command and display buffers
                        for (inpos = 0; inpos < (S32)strlen(rgCmds[iCmdIndex]); inpos++, outpos++)
                        {
                           outbuf[outpos] = rgCmds[iCmdIndex][inpos];
                           inbuf [inpos ] = rgCmds[iCmdIndex][inpos];
                        }
                        break;
                     case 'C':
                        /* right arrow */
                        break;
                     case 'D':
                        /* left arrow */
                        break;
                  }
                  // read again to get rid of a bad char.
                  //read(stdIn, &key, sizeof(char));
                  break;
               } else {
                  inbuf[inpos++] = typedData[i];
                  outbuf[outpos++] = typedData[i];
                  break;
               }
               break;
            default:
               inbuf[inpos++] = typedData[i];
               outbuf[outpos++] = typedData[i];
               break;
         }
        }
        if (outpos)
        {
           outbuf[outpos] = 0;
           printf("%s", outbuf);
        }
      }
   }
}
