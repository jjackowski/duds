/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://www.somewhere.org/somepath/license.html.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
/**
 * @file
 * A demonstration of using the menu system with the bit-per-pixel renderer.
 */

#include <duds/hardware/devices/displays/SimulatedBppDisplay.hpp>
#include <duds/hardware/devices/displays/ST7920.hpp>
#include <duds/hardware/interface/linux/GpioDevPort.hpp>
#include <duds/hardware/interface/PinConfiguration.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <duds/ui/graphics/BppImageArchive.hpp>
#include <duds/ui/menu/MenuAccess.hpp>
#include <duds/ui/menu/renderers/BppIconItem.hpp>
#include <duds/ui/menu/renderers/BppMenuRenderer.hpp>
#include <duds/ui/PathStringGenerator.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <assert.h>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/program_options.hpp>
#include <csignal>
#include <duds/os/linux/EvdevErrors.hpp>
#include <duds/os/linux/EvdevInput.hpp>
#include <assert.h>

std::sig_atomic_t quit = 0;

void signalHandler(int) {
	quit = 1;
}

// Doxygen 1.8.16 can find typedefs from these shortened namespaces, but not
// classes. In a few spots, some classes are referenced by the complete
// namespace so that Doxygen will list this source file as an example in the
// class documentation.
namespace display = duds::hardware::display;
namespace graphics = duds::ui::graphics;
namespace menu = duds::ui::menu;
namespace ui = duds::ui;
namespace os = duds::os::linux;

duds::ui::graphics::BppImageArchive menuicons;

class MenuViews;

void toggleVisibility(
	duds::ui::menu::GenericMenuItem &toggle,
	duds::ui::menu::MenuItemSptr vischange
) {
	vischange->changeVisibility(toggle.isToggledOn());
}

void toggleState(
	duds::ui::menu::GenericMenuItem &toggle
) {
	toggle.toggle();
}

/**
 * Handles the Menu objects so that only one copy of each will be needed.
 */
class Menus {
	menu::MenuSptr root;
	menu::MenuSptr subs[16][16];
	void makeRootMenuItem(menu::MenuAccess &ma);
	void makeSubMenuItem(
		menu::MenuAccess &ma,
		const graphics::BppFontSptr &iconFont,
		int x,
		int y
	);
	void makeBackMenuItem(menu::MenuAccess &ma);
public:
	Menus(const graphics::BppFontSptr &iconFont) {
		// first, make all the menu ojbects
		root = duds::ui::menu::Menu::make("Root");
		for (int l0 = 0; l0 < 16; ++l0) {
			for (int l1 = 0; l1 < 16; ++l1) {
				std::ostringstream oss;
				oss << "Sub " << l0 << '-' << l1;
				subs[l0][l1] = menu::Menu::make(oss.str());
			}
		}
		// next, make items for the menus
		{ // root
			duds::ui::menu::MenuAccess ma(root);
			for (int i = 0; i < 8; ++i) {
				makeSubMenuItem(ma, iconFont, i, 0);
			}
			{ // invisible item & toggle control
				menu::GenericMenuItemSptr invis = menu::GenericMenuItem::make(
					"Initially invisible",
					menu::MenuItem::Invisible
				);
				menu::GenericMenuItemSptr indis = menu::GenericMenuItem::make(
					"Disabled",
					menu::MenuItem::Invisible | menu::MenuItem::Disabled
				);
				menu::GenericMenuItemSptr intog = menu::GenericMenuItem::make(
					"Another toggle",
					menu::MenuItem::Invisible | menu::MenuItem::Toggle |
					menu::MenuItem::ToggledOn
				);
				menu::GenericMenuItemSptr tog = menu::GenericMenuItem::make(
					"Show invisible", menu::MenuItem::Toggle
				);
				tog->choseConnect(std::bind(
					&menu::MenuItem::toggle, std::placeholders::_3
				));
				tog->choseConnect(std::bind(
					&toggleVisibility, std::placeholders::_3, invis
				));
				tog->choseConnect(std::bind(
					&toggleVisibility, std::placeholders::_3, indis
				));
				tog->choseConnect(std::bind(
					&toggleVisibility, std::placeholders::_3, intog
				));
				intog->choseConnect(std::bind(
					&menu::MenuItem::toggle, std::placeholders::_3
				));
				ma.append(std::move(tog));
				ma.append(std::move(invis));
				ma.append(std::move(indis));
				ma.append(std::move(intog));
			}
			// long item; test clipping on right side
			ma.append(menu::GenericMenuItem::make(
				"Does nothing - 0123456789 - really long"
			));
		}
		// sub menus
		for (int l0 = 0; l0 < 16; ++l0) {
			for (int l1 = 0; l1 < 16; ++l1) {
				menu::MenuAccess ma(subs[l0][l1]);
				std::ostringstream oss;
				if (l1 < 15) {
					makeSubMenuItem(ma, iconFont, l0, l1 + 1);
				}
				if (l1 > 0) {
					makeSubMenuItem(ma, iconFont, l0, l1 - 1);
				}
				if (l0 < 15) {
					makeSubMenuItem(ma, iconFont, l0 + 1, l1);
				}
				if (l0 > 0) {
					makeSubMenuItem(ma, iconFont, l0 - 1, l1);
				}
				makeBackMenuItem(ma);
				makeRootMenuItem(ma);
			}
		}
	}
	const menu::MenuSptr &rootMenu() const {
		return root;
	}
	const menu::MenuSptr &subMenu(int x, int y) const {
		return subs[x][y];
	}
};

