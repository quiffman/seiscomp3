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

// This file was created by a source code generator.
// Do not modify the contents. Change the definition and run the generator
// again!


#ifndef __SEISCOMP_DATAMODEL_STREAM_H__
#define __SEISCOMP_DATAMODEL_STREAM_H__


#include <seiscomp3/core/datetime.h>
#include <string>
#include <seiscomp3/datamodel/object.h>
#include <seiscomp3/core/exceptions.h>


namespace Seiscomp {
namespace DataModel {


DEFINE_SMARTPOINTER(Stream);

class SensorLocation;


class SC_CORE_DATAMODEL_API StreamIndex {
	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		StreamIndex();
		StreamIndex(const std::string& code,
		            Seiscomp::Core::Time start);

		//! Copy constructor
		StreamIndex(const StreamIndex&);


	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		bool operator==(const StreamIndex&) const;
		bool operator!=(const StreamIndex&) const;


	// ------------------------------------------------------------------
	//  Attributes
	// ------------------------------------------------------------------
	public:
		std::string code;
		Seiscomp::Core::Time start;
};


class SC_CORE_DATAMODEL_API Stream : public Object {
	DECLARE_SC_CLASS(Stream);
	DECLARE_SERIALIZATION;
	DECLARE_METAOBJECT;

	// ------------------------------------------------------------------
	//  Xstruction
	// ------------------------------------------------------------------
	public:
		//! Constructor
		Stream();

		//! Copy constructor
		Stream(const Stream& other);

		//! Destructor
		~Stream();
	

	// ------------------------------------------------------------------
	//  Operators
	// ------------------------------------------------------------------
	public:
		//! Copies the metadata of other to this
		Stream& operator=(const Stream& other);
		//! Checks for equality of two objects. Childs objects
		//! are not part of the check.
		bool operator==(const Stream& other) const;
		bool operator!=(const Stream& other) const;

		//! Wrapper that calls operator==
		bool equal(const Stream& other) const;


	// ------------------------------------------------------------------
	//  Setters/Getters
	// ------------------------------------------------------------------
	public:
		void setCode(const std::string& code);
		const std::string& code() const;

		void setStart(Seiscomp::Core::Time start);
		Seiscomp::Core::Time start() const;

		void setEnd(const OPT(Seiscomp::Core::Time)& end);
		Seiscomp::Core::Time end() const throw(Seiscomp::Core::ValueException);

		void setDatalogger(const std::string& datalogger);
		const std::string& datalogger() const;

		void setDataloggerSerialNumber(const std::string& dataloggerSerialNumber);
		const std::string& dataloggerSerialNumber() const;

		void setDataloggerChannel(const OPT(int)& dataloggerChannel);
		int dataloggerChannel() const throw(Seiscomp::Core::ValueException);

		void setSensor(const std::string& sensor);
		const std::string& sensor() const;

		void setSensorSerialNumber(const std::string& sensorSerialNumber);
		const std::string& sensorSerialNumber() const;

		void setSensorChannel(const OPT(int)& sensorChannel);
		int sensorChannel() const throw(Seiscomp::Core::ValueException);

		void setClockSerialNumber(const std::string& clockSerialNumber);
		const std::string& clockSerialNumber() const;

		void setSampleRateNumerator(const OPT(int)& sampleRateNumerator);
		int sampleRateNumerator() const throw(Seiscomp::Core::ValueException);

		void setSampleRateDenominator(const OPT(int)& sampleRateDenominator);
		int sampleRateDenominator() const throw(Seiscomp::Core::ValueException);

		void setDepth(const OPT(double)& depth);
		double depth() const throw(Seiscomp::Core::ValueException);

		void setAzimuth(const OPT(double)& azimuth);
		double azimuth() const throw(Seiscomp::Core::ValueException);

		void setDip(const OPT(double)& dip);
		double dip() const throw(Seiscomp::Core::ValueException);

		void setGain(const OPT(double)& gain);
		double gain() const throw(Seiscomp::Core::ValueException);

		void setGainFrequency(const OPT(double)& gainFrequency);
		double gainFrequency() const throw(Seiscomp::Core::ValueException);

		void setGainUnit(const std::string& gainUnit);
		const std::string& gainUnit() const;

		void setFormat(const std::string& format);
		const std::string& format() const;

		void setFlags(const std::string& flags);
		const std::string& flags() const;

		void setRestricted(const OPT(bool)& restricted);
		bool restricted() const throw(Seiscomp::Core::ValueException);

		void setShared(const OPT(bool)& shared);
		bool shared() const throw(Seiscomp::Core::ValueException);


	// ------------------------------------------------------------------
	//  Index management
	// ------------------------------------------------------------------
	public:
		//! Returns the object's index
		const StreamIndex& index() const;

		//! Checks two objects for equality regarding their index
		bool equalIndex(const Stream* lhs) const;

	
	// ------------------------------------------------------------------
	//  Public interface
	// ------------------------------------------------------------------
	public:
		SensorLocation* sensorLocation() const;

		//! Implement Object interface
		bool assign(Object* other);
		bool attachTo(PublicObject* parent);
		bool detachFrom(PublicObject* parent);
		bool detach();

		//! Creates a clone
		Object* clone() const;

		void accept(Visitor*);


	// ------------------------------------------------------------------
	//  Implementation
	// ------------------------------------------------------------------
	private:
		// Index
		StreamIndex _index;

		// Attributes
		OPT(Seiscomp::Core::Time) _end;
		std::string _datalogger;
		std::string _dataloggerSerialNumber;
		OPT(int) _dataloggerChannel;
		std::string _sensor;
		std::string _sensorSerialNumber;
		OPT(int) _sensorChannel;
		std::string _clockSerialNumber;
		OPT(int) _sampleRateNumerator;
		OPT(int) _sampleRateDenominator;
		OPT(double) _depth;
		OPT(double) _azimuth;
		OPT(double) _dip;
		OPT(double) _gain;
		OPT(double) _gainFrequency;
		std::string _gainUnit;
		std::string _format;
		std::string _flags;
		OPT(bool) _restricted;
		OPT(bool) _shared;
};


}
}


#endif
