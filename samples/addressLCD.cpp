/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2018  Jeff Jackowski
 */
/**
 * @file
 * A sample of using HD44780 and TextDisplayStream along with BppImage to
 * define graphic icons for use with the display. Shows IPv4 addresses on the
 * display with icons for wired and wireless networks.
 */

#include <duds/hardware/devices/displays/HD44780.hpp>
#include <duds/hardware/devices/displays/TextDisplayStream.hpp>
#include <duds/hardware/devices/displays/BppImageArchive.hpp>
#include <duds/hardware/interface/linux/SysFsPort.hpp>
#include <duds/hardware/interface/ChipPinSelectManager.hpp>
#include <iostream>
#include <thread>
#include <iomanip>
#include <algorithm>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>
#include <duds/general/IntegerBiDirIterator.hpp>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/asio/ip/address.hpp>
#include <linux/wireless.h>
#include <boost/program_options.hpp>

//#include <neticons.h>

bool quit = false;

namespace displays = duds::hardware::devices::displays;

class NetInterface {
	boost::asio::ip::address addr;
	std::string ifname;
	std::string id;
	void wlQuery() {
		// query for wireless network data
		// request struct
		iwreq req;
		// dummy socket
		int sock = socket(AF_INET, SOCK_DGRAM, 0);
		// copy in network interface name
		std::copy_n(
			ifname.begin(),
			std::min(ifname.size(), sizeof(req.ifr_name)),
			req.ifr_name
		);
		// ask for network name
		char essidbuff[32];
		req.u.essid.pointer = essidbuff;
		req.u.essid.length = 32;
		if (ioctl(sock, SIOCGIWESSID, &req) >= 0) {
			id = essidbuff;
		}
		close(sock);
	}
public:
	NetInterface(const std::string &n, const sockaddr_in *sa) :
	ifname(n),
	addr(boost::asio::ip::address_v4(
		htonl(sa->sin_addr.s_addr)
		// sin_addr is allways big endian; must not use int constructor since it
		// expects host byte order
		//(boost::asio::ip::address_v4::bytes_type*)&(sa->sin_addr.s_addr)
	)) {
		wlQuery();
	}
	NetInterface(const std::string &n, const boost::asio::ip::address_v4 &sa) :
	ifname(n), addr(sa) {
		wlQuery();
	}
	/*

	iw_statistics *stats;

	//have to use a socket for ioctl
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	//make room for the iw_statistics object
	req.u.data.pointer = (iw_statistics *)malloc(sizeof(iw_statistics));
	req.u.data.length = sizeof(iw_statistics);

	//this will gather the signal strength
	if(ioctl(sockfd, SIOCGIWSTATS, &req) == -1){
	  //die with error, invalid interface
	  fprintf(stderr, "Invalid interface.\n");
	  return(-1);
	}
	else if(((iw_statistics *)req.u.data.pointer)->qual.updated & IW_QUAL_DBM){
	  //signal is measured in dBm and is valid for us to use
	  sigInfo->level=((iw_statistics *)req.u.data.pointer)->qual.level - 256;
	}

	//SIOCGIWESSID for ssid
	char buffer[32];
	memset(buffer, 0, 32);
	req.u.essid.pointer = buffer;
	req.u.essid.length = 32;
	//this will gather the SSID of the connected network
	if(ioctl(sockfd, SIOCGIWESSID, &req) == -1){
	  //die with error, invalid interface
	  return(-1);
	}
	else{
	  memcpy(&sigInfo->ssid, req.u.essid.pointer, req.u.essid.length);
	  memset(&sigInfo->ssid[req.u.essid.length],0,1);
	}

	//SIOCGIWRATE for bits/sec (convert to mbit)
	int bitrate=-1;
	//this will get the bitrate of the link
	if(ioctl(sockfd, SIOCGIWRATE, &req) == -1){
	  fprintf(stderr, "bitratefail");
	  return(-1);
	}else{
	  memcpy(&bitrate, &req.u.bitrate, sizeof(int));
	  sigInfo->bitrate=bitrate/1000000;
	}


	//SIOCGIFHWADDR for mac addr
	ifreq req2;
	strcpy(req2.ifr_name, iwname);
	//this will get the mac address of the interface
	if(ioctl(sockfd, SIOCGIFHWADDR, &req2) == -1){
	  fprintf(stderr, "mac error");
	  return(-1);
	}
	else{
	  sprintf(sigInfo->mac, "%.2X", (unsigned char)req2.ifr_hwaddr.sa_data[0]);
	  for(int s=1; s<6; s++){
			sprintf(sigInfo->mac+strlen(sigInfo->mac), ":%.2X", (unsigned char)req2.ifr_hwaddr.sa_data[s]);
	  }
	}
	close(sockfd);
	*/

