/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2019  Jeff Jackowski
 */
#ifndef GENERICMENUITEM_HPP
#define GENERICMENUITEM_HPP

#include <duds/ui/menu/MenuItem.hpp>
#include <boost/signals2/signal.hpp>

namespace duds { namespace ui { namespace menu {

/**
 * A generic take on MenuItem that invokes a Boost signal when the item is
 * chosen, selected, or deselected.
 * @author  Jeff Jackowski
 */
class GenericMenuItem : public MenuItem {
public:
	/**
	 * The signal type invoked when the item is chosen, selected, or deselected.
	 * @param invokingView  The view used to chose or select the menu item.
	 * @param access        An access object for the menu that may be used
	 *                      to modify the menu.
	 * @param self          The GenericMenuItem that invoked the signal.
	 */
	typedef boost::signals2::signal<
		void(
			MenuView &invokingView,
			const MenuAccess &access,
			GenericMenuItem &self
		)
	> Signal;
private:
	/**
	 * The signal that is invoked when the user choses this menu item.
	 */
	Signal choseSig;
	/**
	 * The signal that is invoked when the user selects this menu item.
	 */
	Signal selSig;
	/**
	 * The signal that is invoked when the user deselects this menu item.
	 */
	Signal deselSig;
public:
	/**
	 * Constructs a new GenericMenuItem.
	 * @note  All MenuItem objects must be managed by std::shared_ptr.
	 * @param label        Short text presented to the user as the menu item.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 * @sa make(const std::string &, Flags)
	 */
	GenericMenuItem(
		const std::string &label,
		Flags flags = Flags::Zero()
	) : MenuItem(label, flags) { }
	/**
	 * Constructs a new GenericMenuItem.
	 * @note  All MenuItem objects must be managed by std::shared_ptr.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 * @sa make(const std::string &, const std::string &, Flags)
	 */
	GenericMenuItem(
		const std::string &label,
		const std::string &description,
		Flags flags = Flags::Zero()
	) : MenuItem(label, description, flags) { }
	/**
	 * Constructs a new GenericMenuItem with an associated value.
	 * @note  All MenuItem objects must be managed by std::shared_ptr.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param value        The current value associated with the menu item.
	 * @param flags        The option flags for the item. While the default is
	 *                     zero, the MenuItem::HasValue flag will be OR'd with
	 *                     the given value by the constructor.
	 * @sa make(const std::string &, const std::string &, const std::string &, Flags)
	 */
	GenericMenuItem(
		const std::string &label,
		const std::string &description,
		const std::string &value,
		Flags flags = Flags::Zero()
	) : MenuItem(label, description, value, flags) { }
	/**
	 * Convenience function to make a shared pointer holding a new
	 * GenericMenuItem.
	 * @param label        Short text presented to the user as the menu item.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	static std::shared_ptr<GenericMenuItem> make(
		const std::string &label,
		Flags flags = Flags::Zero()
	) {
		return std::make_shared<GenericMenuItem>(label, flags);
	}
	/**
	 * Convenience function to make a shared pointer holding a new
	 * GenericMenuItem.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	static std::shared_ptr<GenericMenuItem> make(
		const std::string &label,
		const std::string &description,
		Flags flags = Flags::Zero()
	) {
		return std::make_shared<GenericMenuItem>(label, description, flags);
	}
	/**
	 * Convenience function to make a shared pointer holding a new
	 * GenericMenuItem with an associated value.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param value        The current value associated with the menu item.
	 * @param flags        The option flags for the item. While the default is
	 *                     zero, the MenuItem::HasValue flag will be OR'd with
	 *                     the given value by the constructor.
	 */
	static std::shared_ptr<GenericMenuItem> make(
		const std::string &label,
		const std::string &description,
		const std::string &value,
		Flags flags = Flags::Zero()
	) {
		return std::make_shared<GenericMenuItem>(label, description, value, flags);
	}
	/**
	 * Invokes the @a choseSig signal in response to the user chosing this
	 * menu item. Use one of the choseConnect() or choseConnectExtended()
	 * functions to make connections to the signal.
	 */
	virtual void chose(MenuView &invokingView, const MenuAccess &access);
	/**
	 * Invokes the @a selSig signal in response to the user selecting this
	 * menu item.
	 */
	virtual void select(MenuView &invokingView, const MenuAccess &access);
	/**
	 * Invokes the @a deselSig signal in response to the user deselecting this
	 * menu item.
	 */
	virtual void deselect(MenuView &invokingView, const MenuAccess &access);
	/**
	 * Make a connection to the generic menu item chose signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection choseConnect(
		const typename Signal::slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return choseSig.connect(slot, at);
	}
	/**
	 * Make a connection to the generic menu item chose signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection choseConnect(
		const typename Signal::group_type &group,
		const typename Signal::slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return choseSig.connect(group, slot, at);
	}
	/**
	 * Make a connection to the generic menu item chose signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection choseConnectExtended(
		const typename Signal::extended_slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return choseSig.connect_extended(slot, at);
	}
	/**
	 * Make a connection to the generic menu item chose signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection choseConnectExtended(
		const typename Signal::group_type &group,
		const typename Signal::extended_slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return choseSig.connect_extended(group, slot, at);
	}
	/**
	 * Disconnect a group from the generic menu item chose signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	void choseDisconnect(
		const typename Signal::group_type &group
	) {
		choseSig.disconnect(group);
	}
	/**
	 * Disconnect from the generic menu item chose signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	template<typename Slot>
	void choseDisconnect(const Slot &slotFunc) {
		choseSig.disconnect(slotFunc);
	}
	/**
	 * Make a connection to the generic menu item select signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection selectConnect(
		const typename Signal::slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return selSig.connect(slot, at);
	}
	/**
	 * Make a connection to the generic menu item select signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection selectConnect(
		const typename Signal::group_type &group,
		const typename Signal::slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return selSig.connect(group, slot, at);
	}
	/**
	 * Make a connection to the generic menu item select signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection selectConnectExtended(
		const typename Signal::extended_slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return selSig.connect_extended(slot, at);
	}
	/**
	 * Make a connection to the generic menu item select signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection selectConnectExtended(
		const typename Signal::group_type &group,
		const typename Signal::extended_slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return selSig.connect_extended(group, slot, at);
	}
	/**
	 * Disconnect a group from the generic menu item select signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	void selectDisconnect(
		const typename Signal::group_type &group
	) {
		selSig.disconnect(group);
	}
	/**
	 * Disconnect from the generic menu item select signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	template<typename Slot>
	void selectDisconnect(const Slot &slotFunc) {
		selSig.disconnect(slotFunc);
	}
	/**
	 * Make a connection to the generic menu item deselect signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection deselectConnect(
		const typename Signal::slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return deselSig.connect(slot, at);
	}
	/**
	 * Make a connection to the generic menu item deselect signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection deselectConnect(
		const typename Signal::group_type &group,
		const typename Signal::slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return deselSig.connect(group, slot, at);
	}
	/**
	 * Make a connection to the generic menu item deselect signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection deselectConnectExtended(
		const typename Signal::extended_slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return deselSig.connect_extended(slot, at);
	}
	/**
	 * Make a connection to the generic menu item deselect signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	boost::signals2::connection deselectConnectExtended(
		const typename Signal::group_type &group,
		const typename Signal::extended_slot_type &slot,
		boost::signals2::connect_position at = boost::signals2::at_back
	) {
		return deselSig.connect_extended(group, slot, at);
	}
	/**
	 * Disconnect a group from the generic menu item deselect signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	void deselectDisconnect(
		const typename Signal::group_type &group
	) {
		deselSig.disconnect(group);
	}
	/**
	 * Disconnect from the generic menu item deselect signal.
	 * See the [Boost reference documentation](https://www.boost.org/doc/libs/1_83_0/doc/html/boost/signals2/signal.html#idp182137616-bb)
	 * for more details, or the [tutorial](https://www.boost.org/doc/libs/1_83_0/doc/html/signals2/tutorial.html)
	 * for an overview of the whole boost::singals2 system.
	 */
	template<typename Slot>
	void deselectDisconnect(const Slot &slotFunc) {
		deselSig.disconnect(slotFunc);
	}
};

/**
 * A shared pointer to a GenericMenuItem.
 */
typedef std::shared_ptr<GenericMenuItem>  GenericMenuItemSptr;

/**
 * A weak pointer to a GenericMenuItem.
 */
typedef std::weak_ptr<GenericMenuItem>  GenericMenuItemWptr;

} } }

#endif        //  #ifndef GENERICMENUITEM_HPP
