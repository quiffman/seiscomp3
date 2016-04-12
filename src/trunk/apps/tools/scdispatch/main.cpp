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

#define SEISCOMP_COMPONENT Dispatcher

#include <seiscomp3/logging/log.h>
#include <seiscomp3/client/application.h>
#include <seiscomp3/communication/servicemessage.h>
#include <seiscomp3/io/archive/xmlarchive.h>
#include <seiscomp3/utils/timer.h>

#include <seiscomp3/datamodel/eventparameters_package.h>


using namespace Seiscomp;


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
class ObjectWriter : protected DataModel::Visitor {
	// ----------------------------------------------------------------------
	// Nested types
	// ----------------------------------------------------------------------
	private:
		typedef std::map<std::string, std::string> RoutingTable;

	// ----------------------------------------------------------------------
	//  X'struction
	// ----------------------------------------------------------------------
	public:
		ObjectWriter(Communication::Connection* connection,
		             DataModel::Operation op,
		             bool test)
		 : Visitor(op != DataModel::OP_REMOVE?DataModel::Visitor::TM_TOPDOWN:DataModel::Visitor::TM_BOTTOMUP),
		   _connection(connection),
		   _errors(0),
		   _count(0),
		   _operation(op),
		   _test(test) {
			_routingTable.insert(std::make_pair(DataModel::Pick::ClassName(), "PICK"));
			_routingTable.insert(std::make_pair(DataModel::Amplitude::ClassName(), "AMPLITUDE"));
			_routingTable.insert(std::make_pair(DataModel::Arrival::ClassName(), "LOCATION"));
			_routingTable.insert(std::make_pair(DataModel::Origin::ClassName(), "LOCATION"));
			_routingTable.insert(std::make_pair(DataModel::OriginReference::ClassName(), "LOCATION"));
			_routingTable.insert(std::make_pair(DataModel::StationMagnitude::ClassName(), "MAGNITUDE"));
			_routingTable.insert(std::make_pair(DataModel::StationMagnitudeContribution::ClassName(), "MAGNITUDE"));
			_routingTable.insert(std::make_pair(DataModel::Magnitude::ClassName(), "MAGNITUDE"));
			_routingTable.insert(std::make_pair(DataModel::FocalMechanism::ClassName(), "FOCMECH"));
			_routingTable.insert(std::make_pair(DataModel::MomentTensor::ClassName(), "FOCMECH"));
			_routingTable.insert(std::make_pair(DataModel::MomentTensorStationContribution::ClassName(), "FOCMECH"));
			_routingTable.insert(std::make_pair(DataModel::MomentTensorComponentContribution::ClassName(), "FOCMECH"));
			_routingTable.insert(std::make_pair(DataModel::MomentTensorPhaseSetting::ClassName(), "FOCMECH"));
			_routingTable.insert(std::make_pair(DataModel::EventDescription::ClassName(), "EVENT"));
			_routingTable.insert(std::make_pair(DataModel::Event::ClassName(), "EVENT"));
		}

	// ----------------------------------------------------------------------
	//  Public interface
	// ----------------------------------------------------------------------
	public:
		//! Does the actual writing
		bool operator()(DataModel::Object* object) {
			std::cout << "operator()(DataModel::Object* object)" << std::endl;
			return (*this)(object, "");
		}

		bool operator()(DataModel::Object* object, const std::string& parentID) {
			std::cout << "operator()(DataModel::Object* object, const std::string& parentID)" << std::endl;
			std::cout << parentID << std::endl;
			_parentID = parentID;
			_errors = 0;
			_count = 0;

			object->accept(this);

			return _errors == 0;
		}

		//! Returns the number of handled objects
		int count() const { return _count; }

		//! Returns the number of errors while writing
		int errors() const { return _errors; }

		void setOperation(DataModel::Operation op) {
			_operation = op;
		}

		DataModel::Operation operation() const {
			return _operation;
		}

		void setRoutingTable(const std::map<std::string, std::string>& table) {
			_routingTable.clear();
			_routingTable = table;
			std::map<std::string, std::string>::const_iterator it = table.begin();
			for ( ; it != table.end(); it++ )
				std::cout << it->first << " : " << it->second << std::endl;
		}

	// ----------------------------------------------------------------------
	//  Visitor interface
	// ----------------------------------------------------------------------
	protected:
		bool visit(DataModel::PublicObject* publicObject) {
			// std::cout << "ObjectWriter::visit(DataModel::PublicObject* publicObject)" << std::endl;
			return write(publicObject);
		}

