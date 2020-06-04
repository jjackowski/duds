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
#include <duds/hardware/display/TextDisplayStream.hpp>
#include <duds/hardware/devices/displays/ST7920.hpp>
#include <duds/ui/graphics/BppImageArchive.hpp>
#include <duds/ui/graphics/BppStringCache.hpp>
#ifdef USE_SYSFS_PORT
#include <duds/hardware/interface/linux/SysFsPort.hpp>
#else
#include <duds/hardware/interface/linux/GpioDevPort.hpp>
#endif
#include <duds/hardware/interface/ChipPinSelectManager.hpp>
#include <duds/hardware/interface/PinConfiguration.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <iostream>
#include <thread>
#include <iomanip>
#include <algorithm>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/asio/ip/address.hpp>
#include <linux/wireless.h>
#include <boost/program_options.hpp>
#include <csignal>

//#include <neticons.h>

std::sig_atomic_t quit = 0;

namespace displays = duds::hardware::devices::displays;
namespace display = duds::hardware::display;
namespace graphics = duds::ui::graphics;

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
			std::min(ifname.size() + 1, sizeof(req.ifr_name)),
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
std::mutex netifsBlock;
std::condition_variable netifUpdate;

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

void netcheck() {
	int netChanges;
	while (!quit) {
		{ // block on netifs to change it
			std::unique_lock<std::mutex> lock(netifsBlock);
			netChanges = Fillnetifs();
		}
		if (netChanges) {
			netifUpdate.notify_all();
		}
		std::this_thread::sleep_for(std::chrono::seconds(8));
	}
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

void showText(
	const std::shared_ptr<displays::HD44780> &tmd
) try {
	display::TextDisplayStream tds(tmd);
	while (!quit) {
		std::unique_lock<std::mutex> lock(netifsBlock);
		tmd->initialize();
		if (netifs.empty()) {
			tds << "No networks.";
			//std::cout << "Found no network interfaces." << std::endl;
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
				tds << addr << display::startLine;
				// console output
				/*
				std::cout << nif.name() << ": " << addr << "\n\t";
				if (nif.isWireless()) {
					std::cout << "Wireless, ESSID: " << nif.essid() << std::endl;
				} else {
					std::cout << "Wired" << std::endl;
				}
				*/
				if (++cnt == tmd->rows()) {
					// no more space
					break;
				}
				// the wireless network name is displayed on 4 row displays or
				// when it is the only network
				if (nif.isWireless() && ((tmd->rows() > 2) || (netifs.size() == 1))) {
					tds << std::right << std::setw(tmd->columns()) << nif.essid() <<
					display::startLine << std::left;
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
		try {
			netifUpdate.wait(lock);
		} catch (...) {
			// bail!
			quit = 1;
			netifUpdate.notify_all();
			return;
		}
		if (quit) {
			return;
		}
	}
} catch (...) {
	std::cerr << "Test failed in showText():\n" <<
	boost::current_exception_diagnostic_information()
	<< std::endl;
}

void showGraphic(
	const display::BppGraphicDisplaySptr &disp,
	const graphics::BppStringCacheSptr &strcache
) try {
	const duds::ui::graphics::ImageDimensions &dispdim = disp->dimensions();
	duds::ui::graphics::BppImage frame(dispdim);
	const int th = strcache->font()->estimatedMaxCharacterSize().h;
	while (!quit) {
		std::unique_lock<std::mutex> lock(netifsBlock);
		frame.clearImage();
		if (netifs.empty()) {
			graphics::ConstBppImageSptr nnt = strcache->text("No networks.");
			duds::ui::graphics::ImageLocation loc(
				std::max((dispdim.w - nnt->width()) / 2, 0),
				std::max((dispdim.h - nnt->height()) / 2, 0)
			);
			frame.write(nnt, loc);
		} else {
			int y = 0;
			for ( auto const &nif : netifs ) {
				std::ostringstream oss;
				oss << nif.name();
				if (nif.isWireless()) {
					oss << ": " << nif.essid();
				}
				oss << "\n  " << nif.address().to_string();
				graphics::ConstBppImageSptr nettext = strcache->text(oss.str());
				duds::ui::graphics::ImageLocation loc(0, y);
				frame.write(nettext, loc);
				y += nettext->height();
				if (y > (dispdim.h - th)) break;
			}
		}
		disp->write(&frame);
		try {
			netifUpdate.wait(lock);
		} catch (...) {
			// bail!
			quit = 1;
			netifUpdate.notify_all();
			return;
		}
		if (quit) {
			return;
		}
	}
} catch (...) {
	std::cerr << "Test failed in showGraphic():\n" <<
	boost::current_exception_diagnostic_information()
	<< std::endl;
}


void signalHandler(int) {
	quit = 1;
	netifUpdate.notify_all();
}

int main(int argc, char *argv[])
try {
	std::string confpath, fontpath;
	std::string imgpath(argv[0]);
	int dispW, dispH;
	bool lcdT = false, lcd20x4 = false, lcdG = false, noinput = false;
	{
		int found = 0;
		while (!imgpath.empty() && (found < 3)) {
			imgpath.pop_back();
			if (imgpath.back() == '/') {
				++found;
			}
		}
		imgpath += "images/";
	}
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
				"lcd16x2",
				"Use text 16x2 LCD. Default if nothing else specified."
			)
			( // the LCD size
				"lcd20x4",
				"Use text 20x4 LCD"
			)
			(
				"st7920",
				"Use a graphic ST7920 LCD"
			)
			(
				"font",
				boost::program_options::value<std::string>(&fontpath)->
					default_value(imgpath + "font_Vx8B.bppia"),
				"Font file for graphic display"
			)
			( // the display width
				"width,x",
				boost::program_options::value<int>(&dispW)->
					default_value(144),
				"ST7920 display width in pixels"
			)
			( // the display height
				"height,y",
				boost::program_options::value<int>(&dispH)->
					default_value(32),
				"ST7920 display height in pixels"
			)
			( // don't read from cin; run everything on one thread
				"noinput",
				"Do not accept input for termination request. OpenRC will claim "
				"this program has crashed without this option because it appears "
				"to send the termination request."
			)
			(
				"conf,c",
				boost::program_options::value<std::string>(&confpath)->
					default_value("samples/pins.conf"),
				"Pin configuration file; REQUIRED"
			)
		;
		boost::program_options::variables_map vm;
		boost::program_options::store(
			boost::program_options::parse_command_line(argc, argv, optdesc),
			vm
		);
		boost::program_options::notify(vm);
		if (vm.count("help")) {
			std::cout << "Show network addresses on an attached text an/or "
			"graphic LCD\n" << argv[0] << " [options]\n" << optdesc << std::endl;
			return 0;
		}
		if (vm.count("lcd16x2")) {
			lcdT = true;
		}
		if (vm.count("lcd20x4")) {
			lcd20x4 = true;
			lcdT = true;
		}
		if (vm.count("st7920")) {
			lcdG = true;
		}
		if (vm.count("noinput")) {
			noinput = true;
		}
		// default is lcd16x2
		if (!lcdT && !lcdG) {
			lcdT = true;
		}
	}
	std::signal(SIGINT, &signalHandler);
	std::signal(SIGTERM, &signalHandler);
	std::thread outputT, outputG;

	// load some icons before messing with hardware
	duds::ui::graphics::BppImageArchive imgArc;
	if (lcdT) {
		imgArc.load(imgpath + "neticons.bppia");
	}
	
	graphics::BppStringCacheSptr fontCache;
	if (lcdG) {
		fontCache = graphics::BppStringCache::make(
			graphics::BppFont::make(fontpath)
		);
	}
	
	// read in digital pin config
	boost::property_tree::ptree tree;
	boost::property_tree::read_info(confpath, tree);
	boost::property_tree::ptree &pinconf = tree.get_child("pins");
	duds::hardware::interface::PinConfiguration pc(pinconf);

	// configure display
	#ifdef USE_SYSFS_PORT
	std::shared_ptr<duds::hardware::interface::linux::SysFsPort> port =
		duds::hardware::interface::linux::SysFsPort::makeConfiguredPort(pc);
	#else
	std::shared_ptr<duds::hardware::interface::linux::GpioDevPort> port =
		duds::hardware::interface::linux::GpioDevPort::makeConfiguredPort(pc);
	#endif
	// pre-fill network data
	Fillnetifs();
	// text LCD driver
	std::shared_ptr<displays::HD44780> tmd;
	if (lcdT) {
		duds::hardware::interface::DigitalPinSet lcdset;
		duds::hardware::interface::ChipSelect lcdsel;
		pc.getPinSetAndSelect(lcdset, lcdsel, "lcdText");
	
		// LCD driver
		tmd = std::make_shared<displays::HD44780>(
			std::move(lcdset),
			std::move(lcdsel),
			lcd20x4 ? 20 : 16,
			lcd20x4 ? 4 : 2
		);
		tmd->initialize();
		tmd->setGlyph(imgArc.get("WiredLAN"), 4);
		for (int i = 0; i < 4; ++i) {
			std::ostringstream oss;
			oss << "WirelessLAN_S" << i;
			tmd->setGlyph(imgArc.get(oss.str()), i);
		}
		outputT = std::thread(showText, std::ref(tmd));
	}
	display::BppGraphicDisplaySptr dispG;
	if (lcdG) {
		duds::hardware::interface::DigitalPinSet lcdset;
		duds::hardware::interface::ChipSelect lcdsel;
		pc.getPinSetAndSelect(lcdset, lcdsel, "lcdGraphic");
		std::shared_ptr<duds::hardware::devices::displays::ST7920> lcd =
			std::make_shared<duds::hardware::devices::displays::ST7920>(
				std::move(lcdset), std::move(lcdsel), dispW, dispH
			);
		lcd->initialize();
		dispG = std::move(lcd);
		outputG = std::thread(showGraphic, std::ref(dispG), std::ref(fontCache));
	}

	if (quit) {
		return 1;
	}
	if (noinput) {
		// will not return except on error or quit (signal)
		netcheck();
	} else {
		std::thread nchk(netcheck);
		std::cin.get();
		quit = 1;
		netifUpdate.notify_all();
		nchk.join();
	}
	if (lcdT) {
		outputT.join();
	}
	if (lcdG) {
		outputG.join();
	}
} catch (...) {
	quit = 1;
	std::cerr << "Test failed in main():\n" <<
	boost::current_exception_diagnostic_information() << std::endl;
	try {
		netifUpdate.notify_all();
	} catch (...) { }
	return 1;
}
