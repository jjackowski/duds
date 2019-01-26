/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
/**
 * @file
 * Test of the menu infrastructure.
 */

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <duds/ui/menu/MenuAccess.hpp>
#include <duds/ui/menu/MenuOutputViewAccess.hpp>
#include <duds/ui/menu/GenericMenuItem.hpp>

#include <iostream>

namespace DM = duds::ui::menu;

void inc(int &i) {
	++i;
}

BOOST_AUTO_TEST_SUITE(Menu)

// if something here isn't working, the tests that follow will likely fail to
// initialize their data and may produce useless results
BOOST_AUTO_TEST_CASE(MenuBasics) {
	DM::MenuSptr menu(DM::Menu::make("Hi"));
	BOOST_CHECK_EQUAL(menu->title(), "Hi");
	BOOST_CHECK_EQUAL(menu->size(), 0);
	BOOST_CHECK_EQUAL(menu->haveToggles(), false);
	DM::GenericMenuItemSptr item(DM::GenericMenuItem::make("Item"));
	BOOST_CHECK_EQUAL(item->menu(), nullptr);
	int val = 0;
	item->choseConnect(std::bind(inc, std::ref(val)));
	{
		DM::MenuAccess ma(menu);
		// lock is recursive, so this must not fail
		DM::MenuAccess ma0(menu);
		BOOST_CHECK_EQUAL(ma.size(), 0);
		BOOST_CHECK_EQUAL(ma0.size(), 0);
		ma.append(item);
		BOOST_REQUIRE_EQUAL(ma.size(), 1);
		BOOST_REQUIRE_EQUAL(ma0.size(), 1);
		BOOST_CHECK_EQUAL(ma.item(0), item);
		BOOST_CHECK_EQUAL(ma.item(0)->label(), "Item");
		BOOST_CHECK_EQUAL(ma.item(0)->menu(), menu.get());
		BOOST_CHECK_THROW(ma.item(1), DM::MenuBoundsError);
	}
	BOOST_REQUIRE_EQUAL(menu->size(), 1);
	DM::MenuViewSptr view = DM::MenuView::make(menu);
	BOOST_CHECK_EQUAL(view->selectedIndex(), 0);
	// chose the menu item
	view->chose();
	// chose function has not yet been called
	BOOST_CHECK_EQUAL(val, 0);
	DM::MenuOutputViewSptr outv = DM::MenuOutputView::make(view, 4);
	// visible list
	{
		DM::MenuOutputViewAccess acc(outv);
		// view was changed above
		BOOST_CHECK_EQUAL(acc.changed(), true);
		// item should have been chosen
		BOOST_CHECK_EQUAL(val, 1);
		// one visible item
		BOOST_CHECK_EQUAL(acc.size(), 1);
		// the one item is selected
		BOOST_CHECK_EQUAL(acc.selected(), 0);
		// item is not a toggle
		BOOST_CHECK_EQUAL(acc.haveToggles(), false);
		DM::MenuVisibleList::const_iterator iter = acc.begin();
		BOOST_CHECK_EQUAL(*iter, item.get());
		BOOST_CHECK_EQUAL(*acc.selectedIter(), item.get());
		++iter;
		BOOST_CHECK(iter == acc.end()); // cannot output iterators to ostream
	}
	// no change
	{
		DM::MenuOutputViewAccess acc(outv);
		// view was not changed
		BOOST_CHECK_EQUAL(acc.changed(), false);
	}
}

BOOST_AUTO_TEST_SUITE_END()

// Holds an index number used for checking the order of menu items. Order
// checks are needed for tests of inserting and removing items, as well as
// checks for which items a MenuSunview lists as currently visible.
class IndexedItem : public DM::GenericMenuItem {
	int idx;
public:
	IndexedItem(const std::string &label, int i) :
		DM::GenericMenuItem(label), idx(i) { }
	IndexedItem(const std::string &label, Flags flags, int i) :
		DM::GenericMenuItem(label, flags), idx(i) { }
	int index() const {
		return idx;
	}
};

typedef std::shared_ptr<IndexedItem>  IndexedItemSptr;

