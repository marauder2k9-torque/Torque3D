cls
@rem --noline --c++ 
bison-flex\win_flex.exe --outfile=tshade.lex.cpp tshade.l
bison-flex\win_bison.exe tshade.y -H --output=tshade.cpp