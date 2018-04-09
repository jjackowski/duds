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
#include <iostream>
#include <thread>
#include <iomanip>
#include <cmath>
#include <boost/exception/diagnostic_information.hpp>
#include <Eigen/Geometry>

bool quit = false;

void makeHorizontal(
	Eigen::Vector3f &res,
	const Eigen::Vector3f &grav,
	const Eigen::Vector3f &mod,
	float &angle
) {
	static const Eigen::Vector3f z(0, 0, 1);
	Eigen::Vector3f g(grav.normalized());
	angle = std::acos(g.dot(z));
	Eigen::Vector3f axis = g.cross(z);
	/*float*/ //angle = std::asin(axis.norm());
	axis.normalize();
	Eigen::Quaternion<float> q(Eigen::AngleAxisf(angle, axis));
	res = q * mod;
}

float heading(const Eigen::Vector3f &dir) {
	/*
	static const Eigen::Vector3f fwd(1, 0, 0);
	Eigen::Vector3f d(dir(0), dir(1), 0);
	d.normalize();
	Eigen::Vector3f a = d.cross(fwd);
	return std::asin(a.norm());
	*/
	float h = std::atan2(dir(2), dir(0));
	if (h < 0) {
		h += 2 * M_PI;
	}
	return h;
}

void runtest(duds::hardware::devices::instruments::LSM9DS1 &acclgyromag)
try {
	static const Eigen::Vector3f z(0, 0, 1);
	duds::hardware::devices::instruments::LSM9DS1::RawSample rsA, rsM;
	acclgyromag.start();
	std::cout.precision(1);
	std::cout << std::fixed;
	// doesn't account for time spent in loop
	std::chrono::milliseconds delay = std::chrono::milliseconds(
		(int)(1000.0f / /*acclgyromag.sampleRate()*/ 2.5f)
	);
	std::this_thread::sleep_for(delay);
	do {
		while (!acclgyromag.sample() && !quit) {
			std::this_thread::sleep_for(std::chrono::milliseconds(8));
		}
		rsA = acclgyromag.rawAccelerometer();
		rsM = acclgyromag.rawMagnetometer();
		Eigen::Vector3f m(rsM.x, rsM.y, rsM.z);
		Eigen::Vector3f g(rsA.x, rsA.y, rsA.z);
		Eigen::Vector3f mT;
		std::cout << "A: " << std::setw(8) << g.norm() << ' ' <<
		std::setw(6) << rsA.x << ", " <<
		std::setw(6) << rsA.y << ", " << std::setw(6) << rsA.z;
		float th;
		makeHorizontal(mT, g, m, th);
		float head = heading(/*mT*/ m);
		std::cout << " th " << std::setw(5) << (th * 180.0f / M_PI);
		// magnetometer vector modified to have X-Y plane perpindicular to the
		// gravity vector
		std::cout << "   M: " << std::setw(7) << mT.norm() << ' ' <<
		std::setw(7) << mT(0) << ", " << std::setw(7) << mT(1) << ", " <<
		std::setw(7) << mT(2) << "  h: " << std::setw(5) <<
		(head * 180.0f / M_PI) << " \r";// std::endl;
		/* unmodified magnetometer vector
		std::cout << "   Mag: " << std::setw(7) << m.norm() << ' ' <<
		std::setw(5) << rsM.x << ", " <<
		std::setw(5) << rsM.y << ", " << std::setw(5) << rsM.z << " \r";// std::endl;
		*/
		std::cout.flush();
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

int main(int argc, char *argv[])
try {
	{ // vector math tests
		Eigen::Vector3f res;
		Eigen::Vector3f grav(1.0f, 0, 9.0f);
		Eigen::Vector3f mod(1.0f, 1.0f, 9.0f);
		float angle;
		std::cout << "Vector rotation tests" << std::endl;
		makeHorizontal(res, grav, grav, angle);
		res.normalize();
		std::cout << "Test 1 result: " << res(0) << ", " << res(1) << ", " <<
		res(2) << "  angle: " << (angle * 180.0f / M_PI) << "  heading: " <<
		(heading(res) * 180.0f / M_PI) << std::endl;
		makeHorizontal(res, grav, mod, angle);
		res.normalize();
		std::cout << "Test 2 result: " << res(0) << ", " << res(1) << ", " <<
		res(2) << "  angle: " << (angle * 180.0f / M_PI) << "  heading: " <<
		(heading(res) * 180.0f / M_PI) << std::endl;
	}
	std::unique_ptr<duds::hardware::interface::I2c> magI2c(
		new duds::hardware::interface::linux::DevI2c(
			argc > 1 ? argv[1] : "/dev/i2c-1",
			0x1E
		)
	);
	std::unique_ptr<duds::hardware::interface::I2c> accelI2c(
		new duds::hardware::interface::linux::DevI2c(
			argc > 1 ? argv[1] : "/dev/i2c-1",
			0x6B
		)
	);
	duds::hardware::devices::instruments::LSM9DS1 acclgyromag(accelI2c, magI2c);
	acclgyromag.configure(2.0f, 2.0f, config);
	std::cout.precision(4);
	//std::cout << "Sampling frequency reported as " << acclgyromag.sampleRate() <<
	//"Hz" << std::endl;
	std::thread doit(runtest, std::ref(acclgyromag));
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