// A complex menu setup to support a variety of tests.
struct MenuFixture {
	DM::MenuSptr menu;
	DM::MenuViewSptr viewA, viewB;
	// outvAA and outvAB will use viewA, while outvB will use viewB
	DM::MenuOutputViewSptr outvAA, outvAB, outvB;
	// All counts start at zero and increment when the corresponding menu item
	// is chosen. Array is larger than needed to support tests that add more
	// menu items.
	int counts[24];
	MenuFixture() :
		menu(DM::Menu::make("Fixture menu")),
		viewA(DM::MenuView::make(menu)),
		viewB(DM::MenuView::make(menu)),
		outvAA(DM::MenuOutputView::make(viewA, 4)),
		outvAB(DM::MenuOutputView::make(viewA, 6)),
		outvB(DM::MenuOutputView::make(viewB, 8)),
		counts { 0 }
	{
		DM::MenuAccess acc(menu);
		// add some menu items
		std::ostringstream oss;
		oss << "Item ";
		for (int loop = 0; loop < 16; ++loop) {
			oss << loop;
			IndexedItemSptr item(
				std::make_shared<IndexedItem>(
					oss.str(),
					(
						// all even items will be toggles
						!(loop & 1) ?
							DM::MenuItem::Toggle :
							DM::MenuItem::Flags(0)
					) | (
						// ever other even item, starting with 0, will be
						// be toggled on; others will be toggled off
						!(loop & 3) ?
							DM::MenuItem::ToggledOn :
							DM::MenuItem::Flags(0)
					) | (
						// every third item, starting with 0, will have a value
						!(loop % 3) ?
							DM::MenuItem::HasValue :
							DM::MenuItem::Flags(0)
					),
					// put loop index into the item for later item ordering checks
					loop
				)
			);
			oss.seekp(5);
			item->choseConnect(std::bind(inc, std::ref(counts[loop])));
			acc.append(std::move(item));
		}
	}
};

BOOST_FIXTURE_TEST_SUITE(Menu_FixtureTests, MenuFixture)

// test the results of MenuFixture::MenuFixture()
BOOST_AUTO_TEST_CASE(FixtureInit) {
	{
		DM::MenuAccess acc(menu);
		BOOST_CHECK_EQUAL(menu->size(), 16);
		int ui = menu->updateIndex();
		std::istringstream iss;
		for (int loop = 0; loop < 16; ++loop) {
			IndexedItemSptr item = std::dynamic_pointer_cast<IndexedItem>(
				acc.item(loop)
			);
			BOOST_CHECK_EQUAL(item->index(), loop);
			iss.str(item->label());
			iss.seekg(0);
			std::string lbl;
			int pos;
			iss >> lbl >> pos;
			BOOST_CHECK_EQUAL(lbl, "Item");
			BOOST_CHECK_EQUAL(pos, loop);
			if (loop & 1) {
				BOOST_CHECK_EQUAL(item->isToggle(), false);
				BOOST_CHECK_THROW(
					item->setToggle(),
					DM::MenuItemNotAToggle
				);
			} else {
				BOOST_CHECK_EQUAL(item->isToggle(), true);
				int ui = menu->updateIndex();
				BOOST_CHECK_NO_THROW(item->changeToggle(item->isToggledOn()));
				// no change occured
				BOOST_CHECK_EQUAL(menu->updateIndex(), ui);
				if (loop & 3) {
					BOOST_CHECK_EQUAL(item->isToggledOn(), false);
				} else {
					BOOST_CHECK_EQUAL(item->isToggledOn(), true);
				}
			}
			if (!(loop % 3)) {
				BOOST_CHECK_EQUAL(item->hasValue(), true);
				BOOST_CHECK_EQUAL(item->value().empty(), true);
			}
		}
		// the menu was not changed
		BOOST_CHECK_EQUAL(menu->updateIndex(), ui);
	}
	{
		DM::MenuOutputViewAccess acc(outvAA);
		BOOST_CHECK_EQUAL(acc.selected(), 0);
		BOOST_CHECK(acc.begin() == acc.selectedIter());
		BOOST_CHECK_EQUAL((*acc.selectedIter())->label(), "Item 0");
		BOOST_CHECK_EQUAL(acc.haveToggles(), true);
		BOOST_CHECK_EQUAL(acc.maxVisible(), 4);
	}
}