		void visit(DataModel::Object* object) {
			// std::cout << "ObjectWriter::visit(DataModel::Object* object)" << std::endl;
			write(object);
		}

	// ----------------------------------------------------------------------
	//  Implementation
	// ----------------------------------------------------------------------
	private:
		bool write(DataModel::Object* object) {
			if ( SCCoreApp->isExitRequested() ) return false;

			RoutingTable::iterator targetIt = _routingTable.find(object->className());
			if ( targetIt == _routingTable.end() )
				return false;

			std::cout << "ObjectWriter::write(DataModel::Object* object) : object = " << object->className() << std::endl;
			Seiscomp::DataModel::PublicObject* parent = object->parent();

			if ( !parent ) {
				std::cout << "No parent found for object " << object->className() << std::endl;
			}

			std::cout << "parentID: " << parent->publicID() << std::endl;
			_parentID.clear();
			_parentID = parent->publicID();

			++_count;

			if ( _test ) return true;

			DataModel::NotifierMessage notifierMessage;
			notifierMessage.attach(new DataModel::Notifier(_parentID, _operation, object));

			unsigned int counter = 0;
			while ( counter <= 4 ) {
				if ( _connection->send(targetIt->second, &notifierMessage) ) {
					if ( _count % 100 == 0 ) SCCoreApp->sync();
					return true;
				}

				std::cout << "Could not send object " << object->className()
				<< " to " << targetIt->second << "@" << _connection->masterAddress()
				<< std::endl;
				if ( _connection->isConnected() ) break;
				++counter;
				sleep(1);
			}

			++_errors;
			return false;
		}

	// ----------------------------------------------------------------------
	// Private data members
	// ----------------------------------------------------------------------
	private:
		Communication::Connection* _connection;
		std::string                _parentID;
		int                        _errors;
		int                        _count;
		DataModel::Operation       _operation;
		RoutingTable               _routingTable;
		bool                       _test;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
class ObjectCounter : protected DataModel::Visitor {
	public:
		ObjectCounter(DataModel::Object* object) : DataModel::Visitor(), _count(0) {
			object->accept(this);
		}

	public:
		unsigned int count() const { return _count; }

	protected:
		bool visit(DataModel::PublicObject*) {
			++_count;
			return true;
		}

		virtual void visit(DataModel::Object*) {
			++_count;
		}

	private:
		unsigned int _count;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
class ObjectDispatcher : public ObjectWriter {

	// ----------------------------------------------------------------------
	//  X'struction
	// ----------------------------------------------------------------------
	public:
		ObjectDispatcher(Communication::Connection* connection,
		                 DataModel::Operation op,
		                 bool test,
		                 unsigned int total,
		                 unsigned int totalProgress)
		 : ObjectWriter(connection, op, test),
		   _total(total),
		   _totalProgress(totalProgress),
		   _lastStep(-1),
		   _failure(0) {}


	// ----------------------------------------------------------------------
	//  Visitor interface
	// ----------------------------------------------------------------------
	protected:
		bool visit(DataModel::PublicObject* publicObject) {
			bool result = ObjectWriter::visit(publicObject);
			if ( !result )
				_failure += ObjectCounter(publicObject).count()-1;

			updateProgress();

			return result;
		}


		void visit(DataModel::Object* object) {
			ObjectWriter::visit(object);
			updateProgress();
		}


		void updateProgress() {
			unsigned int current = count() + _failure;
			unsigned int progress = current * _totalProgress / _total;
			if ( progress != _lastStep ) {
				_lastStep = progress;
				std::cout << "." << std::flush;
			}
		}


	private:
		unsigned int _total;
		unsigned int _totalProgress;
		unsigned int _lastStep;
		unsigned int _failure;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
class DispatchTool : public Seiscomp::Client::Application {
	public:
		DispatchTool(int argc, char** argv)
		 : Application(argc, argv),
		   _operation(DataModel::OP_ADD) {

			setAutoApplyNotifierEnabled(false);
			setInterpretNotifierEnabled(false);
			setMessagingEnabled(true);
			setDatabaseEnabled(false, false);
			setPrimaryMessagingGroup(Communication::Protocol::LISTENER_GROUP);
			addMessagingSubscription("");
		}


	protected:
		void createCommandLineDescription() {
			commandline().addGroup("Dispatch");
			commandline().addOption("Dispatch", "input,i", "File to dispatch to messaging", &_inputFile, false);
			commandline().addOption(
					"Dispatch",
					"operation,O",
					"Notifier operation: add, update or remove",
					&_notifierOperation,
					false
			);
			commandline().addOption("Dispatch", "routingtable",
			                        "Specify routing table as list of object:group pairs",
			                        &_routingTable, false);

			commandline().addOption("Dispatch", "print-objects", "Print names of routable objects");
			commandline().addOption("Dispatch", "test", "Do not send any object");
		}