	//NetInterface(const std::string &n, const sockaddr_in6 *sa);
	const boost::asio::ip::address &address() const {
		return addr;
	}
	const std::string &name() const {
		return ifname;
	}
	const std::string &essid() const {
		return id;
	}
	bool isWireless() const {
		return !id.empty();
	}
	bool operator < (const NetInterface &ni) const {
		return ifname < ni.name();
	}
	bool operator < (const std::string &ni) const {
		return ifname < ni;
	}
};

bool operator < (const std::string &name, const NetInterface &ni) {
	return name < ni.name();
}

struct GenericTransparentComp {
	typedef int is_transparent;
	template <class A, class B>
	bool operator()(const A &a, const B &b) const {
		return a < b;
	}
};

std::set<NetInterface, GenericTransparentComp> netifs;

int Fillnetifs() {
	std::set<std::string> seen;
	ifaddrs *ifAddrStruct = nullptr;
	ifaddrs *ifa = nullptr;
	getifaddrs(&ifAddrStruct);
	int updates = 0;
	
	for (ifa = ifAddrStruct; ifa; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr) {
			continue;
		}
		// IPv4 address?
		if (ifa->ifa_addr->sa_family == AF_INET) {
			boost::asio::ip::address_v4 ip4(
				htonl(((sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr)
				//(boost::asio::ip::address_v4::bytes_type)
				//&((sockaddr_in*)ifa->ifa_addr)->sin_addr
			);
			if (!ip4.is_loopback() && !ip4.is_multicast()) {
				// look for an existing network
				std::set<NetInterface>::iterator iter = netifs.find(ifa->ifa_name);
				// found it?
				if (iter != netifs.end()) {
					// different address?
					if (iter->address() != ip4) {
						// replace the object
						netifs.erase(iter);
						netifs.emplace(ifa->ifa_name, ip4);
						++updates;
					}
					// if same address, leave object as is
				} else {
					// new network, new object
					netifs.emplace(ifa->ifa_name, ip4);
					++updates;
				}
				// always mark as seen
				seen.insert(ifa->ifa_name);
			}
		}
		// IPv6 address?
		/*  ignore for now
		else if (ifa->ifa_addr->sa_family == AF_INET6) {
			boost::asio::ip::address_v6 ip6(
				(boost::asio::ip::address_v6::bytes_type*)
				&((sockaddr_in6*)ifa->ifa_addr)->sin6_addr
			);
			if (!ip6.is_loopback() && !ip6.is_multicast()) {
				netifs.emplace(ifa->ifa_name, ip6);
			}
		}
		*/
	}
	if (ifAddrStruct) {
		freeifaddrs(ifAddrStruct);
	}
	// check for removals
	std::set<NetInterface, GenericTransparentComp>::iterator iter = netifs.begin();
	while (iter != netifs.end()) {
		std::set<std::string>::iterator siter = seen.find(iter->name());
		// not found?
		if (siter == seen.end()) {
			// remove it
			iter = netifs.erase(iter);
			++updates;
		} else {
			++iter;
		}
	}
	return updates;
}

/*  Display
16x2
0123456789012345
I 192.168.1.200
I192.168.100.200

20x4
01234567890123456789
I   192.168.1.200
I  192.168.100.200
*/

void show(
	const std::shared_ptr<displays::HD44780> &tmd
) try {
	displays::TextDisplayStream tds(tmd);
	int updates = 1;
	Fillnetifs();
	do {
		if (updates) {
			std::cout << "--- Network change ---" << std::endl;
			tmd->initialize();
			if (netifs.empty()) {
				tds << "No networks.";
				std::cout << "Found no network interfaces." << std::endl;
			} else {
				int cnt = 0;
				for ( auto const &nif : netifs ) {
					if (nif.isWireless()) {
						tds << '\2';
					} else {
						tds << '\4';
					}
					std::string addr = nif.address().to_string();
					int len = tmd->columns() - addr.size() - 1;
					if (len) {
						tds << ' ';
					}
					tds << addr << displays::startLine;
					// console output
					std::cout << nif.name() << ": " << addr << "\n\t";
					if (nif.isWireless()) {
						std::cout << "Wireless, ESSID: " << nif.essid() << std::endl;
					} else {
						std::cout << "Wired" << std::endl;
					}
					if (++cnt == tmd->rows()) {
						// no more space
						break;
					}
					// the wireless network name is displayed on 4 row displays or
					// when it is the only network
					if (nif.isWireless() && ((tmd->rows() > 2) || (netifs.size() == 1))) {
						tds << std::right << std::setw(tmd->columns()) << nif.essid() <<
						displays::startLine << std::left;
						if (++cnt == tmd->rows()) {
							// no more space
							break;
						}
					}
				}
				/* icon test
				if (cnt < tmd->rows()) {
					tds << "Wireless: \10\1\2\3";
				}
				*/
			}
		}
		// wait for changes
		std::this_thread::sleep_for(std::chrono::seconds(16));
		updates = Fillnetifs();
	} while (!quit);
} catch (...) {
	std::cerr << "Test failed in show():\n" <<
	boost::current_exception_diagnostic_information()
	<< std::endl;
}

typedef duds::general::IntegerBiDirIterator<unsigned int>  uintIterator;

int main(int argc, char *argv[])
try {
	bool lcd20x4 = false, noinput = false;
	{ // option parsing
		boost::program_options::options_description optdesc(
			"Options for addressLCD"
		);
		optdesc.add_options()
			( // help info
				"help,h",
				"Show this help message"
			)
			( // the LCD size
				"lcd20x4",
				"Use 20x4 LCD instead of 16x2"
			)
			( // don't read from cin; run everything on one thread
				"noinput",
				"Do not accept input for termination request. OpenRC will claim "
				"this program has crashed without this option because it appears "
				"to send the termination request."
			)
		;
		boost::program_options::variables_map vm;
		boost::program_options::store(
			boost::program_options::parse_command_line(argc, argv, optdesc),
			vm
		);
		boost::program_options::notify(vm);
		if (vm.count("help")) {
			std::cout << "Show network addresses on attached text LCD\n" <<
			argv[0] << " [options]\n" << optdesc << std::endl;
			return 0;
		}
		if (vm.count("lcd20x4")) {
			lcd20x4 = true;
		}
		if (vm.count("noinput")) {
			noinput = true;
		}
	}

	std::string iconpath(argv[0]);
	while (!iconpath.empty() && (iconpath.back() != '/')) {
		iconpath.pop_back();
	}
	iconpath += "neticons.bppia";
	// load some icons before messing with hardware
	displays::BppImageArchive imgArc;
	imgArc.load(iconpath);
	std::shared_ptr<displays::BppImage> wiredIcon = imgArc.get("WiredLAN");
	std::shared_ptr<displays::BppImage> wirelessIcon[4] = {
		imgArc.get("WirelessLAN_S0"),
		imgArc.get("WirelessLAN_S1"),
		imgArc.get("WirelessLAN_S2"),
		imgArc.get("WirelessLAN_S3"),
	};
	/*
	std::shared_ptr<displays::BppImage> wiredIcon =
		displays::BppImage::make(WiredLAN);
		//std::make_shared<displays::BppImage>(WiredLAN);
	std::shared_ptr<displays::BppImage> wirelessIcon =
		std::make_shared<displays::BppImage>(WirelessLAN_S2);
	//std::shared_ptr<displays::BppImage> blockIcon =
	//	std::make_shared<displays::BppImage>(TestBlock);
	*/

	// configure display
	//                       LCD pins:  4  5   6   7  RS   E
	std::vector<unsigned int> gpios = { 5, 6, 19, 26, 20, 21 };
	std::shared_ptr<duds::hardware::interface::linux::SysFsPort> port =
		std::make_shared<duds::hardware::interface::linux::SysFsPort>(
			gpios, 0
		);
	assert(!port->simultaneousOperations());  //  :-(
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
	// LCD driver
	std::shared_ptr<displays::HD44780> tmd =
		std::make_shared<displays::HD44780>(
			lcdset, lcdsel, lcd20x4 ? 20 : 16, lcd20x4 ? 4 : 2
		);
	tmd->initialize();
	tmd->setGlyph(wiredIcon, 4);
	for (int i = 0; i < 4; ++i) {
		tmd->setGlyph(wirelessIcon[i], i);
	}

	if (noinput) {
		// will not return
		show(tmd);
	} else {
		std::thread doit(show, std::ref(tmd));
		std::cin.get();
		quit = true;
		doit.join();
	}
} catch (...) {
	std::cerr << "Test failed in main():\n" <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
