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


#ifndef __SEISCOMP_SERVICES_RECORDSTREAM_COMBINED_H__
#define __SEISCOMP_SERVICES_RECORDSTREAM_COMBINED_H__

#include <string>
#include <iostream>
#include <seiscomp3/core/datetime.h>
#include <seiscomp3/core/timewindow.h>
#include <seiscomp3/io/recordstream.h>
#include <seiscomp3/io/recordstream/slconnection.h>
#include <seiscomp3/io/recordstream/arclink.h>
#include <seiscomp3/core.h>

namespace Seiscomp {
namespace RecordStream {
namespace Combined {
namespace _private {

DEFINE_SMARTPOINTER(CombinedConnection);

class SC_SYSTEM_CORE_API CombinedConnection : public Seiscomp::IO::RecordStream {
	DECLARE_SC_CLASS(CombinedConnection);

	public:
		//! C'tor
		CombinedConnection();
		
		//! Initializing Constructor
		CombinedConnection(std::string serverloc);

		//! Destructor
		virtual ~CombinedConnection();

		virtual bool setRecordType(const char*);

		//! Initialize the arclink connection.
		bool setSource(std::string serverloc);
		
		//! Supply user credentials
		bool setUser(std::string name, std::string password);
		
		//! Adds the given stream to the server connection description
		bool addStream(std::string net, std::string sta, std::string loc, std::string cha);

		//! Adds the given stream to the server connection description
		bool addStream(std::string net, std::string sta, std::string loc, std::string cha,
			const Seiscomp::Core::Time &stime, const Seiscomp::Core::Time &etime);

		//! Removes the given stream from the connection description. Returns true on success; false otherwise.
		bool removeStream(std::string net, std::string sta, std::string loc, std::string cha);
  
		//! Adds the given start time to the server connection description
		bool setStartTime(const Seiscomp::Core::Time &stime);
		
		//! Adds the given end time to the server connection description
		bool setEndTime(const Seiscomp::Core::Time &etime);
		
		//! Adds the given end time window to the server connection description
		bool setTimeWindow(const Seiscomp::Core::TimeWindow &w);

		//! Sets timeout
		bool setTimeout(int seconds);
  
		//! Removes all stream list, time window, etc. -entries from the connection description object.
		bool clear();

		//! Terminates the arclink connection.
		void close();

		//! Reconnects a terminated arclink connection.
		bool reconnect();
 
		//! Returns the data stream
		std::istream& stream();
		
		Record* createRecord(Array::DataType, Record::Hint);
		
	private:
		bool _useArclink;
		bool _download;
		int _nstream;
		Core::TimeSpan  _seedlinkAvailability;
		Core::Time      _startTime;
		Core::Time      _endTime;
		Core::Time      _curtime;
		Core::Time      _arclinkEndTime;
		size_t          _nArclinkStreams;
		size_t          _nSeedlinkStreams;
		std::set<StreamIdx> _streams;
		SLConnectionPtr _seedlink;
		Arclink::ArclinkConnectionPtr _arclink;
};

} // namespace _private
} // namespace Combined
} // namespace RecordStream
} // namespace Seiscomp

#endif

