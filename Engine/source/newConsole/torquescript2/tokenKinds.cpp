#include "newConsole/torquescript2/tokenKinds.h"

namespace newConsole
{
namespace ts2
{

const char* tokenKindName(TokenKind kind)
{
   switch (kind)
   {
#define NC_TOKEN(name) case TokenKind::name: return #name;
#include "newConsole/torquescript2/tokenKinds.inc"
#undef NC_TOKEN
   }
   return "<unknown-token>";
}

} // namespace ts2
} // namespace newConsole
