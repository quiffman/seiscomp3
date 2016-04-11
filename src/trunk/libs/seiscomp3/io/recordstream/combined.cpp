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


#define SEISCOMP_COMPONENT CombinedConnection

#include <cstdio>
#include <string>
#include <iostream>
#include <seiscomp3/logging/log.h>
#include <seiscomp3/core/datetime.h>
#include <seiscomp3/core/strings.h>
#include <seiscomp3/io/recordstream.h>

#include "combined.h"

namespace Seiscomp {
namespace RecordStream {
namespace Combined {
namespace _private {

using namespace std;
using namespace Seiscomp::Core;
using namespace Seiscomp::IO;
using namespace Seiscomp::RecordStream::Arclink;

const TimeSpan defaultSeedlinkAvailability = 3600;

IMPLEMENT_SC_CLASS_DERIVED(CombinedConnection,
                           Seiscomp::IO::RecordStream,
                           "CombinedConnection");

REGISTER_RECORDSTREAM(CombinedConnection, "combined");

CombinedConnection::CombinedConnection()
	: _useArclink(false), _download(false), _nstream(0) {
	_seedlinkAvailability = defaultSeedlinkAvailability;
	_seedlink = new SLConnection();
	_arclink = new ArclinkConnection();
	_nArclinkStreams = _nSeedlinkStreams = 0;
}

CombinedConnection::CombinedConnection(std::string serverloc)
	: _useArclink(false), _download(false), _nstream(0) {
	_seedlinkAvailability = defaultSeedlinkAvailability;
	_seedlink = new SLConnection();
	_arclink = new ArclinkConnection();
	_nArclinkStreams = _nSeedlinkStreams = 0;
	setSource(serverloc);
}

CombinedConnection::~CombinedConnection() {
}

bool CombinedConnection::setRecordType(const char* type) {
	return _seedlink->setRecordType(type) &&
	       _arclink->setRecordType(type);
}

bool CombinedConnection::setSource(std::string serverloc) {
	string::size_type sep = serverloc.find(';');
	if ( sep == string::npos ) {
		SEISCOMP_ERROR("Invalid RecordStream address: %s", serverloc.c_str());
		throw RecordStreamException("invalid address");
	}

	string::size_type param_sep = serverloc.find("??");
	if ( param_sep != string::npos ) {
		if ( param_sep < sep ) {
			SEISCOMP_ERROR("Invalid RecordStream address, parameter separator '\?\?' comes before host separator: %s", serverloc.c_str());
			throw RecordStreamException("invalid address format");
		}

		string params = serverloc.substr(param_sep+2);
		serverloc.erase(param_sep);

		vector<string> toks;
		Core::split(toks, params.c_str(), "&");
		if ( !toks.empty() ) {
			for ( std::vector<std::string>::iterator it = toks.begin();
			      it != toks.end(); ++it ) {
				std::string name, value;

				size_t pos = it->find('=');
				if ( pos != std::string::npos ) {
					name = it->substr(0, pos);
					value = it->substr(pos+1);
				}
				else {
					name = *it;
					value = "";
				}

				if ( name == "slinkMax" ) {
					double seconds;
					if ( !Core::fromString(seconds, value) ) {
						SEISCOMP_ERROR("Invalid RecordStream parameter value for '%s' in %s: expected a numerical value",
						               name.c_str(), serverloc.c_str());
						throw RecordStreamException("invalid address parameter value");
					}

					SEISCOMP_DEBUG("set slinkMax for combined recordstream to %f", seconds);
					_seedlinkAvailability = Core::TimeSpan(seconds);
				}
			}
		}
	}

	_seedlink->setSource(string(serverloc, 0, sep));
	_arclink->setSource(string(serverloc, sep + 1, string::npos));

	_curtime = Time::GMT();
	_arclinkEndTime = _curtime - _seedlinkAvailability;
	_nArclinkStreams = _nSeedlinkStreams = 0;

	return true;
}

bool CombinedConnection::setUser(std::string name, std::string password) {
	_arclink->setUser(name, password);
	return true;
}

bool CombinedConnection::addStream(std::string net, std::string sta, std::string loc, std::string cha) {
	SEISCOMP_DEBUG("add stream %d %s.%s.%s.%s", _nstream, net.c_str(),
		sta.c_str(), loc.c_str(), cha.c_str());
	// Streams without a time span are inserted into a temporary list
	// and will be resolved when the data is requested the first time
	pair<set<StreamIdx>::iterator, bool> result;
	result = _streams.insert(StreamIdx(net, sta, loc, cha));
	return result.second;
}

bool CombinedConnection::addStream(std::string net, std::string sta, std::string loc, std::string cha,
	const Seiscomp::Core::Time &stime, const Seiscomp::Core::Time &etime) {
	SEISCOMP_DEBUG("add stream %d %s.%s.%s.%s", _nstream, net.c_str(),
		sta.c_str(), loc.c_str(), cha.c_str());

	if ( stime.valid() && stime < _arclinkEndTime ) {
		if ( etime.valid() && etime <= _arclinkEndTime ) {
			_arclink->addStream(net, sta, loc, cha, stime, etime);
			++_nArclinkStreams;
		}
		else {
			_arclink->addStream(net, sta, loc, cha, stime, _arclinkEndTime);
			_seedlink->addStream(net, sta, loc, cha, _arclinkEndTime, etime);
			++_nArclinkStreams;
			++_nSeedlinkStreams;
		}
	}
	else {
		_seedlink->addStream(net, sta, loc, cha, stime, etime);
		++_nSeedlinkStreams;
	}

	++_nstream;
	return true;
}

bool CombinedConnection::removeStream(std::string net, std::string sta, std::string loc, std::string cha) {
	_seedlink->removeStream(net, sta, loc, cha);
	_arclink->removeStream(net, sta, loc, cha);
	_streams.erase(StreamIdx(net, sta, loc, cha));
	return true;
}

bool CombinedConnection::setStartTime(const Seiscomp::Core::Time &stime) {
	_startTime = stime;
	return true;
}

bool CombinedConnection::setEndTime(const Seiscomp::Core::Time &etime) {
	_endTime = etime;
	return true;
}

bool CombinedConnection::setTimeWindow(const Seiscomp::Core::TimeWindow &w) {
	return setStartTime(w.startTime()) && setEndTime(w.endTime());
}


bool CombinedConnection::setTimeout(int seconds) {
	_seedlink->setTimeout(seconds);
	_arclink->setTimeout(seconds);
	return true;
}

bool CombinedConnection::clear() {
	_seedlink->clear();
	_arclink->clear();
	return true;
}

void CombinedConnection::close() {
	_seedlink->close();
	_arclink->close();
	_nArclinkStreams = _nSeedlinkStreams = 0;
}

bool CombinedConnection::reconnect() {
	_seedlink->reconnect();
	_arclink->reconnect();
	return true;
}

std::istream& CombinedConnection::stream() {
	if ( !_download ) {
		// Add the temporary saved streams (added without a time span)
		// now and split them correctly
		for ( set<StreamIdx>::iterator it = _streams.begin(); it != _streams.end(); ++it)
			addStream(it->network(), it->station(), it->location(), it->channel(),
			          _startTime, _endTime);
		_streams.clear();

		_download = true;
		_useArclink = _nArclinkStreams > 0;
		if ( _useArclink )
			SEISCOMP_DEBUG("start %d Arclink requests", (int)_nArclinkStreams);
		else
			SEISCOMP_DEBUG("start %d Seedlink requests", (int)_nSeedlinkStreams);
	}

	if ( _useArclink ) {
		std::istream &is = _arclink->stream();
		if ( !is.eof() )
			return is;
		else {
			_arclink->close();
			_useArclink = false;
			SEISCOMP_DEBUG("start %d Seedlink requests", (int)_nSeedlinkStreams);
		}

		return _seedlink->stream();
	}
	else
		return _seedlink->stream();
}

Record* CombinedConnection::createRecord(Array::DataType dt, Record::Hint h) {
	if ( _useArclink )
		return _arclink->createRecord(dt, h);
	else
		return _seedlink->createRecord(dt, h);
}

} // namespace _private
} // namespace Combined
} // namespace RecordStream
} // namespace Seiscomp

