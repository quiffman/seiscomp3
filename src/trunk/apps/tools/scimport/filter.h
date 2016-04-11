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



#ifndef __SEISCOMP_APPLICATIONS_FILTER__
#define __SEISCOMP_APPLICATIONS_FILTER__

#define SEISCOMP_COMPONENT ScImport

#include <seiscomp3/datamodel/pick.h>
#include <seiscomp3/datamodel/amplitude.h>
#include <seiscomp3/datamodel/origin.h>
#include <seiscomp3/datamodel/event.h>
#include <seiscomp3/datamodel/stationmagnitude.h>
#include <seiscomp3/datamodel/magnitude.h>
#include <seiscomp3/datamodel/waveformquality.h>
#include <seiscomp3/datamodel/utils.h>
#include <seiscomp3/core/strings.h>
#include <seiscomp3/logging/log.h>
#include <seiscomp3/client/application.h>

namespace Seiscomp {
namespace Applications {

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
/**
 * \class Filter
 * \brief Abstract class to implement a object filter for scimport.
 *
 * To implement a filter subclass some macros have do be called.
 * \code
 * class YourFilter : public Filter {
 *  	DEFINE_MESSAGE_FILTER( DataModel::SpecificObject::ClassName(), YourFilter );
 *
 *	public:
 *		virtual void init(Config* conf, std::string& configPrefix)	{ ..... }
 *		virtual bool filterImpl(Core::BaseObject* object) { .... }
 * };
 * \endcode
 *
 * Additionally you have to place another macro in your implementation file.
 * \code
 * IMPLEMENT_MESSAGE_FILTER(YourFilter);
 * \endcode
 *
 * Once a filter is implemented it will be automatically registered and used
 * in the message filtering process.
 */
class Filter
{
public:
	enum Operator { EQ = 0, LT, GT, ANY, INVALID };

private:
	typedef std::map<std::string, Filter*> Filters;
	typedef std::map<std::string, Operator> Operators;

protected:

	Filter() : enabled(false), wildcardStr("*")
	{
		_Operators.insert(std::make_pair<std::string, Operator>("eq", EQ));
		_Operators.insert(std::make_pair<std::string, Operator>("lt", LT));
		_Operators.insert(std::make_pair<std::string, Operator>("gt", GT));
		_Operators.insert(std::make_pair<std::string, Operator>("*", ANY));
	}

	virtual ~Filter() {
	}

public:
	bool filter(Core::BaseObject* object)
	{
		if (enabled)
			return filterImpl(object);
		return enabled;
	}

	static Filter* GetFilter(const std::string& className)
	{
		Filters::iterator it = _Registry.find(className);
		if (it == _Registry.end())
			return NULL;
		return it->second;
	}

	static void Init(Client::Application& app) {
		Filters::iterator it = _Registry.begin();
		for ( ; it != _Registry.end(); ++it )
			it->second->init(app);
	}

protected:

	/**
	 * Needs to be implemented by a subclass to add filter functionality for
	 * a specific object. Returns true if the class is eligible to pass the
	 * filter. False otherwise.
	 * @param object
	 * @return bool
	 */
	virtual bool filterImpl(Core::BaseObject* object) = 0;

	/**
	 * Needs to be implemented by a subclass in order to add initalization code
	 * like reading from a configuration file, default assignement to members and
	 * the like.
	 */
	virtual void init(Client::Application& app) = 0;

	static void RegisterFilter(const std::string& name, Filter* filterObject)
	{
		_Registry[name] = filterObject;
	}

	static Operator GetOperator(const std::string& op)
	{
		Operators::iterator it = _Operators.find(op);
		if (it != _Operators.end())
			return it->second;
		return INVALID;
	}

protected:
	bool enabled;
	const std::string wildcardStr;

private:

