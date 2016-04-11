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


#include <seiscomp3/core/strings.h>
#include <seiscomp3/core/exceptions.h>

#include <seiscomp3/math/filter.h>
#include <seiscomp3/math/filter/chainfilter.h>
#include <seiscomp3/math/filter/abs.h>
#include <seiscomp3/math/filter/const.h>
#include <seiscomp3/math/filter/rca.h>
#include <seiscomp3/math/filter/op2filter.h>
#include <seiscomp3/math/filter/stalta.h>
#include <seiscomp3/math/filter/seismometers.h>
#include <seiscomp3/math/filter/iirintegrate.h>
#include <seiscomp3/math/filter/iirdifferentiate.h>
#include <seiscomp3/math/filter/taper.h>
#include <seiscomp3/math/filter/rmhp.h>
#include <seiscomp3/math/filter/biquad.h>
#include <seiscomp3/math/filter/butterworth.h>
#include <seiscomp3/math/filter/taper.h>

#include <seiscomp3/core/interfacefactory.ipp>

#include <boost/regex.hpp>
#include <boost/spirit.hpp>
#include <boost/spirit/phoenix/binders.hpp>
#include <boost/spirit/error_handling.hpp>

#include <sstream>
#include <string>
#include <numeric>
#include <set>


IMPLEMENT_INTERFACE_FACTORY(Seiscomp::Math::Filtering::InPlaceFilter<float>, SC_CORE_MATH_API);
IMPLEMENT_INTERFACE_FACTORY(Seiscomp::Math::Filtering::InPlaceFilter<double>, SC_CORE_MATH_API);


using namespace std;
using namespace Seiscomp;


namespace {


Math::GroundMotion int2gm(int value)
{
	switch (value) {
		case 0:
			return Math::Displacement;
		case 1:
			return Math::Velocity;
		case 2:
			return Math::Acceleration;
		default:
			break;
	}

	throw Core::TypeConversionException("input out of range");
}

// A production can have an associated closure, to store information
// for that production.
struct LiteralClosure : boost::spirit::closure<LiteralClosure, double> {
	member1 value;
};

template <typename T>
struct ValueClosure : boost::spirit::closure<ValueClosure<T>, T> {
	typename boost::spirit::closure<ValueClosure<T>, T>::member1 value;
};

template <typename T>
struct FilterClosure : boost::spirit::closure<FilterClosure<T>, T, string> {
	typename boost::spirit::closure<ValueClosure<T>, T, string>::member1 value;
	typename boost::spirit::closure<ValueClosure<T>, T, string>::member2 name;
};

struct StringClosure : boost::spirit::closure<StringClosure, string> {
	member1 name;
};


template <class _Tp>
struct power : public binary_function<_Tp, _Tp, _Tp> {
	_Tp
	operator()(const _Tp& __x, const _Tp& __y) const {
		return ::pow(__x, __y);
	}
};


template <typename T> struct Parser;

template <typename ParserT>
struct Generator {
	typedef typename ParserT::component_type component_type;
	typedef typename ParserT::value_type value_type;

	Generator(ParserT &p)
	 : parser(p) {}

	value_type constant(double f) const {
		return save(new Math::Filtering::ConstFilter<component_type>(static_cast<component_type>(f)));
	}

	value_type abs(value_type f) const {
		return chain(f, save(new Math::Filtering::AbsFilter<component_type>()));
	}

	value_type add(value_type a, value_type b) const {
		free(a); free(b);
		return save(new Math::Filtering::Op2Filter<component_type, plus>(a, b));
	}

	value_type sub(value_type a, value_type b) const {
		free(a); free(b);
		return save(new Math::Filtering::Op2Filter<component_type, minus>(a, b));
	}

	value_type mul(value_type a, value_type b) const {
		free(a); free(b);
		return save(new Math::Filtering::Op2Filter<component_type, multiplies>(a, b));
	}

	value_type div(value_type a, value_type b) const {
		free(a); free(b);
		return save(new Math::Filtering::Op2Filter<component_type, divides>(a, b));
	}

	value_type pow(value_type f, value_type p) const {
		free(f); free(p);
		return save(new Math::Filtering::Op2Filter<component_type, power>(f, p));
	}

	value_type chain(value_type f, value_type p) const {
		free(f); free(p);
		Math::Filtering::ChainFilter<component_type> *cf = new Math::Filtering::ChainFilter<component_type>();
		cf->add(f);
		cf->add(p);
		return save(cf);
	}