/**
 * Handles a copy of MenuView objects and attaches them to the menus in Menus.
 * There can be multiple MenuViews objects.
 */
class MenuViews {
	menu::MenuViewSptr root;
	menu::MenuViewSptr subs[16][16];
	menu::MenuViewSptr curr;
	graphics::BppStringCacheSptr strcache;
public:
	duds::ui::menu::renderers::BppMenuRenderer renderer;
	duds::ui::Path path;
	duds::ui::PathStringGenerator psgen;
private:
	void setMenu() {
		menu::MenuViewSptr view = std::static_pointer_cast<duds::ui::menu::MenuView>(
			path.currentPage()
		);
		if (view) {
			curr = view;
		}
		//std::cout << "Path: size = " << path.size() << ", " <<
		//psgen.generate(path) << "            " << std::endl;
	}
public:
	MenuViews(
		const Menus &menus,
		const graphics::BppStringCacheSptr &sc,
		const os::EvdevInputSptr input
	) :
		renderer(strcache, menu::renderers::BppMenuRenderer::InvertSelected),
		strcache(sc),
		psgen("/", ">")
	{
		renderer.toggledOffIcon(menuicons.get("Unmarked8x8"));
		renderer.toggledOnIcon(menuicons.get("Checked8x8"));
		// not really needed with inverted selection rendering
		//renderer.selectedIcon(menuicons.get("SelectTriangle"));
		renderer.disabledIcon(menuicons.get("Disabled8x8"));
		renderer.iconDimensions(strcache->font()->estimatedMaxCharacterSize());
		if (renderer.iconDimensions().w > 0) {
			renderer.iconTextMargin(1);
		}
		renderer.addScrollBar(2);
		root = curr = menu::MenuView::make(menus.rootMenu());
		root->context() = this;
		path.push(root);
		for (int l0 = 0; l0 < 16; ++l0) {
			for (int l1 = 0; l1 < 16; ++l1) {
				subs[l0][l1] = duds::ui::menu::MenuView::make(menus.subMenu(l0, l1));
				subs[l0][l1]->context() = this;
			}
		}
		psgen.currentHeader("[");
		psgen.currentFooter("]");
		//configure input
		if (input) {
			input->inputConnect(
				duds::os::linux::EventTypeCode(EV_KEY, KEY_UP),
				std::bind(&MenuViews::inputUp, this, std::placeholders::_2)
			);
			input->inputConnect(
				os::EventTypeCode(EV_KEY, KEY_PAGEUP),
				std::bind(&MenuViews::inputPageUp, this, std::placeholders::_2)
			);
			input->inputConnect(
				os::EventTypeCode(EV_KEY, KEY_DOWN),
				std::bind(&MenuViews::inputDown, this, std::placeholders::_2)
			);
			input->inputConnect(
				os::EventTypeCode(EV_KEY, KEY_PAGEDOWN),
				std::bind(&MenuViews::inputPageDown, this, std::placeholders::_2)
			);
			input->inputConnect(
				os::EventTypeCode(EV_KEY, KEY_ENTER),
				std::bind(&MenuViews::inputChose, this, std::placeholders::_2)
			);
			input->inputConnect(
				os::EventTypeCode(EV_KEY, KEY_LEFT),
				std::bind(&MenuViews::inputBack, this, std::placeholders::_2)
			);
			input->inputConnect(
				os::EventTypeCode(EV_KEY, KEY_RIGHT),
				std::bind(&MenuViews::inputForward, this, std::placeholders::_2)
			);
		}
	}
	void inputUp(int val) {
		if (val > 0) {
			curr->forward(val);
		}
	}
	void inputPageUp(int val) {
		if (val > 0) {
			curr->forward(renderer.maxVisible());
		}
	}
	void inputDown(int val) {
		if (val > 0) {
			curr->backward(val);
		}
	}
	void inputPageDown(int val) {
		if (val > 0) {
			curr->backward(renderer.maxVisible());
		}
	}
	void inputChose(int val) {
		if (val > 0) {
			curr->chose();
		}
	}
	void inputBack(int val) {
		if (val > 0) {
			back();
		}
	}
	void inputForward(int val) {
		if (val > 0) {
			forward();
		}
	}
	void back() {
		path.back();
		setMenu();
	}
	void forward() {
		path.forward();
		setMenu();
	}
	void changePage(const ui::PageSptr &nextpage) {
		path.push(nextpage);
		setMenu();
	}
	void changePage(int x, int y) {
		path.push(subs[x][y]);
		setMenu();
	}
	void changeToRoot() {
		path.push(root);
		setMenu();
	}
	const graphics::BppStringCacheSptr &stringCache() const {
		return strcache;
	}
	std::string pathString() const {
		return psgen.generate(path);
	}
	const menu::MenuViewSptr &rootView() const {
		return root;
	}
	const menu::MenuViewSptr &subView(int x, int y) const {
		return subs[x][y];
	}
	const menu::MenuViewSptr &view() const {
		return curr;
	}
};