	static Filters   _Registry;
	static Operators _Operators;

};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#define DEFINE_MESSAGE_FILTER( OBJECTNAME, FILTERCLASS ) \
public: \
	FILTERCLASS() { \
		RegisterFilter(OBJECTNAME, this); \
} \
private: \
	static FILTERCLASS __##FILTERCLASS##_Object__;
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#define IMPLEMENT_MESSAGE_FILTER( FILTERCLASS ) \
FILTERCLASS FILTERCLASS::__##FILTERCLASS##_Object__;
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
class PickFilter : public Filter
{
	DEFINE_MESSAGE_FILTER( DataModel::Pick::ClassName(), PickFilter )

public:
	virtual void init(Client::Application& app)
	{
		try
		{
			_statusStr = app.configGetString("filter.pick.status");
			if (!Core::fromString(_status, _statusStr))
				if (_statusStr != wildcardStr)
					return;

			_phase = app.configGetString("filter.pick.phase");

			_agencyID = app.configGetString("filter.pick.agencyID");

			_networkCode = app.configGetString("filter.pick.networkCode");

			enabled = true;
			// SEISCOMP_DEBUG("PickFilter: enabled");
		}
		catch(Config::Exception& e)
		{
			std::cerr << "PickFilter: could not find option: " << e.what() << " [Filter disabled]" << std::endl;
		}
	}

	virtual bool filterImpl(Core::BaseObject* object)
	{
		// std::cout << "Hello, I am a PickFilter" << std::endl;
		DataModel::Pick* pick = DataModel::Pick::Cast(object);
		if (!pick)
			return false;

		SEISCOMP_DEBUG("ClassName: %s Status: %s Phase: %s AgencyID: %s",
		               pick->className(), Core::toString(pick->evaluationMode()).c_str(),
		               pick->phaseHint().code().c_str(), objectAgencyID(pick).c_str());

		if ( _statusStr != wildcardStr )
			if ( _status != pick->evaluationMode() )
				return false;
		if ( _phase != wildcardStr )
			if ( _phase != pick->phaseHint().code() )
				return false;
		if ( _agencyID != wildcardStr )
			if (_agencyID != objectAgencyID(pick))
				return false;
		if ( _networkCode != wildcardStr )
			if ( _networkCode != pick->waveformID().networkCode() )
				return false;
		return true;
	}

private:
	std::string               _statusStr;
	DataModel::EvaluationMode _status;
	std::string               _phase;
	std::string               _agencyID;
	std::string               _networkCode;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
class AmplitudeFilter : public Filter
{
	DEFINE_MESSAGE_FILTER( DataModel::Amplitude::ClassName(), AmplitudeFilter )

public:
	virtual void init(Client::Application& app)
	{
		try
		{
			std::string op = app.configGetString("filter.amplitude.operator");
			_operator = GetOperator(op);
			if (_operator == INVALID)
			{
				SEISCOMP_ERROR("Operator: %s is invalid", op.c_str());
				return;
			}

			std::string amplitudeStr = app.configGetString("filter.amplitude.amplitude");
			if (amplitudeStr == wildcardStr)
				_operator = ANY;
			else
				if (!Core::fromString(_amplitude, amplitudeStr))
					return;

			_agencyID = app.configGetString("filter.amplitude.agencyID");

			enabled = true;
			// SEISCOMP_DEBUG("AmplitudeFilter: enabled");
		}
		catch (Config::Exception& e)
		{
			std::cerr << "AmplitudeFilter: could not find option: " << e.what() << " [Filter disabled]" << std::endl;
		}
	}


