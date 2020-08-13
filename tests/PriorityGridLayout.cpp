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
 * Test of the PriorityGridLayout and related classes.
 */

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <duds/ui/graphics/PriorityGridLayout.hpp>

// Define this for console output to manually verify image contents.
// It will result in a LOT of output.
//#define CONSOLE_OUT

#ifdef CONSOLE_OUT
#include <duds/hardware/devices/displays/SimulatedBppDisplay.hpp>
#endif

namespace DUG = duds::ui::graphics; // Duds, Ui, Graphics

// --------------------------------------------------------------------------
// Data structures to support and simplify the tests

struct PanelTracker;

/**
 * A panel that renders a simple test pattern and updates a PanelTracker when
 * added to and removed from a PriorityGridLayout.
 */
class TestPanel : public DUG::Panel, public std::enable_shared_from_this<TestPanel> {
	PanelTracker *pt;
public:
	TestPanel(PanelTracker *p) : pt(p) { }
	/**
	 * The rendered image. It is changed by render() whener the dimensions do
	 * not match what is needed for the test. Used after rendering to test that
	 * the image is on the frame.
	 */
	DUG::BppImage img;
	/**
	 * The margins to use in render().
	 */
	DUG::PanelMargins mrg = { 0, 0, 0, 0 };
	/**
	 * The maximum dimensions; used to limit rendered size only if not empty.
	 */
	DUG::ImageDimensions maxDim = { 0, 0 };
	/**
	 * The panel's priority value. Used to generate the test pattern in the
	 * rendered image, and to inform the PanelTracker that the panel was
	 * rendered.
	 */
	unsigned int priority;
	/**
	 * Informs the panel tracker that the panel has been added.
	 */
	virtual void added(DUG::PriorityGridLayout *pgl, unsigned int pri);
	/**
	 * Informs the panel tracker that the panel has been removed.
	 */
	virtual void removing(DUG::PriorityGridLayout *pgl, unsigned int pri);
	/**
	 * Renders a test patteren that has corner marks and includes the panel's
	 * priority value in the interior.
	 */
	virtual const DUG::BppImage *render(
		DUG::ImageLocation &offset,
		DUG::ImageDimensions &dim,
		DUG::PanelMargins &margin,
		int sizeStep
	);
};

typedef std::shared_ptr<TestPanel>  TestPanelSptr;

/**
 * Holds data for a specific panel during tests.
 */
struct TestPanelData {
	/**
	 * The panel; may not be needed.
	 */
	TestPanelSptr panel;
	/**
	 * A copy of the configuration data; may not be needed.
	 */
	DUG::GridLayoutConfig conf;
	/**
	 * The size-step determined by the layout. TestPanel will update this
	 * value in its render() function.
	 */
	int sizeStep;
};

/**
 * Stores information on where a panel should be located on the frame, and
 * expanded to include modifying data for the panel to assist in making
 * test cases.
 * @author  Jeff Jackowski
 */
struct Spot {
	/**
	 * The expected location of the rendered panel. Does not include margins.
	 */
	DUG::ImageLocation loc;
	/**
	 * The expected size of the rendered panel. Does not include margins.
	 */
	DUG::ImageDimensions dim;
	/**
	 * The maximum size of the panel to render; provided to TestPanel.
	 */
	DUG::ImageDimensions maxDim = { 0, 0 };
	/**
	 * The minimum size of the panel to render; changes the value for the first
	 * size-step if not empty.
	 */
	DUG::ImageDimensions minDim = { 0, 0 };
	/**
	 * Margins to use during panel render; provided to TestPanel.
	 */
	DUG::PanelMargins mrg = { 0, 0, 0, 0 };
	/**
	 * Panel priority; used for panel lookup.
	 */
	unsigned int pri;
	/**
	 * Additional flags to add to the panel configuration prior to performing
	 * the layout.
	 */
	DUG::GridLayoutConfig::Flags orFlags = DUG::GridLayoutConfig::Flags::Zero();
	Spot(
		const DUG::ImageLocation &l,
		const DUG::ImageDimensions &d,
		const DUG::ImageDimensions &md,
		const DUG::PanelMargins &m,
		unsigned int p
	) : loc(l), dim(d), maxDim(md), mrg(m), pri(p) { }
	Spot(
		const DUG::ImageLocation &l,
		const DUG::ImageDimensions &d,
		const DUG::PanelMargins &m,
		unsigned int p
	) : loc(l), dim(d), mrg(m), pri(p) { }
	Spot(
		const DUG::ImageLocation &l,
		const DUG::ImageDimensions &d,
		unsigned int p
	) : loc(l), dim(d), pri(p) { }
	Spot(
		const DUG::ImageLocation &l,
		const DUG::ImageDimensions &d,
		const DUG::ImageDimensions &md,
		const DUG::PanelMargins &m,
		unsigned int p,
		DUG::GridLayoutConfig::Flags f
	) : loc(l), dim(d), maxDim(md), mrg(m), pri(p), orFlags(f) { }
	Spot(
		const DUG::ImageLocation &l,
		const DUG::ImageDimensions &d,
		const DUG::ImageDimensions &md,
		const DUG::PanelMargins &m,
		unsigned int p,
		DUG::GridLayoutConfig::Flags f,
		const DUG::ImageDimensions &min
	) : loc(l), dim(d), maxDim(md), minDim(min), mrg(m), pri(p), orFlags(f) { }
	Spot(
		const DUG::ImageLocation &l,
		const DUG::ImageDimensions &d,
		const DUG::ImageDimensions &md,
		const DUG::PanelMargins &m,
		unsigned int p,
		const DUG::ImageDimensions &min
	) : loc(l), dim(d), maxDim(md), minDim(min), mrg(m), pri(p) { }
	Spot(
		const DUG::ImageLocation &l,
		const DUG::ImageDimensions &d,
		const DUG::PanelMargins &m,
		unsigned int p,
		DUG::GridLayoutConfig::Flags f
	) : loc(l), dim(d), mrg(m), pri(p), orFlags(f) { }
	Spot(
		const DUG::ImageLocation &l,
		const DUG::ImageDimensions &d,
		unsigned int p,
		DUG::GridLayoutConfig::Flags f
	) : loc(l), dim(d), pri(p), orFlags(f) { }
};

