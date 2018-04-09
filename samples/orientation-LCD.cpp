/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/LSM9DS1.hpp>
#include <duds/hardware/interface/linux/DevI2c.hpp>
#include <duds/hardware/interface/I2cErrors.hpp>
#include <duds/hardware/devices/displays/HD44780.hpp>
#include <duds/hardware/devices/displays/TextDisplayStream.hpp>
#include <duds/hardware/interface/linux/SysFsPort.hpp>
#include <duds/hardware/interface/ChipPinSelectManager.hpp>
#include <duds/general/IntegerBiDirIterator.hpp>
#include <iostream>
#include <thread>
#include <iomanip>
#include <cmath>
#include <boost/exception/diagnostic_information.hpp>
#include <Eigen/Geometry>
#include <boost/program_options.hpp>

bool quit = false;

namespace displays = duds::hardware::devices::displays;


// https://www.hindawi.com/journals/js/2010/967245/
// https://github.com/kriswiner/MPU6050/wiki/Simple-and-Effective-Magnetometer-Calibration

// ----------------------------------------------------------------------------
// simplistic calibration

const duds::hardware::devices::instruments::LSM9DS1::Settings calconfig = {
	/* .accelerometer = */ 0,
	/* .gyroscope = */ 0,
	/* .magnetometer = */ 1,
	/* .accelRange = */ duds::hardware::devices::instruments::LSM9DS1::AccelRange2g,
	/* .gyroRange = */ duds::hardware::devices::instruments::LSM9DS1::GyroRange4p276rps,
	/* .magRange = */ duds::hardware::devices::instruments::LSM9DS1::MagRange400uT,
	/* .gyroLowPower = */ 1,
	/* .gyroHighPass = */ 0,
	/* .magLowPower = */ 0,
	/* .xyMagMode = */ duds::hardware::devices::instruments::LSM9DS1::AxesHighPerformance,
	/* .zMagMode = */ duds::hardware::devices::instruments::LSM9DS1::AxesHighPerformance,
	/* .magTempComp = */ 0
};

void runcalibrationtest(
	duds::hardware::devices::instruments::LSM9DS1 &acclgyromag,
	displays::TextDisplayStream &tds,
	Eigen::Vector3d bias,
	Eigen::Vector3d scale
)
try {
	//std::vector<Eigen::Vector3d> magvecs(8000);
	Eigen::Vector3d minS(256, 256, 256), maxS(-256, -256, -256);
	duds::hardware::devices::instruments::LSM9DS1::ConvertedSample sample;
	std::chrono::milliseconds delay = std::chrono::milliseconds(8);
	acclgyromag.configure(0, 80.0f, calconfig);
	std::cout << "Begin calibration" << std::endl;
	tds << "Begin calibration\n";
	std::this_thread::sleep_for(std::chrono::seconds(2));
	std::cout << "\tnow!" << std::endl;
	tds << "  now!\n";
	for (int pos = 0; pos < 8192; ++pos) {
		while (1) {
			try {
				while (!acclgyromag.sample()) {
					std::this_thread::sleep_for(std::chrono::milliseconds(2));
				}
				break;
			} catch (duds::hardware::interface::I2cErrorNoDevice &) {
				acclgyromag.configure(0, 80.0f, calconfig);
				std::this_thread::sleep_for(std::chrono::milliseconds(32));
			}
		}
		acclgyromag.magnetometerQuantity(sample);
		//magvecs[pos](0) = sample.x();
		//magvecs[pos](1) = sample.y();
		//magvecs[pos](2) = sample.z();
		if (sample.x() > maxS(0)) {
			maxS(0) = sample.x();
		}
		if (sample.y() > maxS(1)) {
			maxS(1) = sample.y();
		}
		if (sample.z() > maxS(2)) {
			maxS(2) = sample.z();
		}
		if (sample.x() < minS(0)) {
			minS(0) = sample.x();
		}
		if (sample.y() < minS(1)) {
			minS(1) = sample.y();
		}
		if (sample.z() < minS(2)) {
			minS(2) = sample.z();
		}
		if ((pos % 256) == 0) {
			tds << "Sample " << pos << '\r';
			std::cout << "Sample " << pos << '\r';
			std::cout.flush();
		}
		std::this_thread::sleep_for(delay);
	}
	tds << displays::clear;
	bias = (maxS + minS) / 2.0;
	scale = (maxS - minS) / 2.0;
	double avgscl = (scale(0) + scale(1) + scale(2)) / 3.0;
	scale = scale.cwiseInverse() * avgscl;
	std::cout << "Calibration complete\n\tbias: " <<
	bias(0) << ", " << bias(1) << ", " << bias(2) << "\n\tscale: " <<
	scale(0) << ", " << scale(1) << ", " << scale(2) << std::endl;
} catch (...) {
	std::cerr << "Program failed in runcalibrationtest(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
}

// ----------------------------------------------------------------------------

