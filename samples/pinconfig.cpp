#include <duds/hardware/interface/PinConfiguration.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <iostream>

namespace duds { namespace hardware { namespace interface {

static void pinout(std::ostream &os, unsigned int id) {
	if (id == PinConfiguration::Pin::NoPin) {
		os << "no pin";
	} else if (id == PinConfiguration::Pin::NoIdSpecified) {
		os << "not given";
	} else {
		os << id;
	}
}

std::ostream &operator << (std::ostream &os, const PinConfiguration::Pin &pin) {
	os << "Global ID: ";
	pinout(os, pin.gid);
	os << ", port ID: ";
	pinout(os, pin.pid);
	if (!pin.name.empty()) {
		os << ", name: " << pin.name;
	}
	return os;
}

std::ostream &operator << (std::ostream &os, const PinConfiguration::ChipSel &cs) {
	os << '(' << cs.chipId << ',' << (int)cs.mgr->type << ')';
	return os;
}

std::ostream &operator << (std::ostream &os, const PinConfiguration::SelMgr &sm) {
	switch (sm.type) {
		case PinConfiguration::SelMgr::Unknown:
			os << "Unknown";
			break;
		case PinConfiguration::SelMgr::Binary:
			os << "Binary";
			break;
		case PinConfiguration::SelMgr::Multiplexer:
			os << "Multiplexer";
			break;
		case PinConfiguration::SelMgr::Pin:
			os << "Pin";
			break;
		case PinConfiguration::SelMgr::PinSet:
			os << "PinSet";
			break;
		default:
			os << "???";
	}
	os << " chip select manager with " << sm.pins.size() << " pins and " <<
	sm.selNames.size() << " selects.\n\tPin global IDs:\n";
	for (unsigned int ui : sm.pins) {
		os << "\t\t" << ui << '\n';
	}
	os << "\tSelects:\n";
	for (const std::pair<std::string, unsigned int> &sel : sm.selNames) {
		os << "\t\t" << sel.first << " (" << sel.second << ')';
		if (sm.type == PinConfiguration::SelMgr::PinSet) {
			os << ", selection state: " <<
			((sm.selStates & (1 << sel.second)) ? 1 : 0);
		}
		os << '\n';
	}
	if (sm.type == PinConfiguration::SelMgr::Pin) {
		os << "\tInitial selection: " << (sm.initSelHigh ? 1 : 0) << '\n';
	} else if (sm.type == PinConfiguration::SelMgr::Binary) {
		os << "\tSelection state: " << (sm.initSelHigh ? 1 : 0) << '\n';
	}
	return os;
}

std::ostream &operator << (std::ostream &os, const PinConfiguration::PinSet &ps) {
	os << ps.pins.size() << " pins:\n";
	int cnt = -1;
	for (auto const &pin : ps.seqIndex()) {
		os << '\t' << ++cnt << ", " << pin << '\n';
	}
	if (!ps.selName.empty()) {
		os << "\tSelect is " << ps.selName << std::endl;
	}
	return os;
}

std::ostream &operator << (std::ostream &os, const PinConfiguration::Port &port) {
	os << port.pins.size() << " pins:\n";
	for (auto const &pin : port.gidIndex()) {
		os << '\t' << pin << '\n';
	}
}

} } }

int main(int argc, char *argv[])
try {
	if (argc != 2) {
		std::cerr << "pinconfig requires one argument: the path to the "
		"configuration file." << std::endl;
		return 1;
	}
	std::cout << "Reading pin configuration from " << argv[1] << '.' << std::endl;
	// will hold the root of the parsed file
	boost::property_tree::ptree tree;
	// parse a file into the property tree
	boost::property_tree::read_info(argv[1], tree);
	// find the subtree at the key "pins"
	boost::property_tree::ptree &pinconf = tree.get_child("pins");
	// use the subtree as the pin configuration
	duds::hardware::interface::PinConfiguration pc(pinconf);
	// show what was found
	for (auto iter = pc.portsBegin(); iter != pc.portsEnd(); ++iter) {
		std::cout << "Port " << iter->first << ", " << iter->second;
	}
	for (auto iter = pc.selectManagersBegin(); iter != pc.selectManagersEnd(); ++iter) {
		std::cout << "Select manager " << iter->first << ", " << iter->second;
	}
	if (pc.selectsBegin() != pc.selectsEnd()) {
		std::cout << "All chip selects:\n"; 
		for (auto iter = pc.selectsBegin(); iter != pc.selectsEnd(); ++iter) {
			std::cout << '\t' << iter->first << '\n';
		}
	}
	for (auto iter = pc.pinSetsBegin(); iter != pc.pinSetsEnd(); ++iter) {
		std::cout << "Pin set " << iter->first << ", " << iter->second;
	}
	return 0;
} catch (...) {
	std::cerr << "Program failed in main():\n" <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