// -----
// Implementations for functions in Menus that use MenuViews

MenuViews *GetMenuView(menu::MenuView &view) {
	return boost::any_cast<MenuViews*>(view.context());
}

void Menus::makeRootMenuItem(menu::MenuAccess &ma) {
	menu::GenericMenuItemSptr gmi = menu::GenericMenuItem::make("Root");
	gmi->choseConnect(std::bind(
		&MenuViews::changeToRoot,
		std::bind(&GetMenuView, std::placeholders::_1)
	));
	ma.append(std::move(gmi));
}

void Menus::makeSubMenuItem(
	menu::MenuAccess &ma,
	const graphics::BppFontSptr &iconFont,
	int x,
	int y
) {
	std::ostringstream oss;
	oss << "Goto Sub " << x << '-' << y;
	duds::ui::menu::renderers::GenericBppMenuIconItemSptr gmi =
		duds::ui::menu::renderers::GenericBppMenuIconItem::make(oss.str());
	// use char vals 1-31 inclusive
	gmi->icon(iconFont->tryGet((x + y * 4) % 30 + 1));
	gmi->choseConnect(std::bind(
		(void(MenuViews::*)(int,int))&MenuViews::changePage,
		std::bind(&GetMenuView, std::placeholders::_1),
		x,
		y
	));
	ma.append(std::move(gmi));
}

void Menus::makeBackMenuItem(menu::MenuAccess &ma) {
	menu::GenericMenuItemSptr gmi = menu::GenericMenuItem::make("Back");
	gmi->choseConnect(std::bind(
		&MenuViews::back,
		std::bind(&GetMenuView, std::placeholders::_1)
	));
	ma.append(std::move(gmi));
}