	virtual bool filterImpl(Core::BaseObject* object)
	{
		// SEISCOMP_DEBUG("Hello, I am a AmplitudeFilter");
		DataModel::Amplitude* amplitude = DataModel::Amplitude::Cast(object);
		if (!amplitude)
			return false;

		if (_agencyID != wildcardStr)
			if (_agencyID != objectAgencyID(amplitude))
				return false;

		SEISCOMP_DEBUG("Amplitude: amplitude: %f", (double)amplitude->amplitude());
		switch (_operator)
		{
			case EQ:
			if (_amplitude == amplitude->amplitude())
				return true;
			break;
			case LT:
			if (_amplitude <= amplitude->amplitude())
				return true;
			break;
			case GT:
			if (_amplitude >= amplitude->amplitude())
				return true;
			break;
			case ANY:
			return true;
			break;
			default:
			break;
		}

		return false;
	}

private:
	Operator _operator;
	double _amplitude;
	std::string _agencyID;

};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
class OriginFilter : public Filter
{
	DEFINE_MESSAGE_FILTER( DataModel::Origin::ClassName(), OriginFilter )

public:
	virtual void init(Client::Application& app)
	{
		try
		{
			std::vector<std::string> tokens;

			// latitude
			_latitude = app.configGetString("filter.origin.latitude");
			if (Core::split(tokens, _latitude.c_str(), ":") == 2)
			{
				if (!Core::fromString(_lat0, tokens[0]))
					return;
				if (!Core::fromString(_lat1, tokens[1]))
					return;
			}
			else
			{
				if (_latitude != wildcardStr)
					return;
			}

			// longitude
			tokens.clear();
			_longitude = app.configGetString("filter.origin.longitude");
			if (Core::split(tokens, _longitude.c_str(), ":") == 2)
			{
				if (!Core::fromString(_lon0, tokens[0]))
					return;
				if (!Core::fromString(_lon1, tokens[1]))
					return;
			}
			else
			{
				if (_longitude != wildcardStr)
					return;
			}

			// depth
			tokens.clear();
			_depth = app.configGetString("filter.origin.depth");
			if (Core::split(tokens, _depth.c_str(), ":") == 2)
			{
				if (!Core::fromString(_depth0, tokens[0]))
					return;
				if (!Core::fromString(_depth1, tokens[1]))
					return;
			}
			else
			{
				if (_depth != wildcardStr)
					return;
			}

			// agencyId
			_agencyID = app.configGetString("filter.origin.agencyID");

			// status
			_statusStr = app.configGetString("filter.origin.status");
			if (!Core::fromString(_status, _statusStr))
				if (_statusStr != wildcardStr)
					return;

			// phases
			_arrivalCountStr = app.configGetString("filter.origin.arrivalcount");
			if (!Core::fromString(_arrivalCount, _arrivalCountStr))
				if (_statusStr != wildcardStr)
					return;

			enabled = true;
			// SEISCOMP_DEBUG("OriginFilter: enabled");
		}
		catch(Config::Exception& e)
		{
			std::cerr << "OriginFilter: could not find option: " << e.what() << " [Filter disabled]" << std::endl;
		}
	}

	virtual bool filterImpl(Core::BaseObject* object)
	{
		// SEISCOMP_DEBUG("Hello, I am a OriginFilter");
		DataModel::Origin* origin = DataModel::Origin::Cast(object);
		if (!origin) {
			DataModel::Arrival* arrival = DataModel::Arrival::Cast(object);
			if ( arrival )
				return true;
			return false;
		}

		if (_latitude != wildcardStr)
			if (origin->latitude() < _lat0 || origin->latitude() > _lat1)
				return false;

		if (_longitude != wildcardStr)
			if (origin->longitude() < _lon0 || origin->longitude() > _lon1)
				return false;

		if (_depth != wildcardStr)
			if (origin->depth() < _depth0 || origin->depth() > _depth1)
				return false;

		if (_agencyID != wildcardStr)
			if (_agencyID != objectAgencyID(origin))
				return false;

		if (_statusStr != wildcardStr)
			if (origin->evaluationMode() != _status)
				return false;

		SEISCOMP_DEBUG("ArrivalCount: %lu", (unsigned long)origin->arrivalCount());
		if (_arrivalCountStr != wildcardStr)
			if (origin->arrivalCount() < _arrivalCount)
				return false;

		return true;
	}

private:
	std::string               _latitude;
	double                    _lat0, _lat1;

	std::string               _longitude;
	double                    _lon0, _lon1;

	std::string               _depth;
	double                    _depth0, _depth1;

	std::string               _agencyID;

	std::string               _statusStr;
	DataModel::EvaluationMode _status;