void makeHorizontal(
	Eigen::Vector3d &res,
	const Eigen::Vector3d &grav,
	const Eigen::Vector3d &mod,
	float &angle
) {
	static const Eigen::Vector3d z(0, 0, 1);
	Eigen::Vector3d g(grav.normalized());
	angle = std::acos(g.dot(z));
	Eigen::Vector3d axis = g.cross(z);
	axis.normalize();
	Eigen::Quaternion<double> q(Eigen::AngleAxisd(angle, axis));
	res = q * mod;
}

float heading(const Eigen::Vector3d &dir) {
	/*
	static const Eigen::Vector3d fwd(0, 1, 0);
	Eigen::Vector3d d(dir(0), dir(1), 0);
	d.normalize();
	Eigen::Vector3d a = d.cross(fwd);
	return std::asin(a.norm());
	*/
	float h = std::atan2(dir(0), dir(1));
	if (h < 0) {
		h += 2 * M_PI;
	}
	return h;
}


std::ostream &operator << (std::ostream &os, const Eigen::Vector3d &v) {
	Eigen::Vector3d n(v.normalized());
	n *= 99.0f;
	os << std::fixed << std::setprecision(0) << std::setw(3) << n(0) << ' ' <<
	std::setw(3) << n(1) << ' ' << std::setw(3) << n(2);
	return os;
}

void runtest(
	duds::hardware::devices::instruments::LSM9DS1 &acclgyromag,
	displays::TextDisplayStream &tds,
	const Eigen::Vector3d &bias,
	const Eigen::Vector3d &scale,
	int htx, int hty, int htz
)
try {
	static const Eigen::Vector3f z(0, 0, 1), htest(htx, hty, htz);
	duds::hardware::devices::instruments::LSM9DS1::RawSample rsA, rsM;
	duds::hardware::devices::instruments::LSM9DS1::ConvertedSample csA, csM;
	acclgyromag.start();
	std::cout << std::fixed;
	// doesn't account for time spent in loop
	std::chrono::milliseconds delay = std::chrono::milliseconds(
		(int)(1000.0f / /*acclgyromag.sampleRate()*/ 5.0f)
	);
	std::this_thread::sleep_for(delay);
	do {
		try {
			while (!acclgyromag.sample() && !quit) {
				std::this_thread::sleep_for(std::chrono::milliseconds(4));
			}
		} catch (...) {
			std::cerr << "Sample failure in runtest(): " <<
			boost::current_exception_diagnostic_information() << std::endl;
		}
		rsA = acclgyromag.rawAccelerometer();
		rsM = acclgyromag.rawMagnetometer();
		acclgyromag.accelerometerQuantity(csA);
		acclgyromag.magnetometerQuantity(csM);
		//Eigen::Vector3f mF(rsM.x, rsM.y, rsM.z);
		Eigen::Vector3d m(csM.x(), csM.y(), csM.z()), tmpM;
		tmpM = (m - bias); // .cwiseProduct(scale);
		m = tmpM.cwiseProduct(scale);
		//Eigen::Vector3f g(rsA.x, rsA.y, rsA.z);
		Eigen::Vector3d g(csA.x(), csA.y(), csA.z());
		Eigen::Vector3d mT;
		std::cout << "A: " << std::fixed << std::setprecision(4) << std::setw(8) << g.norm() << ' ' <<
		std::setw(6) << csA.x() << ", " <<
		std::setw(6) << csA.y() << ", " << std::setw(6) << csA.z();
		float th;
		makeHorizontal(mT, g, m, th);
		float head = heading(/*mT*/ m);
		std::cout << " th " << std::setw(5) << (th * 180.0f / M_PI);
		// magnetometer vector modified to have X-Y plane perpindicular to the
		// gravity vector
		std::cout << "   M: " << std::scientific << std::setprecision(6) << std::setw(9) << mT.norm() << ' ' <<
		std::setw(9) << mT(0) << ", " << std::setw(9) << mT(1) << ", " <<
		std::setw(9) << mT(2) << "  h: " << std::setw(5) << std::setprecision(1) << std::fixed <<
		(head * 180.0f / M_PI) << "  \r";// std::endl;
		/* unmodified magnetometer vector
		std::cout << "   Mag: " << std::setw(7) << m.norm() << ' ' <<
		std::setw(5) << rsM.x << ", " <<
		std::setw(5) << rsM.y << ", " << std::setw(5) << rsM.z << " \r";// std::endl;
		*/
		std::cout.flush();

		//float bozo;
		//makeHorizontal(mT, g, htest, bozo);

		// LCD out
		tds << displays::move(0,0) << "Up:" << std::fixed << std::setprecision(0) << std::setw(4) <<
		(th * 180.0f / M_PI) << " H:" << std::setprecision(0) << std::setw(4) <<
		(head * 180.0f / M_PI) << ',' << std::setprecision(0) << std::setw(4) <<
		(heading(mT) * 180.0f / M_PI) << displays::startLine << "Grav    " <<
		g << displays::startLine << "Mag     " << m << displays::startLine <<
		"Mag mod " << mT << displays::startLine;
		//"Testvec " << mT << displays::startLine;

		//tds << displays::move(0, 0);
		std::this_thread::sleep_for(delay);
	} while (!quit);
} catch (...) {
	std::cerr << "Program failed in runtest(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
}