void runtest(
	display::BppGraphicDisplaySptr disp,
	const graphics::BppStringCacheSptr &tcache,
	MenuViews &views,
	const os::EvdevInputSptr &input
) try {
	graphics::BppImage frame(disp->dimensions());
	frame.clearImage();
	graphics::BppImageSptr menuimg;
	menu::MenuOutput menuout;
	graphics::ImageLocation mdest;
	int theight;
	{
		graphics::ImageDimensions tdim =
			tcache->font()->estimatedMaxCharacterSize();
		theight = tdim.h;
		mdest.x = 0;
		mdest.y = tdim.h + 1;
		graphics::ImageDimensions cdim =
			views.stringCache()->font()->estimatedMaxCharacterSize();
		int lines = (disp->height() - mdest.y) / cdim.h + 1;
		menuout.attach(views.rootView(), lines);
		views.renderer.maxVisible(lines);
		// may restrict length with variable width font, so use double width
		views.psgen.maxLength((disp->width() * 2) / tdim.w);
		frame.invertLines(tdim.h, 1);
		menuimg = graphics::BppImage::make(disp->width(), disp->height() - mdest.y);
	}
	int cnt = 0;
	while ((input || (++cnt < 48)) && !quit) {
		{ // render the menu
			menu::MenuOutputAccess moa(menuout);
			if (moa.changed()) {
				views.renderer.render(menuimg, moa);
				frame.write(menuimg, mdest);
				//label = moa.selectedItem().label();
				// render title (path)
				frame.clearLines(0, theight);
				graphics::ConstBppImageSptr title = tcache->text(views.pathString());
				// work out right side of title when it is too long to fit
				graphics::ImageLocation titleSrc(0, 0);
				graphics::ImageDimensions titleDim = title->dimensions();
				if (titleDim.w > frame.width()) {
					titleSrc.x = titleDim.w - frame.width();
					titleDim.w = frame.width();
				}
				// write the title, right justified if it doesn't fit
				frame.write(
					title,
					graphics::ImageLocation(0, 0),
					titleSrc,
					titleDim
				);
				disp->write(&frame);
			}
		}
		
		// must partially move this out to handle multiple displays
		if (input) {
			std::this_thread::sleep_for(std::chrono::milliseconds(32));
			views.view()->update();
			// only needed when visible menu changes
			menuout.attach(views.view());
		} else {
			std::this_thread::sleep_for(std::chrono::seconds(2));
			//views.view()->jumpToLast();
			//views.view()->backward(3);
			//views.view()->update();
			if ((cnt & 15) == 8) {
				views.back();
				menuout.attach(views.view());
				views.view()->forward();
				views.view()->update();
			} else if (cnt & 1) {
				views.view()->chose();
				views.view()->update();
				menuout.attach(views.view());
			} else {
				views.view()->backward(2);
				views.view()->update();
			}
		}
	}
	quit = true;
} catch (...) {
	std::cerr << "Test failed in runtest():\n" <<
	boost::current_exception_diagnostic_information()
	<< std::endl;
	quit = true;
}

void doPoll(os::Poller &poller)
try {
	while (!quit) {
		// wait time is how long it may take this thread to end, and it must
		// end before program termination
		poller.wait(std::chrono::milliseconds(64));
	}
} catch (...) {
	std::cerr << "Test failed in doPoll():\n" <<
	boost::current_exception_diagnostic_information() << std::endl;
	quit = true;
}


