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

#include <api/FavoriteHubApi.h>
#include <api/FavoriteHubUtils.h>
#include <web-server/JsonUtil.h>

#include <airdcpp/AirUtil.h>
#include <airdcpp/ClientManager.h>
#include <airdcpp/FavoriteManager.h>
#include <airdcpp/ShareManager.h>

namespace webserver {
	const PropertyList FavoriteHubApi::properties = {
		{ PROP_NAME, "name", TYPE_TEXT, SERIALIZE_TEXT, SORT_TEXT },
		{ PROP_HUB_URL, "hub_url", TYPE_TEXT, SERIALIZE_TEXT, SORT_TEXT },
		{ PROP_HUB_DESCRIPTION, "hub_description", TYPE_TEXT, SERIALIZE_TEXT, SORT_TEXT },
		{ PROP_AUTO_CONNECT, "auto_connect", TYPE_NUMERIC_OTHER, SERIALIZE_BOOL, SORT_NUMERIC },
		{ PROP_SHARE_PROFILE, "share_profile", TYPE_TEXT, SERIALIZE_CUSTOM, SORT_TEXT },
		{ PROP_CONNECT_STATE, "connect_state", TYPE_NUMERIC_OTHER, SERIALIZE_CUSTOM, SORT_NUMERIC },
		{ PROP_NICK, "nick", TYPE_TEXT, SERIALIZE_TEXT, SORT_TEXT },
		{ PROP_HAS_PASSWORD, "has_password", TYPE_NUMERIC_OTHER, SERIALIZE_BOOL, SORT_NUMERIC },
		{ PROP_USER_DESCRIPTION, "user_description", TYPE_TEXT, SERIALIZE_TEXT, SORT_TEXT },
		{ PROP_IGNORE_PM, "ignore_private_messages", TYPE_NUMERIC_OTHER, SERIALIZE_BOOL, SORT_NUMERIC },
	};

	const PropertyItemHandler<FavoriteHubEntryPtr> FavoriteHubApi::itemHandler = {
		properties,
		FavoriteHubUtils::getStringInfo, FavoriteHubUtils::getNumericInfo, FavoriteHubUtils::compareEntries, FavoriteHubUtils::serializeHub
	};

	FavoriteHubApi::FavoriteHubApi(Session* aSession) : ApiModule(aSession, Access::FAVORITE_HUBS_VIEW),
		view("favorite_hub_view", this, itemHandler, FavoriteHubUtils::getEntryList) {

		FavoriteManager::getInstance()->addListener(this);

		METHOD_HANDLER("hubs", Access::FAVORITE_HUBS_VIEW, ApiRequest::METHOD_GET, (NUM_PARAM, NUM_PARAM), false, FavoriteHubApi::handleGetHubs);
		METHOD_HANDLER("hub", Access::FAVORITE_HUBS_EDIT, ApiRequest::METHOD_POST, (), true, FavoriteHubApi::handleAddHub);
		METHOD_HANDLER("hub", Access::FAVORITE_HUBS_EDIT, ApiRequest::METHOD_DELETE, (TOKEN_PARAM), false, FavoriteHubApi::handleRemoveHub);
		METHOD_HANDLER("hub", Access::FAVORITE_HUBS_EDIT, ApiRequest::METHOD_PATCH, (TOKEN_PARAM), true, FavoriteHubApi::handleUpdateHub);
		METHOD_HANDLER("hub", Access::FAVORITE_HUBS_VIEW, ApiRequest::METHOD_GET, (TOKEN_PARAM), false, FavoriteHubApi::handleGetHub);
	}

	FavoriteHubApi::~FavoriteHubApi() {
		FavoriteManager::getInstance()->removeListener(this);
	}

	api_return FavoriteHubApi::handleGetHubs(ApiRequest& aRequest) {
		auto j = Serializer::serializeItemList(aRequest.getRangeParam(0), aRequest.getRangeParam(1), itemHandler, FavoriteHubUtils::getEntryList());
		aRequest.setResponseBody(j);

		return websocketpp::http::status_code::ok;
	}

