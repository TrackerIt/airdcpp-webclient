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

#ifndef DCPLUSPLUS_DCPP_SEARCHRESULTINFO_H
#define DCPLUSPLUS_DCPP_SEARCHRESULTINFO_H

#include <web-server/stdinc.h>

#include <airdcpp/typedefs.h>
#include <airdcpp/GetSet.h>

#include <airdcpp/DupeType.h>
#include <airdcpp/SearchResult.h>
#include <airdcpp/QueueItemBase.h>
#include <airdcpp/TargetUtil.h>

namespace webserver {
	typedef uint32_t ResultToken;

	class SearchResultInfo {
	public:
		typedef shared_ptr<SearchResultInfo> Ptr;
		struct RelevanceSort {
			bool operator()(const Ptr& left, const Ptr& right) const noexcept { return left->getTotalRelevance() > right->getTotalRelevance(); }
		};

		typedef vector<Ptr> List;
		typedef unordered_map<TTHValue, Ptr> Map;
		typedef set<Ptr, RelevanceSort> Set;

		SearchResultInfo(const SearchResultPtr& aSR, SearchResult::RelevanceInfo&& aRelevance);
		~SearchResultInfo() {	}

		const UserPtr& getUser() const noexcept { return sr->getUser().user; }
		const string& getHubUrl() const noexcept { return sr->getUser().hint; }

		bool hasUser(const UserPtr& aUser) const noexcept;
		void addChildResult(const SearchResultInfo::Ptr& aResult) noexcept;
		api_return download(const string& aTargetDirectory, const string& aTargetName, TargetUtil::TargetType aTargetType, QueueItemBase::Priority p);

		bool isDirectory() const noexcept {
			return sr->getType() == SearchResult::TYPE_DIRECTORY;
		}

		SearchResultPtr sr;
		IGETSET(DupeType, dupe, Dupe, DUPE_NONE);

		double getTotalRelevance() const noexcept;
		double getMatchRelevance() const noexcept;

		ResultToken getToken() const noexcept {
			return token;
		}

		int getHits() const noexcept {
			return hits;
		}

		const string& getCountry() const noexcept {
			return country;
		}

		double getConnectionSpeed() const noexcept;
		void getSlots(int& free_, int& total_) const noexcept;
		string getSlotStr() const noexcept;
	private:
		SearchResultInfo* parent = nullptr;
		SearchResultInfo::List children;

		SearchResult::RelevanceInfo relevanceInfo;

		string country;
		ResultToken token;
		int hits = 0;

		static FastCriticalSection cs;
	};

	typedef SearchResultInfo::Ptr SearchResultInfoPtr;
}

#endif