const duds::hardware::devices::instruments::LSM9DS1::Settings config = {
	/* .accelerometer = */ 1,
	/* .gyroscope = */ 0,
	/* .magnetometer = */ 1,
	/* .accelRange = */ duds::hardware::devices::instruments::LSM9DS1::AccelRange2g,
	/* .gyroRange = */ duds::hardware::devices::instruments::LSM9DS1::GyroRange4p276rps,
	/* .magRange = */ duds::hardware::devices::instruments::LSM9DS1::MagRange400uT,
	/* .gyroLowPower = */ 1,
	/* .gyroHighPass = */ 0,
	/* .magLowPower = */ 0,
	/* .xyMagMode = */ duds::hardware::devices::instruments::LSM9DS1::AxesHighPerformance,
	/* .zMagMode = */ duds::hardware::devices::instruments::LSM9DS1::AxesHighPerformance,
	/* .magTempComp = */ 0
};

typedef duds::general::IntegerBiDirIterator<unsigned int>  uintIterator;

int main(int argc, char *argv[])
try {
	std::string i2cpath;
	int htx = 1, hty = 0, htz = 0;
	{ // option parsing
		boost::program_options::options_description optdesc("Options");
		optdesc.add_options()
			( // help info
				"help,h",
				"Show this help message"
			)
			( // the I2C device file to use
				"i2cdev,i",
				boost::program_options::value<std::string>(&i2cpath)->
					default_value("/dev/i2c-1"),
				"Specify I2C device file"
			)
			(",x",boost::program_options::value<int>(&htx),"X test vec")
			(",y",boost::program_options::value<int>(&hty),"Y test vec")
			(",z",boost::program_options::value<int>(&htz),"Z test vec")
			//("bx",boost::program_options::value<int>(&htz),"X bias")
		;
		boost::program_options::variables_map vm;
		boost::program_options::store(
			boost::program_options::parse_command_line(argc, argv, optdesc),
			vm
		);
		boost::program_options::notify(vm);
		if (vm.count("help")) {
			std::cout << "Accelerometer and magnetometer test\n" << argv[0]
			<< " [options]\n" << optdesc << std::endl;
			return 0;
		}
	}

	// Setup for LSM9DS1
	std::unique_ptr<duds::hardware::interface::I2c> magI2c(
		new duds::hardware::interface::linux::DevI2c(i2cpath, 0x1E)
	);
	std::unique_ptr<duds::hardware::interface::I2c> accelI2c(
		new duds::hardware::interface::linux::DevI2c(i2cpath, 0x6B)
	);
	duds::hardware::devices::instruments::LSM9DS1 acclgyromag(accelI2c, magI2c);

	// Setup for LCD
	//                       LCD pins:  4  5   6   7  RS   E
	std::vector<unsigned int> gpios = { 5, 6, 19, 26, 20, 21 };
	std::shared_ptr<duds::hardware::interface::linux::SysFsPort> port =
		std::make_shared<duds::hardware::interface::linux::SysFsPort>(
			gpios, 0
		);
	// select pin
	std::unique_ptr<duds::hardware::interface::DigitalPinAccess> selacc =
		port->access(5); // gpio 21
	std::shared_ptr<duds::hardware::interface::ChipPinSelectManager> selmgr =
		std::make_shared<duds::hardware::interface::ChipPinSelectManager>(
			selacc
		);
	assert(!selacc);
	duds::hardware::interface::ChipSelect lcdsel(selmgr, 1);
	// set for LCD data
	gpios.clear();
	gpios.insert(gpios.begin(), uintIterator(0), uintIterator(5));
	duds::hardware::interface::DigitalPinSet lcdset(port, gpios);
	// display object
	std::shared_ptr<displays::HD44780> tmd =
		std::make_shared<displays::HD44780>(
			lcdset, lcdsel, 20, 4
		);
	tmd->initialize();
	displays::TextDisplayStream tds(tmd);
	
	Eigen::Vector3d bias(-7.147e-06, 4.1335e-05, 5.4474e-05);
	Eigen::Vector3d scale(1.00074, 0.977949, 1.02229);
	runcalibrationtest(acclgyromag, tds, bias, scale);

	acclgyromag.configure(5.0f, 5.0f, config);

	std::cout.precision(4);
	//std::cout << "Sampling frequency reported as " << acclgyromag.sampleRate() <<
	//"Hz" << std::endl;
	std::thread doit(
		runtest,
		std::ref(acclgyromag),
		std::ref(tds),
		std::ref(bias),
		std::ref(scale),
		htx,
		hty,
		htz
	);
	std::cin.get();
	quit = true;
	doit.join();
	std::cout << std::endl;
	return 0;
} catch (...) {
	std::cerr << "Program failed in main(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