	void FavoriteHubApi::updateProperties(FavoriteHubEntryPtr& aEntry, const json& j, bool aNewHub) {
		auto name = JsonUtil::getOptionalField<string>("name", j, false, aNewHub);

		auto server = JsonUtil::getOptionalField<string>("hub_url", j, false, aNewHub);
		if (server) {
			if (!FavoriteManager::getInstance()->isUnique(*server, aEntry->getToken())) {
				JsonUtil::throwError("hub_url", JsonUtil::ERROR_EXISTS, STRING(FAVORITE_HUB_ALREADY_EXISTS));
			}
		}

		auto shareProfileToken = FavoriteHubUtils::deserializeIntHubSetting("share_profile", j);
		if (shareProfileToken && *shareProfileToken != HUB_SETTING_DEFAULT_INT) {
			if (!AirUtil::isAdcHub(!server ? aEntry->getServer() : *server) && *shareProfileToken != SETTING(DEFAULT_SP)) {
				JsonUtil::throwError("share_profile", JsonUtil::ERROR_INVALID, "Share profiles can't be changed for NMDC hubs");
			}

			if (*shareProfileToken) {
				auto shareProfilePtr = ShareManager::getInstance()->getShareProfile(*shareProfileToken, false);
				if (!shareProfilePtr) {
					JsonUtil::throwError("share_profile", JsonUtil::ERROR_INVALID, "Invalid share profile");
				}
			}
		}

		// We have valid values
		if (name) {
			aEntry->setName(*name);
		}

		if (server) {
			aEntry->setServer(*server);
		}

		if (shareProfileToken) {
			aEntry->get(HubSettings::ShareProfile) = *shareProfileToken;
		}

		// Values that don't need to be validated
		for (const auto& i : json::iterator_wrapper(j)) {
			auto key = i.key();
			if (key == "auto_connect") {
				aEntry->setAutoConnect(JsonUtil::parseValue<bool>("auto_connect", i.value()));
			} else if (key == "hub_description") {
				aEntry->setDescription(JsonUtil::parseValue<string>("hub_description", i.value()));
			} else if (key == "password") {
				aEntry->setPassword(JsonUtil::parseValue<string>("password", i.value()));
			} else if (key == "nick") {
				aEntry->get(HubSettings::Nick) = JsonUtil::parseValue<string>("nick", i.value());
			} else if (key == "user_description") {
				aEntry->get(HubSettings::Description) = JsonUtil::parseValue<string>("user_description", i.value());
			} else if (key == "ignore_private_messages") {
				aEntry->setFavNoPM(JsonUtil::parseValue<bool>("ignore_private_messages", i.value()));
			} else if (key == "nmdc_encoding") {
				aEntry->get(HubSettings::NmdcEncoding) = JsonUtil::parseValue<string>("nmdc_encoding", i.value());
			}
		}
	}

	api_return FavoriteHubApi::handleAddHub(ApiRequest& aRequest) {
		const auto& reqJson = aRequest.getRequestBody();

		FavoriteHubEntryPtr e = new FavoriteHubEntry();
		updateProperties(e, reqJson, true);

		FavoriteManager::getInstance()->addFavoriteHub(e);

		aRequest.setResponseBody({
			{ "id", e->getToken() }
		});
		return websocketpp::http::status_code::ok;
	}

	api_return FavoriteHubApi::handleRemoveHub(ApiRequest& aRequest) {
		auto token = aRequest.getTokenParam(0);
		if (!FavoriteManager::getInstance()->removeFavoriteHub(token)) {
			aRequest.setResponseErrorStr("Hub not found");
			return websocketpp::http::status_code::not_found;
		}

		return websocketpp::http::status_code::ok;
	}

	api_return FavoriteHubApi::handleGetHub(ApiRequest& aRequest) {
		auto entry = FavoriteManager::getInstance()->getFavoriteHubEntry(aRequest.getTokenParam(0));
		if (!entry) {
			aRequest.setResponseErrorStr("Hub not found");
			return websocketpp::http::status_code::not_found;
		}

		aRequest.setResponseBody(Serializer::serializeItem(entry, itemHandler));
		return websocketpp::http::status_code::ok;
	}

	api_return FavoriteHubApi::handleUpdateHub(ApiRequest& aRequest) {
		auto e = FavoriteManager::getInstance()->getFavoriteHubEntry(aRequest.getTokenParam(0));
		if (!e) {
			aRequest.setResponseErrorStr("Hub not found");
			return websocketpp::http::status_code::not_found;
		}

		updateProperties(e, aRequest.getRequestBody(), false);
		FavoriteManager::getInstance()->onFavoriteHubUpdated(e);

		return websocketpp::http::status_code::ok;
	}

	void FavoriteHubApi::on(FavoriteManagerListener::FavoriteHubAdded, const FavoriteHubEntryPtr& e)  noexcept {
		view.onItemAdded(e);
	}
	void FavoriteHubApi::on(FavoriteManagerListener::FavoriteHubRemoved, const FavoriteHubEntryPtr& e) noexcept {
		view.onItemRemoved(e);
	}
	void FavoriteHubApi::on(FavoriteManagerListener::FavoriteHubUpdated, const FavoriteHubEntryPtr& e) noexcept {
		view.onItemUpdated(e, toPropertyIdSet(properties));
	}
}