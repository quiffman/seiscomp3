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


#define SEISCOMP_COMPONENT STAXML
#include "convert2sc3.h"

#include <stationxml/stamessage.h>
#include <stationxml/network.h>
#include <stationxml/station.h>
#include <stationxml/stationepoch.h>
#include <stationxml/channel.h>
#include <stationxml/channelepoch.h>
#include <stationxml/response.h>
#include <stationxml/coefficients.h>
#include <stationxml/fir.h>
#include <stationxml/numeratorcoefficient.h>
#include <stationxml/polynomial.h>
#include <stationxml/polynomialcoefficient.h>
#include <stationxml/polesandzeros.h>
#include <stationxml/poleandzero.h>

#include <seiscomp3/core/timewindow.h>
#include <seiscomp3/datamodel/inventory_package.h>
#include <seiscomp3/datamodel/utils.h>
#include <seiscomp3/io/archive/xmlarchive.h>
#include <seiscomp3/utils/replace.h>
#include <seiscomp3/logging/log.h>

#include <boost/lexical_cast.hpp>

#include <iostream>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>


using namespace std;


namespace Seiscomp {


namespace {


#define UNDEFINED   ""
#define ACCEL1      "M/S**2"
#define ACCEL2      "M/S2"
#define VELOCITY    "M/S"
#define DISPLACE    "M"
#define CURRENT     "V"
#define DIGITAL     "COUNTS"
#define TEMPERATURE "C"
#define PRESSURE    "PA"


typedef pair<double,double> Location;
typedef pair<Location,double> LocationElevation;
typedef pair<string,Location> EpochIndex;
typedef pair<StationXML::Channel*,StationXML::ChannelEpoch*> ChannelEpoch;

bool epochLowerThan(const ChannelEpoch &e1, const ChannelEpoch &e2) {
	return e1.second->start() < e2.second->start();
}

typedef list<ChannelEpoch> Epochs;
struct EpochEntry {
	EpochEntry() : initialized(false) {}
	Epochs epochs;
	Core::Time start;
	OPT(Core::Time) end;
	bool initialized;
};


inline bool leap(int y) {
	return (y % 400 == 0 || (y % 4 == 0 && y % 100 != 0));
}


inline int doy(int y, int m) {
	static const int DOY[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
	return (DOY[m] + (leap(y) && m >= 2));
}


inline string date2str(const Core::Time &t) {
	int year, month, day, hour;
	t.get(&year, &month, &day, &hour);

	if ( month < 1 || month > 12 || day < 1 || day > 31 ) {
		SEISCOMP_ERROR("invalid date: month=%d, day=%d", month, day);
		month = 1;
		day = 0;
	}

	char buf[16];
	snprintf(buf, 15, "%d.%03d.%02d", year, doy(year, month - 1) + day, hour);
	return buf;
}


string timeToStr(const Core::Time &time) {
	if ( time.microseconds() == 0 && time.seconds() % 86400 == 0 ) {
		return time.toString("%F");
	}

	return time.toString("%FT%T");
}


ostream &operator<<(ostream &os, const set<string> &s) {
	bool first = true;
	for ( set<string>::const_iterator it = s.begin(); it != s.end(); ++ it ) {
		if ( !first ) os << " ";
		os << *it;
		first = false;
	}
	return os;
}


ostream &operator<<(ostream &os, const Epochs &l) {
	set<string> s;
	for ( Epochs::const_iterator it = l.begin(); it != l.end(); ++ it )
		s.insert(it->first->code());
	return os << s;
}


bool respLowerThan(const StationXML::Response *r1, const StationXML::Response *r2) {
	return r1->stage() < r2->stage();
}


pair<int,int> double2frac(double d) {
	double df = 1;
	int top = 1;
	int bot = 1;

	while ( fabs(df-d) > 1e-05 ) {
		if ( df < d )
			++top;
		else {
			++bot;
			top = int(d * bot);
		}

		df = (double)top / (double)bot;
	}

	return pair<int,int>(top,bot);
}


bool contains(const Core::TimeWindow &tw, const Core::Time &time) {
	if ( !time.valid() ) return false;
	if ( tw.endTime().valid() )
		return tw.contains(time);
	return time >= tw.startTime();
}


// Special overlap check for time windows where end time might be open
bool overlaps(const Core::TimeWindow &tw1, const Core::TimeWindow &tw2) {
	if ( contains(tw1, tw2.startTime()) ) return true;
	if ( contains(tw1, tw2.endTime()) ) return true;
	if ( contains(tw2, tw1.startTime()) ) return true;
	if ( contains(tw2, tw1.endTime()) ) return true;
	return false;
}



template <typename T>
bool overlaps(const T *epoch1, const T *epoch2) {
	Core::TimeWindow tw1, tw2;
	tw1.setStartTime(epoch1->start());
	try { tw1.setEndTime(epoch1->end()); } catch ( ... ) {}
	tw2.setStartTime(epoch2->start());
	try { tw2.setEndTime(epoch2->end()); } catch ( ... ) {}

	return overlaps(tw1, tw2);
}



void checkFIR(DataModel::ResponseFIR *rf) {
	vector<double> &v = rf->coefficients().content();
	int nc = v.size();

	if ( rf->numberOfCoefficients() != nc ) {
		SEISCOMP_WARNING("expected %d coefficients, found %d: will be corrected",
						 rf->numberOfCoefficients(), nc);
		rf->setNumberOfCoefficients(nc);
	}

	if ( nc == 0 || rf->symmetry() != "A" ) return;

	int i = 0;
	for ( ; 2 * i < nc; ++i )
		if ( v[i] != v[nc - 1 - i] ) break;

	if ( 2 * i > nc ) {
		rf->setNumberOfCoefficients(i);
		rf->setSymmetry("B");
		v.resize(i);
		//SEISCOMP_DEBUG("A(%d) -> B(%d)", nc, i);
	}
	else if ( 2 * i == nc ) {
		rf->setNumberOfCoefficients(i);
		rf->setSymmetry("C");
		v.resize(i);
		//SEISCOMP_DEBUG("A(%d) -> C(%d)", nc, i);
	}
	else {
		//SEISCOMP_DEBUG("A(%d) -> A(%d)", nc, nc);
	}
}


void checkPAZ(DataModel::ResponsePAZ *rp) {
	if ( rp->numberOfPoles() != (int)rp->poles().content().size() ) {
		SEISCOMP_WARNING("expected %d poles, found %lu", rp->numberOfPoles(),
						 (unsigned long)rp->poles().content().size());
		rp->setNumberOfPoles(rp->poles().content().size());
	}

	if ( rp->numberOfZeros() != (int)rp->zeros().content().size() ) {
		SEISCOMP_WARNING("expected %d zeros, found %lu", rp->numberOfZeros(),
						 (unsigned long)rp->zeros().content().size());
		rp->setNumberOfZeros(rp->zeros().content().size());
	}
}


void checkPoly(DataModel::ResponsePolynomial *rp) {
	if ( rp->numberOfCoefficients() != (int)rp->coefficients().content().size() ) {
		SEISCOMP_WARNING("expected %d coefficients, found %lu", rp->numberOfCoefficients(),
						 (unsigned long)rp->coefficients().content().size());
		rp->setNumberOfCoefficients(rp->coefficients().content().size());
	}
}


// Macro to backup the result of an optional value
#define BCK(name, type, query) \
	OPT(type) name;\
	try { name = query; } catch ( ... ) {}

// Macro to update the need-update state using the backup'd value
// and the current value
#define UPD(target, bck, type, query) \
	if ( !target ) {\
		OPT(type) tmp; try { tmp = query; } catch ( ... ) {}\
		if ( tmp != bck ) target = true;\
	}

#define UPD2(target, bck, type, query) \
	if ( !target ) {\
		OPT(type) tmp; try { tmp = query; } catch ( ... ) {}\
		if ( tmp != bck ) { cout << #query" -> " << Core::toString(bck) << " != " << Core::toString(query) << endl; target = true;}\
	}


#define UPD_DBG(bck, type, query) \
	{ OPT(type) tmp; try { tmp = query; } catch ( ... ) {}\
	  if ( tmp != bck ) { SEISCOMP_DEBUG(#query" changed"); } }


#define COMPARE_AND_RETURN(type, inst1, inst2, query) \
	{\
		BCK(tmp1, type, inst1->query)\
		BCK(tmp2, type, inst2->query)\
		if ( tmp1 != tmp2 ) return false;\
	}


bool equal(const DataModel::ResponseFIR *f1, const DataModel::ResponseFIR *f2) {
	COMPARE_AND_RETURN(double, f1, f2, gain())
	COMPARE_AND_RETURN(int, f1, f2, decimationFactor())
	COMPARE_AND_RETURN(double, f1, f2, delay())
	COMPARE_AND_RETURN(double, f1, f2, correction())
	COMPARE_AND_RETURN(int, f1, f2, numberOfCoefficients())

	if ( f1->symmetry() != f2->symmetry() ) return false;

	const DataModel::RealArray *coeff1 = NULL;
	const DataModel::RealArray *coeff2 = NULL;

	try { coeff1 = &f1->coefficients(); } catch ( ... ) {}
	try { coeff2 = &f2->coefficients(); } catch ( ... ) {}

	// One set and not the other?
	if ( (!coeff1 && coeff2) || (coeff1 && !coeff2) ) return false;

	// Both unset?
	if ( !coeff1 && !coeff2 ) return true;

	// Both set, compare content
	const vector<double> &c1 = coeff1->content();
	const vector<double> &c2 = coeff2->content();
	if ( c1.size() != c2.size() ) return false;
	for ( size_t i = 0; i < c1.size(); ++i )
		if ( c1[i] != c2[i] ) return false;

	return true;
}



bool equal(const DataModel::ResponsePAZ *p1, const DataModel::ResponsePAZ *p2) {
	if ( p1->type() != p2->type() ) return false;

	COMPARE_AND_RETURN(double, p1, p2, gain())
	COMPARE_AND_RETURN(double, p1, p2, gainFrequency())
	COMPARE_AND_RETURN(double, p1, p2, normalizationFactor())
	COMPARE_AND_RETURN(double, p1, p2, normalizationFrequency())
	COMPARE_AND_RETURN(int, p1, p2, numberOfPoles())
	COMPARE_AND_RETURN(int, p1, p2, numberOfZeros())

	// Compare poles
	const DataModel::ComplexArray *poles1 = NULL;
	const DataModel::ComplexArray *poles2 = NULL;

	try { poles1 = &p1->poles(); } catch ( ... ) {}
	try { poles2 = &p2->poles(); } catch ( ... ) {}

	// One set and not the other?
	if ( (!poles1 && poles2) || (poles1 && !poles2) ) return false;

	// Both unset?
	if ( !poles1 && !poles2 ) return true;

	// Both set, compare content
	const vector< complex<double> > &pc1 = poles1->content();
	const vector< complex<double> > &pc2 = poles2->content();

	if ( pc1.size() != pc2.size() ) return false;
	for ( size_t i = 0; i < pc1.size(); ++i )
		if ( pc1[i] != pc2[i] ) return false;

	// Compare zeros
	const DataModel::ComplexArray *zeros1 = NULL;
	const DataModel::ComplexArray *zeros2 = NULL;

	try { zeros1 = &p1->zeros(); } catch ( ... ) {}
	try { zeros2 = &p2->zeros(); } catch ( ... ) {}

	// One set and not the other?
	if ( (!zeros1 && zeros2) || (zeros1 && !zeros2) ) return false;

	// Both unset?
	if ( !zeros1 && !zeros2 ) return true;

	// Both set, compare content
	const vector< complex<double> > &zc1 = zeros1->content();
	const vector< complex<double> > &zc2 = zeros2->content();

	if ( zc1.size() != zc2.size() ) return false;
	for ( size_t i = 0; i < zc1.size(); ++i )
		if ( zc1[i] != zc2[i] ) return false;

	// Everything is equal
	return true;
}


bool equal(const DataModel::ResponsePolynomial *p1, const DataModel::ResponsePolynomial *p2) {
	COMPARE_AND_RETURN(double, p1, p2, gain())
	COMPARE_AND_RETURN(double, p1, p2, gainFrequency())

	if ( p1->frequencyUnit() != p2->frequencyUnit() ) return false;
	if ( p1->approximationType() != p2->approximationType() ) return false;

	COMPARE_AND_RETURN(double, p1, p2, approximationLowerBound())
	COMPARE_AND_RETURN(double, p1, p2, approximationUpperBound())
	COMPARE_AND_RETURN(double, p1, p2, approximationError())
	COMPARE_AND_RETURN(int, p1, p2, numberOfCoefficients())

	const DataModel::RealArray *coeff1 = NULL;
	const DataModel::RealArray *coeff2 = NULL;

	try { coeff1 = &p1->coefficients(); } catch ( ... ) {}
	try { coeff2 = &p2->coefficients(); } catch ( ... ) {}

	// One set and not the other?
	if ( (!coeff1 && coeff2) || (coeff1 && !coeff2) ) return false;

	// Both unset?
	if ( !coeff1 && !coeff2 ) return true;

	// Both set, compare content
	const vector<double> &c1 = coeff1->content();
	const vector<double> &c2 = coeff2->content();
	if ( c1.size() != c2.size() ) return false;
	for ( size_t i = 0; i < c1.size(); ++i )
		if ( c1[i] != c2[i] ) return false;

	return true;
}


bool equal(const DataModel::Datalogger *d1, const DataModel::Datalogger *d2) {
	if ( d1->description() != d2->description() ) return false;
	if ( d1->digitizerModel() != d2->digitizerModel() ) return false;
	if ( d1->digitizerManufacturer() != d2->digitizerManufacturer() ) return false;
	if ( d1->recorderModel() != d2->recorderModel() ) return false;
	if ( d1->recorderManufacturer() != d2->recorderManufacturer() ) return false;
	if ( d1->clockModel() != d2->clockModel() ) return false;
	if ( d1->clockManufacturer() != d2->clockManufacturer() ) return false;
	if ( d1->clockType() != d2->clockType() ) return false;

	COMPARE_AND_RETURN(double, d1, d2, gain())
	COMPARE_AND_RETURN(double, d1, d2, maxClockDrift())

	return true;
}


bool equal(const DataModel::Sensor *s1, const DataModel::Sensor *s2) {
	if ( s1->description() != s2->description() ) return false;
	if ( s1->model() != s2->model() ) return false;
	if ( s1->manufacturer() != s2->manufacturer() ) return false;
	if ( s1->type() != s2->type() ) return false;
	if ( s1->unit() != s2->unit() ) return false;
	if ( s1->response() != s2->response() ) return false;

	COMPARE_AND_RETURN(double, s1, s2, lowFrequency())
	COMPARE_AND_RETURN(double, s1, s2, highFrequency())

	return true;
}


bool isMetaResponse(const StationXML::Response *resp) {
	return resp->polesAndZerosCount() == 0 &&
	       resp->coefficientsCount() == 0 &&
	       resp->responseListCount() == 0 &&
	       resp->genericCount() == 0 &&
	       resp->fIRCount() == 0 &&
	       resp->polynomialCount() == 0;
}


bool isFIRResponse(const StationXML::Response *resp) {
	if ( resp == NULL ) return false;
	return resp->fIRCount() > 0;
}


bool isCoeffResponse(const StationXML::Response *resp) {
	if ( resp == NULL ) return false;
	return resp->coefficientsCount() > 0;
}


bool isPAZResponse(const StationXML::Response *resp) {
	if ( resp == NULL ) return false;
	return resp->polesAndZerosCount() > 0;
}


bool isPolyResponse(const StationXML::Response *resp) {
	if ( resp == NULL ) return false;
	return resp->polynomialCount() > 0;
}


string getBaseUnit(const string &unitText) {
	size_t pos = unitText.find(' ');
	if ( pos == string::npos ) return unitText;
	return unitText.substr(0, pos);
}


const StationXML::Response *
findSensorResponse(const StationXML::ChannelEpoch *epoch) {
	const StationXML::Response *resp;

	for ( size_t i = 0; i < epoch->responseCount(); ++i ) {
		resp = epoch->response(i);
		// Only need poles&zeros
		if ( resp->polesAndZerosCount() == 0 ) continue;

		StationXML::PolesAndZeros *paz = resp->polesAndZeros(0);

		string inputUnits = getBaseUnit(paz->inputUnits());
		string outputUnits = getBaseUnit(paz->outputUnits());

		if ( inputUnits == VELOCITY && outputUnits == CURRENT )
			return resp;

		if ( inputUnits == ACCEL1 && outputUnits == CURRENT )
			return resp;

		if ( inputUnits == ACCEL2 && outputUnits == CURRENT )
			return resp;

		if ( inputUnits == DISPLACE && outputUnits == CURRENT )
			return resp;

		if ( inputUnits == VELOCITY && outputUnits == DIGITAL )
			return resp;

		if ( inputUnits == ACCEL1 && outputUnits == DIGITAL )
			return resp;

		if ( inputUnits == ACCEL2 && outputUnits == DIGITAL )
			return resp;

		if ( inputUnits == DISPLACE && outputUnits == DIGITAL )
			return resp;
	}

	for ( size_t i = 0; i < epoch->responseCount(); ++i ) {
		resp = epoch->response(i);
		// Only need poles&zeros
		if ( resp->polynomialCount() == 0 ) continue;

		StationXML::Polynomial *poly = resp->polynomial(0);

		string inputUnits = getBaseUnit(poly->inputUnits());
		string outputUnits = getBaseUnit(poly->outputUnits());

		if ( inputUnits == TEMPERATURE && outputUnits == CURRENT )
			return resp;

		if ( inputUnits == PRESSURE && outputUnits == CURRENT )
			return resp;

		if ( inputUnits == TEMPERATURE && outputUnits == DIGITAL )
			return resp;

		if ( inputUnits == PRESSURE && outputUnits == DIGITAL )
			return resp;
	}

	return NULL;
}


const char *sensorUnit(const StationXML::Response *resp,
                       const StationXML::PolesAndZeros *&paz,
                       const StationXML::Polynomial *&poly) {
	paz = NULL;
	poly = NULL;

	if ( isPAZResponse(resp) ) {
		paz = resp->polesAndZeros(0);

		string inputUnits = getBaseUnit(paz->inputUnits());
		string outputUnits = getBaseUnit(paz->outputUnits());

		if ( inputUnits == VELOCITY && outputUnits == CURRENT )
			return VELOCITY;

		if ( inputUnits == ACCEL1 && outputUnits == CURRENT )
			return ACCEL1;

		if ( inputUnits == ACCEL2 && outputUnits == CURRENT )
			return ACCEL1;

		if ( inputUnits == DISPLACE && outputUnits == CURRENT )
			return DISPLACE;

		if ( inputUnits == VELOCITY && outputUnits == DIGITAL )
			return VELOCITY;

		if ( inputUnits == ACCEL1 && outputUnits == DIGITAL )
			return ACCEL1;

		if ( inputUnits == ACCEL2 && outputUnits == DIGITAL )
			return ACCEL1;

		if ( inputUnits == DISPLACE && outputUnits == DIGITAL )
			return DISPLACE;
	}
	else if ( isPolyResponse(resp) ) {
		poly = resp->polynomial(0);

		string inputUnits = getBaseUnit(poly->inputUnits());
		string outputUnits = getBaseUnit(poly->outputUnits());

		if ( inputUnits == TEMPERATURE && outputUnits == CURRENT )
			return TEMPERATURE;

		if ( inputUnits == PRESSURE && outputUnits == CURRENT )
			return PRESSURE;

		if ( inputUnits == TEMPERATURE && outputUnits == DIGITAL )
			return TEMPERATURE;

		if ( inputUnits == PRESSURE && outputUnits == DIGITAL )
			return PRESSURE;
	}

	return UNDEFINED;
}


DataModel::Network *findNetwork(DataModel::Inventory *inv, const string &code,
                                const StationXML::StationEpoch *epoch) {
	for ( size_t i = 0; i < inv->networkCount(); ++i ) {
		DataModel::Network *net = inv->network(i);
		if ( net->code() != code ) continue;

		// Check for overlapping time windows
		if ( epoch->start() < net->start() ) continue;
		OPT(Core::Time) end1, end2;
		try { end1 = epoch->end(); } catch ( ... ) {}
		try { end2 = net->end(); } catch ( ... ) {}

		// Network time window open, ok
		if ( !end2 ) return net;
		// Epoch time window open, not ok
		if ( !end1 ) continue;

		// Epoch time window end greater than network end, not ok
		if ( *end1 > *end2 ) continue;

		return net;
	}

	return NULL;
}


bool findElement(const string &line, const string &sub) {
	size_t pos = line.find(' ');
	if ( pos == string::npos )
		return line == sub;

	size_t last = 0;
	while ( pos != string::npos ) {
		if ( line.compare(last, pos-last, sub) == 0 )
			return true;

		last = line.find_first_not_of(' ', pos);
		pos = line.find(' ', last);
	}

	return line.compare(last, pos, sub) == 0;
}


DataModel::ResponseFIRPtr convert(const StationXML::Response *resp,
                                  const StationXML::Coefficients *coeff) {
	if ( coeff->cfTransferFunctionType() != StationXML::CFTFT_DIGITAL ) {
		SEISCOMP_WARNING("only response coefficients with transfer function "
		                 "type \"DIGITAL\" supported");
		return NULL;
	}

	DataModel::ResponseFIRPtr rf = DataModel::ResponseFIR::Create();
	rf->setName(rf->publicID());

	rf->setGain(resp->stageSensitivity().sensitivityValue());
	try { rf->setDecimationFactor(resp->decimation().factor()); }
	catch ( ... ) {}
	try { rf->setDelay(resp->decimation().delay().value()*resp->decimation().inputSampleRate().value()); }
	catch ( ... ) {}
	try { rf->setCorrection(resp->decimation().correction().value()*resp->decimation().inputSampleRate().value()); }
	catch ( ... ) {}

	rf->setNumberOfCoefficients(coeff->numeratorCount());
	rf->setSymmetry("A");
	rf->setCoefficients(DataModel::RealArray());
	vector<double> &numerators = rf->coefficients().content();
	for ( size_t n = 0; n < coeff->numeratorCount(); ++n ) {
		StationXML::FloatType *num = coeff->numerator(n);
		numerators.push_back(num->value());
	}

	return rf;
}


DataModel::ResponseFIRPtr convert(const StationXML::Response *resp,
                                  const StationXML::FIR *fir) {
	DataModel::ResponseFIRPtr rf = DataModel::ResponseFIR::Create();
	rf->setName(rf->publicID());

	rf->setGain(resp->stageSensitivity().sensitivityValue());
	try { rf->setDecimationFactor(resp->decimation().factor()); }
	catch ( ... ) {}
	try { rf->setDelay(resp->decimation().delay().value()*resp->decimation().inputSampleRate().value()); }
	catch ( ... ) {}
	try { rf->setCorrection(resp->decimation().correction().value()*resp->decimation().inputSampleRate().value()); }
	catch ( ... ) {}

	rf->setNumberOfCoefficients(fir->numeratorCoefficientCount());

	try {
		switch ( fir->symmetry() ) {
			case StationXML::ST_ODD:
				// According to SEED manual chapter 5/50p, blockette 41
				rf->setSymmetry("B");
				break;
			case StationXML::ST_EVEN:
				rf->setSymmetry("C");
				break;
			default:
				rf->setSymmetry("A");
				break;
		}
	}
	catch ( ... ) {}

	rf->setCoefficients(DataModel::RealArray());
	vector<double> &numerators = rf->coefficients().content();

	// Sort coefficients according to its i attribute
	vector< pair<int,int> > sortedIdx;
	for ( size_t n = 0; n < fir->numeratorCoefficientCount(); ++n ) {
		StationXML::NumeratorCoefficient *num = fir->numeratorCoefficient(n);
		sortedIdx.push_back(pair<int,int>(num->i(), n));
	}
	sort(sortedIdx.begin(), sortedIdx.end());

	for ( size_t n = 0; n < fir->numeratorCoefficientCount(); ++n ) {
		StationXML::NumeratorCoefficient *num = fir->numeratorCoefficient(sortedIdx[n].second);
		numerators.push_back(num->value());
	}

	return rf;
}



DataModel::ResponsePAZPtr convert(const StationXML::Response *resp,
                                  const StationXML::PolesAndZeros *paz) {
	DataModel::ResponsePAZPtr rp = DataModel::ResponsePAZ::Create();
	rp->setName(rp->publicID());

	switch ( paz->pzTransferFunctionType() ) {
		case StationXML::PZTFT_LAPLACE_RAD:
			rp->setType("A");
			break;
		case StationXML::PZTFT_LAPLACE_HZ:
			rp->setType("B");
			break;
		case StationXML::PZTFT_DIGITAL_Z_TRANSFORM:
			rp->setType("D");
			break;
		default:
			break;
	}

	rp->setGain(resp->stageSensitivity().sensitivityValue());
	try { rp->setGainFrequency(resp->stageSensitivity().frequency()); }
	catch ( ... ) {}

	rp->setNormalizationFactor(paz->normalizationFactor());
	rp->setNormalizationFrequency(paz->normalizationFreq().value());
	rp->setNumberOfZeros(paz->zeroCount());
	rp->setNumberOfPoles(paz->poleCount());

	rp->setZeros(DataModel::ComplexArray());
	vector< complex<double> > &zeros = rp->zeros().content();

	// Sort zeros according to its number attribute
	vector< pair<int,int> > sortedIdx;
	for ( size_t z = 0; z < paz->zeroCount(); ++z ) {
		StationXML::PoleAndZero *val = paz->zero(z);
		sortedIdx.push_back(pair<int,int>(val->number(), z));
	}
	sort(sortedIdx.begin(), sortedIdx.end());

	for ( size_t z = 0; z < sortedIdx.size(); ++z ) {
		StationXML::PoleAndZero *val = paz->zero(sortedIdx[z].second);
		zeros.push_back(complex<double>(val->real().value(),val->imaginary().value()));
	}

	rp->setPoles(DataModel::ComplexArray());
	vector< complex<double> > &poles = rp->poles().content();

	// Sort poles according to its number attribute
	sortedIdx.clear();
	for ( size_t p = 0; p < paz->poleCount(); ++p ) {
		StationXML::PoleAndZero *val = paz->pole(p);
		sortedIdx.push_back(pair<int,int>(val->number(), p));
	}
	sort(sortedIdx.begin(), sortedIdx.end());

	for ( size_t p = 0; p < sortedIdx.size(); ++p ) {
		StationXML::PoleAndZero *val = paz->pole(sortedIdx[p].second);
		poles.push_back(complex<double>(val->real().value(),val->imaginary().value()));
	}

	return rp;
}


DataModel::ResponsePolynomialPtr convert(const StationXML::Response *resp,
                                         const StationXML::Polynomial *poly) {
	DataModel::ResponsePolynomialPtr rp = DataModel::ResponsePolynomial::Create();
	rp->setName(rp->publicID());

	// NOTE: The type is currently not supported by StationXML
	//rp->setType(paz->???)

	rp->setGain(resp->stageSensitivity().sensitivityValue());
	try { rp->setGainFrequency(resp->stageSensitivity().frequency()); }
	catch ( ... ) {}

	// Type is not given in StationXML. Settings type to A (rad/sec)
	rp->setFrequencyUnit("A");
	const char *atstr = poly->approximationType().toString();
	if ( atstr != NULL )
		rp->setApproximationType(atstr);
	rp->setApproximationLowerBound(poly->freqLowerBound().value());
	rp->setApproximationUpperBound(poly->freqUpperBound().value());
	rp->setApproximationError(poly->maxError().value());
	rp->setNumberOfCoefficients(poly->coefficientCount());

	rp->setCoefficients(DataModel::RealArray());
	vector<double> &coeff = rp->coefficients().content();

	// Sort zeros according to its number attribute
	vector< pair<int,int> > sortedIdx;
	for ( size_t c = 0; c < poly->coefficientCount(); ++c ) {
		StationXML::PolynomialCoefficient *val = poly->coefficient(c);
		sortedIdx.push_back(pair<int,int>(val->number(), c));
	}
	sort(sortedIdx.begin(), sortedIdx.end());

	for ( size_t c = 0; c < sortedIdx.size(); ++c ) {
		StationXML::PolynomialCoefficient *val = poly->coefficient(sortedIdx[c].second);
		coeff.push_back(val->value());
	}

	return rp;
}


DataModel::Network *
createNetwork(const string &code)
{
	string id = "NET/" + code + "/" +
	            Core::Time::GMT().toString("%Y%m%d%H%M%S.%f") + "." +
	            Core::toString(Core::BaseObject::ObjectCount());
	return DataModel::Network::Create(id);
}


DataModel::Station *
createStation(const string &net, const string &code)
{
	string id = "STA/" + net + "/" + code + "/" +
	            Core::Time::GMT().toString("%Y%m%d%H%M%S.%f") + "." +
	            Core::toString(Core::BaseObject::ObjectCount());
	return DataModel::Station::Create(id);
}


DataModel::SensorLocation *
createSensorLocation(const string &net, const string &sta, const string &code)
{
	string id = "LOC/" + net + "/" + sta + "/" + code + "/" +
	            Core::Time::GMT().toString("%Y%m%d%H%M%S.%f") + "." +
	            Core::toString(Core::BaseObject::ObjectCount());
	return DataModel::SensorLocation::Create(id);
}


}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Convert2SC3::Convert2SC3(DataModel::Inventory *inv) : _inv(inv) {
	if ( !_inv ) return;

	for ( size_t i = 0; i < _inv->dataloggerCount(); ++i ) {
		DataModel::Datalogger *d = _inv->datalogger(i);
		_dataloggerLookup[d->name()] = d;
	}

	for ( size_t i = 0; i < _inv->sensorCount(); ++i ) {
		DataModel::Sensor *s = _inv->sensor(i);
		_sensorLookup[s->name()] = s;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const Convert2SC3::TupleSet &Convert2SC3::visitedStations() const {
	return _visitedStations;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const Convert2SC3::NetworkSet &Convert2SC3::touchedNetworks() const {
	return _touchedNetworks;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const Convert2SC3::StationSet &Convert2SC3::touchedStations() const {
	return _touchedStations;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Convert2SC3::push(const StationXML::StaMessage *msg) {
	if ( _inv == NULL ) return false;

	// Process networks. All networks are then under our control and
	// unprocessed epochs will be removed later
	for ( size_t n = 0; n < msg->networkCount(); ++n ) {
		if ( _interrupted ) return false;
		StationXML::Network *net = msg->network(n);

		string netCode = net->code();
		Core::trim(netCode);

		if ( netCode.empty() ) {
			SEISCOMP_WARNING("network[%d]: code empty: ignoring", (int)n);
			continue;
		}

		Core::Time start;
		try {
			start = net->start();
		}
		catch ( ... ) {
			start = Core::Time(1902,1,1);
		}

		// Mark network as seen
		_visitedStations.insert(StringTuple(netCode, ""));

		SEISCOMP_INFO("Processing network %s (%s)",
		              netCode.c_str(), start.toString("%F %T").c_str());

		bool newInstance = false;
		bool needUpdate = false;

		DataModel::NetworkPtr sc_net;
		sc_net = _inv->network(DataModel::NetworkIndex(netCode, start));

		if ( !sc_net ) {
			sc_net = createNetwork(netCode);
			sc_net->setCode(netCode);
			sc_net->setStart(start);
			newInstance = true;
		}

		BCK(oldRestricted, bool, sc_net->restricted());
		BCK(oldShared, bool, sc_net->shared());
		BCK(oldEnd, Core::Time, sc_net->end());
		string oldDescription = sc_net->description();

		sc_net->setRestricted(false);
		sc_net->setShared(true);

		try { sc_net->setEnd(net->end()); }
		catch ( ... ) { sc_net->setEnd(Core::None); }

		sc_net->setDescription(net->description());

		UPD(needUpdate, oldRestricted, bool, sc_net->restricted());
		UPD(needUpdate, oldShared, bool, sc_net->shared());
		UPD(needUpdate, oldEnd, Core::Time, sc_net->end());
		if ( oldDescription != sc_net->description() ) needUpdate = true;

		if ( newInstance ) {
			SEISCOMP_DEBUG("Added new network epoch: %s (%s)",
			               sc_net->code().c_str(), sc_net->start().iso().c_str());
			_inv->add(sc_net.get());
		}
		else if ( needUpdate ) {
			SEISCOMP_DEBUG("Updated network epoch: %s (%s)",
			               sc_net->code().c_str(), sc_net->start().iso().c_str());
			sc_net->update();
		}

		_touchedNetworks.insert(NetworkIndex(sc_net->code(), sc_net->start()));

		for ( size_t s = 0; s < net->stationCount(); ++s ) {
			if ( _interrupted ) break;
			StationXML::Station *sta = net->station(s);
			string staCode = sta->code();
			Core::trim(staCode);

			if ( staCode.empty() ) {
				SEISCOMP_WARNING("network[%d]/station[%d]: empty code: ignoring",
				                 (int)n, (int)s);
				continue;
			}

			// Mark station as seen
			_visitedStations.insert(StringTuple(netCode, staCode));

			for ( size_t se = 0; se < sta->epochCount(); ++se ) {
				if ( _interrupted ) break;
				StationXML::StationEpoch *staepoch = sta->epoch(se);
				process(sc_net.get(), sta, staepoch);
			}
		}
	}

	// Process stations in the message without networks. All networks
	// are left in the database and only new networks are created if
	// not available.
	for ( size_t s = 0; s < msg->stationCount(); ++s ) {
		if ( _interrupted ) break;
		StationXML::Station *sta = msg->station(s);
		string netCode = sta->netCode();
		string staCode = sta->code();
		Core::trim(netCode);
		Core::trim(staCode);

		if ( netCode.empty() ) {
			SEISCOMP_WARNING("station[%d]: empty net-code: ignoring", (int)s);
			continue;
		}

		if ( staCode.empty() ) {
			SEISCOMP_WARNING("station[%d]: empty code: ignoring", (int)s);
			continue;
		}

		// Mark station as seen
		_visitedStations.insert(StringTuple(netCode, staCode));

		for ( size_t se = 0; se < sta->epochCount(); ++se ) {
			if ( _interrupted ) break;
			StationXML::StationEpoch *staepoch = sta->epoch(se);
			process(NULL, sta, staepoch);
		}
	}

	return !_interrupted;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Convert2SC3::cleanUp() {
	SEISCOMP_INFO("Clean up inventory");

	for ( size_t n = 0; n < _inv->networkCount(); ) {
		DataModel::Network *net = _inv->network(n);
		if ( _visitedStations.find(StringTuple(net->code(), "")) == _visitedStations.end() ) {
			SEISCOMP_DEBUG("Leaving unknown network %s untouched", net->code().c_str());
			++n;
			continue;
		}

		NetworkIndex net_idx(net->code(), net->start());
		if ( _touchedNetworks.find(net_idx) == _touchedNetworks.end() ) {
			SEISCOMP_INFO("Removing epoch %s (%s)", net->code().c_str(), net->start().iso().c_str());
			// TODO: Remove epoch
			_inv->removeNetwork(n);
			continue;
		}

		++n;

		for ( size_t s = 0; s < net->stationCount(); ) {
			DataModel::Station *sta = net->station(s);
			if ( _visitedStations.find(StringTuple(net->code(), sta->code())) == _visitedStations.end() ) {
				SEISCOMP_DEBUG("Leaving unknown station %s.%s untouched",
				               net->code().c_str(), sta->code().c_str());
				++s;
				continue;
			}

			StationIndex sta_idx(net_idx, EpochIndex(sta->code(), sta->start()));
			if ( _touchedStations.find(sta_idx) == _touchedStations.end() ) {
				SEISCOMP_INFO("Removing epoch %s.%s (%s)",
				              net->code().c_str(), sta->code().c_str(),
				              sta->start().iso().c_str());
				// TODO: Remove epoch
				net->removeStation(s);
				continue;
			}

			++s;

			for ( size_t l = 0; l < sta->sensorLocationCount(); ) {
				DataModel::SensorLocation *loc = sta->sensorLocation(l);
				LocationIndex loc_idx(sta_idx, EpochIndex(loc->code(), loc->start()));
				if ( _touchedSensorLocations.find(loc_idx) == _touchedSensorLocations.end() ) {
					SEISCOMP_INFO("Removing epoch %s.%s.%s (%s)",
					              net->code().c_str(), sta->code().c_str(),
					              loc->code().c_str(), loc->start().iso().c_str());
					sta->removeSensorLocation(l);
					continue;
				}

				++l;

				for ( size_t c = 0; c < loc->streamCount(); ) {
					DataModel::Stream *stream = loc->stream(c);
					StreamIndex cha_idx(loc_idx, EpochIndex(stream->code(), stream->start()));
					if ( _touchedStreams.find(cha_idx) == _touchedStreams.end() ) {
						SEISCOMP_INFO("Removing epoch %s.%s.%s.%s (%s)",
						              net->code().c_str(), sta->code().c_str(),
						              loc->code().c_str(), stream->code().c_str(),
						              stream->start().iso().c_str());
						loc->removeStream(c);
					}
					else
						++c;
				}

				for ( size_t c = 0; c < loc->auxStreamCount(); ++c ) {
					DataModel::AuxStream *aux = loc->auxStream(c);
					StreamIndex cha_idx(loc_idx, EpochIndex(aux->code(), aux->start()));
					if ( _touchedAuxStreams.find(cha_idx) == _touchedAuxStreams.end() ) {
						SEISCOMP_INFO("Removing epoch %s.%s.%s.%s (%s)",
						              net->code().c_str(), sta->code().c_str(),
						              loc->code().c_str(), aux->code().c_str(),
						              aux->start().iso().c_str());
						loc->removeAuxStream(c);
					}
					else
						++c;
				}
			}
		}
	}

	/*
	int unusedResponses = 0;

	// Collect all publicID's of used responses so far
	StringSet usedResponses;
	for ( size_t s = 0; s < _inv->sensorCount(); ++s ) {
		DataModel::Sensor *sensor = _inv->sensor(s);
		if ( sensor->response().empty() ) continue;
		usedResponses.insert(sensor->response());
	}

	for ( size_t d = 0; d < _inv->dataloggerCount(); ++d ) {
		if ( _interrupted ) return;

		DataModel::Datalogger *dl = _inv->datalogger(d);
		for ( size_t i = 0; i < dl->decimationCount(); ++i ) {
			DataModel::Decimation *deci = dl->decimation(i);
			try {
				const string &c = deci->analogueFilterChain().content();
				if ( !c.empty() ) {
					vector<string> ids;
					Core::fromString(ids, c);
					usedResponses.insert(ids.begin(), ids.end());
				}
			}
			catch ( ... ) {}

			try {
				const string &c = deci->digitalFilterChain().content();
				if ( !c.empty() ) {
					vector<string> ids;
					Core::fromString(ids, c);
					usedResponses.insert(ids.begin(), ids.end());
				}
			}
			catch ( ... ) {}
		}
	}

	if ( _interrupted ) return;

	// Go through all available responses and remove unused ones
	for ( size_t r = 0; r < _inv->responseFIRCount(); ) {
		DataModel::ResponseFIR *resp = _inv->responseFIR(r);
		// Response not used -> remove it
		if ( usedResponses.find(resp->publicID()) == usedResponses.end() ) {
			_inv->removeResponseFIR(r);
			++unusedResponses;
		}
		else
			++r;
	}

	for ( size_t r = 0; r < _inv->responsePAZCount(); ) {
		DataModel::ResponsePAZ *resp = _inv->responsePAZ(r);
		// Response not used -> remove it
		if ( usedResponses.find(resp->publicID()) == usedResponses.end() ) {
			_inv->removeResponsePAZ(r);
			++unusedResponses;
		}
		else
			++r;
	}

	for ( size_t r = 0; r < _inv->responsePolynomialCount(); ) {
		DataModel::ResponsePolynomial *resp = _inv->responsePolynomial(r);
		// Response not used -> remove it
		if ( usedResponses.find(resp->publicID()) == usedResponses.end() ) {
			_inv->removeResponsePolynomial(r);
			++unusedResponses;
		}
		else
			++r;
	}

	if ( unusedResponses )
		SEISCOMP_INFO("Removed %d unused responses", unusedResponses);
	*/
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
//
// Process a station epoch. If sc_net is NULL then the network has not
// yet been created because this epoch comes from a station without a
// network tag in the message.
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Convert2SC3::process(DataModel::Network *sc_net,
                          const StationXML::Station *sta,
                          const StationXML::StationEpoch *staepoch) {
	typedef map<LocationElevation, EpochEntry> EpochLocationMap;
	typedef map<string, EpochLocationMap> EpochCodeMap;

	Core::Time start = staepoch->start();
	string staCode = sta->code();
	Core::trim(staCode);

	if ( sc_net == NULL ) {
		SEISCOMP_INFO("Processing station %s/%s (%s)",
		              sta->netCode().c_str(), staCode.c_str(),
		              start.toString("%F %T").c_str());

		string netCode = sta->netCode();
		Core::trim(netCode);

		// Find a proper network or create one if needed
		sc_net = findNetwork(_inv, netCode, staepoch);

		// No network found
		if ( sc_net == NULL ) {
			SEISCOMP_WARNING("No network found for station %s.%s, skipping",
			                 netCode.c_str(), staCode.c_str());
			return false;
		}
	}
	else
		SEISCOMP_INFO("Processing station %s/%s (%s)",
		              sc_net->code().c_str(), staCode.c_str(),
		              start.toString("%F %T").c_str());

	bool newInstance = false;
	bool needUpdate = false;

	DataModel::StationPtr sc_sta;
	sc_sta = sc_net->station(DataModel::StationIndex(staCode, start));
	if ( !sc_sta ) {
		sc_sta = createStation(sc_net->code(), staCode);
		sc_sta->setCode(staCode);
		sc_sta->setStart(start);
		newInstance = true;
	}

	BCK(oldLat, double, sc_sta->latitude());
	BCK(oldLon, double, sc_sta->longitude());
	BCK(oldElev, double, sc_sta->elevation());
	BCK(oldEnd, Core::Time, sc_sta->end());

	try {
		if ( staepoch->end().valid() )
			sc_sta->setEnd(staepoch->end());
		else
			sc_sta->setEnd(Core::None);
	}
	catch ( ... ) { sc_sta->setEnd(Core::None); }

	string oldDesc = sc_sta->description();
	string oldCountry = sc_sta->country();
	BCK(oldRestricted, bool, sc_sta->restricted());
	BCK(oldShared, bool, sc_sta->shared());
	string oldPlace = sc_sta->place();
	string oldAffiliation = sc_sta->affiliation();
	string oldType = sc_sta->type();

	sc_sta->setLatitude(staepoch->latitude().value());
	sc_sta->setLongitude(staepoch->longitude().value());
	sc_sta->setElevation(staepoch->elevation().value());
	sc_sta->setDescription(staepoch->site().description());
	sc_sta->setCountry(staepoch->site().country());
	sc_sta->setAffiliation(sta->agencyCode());
	sc_sta->setType("");

	string place;
	if ( !staepoch->site().region().empty() )
		place += staepoch->site().region();
	if ( !staepoch->site().county().empty() ) {
		if ( !place.empty() ) place += " / ";
		place += staepoch->site().county();
	}
	if ( !staepoch->site().town().empty() ) {
		if ( !place.empty() ) place += " / ";
		place += staepoch->site().town();
	}

	sc_sta->setPlace(place);
	sc_sta->setRestricted(false);
	sc_sta->setShared(true);

	UPD(needUpdate, oldLat, double, sc_sta->latitude());
	UPD(needUpdate, oldLon, double, sc_sta->longitude());
	UPD(needUpdate, oldElev, double, sc_sta->elevation());
	UPD(needUpdate, oldEnd, Core::Time, sc_sta->end());
	if ( oldDesc != sc_sta->description() ) needUpdate = true;
	if ( oldCountry != sc_sta->country() ) needUpdate = true;
	UPD(needUpdate, oldRestricted, bool, sc_sta->restricted());
	UPD(needUpdate, oldShared, bool, sc_sta->shared());
	if ( oldPlace != sc_sta->place() ) needUpdate = true;
	if ( oldAffiliation != sc_sta->affiliation() ) needUpdate = true;
	if ( oldType != sc_sta->type() ) needUpdate = true;

	if ( newInstance ) {
		sc_net->add(sc_sta.get());
		SEISCOMP_DEBUG("Added new station epoch: %s (%s)",
		               sc_sta->code().c_str(), sc_sta->start().iso().c_str());
	}
	else if ( needUpdate ) {
		sc_sta->update();
		SEISCOMP_DEBUG("Updated station epoch: %s (%s)",
		               sc_sta->code().c_str(), sc_sta->start().iso().c_str());
	}

	// Register station
	_touchedStations.insert(
		StationIndex(
			NetworkIndex(sc_net->code(), sc_net->start()),
			EpochIndex(sc_sta->code(), sc_sta->start())
		)
	);

	EpochCodeMap epochMap;

	for ( size_t c = 0; c < staepoch->channelCount(); ++c ) {
		StationXML::Channel *cha = staepoch->channel(c);
		string locationCode = cha->locationCode();
		Core::trim(locationCode);

		for ( size_t e = 0; e < cha->epochCount(); ++e ) {
			StationXML::ChannelEpoch *epoch = cha->epoch(e);

			// Create index based on code and location to form
			// SensorLocation objects later
			LocationElevation loc_loc(Location(epoch->latitude().value(),
			                                   epoch->longitude().value()),
			                                   epoch->elevation().value());

			EpochEntry &entry = epochMap[locationCode][loc_loc];
			entry.epochs.push_back(ChannelEpoch(cha,epoch));

			Core::Time start = epoch->start();
			OPT(Core::Time) end;
			try { end = epoch->end(); } catch ( ... ) {}

			if ( !entry.initialized ) {
				entry.start = start;
				entry.end = end;
				entry.initialized = true;
			}
			else {
				if ( entry.start > start )
					entry.start = start;

				if ( !end ) entry.end = end;
				else if ( entry.end && *end > *entry.end )
					entry.end = end;
			}
		}
	}

	// After collecting all channel epoch, check for overlapping
	// time windows of different locations. If they overlap
	// valid SensorLocation definitions cannot be formed.
	EpochCodeMap::iterator it;

	// Iterate over all location codes
	for ( it = epochMap.begin(); it != epochMap.end(); ++it ) {
		// Iterate over all locations (lat/lon/elev) of this sensor
		EpochLocationMap::iterator lit, lit2;

		Epochs::iterator eit, eit2;

		set<StationXML::ChannelEpoch*> overlappingEpochs;

		// Check for location/channel epoch overlaps for each lat/lon/elev
		// group
		for ( lit = it->second.begin(); lit != it->second.end(); ++lit ) {
			EpochEntry &entry = lit->second;

			// Iterate over all channel epochs of a group
			for ( eit = entry.epochs.begin(); eit != entry.epochs.end(); ++eit ) {
				StationXML::ChannelEpoch *chaepoch = eit->second;

				// Already checked?
				if ( overlappingEpochs.find(chaepoch) != overlappingEpochs.end() )
					continue;

				// Go through all other groups
				for ( lit2 = it->second.begin(); lit2 != it->second.end(); ++lit2 ) {
					EpochEntry &entry2 = lit2->second;

					if ( lit == lit2 ) continue;

					// Iterate over all channel epochs of a group
					for ( eit2 = entry2.epochs.begin(); eit2 != entry2.epochs.end(); ++eit2 ) {
						StationXML::ChannelEpoch *chaepoch2 = eit2->second;

						if ( overlaps(chaepoch, chaepoch2) ) {
							overlappingEpochs.insert(chaepoch);
							overlappingEpochs.insert(chaepoch2);
						}
					}
				}
			}
		}

		if ( !overlappingEpochs.empty() ) {
			cerr << "C  " << sta->netCode() << "." << sta->code()
			     << " location '" << it->first << "' has overlapping epochs: skipping" << endl;

			for ( lit = it->second.begin(); lit != it->second.end(); ++lit ) {
				EpochEntry &entry = lit->second;
				bool firstHit = true;
				for ( eit = entry.epochs.begin(); eit != entry.epochs.end(); ++eit ) {
					StationXML::Channel *cha = eit->first;
					StationXML::ChannelEpoch *chaepoch = eit->second;
					if ( overlappingEpochs.find(chaepoch) == overlappingEpochs.end() )
						continue;

					if ( firstHit ) {
						cerr << "     " << Core::toString(lit->first.first.first) << "° "
						                << Core::toString(lit->first.first.second) << "° "
						                << Core::toString(lit->first.second) << "m" << endl;
						firstHit = false;
					}

					cerr << "       " << cha->code() << "  " << timeToStr(chaepoch->start());
					try {
						Core::Time end = chaepoch->end();
						cerr << " ~ " << timeToStr(end);
					}
					catch ( ... ) {}
					cerr << endl;
				}
			}

			// Skip this location code
			continue;
		}

		for ( lit = it->second.begin(); lit != it->second.end(); ++lit ) {
			EpochEntry &entry = lit->second;

			entry.epochs.sort(epochLowerThan);

			DataModel::SensorLocationPtr sc_loc;

			// Iterate sorted epochs
			Core::Time sensorLocationStart;
			OPT(Core::Time) sensorLocationEnd;
			bool newTw = true;
			for ( eit = entry.epochs.begin(); eit != entry.epochs.end(); ++eit) {
				StationXML::Channel *cha = eit->first;
				StationXML::ChannelEpoch *chaepoch = eit->second;

				// A new time window should be started
				if ( newTw ) {
					sensorLocationStart = chaepoch->start();
					try { sensorLocationEnd = chaepoch->end(); }
					catch ( ... ) { sensorLocationEnd = Core::None; }
					newTw = false;
					sc_loc = NULL;
				}
				// Extend the existing time window or create a new one
				// if the start time of the epoch overlaps with the
				// current tw
				else if ( sensorLocationEnd ) {
					if ( chaepoch->start() >= *sensorLocationEnd ) {
						sensorLocationStart = chaepoch->start();
						try { sensorLocationEnd = chaepoch->end(); }
						catch ( ... ) { sensorLocationEnd = Core::None; }
						sc_loc = NULL;
					}
					else {
						// Extend end time of sensor location if necessary
						try {
							if ( chaepoch->end() > *sensorLocationEnd ) {
								sensorLocationEnd = chaepoch->end();
								sc_loc->setEnd(*sensorLocationEnd);
							}
						}
						catch ( ... ) {}
					}
				}

				if ( !sc_loc ) {
					newInstance = false;
					needUpdate = false;

					sc_loc = sc_sta->sensorLocation(DataModel::SensorLocationIndex(it->first, sensorLocationStart));
					if ( !sc_loc ) {
						sc_loc = createSensorLocation(sc_net->code(), sc_sta->code(), it->first);
						sc_loc->setCode(it->first);
						sc_loc->setStart(sensorLocationStart);
						newInstance = true;
					}

					BCK(oldLat, double, sc_loc->latitude());
					BCK(oldLon, double, sc_loc->longitude());
					BCK(oldElev, double, sc_loc->elevation());
					BCK(oldEnd, Core::Time, sc_loc->end());

					sc_loc->setLatitude(lit->first.first.first);
					sc_loc->setLongitude(lit->first.first.second);
					sc_loc->setElevation(lit->first.second);
					sc_loc->setEnd(sensorLocationEnd);

					UPD(needUpdate, oldLat, double, sc_loc->latitude());
					UPD(needUpdate, oldLon, double, sc_loc->longitude());
					UPD(needUpdate, oldElev, double, sc_loc->elevation());
					UPD(needUpdate, oldEnd, Core::Time, sc_loc->end());

					if ( newInstance ) {
						sc_sta->add(sc_loc.get());
						SEISCOMP_DEBUG("Added new sensor location epoch: %s (%s)",
						               sc_loc->code().c_str(), sc_loc->start().iso().c_str());
					}
					else if ( needUpdate ) {
						sc_loc->update();
						SEISCOMP_DEBUG("Updated sensor location epoch: %s (%s)",
						               sc_loc->code().c_str(), sc_loc->start().iso().c_str());
					}

					// Register sensor location
					_touchedSensorLocations.insert(
						LocationIndex(
							StationIndex(
								NetworkIndex(sc_net->code(), sc_net->start()),
								EpochIndex(sc_sta->code(), sc_sta->start())
							),
							EpochIndex(sc_loc->code(), sc_loc->start())
						)
					);
				}

				process(sc_loc.get(), cha, chaepoch, sc_sta->restricted());
			}
		}
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Convert2SC3::process(DataModel::SensorLocation *sc_loc,
                          const StationXML::Channel *cha,
                          const StationXML::ChannelEpoch *epoch,
                          bool isRestricted) {
	bool newInstance = false;
	bool needUpdate = false;
	bool auxStream = false;

	const StationXML::Response *resp = findSensorResponse(epoch);
	if ( resp == NULL ) {
		// try to find instrument sensitivity value and units
		try {
			epoch->instrumentSensitivity().sensitivityValue();
			if ( epoch->instrumentSensitivity().sensitivityUnits().empty() )
				auxStream = true;
		}
		catch ( ... ) {
			auxStream = true;
		}
	}

	Core::Time start = epoch->start();
	string chaCode = cha->code();
	Core::trim(chaCode);

	if ( auxStream ) {
		DataModel::AuxStreamPtr sc_aux;
		sc_aux = sc_loc->auxStream(DataModel::AuxStreamIndex(chaCode, start));
		if ( !sc_aux ) {
			sc_aux = new DataModel::AuxStream;
			sc_aux->setCode(chaCode);
			sc_aux->setStart(start);
			newInstance = true;
		}

		BCK(oldEnd, Core::Time, sc_aux->end());
		string oldFlags = sc_aux->flags();
		string oldFormat = sc_aux->flags();
		BCK(oldRestricted, bool, sc_aux->restricted());
		string oldDevice = sc_aux->device();
		string oldDeviceSN = sc_aux->deviceSerialNumber();
		string oldSource = sc_aux->source();

		try {
			if ( epoch->end().valid() )
				sc_aux->setEnd(epoch->end());
			else
				sc_aux->setEnd(Core::None);
		}
		catch ( ... ) { sc_aux->setEnd(Core::None); }

		sc_aux->setFlags("");
		sc_aux->setFormat(epoch->storageFormat());
		sc_aux->setRestricted(isRestricted);
		try { sc_aux->setDevice(epoch->sensor().equipType()); }
		catch ( ... ) { sc_aux->setDevice(""); }
		try { sc_aux->setDeviceSerialNumber(epoch->sensor().serialNumber()); }
		catch ( ... ) { sc_aux->setDeviceSerialNumber(""); }
		sc_aux->setSource("");

		UPD(needUpdate, oldEnd, Core::Time, sc_aux->end());
		if ( oldFlags != sc_aux->flags() ) needUpdate = true;
		if ( oldFormat != sc_aux->flags() ) needUpdate = true;
		UPD(needUpdate, oldRestricted, bool, sc_aux->restricted());
		if ( oldDevice != sc_aux->device() ) needUpdate = true;
		if ( oldDeviceSN != sc_aux->deviceSerialNumber() ) needUpdate = true;
		if ( oldSource != sc_aux->source() ) needUpdate = true;

		if ( newInstance ) {
			sc_loc->add(sc_aux.get());
			SEISCOMP_DEBUG("Added new aux stream epoch: %s (%s)",
			               sc_aux->code().c_str(), sc_aux->start().iso().c_str());
		}
		else if ( needUpdate ) {
			sc_aux->update();
			SEISCOMP_DEBUG("Updated aux stream epoch: %s (%s)",
			               sc_aux->code().c_str(), sc_aux->start().iso().c_str());
		}

		// Register aux stream
		_touchedAuxStreams.insert(
			StreamIndex(
				LocationIndex(
					StationIndex(
						NetworkIndex(sc_loc->station()->network()->code(), sc_loc->station()->network()->start()),
						EpochIndex(sc_loc->station()->code(), sc_loc->station()->start())
					),
					EpochIndex(sc_loc->code(), sc_loc->start())
				),
				EpochIndex(sc_aux->code(), sc_aux->start())
			)
		);

		return true;
	}

	DataModel::StreamPtr sc_stream;
	sc_stream = sc_loc->stream(DataModel::StreamIndex(chaCode, start));
	if ( !sc_stream ) {
		sc_stream = new DataModel::Stream;
		sc_stream->setCode(chaCode);
		sc_stream->setStart(start);
		newInstance = true;
	}

	BCK(oldEnd, Core::Time, sc_stream->end());
	BCK(oldDep, double, sc_stream->depth());
	string oldFormat = sc_stream->format();
	BCK(oldRestricted, bool, sc_stream->restricted());
	BCK(oldsrNum, int, sc_stream->sampleRateNumerator());
	BCK(oldsrDen, int, sc_stream->sampleRateDenominator());
	BCK(oldAzi, double, sc_stream->azimuth());
	BCK(oldDip, double, sc_stream->dip());
	BCK(oldDLcha, int, sc_stream->dataloggerChannel());
	BCK(oldSNcha, int, sc_stream->sensorChannel());
	BCK(oldGain, double, sc_stream->gain());
	BCK(oldGainFreq, double, sc_stream->gainFrequency());
	string oldGainUnit = sc_stream->gainUnit();
	string oldDL = sc_stream->datalogger();
	string oldDLSN = sc_stream->dataloggerSerialNumber();
	string oldSN = sc_stream->sensor();
	string oldSNSN = sc_stream->sensorSerialNumber();

	try {
		if ( epoch->end().valid() )
			sc_stream->setEnd(epoch->end());
		else
			sc_stream->setEnd(Core::None);
	}
	catch ( ... ) { sc_stream->setEnd(Core::None); }

	sc_stream->setDepth(epoch->depth().value());
	sc_stream->setFormat(epoch->storageFormat());
	sc_stream->setRestricted(isRestricted);

	// Set sample rate
	try {
		pair<int,int> rat = double2frac(epoch->sampleRate().value());
		sc_stream->setSampleRateNumerator(rat.first);
		sc_stream->setSampleRateDenominator(rat.second);
	}
	catch ( ... ) {}

	// Set orientation
	bool hasOrientation = false;
	try { sc_stream->setAzimuth(epoch->azimuth().value()); hasOrientation = true; }
	catch ( ... ) {}
	try { sc_stream->setDip(epoch->dip().value()); }
	catch ( ... ) { hasOrientation = false; }

	// Set datalogger/sensor channel according to the component code
	if ( sc_stream->code().substr(2,1)=="N" || sc_stream->code().substr(2,1)=="1" ) {
		sc_stream->setDataloggerChannel(1);
		sc_stream->setSensorChannel(1);
	}
	else if ( sc_stream->code().substr(2,1)=="E" || sc_stream->code().substr(2,1)=="2" ) {
		sc_stream->setDataloggerChannel(2);
		sc_stream->setSensorChannel(2);
	}
	else {
		sc_stream->setDataloggerChannel(0);
		sc_stream->setSensorChannel(0);
	}

	// Store responses
	_responses.clear();

	for ( size_t i = 0; i < epoch->responseCount(); ++i )
		_responses.push_back(epoch->response(i));

	_responses.sort(respLowerThan);

	bool foundStage0 = false;
	for ( Responses::iterator it = _responses.begin(); it != _responses.end(); ++it ) {
		if ( (*it)->stage() == 0 ) {
			sc_stream->setGain((*it)->stageSensitivity().sensitivityValue());
			try { sc_stream->setGainFrequency((*it)->stageSensitivity().frequency()); }
			catch ( ... ) { sc_stream->setGainFrequency(Core::None); }

			sc_stream->setGainUnit((*it)->stageSensitivity().sensitivityUnits());
			foundStage0 = true;
			break;
		}
	}

	if ( !foundStage0 ) {
		try {
			sc_stream->setGain(epoch->instrumentSensitivity().sensitivityValue());
		}
		catch ( ... ) {
			SEISCOMP_WARNING("%s.%s.%s.%s: response stage 0 not found and "
			                 "instrument sensitivity not set: gain undefined",
			                 sc_loc->station()->network()->code().c_str(),
			                 sc_loc->station()->code().c_str(),
			                 sc_loc->code().c_str(), sc_stream->code().c_str());
		}

		try {
			sc_stream->setGainFrequency(epoch->instrumentSensitivity().frequency());
		}
		catch ( ... ) {
			SEISCOMP_WARNING("%s.%s.%s.%s: response stage 0 not found and "
			                 "instrument sensitivity freq not set: freq undefined",
			                 sc_loc->station()->network()->code().c_str(),
			                 sc_loc->station()->code().c_str(),
			                 sc_loc->code().c_str(), sc_stream->code().c_str());
		}

		try {
			string unit = epoch->instrumentSensitivity().sensitivityUnits();
			// Cut from first whitespace
			sc_stream->setGainUnit(unit.substr(0, unit.find(' ')));
		}
		catch ( ... ) {
			SEISCOMP_WARNING("%s.%s.%s.%s: response stage 0 not found and "
			                 "instrument sensitivity unit not set: unit undefined",
			                 sc_loc->station()->network()->code().c_str(),
			                 sc_loc->station()->code().c_str(),
			                 sc_loc->code().c_str(), sc_stream->code().c_str());
		}
	}

	// If orientation is available and gain is negative, invert orientation
	if ( hasOrientation ) {
		try {
			if ( sc_stream->gain() < 0 ) {
				if ( sc_stream->azimuth() < 180.0 )
					sc_stream->setAzimuth(sc_stream->azimuth() + 180.0);
				else
					sc_stream->setAzimuth(sc_stream->azimuth() - 180.0);
				sc_stream->setDip(-sc_stream->dip());
				sc_stream->setGain(-sc_stream->gain());
			}
		}
		catch ( ... ) {}
	}

	sc_stream->setDataloggerSerialNumber("");

	try {
		sc_stream->setDataloggerSerialNumber(epoch->datalogger().serialNumber());
	}
	catch ( ... ) {}

	try {
		sc_stream->setSensorSerialNumber(epoch->sensor().serialNumber());
	}
	catch ( ... ) {}

	// Set a default serial number if there is none
	if ( sc_stream->dataloggerSerialNumber().empty() ) {
		/*
		sc_stream->setDataloggerSerialNumber(
			sc_loc->station()->code() + "." + sc_loc->code() +
			sc_stream->code() + "." +
			date2str(sc_stream->start())
		);
		*/
		sc_stream->setDataloggerSerialNumber("xxxx");
	}

	// Set a default serial number if there is none
	if ( sc_stream->sensorSerialNumber().empty() )
		sc_stream->setSensorSerialNumber("yyyy");

	string dataloggerName;
	dataloggerName = sc_loc->station()->network()->code() + "." +
	                 sc_loc->station()->code() + "." + sc_loc->code() +
	                 sc_stream->code() + "." +
	                 date2str(sc_stream->start());

	// Update datalogger information
	DataModel::DataloggerPtr sc_dl = updateDatalogger(dataloggerName, epoch);
	if ( sc_dl ) {
		sc_stream->setDatalogger(sc_dl->publicID());
		process(sc_dl.get(), sc_stream.get(), epoch);
	}
	else
		sc_stream->setDatalogger("");

	// Update sensor information
	string sensorName;
	sensorName = sc_loc->station()->network()->code() + "." +
	             sc_loc->station()->code() + "." + sc_loc->code() +
	             sc_stream->code() + "." +
	             date2str(sc_stream->start());

	DataModel::SensorPtr sc_sens = updateSensor(sensorName, epoch, resp);
	if ( sc_sens ) {
		sc_stream->setSensor(sc_sens->publicID());
		if ( resp )
			process(sc_sens.get(), sc_stream.get(), epoch, resp);
	}

	UPD(needUpdate, oldEnd, Core::Time, sc_stream->end());
	UPD(needUpdate, oldDep, double, sc_stream->depth());
	if ( oldFormat != sc_stream->format() ) needUpdate = true;
	UPD(needUpdate, oldRestricted, bool, sc_stream->restricted());
	UPD(needUpdate, oldsrNum, int, sc_stream->sampleRateNumerator());
	UPD(needUpdate, oldsrDen, int, sc_stream->sampleRateDenominator());
	UPD(needUpdate, oldAzi, double, sc_stream->azimuth());
	UPD(needUpdate, oldDip, double, sc_stream->dip());
	UPD(needUpdate, oldDLcha, int, sc_stream->dataloggerChannel());
	UPD(needUpdate, oldSNcha, int, sc_stream->sensorChannel());
	UPD(needUpdate, oldGain, double, sc_stream->gain());
	UPD(needUpdate, oldGainFreq, double, sc_stream->gainFrequency());
	if ( oldGainUnit != sc_stream->gainUnit() ) needUpdate = true;
	if ( oldDL != sc_stream->datalogger() ) needUpdate = true;
	if ( oldDLSN != sc_stream->dataloggerSerialNumber() ) needUpdate = true;
	if ( oldSN != sc_stream->sensor() ) needUpdate = true;
	if ( oldSNSN != sc_stream->sensorSerialNumber() ) needUpdate = true;

	if ( newInstance ) {
		sc_loc->add(sc_stream.get());
		SEISCOMP_DEBUG("Added new stream epoch: %s (%s)",
		               sc_stream->code().c_str(), sc_stream->start().iso().c_str());
	}
	else if ( needUpdate ) {
		sc_stream->update();
		SEISCOMP_DEBUG("Updated stream epoch: %s (%s)",
		               sc_stream->code().c_str(), sc_stream->start().iso().c_str());
	}

	// Register aux stream
	_touchedStreams.insert(
		StreamIndex(
			LocationIndex(
				StationIndex(
					NetworkIndex(sc_loc->station()->network()->code(), sc_loc->station()->network()->start()),
					EpochIndex(sc_loc->station()->code(), sc_loc->station()->start())
				),
				EpochIndex(sc_loc->code(), sc_loc->start())
			),
			EpochIndex(sc_stream->code(), sc_stream->start())
		)
	);

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Convert2SC3::process(DataModel::Datalogger *sc_dl, DataModel::Stream *sc_stream,
                        const StationXML::ChannelEpoch *epoch) {
	updateDecimation(sc_dl, sc_stream, epoch);
	updateDataloggerCalibration(sc_dl, sc_stream, epoch);
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Convert2SC3::process(DataModel::Sensor *sc_sens, DataModel::Stream *sc_stream,
                          const StationXML::ChannelEpoch *epoch,
                          const StationXML::Response *resp) {
	updateSensorCalibration(sc_sens, sc_stream, epoch, resp);
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
DataModel::Datalogger *
Convert2SC3::updateDatalogger(const std::string &name,
                              const StationXML::ChannelEpoch *epoch) {
	const StationXML::Response *resp = findDataloggerResponse();

	/*
	if ( resp == NULL ) {
		SEISCOMP_DEBUG("Datalogger response not found, ignore it");
		return NULL;
	}
	*/

	if ( resp ) {
		cerr << "Found datalogger response stage: " << resp->stage() << endl;
	}
	else
		cerr << "Datalogger response not found" << endl;

	DataModel::DataloggerPtr sc_dl = DataModel::Datalogger::Create();
	//sc_dl->setName(sc_dl->publicID());
	sc_dl->setName(name);

	bool emptyDL = true;

	try { epoch->datalogger(); emptyDL = false; }
	catch ( ... ) {}

	try { sc_dl->setDescription(epoch->datalogger().description()); }
	catch ( ... ) { sc_dl->setDescription(""); }

	try { sc_dl->setDigitizerModel(epoch->datalogger().model()); }
	catch ( ... ) { sc_dl->setDigitizerModel(""); }

	try { sc_dl->setDigitizerManufacturer(epoch->datalogger().manufacturer()); }
	catch ( ... ) { sc_dl->setDigitizerManufacturer(""); }

	try {
		// Convert StationXML clockdrift (seconds/sample) to seconds/second
		double drift = epoch->clockDrift().value() * epoch->sampleRate().value();
		sc_dl->setMaxClockDrift(drift);
		emptyDL = false;
	}
	catch ( ... ) {
		sc_dl->setMaxClockDrift(Core::None);
	}

	if ( resp ) {
		sc_dl->setGain(fabs(resp->stageSensitivity().sensitivityValue()));
		emptyDL = false;
	}

	if ( emptyDL ) return NULL;

	/*
	bool newInstance = true;
	for ( size_t d = 0; d < _inv->dataloggerCount(); ++d ) {
		DataModel::Datalogger *dl = _inv->datalogger(d);
		if ( equal(dl, sc_dl.get()) ) {
			sc_dl = dl;
			newInstance = false;
			break;
		}
	}
	*/

	sc_dl = pushDatalogger(sc_dl.get());

	return sc_dl.get();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
DataModel::Decimation *
Convert2SC3::updateDecimation(DataModel::Datalogger *sc_dl,
                              DataModel::Stream *sc_stream,
                              const StationXML::ChannelEpoch *epoch) {
	bool newInstance = false;
	bool needUpdate = false;

	int numerator, denominator;

	try {
		numerator = sc_stream->sampleRateNumerator();
		denominator = sc_stream->sampleRateDenominator();
	}
	catch ( ... ) {
		return NULL;
	}

	DataModel::DecimationPtr sc_deci = sc_dl->decimation(DataModel::DecimationIndex(numerator, denominator));
	if ( sc_deci == NULL ) {
		sc_deci = new DataModel::Decimation();
		sc_deci->setSampleRateNumerator(numerator);
		sc_deci->setSampleRateDenominator(denominator);
		sc_dl->add(sc_deci.get());
		newInstance = true;
	}

	BCK(oldAFC, DataModel::Blob, sc_deci->analogueFilterChain())
	BCK(oldDFC, DataModel::Blob, sc_deci->analogueFilterChain())

	sc_deci->setAnalogueFilterChain(Core::None);
	sc_deci->setDigitalFilterChain(Core::None);

	updateDataloggerDigital(sc_dl, sc_deci.get(), epoch);
	updateDataloggerAnalogue(sc_dl, sc_deci.get(), epoch);

	UPD(needUpdate, oldAFC, DataModel::Blob, sc_deci->analogueFilterChain())
	UPD(needUpdate, oldDFC, DataModel::Blob, sc_deci->analogueFilterChain())

	if ( !newInstance && needUpdate ) {
		sc_deci->update();
		SEISCOMP_DEBUG("Reused datalogger decimation for stream %s",
		               sc_stream->code().c_str());
	}

	return sc_deci.get();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
DataModel::DataloggerCalibration *
Convert2SC3::updateDataloggerCalibration(DataModel::Datalogger *sc_dl,
                                       DataModel::Stream *sc_stream,
                                       const StationXML::ChannelEpoch *epoch) {
	bool newInstance = false;
	bool needUpdate = false;
	const StationXML::Response *resp = findDataloggerResponse();

	if ( resp == NULL ) {
		SEISCOMP_DEBUG("No datalogger response available, skip calibrations");
		return NULL;
	}

	DataModel::DataloggerCalibrationIndex idx(
		sc_stream->dataloggerSerialNumber(),
		sc_stream->dataloggerChannel(), sc_stream->start()
	);

	DataModel::DataloggerCalibrationPtr sc_cal = sc_dl->dataloggerCalibration(idx);
	if ( sc_cal == NULL ) {
		sc_cal = new DataModel::DataloggerCalibration();
		sc_cal->setSerialNumber(sc_stream->dataloggerSerialNumber());
		sc_cal->setChannel(sc_stream->dataloggerChannel());
		sc_cal->setStart(sc_stream->start());
		sc_dl->add(sc_cal.get());
		newInstance = true;
	}

	BCK(oldEnd, Core::Time, sc_cal->end());
	BCK(oldGain, double, sc_cal->gain());
	BCK(oldGainFreq, double, sc_cal->gainFrequency());

	try { sc_cal->setEnd(sc_stream->end()); }
	catch ( ... ) { sc_cal->setEnd(Core::None); }

	sc_cal->setGain(Core::None);
	sc_cal->setGainFrequency(Core::None);

	if ( resp ) {
		sc_cal->setGain(fabs(resp->stageSensitivity().sensitivityValue()));
		try { sc_cal->setGainFrequency(fabs(resp->stageSensitivity().frequency())); }
		catch ( ... ) {}
	}
	else
		SEISCOMP_WARNING("%s/%s cal[%d]: correct response stage not found: gain undefined",
		                 sc_dl->name().c_str(), sc_cal->serialNumber().c_str(),
		                 sc_cal->channel());

	UPD(needUpdate, oldEnd, Core::Time, sc_cal->end());
	UPD(needUpdate, oldGain, double, sc_cal->gain());
	UPD(needUpdate, oldGainFreq, double, sc_cal->gainFrequency());

	if ( !newInstance && needUpdate ) {
		sc_cal->update();
		SEISCOMP_DEBUG("Reused datalogger calibration for stream %s",
		               sc_stream->code().c_str());
	}

	return sc_cal.get();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Convert2SC3::updateDataloggerDigital(DataModel::Datalogger *sc_dl,
                                          DataModel::Decimation *sc_deci,
                                          const StationXML::ChannelEpoch *epoch) {
	/*
	string channel_name = sc_stream->sensorLocation()->station()->code() + "." +
	                      sc_stream->sensorLocation()->code() + "." +
	                      sc_stream->code() + "." + date2str(sc_stream->start());
	*/

	Responses::iterator it;
	for( it = _responses.begin(); it != _responses.end() ; ++it ) {
		string respID;
		if ( isPAZResponse(*it) ) {
			StationXML::PolesAndZeros *paz = (*it)->polesAndZeros(0);
			if ( paz->inputUnits() != DIGITAL || paz->outputUnits() != DIGITAL )
				continue;

			bool newPAZ = true;

			// Create PAZ ...
			DataModel::ResponsePAZPtr rp = convert(*it, paz);
			// Type is Z-transform (Digital)
			rp->setType("D");
			checkPAZ(rp.get());

			for ( size_t f = 0; f < _inv->responsePAZCount(); ++f ) {
				DataModel::ResponsePAZ *paz = _inv->responsePAZ(f);
				if ( equal(paz, rp.get()) ) {
					rp = paz;
					newPAZ = false;
					break;
				}
			}

			if ( newPAZ ) {
				_inv->add(rp.get());
				//SEISCOMP_DEBUG("Added new PAZ response from paz: %s", rp->publicID().c_str());
			}
			else {
				//SEISCOMP_DEBUG("Reused PAZ response from paz: %s", rp->publicID().c_str());
			}

			respID = rp->publicID();
		}
		else if ( isCoeffResponse(*it) || isFIRResponse(*it) ) {
			// Check COEFF or FIR responses
			bool newFIR = true;

			DataModel::ResponseFIRPtr rf;

			if ( isFIRResponse(*it) ) {
				StationXML::FIR *fir = (*it)->fIR(0);
				rf = convert(*it, fir);
			}
			else if ( isCoeffResponse(*it) ) {
				StationXML::Coefficients *coeff = (*it)->coefficients(0);
				if ( coeff->numeratorCount() == 0 ) continue;
				rf = convert(*it, coeff);
			}

			checkFIR(rf.get());

			for ( size_t f = 0; f < _inv->responseFIRCount(); ++f ) {
				DataModel::ResponseFIR *fir = _inv->responseFIR(f);
				if ( equal(fir, rf.get()) ) {
					rf = fir;
					newFIR = false;
					break;
				}
			}

			if ( newFIR ) {
				_inv->add(rf.get());
				//SEISCOMP_DEBUG("Added new FIR filter from coefficients: %s", rf->publicID().c_str());
			}
			else {
				//SEISCOMP_DEBUG("Reuse FIR filter from coefficients: %s", rf->publicID().c_str());
			}

			respID = rf->publicID();
		}
		else
			continue;

		string dfc;

		try { dfc = sc_deci->digitalFilterChain().content(); }
		catch( ... ) {}

		if ( !dfc.empty() ) dfc += " ";
		dfc += respID;
		DataModel::Blob blob;
		blob.setContent(dfc);
		sc_deci->setDigitalFilterChain(blob);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Convert2SC3::updateDataloggerAnalogue(DataModel::Datalogger *sc_dl,
                                           DataModel::Decimation *sc_deci,
                                           const StationXML::ChannelEpoch *epoch) {
	Responses::iterator it;

	// Find PAZ for V->V
	for ( it = _responses.begin(); it != _responses.end(); ++it ) {
		if ( (*it)->polesAndZerosCount() == 0 ) continue;
		StationXML::PolesAndZeros *paz = (*it)->polesAndZeros(0);
		if ( paz->inputUnits() != CURRENT || paz->outputUnits() != CURRENT ) continue;

		bool newPAZ = true;

		// Create PAZ ...
		DataModel::ResponsePAZPtr rp = convert(*it, paz);
		checkPAZ(rp.get());

		for ( size_t f = 0; f < _inv->responsePAZCount(); ++f ) {
			DataModel::ResponsePAZ *paz = _inv->responsePAZ(f);
			if ( equal(paz, rp.get()) ) {
				rp = paz;
				newPAZ = false;
				break;
			}
		}

		if ( newPAZ ) {
			_inv->add(rp.get());
			//SEISCOMP_DEBUG("Added new PAZ response from paz: %s", rp->publicID().c_str());
		}
		else {
			//SEISCOMP_DEBUG("Reused PAZ response from paz: %s", rp->publicID().c_str());
		}

		string dfc;

		try { dfc = sc_deci->analogueFilterChain().content(); }
		catch( ... ) {}

		if ( !dfc.empty() ) dfc += " ";
		dfc += rp->publicID();
		DataModel::Blob blob;
		blob.setContent(dfc);
		sc_deci->setAnalogueFilterChain(blob);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
DataModel::Sensor *
Convert2SC3::updateSensor(const std::string &name,
                          const StationXML::ChannelEpoch *epoch,
                          const StationXML::Response *resp) {
	const StationXML::PolesAndZeros *paz = NULL;
	const StationXML::Polynomial *poly = NULL;

	const char* unit;

	unit = sensorUnit(resp, paz, poly);

	DataModel::SensorPtr sc_sens = DataModel::Sensor::Create();
	//sc_sens->setName(sc_sens->publicID());
	sc_sens->setName(name);

	bool emptySensor = true;

	// No type yet: we use values as SM, SP, BB, VBB, ...
	// but there is no equivalent to StationXML
	//sc_sens->setType("...");

	try { epoch->sensor(); emptySensor = false; }
	catch ( ... ) {}

	try {
		sc_sens->setDescription(epoch->sensor().description());
		sc_sens->setManufacturer(epoch->sensor().manufacturer());
		sc_sens->setModel(epoch->sensor().model());

		if ( sc_sens->description().empty() )
			sc_sens->setDescription(epoch->sensor().equipType());
		else if ( sc_sens->model().empty() )
			sc_sens->setModel(epoch->sensor().equipType());
	}
	catch ( ... ) {}

	sc_sens->setUnit(unit);
	if ( !sc_sens->unit().empty() ) emptySensor = false;

	if ( paz ) {
		DataModel::ResponsePAZPtr rp = convert(resp, paz);
		checkPAZ(rp.get());

		bool newPAZ = true;
		for ( size_t f = 0; f < _inv->responsePAZCount(); ++f ) {
			DataModel::ResponsePAZ *paz = _inv->responsePAZ(f);
			if ( equal(paz, rp.get()) ) {
				rp = paz;
				newPAZ = false;
				break;
			}
		}

		if ( newPAZ ) {
			_inv->add(rp.get());
			SEISCOMP_DEBUG("Added new Sensor.ResponsePAZ from paz: %s", rp->publicID().c_str());
		}
		else {
			SEISCOMP_DEBUG("Reused Sensor.ResponsePAZ from paz: %s", rp->publicID().c_str());
		}

		SEISCOMP_DEBUG("Update Sensor.response: %s -> %s",
		               sc_sens->publicID().c_str(), rp->publicID().c_str());

		sc_sens->setResponse(rp->publicID());
		emptySensor = false;
	}
	else if ( poly ) {
		DataModel::ResponsePolynomialPtr rp = convert(resp, poly);
		checkPoly(rp.get());

		bool newPoly = true;
		for ( size_t f = 0; f < _inv->responsePolynomialCount(); ++f ) {
			DataModel::ResponsePolynomial *poly = _inv->responsePolynomial(f);
			if ( equal(poly, rp.get()) ) {
				rp = poly;
				newPoly = false;
				break;
			}
		}

		if ( newPoly ) {
			_inv->add(rp.get());
			//SEISCOMP_DEBUG("Added new polynomial response from poly: %s", rp->publicID().c_str());
		}
		else {
			//SEISCOMP_DEBUG("reused polynomial response from poly: %s", rp->publicID().c_str());
		}

		sc_sens->setResponse(rp->publicID());
		emptySensor = false;
	}

	/*
	bool newInstance = true;
	for ( size_t s = 0; s < _inv->sensorCount(); ++s ) {
		DataModel::Sensor *sens = _inv->sensor(s);
		if ( equal(sens, sc_sens.get()) ) {
			sc_sens = sens;
			newInstance = false;
			break;
		}
	}
	*/

	if ( emptySensor ) return NULL;

	SEISCOMP_DEBUG("Pushing new sensor: %s", sc_sens->publicID().c_str());
	sc_sens = pushSensor(sc_sens.get());

	return sc_sens.get();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
DataModel::SensorCalibration *
Convert2SC3::updateSensorCalibration(DataModel::Sensor *sc_sens, DataModel::Stream *sc_stream,
                                     const StationXML::ChannelEpoch *epoch,
                                     const StationXML::Response *resp) {
	bool newInstance = false;
	bool needUpdate = false;

	DataModel::SensorCalibrationIndex idx(
		sc_stream->sensorSerialNumber(),
		sc_stream->sensorChannel(), sc_stream->start()
	);

	DataModel::SensorCalibrationPtr sc_cal = sc_sens->sensorCalibration(idx);
	if ( sc_cal == NULL ) {
		sc_cal = new DataModel::SensorCalibration();
		sc_cal->setSerialNumber(sc_stream->sensorSerialNumber());
		sc_cal->setChannel(sc_stream->sensorChannel());
		sc_cal->setStart(sc_stream->start());
		sc_sens->add(sc_cal.get());
		newInstance = true;
	}

	BCK(oldEnd, Core::Time, sc_cal->end());
	BCK(oldGain, double, sc_cal->gain());
	BCK(oldGainFreq, double, sc_cal->gainFrequency());

	try { sc_cal->setEnd(sc_stream->end()); }
	catch ( ... ) { sc_cal->setEnd(Core::None); }

	sc_cal->setGain(Core::None);
	sc_cal->setGainFrequency(Core::None);

	sc_cal->setGain(fabs(resp->stageSensitivity().sensitivityValue()));
	try { sc_cal->setGainFrequency(fabs(resp->stageSensitivity().frequency())); }
	catch ( ... ) {}

	UPD(needUpdate, oldEnd, Core::Time, sc_cal->end());
	UPD(needUpdate, oldGain, double, sc_cal->gain());
	UPD(needUpdate, oldGainFreq, double, sc_cal->gainFrequency());

	if ( !newInstance && needUpdate ) sc_cal->update();

	return sc_cal.get();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const StationXML::Response *Convert2SC3::findDataloggerResponse() const {
	Responses::const_iterator it;
	for ( it = _responses.begin(); it != _responses.end(); ++it ) {
		if ( !isFIRResponse(*it) && !isCoeffResponse(*it) ) continue;

		if ( isCoeffResponse(*it) ) {
			StationXML::Coefficients *rc = (*it)->coefficients(0);
			if ( rc->numeratorCount() == 0 )
				return *it;
		}

		// Find the stage before the first coefficient definition
		int stageToFind = (*it)->stage() - 1;
		for ( it = _responses.begin(); it != _responses.end(); ++it ) {
			if ( (*it)->stage() == stageToFind )
				return *it;
		}

		break;
	}

	// Check if the last response is a meta response

	return NULL;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
DataModel::Datalogger *
Convert2SC3::pushDatalogger(DataModel::Datalogger *dl) {
	ObjectLookup::iterator it = _dataloggerLookup.find(dl->name());
	if ( it != _dataloggerLookup.end() ) {
		DataModel::Datalogger *cdl = (DataModel::Datalogger*)it->second;
		if ( !equal(cdl, dl) ) {
			*cdl = *dl;
			cdl->update();
			SEISCOMP_DEBUG("Updated datalogger: %s", cdl->publicID().c_str());
		}
		else
			SEISCOMP_DEBUG("Reused datalogger: %s", cdl->publicID().c_str());
		return cdl;
	}

	_inv->add(dl);
	_dataloggerLookup[dl->name()] = dl;
	SEISCOMP_DEBUG("Added new datalogger: %s", dl->publicID().c_str());
	return dl;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
DataModel::Sensor *Convert2SC3::pushSensor(DataModel::Sensor *sens) {
	ObjectLookup::iterator it = _sensorLookup.find(sens->name());
	if ( it != _sensorLookup.end() ) {
		DataModel::Sensor *csens = (DataModel::Sensor*)it->second;
		if ( !equal(csens, sens) ) {
			*csens = *sens;
			csens->update();
			SEISCOMP_DEBUG("Updated sensor: %s", csens->publicID().c_str());
		}
		else
			SEISCOMP_DEBUG("Reused sensor: %s", csens->publicID().c_str());
		return csens;
	}

	_inv->add(sens);
	_sensorLookup[sens->name()] = sens;
	SEISCOMP_DEBUG("Added new sensor: %s", sens->publicID().c_str());
	return sens;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
