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



#include <math.h>
#include <iostream>
#include <stdexcept>

#include <seiscomp3/config/environment.h>
#include <seiscomp3/math/geo.h>
#include <seiscomp3/seismology/ttt/locsat.h>


namespace Seiscomp {
namespace TTT {


extern "C" {

void distaz2_(double *lat1, double *lon1, double *lat2, double *lon2, double *delta, double *azi1, double *azi2);
int setup_tttables_dir (const char *new_dir);
double compute_ttime(double distance, double depth, char *phase, int extrapolate);
int num_phases();
char **phase_types();

}


std::string Locsat::_model;
int Locsat::_tabinCount = 0;

Locsat::Locsat() : _initialized(false) {}


Locsat::Locsat(const Locsat &other) {
	*this = other;
}


Locsat &Locsat::operator=(const Locsat &other) {
	_initialized = other._initialized;
	if ( _initialized ) {
		++_tabinCount;
	}

	return *this;
}


Locsat::~Locsat() {
	if ( _initialized ) {
		--_tabinCount;

		if ( !_tabinCount ) {
			_model.clear();
		}
	}
}


bool Locsat::setModel(const std::string &model) {
	if ( _tabinCount && _model != model ) return false;

	if ( !_tabinCount )
		InitPath(model);

	++_tabinCount;

	_initialized = true;

	return true;
}


const std::string &Locsat::model() const {
	return _model;
}


void Locsat::InitPath(const std::string &model)
{
	if ( _tabinCount ) {
		if ( model != _model )
			throw MultipleModelsError(_model);
		return;
	}

	setup_tttables_dir((Environment::Instance()->shareDir() + "/locsat/tables/" + model).c_str());
	_model = model;
}


TravelTimeList *Locsat::compute(double delta, double depth) {
	int nphases = num_phases();
	char **phases = phase_types();

	TravelTimeList *ttlist = new TravelTimeList;
	ttlist->delta = delta;
	ttlist->depth = depth;

	for ( int i = 0; i < nphases; ++i ) {
		char *phase = phases[i];
		double ttime = compute_ttime(delta, depth, phase, 1);
		if ( ttime < 0 ) continue;

		ttlist->push_back(TravelTime(phase, ttime, 0, 0, 0, 0));
	}

	ttlist->sortByTime();

	return ttlist;
}


TravelTimeList *Locsat::compute(double lat1, double lon1, double dep1,
                                double lat2, double lon2, double alt2,
                                int ellc) {
	if ( !_tabinCount ) setModel("iasp91");

	double delta, azi1, azi2;

//	Math::Geo::delazi(lat1, lon1, lat2, lon2, &delta, &azi1, &azi2);
	distaz2_(&lat1, &lon1, &lat2, &lon2, &delta, &azi1, &azi2);

	/* TODO apply ellipticity correction */
	TravelTimeList *ttlist = compute(delta, dep1);
	ttlist->delta = delta;
	ttlist->depth = dep1;
	TravelTimeList::iterator it;
	for (it = ttlist->begin(); it != ttlist->end(); ++it) {
		double ecorr = 0.;
		if ( ellipcorr((*it).phase, lat1, lon1, lat2, lon2, dep1, ecorr) ) {
//			fprintf(stderr, " %7.3f %5.1f  TT = %.3f ecorr = %.3f\n", delta, dep1, (*it).time, ecorr);
			(*it).time += ecorr;
		}
	}
	return ttlist;
}


TravelTime Locsat::computeFirst(double delta, double depth) throw(NoPhaseError) {
	int nphases = num_phases();
	char **phases = phase_types();

	if ( nphases < 1 ) throw NoPhaseError();
	char *phase = phases[0];
	double ttime = compute_ttime(delta, depth, phase, 1);
	if ( ttime < 0 ) throw NoPhaseError();

	return TravelTime(phase, ttime, 0, 0, 0, 0);
}


TravelTime Locsat::computeFirst(double lat1, double lon1, double dep1,
                                double lat2, double lon2, double alt2,
                                int ellc) throw(NoPhaseError) {
	if ( !_tabinCount ) setModel("iasp91");

	double delta, azi1, azi2;

	Math::Geo::delazi(lat1, lon1, lat2, lon2, &delta, &azi1, &azi2);

	/* TODO apply ellipticity correction */
	return computeFirst(delta, dep1);
}


REGISTER_TRAVELTIMETABLE(Locsat, "LOCSAT");


}
}
