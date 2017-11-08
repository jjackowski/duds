/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>
#include <iostream>
#include <duds/hardware/devices/clocks/Clock.hpp>
#include <duds/data/GenericValueVisitor.hpp>
#include <duds/time/interstellar/Serialize.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <fstream>
#include <sstream>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_serialize.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>

// following needed for Boost 1.55
//#include <duds/general/ArraySerialize-depricate.hpp>

using duds::data::Unit;
using duds::data::Sample;
using duds::data::GenericSample;
using duds::data::CompactSample;
using duds::data::SampleNU;
using duds::data::GenericValue;
using duds::data::Measurement;
using duds::data::GenericMeasurement;
using duds::data::CompactMeasurement;

/*
template <class VT = double, class QT = VT, class TT = duds::time::interstellar::NanoTime>
struct Measurement {
	Sample<TT, QT> time;
	Sample<VT, QT> measured;
};
*/
template <class VT = double, class QT = VT, class TT = duds::time::interstellar::NanoTime>
struct MeasurementTNU {
	SampleNU<TT, QT> time;
	GenericSample<VT, QT> measured;
};
/*
template <class VT = double, class QT = VT, class TT = duds::time::interstellar::NanoTime>
struct CompactMeasurement {
	CompactSample<TT, QT> time;
	CompactSample<VT, QT> measured;
};
*/
struct Qual {
	double accuracy;
	double precision;
	double resolution;
};

template <typename VT>
struct SampoC : public Qual {
	VT value;
	Unit qualityUnit() const {
		// not sure if this is the best way to write a compile-time type check,
		// but optimization should make it a non-issue for the end result
		if (std::is_same<duds::data::Quantity,VT>::value) {
			// for VT == Quantity
			return value.unit;
		} else {
			// for VT == something else
			return duds::data::units::Second;
		}
	}
	duds::data::Quantity getAccuracyQuantity() const {
		return duds::data::Quantity(accuracy, qualityUnit());
	}
};

typedef SampoC<duds::data::Quantity>  Sampo;
typedef SampoC<duds::time::interstellar::NanoTime>  SampoT;
typedef SampoC<duds::time::interstellar::Nanoseconds>  SampoD;

/*
struct Sampo {
	double accuracy;
	double precision;
	double resolution;
	duds::data::Quantity value;
	//std::int32_t flags;
};

template <typename VT>
struct Sampo2 {
	VT accuracy;
	VT precision;
	VT resolution;
	VT value;
	//std::int32_t flags;
};
*/

typedef boost::variant<
	Sampo,
	SampoT,
	SampoD,
	//Sampo2<duds::time::interstellar::Nanoseconds>,
	//Sampo2<duds::time::interstellar::NanoTime>,
	//boost::recursive_wrapper< Sampo2<duds::time::interstellar::FemtoTime> >,
	GenericValue
	//std::array<char,32>
	//boost::recursive_wrapper<GenericValue>
	
	// maybe some kind of error for a failed attempt to read a sensor?
	
>  ValVar;

struct SampleCont {
	boost::uuids::uuid origin;
	ValVar values;
};
struct SampleCont2 {
	boost::uuids::uuid origin;
	GenericValue value;
};
struct SampleCont3 {
	SampleCont sc;
	boost::uuids::uuid origin;
	//Sampo2<duds::time::interstellar::Nanoseconds> value;
	SampoT value;
};
/*
struct SampleCont4 {
	SampleCont sc;
	boost::uuids::uuid origin;
	//Sampo timevalue;
	Sampo2<double> timevalue;
};
*/