	std::string               _arrivalCountStr;
	size_t                    _arrivalCount;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
class EventFilter : public Filter
{
	DEFINE_MESSAGE_FILTER( DataModel::Event::ClassName(), EventFilter )

public:
	virtual void init(Client::Application& app)
	{
		try
		{
			_eventTypeStr = app.configGetString("filter.event.type");
			if (!Core::fromString(_eventType, _eventTypeStr))
				if (_eventTypeStr != wildcardStr)
					return;

			enabled = true;
			// SEISCOMP_DEBUG("EventFilter: enabled");
		}
		catch (Config::Exception& e)
		{
			std::cerr << "EventFilter: could not find option: " << e.what() << " [Filter disabled]" << std::endl;
		}
	}

	virtual bool filterImpl(Core::BaseObject* object)
	{
		// SEISCOMP_DEBUG("Hello, I am a EventFilter");
		DataModel::Event* event = DataModel::Event::Cast(object);
		if (!event)
			return false;

		if (_eventTypeStr != wildcardStr)
			if (_eventType != event->type())
				return false;

		return true;
	}

	std::string          _eventTypeStr;
	DataModel::EventType _eventType;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
class StationMagnitudeFilter : public Filter
{
	DEFINE_MESSAGE_FILTER( DataModel::StationMagnitude::ClassName(), StationMagnitudeFilter )

public:
	virtual void init(Client::Application& app)
	{
		try
		{
			_typeStr = app.configGetString("filter.stationMagnitude.type");
			enabled = true;
			// SEISCOMP_DEBUG("StationMagnitudeFilter: enabled");
		}
		catch (Config::Exception& e)
		{
			std::cerr << "StationMagnitudeFilter: could not find option: " << e.what() << " [Filter disabled]" << std::endl;
		}
	}

	virtual bool filterImpl(Core::BaseObject* object)
	{
		// SEISCOMP_DEBUG("Hello, I am a StationMagnitudeFilter");
		DataModel::StationMagnitude* magnitude = DataModel::StationMagnitude::Cast(object);
		if (!magnitude)
			return false;

		if (_typeStr != wildcardStr)
			if (_typeStr!= magnitude->type())
				return false;

		return true;
	}

private:
	std::string          _typeStr;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
class MagnitudeFilter : public Filter
{
	DEFINE_MESSAGE_FILTER( DataModel::Magnitude::ClassName(), MagnitudeFilter )

public:
	virtual void init(Client::Application& app)
	{
		try
		{
			_typeStr = app.configGetString("filter.magnitude.type");
			enabled = true;
			// SEISCOMP_DEBUG("MagnitudeFilter: enabled");
		}
		catch (Config::Exception& e)
		{
			std::cerr << "MagnitudeFilter: could not find option: " << e.what() << " [Filter disabled]" << std::endl;
		}
	}

	virtual bool filterImpl(Core::BaseObject* object)
	{
		// SEISCOMP_DEBUG("Hello, I am a MagnitudeFilter");
		DataModel::Magnitude* magnitude = DataModel::Magnitude::Cast(object);
		if (!magnitude)
			return false;

		if (_typeStr != wildcardStr)
			if (_typeStr != magnitude->type())
				return false;

		return true;
	}

private:
	std::string          _typeStr;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
class WaveformQualityFilter : public Filter
{
	DEFINE_MESSAGE_FILTER( DataModel::WaveformQuality::ClassName(), WaveformQualityFilter )

public:
	virtual void init(Client::Application& app)
	{
		try
		{
			_typeStr = app.configGetString("filter.qc.type");
			if (!Core::fromString(_type, _typeStr))
				if (_typeStr != wildcardStr)
					return;

			enabled = true;
			// SEISCOMP_DEBUG("WaveformQualityFilter: enabled");
		}
		catch (Config::Exception& e)
		{
			std::cerr << "WaveformQulaityFilter: could not find option: " << e.what() << " [Filter disabled]" << std::endl;
		}
	}

	virtual bool filterImpl(Core::BaseObject* object)
	{
		// SEISCOMP_DEBUG("Hello, I am a WaveformQulaityFilter");
		DataModel::WaveformQuality* wq = DataModel::WaveformQuality::Cast(object);
		if (!wq)
			return false;

		if (_typeStr != wildcardStr)
			if (_type != wq->type())
				return false;

		return true;
	}
private:
	std::string       _typeStr;
	std::string       _type;
};
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



} // namespace Applications
} // namespace Seiscomp

#endif