	value_type create(const string &filterType) const {
		value_type f = NULL;
		typename ParserT::parameter_list &parameters = parser.parameters;

		if ( filterType == "self" )
			f = new Math::Filtering::SelfFilter<component_type>();
		else if ( filterType == "BW" )
			f = new Math::Filtering::IIR::ButterworthBandpass<component_type>();
		else if ( filterType == "BW_LP" )
			f = new Math::Filtering::IIR::ButterworthLowpass<component_type>();
		else if ( filterType == "BW_HP" )
			f = new Math::Filtering::IIR::ButterworthHighpass<component_type>();
		else if ( filterType == "AC" || filterType == "AVG" )
			f = new Math::Filtering::Average<component_type>();
		else if ( filterType == "STALTA" )
			f = new Math::Filtering::STALTA<component_type>();
		else if ( filterType == "INT" )
			f = new Math::Filtering::IIRIntegrate<component_type>();
		else if ( filterType == "DIFF" )
			f = new Math::Filtering::IIRDifferentiate<component_type>();
		else if ( filterType == "WWSSN_SP" )
			f = new Math::Filtering::IIR::WWSSN_SP_Filter<component_type>();
		else if ( filterType == "WWSSN_LP" )
			f = new Math::Filtering::IIR::WWSSN_LP_Filter<component_type>();
		else if ( filterType == "WA" )
			f = new Math::Filtering::IIR::WoodAndersonFilter<component_type>();
		else if ( filterType == "SM5" )
			f = new Math::Filtering::IIR::Seismometer5secFilter<component_type>();
		else if ( filterType == "ITAPER" )
			f = new Math::Filtering::InitialTaper<component_type>();
		else if ( filterType == "RMHP" )
			f = new Math::Filtering::RunningMeanHighPass<component_type>();
		else {
			f = Math::Filtering::InPlaceFilterFactory<component_type>::Create(filterType.c_str());
			if ( f == NULL ) {
				parameters.clear();
				parser.error_message = "unknown filter '" + filterType + "'";
				return NULL;
			}
		}

		size_t result = f->setParameters(parameters.size(), &parameters[0]);
		if ( result != parameters.size() ) {
			parameters.clear();
			delete f;

			stringstream ss;
			if ( result >= 0 )
				ss << "filter '" << filterType << "' takes " << result << " parameters";
			else
				ss << "wrong parameter at position " << (-result) << " for filter '" << filterType << "'";

			parser.error_message = ss.str();
			return NULL;
		}

		parameters.clear();

		return save(f);
	}

	value_type save(value_type v) const {
		parser.tmp.insert(v);
		return v;
	}

	void free(value_type v) const {
		parser.tmp.erase(v);
	}

	ParserT &parser;
};


template <typename T>
struct Parser : boost::spirit::grammar< Parser<T> >
{
	typedef T component_type;
	typedef Math::Filtering::InPlaceFilter<component_type> *value_type;
	typedef set<value_type> tmp_list;
	typedef vector<double> parameter_list;
	typedef Generator< Parser<T> > generator_type;

	// The parser object is copied a lot, so instead of keeping its own table
	// of variables, it keeps track of a reference to a common table.
	Parser(value_type &res, parameter_list &params, tmp_list &t, string &err)
	 : result(res), parameters(params), tmp(t), error_message(err), generator(*this) {
		result = NULL;
		parameters.clear();
	}

	struct Handler {
		template <typename ScannerT, typename ErrorT>
		boost::spirit::error_status<>
		operator()(ScannerT const& /*scan*/, ErrorT const& /*error*/) const {
			return boost::spirit::error_status<> (boost::spirit::error_status<>::fail);
		}
	};

	template <typename ParserT>
	struct ErrorCheck {
		ErrorCheck(ParserT const &p) : parser(p) {}

		template <typename Iterator>
		void operator()(Iterator first, Iterator last) const {
			if ( !parser.error_message.empty() )
				boost::spirit::throw_(first, (int)-1);
		}

		ParserT const &parser;
	};