/**
 * Used to configure a panel for testing.
 */
struct PanelConfig {
	/**
	 * The GridLayoutConfig used for the panel.
	 */
	DUG::GridLayoutConfig glc;
	/**
	 * The panel's priority.
	 */
	unsigned int pri;
};

/**
 * Defines a buch of panels that are something like what might be part of a
 * user interface, but also attempts to use many features of the grid layout.
 */
const PanelConfig screen[] = {
	{ // top right low priority data
		// uses 1/4 width of initial frame size (128 wide)
		{ // GridLayoutConfig
			{ // GridSizeSteps
				{ // GridSizeStep
					{ // ImageDimensions, minimum
						32, 7
					},
					{ // GridLocation
						1, 0
					},
					// GridLayoutConfig::Flags
					DUG::GridLayoutConfig::PanelJustifyRight
				},
			},
			//DUG::GridLayoutConfig::Flags::Zero()
		},
		// priority
		15
	},
	{ // top left header
		{ // GridLayoutConfig
			{ // GridSizeSteps
				{ // GridSizeStep
					{ // ImageDimensions, minimum
						128 - 33, 8
					},
					{ // GridLocation
						0, 0
					},
					// GridLayoutConfig::Flags
				},
				{ // GridSizeStep
					{ // ImageDimensions, minimum
						32, 7
					},
					{ // GridLocation
						1, 0
					},
					// GridLayoutConfig::Flags
					DUG::GridLayoutConfig::PanelJustifyRight
				},
			},
		},
		// priority
		14
	},
	{ // mid-left generally important stuff
		{ // GridLayoutConfig
			{ // GridSizeSteps
				{ // GridSizeStep
					{ // ImageDimensions, minimum
						12 * 8, 16
					},
					{ // GridLocation
						0, 1
					},
					// GridLayoutConfig::Flags
					DUG::GridLayoutConfig::PanelCenter
				},
				// get a bit smaller but stay in the same grid spot
				{ // GridSizeStep
					{ // ImageDimensions, minimum
						12 * 6, 16
					},
					{ // GridLocation
						0, 1
					},
					// GridLayoutConfig::Flags
					DUG::GridLayoutConfig::PanelCenter
				},
				// take over top right header
				{ // GridSizeStep
					{ // ImageDimensions, minimum
						128 - 33, 8
					},
					{ // GridLocation
						0, 0
					},
					// GridLayoutConfig::Flags
					//  justify left & up (defaults)
				},
			},
		},
		// priority
		4
	},
	{ // mid-right extra important stuff that isn't always shown
		{ // GridLayoutConfig
			{ // GridSizeSteps
				{ // GridSizeStep
					{ // ImageDimensions, minimum
						50, 16
					},
					{ // GridLocation
						1, 1
					},
					// GridLayoutConfig::Flags
					DUG::GridLayoutConfig::PanelWidthExpand |
					DUG::GridLayoutConfig::PanelJustifyRight
				},
			},
		},
		// priority
		2
	},
	{ // bottom data
		{ // GridLayoutConfig
			{ // GridSizeSteps
				{ // GridSizeStep
					{ // ImageDimensions, minimum
						64, 8
					},
					{ // GridLocation
						0, 2
					},
					// GridLayoutConfig::Flags
					DUG::GridLayoutConfig::PanelHeightExpand
				},
			},
		},
		// priority
		8
	},
};

