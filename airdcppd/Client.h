/*
 * Copyright (C) 2012-2015 AirDC++ Project
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

#ifndef AIRDCPPD_CLIENT_H
#define AIRDCPPD_CLIENT_H

#include <airdcpp/stdinc.h>
#include <airdcpp/ClientManagerListener.h>
#include <airdcpp/DirectoryListingManagerListener.h>

#include "CDMDebug.h"

namespace airdcppd {

using namespace dcpp;

class Client : private ClientManagerListener, private DirectoryListingManagerListener {

public:
	Client(bool aAsDaemon);
	void run();
	void stop();
private:
	bool startup();
	void shutdown();

	static std::string getDefaultNick() noexcept;

	void on(DirectoryListingManagerListener::OpenListing, const DirectoryListingPtr& aList, const string& aDir, const string& aXML) noexcept;
	void on(ClientManagerListener::ClientCreated, const ClientPtr&) noexcept;

	bool started = false;
	bool asDaemon = false;
	
	unique_ptr<CDMDebug> cdmDebug;
};

} // namespace airdcppd

#endif //
