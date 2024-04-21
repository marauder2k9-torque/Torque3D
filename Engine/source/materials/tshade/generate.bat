cls
@rem --noline --c++ 
bison-flex\win_bison.exe tshade.y --header=tshade.h --output=tshade.cpp
bison-flex\win_flex.exe --outfile=tshade.lex.cpp tshade.l