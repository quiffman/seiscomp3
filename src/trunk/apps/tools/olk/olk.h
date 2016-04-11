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





#include <vector>
#include <stdlib.h>
#include <fstream> 
#include <iterator>
#include <iostream>
#include <iomanip>

#include <seiscomp3/logging/log.h>
#include <seiscomp3/client/inventory.h>
#include <seiscomp3/datamodel/station.h>
#include <seiscomp3/datamodel/origin.h>
#include <seiscomp3/datamodel/pick.h>
#include <seiscomp3/datamodel/event.h>
#include <seiscomp3/datamodel/amplitude.h>
#include <seiscomp3/datamodel/magnitude.h>
#include <seiscomp3/datamodel/stationmagnitude.h>
#include <seiscomp3/datamodel/eventparameters.h>
//eventparameters_package bindet alles ein

#include <seiscomp3/client/application.h>
#include <seiscomp3/datamodel/messages.h>
#include <seiscomp3/core/datamessage.h>

#include <seiscomp3/core/datetime.h>
#include <seiscomp3/core/strings.h>

#include <seiscomp3/math/geo.h>
#include <seiscomp3/seismology/ttt.h>

#include <seiscomp3/core/interruptible.h>

//#include <seiscomp3/core/typedarray.h>
//#include <seiscomp3/core/recordsequence.h>
//#include <seiscomp3/io/recordinput.h>
//#include <seiscomp3/io/recordstream.h>
//#include <seiscomp3/processing/streambuffer.h>
//#include <seiscomp3/math/filter.h>

namespace Seiscomp {

namespace Applications {

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
class olk : public Seiscomp::Client::Application {
	public:
		
		olk(int argc, char** argv);
		~olk(){};

	protected:
		
		bool initConfiguration();
		bool init();
		bool run();
		
	private:
		DataModel::Inventory* _inv;
		std::vector<DataModel::EventPtr> _events;
		std::vector<DataModel::OriginPtr> _origins;
		std::vector<DataModel::PickPtr> _picks;	
		std::vector<DataModel::AmplitudePtr> _amplitudes;		
		int _counter;
		bool readEventsFromDB(double days);
		int _actualEvent;
		DataModel::MagnitudePtr _netMag;
		DataModel::OriginPtr _originInRevision;
		std::vector<DataModel::OriginPtr> sentOrigins;
//		Seiscomp::Math::Filtering::InPlaceFilter<float> *_filter;
//		Processing::StreamBuffer _recordBuffer;

	private: //Methods	
		void commit(DataModel::OriginPtr originToShow);
		void plotEvents();
		void plotEventInformation();
		void plotOriginInformation(DataModel::OriginPtr originToShow, bool shortInfo);
		void plotResiduals(DataModel::OriginPtr originToShow);
		void plotMagnitudes(DataModel::OriginPtr originToShow, int cmdMagType);
		DataModel::Origin fitRMS(double newrms, DataModel::Origin sourceOrigin);
		void loadPicksnAmplitudes(const std::string& originID);
		double getMax(std::vector<double> values); //Can be replaced if correct function is known
		void plotDiagram(std::vector<double> xvalues, std::vector<double> yvalues, std::vector<double> weight);
		void deselectArrivalByDistance(DataModel::OriginPtr tempOrigin);
		void deselectArrivalByResidual(DataModel::OriginPtr tempOrigin);
		void selectAllArrivals(DataModel::OriginPtr tempOrigin);
//		void deselectMagnitudeByResidual(DataModel::OriginPtr tempOrigin);
//		void selectAllmagnitude(DataModel::OriginPtr tempOrigin);
		bool reLocate(DataModel::OriginPtr tempOrigin, std::string depth);
//		bool reCalculate(DataModel::OriginPtr tempOrigin);
		DataModel::OriginPtr copyOrigin(DataModel::OriginPtr sourceOrigin);
		void listMagTypes(DataModel::OriginPtr originToShow);
		void menue();
		bool fixDepth(DataModel::OriginPtr tempOrigin);
		//void pick(DataModel::OriginPtr originToShow);
		//Record* filteredRecord(const Record* rec) const;
		//Core::TimeWindow arrivalTimeSpan(DataModel::OriginPtr tempOrigin);
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

}

}
