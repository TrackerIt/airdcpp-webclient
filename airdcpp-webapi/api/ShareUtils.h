/*
* Copyright (C) 2011-2016 AirDC++ Project
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef DCPLUSPLUS_DCPP_SHARE_UTILS_H
#define DCPLUSPLUS_DCPP_SHARE_UTILS_H

#include <web-server/stdinc.h>

#include <airdcpp/typedefs.h>
#include <airdcpp/StringMatch.h>
#include <airdcpp/ShareDirectoryInfo.h>

namespace webserver {
	class ShareUtils {
	public:
		static string formatRefreshState(const ShareDirectoryInfoPtr& aItem) noexcept;

		static json serializeItem(const ShareDirectoryInfoPtr& aItem, int aPropertyName) noexcept;
		static bool filterItem(const ShareDirectoryInfoPtr& aItem, int aPropertyName, const StringMatch& aTextMatcher, double aNumericMatcher) noexcept;

		static int compareItems(const ShareDirectoryInfoPtr& a, const ShareDirectoryInfoPtr& b, int aPropertyName) noexcept;
		static std::string getStringInfo(const ShareDirectoryInfoPtr& a, int aPropertyName) noexcept;
		static double getNumericInfo(const ShareDirectoryInfoPtr& a, int aPropertyName) noexcept;
	};
}

#endif