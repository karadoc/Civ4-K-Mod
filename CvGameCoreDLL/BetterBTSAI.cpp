#include "CvGameCoreDLL.h"

#include "BetterBTSAI.h"

// AI decision making logging

void logBBAI(char* format, ... )
{
#ifdef LOG_AI
	static char buf[2048];
	_vsnprintf( buf, 2048-4, format, (char*)(&format+1) );
	gDLL->logMsg("BBAI.log", buf);
#endif
}