	// Following is the grammar definition.
	template <typename ScannerT>
	struct definition {
		definition(Parser const& self) {
			using namespace boost::spirit;
			using namespace phoenix;

			identifier
			= lexeme_d
			  [
			      alpha_p
			      >> *(alnum_p | '_')
			  ][identifier.name = construct_<string>(arg1, arg2)]
			  ;

			// The longest_d directive is built-in to tell the parser to make
			// the longest match it can. Thus "1.23" matches real_p rather than
			// int_p followed by ".23".
			literal
			= longest_d
			  [
			      int_p[literal.value = arg1]
			      | real_p[literal.value = arg1]
			  ]
			  ;

			group
			= '('
			  >> expression[group.value = arg1]
			  >> ')'
			  ;

			abs
			= '|'
			  >> expression[abs.value = bind(&generator_type::abs)(self.generator, arg1)]
			  >> '|'
			  ;

			filter
			= (
			      identifier[filter.name = arg1]
			      >> ! ( '('
			             >> !list_p.direct(literal[bind(&Parser::add_parameter)(self, arg1)], ',')
			             >> ')'
			           )
			  )
			  [filter.value = bind(&Parser::initialize)(self, filter.name)][ErrorCheck<Parser>(self)]
			  ;

			constant
			= literal[constant.value = bind(&generator_type::constant)(self.generator, arg1)]
			  ;

			// A statement can end at the end of the line, or with a semicolon.
			statement
			= guard<int>()
			  ( expression[bind(&Parser::setResult)(self, arg1)]
			    >> (end_p | ';')
			  ) [Handler()]
			  ;

			value
			= constant[value.value = arg1]
			  | group[value.value = arg1]
			  | filter[value.value = arg1]
			  | abs[value.value = arg1]
			  ;

			factor
			= value[factor.value = arg1]
			  >> * ( '^' >> value[factor.value = bind(&generator_type::pow)(self.generator, factor.value, arg1)])
			  ;

			multiplication
			= factor[multiplication.value = arg1]
			  >> * ( ( '*' >> factor[multiplication.value = bind(&generator_type::mul)(self.generator, multiplication.value, arg1)])
			       | ( '/' >> factor[multiplication.value = bind(&generator_type::div)(self.generator, multiplication.value, arg1)])
			       )
			  ;

			addition
			= multiplication[addition.value = arg1]
			  >> * ( ( '+' >> multiplication[addition.value = bind(&generator_type::add)(self.generator, addition.value, arg1)])
			       | ( '-' >> multiplication[addition.value = bind(&generator_type::sub)(self.generator, addition.value, arg1)])
			       )
			  ;

			expression
			= addition[expression.value = arg1]
			  >> * ( ( str_p ( "->" ) | ">>" ) >> addition[expression.value = bind(&generator_type::chain)(self.generator, expression.value, arg1)])
			  ;
		}

		// value_typehe start symbol is returned from start().
		boost::spirit::rule<ScannerT> const&
		start() const { return statement; }

		// Each rule must be declared, optionally with an associated closure.
		boost::spirit::rule<ScannerT> statement;
		boost::spirit::rule<ScannerT, StringClosure::context_t>  identifier;
		boost::spirit::rule<ScannerT, LiteralClosure::context_t> literal;
		boost::spirit::rule<ScannerT, typename FilterClosure<value_type>::context_t> filter;
		boost::spirit::rule<ScannerT, typename ValueClosure<value_type>::context_t> expression, factor, value,
		constant, group, abs,
		multiplication, addition;
	};

	void add_parameter(double value) const {
		parameters.push_back(value);
	}

	value_type initialize(const string &filterType) const {
		return generator.create(filterType);
	}

	void setResult(value_type x) const {
		generator.free(x);
		result = x;
	}

	value_type &result;
	parameter_list &parameters;
	tmp_list &tmp;
	string &error_message;
	generator_type generator;
};

}


namespace Seiscomp
{
namespace Math
{
namespace Filtering
{


template <typename T>
InPlaceFilter<T> *InPlaceFilter<T>::Create(const string &strFilter, string *error_str) {
	string error;
	typename Parser<T>::parameter_list parameters;
	typename Parser<T>::tmp_list tmp;
	typename Parser<T>::value_type result;
	Parser<T> parser(result, parameters, tmp, error);
	boost::spirit::parse_info<string::const_iterator> info;

	info = boost::spirit::parse(strFilter.begin(), strFilter.end(), parser, boost::spirit::space_p);
	/*
	if ( !info.hit )
		SEISCOMP_ERROR("compile filter: %s", error.c_str());
	*/

	typename Parser<T>::tmp_list::iterator it;
	for ( it = tmp.begin(); it != tmp.end(); ++it )
		delete *it;

	if ( error_str ) *error_str = error;

	return result;
}


template SC_CORE_MATH_API
InPlaceFilter<float> *InPlaceFilter<float>::Create(const string &strFilter, std::string *strError);

template SC_CORE_MATH_API
InPlaceFilter<double> *InPlaceFilter<double>::Create(const string &strFilter, std::string *strError);


}
}
}
