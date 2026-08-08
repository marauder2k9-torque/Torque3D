//-----------------------------------------------------------------------------
// gui_rev2/core/newGuiTypes.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "gui_rev2/core/newGuiTypes.h"

// Accepted syntax: "auto", "50%", "128", "128px".
bool NewGuiDimension::setFromString(NewGuiDimension& out, const char* str)
{
   if (!str || !str[0])
   {
      out = NewGuiDimension::fromAuto();
      return true;
   }

   if (dStricmp(str, "auto") == 0)
   {
      out = NewGuiDimension::fromAuto();
      return true;
   }

   // Find a trailing '%' or "px" suffix; everything before it is the
   // numeric value.
   const U32 len = dStrlen(str);
   if (str[len - 1] == '%')
   {
      out = NewGuiDimension::fromPercent((F32)dAtof(str));
      return true;
   }

   out = NewGuiDimension::fromPixels((F32)dAtof(str));
   return true;
}

const char* NewGuiDimension::toString(const NewGuiDimension& dim)
{
   static const U32 bufSize = 32;
   char* buf = Con::getReturnBuffer(bufSize);

   switch (dim.mode)
   {
   case NewGuiDimension::Auto:
      dSprintf(buf, bufSize, "auto");
      break;
   case NewGuiDimension::Percent:
      dSprintf(buf, bufSize, "%g%%", dim.value);
      break;
   case NewGuiDimension::Pixels:
   default:
      dSprintf(buf, bufSize, "%g", dim.value);
      break;
   }

   return buf;
}

// Accepted syntax (CSS shorthand): "8", "8 16", "8 16 4", "8 16 4 2".
bool NewGuiEdgeInsets::setFromString(NewGuiEdgeInsets& out, const char* str)
{
   if (!str || !str[0])
   {
      out = NewGuiEdgeInsets();
      return true;
   }

   F32 values[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
   S32 count = dSscanf(str, "%g %g %g %g", &values[0], &values[1], &values[2], &values[3]);

   switch (count)
   {
   case 1:
      out = NewGuiEdgeInsets(values[0]);
      return true;
   case 2:
      out = NewGuiEdgeInsets(values[0], values[1], values[0], values[1]);
      return true;
   case 3:
      out = NewGuiEdgeInsets(values[0], values[1], values[2], values[1]);
      return true;
   case 4:
      out = NewGuiEdgeInsets(values[0], values[1], values[2], values[3]);
      return true;
   default:
      out = NewGuiEdgeInsets();
      return false;
   }
}

const char* NewGuiEdgeInsets::toString(const NewGuiEdgeInsets& insets)
{
   static const U32 bufSize = 64;
   char* buf = Con::getReturnBuffer(bufSize);
   dSprintf(buf, bufSize, "%g %g %g %g", insets.top, insets.right, insets.bottom, insets.left);
   return buf;
}

//-----------------------------------------------------------------------------
// Console type registration
//-----------------------------------------------------------------------------
IMPLEMENT_STRUCT(NewGuiDimension,
   NewGuiDimension, ,
   "")

   //FIELD(left, leftPadding, 1, "")
   //FIELD(right, rightPadding, 1, "")
   //FIELD(top, topPadding, 1, "")
   //FIELD(bottom, bottomPadding, 1, "")

   END_IMPLEMENT_STRUCT;

ConsoleType(NewGuiDimension, TypeNewGuiDimension, NewGuiDimension, "")
ImplementConsoleTypeCasters(TypeNewGuiDimension, NewGuiDimension)

ConsoleGetType(TypeNewGuiDimension)
{
   const NewGuiDimension* dim = (const NewGuiDimension*)dptr;
   return NewGuiDimension::toString(*dim);
}

ConsoleSetType(TypeNewGuiDimension)
{
   NewGuiDimension* dim = (NewGuiDimension*)dptr;
   if (argc == 1)
   {
      NewGuiDimension parsed;
      if (NewGuiDimension::setFromString(parsed, argv[0]))
         *dim = parsed;
   }
   else
      Con::printf("(TypeNewGuiDimension) Cannot set multiple args to a single dimension.");
}

IMPLEMENT_STRUCT(NewGuiEdgeInsets,
   NewGuiEdgeInsets, ,
   "")

   //FIELD(left, leftPadding, 1, "")
   //FIELD(right, rightPadding, 1, "")
   //FIELD(top, topPadding, 1, "")
   //FIELD(bottom, bottomPadding, 1, "")

   END_IMPLEMENT_STRUCT;
ConsoleType(NewGuiEdgeInsets, TypeNewGuiEdgeInsets, NewGuiEdgeInsets, "")
ImplementConsoleTypeCasters(TypeNewGuiEdgeInsets, NewGuiEdgeInsets)

ConsoleGetType(TypeNewGuiEdgeInsets)
{
   const NewGuiEdgeInsets* insets = (const NewGuiEdgeInsets*)dptr;
   return NewGuiEdgeInsets::toString(*insets);
}

ConsoleSetType(TypeNewGuiEdgeInsets)
{
   NewGuiEdgeInsets* insets = (NewGuiEdgeInsets*)dptr;
   if (argc == 1)
   {
      NewGuiEdgeInsets parsed;
      if (NewGuiEdgeInsets::setFromString(parsed, argv[0]))
         *insets = parsed;
   }
   else if (argc == 4)
   {
      insets->top = (F32)dAtof(argv[0]);
      insets->right = (F32)dAtof(argv[1]);
      insets->bottom = (F32)dAtof(argv[2]);
      insets->left = (F32)dAtof(argv[3]);
   }
   else
      Con::printf("(TypeNewGuiEdgeInsets) Expects 1 (shorthand string) or 4 (t r b l) args.");
}