BOOST_AUTO_TEST_CASE(Toggles) {
	int ui = menu->updateIndex();
	// change all toggle states to clear
	{
		DM::MenuAccess acc(menu);
		for (int loop = 0; loop < 16; loop += 2) {
			BOOST_CHECK_NO_THROW(acc.clearToggle(loop));
		}
	}
	// only 4 items were changed; other 4 were already clear
	BOOST_CHECK_EQUAL(menu->updateIndex(), ui + 4);
	// change all toggle states to set
	{
		DM::MenuAccess acc(menu);
		for (int loop = 0; loop < 16; loop += 2) {
			DM::MenuItemSptr item = acc.item(loop);
			BOOST_CHECK_EQUAL(item->isToggledOn(), false);
			BOOST_CHECK_NO_THROW(item->setToggle());
			BOOST_CHECK_EQUAL(item->isToggledOn(), true);
		}
	}
	// 8 items changed since last update index check
	BOOST_CHECK_EQUAL(menu->updateIndex(), ui + 12);
}

BOOST_AUTO_TEST_CASE(Values) {
	int ui = menu->updateIndex();
	// change all values to a count
	{
		DM::MenuAccess acc(menu);
		std::ostringstream oss;
		for (int loop = 0; loop < 16; loop += 3) {
			oss << loop;
			BOOST_CHECK_NO_THROW(acc.value(loop, oss.str()));
			oss.seekp(0);
		}
	}
	// 6 items were changed
	BOOST_CHECK_EQUAL(menu->updateIndex(), ui + 6);
	// check values
	{
		DM::MenuAccess acc(menu);
		std::istringstream iss;
		for (int loop = 0; loop < 16; loop += 3) {
			DM::MenuItemSptr item = acc.item(loop);
			iss.str(item->value());
			int v;
			iss >> v;
			iss.seekg(0);
			BOOST_CHECK_EQUAL(v, loop);
		}
	}
}

BOOST_AUTO_TEST_CASE(Visibility) {
	{
		DM::MenuOutputViewAccess acc(outvAA);
		BOOST_CHECK_EQUAL(acc.size(), 4);
		DM::MenuVisibleList::const_iterator iter = acc.begin();
		int cnt = 0;
		while (iter != acc.end()) {
			IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
			BOOST_CHECK_EQUAL(item->index(), cnt);
			++cnt;
			++iter;
		}
		// only 4 items should be visible
		BOOST_CHECK_EQUAL(cnt, 4);
	}
	// adjust the selection
	viewA->jump(10);
	{
		DM::MenuOutputViewAccess acc(outvAA);
		BOOST_CHECK_EQUAL(acc.size(), 4);
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		BOOST_CHECK_EQUAL(item->index(), 10);
		iter = acc.begin();
		int cnt = 9; // two items in direction of change from selection
		while (iter != acc.end()) {
			item = dynamic_cast<IndexedItem*>(*iter);
			BOOST_CHECK_EQUAL(item->index(), cnt);
			++cnt;
			++iter;
		}
		// 12 is the last item visible; was incremented one past in the loop
		BOOST_CHECK_EQUAL(cnt, 13);
	}
	// selection change should not have altered viewB or outvB
	{
		DM::MenuOutputViewAccess acc(outvB);
		BOOST_CHECK_EQUAL(acc.size(), 8);
		// move selection
		viewB->backward(1);
		// selection should still be on zero until acc is destructed
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		BOOST_CHECK_EQUAL(item->index(), 0);
		iter = acc.begin();
		int cnt = 0;
		while (iter != acc.end()) {
			IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
			BOOST_CHECK_EQUAL(item->index(), cnt);
			++cnt;
			++iter;
		}
		// only 8 items should be visible
		BOOST_CHECK_EQUAL(cnt, 8);
	}
	// check moved selection on view B
	{
		DM::MenuOutputViewAccess acc(outvB);
		BOOST_CHECK_EQUAL(acc.size(), 8);
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		BOOST_CHECK_EQUAL(item->index(), 1);
	}
	// adjust the selection to the end
	viewA->backward(5);
	{
		DM::MenuOutputViewAccess acc(outvAA);
		BOOST_CHECK_EQUAL(acc.size(), 4);
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		BOOST_CHECK_EQUAL(item->index(), 15);
		iter = acc.begin();
		int cnt = 12; // two items in direction of change from selection
		while (iter != acc.end()) {
			item = dynamic_cast<IndexedItem*>(*iter);
			BOOST_CHECK_EQUAL(item->index(), cnt);
			++cnt;
			++iter;
		}
		// 15 is the last item visible; was incremented one past in the loop
		BOOST_CHECK_EQUAL(cnt, 16);
	}
	// adjust the selection past the end; should wrap to start
	viewA->backward(5);
	{
		DM::MenuOutputViewAccess acc(outvAA);
		BOOST_CHECK_EQUAL(acc.size(), 4);
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		BOOST_CHECK_EQUAL(item->index(), 0);
	}
	// adjust the selection past the start; should wrap to end
	viewA->forward(10);
	{
		DM::MenuOutputViewAccess acc(outvAA);
		BOOST_CHECK_EQUAL(acc.size(), 4);
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		BOOST_CHECK_EQUAL(item->index(), 15);
	}
	// adjust the selection near the start
	viewA->forward(10);
	{
		DM::MenuOutputViewAccess acc(outvAA);
		BOOST_CHECK_EQUAL(acc.size(), 4);
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		BOOST_CHECK_EQUAL(item->index(), 5);
		iter = acc.begin();
		int cnt = 3; // two items in direction of change from selection
		while (iter != acc.end()) {
			item = dynamic_cast<IndexedItem*>(*iter);
			BOOST_CHECK_EQUAL(item->index(), cnt);
			++cnt;
			++iter;
		}
		// 6 is the last item visible; was incremented one past in the loop
		BOOST_CHECK_EQUAL(cnt, 7);
	}
}

