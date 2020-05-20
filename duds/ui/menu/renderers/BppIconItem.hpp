/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <duds/ui/menu/GenericMenuItem.hpp>
#include <duds/ui/graphics/BppImage.hpp>

namespace duds { namespace ui { namespace menu { namespace renderers {

/**
 * Generalized item thingy that holds a bit-per-pixel image as an icon.
 * Intended to be used as one of at least two base classes: one class that
 * defines some kind of item, and this one to add an icon.
 * @author  Jeff Jackowski
 */
class BppIconItem {
	/**
	 * The contained icon image.
	 */
	duds::ui::graphics::ConstBppImageSptr img;
public:
	/**
	 * Constructs the item without an icon.
	 */
	BppIconItem() = default;
	/**
	 * Constructs the item with the given icon.
	 * @param icon  The image for the item's icon.
	 */
	BppIconItem(const duds::ui::graphics::ConstBppImageSptr &icon) : img(icon) { }
	/**
	 * Constructs the item with the given icon.
	 * @param icon  The image for the item's icon. The shared pointer is
	 *              moved into this object.
	 */
	BppIconItem(duds::ui::graphics::ConstBppImageSptr &&icon) :
	img(std::move(icon)) { }
	/**
	 * Returns the item's icon.
	 */
	const duds::ui::graphics::ConstBppImageSptr &icon() const {
		return img;
	}
	/**
	 * Sets the item's icon to the given image.
	 * @param newimg  The new image for the item's icon.
	 */
	void icon(const duds::ui::graphics::ConstBppImageSptr &newimg) {
		img = newimg;
	}
	/**
	 * Sets the item's icon to the given image.
	 * @param newimg  The new image for the item's icon. The shared pointer is
	 *                moved into this object.
	 */
	void icon(duds::ui::graphics::ConstBppImageSptr &&newimg) {
		img = std::move(newimg);
	}
};

/**
 * A shared pointer to a BppIconItem object.
 */
typedef std::shared_ptr<BppIconItem>  BppIconItemSptr;

/**
 * A MenuIten that holds a bit-per-pixel icon to represent the item.
 * When using a dynamic_cast from a MenuItem in an attempt to obtain an icon,
 * cast to a BppIconItem.
 * @author  Jeff Jackowski
 */
class BppMenuIconItem : public MenuItem, public BppIconItem {
public:
	/**
	 * Constructs a new BppMenuIconItem.
	 * @note  All BppMenuIconItem objects must be managed by std::shared_ptr.
	 * @param label        Short text presented to the user as the menu item.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	BppMenuIconItem(
		const std::string &label,
		Flags flags = Flags::Zero()
	) : MenuItem(label, flags) { }
	/**
	 * Constructs a new BppMenuIconItem.
	 * @note  All BppMenuIconItem objects must be managed by std::shared_ptr.
	 * @param icon         The image for the item's icon.
	 * @param label        Short text presented to the user as the menu item.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	BppMenuIconItem(
		const duds::ui::graphics::BppImageSptr &icon,
		const std::string &label,
		Flags flags = Flags::Zero()
	) : MenuItem(label, flags), BppIconItem(icon) { }
	/**
	 * Constructs a new BppMenuIconItem.
	 * @note  All BppMenuIconItem objects must be managed by std::shared_ptr.
	 * @param icon         The image for the item's icon. The shared pointer is
	 *                     moved into this object.
	 * @param label        Short text presented to the user as the menu item.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	BppMenuIconItem(
		duds::ui::graphics::BppImageSptr &&icon,
		const std::string &label,
		Flags flags = Flags::Zero()
	) : MenuItem(label, flags), BppIconItem(std::move(icon)) { }
	/**
	 * Constructs a new BppMenuIconItem.
	 * @note  All BppMenuIconItem objects must be managed by std::shared_ptr.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	BppMenuIconItem(
		const std::string &label,
		const std::string &description,
		Flags flags = Flags::Zero()
	) : MenuItem(label, description, flags) { }
	/**
	 * Constructs a new BppMenuIconItem.
	 * @note  All BppMenuIconItem objects must be managed by std::shared_ptr.
	 * @param icon         The image for the item's icon.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	BppMenuIconItem(
		const duds::ui::graphics::BppImageSptr &icon,
		const std::string &label,
		const std::string &description,
		Flags flags = Flags::Zero()
	) : MenuItem(label, description, flags), BppIconItem(icon) { }
	/**
	 * Constructs a new BppMenuIconItem.
	 * @note  All BppMenuIconItem objects must be managed by std::shared_ptr.
	 * @param icon         The image for the item's icon. The shared pointer is
	 *                     moved into this object.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	BppMenuIconItem(
		duds::ui::graphics::BppImageSptr &&icon,
		const std::string &label,
		const std::string &description,
		Flags flags = Flags::Zero()
	) : MenuItem(label, description, flags), BppIconItem(std::move(icon)) { }
	/**
	 * Constructs a new BppMenuIconItem with an associated value.
	 * @note  All BppMenuIconItem objects must be managed by std::shared_ptr.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param value        The current value associated with the menu item.
	 * @param flags        The option flags for the item. While the default is
	 *                     zero, the MenuItem::HasValue flag will be OR'd with
	 *                     the given value by the constructor.
	 */
	BppMenuIconItem(
		const std::string &label,
		const std::string &description,
		const std::string &value,
		Flags flags = Flags::Zero()
	) : MenuItem(label, description, value, flags) { }
	/**
	 * Constructs a new BppMenuIconItem with an associated value.
	 * @note  All BppMenuIconItem objects must be managed by std::shared_ptr.
	 * @param icon         The image for the item's icon.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param value        The current value associated with the menu item.
	 * @param flags        The option flags for the item. While the default is
	 *                     zero, the MenuItem::HasValue flag will be OR'd with
	 *                     the given value by the constructor.
	 */
	BppMenuIconItem(
		const duds::ui::graphics::BppImageSptr &icon,
		const std::string &label,
		const std::string &description,
		const std::string &value,
		Flags flags = Flags::Zero()
	) : MenuItem(label, description, value, flags), BppIconItem(icon) { }
	/**
	 * Constructs a new BppMenuIconItem with an associated value.
	 * @note  All BppMenuIconItem objects must be managed by std::shared_ptr.
	 * @param icon         The image for the item's icon. The shared pointer is
	 *                     moved into this object.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param value        The current value associated with the menu item.
	 * @param flags        The option flags for the item. While the default is
	 *                     zero, the MenuItem::HasValue flag will be OR'd with
	 *                     the given value by the constructor.
	 */
	BppMenuIconItem(
		duds::ui::graphics::BppImageSptr &&icon,
		const std::string &label,
		const std::string &description,
		const std::string &value,
		Flags flags = Flags::Zero()
	) : MenuItem(label, description, value, flags), BppIconItem(std::move(icon)) { }
};

/**
 * A shared pointer to a BppMenuIconItem object.
 */
typedef std::shared_ptr<BppMenuIconItem>  BppMenuIconItemSptr;

/**
 * A GenericMenuItem that holds a bit-per-pixel icon to represent the item.
 * When using a dynamic_cast from a MenuItem in an attempt to obtain an icon,
 * cast to a BppIconItem.
 * @author  Jeff Jackowski
 */
class GenericBppMenuIconItem : public GenericMenuItem, public BppIconItem {
public:
	/**
	 * Constructs a new GenericBppMenuIconItem.
	 * @note  All GenericBppMenuIconItem objects must be managed by
	 *        std::shared_ptr.
	 * @param label        Short text presented to the user as the menu item.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	GenericBppMenuIconItem(
		const std::string &label,
		Flags flags = Flags::Zero()
	) : GenericMenuItem(label, flags) { }
	/**
	 * Constructs a new GenericBppMenuIconItem.
	 * @note  All GenericBppMenuIconItem objects must be managed by
	 *        std::shared_ptr.
	 * @param icon         The image for the item's icon.
	 * @param label        Short text presented to the user as the menu item.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	GenericBppMenuIconItem(
		const duds::ui::graphics::ConstBppImageSptr &icon,
		const std::string &label,
		Flags flags = Flags::Zero()
	) : GenericMenuItem(label, flags), BppIconItem(icon) { }
	/**
	 * Constructs a new GenericBppMenuIconItem.
	 * @note  All GenericBppMenuIconItem objects must be managed by
	 *        std::shared_ptr.
	 * @param icon         The image for the item's icon. The shared pointer is
	 *                     moved into this object.
	 * @param label        Short text presented to the user as the menu item.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	GenericBppMenuIconItem(
		duds::ui::graphics::ConstBppImageSptr &&icon,
		const std::string &label,
		Flags flags = Flags::Zero()
	) : GenericMenuItem(label, flags), BppIconItem(std::move(icon)) { }
	/**
	 * Constructs a new GenericBppMenuIconItem.
	 * @note  All GenericBppMenuIconItem objects must be managed by
	 *        std::shared_ptr.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	GenericBppMenuIconItem(
		const std::string &label,
		const std::string &description,
		Flags flags = Flags::Zero()
	) : GenericMenuItem(label, description, flags) { }
	/**
	 * Constructs a new GenericBppMenuIconItem.
	 * @note  All GenericBppMenuIconItem objects must be managed by
	 *        std::shared_ptr.
	 * @param icon         The image for the item's icon.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	GenericBppMenuIconItem(
		const duds::ui::graphics::ConstBppImageSptr &icon,
		const std::string &label,
		const std::string &description,
		Flags flags = Flags::Zero()
	) : GenericMenuItem(label, description, flags), BppIconItem(icon) { }
	/**
	 * Constructs a new GenericBppMenuIconItem.
	 * @note  All GenericBppMenuIconItem objects must be managed by
	 *        std::shared_ptr.
	 * @param icon         The image for the item's icon. The shared pointer is
	 *                     moved into this object.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	GenericBppMenuIconItem(
		duds::ui::graphics::ConstBppImageSptr &&icon,
		const std::string &label,
		const std::string &description,
		Flags flags = Flags::Zero()
	) : GenericMenuItem(label, description, flags), BppIconItem(std::move(icon)) { }
	/**
	 * Constructs a new GenericBppMenuIconItem with an associated value.
	 * @note  All GenericBppMenuIconItem objects must be managed by
	 *        std::shared_ptr.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param value        The current value associated with the menu item.
	 * @param flags        The option flags for the item. While the default is
	 *                     zero, the MenuItem::HasValue flag will be OR'd with
	 *                     the given value by the constructor.
	 */
	GenericBppMenuIconItem(
		const std::string &label,
		const std::string &description,
		const std::string &value,
		Flags flags = Flags::Zero()
	) : GenericMenuItem(label, description, value, flags) { }
	/**
	 * Constructs a new GenericBppMenuIconItem with an associated value.
	 * @note  All GenericBppMenuIconItem objects must be managed by
	 *        std::shared_ptr.
	 * @param icon         The image for the item's icon.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param value        The current value associated with the menu item.
	 * @param flags        The option flags for the item. While the default is
	 *                     zero, the MenuItem::HasValue flag will be OR'd with
	 *                     the given value by the constructor.
	 */
	GenericBppMenuIconItem(
		const duds::ui::graphics::ConstBppImageSptr &icon,
		const std::string &label,
		const std::string &description,
		const std::string &value,
		Flags flags = Flags::Zero()
	) : GenericMenuItem(label, description, value, flags), BppIconItem(icon) { }
	/**
	 * Constructs a new GenericBppMenuIconItem with an associated value.
	 * @note  All GenericBppMenuIconItem objects must be managed by
	 *        std::shared_ptr.
	 * @param icon         The image for the item's icon. The shared pointer is
	 *                     moved into this object.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param value        The current value associated with the menu item.
	 * @param flags        The option flags for the item. While the default is
	 *                     zero, the MenuItem::HasValue flag will be OR'd with
	 *                     the given value by the constructor.
	 */
	GenericBppMenuIconItem(
		duds::ui::graphics::ConstBppImageSptr &&icon,
		const std::string &label,
		const std::string &description,
		const std::string &value,
		Flags flags = Flags::Zero()
	) : GenericMenuItem(label, description, value, flags),
	BppIconItem(std::move(icon)) { }
	/**
	 * Convenience function to make a shared pointer holding a new
	 * GenericBppMenuIconItem.
	 * @param label        Short text presented to the user as the menu item.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	static std::shared_ptr<GenericBppMenuIconItem> make(
		const std::string &label,
		Flags flags = Flags::Zero()
	) {
		return std::make_shared<GenericBppMenuIconItem>(label, flags);
	}
	/**
	 * Convenience function to make a shared pointer holding a new
	 * GenericBppMenuIconItem.
	 * @param icon         The image for the item's icon.
	 * @param label        Short text presented to the user as the menu item.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	static std::shared_ptr<GenericBppMenuIconItem> make(
		const duds::ui::graphics::ConstBppImageSptr &icon,
		const std::string &label,
		Flags flags = Flags::Zero()
	) {
		return std::make_shared<GenericBppMenuIconItem>(icon, label, flags);
	}
	/**
	 * Convenience function to make a shared pointer holding a new
	 * GenericBppMenuIconItem.
	 * @param icon         The image for the item's icon. The shared pointer is
	 *                     moved into this object.
	 * @param label        Short text presented to the user as the menu item.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	static std::shared_ptr<GenericBppMenuIconItem> make(
		duds::ui::graphics::ConstBppImageSptr &&icon,
		const std::string &label,
		Flags flags = Flags::Zero()
	) {
		return std::make_shared<GenericBppMenuIconItem>(
			std::move(icon),
			label,
			flags
		);
	}
	/**
	 * Convenience function to make a shared pointer holding a new
	 * GenericBppMenuIconItem.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	static std::shared_ptr<GenericBppMenuIconItem> make(
		const std::string &label,
		const std::string &description,
		Flags flags = Flags::Zero()
	) {
		return std::make_shared<GenericBppMenuIconItem>(label, description, flags);
	}
	/**
	 * Convenience function to make a shared pointer holding a new
	 * GenericBppMenuIconItem.
	 * @param icon         The image for the item's icon.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	static std::shared_ptr<GenericBppMenuIconItem> make(
		const duds::ui::graphics::ConstBppImageSptr &icon,
		const std::string &label,
		const std::string &description,
		Flags flags = Flags::Zero()
	) {
		return std::make_shared<GenericBppMenuIconItem>(
			icon,
			label,
			description,
			flags
		);
	}
	/**
	 * Convenience function to make a shared pointer holding a new
	 * GenericBppMenuIconItem.
	 * @param icon         The image for the item's icon. The shared pointer is
	 *                     moved into this object.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	static std::shared_ptr<GenericBppMenuIconItem> make(
		duds::ui::graphics::ConstBppImageSptr &&icon,
		const std::string &label,
		const std::string &description,
		Flags flags = Flags::Zero()
	) {
		return std::make_shared<GenericBppMenuIconItem>(
			std::move(icon),
			label,
			description,
			flags
		);
	}
	/**
	 * Convenience function to make a shared pointer holding a new
	 * GenericBppMenuIconItem.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param value        The current value associated with the menu item.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	static std::shared_ptr<GenericBppMenuIconItem> make(
		const std::string &label,
		const std::string &description,
		const std::string &value,
		Flags flags = Flags::Zero()
	) {
		return std::make_shared<GenericBppMenuIconItem>(
			label,
			description,
			value,
			flags
		);
	}
	/**
	 * Convenience function to make a shared pointer holding a new
	 * GenericBppMenuIconItem.
	 * @param icon         The image for the item's icon.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param value        The current value associated with the menu item.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	static std::shared_ptr<GenericBppMenuIconItem> make(
		const duds::ui::graphics::ConstBppImageSptr &icon,
		const std::string &label,
		const std::string &description,
		const std::string &value,
		Flags flags = Flags::Zero()
	) {
		return std::make_shared<GenericBppMenuIconItem>(
			icon,
			label,
			description,
			value,
			flags
		);
	}
	/**
	 * Convenience function to make a shared pointer holding a new
	 * GenericBppMenuIconItem.
	 * @param icon         The image for the item's icon. The shared pointer is
	 *                     moved into this object.
	 * @param label        Short text presented to the user as the menu item.
	 * @param description  Longer text optionally presented to the user to
	 *                     provide a better idea of what the option does.
	 * @param value        The current value associated with the menu item.
	 * @param flags        The option flags for the item. The default is zero:
	 *                     enabled, visible, no value, and not a toggle.
	 */
	static std::shared_ptr<GenericBppMenuIconItem> make(
		duds::ui::graphics::ConstBppImageSptr &&icon,
		const std::string &label,
		const std::string &description,
		const std::string &value,
		Flags flags = Flags::Zero()
	) {
		return std::make_shared<GenericBppMenuIconItem>(
			std::move(icon),
			label,
			description,
			value,
			flags
		);
	}
};

/**
 * A shared pointer to a GenericBppMenuIconItem object.
 */
typedef std::shared_ptr<GenericBppMenuIconItem>  GenericBppMenuIconItemSptr;

} } } }
