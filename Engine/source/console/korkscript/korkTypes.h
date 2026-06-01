#pragma once

namespace KorkScript
{
   enum class TypeReq
   {
      None,    ///< No preference — use whatever the node naturally produces
      UInt,    ///< Produce an unsigned integer value
      Float,   ///< Produce a float value
      String,  ///< Produce a string value
   };
}