// change enabled and visible flags to test change in visible items
BOOST_AUTO_TEST_CASE(Visibility_Change) {
	// change several items to be disabled
	{
		DM::MenuAccess acc(menu);
		for (int loop = 0; loop < 2; ++loop) {
			BOOST_CHECK_NO_THROW(acc.disable(loop));
		}
	}
	// the disabled change should not alter what is visible
	{
		DM::MenuOutputViewAccess acc(outvAA);
		BOOST_CHECK_EQUAL(acc.size(), 4);
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		// first enabled item is selected
		BOOST_CHECK_EQUAL(item->index(), 2);
		iter = acc.begin();
		// item zero not visible because selection was implicitly moved by
		// disabling the previously selected item (0)
		int cnt = 1;
		while (iter != acc.end()) {
			item = dynamic_cast<IndexedItem*>(*iter);
			BOOST_CHECK_EQUAL(item->index(), cnt);
			// first 2 items were disabled above
			if (cnt < 2) {
				BOOST_CHECK_EQUAL(item->isDisabled(), true);
			} else {
				BOOST_CHECK_EQUAL(item->isDisabled(), false);
			}
			// obviously should be visible
			BOOST_CHECK_EQUAL(item->isVisible(), true);
			++cnt;
			++iter;
		}
		// 4 items visible, item zero not visible because selection was
		// implicitly moved by disabling the previously selected item (0)
		BOOST_CHECK_EQUAL(cnt, 5);
	}
	// change several items to be invisible
	{
		DM::MenuAccess acc(menu);
		for (int loop = 3; loop < 5; ++loop) {
			BOOST_CHECK_NO_THROW(acc.hide(loop));
		}
	}
	// the above change should make items 3 & 4 invisible
	{
		DM::MenuOutputViewAccess acc(outvAA);
		BOOST_CHECK_EQUAL(acc.size(), 4);
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		BOOST_CHECK_EQUAL(item->index(), 2);
		iter = acc.begin();
		int cnt = 1;
		while (iter != acc.end()) {
			item = dynamic_cast<IndexedItem*>(*iter);
			BOOST_CHECK_EQUAL(item->index(), cnt);
			// first 2 items were disabled
			if (cnt < 2) {
				BOOST_CHECK_EQUAL(item->isDisabled(), true);
			} else {
				BOOST_CHECK_EQUAL(item->isDisabled(), false);
				if (cnt == 2) {
					// next two items are invisible
					cnt += 2;
				}
			}
			// obviously should be visible
			BOOST_CHECK_EQUAL(item->isVisible(), true);
			++cnt;
			++iter;
		}
		// 4 items visible; two greater than last check because of hidden items
		BOOST_CHECK_EQUAL(cnt, 7);
	}
	// advance 1 item
	viewA->backward(1);
	// chose it
	viewA->chose();
	// chose action has not yet occured
	BOOST_CHECK_EQUAL(counts[5], 0);
	// change selection again; should do nothing because of chose()
	viewA->backward(1);
	// should have moved selection from 2 to 5 since 3 & 4 are still invisible
	{
		DM::MenuOutputViewAccess acc(outvAA);
		// chose action occured
		BOOST_CHECK_EQUAL(counts[5], 1);
		BOOST_CHECK_EQUAL(acc.size(), 4);
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		BOOST_CHECK_EQUAL(item->index(), 5);
		iter = acc.begin();
		int cnt = 2;
		while (iter != acc.end()) {
			item = dynamic_cast<IndexedItem*>(*iter);
			BOOST_CHECK_EQUAL(item->index(), cnt);
			// first 2 items were disabled
			if (cnt < 2) {
				BOOST_CHECK_EQUAL(item->isDisabled(), true);
			} else {
				BOOST_CHECK_EQUAL(item->isDisabled(), false);
				if (cnt == 2) {
					// next two items are invisible
					cnt += 2;
				}
			}
			// obviously should be visible
			BOOST_CHECK_EQUAL(item->isVisible(), true);
			++cnt;
			++iter;
		}
		// 4 items visible
		BOOST_CHECK_EQUAL(cnt, 8);
	}
	// change item 4 to be visible
	{
		DM::MenuAccess acc(menu);
		BOOST_CHECK_NO_THROW(acc.show(4));
	}
	// first visible item should now be 4
	{
		DM::MenuOutputViewAccess acc(outvAA);
		BOOST_CHECK_EQUAL(acc.size(), 4);
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		BOOST_CHECK_EQUAL(item->index(), 5);
		iter = acc.begin();
		int cnt = 4;
		while (iter != acc.end()) {
			item = dynamic_cast<IndexedItem*>(*iter);
			BOOST_CHECK_EQUAL(item->index(), cnt);
			// obviously should be visible
			BOOST_CHECK_EQUAL(item->isVisible(), true);
			++cnt;
			++iter;
		}
		// 4 items visible
		BOOST_CHECK_EQUAL(cnt, 8);
	}
}

