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


#ifndef WIN32

#include <seiscomp3/logging/syslog.h>
#include <syslog.h>

#ifndef SYSLOG_FACILITY
#  define SYSLOG_FACILITY LOG_LOCAL0
#endif


namespace Seiscomp {
namespace Logging {


SyslogOutput::SyslogOutput()
	: _openFlag(false) {
}

SyslogOutput::SyslogOutput(const char* ident) 
	: _openFlag(false) {
	SyslogOutput::open(ident);
}

SyslogOutput::~SyslogOutput() {
	SyslogOutput::close();
}

bool SyslogOutput::open(const char* ident) {
	openlog(ident, 0, SYSLOG_FACILITY);
	_openFlag = true;
	return true;
}

void SyslogOutput::close() {
	if ( _openFlag )
		closelog();
	
	_openFlag = false;
}

bool SyslogOutput::isOpen() const {
	return _openFlag;
}

void SyslogOutput::log(const char* channelName,
                       LogLevel level,
                       const char* msg,
                       time_t time) {
	
	int priority = LOG_ALERT;
	switch ( level ) {
		case LL_CRITICAL:
			priority = LOG_CRIT;
			break;
		case LL_ERROR:
			priority = LOG_ERR;
			break;
		case LL_WARNING:
			priority = LOG_WARNING;
			break;
		case LL_NOTICE:
			priority = LOG_NOTICE;
			break;
		case LL_INFO:
			priority = LOG_INFO;
			break;
		case LL_DEBUG:
		case LL_UNDEFINED:
			priority = LOG_DEBUG;
			break;
		default:
			break;
	}

	syslog(priority, "[%s/%s] %s", channelName, component(), msg);
}



}
}

#endif