/**
 * Keeps track of panel data outside of the layout under test.
 */
struct PanelTracker {
	/**
	 * The layout object being tested.
	 */
	DUG::PriorityGridLayout pgl;
	typedef std::map<TestPanel*, TestPanelData>  PanelMap;
	/**
	 * The panels that have been added to the layout. This is updated by
	 * TestPanel as the panels are added and removed so that the panels can be
	 * tracked outside the layout for this test.
	 */
	PanelMap panels;
	/**
	 * The destination image for the layout. Its contents are not monitored
	 * by this test.
	 */
	DUG::BppImage frame;
	/**
	 * A list of panel priority values that is filled as the panels are rendered.
	 * This allows testing of which panels are rendered.
	 * It needs to be cleared bewteen calls to DUG::PriorityGridLayout::render().
	 */
	std::vector<unsigned int> rendered;
	/**
	 * Configures the output frame image.
	 */
	PanelTracker() : frame(128, 32) {
		frame.clearImage();
		pgl.renderFill(frame.dimensions());
	}
	/**
	 * Makes a new panel for testing without an image.
	 */
	TestPanelSptr makePanel() {
		return std::make_shared<TestPanel>(this);
	}
	/**
	 * Makes all the panels defined in the given PanelConfig array, and gives
	 * them a tiny image.
	 */
	template <std::size_t N>
	void makePanels(const PanelConfig (&pca)[N]) {
		for (const PanelConfig &pc : pca) {
			pgl.add(std::make_shared<TestPanel>(this), pc.glc, pc.pri);
			PanelMap::iterator iter = findPanel(pc.pri);
			BOOST_REQUIRE(iter != panels.end());
			// TestPanel::render() will not make a test pattern unless the image
			// has non-zero size
			iter->first->img.resize(1, 1);
		}
		BOOST_REQUIRE_EQUAL(panels.size(), N);
	}
	/**
	 * Tests if the given panel has been added to the layout.
	 */
	bool panelExists(TestPanel *tp) const {
		return panels.count(tp) > 0;
	}
	/**
	 * Returns the data about a panel; the data is tracked outside of the layout.
	 */
	TestPanelData &panelData(TestPanel *tp) {
		PanelMap::iterator iter = panels.find(tp);
		BOOST_REQUIRE(iter != panels.end());
		return iter->second;
	}
	/**
	 * Returns an iterator to a panel found by priority value.
	 */
	PanelMap::iterator findPanel(unsigned int pri) {
		return std::find_if(
			panels.begin(),
			panels.end(),
			[pri] (auto i) {
				return i.first->priority == pri;
			}
		);
	}
	/**
	 * Returns an iterator to a panel found by priority value.
	 */
	PanelMap::const_iterator findPanel(unsigned int pri) const {
		return std::find_if(
			panels.cbegin(),
			panels.cend(),
			[pri] (auto i) {
				return i.first->priority == pri;
			}
		);
	}
	/**
	 * Tests if a panel with the given priority has been added to the layout.
	 */
	bool priorityExists(unsigned int pri) const {
		PanelMap::const_iterator iter = findPanel(pri);
		return iter != panels.cend();
	}
	/**
	 * Tests if the frame has a panel's image at the given location.
	 */
	bool imageMatch(const Spot &spot) const {
		// find the panel
		PanelMap::const_iterator iter = findPanel(spot.pri);
		BOOST_REQUIRE(iter != panels.cend());
		// make a new image to hold what should be the panel's image
		DUG::BppImage pimg(spot.dim);
		// copy from the frame
		pimg.write(&frame, { 0, 0 }, spot.loc, spot.dim);
		/*
		#ifdef CONSOLE_OUT
		std::cout << "Copy " << pimg.dimensions() << std::endl;
		duds::hardware::devices::displays::SimulatedBppDisplay sd;
		sd.configure(pimg.dimensions());
		sd.write(&pimg);
		#endif
		*/
		// check the contents against the panel's image
		return pimg == iter->first->img;
	}
};

void TestPanel::added(DUG::PriorityGridLayout *pgl, unsigned int pri) {
	pt->panels[this] = { shared_from_this(), pgl->panelConfig(pri), -1 };
	priority = pri;
}

void TestPanel::removing(DUG::PriorityGridLayout *pgl, unsigned int pri) {
	pt->panels.erase(this);
}