int main()
try {
	Unit power = duds::data::units::Watt;
	Unit current = duds::data::units::Ampere;
	Unit voltage = duds::data::units::Volt;
	std::cout << "A,A: " << current.ampere() << "\nV,A: " << voltage.ampere()
		<< "\nV,s: " << voltage.second() << std::endl;
	Unit test = current * voltage;
	assert(test == power);
	std::cout << "Worked." << std::endl;

	std::cout << "Size of Sample: " << sizeof(Sample) <<
	"\nSize of GenericSample<GenericValue,Quantity>: " <<
	sizeof(GenericSample< GenericValue,duds::data::Quantity >) <<
	"\nSize of GenericSample<Femtoseconds,double>: " <<
	sizeof(GenericSample<duds::time::interstellar::Femtoseconds,double>) <<
	"\nSize of GenericSample<Nanoseconds,double>: " <<
	sizeof(GenericSample<duds::time::interstellar::Nanoseconds,double>) <<
	"\nSize of GenericSample<Femtoseconds,Femtoseconds>: " <<
	sizeof(GenericSample<duds::time::interstellar::Femtoseconds, duds::time::interstellar::Femtoseconds>)
	<< "\nSize of GenericSample<GenericValue,double>: " <<
	sizeof(GenericSample<GenericValue,double>) <<
	"\nSize of CompactSample<GenericValue,double>: " <<
	sizeof(CompactSample<GenericValue,double>) <<
	"\nSize of Measurement: " <<
	sizeof(Measurement) <<
	"\nSize of GenericMeasurement<Quantity,double,duds::time::interstellar::Nanoseconds,float>: " <<
	sizeof(GenericMeasurement<duds::data::Quantity,double,duds::time::interstellar::Nanoseconds,float>) <<
	//"\nSize of CompactMeasurement<GenericValue,double>: " <<
	//sizeof(CompactMeasurement<GenericValue,double>) <<
	"\nSize of GenericValue:     " << sizeof(GenericValue)
	//<< "\nSize of array 2: " << sizeof(std::array<double, 2>)
	//<< "\nSize of array 3: " << sizeof(std::array<double, 3>)
	<< "\nSize of Quantity:         " << sizeof(duds::data::Quantity)
	<< "\nSize of ExtendedQuantity: " << sizeof(duds::data::ExtendedQuantity<>)
	<< "\nSize of Sampo:            " << sizeof(Sampo)
	//<< "\nSize of Sampo2:           " << sizeof(Sampo2<duds::time::interstellar::NanoTime>)
	<< "\nSize of SampoT:           " << sizeof(SampoT)
	<< "\nSize of ValVar:           " << sizeof(ValVar)
	<< "\nSize of SampleCont:       " << sizeof(SampleCont)
	<< "\nSize of SampleCont2:      " << sizeof(SampleCont2)
	<< "\nSize of SampleCont3:      " << sizeof(SampleCont3)
	//<< "\nSize of SampleCont4:      " << sizeof(SampleCont4)
	<< "\nSize of Femtoseconds:     " << sizeof(duds::time::interstellar::Femtoseconds)
	/*
	<< "\nSize of time sample:      " << sizeof(duds::time::TimeSample)
	<< "\nSize of nano time sample: " << sizeof(duds::time::NanoTimeSample)
	<< "\nSize of femto time sample:" << sizeof(duds::time::FemtoTimeSample)
	*/
	<< std::endl;

	duds::data::int128_w fs;
	GenericValue sv;
	{
		std::ifstream ifs("femto.xml");
		if (ifs.good()) {
			try {
				boost::archive::xml_iarchive ia(ifs);
				ia >> boost::serialization::make_nvp("time", fs) >>
					boost::serialization::make_nvp("value", sv);
				std::cout << "Read in time: " << std::hex << fs << std::endl;
			} catch (...) {
				std::cout << "Failed to read femto.xml." << std::endl;
			}
		} else {
			std::cout << "Failed to open femto.xml." << std::endl;
		}
	}

	duds::time::interstellar::Femtoseconds now =
		duds::time::interstellar::FemtoClock::now().time_since_epoch();
	fs = now.count();
	sv = now;
	std::cout << "String visit femtos: " << boost::apply_visitor(
		duds::data::GenericValueStringVisitor(), sv) << std::endl;
	sv = now; // fs; //std::string("Yoyodyne");
	{
		std::ofstream ofs("femto.xml");
		/*
		duds::time::interstellar::Milliseconds now =
			duds::time::interstellar::MilliClock::now().time_since_epoch();
		std::int64_t ms = now.count();
		std::ofstream ofs("milli.xml");
		*/
		boost::archive::xml_oarchive oa(ofs);
		oa << boost::serialization::make_nvp("time", fs) <<
			boost::serialization::make_nvp("value", sv);
	}
	std::cout << "out time " << std::hex << fs << std::endl;
	{
		std::ifstream ifs("femto.xml");
		boost::archive::xml_iarchive ia(ifs);
		ia >> boost::serialization::make_nvp("time", fs) >>
			boost::serialization::make_nvp("value", sv);
	}
	std::cout << " in time " << std::hex << fs << std::endl;
	//__int128 ack = 4;
	//std::cout << ack;
	/**/
	std::ostringstream oss;
	{
		sv = duds::data::int128_w(15);
		boost::archive::xml_oarchive oa(oss);
		std::cout << "String visit 0: " << boost::apply_visitor(
			duds::data::GenericValueStringVisitor(), sv) << std::endl;
		oa << boost::serialization::make_nvp("sv0", sv);
		sv = 273.15;
		std::cout << "String visit 1: " << boost::apply_visitor(
			duds::data::GenericValueStringVisitor(), sv) << std::endl;
		oa << boost::serialization::make_nvp("sv1", sv);
		sv = 14L;
		std::cout << "String visit 2: " << boost::apply_visitor(
			duds::data::GenericValueStringVisitor(), sv) << std::endl;
		oa << boost::serialization::make_nvp("sv2", sv);
		sv = duds::time::interstellar::FemtoClock::now();
		std::cout << "String visit 3: " << boost::apply_visitor(
			duds::data::GenericValueStringVisitor(), sv) << std::endl;
		oa << boost::serialization::make_nvp("sv3", sv);
		sv = duds::data::Quantity(5.02, duds::data::units::Volt);
		std::cout << "String visit 4: " << boost::apply_visitor(
			duds::data::GenericValueStringVisitor(), sv) << std::endl;
		oa << boost::serialization::make_nvp("sv4", sv);
		/*
		duds::data::ExtendedUnit eu(duds::data::units::Kelvin);
		eu.offset(273.15f);
		duds::data::ExtendedQuantity<> eq;
		eq.unit = eu;
		eq.value = 22.375;
		sv = eq;
		std::cout << "String visit 5: " << boost::apply_visitor(
			duds::data::GenericValueStringVisitor(), sv) << std::endl;
		oa << boost::serialization::make_nvp("sv5", sv);
		*/
		std::array<double, 2> ad;
		ad[0] = 1.4; ad[1] = 2.8;
		sv = ad;
		std::cout << "String visit A: " << boost::apply_visitor(
			duds::data::GenericValueStringVisitor(), sv) << std::endl;
		oa << boost::serialization::make_nvp("svA", sv);
	}
	std::cout << oss.str() << std::endl;

	duds::data::ExtendedUnit eu;
	float cof = 273.15f;
	eu.offset(cof);
	std::cout << "EU float test: " << cof << ", " << eu.offsetf() << std::endl;
	double cod = 273.15;
	eu.offset(cod);
	std::cout << "EU double test: " << cod << ", " << eu.offset() << std::endl;

	return 0;
}
catch (...) {
	std::cerr << "ERROR: " << boost::current_exception_diagnostic_information()
	<< std::endl;
	return __LINE__;
}
