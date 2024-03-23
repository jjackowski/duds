/*
 * This file is part of the DUDS project. It is subject to the BSD-style
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at https://github.com/jjackowski/duds/blob/master/LICENSE.
 * No part of DUDS, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 *
 * Copyright (C) 2020  Jeff Jackowski
 */
#include <duds/ui/graphics/PriorityGridLayout.hpp>
#include <duds/ui/graphics/LayoutErrors.hpp>
#include <duds/general/Errors.hpp>

namespace duds { namespace ui { namespace graphics {

bool PriorityGridLayout::RowData::minCols(int mc) noexcept {
	if (panels.size() <= mc) {
		panels.resize(mc + 1, std::pair<unsigned int, PanelStatus*>(0, nullptr));
		return false;
	}
	return true;
}

PriorityGridLayout::RowData::KeyPanel &PriorityGridLayout::RowData::operator [] (int c) {
	minCols(c);
	return panels[c];
}

bool PriorityGridLayout::minRows(RowVec &rv, int ms) {
	if (rv.size() <= ms) {
		rv.resize(ms + 1);
		return false;
	}
	return true;
}

unsigned int PriorityGridLayout::panelAt(RowVec &rv, const GridLocation &gl) noexcept {
	// spot exists?
	if (minRows(rv, gl.r + 1) && rv[gl.r].minCols(gl.c + 1)) {
		return rv[gl.r].panels[gl.c].first;
	}
	return 0;
}


PriorityGridLayout::PanelStatus::PanelStatus(
	const PanelSptr &pspt,
	const GridLayoutConfig &conf
) : panel(pspt), config(conf), hidden(true) { }

PriorityGridLayout::PanelStatus::PanelStatus(
	const PanelSptr &pspt,
	GridLayoutConfig &&conf
) : panel(pspt), config(std::move(conf)), hidden(true) { }

void PriorityGridLayout::maxRowHeight(int row, std::int16_t height) {
	if (rowMaxHeight.size() <= row) {
		rowMaxHeight.resize(row + 1, 0x7FFF);
	}
	rowMaxHeight[row] = height;
}

std::int16_t PriorityGridLayout::maxRowHeight(int row) const {
	if (rowMaxHeight.size() <= row) {
		return 0x7FFF;
	}
	return rowMaxHeight[row];
}

bool PriorityGridLayout::add(
	const PanelSptr &panel,
	const GridLayoutConfig &config,
	unsigned int pri
) {
	if (pri == 0) {
		DUDS_THROW_EXCEPTION(LayoutPriorityInvalidError());
	}
	std::pair<GridConfig::iterator, bool> res =
		configs.try_emplace(pri, PanelStatus(panel, config));
	if (res.second) {
		try {
			panel->added(this, pri);
			return true;
		} catch (...) {
			configs.erase(pri);
			throw;
		}
	}
	return false;
}

bool PriorityGridLayout::add(
	const PanelSptr &panel,
	const GridSizeStep &config,
	unsigned int pri
) {
	if (pri == 0) {
		DUDS_THROW_EXCEPTION(LayoutPriorityInvalidError());
	}
	std::pair<GridConfig::iterator, bool> res =
		configs.try_emplace(pri, PanelStatus(panel, GridLayoutConfig(config)));
	if (res.second) {
		try {
			panel->added(this, pri);
			return true;
		} catch (...) {
			configs.erase(pri);
			throw;
		}
	}
	return false;
}

unsigned int PriorityGridLayout::add(
	const PanelSptr &panel,
	const GridLayoutConfig &config
) {
	unsigned int pri;
	if (configs.empty()) {
		pri = 1;
	} else {
		GridConfig::const_reverse_iterator last = configs.crbegin();
		pri = last->first + 1;
	}
	addOrReplace(panel, config, pri);
	return pri;
}

unsigned int PriorityGridLayout::add(
	const PanelSptr &panel,
	const GridSizeStep &config
) {
	unsigned int pri;
	if (configs.empty()) {
		pri = 1;
	} else {
		GridConfig::const_reverse_iterator last = configs.crbegin();
		pri = last->first + 1;
	}
	addOrReplace(panel, config, pri);
	return pri;
}

void PriorityGridLayout::addOrReplace(
	const PanelSptr &panel,
	const GridLayoutConfig &config,
	unsigned int pri
) {
	if (pri == 0) {
		DUDS_THROW_EXCEPTION(LayoutPriorityInvalidError());
	}
	configs[pri] = PanelStatus(panel, config);
	try {
		panel->added(this, pri);
	} catch (...) {
		configs.erase(pri);
		throw;
	}
}

void PriorityGridLayout::addOrReplace(
	const PanelSptr &panel,
	const GridSizeStep &config,
	unsigned int pri
) {
	if (pri == 0) {
		DUDS_THROW_EXCEPTION(LayoutPriorityInvalidError());
	}
	configs[pri] = PanelStatus(panel, GridLayoutConfig(config));
	try {
		panel->added(this, pri);
	} catch (...) {
		configs.erase(pri);
		throw;
	}
}

void PriorityGridLayout::remove(unsigned int pri) {
	GridConfig::iterator iter = configs.find(pri);
	if (iter != configs.end()) {
		iter->second.panel->removing(this, pri);
		configs.erase(iter);
	}
}

void PriorityGridLayout::remove(const PanelSptr &panel) {
	GridConfig::iterator iter = std::find_if(
		configs.begin(),
		configs.end(),
		[panel] (auto i) {
			return i.second.panel == panel;
		}
	);
	if (iter != configs.end()) {
		iter->second.panel->removing(this, iter->first);
		configs.erase(iter);
	}
}

GridLayoutConfig &PriorityGridLayout::panelConfig(unsigned int pri) {
	GridConfig::iterator iter = configs.find(pri);
	if (iter == configs.end()) {
		DUDS_THROW_EXCEPTION(PanelNotFoundError() << PanelPriority(pri));
	}
	return iter->second.config;
}

const GridLayoutConfig &PriorityGridLayout::panelConfig(unsigned int pri) const {
	GridConfig::const_iterator iter = configs.find(pri);
	if (iter == configs.end()) {
		DUDS_THROW_EXCEPTION(PanelNotFoundError() << PanelPriority(pri));
	}
	return iter->second.config;
}

int PriorityGridLayout::layout() {
	// tabulated data on each row
	RowVec rdat;
	rdat.reserve(8);
	// total dimensions used
	ImageDimensions total(0, 0);
	int placed = 0;
	// rows with height expansion requests
	int heightExpand = 0;
	// place items into grid positions in priority order
	for (auto &pstat : configs) {
		// re-initialize to initial size-step
		pstat.second.sizeStep = 0;
		// hide if flagged as hidden or no size-steps
		pstat.second.hidden = (pstat.second.sizeStep < 0) ||
			pstat.second.config.sizes.empty() ||
			((pstat.second.config.flags & GridLayoutConfig::PanelHidden) > 0);
		// skip hidden panels
		if (pstat.second.hidden) {
			continue;
		}
		RowData *row = nullptr;
		ImageDimensions maxarea;
		GridLocation gl;
		bool done = false;
		// find the first size-step that fits
		for (; pstat.second.sizeStep < pstat.second.config.sizes.size();
			++pstat.second.sizeStep
		) {
			// advanced to hidden spot?
			if (pstat.second.config.sizes[pstat.second.sizeStep].flags &
			GridLayoutConfig::PanelHidden) {
				// continue to next spot
				continue;
			}
			// copy location for quicker access
			gl = pstat.second.currentStep().loc;
			// see if the spot is available
			if (!panelAt(rdat, gl)) {
				// attempt to use this spot
				// row change?
				if (row != &(rdat[gl.r])) {
					// get the row data that covers the spot
					row = &(rdat[gl.r]);
					// compute the largest area currently available
					maxarea.w = fill.w - row->used.w;
					maxarea.h = std::min(
						maxRowHeight(gl.r),
						(std::int16_t)(fill.h - total.h + row->used.h)
					);
				}
				// minimum requested size fits?
				if (maxarea.fits(pstat.second.config.sizes[pstat.second.sizeStep].minDim)) {
					done = true;
					break;
				}
			}
			// else, spot not availble; loop for next size-step
		}
		// found a spot?
		if (done) {
			assert(maxarea.fits(pstat.second.config.sizes[pstat.second.sizeStep].minDim));
			assert(gl == pstat.second.config.sizes[pstat.second.sizeStep].loc);
			// when running here, a suitable spot was found
			// record the usage of this spot
			(*row)[gl.c] = RowData::KeyPanel(pstat.first, &pstat.second);
			// store minimum dimensions as current panel dimensions
			pstat.second.dim =
				pstat.second.config.sizes[pstat.second.sizeStep].minDim;
			// tabulate area used
			row->used.w += pstat.second.dim.w;
			total.h -= row->used.h;
			row->used.h = std::max(row->used.h, pstat.second.dim.h);
			total.h += row->used.h;
			// note a width expansion request
			if (pstat.second.flags() & GridLayoutConfig::PanelWidthExpand) {
				row->widthExpand++;
			}
			// height expansion
			if (!row->heightExpand &&   // only note expansion once
				(pstat.second.flags() & GridLayoutConfig::PanelHeightExpand)
			) {
				row->heightExpand = true;
				heightExpand++;
			}
			++placed;
		}
		// reached end without finding a spot?
		else /* if (!done) */ {
			// mark as hidden
			pstat.second.hidden = true;
		}
	}
	// consider height expansion
	int addnlH, extraH;
	if (heightExpand) {
		addnlH = (fill.h - total.h) / heightExpand;
		extraH = (fill.h - total.h) % heightExpand;
		assert((total.h + addnlH + extraH) == fill.h);
		// if there may be maximum height limitations present . . .
		if (!rowMaxHeight.empty()) {
			// . . . check for rows expanding past the maximum
			for (int idx = 0; idx < rdat.size(); ++idx) {
				RowData &row = rdat[idx];
				if (row.heightExpand) {
					int eh = addnlH + row.used.h; // expanded height
					if (extraH) {
						++eh;
					}
					int mh = maxRowHeight(idx);   // max height
					assert(mh >= 0);  // should not have a height greater than max
					if (mh < eh) {
						// enlarge to maximum
						total.h += mh - row.used.h;
						row.used.h = mh;
						// don't make it any taller
						row.heightExpand = false;
						--heightExpand;
					}
				}
			}
			// the algorithm needs to be iterative to be perfect; just do one
			// iteration and hope it fits well
			if (heightExpand) {
				addnlH = (fill.h - total.h) / heightExpand;
				extraH = (fill.h - total.h) % heightExpand;
				assert((total.h + addnlH + extraH) == fill.h);
			}
		}
	} else {
		addnlH = extraH = 0;
	}
	// compute finalized image dimensions for the panels
	for (int idx = 0; idx < rdat.size(); ++idx) {
		RowData &row = rdat[idx];
		if (row.heightExpand) {
			assert(heightExpand);
			row.used.h += addnlH;
			if (extraH) {
				row.used.h++;
				extraH--;
			}
			int mh = maxRowHeight(idx);   // max height
			// past max?
			if (row.used.h > mh) {
				// allow following rows to use unused space
				if (--heightExpand) {
					addnlH += (row.used.h - mh) / heightExpand;
				}
				row.used.h = mh;
			}
		}
		int additonal = 0, extra = 0;
		if (row.widthExpand) {
			// compute amount of expansion
			additonal = (fill.w - row.used.w) / row.widthExpand;
			extra = (fill.w - row.used.w) % row.widthExpand;
			assert((row.used.w + additonal * row.widthExpand + extra) == fill.w);
			row.used.w = fill.w;  // should be true by the end of the next loop
		}
		int pref = 0; // index of item getting width remainder
		// apply width expansion
		for (int col = 0; col < row.panels.size(); ++col) {
			RowData::KeyPanel &kp = row.panels[col];
			assert((kp.first && kp.second) || (!kp.first && !kp.second));
			// used column?
			if (kp.first) {
				assert(~kp.second->flags() & GridLayoutConfig::PanelHidden);
				assert(!kp.second->hidden);
				// update height
				kp.second->dim.h = row.used.h;
				// width expansion requested?
				if (
					row.widthExpand &&
					(kp.second->flags() & GridLayoutConfig::PanelWidthExpand)
				) {
					kp.second->dim.w += additonal;
					// higher priortity?
					if (extra && (kp.first < row.panels[pref].first)) {
						pref = col;
					}
				}
			}
		}
		if (extra) {
			// apply more to the highest priority panel
			row.panels[pref].second->dim.w += extra;
		}
	}
	// compute finalized image locations for the panels
	total.h = 0;
	for (auto &row : rdat) {
		total.w = 0;
		for (auto &col : row.panels) {
			// used column?
			if (col.first) {
				col.second->loc.x = total.w;
				col.second->loc.y = total.h;
				total.w += col.second->dim.w;
			}
		}
		assert(total.w == row.used.w);
		assert(total.w <= fill.w);
		total.h += row.used.h;
	}
	assert(total.h <= fill.h);
	return placed;
}

void PriorityGridLayout::render(BppImage *dest) {
	{ // check dest size
		ImageLocation chk = (offset + fill) - ImageLocation(1, 1);
		if (!dest->dimensions().withinBounds(chk)) {
			DUDS_THROW_EXCEPTION(ImageBoundsError() <<
				ImageErrorLocation(offset) <<
				ImageErrorSourceDimensions(fill) <<
				ImageErrorTargetDimensions(dest->dimensions())
			);
		}
	}
	// Render each panel. This is done in priority order because of the data
	// structure used; rendering could be done in random order and succeed.
	for (auto pstat : configs) {
		// not hidden?
		if (!pstat.second.hidden) {
			ImageLocation off(0, 0);
			ImageDimensions dim(pstat.second.dim);
			PanelMargins margin = { 0, 0, 0, 0 };
			const BppImage *img;
			try {
				img = pstat.second.panel->render(
					off,
					dim,
					margin,
					pstat.second.sizeStep
				);
			} catch (boost::exception &be) {
				be << PanelPriority(pstat.first) <<
				PanelSizeStep(pstat.second.sizeStep);
				throw;
			}
			// showing something other than blank space?
			if (img) {
				ImageDimensions dimIncMar(
					dim.w + margin.l + margin.r,
					dim.h + margin.t + margin.b
				);
				// fit check
				if (!pstat.second.dim.fits(dimIncMar)) {
					// failed
					DUDS_THROW_EXCEPTION(PanelImageTooLargeError() <<
						PanelPriority(pstat.first) <<
						PanelSizeStep(pstat.second.sizeStep) <<
						ImageErrorLocation(off) <<
						ImageErrorSourceDimensions(dimIncMar) <<
						ImageErrorTargetDimensions(pstat.second.dim)
					);
				}
				// potentially adjust location based on justification or
				// centering options
				GridLayoutConfig::Flags flags = pstat.second.flags();
				ImageLocation loc = pstat.second.loc + offset;
				loc.x += margin.l;
				loc.y += margin.t;
				// width
				if (pstat.second.dim.w != dimIncMar.w) {
					if (flags & GridLayoutConfig::PanelJustifyRight) {
						loc.x += pstat.second.dim.w - dimIncMar.w;
					} else if (flags & GridLayoutConfig::PanelCenterHoriz) {
						loc.x += (pstat.second.dim.w - dimIncMar.w) / 2;
					}
				}
				// height
				if (pstat.second.dim.h != dimIncMar.h) {
					if (flags & GridLayoutConfig::PanelJustifyDown) {
						loc.y += pstat.second.dim.h - dimIncMar.h;
					} else if (flags & GridLayoutConfig::PanelCenterVert) {
						loc.y += (pstat.second.dim.h - dimIncMar.h) / 2;
					}
				}
				// output!
				dest->write(img, loc, off, dim);
			}
		}
	}
}

ImageDimensions PriorityGridLayout::layoutDimensions(unsigned int pri) const {
	GridConfig::const_iterator iter = configs.find(pri);
	if (iter == configs.end()) {
		return ImageDimensions(0, 0);
	}
	return iter->second.dim;
}

ImageLocation PriorityGridLayout::layoutLocation(unsigned int pri) const {
	GridConfig::const_iterator iter = configs.find(pri);
	if (iter == configs.end()) {
		return ImageLocation(0, 0);
	}
	return iter->second.loc;
}

bool PriorityGridLayout::layoutPosition(
	ImageDimensions &dim,
	ImageLocation &loc,
	unsigned int pri
) const {
	GridConfig::const_iterator iter = configs.find(pri);
	if (iter == configs.end()) {
		return false;
	}
	dim = iter->second.dim;
	loc = iter->second.loc;
	return true;
}

} } }