const DUG::BppImage *TestPanel::render(
	DUG::ImageLocation &offset,
	DUG::ImageDimensions &dim,
	DUG::PanelMargins &margin,
	int sizeStep
) {
	pt->panels[this].sizeStep = sizeStep;
	BOOST_CHECK(dim.fits(pt->panels[this].conf.sizes[sizeStep].minDim));
	pt->rendered.push_back(priority);
	if (img.empty()) {
		return nullptr;
	} else {
		// set the margins
		margin = mrg;
		// remove margins from the dimensions to make everything fit
		dim.w -= mrg.l + mrg.r;
		dim.h -= mrg.t + mrg.b;
		// limit size
		if (!maxDim.empty()) {
			dim = maxDim.clip(dim);
		}
		// if this check fails, the test data is bad; the test should allow the
		// image to fit
		BOOST_REQUIRE((dim.w > 0) && (dim.h > 0));
		// different size from the panel's image?
		if (img.dimensions() != dim) {
			// resize the image
			img.resize(dim);
			// render a test image
			img.clearImage();
			// put the lower nibble of the panel priority into the image interior
			DUG::BppImage::PixelBlock pblk;
			std::memset(&pblk, priority | (priority << 4), sizeof(pblk));
			img.patternLines(2, dim.h - 4, pblk);
			// write corners
			// top left
			img.drawBox(0, 0, 3, 1);
			img.drawBox(0, 1, 1, 2);
			img.drawBox(1, 1, 2, 2, false);
			// bottom left
			img.drawBox(0, dim.h - 1, 3, 1);
			img.drawBox(0, dim.h - 3, 1, 2);
			img.drawBox(1, dim.h - 3, 2, 2, false);
			// top right
			img.drawBox(dim.w - 3, 0, 3, 1);
			img.drawBox(dim.w - 1, 1, 1, 2);
			img.drawBox(dim.w - 3, 1, 2, 2, false);
			// bottom right
			img.drawBox(dim.w - 3, dim.h - 1, 3, 1);
			img.drawBox(dim.w - 1, dim.h - 3, 1, 2);
			img.drawBox(dim.w - 3, dim.h - 3, 2, 2, false);
			// left side
			img.drawBox(0, 3, 2, dim.h - 6, false);
			// right side
			img.drawBox(dim.w - 2, 3, 2, dim.h - 6, false);
			// console output to verify good result when working on the test code
			#ifdef CONSOLE_OUT
			std::cout << "Test panel " << priority << " image " << dim << std::endl;
			duds::hardware::devices::displays::SimulatedBppDisplay sd;
			sd.configure(dim);
			sd.write(&img);
			#endif
		}
		return &img;
	}
}


// --------------------------------------------------------------------------
// The tests

BOOST_FIXTURE_TEST_SUITE(PriorityGridLayout_FixtureTests, PanelTracker)

BOOST_AUTO_TEST_CASE(PriorityGridLayout_Simple) {
	DUG::GridSizeStep gss = {
		{ 16, 16 },
		{ 0, 0 },
		DUG::GridLayoutConfig::Flags::Zero()
	};
	TestPanelSptr tps = makePanel();
	unsigned int pri = pgl.add(tps, gss);
	BOOST_CHECK_EQUAL(pri, 1);
	BOOST_CHECK(priorityExists(pri));
	BOOST_CHECK(findPanel(pri)->first == tps.get());
	BOOST_CHECK(!pgl.add(tps, gss, pri));
	BOOST_REQUIRE_EQUAL(pgl.layout(), 1);
	pgl.render(&frame);
	BOOST_REQUIRE(!rendered.empty());
	BOOST_CHECK_EQUAL(rendered.front(), pri);
	BOOST_CHECK_EQUAL(panelData(tps.get()).sizeStep, 0);
	// check frame; should be all zeros
	for (auto blk : frame.data()) {
		BOOST_CHECK_EQUAL(blk, 0);
	}
	// give the panel an image
	tps->img.resize(1, 1);
	// render again
	frame.clearImage();
	pgl.render(&frame);
	// check panel for test image
	BOOST_CHECK(imageMatch({ { 0, 0 }, gss.minDim, pri }));
	#ifdef CONSOLE_OUT
	{
		std::cout << "Frame " << frame.dimensions() << std::endl;
		duds::hardware::devices::displays::SimulatedBppDisplay sd;
		sd.configure(frame.dimensions());
		sd.write(&frame);
	}
	#endif
	// change panel to center it on the frame
	pgl.panelConfig(pri).flags =
		DUG::GridLayoutConfig::PanelExpand | DUG::GridLayoutConfig::PanelCenter;
	BOOST_REQUIRE_EQUAL(pgl.layout(), 1);
	pgl.render(&frame);
	BOOST_CHECK(imageMatch({ { 0, 0 }, frame.dimensions(), pri }));
	#ifdef CONSOLE_OUT
	{
		std::cout << "Frame " << frame.dimensions() << std::endl;
		duds::hardware::devices::displays::SimulatedBppDisplay sd;
		sd.configure(frame.dimensions());
		sd.write(&frame);
	}
	#endif
	// limit the panel's size
	tps->maxDim = { 8, 8 };
	frame.clearImage();
	pgl.render(&frame);
	BOOST_CHECK(imageMatch({
		{
			std::int16_t((frame.width() - tps->maxDim.w) / 2),
			std::int16_t((frame.height() - tps->maxDim.h) / 2)
		},
		tps->maxDim,
		pri
	}));
	#ifdef CONSOLE_OUT
	{
		std::cout << "Frame " << frame.dimensions() << std::endl;
		duds::hardware::devices::displays::SimulatedBppDisplay sd;
		sd.configure(frame.dimensions());
		sd.write(&frame);
	}
	#endif
	// adjust margins
	tps->mrg = { 8, 0, 8, 0 };
	frame.clearImage();
	pgl.render(&frame);
	BOOST_CHECK(imageMatch({
		{
			std::int16_t((frame.width() - tps->maxDim.w) / 2 + 4),
			std::int16_t((frame.height() - tps->maxDim.h) / 2 + 4)
		},
		tps->maxDim,
		pri
	}));
	#ifdef CONSOLE_OUT
	{
		std::cout << "Frame " << frame.dimensions() << std::endl;
		duds::hardware::devices::displays::SimulatedBppDisplay sd;
		sd.configure(frame.dimensions());
		sd.write(&frame);
	}
	#endif
	pgl.remove(pri);
	BOOST_CHECK(!priorityExists(pri));
}

