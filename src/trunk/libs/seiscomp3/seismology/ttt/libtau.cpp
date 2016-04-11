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

#include <seiscomp3/system/environment.h>
#include <seiscomp3/math/geo.h>
#include <seiscomp3/seismology/ttt/libtau.h>


namespace {


double takeoff_angle(double p, double zs, double vzs) {
	// Compute the "takeoff angle" of a wave at the source.
	//
	// p  is the *angular* slowness of the wave, i.e. sec/deg
	// zs is the source depth
	// vz is the velocity at the source
	double pv;

	p  = p*180./M_PI;       // make p slowness in sec/rad
	pv = p*vzs/(6371.-zs);
	if (pv>1.) pv = 1.;

	return 180.*asin(pv)/M_PI; 
}


}


extern "C" {

#include <libtau/tau.h>

void distaz2_(double *lat1, double *lon1, double *lat2, double *lon2, double *delta, double *azi1, double *azi2);

}


namespace Seiscomp {
namespace TTT {



std::string LibTau::_model;
int LibTau::_tabinCount = 0;
double LibTau::_depth = -1;


LibTau::LibTau() : _initialized(false) {}


LibTau::LibTau(const LibTau &other) {
	*this = other;
}


LibTau &LibTau::operator=(const LibTau &other) {
	_initialized = other._initialized;
	if ( _initialized )
		++_tabinCount;

	return *this;
}


LibTau::~LibTau() {
	if ( !_initialized ) return;

	if ( _tabinCount > 0 ) {
		--_tabinCount;
		if ( _tabinCount < 1 )
			_tabinCount = 1;
	}

	if ( !_tabinCount ) {
		_model.clear();
		_depth = -1;
		tabout();
	}
}


bool LibTau::setModel(const std::string &model) {
	if ( _tabinCount && _model != model ) return false;

	_initialized = true;

	InitPath(model);
	++_tabinCount;

	return true;
}


const std::string &LibTau::model() const {
	return _model;
}


void LibTau::InitPath(const std::string &model) {
	if ( _tabinCount ) {
		if ( model != _model )
			throw MultipleModelsError(_model);
		return;
	}

	std::string tablePath = Environment::Instance()->shareDir() +
	                        "/ttt/" + model;

	int err = tabin(tablePath.c_str());
	if ( err ) {
		std::ostringstream errmsg;
		errmsg  << tablePath << ".hed and "
			<< tablePath << ".tbl";
		throw FileNotFoundError(errmsg.str());
	}

	brnset("all");
	_model = model;
}

void LibTau::SetDepth(double depth) {
	if ( depth <= 0. ) depth = 0.01; // XXX Hack!!!
	
	if ( depth <= 0. || depth > 800 ) {
		std::ostringstream errmsg;
		errmsg.precision(8);
		errmsg  << "Source depth of " << depth 
			<< " km is out of range of 0 < z <= 800";
		throw std::out_of_range(errmsg.str());
	}

	if ( depth != _depth ) {
		_depth = depth;
		depset(_depth);
	}
}


TravelTimeList *LibTau::compute(double delta, double depth) {
	int n;
	char ph[1000], *phase[100];
	float time[100], p[100], dtdd[100], dtdh[100], dddp[100], vp, vs;
	TravelTimeList *ttlist = new TravelTimeList;
	ttlist->delta = delta;
	ttlist->depth = depth;
	
	SetDepth(depth);

	for(int i=0; i<100; i++)
		phase[i] = &ph[10*i];

	trtm(delta, &n, time, p, dtdd, dtdh, dddp, phase);
	emdlv(6371-depth, &vp, &vs);

	for(int i=0; i<n; i++) {
		float v = (phase[i][0]=='s' || phase[i][0]=='S') ? vs : vp;
		float takeoff = takeoff_angle(dtdd[i], depth, v);

		if (dtdh[i] > 0.)
			takeoff = 180.-takeoff;
		ttlist->push_back(
			TravelTime(phase[i],
				   time[i], dtdd[i], dtdh[i], dddp[i],
				   takeoff) );
	}

	ttlist->sortByTime();

	return ttlist;
}


TravelTimeList *LibTau::compute(double lat1, double lon1, double dep1,
                                double lat2, double lon2, double alt2,
                                int ellc /* by now always 0 */) {
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
		if (ellipcorr((*it).phase, lat1, lon1, lat2, lon2, dep1, ecorr)) {
//			fprintf(stderr, " %7.3f %5.1f  TT = %.3f ecorr = %.3f\n", delta, dep1, (*it).time, ecorr);
			(*it).time += ecorr;
		}
	}
	return ttlist;
}


TravelTime LibTau::computeFirst(double delta, double depth) throw(NoPhaseError) {
	int n;
	char ph[1000], *phase[100];
	float time[100], p[100], dtdd[100], dtdh[100], dddp[100], vp, vs;
	
	SetDepth(depth);

	for(int i=0; i<100; i++)
		phase[i] = &ph[10*i];

	trtm(delta, &n, time, p, dtdd, dtdh, dddp, phase);
	emdlv(6371-depth, &vp, &vs);

	if (n) {
		float v = (phase[0][0]=='s' || phase[0][0]=='S') ? vs : vp;
		float takeoff = takeoff_angle(dtdd[0], depth, v);
		if (dtdh[0] > 0.)
			takeoff = 180.-takeoff;

		return TravelTime(phase[0], time[0], dtdd[0], dtdh[0], dddp[0], takeoff);
	}

	throw NoPhaseError();
}


TravelTime LibTau::computeFirst(double lat1, double lon1, double dep1,
                                   double lat2, double lon2, double alt2,
                                   int ellc) throw(NoPhaseError) {
	if ( !_tabinCount ) setModel("iasp91");

	double delta, azi1, azi2;

	Math::Geo::delazi(lat1, lon1, lat2, lon2, &delta, &azi1, &azi2);

	/* TODO apply ellipticity correction */
	return computeFirst(delta, dep1);
}


REGISTER_TRAVELTIMETABLE(LibTau, "libtau");


}
}
