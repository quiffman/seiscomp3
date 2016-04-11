/*===========================================================================================================================
    Name:       inventory.h

    Language:   C++, ANSI standard.

    Author:     Peter de Boer, KNMI

    Revision:	2008-01-17	0.1	initial version

===========================================================================================================================*/
#ifndef MYINVENTORY_H
#define MYINVENTORY_H

#include <sstream>
#include <seiscomp3/communication/connection.h>
#include <seiscomp3/datamodel/inventory.h>
#include <seiscomp3/datamodel/qualitycontrol.h>
#include "synchro.h"
#include "define.h"
#include "tmanip.h"
#include "mystring.h"
#include "seed.h"

class Inventory
{
	public:
		Inventory(INIT_MAP& init, Seiscomp::Communication::Connection* connection, Seiscomp::Applications::SynchroCallbacks* cb);
		VolumeIndexControl *vic;
		AbbreviationDictionaryControl *adc;
		StationControl *sc;
		bool SetConnection();
		void CloseConnection();
		void SendNotifiers();
		void SynchronizeInventory();
	protected:
	private:
		//std::stringstream command, output;
		std::string station_name;
		std::string channel_name;
		std::map<std::string, Seiscomp::DataModel::NetworkPtr> networks;
		std::map<std::pair<std::pair<std::string, std::string>, Seiscomp::Core::Time>, Seiscomp::DataModel::StationPtr> stations;
		std::set<std::pair<std::pair<std::pair<std::pair<std::string, std::string>, std::string>, Seiscomp::Core::Time>, Seiscomp::Core::Time > > sensor_locations;
		std::set<std::pair<std::pair<std::pair<std::pair<std::pair<std::pair<std::string, std::string>, std::string>, std::string>, Seiscomp::Core::Time>, Seiscomp::Core::Time>, Seiscomp::Core::Time> > seis_streams;
		std::set<std::pair<std::pair<std::pair<std::pair<std::pair<std::pair<std::string, std::string>, std::string>, std::string>, Seiscomp::Core::Time>, Seiscomp::Core::Time>, Seiscomp::Core::Time> > aux_streams;
		INIT_MAP init_values;
		std::map<std::vector<std::string>, std::string> encoding;
		Seiscomp::Communication::Connection* _connection;
		Seiscomp::DataModel::QualityControl* _qc;
		Seiscomp::Applications::SynchroCallbacks* _cb;
		//STATION_INFO info;
		//Logging *log;
		int sequence_number;
		void ProcessStation();
		void CleanupDatabase();
		void GetComment(StationIdentifier&);
		void GetStationComment(Comment&, Seiscomp::DataModel::WaveformStreamID *);
		void GetChannelComment(ChannelIdentifier&, Seiscomp::DataModel::WaveformStreamID *);
		Seiscomp::Core::Time GetTime(std::string);
		void ProcessStream(StationIdentifier&, Seiscomp::DataModel::StationPtr);
		void ProcessComponent(ChannelIdentifier&, Seiscomp::DataModel::StreamPtr); 
		void ProcessDatalogger(ChannelIdentifier&, Seiscomp::DataModel::StreamPtr);
		void ProcessDecimation(ChannelIdentifier&, Seiscomp::DataModel::DataloggerPtr, Seiscomp::DataModel::StreamPtr);
		void ProcessDataloggerCalibration(ChannelIdentifier&, Seiscomp::DataModel::DataloggerPtr, Seiscomp::DataModel::StreamPtr);
		void ProcessDataloggerFIR(ChannelIdentifier&, Seiscomp::DataModel::DataloggerPtr, Seiscomp::DataModel::StreamPtr strm);
		void ProcessDataloggerPAZ(ChannelIdentifier&, Seiscomp::DataModel::DataloggerPtr, Seiscomp::DataModel::StreamPtr strm);
		void ProcessPAZSensor(ChannelIdentifier&, Seiscomp::DataModel::StreamPtr);
		void ProcessPolySensor(ChannelIdentifier&, Seiscomp::DataModel::StreamPtr);
		void ProcessSensorCalibration(ChannelIdentifier&, Seiscomp::DataModel::SensorPtr, Seiscomp::DataModel::StreamPtr);
		void ProcessSensorPAZ(ChannelIdentifier& ci, Seiscomp::DataModel::SensorPtr sm);
		void ProcessSensorPolynomial(ChannelIdentifier& ci, Seiscomp::DataModel::SensorPtr sm);
		Seiscomp::DataModel::StationPtr InsertStation(StationIdentifier&, Seiscomp::DataModel::NetworkPtr);
		Seiscomp::DataModel::SensorLocationPtr InsertSensorLocation(ChannelIdentifier& ci, Seiscomp::DataModel::StationPtr station, const Seiscomp::Core::Time& loc_start, const Seiscomp::Core::Time& loc_end);
		Seiscomp::DataModel::StreamPtr InsertStream(ChannelIdentifier&, Seiscomp::DataModel::SensorLocationPtr, bool restricted);
		Seiscomp::DataModel::AuxStreamPtr InsertAuxStream(ChannelIdentifier&, Seiscomp::DataModel::SensorLocationPtr, bool restricted);
		Seiscomp::DataModel::DataloggerPtr InsertDatalogger(ChannelIdentifier&, Seiscomp::DataModel::StreamPtr, const std::string& name);
		void InsertDecimation(ChannelIdentifier&, Seiscomp::DataModel::DataloggerPtr, Seiscomp::DataModel::StreamPtr);
		void InsertDataloggerCalibration(ChannelIdentifier&, Seiscomp::DataModel::DataloggerPtr, Seiscomp::DataModel::StreamPtr);
		Seiscomp::DataModel::ResponseFIRPtr InsertRespCoeff(ChannelIdentifier&, unsigned int&);
		Seiscomp::DataModel::ResponseFIRPtr InsertResponseFIRr(ChannelIdentifier&, unsigned int&);
		Seiscomp::DataModel::SensorPtr InsertSensor(ChannelIdentifier&, Seiscomp::DataModel::StreamPtr, const char* unit, const std::string& name);
		void InsertSensorCalibration(ChannelIdentifier&, Seiscomp::DataModel::SensorPtr, Seiscomp::DataModel::StreamPtr);
		Seiscomp::DataModel::ResponsePAZPtr InsertResponsePAZ(ChannelIdentifier&, std::string);
		Seiscomp::DataModel::ResponsePolynomialPtr InsertResponsePolynomial(ChannelIdentifier&, std::string);
		void UpdateStation(StationIdentifier&, Seiscomp::DataModel::StationPtr);
		void UpdateSensorLocation(ChannelIdentifier& ci, Seiscomp::DataModel::SensorLocationPtr loc, const Seiscomp::Core::Time& loc_start, const Seiscomp::Core::Time& loc_end);
		void UpdateStream(ChannelIdentifier&, Seiscomp::DataModel::StreamPtr, bool restricted);
		void UpdateAuxStream(ChannelIdentifier&, Seiscomp::DataModel::AuxStreamPtr, bool restricted);
		void UpdateDatalogger(ChannelIdentifier&, Seiscomp::DataModel::DataloggerPtr, Seiscomp::DataModel::StreamPtr);
		void UpdateDecimation(ChannelIdentifier&, Seiscomp::DataModel::DecimationPtr, Seiscomp::DataModel::StreamPtr);
		void UpdateDataloggerCalibration(ChannelIdentifier&, Seiscomp::DataModel::DataloggerCalibrationPtr, Seiscomp::DataModel::StreamPtr);
		void UpdateRespCoeff(ChannelIdentifier&, Seiscomp::DataModel::ResponseFIRPtr, unsigned int&);
		void UpdateResponseFIRr(ChannelIdentifier&, Seiscomp::DataModel::ResponseFIRPtr, unsigned int&);
		void UpdateSensor(ChannelIdentifier&, Seiscomp::DataModel::SensorPtr, const char* unit);
		void UpdateSensorCalibration(ChannelIdentifier&, Seiscomp::DataModel::SensorCalibrationPtr, Seiscomp::DataModel::StreamPtr);
		void UpdateResponsePAZ(ChannelIdentifier&, Seiscomp::DataModel::ResponsePAZPtr);
		void UpdateResponsePolynomial(ChannelIdentifier&, Seiscomp::DataModel::ResponsePolynomialPtr);
		std::string GetNetworkDescription(int);
		std::string GetInstrumentName(int);
		std::string GetInstrumentManufacturer(int);
		std::string GetInstrumentType(int);
		std::string GetStationInstrument(int);
		int GetPAZSequence(ChannelIdentifier&, std::string, std::string);
		int GetPolySequence(ChannelIdentifier&, std::string, std::string);
		bool IsPAZStream(ChannelIdentifier& ci);
		bool IsPolyStream(ChannelIdentifier& ci);
};
#endif /* MYINVENTORY_H */