int main(int argc, char *argv[])
try {
	std::string devpath, mfontpath, tfontpath, miconpath, confpath, lcdname;
	int dispW, dispH;
	bool grabinput = false, uselcd = false;
	std::string imgpath(argv[0]);
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
			"Options for bit-per-pixel menu test"
		);
		optdesc.add_options()
			( // help info
				"help,h",
				"Show this help message"
			)
			( // the display width
				"width,x",
				boost::program_options::value<int>(&dispW)->
					default_value(144),
				"Display width in pixels"
			)
			( // the display height
				"height,y",
				boost::program_options::value<int>(&dispH)->
					default_value(32),
				"Display height in pixels"
			)
			(
				"input,i",
				boost::program_options::value<std::string>(&devpath),
					//->default_value("/dev/input/event0"),
				"Input device path, typically /dev/input/event[0-9]+. If "
				"unspecified, pre-programmed input will be used."
			)
			( // request exclusive access to input
				"grab,g",
				"Request exclusive access to the input device. Intended to "
				"prevent input from showing up on the same console that has this "
				"program's output."
			)
			(
				"tfont",
				boost::program_options::value<std::string>(&tfontpath)->
					default_value(imgpath + "font_Vx7.bppia"),
				"Title font file"
			)
			(
				"mfont",
				boost::program_options::value<std::string>(&mfontpath)->
					default_value(imgpath + "font_Vx8B.bppia"),
				"Menu font file"
			)
			(
				"icons",
				boost::program_options::value<std::string>(&miconpath)->
					default_value(imgpath + "menuicons.bppia"),
				"Menu icon image file"
			)
			(
				"st7920",
				"Use a graphic ST7920 LCD"
			)
			(
				"conf,c",
				boost::program_options::value<std::string>(&confpath)->
					default_value("samples/pins.conf"),
				"Pin configuration file; required if LCD used"
			)
			(
				"lcdname",
				boost::program_options::value<std::string>(&lcdname)->
					default_value("lcdGraphic"),
				"Name of LCD inside pin configuration"
			)
		;
		boost::program_options::variables_map vm;
		boost::program_options::store(
			boost::program_options::parse_command_line(argc, argv, optdesc),
			vm
		);
		boost::program_options::notify(vm);
		if (vm.count("help")) {
			std::cout << "Test of bit-per-pixel menu\n" <<
			argv[0] << " [options]\n" << optdesc << std::endl;
			return 0;
		}
		if (vm.count("grab")) {
			grabinput = true;
		}
		if (vm.count("st7920")) {
			uselcd = true;
		}
	}
	std::signal(SIGINT, &signalHandler);
	std::signal(SIGTERM, &signalHandler);

	// configure input
	duds::os::linux::Poller poller;
	duds::os::linux::EvdevInputSptr einput;
	std::thread inputPolling;
	if (!devpath.empty()) {
		try {
			einput = os::EvdevInput::make(devpath);
		} catch (duds::os::linux::EvdevFileOpenError &) {
			std::cerr << "Failed to open device file " << devpath << std::endl;
			return 2;
		}  catch (duds::os::linux::EvdevInitError &eie) {
			std::cerr << "Failed to initalize libevdev, error code " <<
			*boost::get_error_info<boost::errinfo_errno>(eie) << std::endl;
			return 3;
		}
		einput->usePoller(poller);
		inputPolling = std::thread(&doPoll, std::ref(poller));
		if (grabinput) {
			// wait until enter key released
			while (einput->value(os::EventTypeCode(EV_KEY, KEY_ENTER)) != 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(32));
			}
			if (!einput->grab()) {
				std::cerr << "Failed to grab input device." << std::endl;
			}
		}
		einput->inputConnect(
			os::EventTypeCode(EV_KEY, KEY_ESC),
			std::bind(&signalHandler, 0)
		);
	}

	// load fonts
	graphics::BppStringCacheSptr mfontCache(duds::ui::graphics::BppStringCache::make(
		graphics::BppFont::make(mfontpath)
	));
	graphics::BppStringCacheSptr tfontCache(graphics::BppStringCache::make(
		tfontpath
	));
	// load check icons
	menuicons.load(miconpath);

	{
		// display configuration
		display::BppGraphicDisplaySptr disp;
		duds::hardware::interface::PinConfiguration pc;
		std::shared_ptr<duds::hardware::interface::DigitalPort> port;
		// ST7920
		if (uselcd) {
			boost::property_tree::ptree tree;
			boost::property_tree::read_info(confpath, tree);
			// if an exception is thrown here, the program will terminate without
			// getting to the catch block below; don't know why
			pc.parse(tree.get_child("pins"));
			port = duds::hardware::interface::linux::GpioDevPort::makeConfiguredPort(pc);
			duds::hardware::interface::DigitalPinSet lcdset;
			duds::hardware::interface::ChipSelect lcdsel;
			pc.getPinSetAndSelect(lcdset, lcdsel, lcdname);
			std::shared_ptr<duds::hardware::devices::displays::ST7920> lcd =
				std::make_shared<duds::hardware::devices::displays::ST7920>(
					std::move(lcdset), std::move(lcdsel), dispW, dispH
				);
			lcd->initialize();
			disp = std::move(lcd);
		} else {
			// comsole output
			disp = std::make_shared<duds::hardware::devices::displays::SimulatedBppDisplay>(dispW, dispH);
		}

		// make the menus
		Menus menus(mfontCache->font());
		MenuViews views(menus, mfontCache, einput);

		runtest(disp, std::ref(tfontCache), std::ref(views), std::ref(einput));
	}

	std::cout << "Title font string cache image size: " << tfontCache->bytes() <<
	" bytes in " << tfontCache->strings() << " strings.\n"
	"Menu font string cache image size: " << mfontCache->bytes() << " bytes in " <<
	mfontCache->strings() << " strings." << std::endl;

	if (einput) {
		inputPolling.join();
	}
} catch (...) {
	quit = true;
	std::cerr << "Test failed in main():\n" <<
	boost::current_exception_diagnostic_information() << std::endl;
	return 1;
}