		bool validateParameters() {
			if ( commandline().hasOption("operation") ) {
				if ( !_operation.fromString(_notifierOperation) ) {
					std::cout << "Notifier operation " << _notifierOperation << " is not valid" << std::endl;
					std::cout << "Operations are add, update and remove" << std::endl;
					return false;
				}
			}
			else if ( commandline().hasOption("print-objects") ) {
				std::cout << DataModel::Pick::ClassName() << std::endl;
				std::cout << DataModel::Arrival::ClassName() << std::endl;
				std::cout << DataModel::Amplitude::ClassName() << std::endl;
				std::cout << DataModel::Origin::ClassName() << std::endl;
				std::cout << DataModel::OriginReference::ClassName() << std::endl;
				std::cout << DataModel::StationMagnitude::ClassName() << std::endl;
				std::cout << DataModel::StationMagnitudeContribution::ClassName() << std::endl;
				std::cout << DataModel::Magnitude::ClassName() << std::endl;
				std::cout << DataModel::EventDescription::ClassName() << std::endl;
				std::cout << DataModel::Event::ClassName() << std::endl;
				return false;
			}

			if ( !commandline().hasOption("input") ) {
				std::cerr << "No input given" << std::endl;
				return false;
			}

			if ( commandline().hasOption("test") )
				setMessagingEnabled(false);

			return true;
		}


		virtual bool initConfiguration() {
			if ( !Application::initConfiguration() ) return false;
			return true;
		}


		virtual bool initSubscriptions() {
			return true;
		}


		bool run() {
			if ( commandline().hasOption("input") )
				return importDatabase();

			return false;
		}


		bool importDatabase() {
			IO::XMLArchive ar;
			if ( _inputFile == "-" )
				ar.open(std::cin.rdbuf());
			else if ( !ar.open(_inputFile.c_str()) ) {
				std::cout << "Error: could not open input file '" << _inputFile << "'" << std::endl;
				return false;
			}

			std::cout << "Parsing file '" << _inputFile << "'..." << std::endl;

			Util::StopWatch timer;
			DataModel::ObjectPtr doc;
			ar >> doc;
			ar.close();

			if ( doc == NULL ) {
				std::cout << "Error: no valid object found in file '" << _inputFile << "'" << std::endl;
				return false;
			}

			std::cout << _operation.toString() << std::endl;
			ObjectDispatcher dispatcher(connection(), _operation, commandline().hasOption("test"), ObjectCounter(doc.get()).count(), 78);

			if ( commandline().hasOption("routingtable") ) {
				std::cout << _routingTable << std::endl;
				std::vector<std::string> tableEntries;
				Core::split(tableEntries, _routingTable.c_str(), ",", false);

				std::map<std::string, std::string> tmpRoutingTable;
				std::vector<std::string>::iterator it = tableEntries.begin();
				for ( ; it != tableEntries.end(); it++ ) {
					std::vector<std::string> entry;
					Core::split(entry, (*it).c_str(), ":", false);
					tmpRoutingTable.insert(std::make_pair(entry[0], entry[1]));
				}
				dispatcher.setRoutingTable(tmpRoutingTable);
			}

			std::cout << "Time needed to parse XML: " << Core::Time(timer.elapsed()).toString("%T.%f") << std::endl;
			std::cout << "Document object type: " << doc->className() << std::endl;
			std::cout << "Total number of objects: " << ObjectCounter(doc.get()).count() << std::endl;

			if ( connection() )
				std::cout << "Dispatching " << doc->className() << " to " << connection()->masterAddress() << std::endl;
			timer.restart();

			dispatcher(doc.get());
			sync();
			std::cout << std::endl;

			std::cout << "While dispatching " << dispatcher.count() << " objects " << dispatcher.errors() << " errors occured" << std::endl;
			std::cout << "Time needed to dispatch " << dispatcher.count() << " objects: " << Core::Time(timer.elapsed()).toString("%T.%f") << std::endl;

			return true;
		}

	// ----------------------------------------------------------------------
	// Private data member
	// ----------------------------------------------------------------------
	private:
		std::string          _inputFile;
		std::string          _notifierOperation;
		std::string          _routingTable;
		DataModel::Operation _operation;

};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int main(int argc, char** argv) {
	DispatchTool app(argc, argv);
	return app.exec();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



