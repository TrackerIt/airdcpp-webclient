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

#include <web-server/stdinc.h>

#include <api/SessionApi.h>

#include <web-server/JsonUtil.h>
#include <web-server/WebSocket.h>
#include <web-server/WebServerManager.h>
#include <web-server/WebUserManager.h>

#include <airdcpp/ActivityManager.h>
#include <airdcpp/ClientManager.h>
#include <airdcpp/SettingsManager.h>
#include <airdcpp/version.h>

namespace webserver {
	SessionApi::SessionApi() {

	}

	string SessionApi::getHostname() noexcept {
#ifdef _WIN32
		TCHAR computerName[1024];
		DWORD size = 1024;
		GetComputerName(computerName, &size);
		return Text::fromT(computerName);
#else
		char hostname[128];
		gethostname(hostname, sizeof hostname);
		return hostname;
#endif
	}

	json SessionApi::getSystemInfo(const string& aIp) const noexcept {
		json retJson;
		retJson["path_separator"] = PATH_SEPARATOR_STR;

		
		// IPv4 addresses will be mapped to IPv6
		auto ip = aIp;
		auto v6 = aIp.find(":") != string::npos;
		if (aIp.find("[::ffff:") == 0) {
			auto end = aIp.rfind("]");
			ip = aIp.substr(8, end-8);
			v6 = false;
		}

		retJson["network_type"] = Util::isPrivateIp(ip, v6) ? "local" : "internet";
#ifdef _WIN32
		retJson["platform"] = "windows";
#elif APPLE
		retJson["platform"] = "osx";
#else
		retJson["platform"] = "other";
#endif
		retJson["hostname"] = getHostname();
		return retJson;
	}

	websocketpp::http::status_code::value SessionApi::handleLogin(ApiRequest& aRequest, bool aIsSecure, const WebSocketPtr& aSocket, const string& aIp) {
		const auto& reqJson = aRequest.getRequestBody();

		auto username = JsonUtil::getField<string>("username", reqJson, false);
		auto password = JsonUtil::getField<string>("password", reqJson, false);

		auto inactivityMinutes = JsonUtil::getOptionalFieldDefault<uint64_t>("max_inactivity", reqJson, 20ULL);
		auto userSession = JsonUtil::getOptionalFieldDefault<bool>("user_session", reqJson, false);

		auto session = WebServerManager::getInstance()->getUserManager().authenticate(username, password, 
			aIsSecure, inactivityMinutes, userSession);

		if (!session) {
			aRequest.setResponseErrorStr("Invalid username or password");
			return websocketpp::http::status_code::unauthorized;
		}

		json retJson = {
			{ "permissions", session->getUser()->getPermissions() },
			{ "token", session->getAuthToken() },
			{ "user", session->getUser()->getUserName() },
			{ "system", getSystemInfo(aIp) },
			{ "run_wizard", SETTING(WIZARD_RUN) },
			{ "cid", ClientManager::getInstance()->getMyCID().toBase32() },
		};

		if (aSocket) {
			session->onSocketConnected(aSocket);
			aSocket->setSession(session);
		}

		aRequest.setResponseBody(retJson);
		return websocketpp::http::status_code::ok;
	}

	api_return SessionApi::handleActivity(ApiRequest& aRequest) {
		auto s = aRequest.getSession();
		if (!s) {
			aRequest.setResponseErrorStr("Not authorized");
			return websocketpp::http::status_code::unauthorized;
		}

		if (!s->isUserSession()) {
			aRequest.setResponseErrorStr("Activity can only be updated for user sessions");
			return websocketpp::http::status_code::bad_request;
		}

		ActivityManager::getInstance()->updateActivity();
		return websocketpp::http::status_code::ok;
	}

	api_return SessionApi::handleLogout(ApiRequest& aRequest) {
		if (!aRequest.getSession()) {
			aRequest.setResponseErrorStr("Not authorized");
			return websocketpp::http::status_code::unauthorized;
		}

		WebServerManager::getInstance()->logout(aRequest.getSession()->getId());

		return websocketpp::http::status_code::ok;
	}

	api_return SessionApi::handleSocketConnect(ApiRequest& aRequest, bool aIsSecure, const WebSocketPtr& aSocket) {
		auto sessionToken = JsonUtil::getField<string>("authorization", aRequest.getRequestBody(), false);

		auto session = WebServerManager::getInstance()->getUserManager().getSession(sessionToken);
		if (!session) {
			aRequest.setResponseErrorStr("Invalid session token");
			return websocketpp::http::status_code::bad_request;
		}

		if (session->isSecure() != aIsSecure) {
			aRequest.setResponseErrorStr("Invalid protocol");
			return websocketpp::http::status_code::bad_request;
		}

		session->onSocketConnected(aSocket);
		aSocket->setSession(session);

		return websocketpp::http::status_code::ok;
	}
}