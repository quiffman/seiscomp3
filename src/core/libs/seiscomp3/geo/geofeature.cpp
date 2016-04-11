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

#include <seiscomp3/geo/geofeature.h>
#include <iostream>

using namespace Seiscomp::Geo;


GeoFeature::GeoFeature(const Category* category, unsigned int rank)
	: _category(category), _rank(rank), _closedPolygon(false) {}


GeoFeature::GeoFeature(const std::string& name, const Category* category,
	unsigned int rank) : _name(name), _category(category), _rank(rank),
	_closedPolygon(false) {}


GeoFeature::~GeoFeature() {
	_vertices.clear();
}


void GeoFeature::addVertex(const Vertex &v, bool newSubFeature) {
	// Initialize/update bounding box if necessary.
	if ( _vertices.empty() ) {
		_bbox.latMin = _bbox.latMax = v.lat;
		_bbox.lonMin = _bbox.lonMax = v.lon;
	}
	else {
		if ( v.lat < _bbox.latMin ) _bbox.latMin = v.lat;
		if ( v.lat > _bbox.latMax ) _bbox.latMax = v.lat;
		if ( v.lon < _bbox.lonMin ) _bbox.lonMin = v.lon;
		if ( v.lon > _bbox.lonMax ) _bbox.lonMax = v.lon;
	}

	// Mark this vertex as the beginning of a new sub feature. Note: The first
	// vertex is never a subfeature
	if ( newSubFeature && !_vertices.empty() )
		_subFeatures.push_back(_vertices.size());

	// Add the new vertex
	_vertices.push_back(v);
}


namespace {


template <typename T>
T sub(T a, T b) {
	T s = a - b;
	if ( s < -180 )
		s += 360;
	else if ( s > 180 )
		s -= 360;
	return s;
}


}


bool GeoFeature::contains(const Vertex &v) const {
	if ( !closedPolygon() ) return false;

	size_t startIdx = 0, endIdx = 0;
	size_t nSubFeat = _subFeatures.size();
	for ( size_t i = 0; i <= nSubFeat; ++i ) {
		endIdx = (i == nSubFeat ? _vertices.size() : _subFeatures[i]);
		if ( contains(v, &_vertices[startIdx], endIdx - startIdx) )
			return true;
		startIdx = endIdx;
	}
	return false;
}


bool GeoFeature::contains(const Vertex& v, const Vertex *polygon,
                          size_t sides) {
	// should not happen since when reading the BNA files the last point is
	// removed if it equals the first point
	if ( polygon[0] == polygon[sides-1] )
		--sides;

	// no polygon if less than 3 sites
	if ( sides < 3 ) return false;

	int j = sides - 1;
	bool oddCrossings = false;

	for ( size_t i = 0; i < sides; j = i++ ) {
		Vertex::ValueType segWidth = sub(polygon[i].lon, polygon[j].lon);
		Vertex::ValueType relLonLeft = sub(v.lon, polygon[j].lon);
		Vertex::ValueType relLonRight = sub(v.lon, polygon[i].lon);

		// Skip segments where v.lon falls outside the segments horizontal
		// boundings
		if ( segWidth >= 0 ) {
			if ( relLonLeft < 0 || relLonRight > 0 ) continue;
		}
		else {
			if ( relLonLeft > 0 || relLonRight < 0 ) continue;
		}

		Vertex::ValueType y0;

		// y0 = latitude of crossing
		y0 = relLonLeft * (polygon[i].lat - polygon[j].lat) /
		     fabs(segWidth) + polygon[j].lat;

		// North crossing
		if ( y0 > v.lat )
			oddCrossings = !oddCrossings;
	}

	return oddCrossings;
}
