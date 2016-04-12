/************************************************************************
 *                                                                      *
 * Copyright (C) 2012 OVSM/IPGP                                         *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * This program is part of 'Projet TSUAREG - INTERREG IV Caraïbes'.     *
 * It has been co-financed by the European Union and le Minitère de     *
 * l'Ecologie, du Développement Durable, des Transports et du Logement. *
 *                                                                      *
 ************************************************************************/


#define SEISCOMP_COMPONENT EW2SC3

#include <seiscomp3/logging/filerotator.h>
#include <seiscomp3/logging/channel.h>
#include <seiscomp3/datamodel/magnitude.h>
#include <seiscomp3/math/geo.h>
#include <seiscomp3/utils/files.h>

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <errno.h>

#include "ew2sc3.h"
#include "config.h"


using namespace std;
using namespace Seiscomp;
using namespace Seiscomp::Core;
using namespace Seiscomp::DataModel;



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EW2SC3::EW2SC3(int argc, char **argv) :
		Application(argc, argv) {

	setAutoApplyNotifierEnabled(true);
	setInterpretNotifierEnabled(true);
	setPrimaryMessagingGroup("EVENT");
	addMessagingSubscription("LOCATION");
	addMessagingSubscription("PICK");
	addMessagingSubscription("MAGNITUDE");
	setConnectionRetries(0xFFFFFFFF);

	std::string appnameinfo = (string) argv[0] + "-processing-info";
	std::string appnameerror = (string) argv[0] + "-processing-error";
	// Processing log file
	_infoChannel = SEISCOMP_DEF_LOGCHANNEL("info", Logging::LL_INFO);
	_infoOutput = new Logging::FileRotatorOutput(
	    Environment::Instance()->logFile(appnameinfo).c_str(), 60 * 60 * 24, 30);
	_infoOutput->subscribe(_infoChannel);

	// Errors/Awkwardness log file
	_errorChannel = SEISCOMP_DEF_LOGCHANNEL("error", Logging::LL_INFO);
	_errorOutput = new Logging::FileRotatorOutput(
	    Environment::Instance()->logFile(appnameerror).c_str(), 60 * 60 * 24, 30);
	_errorOutput->subscribe(_errorChannel);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EW2SC3::~EW2SC3() {

	delete _infoChannel;
	delete _infoOutput;

	delete _errorChannel;
	delete _errorOutput;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool EW2SC3::config() {

	int error = 0;

	try {
		_configPath = configGetString("ew2sc3.configPath");
		if ( _configPath.size() > 0 ) {
			if ( _configPath[_configPath.size() - 1] != '/' )
				_configPath += '/';

			if ( !Util::pathExists(_configPath) )
				Util::createPath(_configPath);
		}
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Can't read 'configPath' variable");
		error++;
	}

	try {
		senderPort = configGetInt("ew2sc3.senderPort");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Can't read 'senderport' variable");
		error++;
	}

	try {
		modId = configGetInt("ew2sc3.modID");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Can't read 'modid' variable");
		error++;
	}

	try {
		instId = configGetInt("ew2sc3.instID");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Can't read 'instid' variable");
		error++;
	}

	try {
		msgId = configGetInt("ew2sc3.msgID");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Can't read 'msgid' variable");
		error++;
	}

	try {
		instName = configGetString("ew2sc3.name");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Can't read 'name' variable");
		error++;
	}

	try {
		instAuthor = configGetString("ew2sc3.author");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Can't read 'author' variable");
		error++;
	}

	try {
		senderHostName = configGetString("ew2sc3.hostname");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Can't read 'hostname' variable");
		error++;
	}

	try {
		defaultLatitude = configGetString("ew2sc3.defaultLatitude");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Can't read 'latitude' variable");
		error++;
	}

	try {
		defaultLongitude = configGetString("ew2sc3.defaultLongitude");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Can't read 'longitude' variable");
		error++;
	}

	try {
		_locProfile = configGetString("ew2sc3.locatorProfile");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Can't read 'locatorProfile' variable");
		error++;
	}
	try {
		_archive = configGetBool("ew2sc3.enableArchiving");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Can't read 'enableArchiving' variable");
		error++;
	}

	try {
		myAliveInt = configGetInt("ew2sc3.myAliveInt");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Cant read 'myAliveInt' variable");
		error++;
	}

	try {
		senderTimeout = configGetInt("ew2sc3.senderTimeout");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Cant read 'senderTimeout' variable");
		error++;
	}

	try {
		maxMsgSize = configGetInt("ew2sc3.maxMsgSize");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Cant read 'maxMsgSize' variable");
		error++;
	}

	try {
		_myAliveString = configGetString("ew2sc3.myAliveString");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Cant read 'myAliveString' variable");
		error++;
	}

	try {
		_senderAliveString = configGetString("ew2sc3.myAliveString");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Cant read 'myAliveString' variable");
		error++;
	}

	try {
		_enableUncertainties = configGetBool("ew2sc3.enableUncertainties");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Cant read 'enableUncertainties' variable");
		error++;
	}

	try {
		_uncertainties = configGetDoubles("ew2sc3.pickerUncertainties");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Cant read 'pickerUncertainties' variable");
		error++;
	}

	try {
		_uncertaintyMax = configGetDouble("ew2sc3.maxUncertainty");
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Cant read 'maxUncertainty' variable");
		error++;
	}

	messageReceiverStatus = 1;
	heartThreadStatus = 1;

	strncpy(senderHeartText, _senderAliveString.c_str(), MAX_ALIVE_STR - 1);
	senderHeartText[MAX_ALIVE_STR - 1] = '\0';

	strncpy(myAliveString, _myAliveString.c_str(), MAX_ALIVE_STR - 1);
	myAliveString[MAX_ALIVE_STR - 1] = '\0';

	return error == 0;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool EW2SC3::initConfiguration() {

	if ( !Client::Application::initConfiguration() )
		return false;

	SEISCOMP_LOG(_infoChannel, "-------------------------------------------------------");
	SEISCOMP_LOG(_infoChannel, "Starting...");

	if ( !EW2SC3::config() ) {
		SEISCOMP_LOG(_errorChannel, "Application initialization failed... Check configuration!");
		return false;
	}

	SEISCOMP_LOG(_infoChannel, "Application initialized with parameters:");
	SEISCOMP_LOG(_infoChannel, "  [configPath]        [%s]", _configPath.c_str());
	SEISCOMP_LOG(_infoChannel, "  [modID]             [%d]", modId);
	SEISCOMP_LOG(_infoChannel, "  [instID]            [%d]", instId);
	SEISCOMP_LOG(_infoChannel, "  [msgID]             [%d]", msgId);
	SEISCOMP_LOG(_infoChannel, "  [name]              [%s]", instName.c_str());
	SEISCOMP_LOG(_infoChannel, "  [author]            [%s]", instAuthor.c_str());
	SEISCOMP_LOG(_infoChannel, "  [senderHostname]    [%s]", senderHostName.c_str());
	SEISCOMP_LOG(_infoChannel, "  [senderPort]        [%d]", senderPort);
	SEISCOMP_LOG(_infoChannel, "  [senderAliveString] [%s]", senderHeartText);
	SEISCOMP_LOG(_infoChannel, "  [senderTimeout]     [%d]", senderTimeout);
	SEISCOMP_LOG(_infoChannel, "  [myAliveString]     [%s]", myAliveString);
	SEISCOMP_LOG(_infoChannel, "  [myAliveInt]        [%d]", myAliveInt);
	SEISCOMP_LOG(_infoChannel, "  [maxMgsSize]        [%d]", maxMsgSize);

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool EW2SC3::run() {

	while ( !isExitRequested() ) {

		::usleep(100 * 1000);

		if ( !connected )
			ew_connect();

		if ( connected && messageReceiverStatus != 0 )
			startListenerThread();

		if ( connected && heartThreadStatus != 0 )
			startHeartbeatThread();
	}

	close(socketDescriptor);

	SEISCOMP_LOG(_infoChannel, "Received application stop request. Leaving...");

	return true;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EW2SC3::runListenerThread() {

	SEISCOMP_LOG(_infoChannel, "Earthworm listener thread launched");
	while ( ew_read() ) {
	}
	SEISCOMP_LOG(_infoChannel, "Earthworm listener thread stopped");
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EW2SC3::runHeartbeatThread() {

	SEISCOMP_LOG(_infoChannel, "Earthworm heartbeat thread launched");
	while ( sendHeartbeat() ) {
	}
	SEISCOMP_LOG(_infoChannel, "Earthworm heartbeat thread stopped");
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EW2SC3::startListenerThread() {
	_listenerThread = new boost::thread(boost::bind(&EW2SC3::runListenerThread, this));
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EW2SC3::startHeartbeatThread() {
	_heartbeatThread = new boost::thread(boost::bind(&EW2SC3::runHeartbeatThread, this));
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int EW2SC3::ew_connect() {

	SEISCOMP_LOG(_infoChannel, "Trying to connect to %s on port %d",
	    senderHostName.c_str(), senderPort);

	socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);

	if ( socketDescriptor < 0 )
		SEISCOMP_ERROR("System couldn't initiate socket");
	else
		SEISCOMP_LOG(_infoChannel, "Received socket description, proceeding with connection");

	server = gethostbyname(senderHostName.c_str());
	if ( !server ) {
		SEISCOMP_ERROR("Host %s is not responding, hostname resolving failed",
		    senderHostName.c_str());
		exit(0);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(senderPort);

	if ( connect(socketDescriptor, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0 ) {

		// Bugfix!
		// Threads never get restarted if one of those is not properly reseted
		close(socketDescriptor);
		messageReceiverStatus = -1;
		heartThreadStatus = -1;

		connected = false;

		SEISCOMP_LOG(_errorChannel, "Can't connect to %s:%d. New attempt in 10 seconds",
		    senderHostName.c_str(), senderPort);

		sleep(9);
	}
	else {
		connected = true;

		SEISCOMP_LOG(_infoChannel, "Network socket opened to Earthworm [%s]",
		    senderHostName.c_str());
		SEISCOMP_LOG(_infoChannel, "      ...Now waiting for events...");
	}

	return EXIT_SUCCESS;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int EW2SC3::sendHeartbeat() {

	time_t now;
	heartThreadStatus = 0;
	time(&lastSocketBeat);

	while ( connected ) {
		sleep(1);
		time(&now);
		if ( difftime(now, lastSocketBeat) > (double) myAliveInt && myAliveInt != 0 ) {
			if ( ew_write(socketDescriptor, myAliveString) != 0 ) {
				SEISCOMP_ERROR("Problem sending alive msg to socket");
				heartThreadStatus = -1;
				_heartbeatThread->join();
			}

			SEISCOMP_DEBUG("Heartbeat sent to %s", senderHostName.c_str());
			lastSocketBeat = now;
		}
	}

	return EXIT_SUCCESS;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int EW2SC3::ew_read() {

	static int state;
	static char chr, lastChr;
	static int nr;
	static int nchar;
	static char startCharacter = STX;
	static char endCharacter = ETX;
	static char escape = ESC;
	static char inBuffer[INBUFFERSIZE];
	static int inchar;

	state = SEARCHING_FOR_MESSAGE_START;
	inchar = -1;
	nr = 0;
	chr = 0;
	nchar = 0;
	messageReceiverStatus = 0;

	// Allocating the message buffer thru memory
	if ( (msgBuffer = (char *) malloc(maxMsgSize)) == (char *) NULL )
		SEISCOMP_LOG(_errorChannel, "Can't allocate buffer size %d", maxMsgSize);

	while ( connected ) {

		if ( ++inchar == nr ) {
			nr = read(socketDescriptor, inBuffer, INBUFFERSIZE);
			if ( nr < 0 ) {
				SEISCOMP_ERROR("Failed to read socket content");
				messageReceiverStatus = -1;
				heartThreadStatus = -1;
				connected = false;
				continue;
			}
			inchar = 0;
		}

		lastChr = chr;
		chr = inBuffer[inchar];

		// Looking for start character
		if ( state == SEARCHING_FOR_MESSAGE_START ) {
			if ( lastChr != escape && chr == startCharacter ) {
				state = ASSEMBLING_MESSAGE;
				continue;
			}
		}

		// Confirm message start
		if ( state == EXPECTING_MESSAGE_START ) {
			if ( chr == startCharacter && lastChr != escape ) {
				nchar = 0;
				state = ASSEMBLING_MESSAGE;
				continue;
			}
			else {
				SEISCOMP_ERROR("Unexpected character from client. Re-synching");
				connected = false;
				state = SEARCHING_FOR_MESSAGE_START;
				continue;
			}
		}

		// Getting the data ready to be de-garbaged if end character matches ETX
		if ( state == ASSEMBLING_MESSAGE ) {
			if ( chr == endCharacter ) {
				msgBuffer[nchar] = 0;

				if ( strcmp(&msgBuffer[9], senderHeartText) == 0 ) {
					// Damn! This message is just an heartbeat :(
					SEISCOMP_DEBUG("Received heartbeat from %s", senderHostName.c_str());
					time((time_t*) &lastServerBeat);
					state = EXPECTING_MESSAGE_START;
					msgBuffer[0] = ' ';
					msgBuffer[9] = ' ';

				}
				else {
					SEISCOMP_DEBUG("Received non-heartbeat from %s", senderHostName.c_str());
					time((time_t*) &lastServerBeat);

					char msg_inst[4], msg_mod[4], msg_type[4];

					strncpy(msg_inst, msgBuffer, 3);
					msg_inst[3] = 0;

					strncpy(msg_mod, &msgBuffer[3], 3);
					msg_mod[3] = 0;

					strncpy(msg_type, &msgBuffer[6], 3);
					msg_type[3] = 0;

					if ( (atoi(msg_type) == msgId) && (atoi(msg_inst) == instId) ) {
						// Checking module id is not mandatory anymore since several
						// modules may produce origins in earthworm... Otherwise replace
						// line by the following:
						// if ((atoi(msg_type) == msgId) && (atoi(msg_inst) == instId) && (atoi(msg_mod) == modId)) {
						SEISCOMP_LOG(_infoChannel, "Incoming origin from Earthworm [msgID: %d | instID: %d | modID: %d]",
						    atoi(msg_type), atoi(msg_inst), atoi(msg_mod));
						extractOrigin(msgBuffer);
					}
					else {
						SEISCOMP_LOG(_errorChannel, "Incoming trace has been ignored [msgID: %d != %d | instID: %d != %d | modID: %d != %d]",
						    atoi(msg_type), msgId, atoi(msg_inst), instId, atoi(msg_mod), modId);
					}
				}
				state = EXPECTING_MESSAGE_START;
				continue;
			}
			else {
				if ( chr == escape ) {
					if ( ++inchar == nr ) {
						nr = read(socketDescriptor, inBuffer, INBUFFERSIZE);
						if ( nr <= 0 ) {
							// The socket is closed, we need to kill ourself and tell the main thread restart us
							SEISCOMP_ERROR("Can not read from network. Restarting");
							messageReceiverStatus = -1; /* file a complaint to the main thread */
							heartThreadStatus = -1;
							_heartbeatThread->join();
							_listenerThread->join();
							break;
						}
						inchar = 0;
					}

					lastChr = chr;
					chr = inBuffer[inchar];

					if ( chr != startCharacter && chr != endCharacter && chr != escape ) {
						// Bad news! unknown escape sequence
						SEISCOMP_ERROR("Unknown escape sequence in message. Re-synching");
						state = SEARCHING_FOR_MESSAGE_START;
						continue;
					}
					else {
						msgBuffer[nchar++] = chr;
						if ( nchar > maxMsgSize ) {
							SEISCOMP_ERROR("Received buffer overflow after %d bytes", maxMsgSize);
							state = SEARCHING_FOR_MESSAGE_START;
							nchar = 0;
							continue;
						}
						continue;
					}
				}

				if ( chr == startCharacter ) {
					// Bad news! Un-escaped start character
					SEISCOMP_ERROR("Un-escaped start character in message. Re-synching");
					state = SEARCHING_FOR_MESSAGE_START;
					continue;
				}

				// So it's not a naked start, escape, or a naked end. Hey, it's just a normal byte
				msgBuffer[nchar++] = chr;
				if ( nchar > maxMsgSize ) {
					SEISCOMP_ERROR("Received buffer overflow after %d bytes", maxMsgSize);
					state = SEARCHING_FOR_MESSAGE_START;
					nchar = 0;
					continue;
				}
				continue;

			}
		}
	}

	return EXIT_SUCCESS;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int EW2SC3::ew_write(int sock, char *msg) {

	char asciilogo[11]; // ascii version of outgoing logo
	char startmsg = STX; // flag for beginning of message
	char endmsg = ETX; // flag for end of message
	int msglength; // total length of msg to be sent
	int rc; // returned transaction state
	msglength = strlen(msg); // assumes msg is null terminated

	// "start of transmission" flag & ascii representation of logo
	sprintf(asciilogo, "%c%3d%3d%3d", startmsg, instId, modId, msgId);
	rc = sendMessage(sock, asciilogo, 10, 0, senderTimeout);
	if ( rc != 10 ) {
		SEISCOMP_ERROR("write() %s", strerror(errno));
		return -1;
	}

	// "message" -> break it into chunks if it's big!
	rc = sendMessage(sock, msg, msglength, 0, senderTimeout);
	if ( rc == -1 ) {
		SEISCOMP_ERROR("write() %s", strerror(errno));
		return -1;
	}

	// "end of transmission" flag
	rc = sendMessage(sock, &endmsg, 1, 0, senderTimeout);
	if ( rc != 1 ) {
		SEISCOMP_ERROR("write() %s", strerror(errno));
		return -1;
	}

	return EXIT_SUCCESS;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int EW2SC3::sendMessage(int sock, char *buf, int buflen, int flags,
                        int timeout_msecs) {

	int bytesjustsent = 0;
	int bytessent = 0;
	int ackcnt = 0; // counter for the number of recv attempts
	int ackpoll = 5; // poll at 0.005 seconds for recv'ing

	while ( bytessent < buflen ) {
		bytesjustsent = send(sock, buf + bytessent, buflen - bytessent, flags);
		if ( bytesjustsent < 0 ) {
			if ( errno != EWOULDBLOCK ) {
				SEISCOMP_ERROR("sendMessage() %s", strerror(errno));
				bytessent = -1;
				break;
			}
			else {
				bytesjustsent = 0;
			}
		}
		bytessent += bytesjustsent;

		// Check timeout when nothing received
		if ( bytesjustsent == 0 ) {
			ackcnt++;
			if ( (ackpoll * ackcnt) > timeout_msecs ) {
				SEISCOMP_ERROR("sendMessage() socket send timeout");
				bytessent = 0;
				break;
			}
			sleep(ackpoll * 1000);
		}
	}

	return bytessent;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int EW2SC3::extractOrigin(char* msg) {

	Notifier::SetEnabled(true);
	EventParametersPtr ep = new EventParameters;

	string eventYear;
	string eventMonth;
	string eventDay;
	string eventHour;
	string eventMin;
	string eventSec;
	string eventDepth;
	string eventPhases;
	string eventAzimuth;
	string eventRms;
	string eventAmplitudeMagnitude;
	string eventCodaDurationMagnitude;
	string eventPreferredMagnitudeLabel;
	string eventPreferredMagnitude;
	string eventLatDeg;
	string eventLatSit;
	string eventLatMin;
	string eventLonDeg;
	string eventLonSit;
	string eventLonMin;
	string eventErh;
	string eventErz;
	string eventMinDistance;
	string zero = "0";
	string neventMonth;
	string neventDay;
	string neventHour;
	string neventMin;
	string neventSec;
	string tmpFile = _configPath + "arc.tmp";


	// temp arc file writing
	try {
		ofstream out(tmpFile.c_str());
		out << &msg[9];
		out.close();
	}
	catch ( exception &e ) {
		SEISCOMP_LOG(_errorChannel, "Tmp Arc exception; %s", e.what());
	}

	string line;
	ifstream in(tmpFile.c_str());
	string eventLatitude;
	string eventLongitude;
	double corr;
	int start_line = 0, idx = 0, p_idx = 0, s_idx = 0;

	// Origin creation
	OriginPtr origin = Origin::Create();
	Time ot;
	CreationInfo oci;

	oci.setCreationTime(Core::Time::GMT());
	oci.setAuthor(instAuthor);
	oci.setAgencyID(agencyID());
	origin->setCreationInfo(oci);
	origin->setEarthModelID(_locProfile);
	origin->setMethodID("Earthworm");
	origin->setEvaluationMode(EvaluationMode(AUTOMATIC));

	while ( in.good() ) {

		getline(in, line);

		// This is the Summary Header of the arc file
		if ( (line.substr(0, 1) != "$") && (line.substr(0, 2) != "$1")
		        && (start_line == 0) && (line.length() > 1) ) {

			eventYear = line.substr(0, 4);
			eventMonth = line.substr(4, 2);
			eventDay = line.substr(6, 2);
			eventHour = line.substr(8, 2);
			eventMin = line.substr(10, 2);
			eventSec = line.substr(12, 2) + "." + line.substr(14, 2);
			eventSec = blank_replace(eventSec, zero);
			eventAmplitudeMagnitude = line.substr(36, 1) + "." + line.substr(37, 2);
			eventAmplitudeMagnitude = blank_replace(eventAmplitudeMagnitude, zero);
			neventMonth = blank_replace(eventMonth, zero);
			neventDay = blank_replace(eventDay, zero);
			neventHour = blank_replace(eventHour, zero);
			neventMin = blank_replace(eventMin, zero);
			neventSec = blank_replace(eventSec, zero);
			eventDepth = line.substr(31, 3) + "." + line.substr(34, 2);
			eventPhases = line.substr(39, 3);
			eventAzimuth = line.substr(42, 3);
			eventMinDistance = line.substr(45, 3);
			eventRms = line.substr(48, 2) + "." + line.substr(50, 2);
			eventLatDeg = line.substr(16, 2);
			if ( line.substr(18, 1) == "S" )
				eventLatSit = "S";
			else
				eventLatSit = "N";
			eventLatMin = line.substr(19, 4);
			eventLonDeg = line.substr(23, 3);
			if ( line.substr(26, 1) == "E" )
				eventLonSit = "E";
			else
				eventLonSit = "W";
			eventLonMin = line.substr(27, 4);
			eventErh = line.substr(85, 4);
			eventErz = line.substr(89, 4);

			eventCodaDurationMagnitude = line.substr(70, 1) + "." + line.substr(71, 2);
			eventCodaDurationMagnitude = blank_replace(eventCodaDurationMagnitude, zero);
			eventPreferredMagnitude = line.substr(147, 1) + "." + line.substr(148, 2);
			eventPreferredMagnitude = blank_replace(eventPreferredMagnitude, zero);
			eventPreferredMagnitudeLabel = "M" + line.substr(146, 1);

			// Archiving enabled ?
			if ( _archive ) {

				if ( !Util::pathExists(_configPath + "archive/") )
					Util::createPath(_configPath + "archive/");

				string archiveFile = _configPath + "archive/" + "event-"
				        + eventYear + "." + eventMonth + "." + eventDay + "."
				        + eventHour + "-" + eventMin + "-" + eventSec + ".arc";
				try {
					ofstream arc(archiveFile.c_str());
					arc << &msg[9];
					arc.close();
					SEISCOMP_LOG(_infoChannel, "Origin archived under %s", archiveFile.c_str());
				}
				catch ( exception &e ) {
					SEISCOMP_LOG(_errorChannel, "Archiving exception; %s", e.what());
				}
			}

			if ( (to_double(eventLonMin) != .0) or (to_double(eventLonDeg) != .0) ) {
				eventLatitude = sexagesimal_to_decimal(to_double(eventLatDeg),
				    to_double(eventLatMin.substr(0, 2) + "." + eventLatMin.substr(2, 2)),
				    eventLatSit);
				eventLongitude = sexagesimal_to_decimal(to_double(eventLonDeg),
				    to_double(eventLonMin.substr(0, 2) + "." + eventLonMin.substr(2, 2)),
				    eventLonSit);
			}
			else {
				eventLatitude = defaultLatitude;
				eventLongitude = defaultLongitude;
			}

			// Hypo71 ERH to SeisComP3 ERZ conversion
			corr = to_double(eventErh) / sqrt(2);

			ot.set(to_int(eventYear), to_int(eventMonth), to_int(eventDay),
			    to_int(eventHour), to_int(eventMin), (int) floor(to_double(eventSec)),
			    (int) ((to_double(eventSec) - floor(to_double(eventSec))) * 1.0E6));
			origin->setTime(ot);
			origin->setLatitude(RealQuantity(to_double(eventLatitude), corr, None, None, None));
			origin->setLongitude(RealQuantity(to_double(eventLongitude), corr, None, None, None));
			origin->setDepth(RealQuantity(to_double(eventDepth), to_double(eventErz), None, None, None));

			ep->add(origin.get());
			NotifierMessagePtr nmsg = Notifier::GetMessage(true);
			connection()->send("LOCATION", nmsg.get());
		}


		// This is the station archive section of the arc file
		if ( (line.substr(0, 1) != "$") && (line.substr(0, 2) != "$1")
		        && (start_line != 0) && (line.length() > 1)
		        && (line.substr(0, 1) != " ") ) {

			string site = line.substr(0, 5);
			string network = line.substr(5, 2);
			string component = line.substr(9, 3);
			string year = line.substr(17, 4);
			string mdhm = line.substr(21, 8);
			string psec = line.substr(29, 3) + "." + line.substr(32, 2);
			string pres = line.substr(34, 4);
			string ssec = line.substr(41, 3) + "." + line.substr(44, 2);
			string loc = line.substr(111, 2);
			string azimuth = line.substr(91, 3);
			string epdist = line.substr(74, 3) + "." + line.substr(77, 1);
			string pPhase = line.substr(14, 1);
			string sPhase = line.substr(47, 1);

			psec = blank_replace(psec, zero);
			pres = blank_replace(pres, zero);
			ssec = blank_replace(ssec, zero);
			azimuth = blank_replace(azimuth, zero);
			epdist = blank_replace(epdist, zero);

			CreationInfo ci;
			ci.setCreationTime(Time::GMT());
			ci.setAgencyID(agencyID());
			ci.setAuthor(instAuthor);
			ci.setModificationTime(Time::GMT());

			Time time = Time::GMT();
			StationPtr station = NULL;
			station = query()->getStation(network, site, time);

			double dist, azi1, azi2;
			if ( station ) {
				Math::Geo::delazi(station->latitude(), station->longitude(),
				    origin->latitude().value(), origin->longitude().value(),
				    &dist, &azi1, &azi2);
			}
			else {
				dist = Math::Geo::km2deg(to_double(epdist));
				azi1 = to_double(azimuth);
			}

			if ( pPhase.compare("P") == 0 ) {

				double res = getResidual(pres);
				PickPtr pPick = Pick::Create();
				pPick->setCreationInfo(ci);
				pPick->setEvaluationStatus(EvaluationStatus(PRELIMINARY));
				Time pt;
				pt.set(to_int(year), to_int(mdhm.substr(0, 2)),
				    to_int(mdhm.substr(2, 2)), to_int(mdhm.substr(4, 2)),
				    to_int(mdhm.substr(6, 2)), (int) floor(to_double(psec)),
				    (int) ((to_double(psec) - floor(to_double(psec))) * 1.0E6));
				pPick->setTime(pt);
				pPick->setEvaluationMode(EvaluationMode(AUTOMATIC));
				pPick->setPhaseHint(DataModel::Phase("P"));
				pPick->setWaveformID(DataModel::WaveformStreamID(strip_space(network),
				    strip_space(site), strip_space(loc), strip_space(component), ""));

				if ( _enableUncertainties ) {
					string pWeight = line.substr(16, 1);
					if ( pWeight == " " )
						pWeight = "4";
					pPick->time().setUncertainty(getSeiscompUncertainty(to_double(pWeight)));
				}

				PickList.push_back(pPick);

				ArrivalPtr pArrival = new Arrival();
				pArrival->setPickID(pPick->publicID());
				pArrival->setWeight(1.0);
				pArrival->setDistance(dist);
				pArrival->setAzimuth(azi2);
				pArrival->setTimeResidual(res);
				pArrival->setPhase(Phase("P"));
				pArrival->setCreationInfo(ci);
				pArrival->setEarthModelID(_locProfile);

				origin->add(pArrival.get());
				NotifierMessagePtr msg = Notifier::GetMessage(true);
				connection()->send("LOCATION", msg.get());

				p_idx++;
			}

			if ( sPhase.compare("S") == 0 ) {

				Time st;
				st.set(to_int(year), to_int(mdhm.substr(0, 2)),
				    to_int(mdhm.substr(2, 2)), to_int(mdhm.substr(4, 2)),
				    to_int(mdhm.substr(6, 2)), (int) floor(to_double(ssec)),
				    (int) ((to_double(ssec) - floor(to_double(ssec))) * 1.0E6));

				string sres = line.substr(50, 4);
				double res = getResidual(sres);

				PickPtr sPick = Pick::Create();
				sPick->setCreationInfo(ci);
				sPick->setEvaluationStatus(EvaluationStatus(PRELIMINARY));
				sPick->setTime(st);
				sPick->setEvaluationMode(EvaluationMode(AUTOMATIC));
				sPick->setPhaseHint(DataModel::Phase("S"));
				sPick->setWaveformID(DataModel::WaveformStreamID(strip_space(network),
				    strip_space(site), strip_space(loc), strip_space(component), ""));

				if ( _enableUncertainties ) {
					string sWeight = line.substr(49, 1);
					if ( sWeight == " " )
						sWeight = "4";
					sPick->time().setUncertainty(getSeiscompUncertainty(to_double(sWeight)));
				}

				PickList.push_back(sPick);

				ArrivalPtr sArrival = new Arrival();
				sArrival->setPickID(sPick->publicID());
				sArrival->setWeight(1.0);
				sArrival->setDistance(dist);
				sArrival->setAzimuth(azi2);
				sArrival->setPhase(Phase("S"));
				sArrival->setTimeResidual(res);
				sArrival->setCreationInfo(ci);
				sArrival->setEarthModelID(_locProfile);

				origin->add(sArrival.get());
				NotifierMessagePtr msg = Notifier::GetMessage(true);
				connection()->send("LOCATION", msg.get());

				s_idx++;
			}
			idx++;
		}
		start_line++;
	}
	in.close();

	CreationInfo mci;
	mci.setCreationTime(Time::GMT());
	mci.setAgencyID(agencyID());
	mci.setAuthor(instAuthor);
	mci.setModificationTime(Time::GMT());

	// Amplitude magnitude
	MagnitudePtr amag = Magnitude::Create();
	amag->setType("ML");
	amag->setOriginID(origin->publicID());
	amag->setMagnitude(RealQuantity(to_double(eventAmplitudeMagnitude)));
	amag->setMethodID("mean");
	amag->setStationCount(idx);
	amag->setCreationInfo(mci);
	origin->add(amag.get());

	// Coda duration magnitude
	MagnitudePtr dmag = Magnitude::Create();
	dmag->setType("Md");
	dmag->setOriginID(origin->publicID());
	dmag->setMagnitude(RealQuantity(to_double(eventCodaDurationMagnitude)));
	dmag->setMethodID("mean");
	dmag->setStationCount(idx);
	dmag->setCreationInfo(mci);
	origin->add(dmag.get());

	OriginQuality oq;
	if ( to_double(eventRms) != .0 )
		oq.setStandardError(to_double(eventRms));
	if ( to_double(eventAzimuth) != .0 )
		oq.setAzimuthalGap(to_double(eventAzimuth));
	oq.setUsedStationCount(idx);
	oq.setAssociatedPhaseCount(p_idx + s_idx);
	oq.setUsedPhaseCount(p_idx + s_idx);
	oq.setDepthPhaseCount(p_idx + s_idx);
	oq.setMinimumDistance(Math::Geo::km2deg(to_double(eventMinDistance)));

	origin->setQuality(oq);
	origin->update();

	// Sending picks in a compact way
	for (unsigned int i = 0; i < PickList.size(); i++) {
		NotifierPtr n = new Notifier("EventParameters", DataModel::OP_ADD, PickList.at(i).get());
		NotifierMessagePtr m = new NotifierMessage;
		m->attach(n.get());
		connection()->send(m.get());
	}

	SEISCOMP_DEBUG("Sending Origin with ID %s", origin->publicID().c_str());
	NotifierMessagePtr nmsg = Notifier::GetMessage(true);
	connection()->send("LOCATION", nmsg.get());
	Notifier::SetEnabled(false);

	SEISCOMP_LOG(_infoChannel, "-------------------------------------------------------");
	SEISCOMP_LOG(_infoChannel, "New origin : %s", origin->publicID().c_str());
	SEISCOMP_LOG(_infoChannel, "      Date : %s/%s/%s @ %s:%s:%s", eventYear.c_str(),
	    eventMonth.c_str(), eventDay.c_str(), eventHour.c_str(),
	    eventMin.c_str(), eventSec.c_str());
	SEISCOMP_LOG(_infoChannel, "  Location : %s / %s", eventLatitude.c_str(), eventLongitude.c_str());
	SEISCOMP_LOG(_infoChannel, "    Phases : %d (%dP/%dS)", (p_idx + s_idx), p_idx, s_idx);
	if ( strip_space(eventDepth).compare(".0") != 0 )
		SEISCOMP_LOG(_infoChannel, " Depth : %s", strip_space(eventDepth).c_str());
	SEISCOMP_LOG(_infoChannel, "Magnitudes : ML %s / Md %s [%s preferred]",
	    eventAmplitudeMagnitude.c_str(), eventCodaDurationMagnitude.c_str(),
	    eventPreferredMagnitudeLabel.c_str());
	SEISCOMP_LOG(_infoChannel, "-------------------------------------------------------");

	return EXIT_SUCCESS;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double EW2SC3::getResidual(string &res) {

	int pos = -1;
	for (unsigned int i = 0; i < res.size(); i++) {
		if ( res.substr(i, 1).compare("-") == 0 )
			pos = i;
	}

	string value;
	if ( pos != -1 ) {
		value = res.substr(0, pos) + "." + res.substr(pos + 1, res.size());
		value = "-" + value;
	}
	else {
		value = res.substr(0, 2) + "." + res.substr(2, 2);
	}

	return to_double(value);
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double EW2SC3::getSeiscompUncertainty(double value) {

	double weight;

	if ( value != _uncertaintyMax )
		weight = _uncertainties.at((int) ((value / _uncertaintyMax) * _uncertainties.size()));
	else
		weight = _uncertainties.at(_uncertainties.size() - 1);

	return weight;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
string EW2SC3::sexagesimal_to_decimal(double deg, double min, string pol) {

	string value;
	double x = min / 60;
	double y = abs(deg) + x;
	if ( (pol == "S") || (pol == "W") || (pol == "s") || (pol == "w") )
		y = -y;
	value = to_string(y);

	return value;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
template<typename T>
string EW2SC3::to_string(const T &value) {

	stringstream sstr;
	sstr << value;

	return sstr.str();
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double EW2SC3::to_double(const string &value) {

	stringstream ss(value);
	double f;
	ss >> f;

	return f;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int EW2SC3::to_int(const string &value) {

	int number;
	istringstream iss(value);
	iss >> number;

	return number;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
string EW2SC3::blank_replace(string &str, string &str1) {

	for (unsigned int i = 0; i < str.length(); i++) {
		if ( str[i] == ' ' ) {
			str.replace(i, str1.length(), str1);
			i--;
		}
	}

	return str;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
string EW2SC3::strip_space(string &str) {

	for (unsigned int i = 0; i < str.length(); i++) {
		if ( str[i] == ' ' ) {
			str.erase(i, 1);
			i--;
		}
	}

	return str;
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EW2SC3::strExplode(const string &s, char c, vector<string> &v) {

	string::size_type i = 0;
	string::size_type j = s.find(c);
	while ( j != string::npos ) {
		v.push_back(s.substr(i, j - i));
		i = ++j;
		j = s.find(c, j);
		if ( j == string::npos ) {
			string st = s.substr(i, s.length());
			if ( strip_space(st) != "" )
				v.push_back(s.substr(i, s.length()));
		}
	}
}
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

