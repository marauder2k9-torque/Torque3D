//-----------------------------------------------------------------------------
// gui_rev2/core/newGuiTypes.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUITYPES_H_
#define _NEWGUITYPES_H_

#ifndef _MPOINT2_H_
#include "math/mPoint2.h"
#endif
#ifndef _MRECT_H_
#include "math/mRect.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif

/// A single authored axis value: pixels, percent, or auto (size to content).
///
/// @code
/// NewGuiDimension w = NewGuiDimension::fromPercent(50.0f);
/// @endcode
struct NewGuiDimension
{
   enum Mode : U8
   {
      Auto = 0,      ///< Size to preferred content.
      Pixels,        ///< Absolute logical-unit value.
      Percent        ///< Percentage of the reference length.
   };

   Mode  mode;
   F32   value;       ///< Meaningless when mode == Auto.

   NewGuiDimension() : mode(Auto), value(0.0f) {}
   NewGuiDimension(Mode m, F32 v) : mode(m), value(v) {}

   static NewGuiDimension fromAuto() { return NewGuiDimension(Auto, 0.0f); }
   static NewGuiDimension fromPixels(F32 v) { return NewGuiDimension(Pixels, v); }
   static NewGuiDimension fromPercent(F32 v) { return NewGuiDimension(Percent, v); }

   bool isAuto() const { return mode == Auto; }
   bool isPixels() const { return mode == Pixels; }
   bool isPercent() const { return mode == Percent; }

   bool operator==(const NewGuiDimension& other) const
   {
      return mode == other.mode && (mode == Auto || value == other.value);
   }
   bool operator!=(const NewGuiDimension& other) const { return !(*this == other); }

   /// Parses script syntax: "auto", "50%", "128", "128px".
   /// @param out Receives the parsed value.
   /// @param str Source string.
   /// @return True if parsing succeeded.
   static bool setFromString(NewGuiDimension& out, const char* str);

   /// Renders back to script syntax.
   /// @param dim Value to render.
   /// @return StringTable-backed buffer via Con::getReturnBuffer.
   static const char* toString(const NewGuiDimension& dim);
};

/// Four-sided edge values, in logical units (padding, margin, border widths).
struct NewGuiEdgeInsets
{
   F32 top;
   F32 right;
   F32 bottom;
   F32 left;

   NewGuiEdgeInsets() : top(0.0f), right(0.0f), bottom(0.0f), left(0.0f) {}
   NewGuiEdgeInsets(F32 t, F32 r, F32 b, F32 l) : top(t), right(r), bottom(b), left(l) {}
   explicit NewGuiEdgeInsets(F32 uniform) : top(uniform), right(uniform), bottom(uniform), left(uniform) {}

   /// @return Sum of left and right insets.
   F32 horizontal() const { return left + right; }

   /// @return Sum of top and bottom insets.
   F32 vertical() const { return top + bottom; }

   bool operator==(const NewGuiEdgeInsets& o) const
   {
      return top == o.top && right == o.right && bottom == o.bottom && left == o.left;
   }
   bool operator!=(const NewGuiEdgeInsets& o) const { return !(*this == o); }

   /// Parses script syntax: "top right bottom left".
   /// @param out Receives the parsed value.
   /// @param str Source string.
   /// @return True if parsing succeeded.
   static bool setFromString(NewGuiEdgeInsets& out, const char* str);

   /// Renders back to script syntax.
   /// @param insets Value to render.
   /// @return StringTable-backed buffer via Con::getReturnBuffer.
   static const char* toString(const NewGuiEdgeInsets& insets);
};

/// Interaction state bitmask, computed once per style pass from tree-owned interaction state.
enum NewGuiStyleStateFlag : U32
{
   NewGuiState_Normal = 0,
   NewGuiState_Hover = BIT(0),   ///< Mouse is over the control.
   NewGuiState_Active = BIT(1),   ///< Mouse is down/pressed on the control.
   NewGuiState_Focus = BIT(2),   ///< Control is the first responder.
   NewGuiState_Disabled = BIT(3),   ///< Control is disabled.
   NewGuiState_Checked = BIT(4),   ///< Control is checked (toggle/radio buttons).
   NewGuiState_Error = BIT(5),   ///< Control is in an error state.
};
typedef U32 NewGuiStyleStateMask;

// Console type registration - lets NewGuiDimension/NewGuiEdgeInsets be used directly with ADD_FIELD.
DECLARE_STRUCT(NewGuiDimension);
DefineConsoleType(TypeNewGuiDimension, NewGuiDimension)
DECLARE_STRUCT(NewGuiEdgeInsets);
DefineConsoleType(TypeNewGuiEdgeInsets, NewGuiEdgeInsets)

#endif // _NEWGUITYPES_H_
