/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#include <duds/hardware/devices/instruments/FXOS8700CQ.hpp>
#include <duds/hardware/interface/linux/DevI2c.hpp>
#include <iostream>
#include <thread>
#include <iomanip>
#include <cmath>
#include <boost/exception/diagnostic_information.hpp>
//#include <Eigen/Geometry>

bool quit = false;

void runtest(duds::hardware::devices::instruments::FXOS8700CQ &accelmag)
try {
	//const Eigen::Vector3f z(0, 0, 1);
	duds::hardware::devices::instruments::FXOS8700CQ::RawSample rsA, rsM;
	accelmag.start();
	std::cout.precision(1);
	std::cout << std::fixed;
	// doesn't account for time spent in loop
	std::chrono::milliseconds delay = std::chrono::milliseconds(
		(int)(1000.0f / accelmag.sampleRate())
	);
	std::this_thread::sleep_for(delay);
	do {
		if (!accelmag.sample()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(8));
		}
		rsA = accelmag.rawAccelerometer();
		rsM = accelmag.rawMagnetometer();
		std::cout << "Accel: " << // std::setw(8) << g.norm() << ' ' <<
		std::setw(6) << rsA.x << ", " <<
		std::setw(6) << rsA.y << ", " << std::setw(6) << rsA.z;
		/*  Attempt at finding angle between Z-axis and the acceleration
		    vector, which is used as the gravity vector.
		    Removed for now to remove dependency on Eigen.
		Eigen::Vector3f m(rsM.x, rsM.y, rsM.z);
		Eigen::Vector3f g(rsA.x, rsA.y, rsA.z);
		g.normalize();
		float dot = g.dot(z);
		// dot prod & mag of cross prod could be used to avoid acos(), cos(), sin()
		float th = std::acos(dot);
		Eigen::Vector3f c = z.cross(g);
		//float th = std::asin(c.norm());
		std::cout << " theta " << std::setw(5) << (th * 180.0f / M_PI);
		//std::cout << " dot " << std::setw(10) << dot;
		c.normalize();  // can't go on line above
		//Eigen::Quaternion<float> q(th, c(0), c(1), c(2));
		Eigen::Matrix3f t(Eigen::AngleAxisf(th, c));
		Eigen::Vector3f mT;
		mT = t * m;
		/ *  commented out when Eigen is in use because the output is not
		     yet useful
		std::cout << "   Mag: " << std::setw(7) << mT(0) << ", " <<
		std::setw(7) << mT(1) << ", " << std::setw(7) << mT(2) << '\r';// std::endl;
		std::cout.flush();
		* /
		std::cout << "   Mag: " << std::setw(7) << m.norm() << ' ' <<
		*/
		std::cout << "  " <<  // remove this line when restoring Eigen
		std::setw(5) << rsM.x << ", " <<
		std::setw(5) << rsM.y << ", " << std::setw(5) << rsM.z << " \r";// std::endl;
		std::cout.flush();
		std::this_thread::sleep_for(delay);
	} while (!quit);
} catch (...) {
	std::cerr << "Program failed in runtest(): " <<
	boost::current_exception_diagnostic_information() << std::endl;
}

const duds::hardware::devices::instruments::FXOS8700CQ::Settings config = {
	/* .accelerometer = */ 1,
	/* .magnetometer = */ 1,
	/* .accelLowNoise = */ 1,
	/* .highPassFilter = */ 0,
	0,
	/* .maxMagnitude = */ duds::hardware::devices::instruments::FXOS8700CQ::Magnitude2g,
	/* .oversampleMode = */ duds::hardware::devices::instruments::FXOS8700CQ::HighResolution,
	/* .oversampleSleepMode = */ duds::hardware::devices::instruments::FXOS8700CQ::LowPower,
	/* .oversampleRatio = */ 7,
	0
};

int main(int argc, char *argv[])
try {
	std::unique_ptr<duds::hardware::interface::I2c> i2c(
		new duds::hardware::interface::linux::DevI2c(
			argc > 1 ? argv[1] : "/dev/i2c-1",
			0x1F
		)
	);
	duds::hardware::devices::instruments::FXOS8700CQ accelmag(i2c);
	accelmag.configure(4.0f, config);
	std::cout.precision(4);
	std::cout << "Sampling frequency reported as " << accelmag.sampleRate() <<
	"Hz" << std::endl;
	std::thread doit(runtest, std::ref(accelmag));
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