// test error conditions around insertions and removals
BOOST_AUTO_TEST_CASE(Visibility_Errors) {
	// add a visible item & check error reporting
	{
		DM::MenuAccess acc(menu);
		// append no item
		BOOST_CHECK_THROW(
			acc.append(DM::MenuItemSptr()),
			DM::MenuNoItemError
		);
		// insert no item
		BOOST_CHECK_THROW(
			acc.insert(8, DM::MenuItemSptr()),
			DM::MenuNoItemError
		);
		// make a new item
		IndexedItemSptr item(std::make_shared<IndexedItem>("Appended 16", 16));
		// insert past end
		BOOST_CHECK_THROW(acc.insert(18, item), DM::MenuBoundsError);
		// really append
		BOOST_CHECK_NO_THROW(acc.append(item));
	}
	// the end of the menu is not visible, so no visibility change
	{
		DM::MenuOutputViewAccess acc(outvAA);
		BOOST_CHECK_EQUAL(acc.size(), 4);
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		// first enabled item is selected
		BOOST_CHECK_EQUAL(item->index(), 0);
		iter = acc.begin();
		int cnt = 0;
		while (iter != acc.end()) {
			item = dynamic_cast<IndexedItem*>(*iter);
			BOOST_CHECK_EQUAL(item->index(), cnt);
			++cnt;
			++iter;
		}
		// 4 items visible
		BOOST_CHECK_EQUAL(cnt, 4);
	}
	// select last item
	viewA->jump(16);
	// the end of the menu is visible, including the appended item
	{
		DM::MenuOutputViewAccess acc(outvAA);
		BOOST_CHECK_EQUAL(acc.size(), 4);
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		// appended item is selected
		BOOST_CHECK_EQUAL(item->index(), 16);
		iter = acc.begin();
		int cnt = 13;
		while (iter != acc.end()) {
			item = dynamic_cast<IndexedItem*>(*iter);
			BOOST_CHECK_EQUAL(item->index(), cnt);
			++cnt;
			++iter;
		}
		BOOST_CHECK_EQUAL(cnt, 17);
	}
	// remove an item that was visible in the last check
	{
		DM::MenuAccess acc(menu);
		// remove no item
		BOOST_CHECK_THROW(
			acc.remove(DM::MenuItemSptr()),
			DM::MenuNoItemError
		);
		// make a new item
		IndexedItemSptr item(std::make_shared<IndexedItem>("Bogus", -1));
		// remove an item not in the menu
		BOOST_CHECK_THROW(
			acc.remove(item),
			DM::MenuItemDoesNotExist
		);
		// remove item from beyond the end
		BOOST_CHECK_THROW(
			acc.remove(18),
			DM::MenuBoundsError
		);
		// remove item 13; first one previously visible
		BOOST_CHECK_NO_THROW(acc.remove(13));
	}
	// item 12 is now visible, item 16 at different position, but still
	// selected
	{
		DM::MenuOutputViewAccess acc(outvAA);
		BOOST_CHECK_EQUAL(acc.size(), 4);
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		// appended item is selected
		BOOST_CHECK_EQUAL(item->index(), 16);
		iter = acc.begin();
		int cnt = 12;
		while (iter != acc.end()) {
			item = dynamic_cast<IndexedItem*>(*iter);
			BOOST_CHECK_EQUAL(item->index(), cnt);
			++cnt;
			// advance an extra time because item 13 is gone
			if (cnt == 13) {
				++cnt;
			}
			++iter;
		}
		BOOST_CHECK_EQUAL(cnt, 17);
	}
}

