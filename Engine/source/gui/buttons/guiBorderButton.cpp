//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"

#include "console/console.h"
#include "gui/buttons/guiButtonBaseCtrl.h"


/// @deprecated 
class GuiBorderButtonCtrl : public GuiButtonBaseCtrl
{
   typedef GuiButtonBaseCtrl Parent;

public:
   DECLARE_CONOBJECT(GuiBorderButtonCtrl);

   GuiBorderButtonCtrl();
};

IMPLEMENT_CONOBJECT(GuiBorderButtonCtrl);

ConsoleDocClass(GuiBorderButtonCtrl,
   "@brief A push button that renders only a border.\n\n"

   "@deprecated Kept for script/TAML compatibility. All rendering now lives on "
   "GuiButtonBaseCtrl; this class simply sets renderStyle = Border. New content "
   "should use GuiButtonBaseCtrl directly.\n\n"

   "A border button consists of a border rendered along its extents according to the border thickness defined in its profile "
   "(GuiControlProfile::border).  For the border color, a color is selected from the profile according to current button state:\n"

   "- Default state: GuiControlProfile::borderColor\n"
   "- Highlighted (mouse is over the button): GuiControlProfile::fontColorHL\n"
   "- Depressed (mouse button down but not yet released): GuiControlProfile::fontColorSEL\n"

   "@ingroup GuiButtons\n"
);

//-----------------------------------------------------------------------------

GuiBorderButtonCtrl::GuiBorderButtonCtrl()
{
   mRenderStyle = RenderStyleBorder;
}
