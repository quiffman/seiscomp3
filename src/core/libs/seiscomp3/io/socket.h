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


#ifndef __SEISCOMP_IO_SOCKET_H__
#define __SEISCOMP_IO_SOCKET_H__

#include <string>
#include <set>
#include <iostream>
#include <sstream>
#include <seiscomp3/core/interruptible.h>
#include <seiscomp3/core/datetime.h>
#include <seiscomp3/utils/timer.h>
#include <seiscomp3/io/recordstream.h>
#include <seiscomp3/io/api.h>

#define BUFSIZE 4096
#define RECSIZE 512

namespace Seiscomp {
namespace IO {

class SC_CORE_IO_API SocketException: public Seiscomp::IO::RecordStreamException {
    public:
    SocketException(): RecordStreamException("Socket exception") {}
    SocketException(const std::string& what): RecordStreamException(what) {}
};

class SC_CORE_IO_API SocketCommandException: public SocketException {
    public:
    SocketCommandException(): SocketException("command not accepted") {}
    SocketCommandException(const std::string& what): SocketException(what) {}
};

class SC_CORE_IO_API SocketTimeout: public SocketException {
    public:
    SocketTimeout(): SocketException("timeout") {}
    SocketTimeout(const std::string& what): SocketException(what) {}
};

class SC_CORE_IO_API SocketResolveError: public SocketException {
    public:
    SocketResolveError(): SocketException("cannot resolve hostname") {}
    SocketResolveError(const std::string& what): SocketException(what) {}
};


class SC_CORE_IO_API Socket: public Seiscomp::Core::InterruptibleObject {
	public:
		Socket();
		~Socket();

	public:
		void setTimeout(int seconds);
		void startTimer();
		void open(const std::string& serverLocation);
		void close();
		bool isOpen();
		bool tryReconnect();
		std::string readline();
		std::string read(int size);
		std::string sendRequest(const std::string& request, bool waitResponse);
		bool isInterrupted();
		void interrupt();
	
	private:
		enum { READ = 0, WRITE = 1 };
		int _pipefd[2];
		int _sockfd;
		char _buf[BUFSIZE + 1];
		int _rp;
		int _wp;
		int _timeout;
		Seiscomp::Util::StopWatch _timer;
		bool _interrupt;
		bool _reconnect;
		std::string _eol;


	private:
		void fillbuf();
		void write(const std::string& s);
		int connectSocket(struct sockaddr *addr, int len);
		int nonblockSocket();
		int addrSocket(char *hostname, char *port, struct sockaddr *addr, size_t *len) ;
		int checkSocket(int secs, int usecs);

	protected:
		void handleInterrupt(int) throw();
};

} // namespace IO
} // namespace Seiscomp

#endif