BOOST_AUTO_TEST_SUITE_END()


struct TestAction {
	int line;  // use to identify the action
	int op; // operation to perform
	std::size_t opidx;   // operation occurs on indicated menu item
	std::size_t pselidx; // select prior to operation
	std::size_t aselidx; // selected after operation
	enum {
		Insert,
		Remove
	};
	std::vector<std::size_t> visAA; // visble on output view AA
	// always check output view B for visibility starting at first item
};

// needed for output when a test fails
std::ostream &operator << (std::ostream &os, const TestAction &ti) {
	os << "TestAction:" << ti.line << '(' << ti.opidx << ',';
	if (ti.op == TestAction::Insert) {
		os << "ins";
	} else {
		os << "rem";
	}
	os << ')';
	return os;
}

const TestAction items[] = {
	// line ID |   action       | idx | sel idx |  visible items out-view AA
	{ __LINE__, TestAction::Insert,  0,  0,  0, { 16,  0,  1,  2 } },
	{ __LINE__, TestAction::Remove,  0,  0,  1, {  1,  2,  3,  4 } },
	{ __LINE__, TestAction::Insert,  1,  0,  0, {  0, 16,  1,  2 } },
	{ __LINE__, TestAction::Remove,  1,  0,  0, {  0,  2,  3,  4 } },
	{ __LINE__, TestAction::Insert,  1,  1,  1, { 16,  1,  2,  3 } },
	{ __LINE__, TestAction::Remove,  1,  1,  2, {  0,  2,  3,  4 } },
	{ __LINE__, TestAction::Insert,  6,  6,  6, { 16,  6,  7,  8 } },
	{ __LINE__, TestAction::Remove,  6,  6,  7, {  5,  7,  8,  9 } },
	{ __LINE__, TestAction::Insert,  7,  6,  6, {  5,  6, 16,  7 } },
	{ __LINE__, TestAction::Remove,  7,  6,  6, {  5,  6,  8,  9 } },
	{ __LINE__, TestAction::Insert,  8,  0,  0, {  0,  1,  2,  3 } },
	{ __LINE__, TestAction::Remove,  8,  0,  0, {  0,  1,  2,  3 } },
	{ __LINE__, TestAction::Insert, 15,  0,  0, {  0,  1,  2,  3 } },
	{ __LINE__, TestAction::Remove, 15,  0,  0, {  0,  1,  2,  3 } },
	{ __LINE__, TestAction::Insert, 15, 14, 14, { 13, 14, 16, 15 } },
	{ __LINE__, TestAction::Remove, 15, 14, 14, { 11, 12, 13, 14 } },
	{ __LINE__, TestAction::Insert, 15, 15, 15, { 13, 14, 16, 15 } },
	{ __LINE__, TestAction::Remove, 15, 15, 14, { 11, 12, 13, 14 } },
	{ __LINE__, TestAction::Insert, 16,  0,  0, {  0,  1,  2,  3 } },
	{ __LINE__, TestAction::Insert, 16, 15, 15, { 13, 14, 15, 16 } }
};

