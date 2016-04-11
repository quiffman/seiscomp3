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


#define SEISCOMP_COMPONENT ArclinkConnection

#include <string>
#include <set>
#include <utility>
#include <limits>
#include <cerrno>
#include <fcntl.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <io.h>
#endif

#include <seiscomp3/logging/log.h>
#include <seiscomp3/core/strings.h>
#include "arclink.h"
// HACK to retrieve the record length
#include <libmseed.h>

#ifdef WIN32
#undef min
#undef max
#define posix_read _read
typedef int ssize_t;
#else
#define posix_read read
#endif

namespace Seiscomp {
namespace RecordStream {
namespace Arclink {
namespace _private {

using namespace std;
using namespace Seiscomp;
using namespace Seiscomp::Core;
using namespace Seiscomp::IO;


IMPLEMENT_SC_CLASS_DERIVED(ArclinkConnection,
			   Seiscomp::IO::RecordStream,
			   "ArclinkConnection");

REGISTER_RECORDSTREAM(ArclinkConnection, "arclink");
  
ArclinkConnection::ArclinkConnection()
    : RecordStream(), _stream(std::istringstream::in|std::istringstream::binary),
     _user("guest@anywhere"), _readingData(false), _chunkMode(false),
     _remainingBytes(0) {}

ArclinkConnection::ArclinkConnection(std::string serverloc)
    : RecordStream(), _stream(std::istringstream::in|std::istringstream::binary),
     _user("guest@anywhere"), _readingData(false), _chunkMode(false),
     _remainingBytes(0){
    setSource(serverloc);
}

ArclinkConnection::~ArclinkConnection() {
}

bool ArclinkConnection::setSource(std::string serverloc) {
	size_t pos = serverloc.find('?');
	if ( pos != std::string::npos ) {
		_serverloc = serverloc.substr(0, pos);
		std::string params = serverloc.substr(pos+1);
		std::vector<std::string> toks;
		split(toks, params.c_str(), "&");
		if ( !toks.empty() ) {
			for ( std::vector<std::string>::iterator it = toks.begin();
			      it != toks.end(); ++it ) {
				std::string name, value;

				pos = it->find('=');
				if ( pos != std::string::npos ) {
					name = it->substr(0, pos);
					value = it->substr(pos+1);
				}
				else {
					name = *it;
					value = "";
				}

				if ( name == "user" )
					_user = value;
				else if ( name == "pwd" )
					_passwd = value;
			}
		}
	}
	else
		_serverloc = serverloc;
	return true;
}

bool ArclinkConnection::setRecordType(const char* type) {
	return !strcmp(type, "mseed");
}

bool ArclinkConnection::setUser(std::string name, std::string password) {
	_user = name;
	_passwd = password;
	return true;
}

bool ArclinkConnection::addStream(std::string net, std::string sta, std::string loc, std::string cha) {
	pair<set<StreamIdx>::iterator, bool> result;
	result = _streams.insert(StreamIdx(net, sta, loc, cha));
	return result.second;
}

bool ArclinkConnection::addStream(std::string net, std::string sta, std::string loc, std::string cha,
	const Seiscomp::Core::Time &stime, const Seiscomp::Core::Time &etime) {
	pair<set<StreamIdx>::iterator, bool> result;
	result = _streams.insert(StreamIdx(net, sta, loc, cha, stime, etime));
	return result.second;
}

bool ArclinkConnection::removeStream(std::string net, std::string sta, std::string loc, std::string cha) {
	_streams.erase(StreamIdx(net, sta, loc, cha));
	return true;
}

bool ArclinkConnection::setStartTime(const Seiscomp::Core::Time &stime) {
	_stime = stime;
	return true;
}

bool ArclinkConnection::setEndTime(const Seiscomp::Core::Time &etime) {
	_etime = etime;
	return true;
}

bool ArclinkConnection::setTimeWindow(const Seiscomp::Core::TimeWindow &w) {
	return setStartTime(w.startTime()) && setEndTime(w.endTime());
}

bool ArclinkConnection::setTimeout(int seconds) {
	_sock.setTimeout(seconds);
	return true;
}

bool ArclinkConnection::clear() {
	this->~ArclinkConnection();
	new(this) ArclinkConnection(_serverloc);
	return true;
}

// Hopefully safe to be called from another thread
void ArclinkConnection::close() {
	_sock.interrupt();
}

bool ArclinkConnection::reconnect() {
	if ( _sock.isOpen() )
		_sock.close();

	_readingData = false;
	return true;
}

void ArclinkConnection::handshake() {
	_sock.sendRequest("HELLO", false);
	string r = _sock.readline();
	if ( r == "ERROR" ) {
		SEISCOMP_ERROR("Remote server did not accept HELLO");
		throw ArclinkCommandException("HELLO");
	}

	string _software = r;
	string _organization = _sock.readline();

	SEISCOMP_DEBUG("%s running at %s", _software.c_str(),
	               _organization.c_str());
    
	if ( _passwd.length() )
		_sock.sendRequest("USER " + _user + " " + _passwd, true);
	else
		_sock.sendRequest("USER " + _user, true);

	_sock.sendRequest("REQUEST WAVEFORM format=MSEED", true);

	for ( set<StreamIdx>::iterator it = _streams.begin(); it != _streams.end(); ++it ) {
		SEISCOMP_DEBUG("Arclink request: %s", it->str(_stime, _etime).c_str());
		if ((it->startTime() == Time() && _stime == Time()) ||
			(it->endTime() == Time() && _etime == Time())) {
			/* invalid time window ignore stream */
			SEISCOMP_WARNING("... has invalid time window -> ignore this request above");
		}
		else
			_sock.sendRequest(it->str(_stime, _etime), false);
	}

	_reqID = _sock.sendRequest("END", true);
	_sock.sendRequest("BDOWNLOAD " + _reqID, false);
	r = _sock.readline();

	if ( r == "ERROR" ) {
		_sock.sendRequest("BCDOWNLOAD " + _reqID, false);
		r = _sock.readline();
	}

	if ( r == "ERROR" ) {
		_chunkMode = false;
		_remainingBytes = 0;
	}
	else if ( r.compare(0, 6, "CHUNK ") == 0 ) {
		char *tail;
		_chunkMode = true;
		_remainingBytes = strtoul(r.c_str() + 6, &tail, 10);
		if ( *tail ) {
			SEISCOMP_ERROR("Invalid ArcLink response: %s", r.c_str());
			throw ArclinkException("invalid response");
		}
		SEISCOMP_DEBUG("Chunk mode detected, first chunk with %d bytes", _remainingBytes);
	}
	else {
		char *tail;
		_chunkMode = false;
		_remainingBytes = strtoul(r.c_str(), &tail, 10);
		if ( *tail ) {
			SEISCOMP_ERROR("Invalid ArcLink response: %s", r.c_str());
			throw ArclinkException("invalid response");
		}
	}
}

void ArclinkConnection::cleanup() {
	_sock.sendRequest("PURGE " + _reqID, true);
}

std::istream& ArclinkConnection::stream() {
	if ( _readingData && !_sock.isOpen() ) {
		_stream.clear(std::ios::eofbit);
		return _stream;
	}

	_sock.startTimer();

	if ( ! _readingData ) {
		_sock.open(_serverloc);
		try {
			handshake();
		}
		catch ( GeneralException ) {
			_sock.close();
			throw;
		}

		_readingData = true;
		if ( _remainingBytes <= 0 ) {
			_sock.close();
			_stream.clear(std::ios::eofbit);
			return _stream;
		}
	}

	try {
		// HACK to retrieve the record length
		string data = _sock.read(RECSIZE);
		int reclen = ms_find_reclen(data.c_str(), RECSIZE, NULL);
		if (reclen > RECSIZE)
			_stream.str(data + _sock.read(reclen - RECSIZE));
		else {
			if (reclen <= 0) SEISCOMP_ERROR("Retrieving the record length failed (try 512 Byte)!");
			reclen = RECSIZE;
			_stream.str(data);
		}
		/////////////////////////////////////
		_remainingBytes -= reclen;

		// Read next chunk size
		if ( _chunkMode && _remainingBytes <= 0 ) {
			string r = _sock.readline();
			if ( r.compare(0, 6, "CHUNK ") == 0 ) {
				char *tail;
				_remainingBytes = strtoul(r.c_str() + 6, &tail, 10);
				if ( *tail ) {
					SEISCOMP_ERROR("Invalid ArcLink response: %s", r.c_str());
					throw ArclinkException("invalid response");
				}
			}
			else
				SEISCOMP_DEBUG("Received status: %s", r.c_str());
		}

		if ( _remainingBytes <= 0 ) {
			cleanup();
			_sock.close();
		}
	}
	catch ( GeneralException ) {
		_sock.close();
		throw;
	}

	return _stream;
}
    
} // namespace _private
} // namespace Arclink
} // namespace RecordStream
} // namespace Seiscomp
