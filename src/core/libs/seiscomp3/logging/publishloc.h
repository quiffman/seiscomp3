/***************************************************************************
 *   Copyright (C) by GFZ Potsdam                                          *
 *                                                                         *
 *   You can redistribute and/or modify this program under the             *
 *   terms of the SeisComP Public License.                                 *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   SeisComP Public License for more details.                             *
 ***************************************************************************/



#ifndef __SC_LOGGING_PUBLISHLOC_H__
#define __SC_LOGGING_PUBLISHLOC_H__

#include <seiscomp3/logging/common.h>

namespace Seiscomp {
namespace Logging {


class SC_CORE_LOGGING_API Channel;
class SC_CORE_LOGGING_API Node;


struct SC_CORE_LOGGING_API PublishLoc {
	~PublishLoc();

	// If the compiler supports printf attribute specification on function
	// pointers, we'll use it here so that the compiler knows to check for
	// proper printf formatting.  If it doesn't support it, then we'll
	// force the check by inserting a bogus inline function..
	//! function to call when we reach the log statement.
	void (*publish)(PublishLoc *, Channel *, const char *format, ...)
	    PRINTF_FP(3,4);

	Node *pub;
	const char *component;
	const char *fileName;
	const char *functionName;
	int lineNum;
	Channel *channel;
};


SC_CORE_LOGGING_API void Register(PublishLoc *loc, Channel *, const char *format, ... ) PRINTF(3,4);


}
}

#endif
