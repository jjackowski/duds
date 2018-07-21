#include <duds/hardware/interface/PinConfiguration.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <iostream>

int main()
try {
	// will hold the root of the parsed file
	boost::property_tree::ptree tree;
	// parse a file into the property tree
	//boost::property_tree::read_info("duds/hardware/interface/pins.conf", tree);
	boost::property_tree::read_info("samples/pins.conf", tree);
	// find the subtree at the key "pins"
	boost::property_tree::ptree &pinconf = tree.get_child("pins");
	// use the subtree as the pin configuration
	duds::hardware::interface::PinConfiguration pc(pinconf);
	// place some useful code here
	return 0;
} catch (...) {
	std::cerr << "Program failed in main():\n" <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