// BOOST_DATA_TEST_CASE_F reverses the ordering of the fixture type and test
// name from BOOST_FIXTURE_TEST_SUITE   Boo!
BOOST_DATA_TEST_CASE_F(
	MenuFixture,
	InsertRemove,
	boost::unit_test::data::make(items)
) {
	// after init; before more changes
	{
		DM::MenuOutputViewAccess accAA(outvAA);
		BOOST_CHECK_EQUAL(accAA.changed(), true);
		// Normally having a second MenuOutputViewAccess in scope on the stack
		// doesn't make sense. It is done here to check that a deadlock does not
		// occur.
		DM::MenuOutputViewAccess accAB(outvAB);
		BOOST_CHECK_EQUAL(accAB.changed(), true);
	}
	{
		// this cannot be part of the scope above; it will cause a deadlock
		DM::MenuOutputViewAccess accB(outvB);
		BOOST_CHECK_EQUAL(accB.changed(), true);
	}
	// select an item on view A
	viewA->jump(sample.pselidx);
	{
		DM::MenuOutputViewAccess accAA(outvAA);
		// will change if the selection is different than the default of zero
		BOOST_CHECK_EQUAL(accAA.changed(), sample.pselidx != 0);
		DM::MenuOutputViewAccess accAB(outvAB);
		// same as above
		BOOST_CHECK_EQUAL(accAB.changed(), sample.pselidx != 0);
	}
	// view B hasn't changed
	{
		DM::MenuOutputViewAccess acc(outvB);
		BOOST_CHECK_EQUAL(acc.changed(), false);
	}
	// modify the menu
	{
		DM::MenuAccess acc(menu);
		// insert?
		if (sample.op == TestAction::Insert) {
			// make a new item
			IndexedItemSptr item(std::make_shared<IndexedItem>("Inserted", 16));
			// insert it
			BOOST_CHECK_NO_THROW(acc.insert(sample.opidx, item));
		// remove?
		} else {  // if (sample.op == TestAction::Remove)
			BOOST_CHECK_NO_THROW(acc.remove(sample.opidx));
		}
	}
	// checks on output view AA
	{
		DM::MenuOutputViewAccess acc(outvAA);
		BOOST_CHECK_EQUAL(acc.changed(), true);
		BOOST_CHECK_EQUAL(acc.size(), 4);
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		BOOST_CHECK_EQUAL(item->index(), sample.aselidx);
		iter = acc.begin();
		int cnt = 0;
		while (iter != acc.end()) {
			item = dynamic_cast<IndexedItem*>(*iter);
			BOOST_CHECK_EQUAL(item->index(), sample.visAA[cnt]);
			++cnt;
			++iter;
		}
		BOOST_CHECK_EQUAL(cnt, 4);
	}
	// checks on output view B
	{
		DM::MenuOutputViewAccess acc(outvB);
		BOOST_CHECK_EQUAL(acc.changed(), true);
		BOOST_CHECK_EQUAL(acc.size(), 8);
		DM::MenuVisibleList::const_iterator iter = acc.selectedIter();
		IndexedItem *item = dynamic_cast<IndexedItem*>(*iter);
		// first item removed?
		if ((sample.op == TestAction::Remove) && (sample.opidx == 0)) {
			BOOST_CHECK_EQUAL(item->index(), 1);
		} else {
			BOOST_CHECK_EQUAL(item->index(), 0);
		}
		iter = acc.begin();
		int cnt = 0;
		while (iter != acc.end()) {
			item = dynamic_cast<IndexedItem*>(*iter);
			//std::cout << "VB  cnt = " << cnt << ", index = " << item->index() << std::endl;
			// at or past changed item?
			if (sample.opidx <= cnt) {
				// insertion?
				if (sample.op == TestAction::Insert) {
					// at change?
					if (sample.opidx == cnt) {
						BOOST_CHECK_EQUAL(item->index(), 16);
					// past change?
					} else {
						BOOST_CHECK_EQUAL(item->index(), cnt - 1);
					}
				// remove?
				} else {  // if (sample.op == TestAction::Remove)
					BOOST_CHECK_EQUAL(item->index(), cnt + 1);
				}
			// before change
			} else {
				BOOST_CHECK_EQUAL(item->index(), cnt);
			}
			++cnt;
			++iter;
		}
		BOOST_CHECK_EQUAL(cnt, 8);
	}
	{
		DM::MenuOutputViewAccess acc(outvAA);
		BOOST_CHECK_EQUAL(acc.changed(), false);
	}
}