BOOST_AUTO_TEST_SUITE_END()


// --------------------------------------------------------------------------
// More test data

struct LayoutTest {
	/**
	 * Identifies the test on failure, and controls the frame background bit
	 * value (line & 1).
	 */
	int line;
	/**
	 * Defines the size of the frame.
	 */
	DUG::ImageDimensions frameDim;
	/**
	 * The expected location and size of each panel. Use empty dimensions for
	 * panels that are not shown.
	 */
	Spot panelPos[std::extent_v<decltype(screen)>];
};

const LayoutTest layoutTests[] = {
	{ // test screen config with defaults
		__LINE__,  // test ID
		{ 128, 32 },  // frame size
		{ // panel data (Spot[])
			{ // top right low priority data
				{ 128 - 33, 0 },   // frame location
				{ 32, 8 },         // size
				15                 // priority
			},
			{ // top left header
				{ 0, 0 },          // frame location
				{ 128 - 33, 8 },   // size
				14                 // priority
			},
			{ // mid-left generally important stuff
				{ 0, 8 },          // frame location
				{ 12 * 8, 16 },    // size
				4                  // priority
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				2
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				8
			},
		}
	},
	{ // test smaller width preventing panel 15 from being placed
		__LINE__,  // test ID
		{ 100, 32 },  // frame size
		{ // panel data (Spot[])
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				15                 // priority
			},
			{ // top left header
				{ 0, 0 },          // frame location
				{ 128 - 33, 8 },   // size
				14                 // priority
			},
			{ // mid-left generally important stuff
				{ 0, 8 },          // frame location
				{ 12 * 8, 16 },    // size
				4                  // priority
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				2
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				8
			},
		}
	},
	{ // test width expand
		__LINE__,  // test ID
		{ 128, 32 },  // frame size
		{ // panel data (Spot[])
			{ // top right low priority data
				{ 128 - 33, 0 },   // frame location
				{ 33, 8 },         // size
				15,                // priority
				DUG::GridLayoutConfig::PanelWidthExpand
			},
			{ // top left header
				{ 0, 0 },          // frame location
				{ 128 - 33, 8 },   // size
				14                 // priority
			},
			{ // mid-left generally important stuff
				{ 0, 8 },          // frame location
				{ 128, 16 },       // size
				4,                 // priority
				DUG::GridLayoutConfig::PanelWidthExpand
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				2
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				8
			},
		}
	},
	{ // test justify right, center horizontally, and margins
		__LINE__,  // test ID
		{ 128, 32 },  // frame size
		{ // panel data (Spot[])
			{ // top right low priority data
				{ 128 - 32, 0 },   // frame location
				{ 32, 7 },         // size
				{ 32, 7 },         // max size
				{ 0, 0, 0, 0 },    // margin (used in rendering)
				15,                // priority
				DUG::GridLayoutConfig::PanelWidthExpand
			},
			{ // top left header
				{ 0, 0 },          // frame location
				{ 128 - 33, 7 },   // size
				{ 0, 0, 0, 1 },    // margin (used in rendering)
				14                 // priority
			},
			{ // mid-left generally important stuff
				{ (128 - 12 * 8) / 2, 8 },    // frame location
				{ 12 * 8, 16 },    // size
				{ 12 * 8, 16 },    // max size
				{ 0, 0, 0, 0 },    // margin (used in rendering)
				4,                 // priority
				DUG::GridLayoutConfig::PanelWidthExpand
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				2
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				8
			},
		}
	},
	{ // test height expand
		__LINE__,  // test ID
		{ 128, 32 },  // frame size
		{ // panel data (Spot[])
			{ // top right low priority data
				{ 128 - 32, 0 },   // frame location
				{ 32, 7 },         // size
				{ 32, 7 },         // max size
				{ 0, 0, 0, 0 },    // margin (used in rendering)
				15,                // priority
				DUG::GridLayoutConfig::PanelWidthExpand
			},
			{ // top left header
				{ 0, 0 },          // frame location
				{ 128 - 33, 7 },   // size
				{ 0, 0, 0, 1 },    // margin (used in rendering)
				14                 // priority
			},
			{ // mid-left generally important stuff
				{ (128 - 12 * 8) / 2, 8 },    // frame location
				{ 12 * 8, 24 },    // size
				{ 12 * 8, 32 },    // max size
				{ 0, 0, 0, 0 },    // margin (used in rendering)
				4,                 // priority
				DUG::GridLayoutConfig::PanelExpand
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				2
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				8
			},
		}
	},
	{ // test centering, both axes
		__LINE__,  // test ID
		{ 128, 32 },  // frame size
		{ // panel data (Spot[])
			{ // top right low priority data
				{ 128 - 32, 0 },   // frame location
				{ 32, 7 },         // size
				{ 32, 7 },         // max size
				{ 0, 0, 0, 0 },    // margin (used in rendering)
				15,                // priority
				DUG::GridLayoutConfig::PanelWidthExpand
			},
			{ // top left header
				{ 0, 0 },          // frame location
				{ 128 - 33, 7 },   // size
				{ 0, 0, 0, 1 },    // margin (used in rendering)
				14                 // priority
			},
			{ // mid-left generally important stuff
				{ (128 - 12 * 8) / 2, 8 + 4 },    // frame location
				{ 12 * 8, 16 },    // size
				{ 12 * 8, 16 },    // max size
				{ 0, 0, 0, 0 },    // margin (used in rendering)
				4,                 // priority
				DUG::GridLayoutConfig::PanelExpand
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				2
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				8
			},
		}
	},
	{ // test centering with margins
		__LINE__,  // test ID
		{ 128, 32 },  // frame size
		{ // panel data (Spot[])
			{ // top right low priority data
				{ 128 - 32, 0 },   // frame location
				{ 32, 7 },         // size
				{ 32, 7 },         // max size
				{ 0, 0, 0, 0 },    // margin (used in rendering)
				15,                // priority
				DUG::GridLayoutConfig::PanelWidthExpand
			},
			{ // top left header
				{ 0, 0 },          // frame location
				{ 128 - 33, 7 },   // size
				{ 0, 0, 0, 1 },    // margin (used in rendering)
				14                 // priority
			},
			{ // mid-left generally important stuff
				{ (128 - (12 * 8 + 4)) / 2 + 1, 8 + 4 - 1 }, // frame location
				{ 12 * 8, 16 },    // size
				{ 12 * 8, 16 },    // max size
				{ 1, 3, 0, 2 },    // margin (used in rendering)
				4,                 // priority
				DUG::GridLayoutConfig::PanelExpand
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				2
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				8
			},
		}
	},
	{ // test priority based selection of size-step
		__LINE__,  // test ID
		{ 128, 32 },  // frame size
		{ // panel data (Spot[])
			{ // top right low priority data
				{ 128 - 32, 0 },   // frame location
				{ 32, 7 },         // size
				{ 32, 7 },         // max size
				{ 0, 0, 0, 0 },    // margin (used in rendering)
				15,                // priority
				DUG::GridLayoutConfig::PanelWidthExpand
			},
			{ // top left header
				{ 0, 0 },          // frame location
				{ 128 - 33, 7 },   // size
				{ 0, 0, 0, 1 },    // margin (used in rendering)
				14                 // priority
			},
			{ // mid-left generally important stuff
				{ 0, 8 },          // frame location
				{ 12 * 6, 16 },    // size
				{ 12 * 6, 16 },    // max size
				{ 0, 0, 0, 0 },    // margin (used in rendering)
				4,                 // priority
				//DUG::GridLayoutConfig::PanelExpand
			},
			{ // extra important stuff to the right of above panel
				{ 12 * 6, 8  },        // frame location
				{ 128 - 12 * 6, 16 },  // size
				2,                     // priority
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				8
			},
		}
	},
	{ // test two panels in same row using width expand
		__LINE__,  // test ID
		{ 128, 32 },  // frame size
		{ // panel data (Spot[])
			{ // top right low priority data
				{ 128 - 32, 0 },   // frame location
				{ 32, 7 },         // size
				{ 32, 7 },         // max size
				{ 0, 0, 0, 0 },    // margin (used in rendering)
				15,                // priority
				DUG::GridLayoutConfig::PanelWidthExpand
			},
			{ // top left header
				{ 0, 0 },          // frame location
				{ 128 - 33, 7 },   // size
				{ 0, 0, 0, 1 },    // margin (used in rendering)
				14                 // priority
			},
			{ // mid-left generally important stuff
				{ 0, 8 },           // frame location
				{ 12 * 6 + 3, 16 }, // size
				{ 128, 16 },        // max size
				{ 0, 0, 0, 0 },     // margin (used in rendering)
				4,                  // priority
				DUG::GridLayoutConfig::PanelWidthExpand
			},
			{ // extra important stuff to the right of above panel
				{ 12 * 6 + 3, 8  },       // frame location
				{ 128 - 12 * 6 - 3, 16 }, // size
				2,                        // priority
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				8
			},
		}
	},
	{ // test two panels in same row using width expand, one with max size & marg
		__LINE__,  // test ID
		{ 128, 32 },  // frame size
		{ // panel data (Spot[])
			{ // top right low priority data
				{ 128 - 32, 0 },   // frame location
				{ 32, 7 },         // size
				{ 32, 7 },         // max size
				{ 0, 0, 0, 0 },    // margin (used in rendering)
				15,                // priority
				DUG::GridLayoutConfig::PanelWidthExpand
			},
			{ // top left header
				{ 0, 0 },          // frame location
				{ 128 - 33, 7 },   // size
				{ 0, 0, 0, 1 },    // margin (used in rendering)
				14                 // priority
			},
			{ // mid-left generally important stuff
				{ 0, 8 + 4 },        // frame location
				{ 12 * 6, 16 },      // size
				{ 12 * 6, 16 },      // max size
				{ 0, 0, 0, 0 },      // margin (used in rendering)
				4,                   // priority
				DUG::GridLayoutConfig::PanelHeightExpand
			},
			{ // extra important stuff to the right of above panel
				// height is expanded because of above panel
				{ 128 - 50, 8 },       // frame location
				{ 50, 18 },            // size
				{ 50, 18 },            // max size
				{ 0, 0, 0, 0 },        // margin (used in rendering)
				2,                     // priority
			},
			{ // hidden
				{ 0, 0 },
				{ 0, 0 },
				8
			},
		}
	},
	{ // add bottom panel; test height expand difference on panel 2 compared to
		// above test
		__LINE__,  // test ID
		{ 128, 32 },  // frame size
		{ // panel data (Spot[])
			{ // top right low priority data
				{ 128 - 32, 0 },   // frame location
				{ 32, 7 },         // size
				{ 32, 7 },         // max size
				{ 0, 0, 0, 0 },    // margin (used in rendering)
				15,                // priority
				DUG::GridLayoutConfig::PanelWidthExpand
			},
			{ // top left header
				{ 0, 0 },          // frame location
				{ 128 - 33, 7 },   // size
				{ 0, 0, 0, 1 },    // margin (used in rendering)
				14                 // priority
			},
			{ // mid-left generally important stuff
				{ 0, 8 },            // frame location
				{ 12 * 6, 16 },      // size
				{ 12 * 6, 16 },      // max size
				{ 0, 0, 0, 0 },      // margin (used in rendering)
				4,                   // priority
				// height not expanded because of panel 8
				DUG::GridLayoutConfig::PanelHeightExpand
			},
			{ // extra important stuff to the right of above panel
				{ 128 - 50, 8 },       // frame location
				{ 50, 16 },            // size
				{ 50, 18 },            // max size
				{ 0, 0, 0, 0 },        // margin (used in rendering)
				2,                     // priority
			},
			{ // bottom data
				{ 0, 8 + 16 },
				{ 64, 8 },
				8
			},
		}
	},
	{ // another margin test
		__LINE__,  // test ID
		{ 128, 32 },  // frame size
		{ // panel data (Spot[])
			{ // top right low priority data
				{ 128 - 32, 0 },   // frame location
				{ 32, 7 },         // size
				{ 32, 7 },         // max size
				{ 0, 0, 0, 0 },    // margin (used in rendering)
				15,                // priority
				DUG::GridLayoutConfig::PanelWidthExpand
			},
			{ // top left header
				{ 0, 0 },          // frame location
				{ 128 - 33, 7 },   // size
				{ 0, 0, 0, 1 },    // margin (used in rendering)
				14                 // priority
			},
			{ // mid-left generally important stuff
				{ 1, 8 },            // frame location
				{ 12 * 6 - 2, 16 },  // size; smaller from margin
				{ 12 * 6, 16 },      // max size
				{ 1, 1, 0, 0 },      // margin (used in rendering)
				4,                   // priority
				// height not expanded because of panel 8
				DUG::GridLayoutConfig::PanelHeightExpand
			},
			{ // extra important stuff to the right of above panel
				{ 128 - 50 - 1, 8 },       // frame location
				{ 50, 16 },            // size
				{ 50, 18 },            // max size
				{ 1, 1, 0, 0 },        // margin (used in rendering)
				2,                     // priority
			},
			{ // bottom data
				{ 0, 8 + 16 },
				{ 64, 8 },
				8
			},
		}
	},
	{ // make panel 2 larger so that panel 4 cannot fit on the same row causing
		// a cascade of grid position changes
		__LINE__,  // test ID
		{ 128, 32 },  // frame size
		{ // panel data (Spot[])
			{ // top right low priority data; removed
				{ 0, 0 },
				{ 0, 0 },
				15
			},
			{ // top left header; moved to top right
				{ 128 - 32, 0 },   // frame location
				{ 32, 7 },         // size
				{ 32, 7 },         // max size
				{ 0, 0, 0, 0 },    // margin (used in rendering)
				14,                // priority
				DUG::GridLayoutConfig::PanelWidthExpand
			},
			{ // mid-left generally important stuff; moved to top left
				{ 0, 0 },          // frame location
				{ 128 - 33, 7 },   // size
				{ 0, 0, 0, 1 },    // margin (used in rendering)
				4                  // priority
			},
			{ // extra important stuff to the right of above panel
				{ 128 - 96, 8 },   // frame location
				{ 96, 16 },            // size
				{ 96, 16 },            // max size
				{ 0, 0, 0, 0 },        // margin (used in rendering)
				2,                     // priority
				{ 64, 16 }             // min size
			},
			{ // bottom data
				{ 0, 8 + 16 },
				{ 64, 8 },
				8
			},
		}
	},
};

