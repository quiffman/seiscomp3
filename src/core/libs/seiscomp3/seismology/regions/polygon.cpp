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



#include <seiscomp3/seismology/regions/polygon.h>

#include <iostream>
#include <fstream>
#include <stdlib.h>

#include <boost/version.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#define SEISCOMP_COMPONENT PolyRegion
#include <seiscomp3/logging/log.h>

namespace fs = boost::filesystem;

namespace Seiscomp
{
namespace Geo
{


namespace {


bool compareByRank(const GeoFeature* gf1, const GeoFeature* gf2 ) {
	return gf1->rank() < gf2->rank();
}


}


PolyRegions::PolyRegions()
{
}


PolyRegions::PolyRegions(const std::string& location)
{
	read(location);
}


PolyRegions::~PolyRegions() {
	for (size_t i = 0; i < _regions.size(); i++ ) {
		delete _regions[i];
	}
}


size_t PolyRegions::read(const std::string& location) {
	fs::path directory;
	try {
		directory = fs::path(location, fs::native);
	}
	catch ( ... ) {
		SEISCOMP_ERROR("Invalid path '%s'", location.c_str());
		return 0;
	}

	fs::directory_iterator end_itr;

	try{
		for (fs::directory_iterator itr(directory); itr != end_itr; ++itr ){

			if (fs::is_directory(*itr))
				continue;

#if (BOOST_VERSION <= 103301)
			if (boost::regex_match(itr->leaf(), boost::regex(".*\\.(?:fep)")))
#else
			if (boost::regex_match(itr->path().leaf(), boost::regex(".*\\.(?:fep)")))
#endif
				if(! readFepBoundaries(itr->string()))
					SEISCOMP_ERROR("Error reading file: %s", itr->string().c_str());
		}
	}
	catch (const std::exception& ex){
		return regionCount();
	}

	info();

	// Sort the features according to their rank
 	std::sort(_regions.begin(), _regions.end(), compareByRank);

	return regionCount();
}


bool PolyRegions::readFepBoundaries(const std::string& filename) {

	SEISCOMP_DEBUG("reading boundary polygons from file: %s", filename.c_str());

	std::ifstream infile(filename.c_str());

	if (infile.bad())
		return false;

	boost::regex vertexLine("^\\s*([-+]?[0-9]*\\.?[0-9]+)\\s+([-+]?[0-9]*\\.?[0-9]+)(?:\\s+([^\\d\\s].*)$|\\s*$)");
	boost::regex LLine("^\\s*L\\s+(.*)$");
	boost::smatch what;
	
  	std::string line;
	bool newPolygon = true;
	GeoFeature *pr;

	while(std::getline(infile, line)) {

		if (newPolygon){
			pr = new GeoFeature();
			newPolygon = false;
		}

		if (boost::regex_match(line, what, vertexLine)){
			pr->addVertex(atof(what.str(2).c_str()), atof(what.str(1).c_str()));
		}
		else if (boost::regex_match(line, what, LLine)){
			pr->setName(what.str(1));
			pr->setClosedPolygon(true);
			addRegion(pr);
			newPolygon = true;
		}
		else {
			//std::cout << "Warning: line ignored: " << line << std::endl;
		}
		
	}

	return true;

}


void PolyRegions::addRegion(GeoFeature *r) {
	_regions.push_back(r);
}


size_t PolyRegions::regionCount() const {
	return _regions.size();
}


GeoFeature *PolyRegions::region(int i) const{
	if( _regions.size() > 0 )
		return _regions[i];
	else
		return NULL;
}


void PolyRegions::print(){

	for ( size_t i = 0; i < _regions.size(); i++ ){
		std::cerr << region(i)->name() << std::endl;
		std::cerr <<  region(i)->vertices().size() << std::endl;

	//for (int j = 0; j < region(i)->vertexCount(); j++)
		//std::cerr << region(i)->vertex(j).lat << "\t" << region(i)->vertex(j).lon << std::endl;

	}

}


void PolyRegions::info(){

	SEISCOMP_DEBUG("Number of PolyRegions loaded: %lu", (unsigned long)_regions.size());

	int sum = 0;
	for ( size_t i = 0; i < _regions.size(); i++ )
		sum += region(i)->vertices().size();

	SEISCOMP_DEBUG("Total number of vertices read in: %d", sum);

}


GeoFeature *PolyRegions::findRegion(double lat, double lon) const {
	for ( size_t i = 0; i < regionCount(); ++i ) {
		if ( region(i)->contains(Vertex(lat, lon)) )
			return region(i);
	}

	return NULL;
}


std::string PolyRegions::findRegionName(double lat, double lon) const {
	GeoFeature *region = findRegion(lat, lon);
	if ( region )
		return region->name();

	return "";
}


}
}
