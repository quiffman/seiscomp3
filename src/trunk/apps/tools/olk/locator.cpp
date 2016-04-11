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




#include "locator.h"
#include "olk.h"

namespace Seiscomp {

namespace {

struct comparePick {
	bool operator()(const Seismology::LocatorInterface::WeightedPick &first,
	                const Seismology::LocatorInterface::WeightedPick &second) const {
		return first.first->time().value() < second.first->time().value();
	}
};

}


DataModel::Origin* Locator::relocate(DataModel::Origin* origin) {
	DataModel::Origin* newOrg = NULL;
	std::string errorMsg;

	try {
		newOrg = Seiscomp::LocSAT::relocate(origin);
		if ( newOrg )
			return newOrg;

		errorMsg = "The Relocation failed for some reason.";
	}
	catch ( Core::GeneralException& e ) {
		errorMsg = e.what();
	}

	PickList picks;
	for ( size_t i = 0; i < origin->arrivalCount(); ++i ) {
		DataModel::Arrival* arrival = origin->arrival(i);
		try {
			if ( arrival->weight() < 0.5 ) continue;
		}
		catch ( ... ) {}

		DataModel::Pick* pick = getPick(arrival);
		if ( !pick )
			throw Core::GeneralException("pick '" + arrival->pickID() + "' not found");

		picks.push_back(Seismology::LocatorInterface::WeightedPick(pick,1));
	}

	if ( picks.empty() )
		throw Core::GeneralException("No picks given to relocate");

	std::sort(picks.begin(), picks.end(), comparePick());
	DataModel::SensorLocation *sloc = getSensorLocation(picks.front().first.get());
	if ( !sloc )
		throw Core::GeneralException("sensor location '" + picks.front().first->waveformID().networkCode() +
		                             "." + picks.front().first->waveformID().stationCode() + "' not found");

	newOrg = locate(picks, sloc->latitude(), sloc->longitude(), 11.0, picks.front().first->time());
	DataModel::OriginPtr tmp = DataModel::Origin::Create();
	*tmp = *origin;
	for ( size_t i = 0; i < origin->arrivalCount(); ++i ) {
		DataModel::ArrivalPtr ar = new DataModel::Arrival(*origin->arrival(i));
		tmp->add(ar.get());
	}

	tmp->setLatitude(sloc->latitude());
	tmp->setLongitude(sloc->longitude());
	tmp->setDepth(DataModel::RealQuantity(11.0));
	tmp->setTime(picks.front().first->time());

	newOrg = Seiscomp::LocSAT::relocate(tmp.get());

	if ( newOrg )
		return newOrg;

	throw Core::GeneralException(errorMsg);
}



}