// needed for output when a test fails
std::ostream &operator << (std::ostream &os, const LayoutTest &lt) {
	os << "LayoutTest: " << lt.line;
	return os;
}

// BOOST_DATA_TEST_CASE_F reverses the ordering of the fixture type and test
// name from BOOST_FIXTURE_TEST_SUITE   Boo!
BOOST_DATA_TEST_CASE_F(
	PanelTracker,
	PriorityGridLayout_LayoutTests,
	boost::unit_test::data::make(layoutTests)
) {
	#ifdef CONSOLE_OUT
	std::cout << "\n----- Test frame " << sample.line <<  " -----" << std::endl;
	#endif
	makePanels(screen);
	if (frame.dimensions() != sample.frameDim) {
		frame.resize(sample.frameDim);
		frame.clearImage();
		pgl.renderFill(frame.dimensions());
	}
	int shown = 0;
	// configure margins & hidden/not hidden
	for (auto pspot : sample.panelPos) {
		// shown?
		if (!pspot.dim.empty()) {
			++shown;
			// configure max dimensions, margins, and flags
			PanelMap::iterator iter = findPanel(pspot.pri);
			iter->first->maxDim = pspot.maxDim;
			iter->first->mrg = pspot.mrg;
			pgl.panelConfig(pspot.pri).flags |= pspot.orFlags;
			if (!pspot.minDim.empty()) {
				pgl.panelConfig(pspot.pri).sizes[0].minDim = pspot.minDim;
			}
		} else if (pspot.pri < 15) {
			// set hidden flag, save for panel 15
			pgl.panelConfig(pspot.pri).flags |= DUG::GridLayoutConfig::PanelHidden;
		}
	}
	BOOST_REQUIRE_EQUAL(pgl.layout(), shown);
	pgl.render(&frame);
	#ifdef CONSOLE_OUT
	{
		std::cout << "Frame for test " << sample.line << ' ' << frame.dimensions()
		<< std::endl;
		duds::hardware::devices::displays::SimulatedBppDisplay sd;
		sd.configure(frame.dimensions());
		sd.write(&frame);
	}
	#endif
	for (auto pspot : sample.panelPos) {
		if (!pspot.dim.empty()) {
			BOOST_TEST(imageMatch(pspot), "Failed on panel priority " << pspot.pri);
		}
	}
}
