#pragma once

#include <QString>

namespace Twitch
{
	inline const char *API_HOST="https://api.twitch.tv/helix/";
	inline const char *CONTENT_HOST="https://static-cdn.jtvnw.net/";

	inline const char *ENDPOINT_CHAT_SETTINGS="chat/settings";
	inline const char *ENDPOINT_STREAM_INFORMATION="streams";
	inline const char *ENDPOINT_CHANNEL_INFORMATION="channels";
	inline const char *ENDPOINT_GAME_INFORMATION="games";
	inline const char *ENDPOINT_USER_FOLLOWS="channels/followers";
	inline const char *ENDPOINT_BADGES="chat/badges/global";
	inline const char *ENDPOINT_SHOUTOUTS="chat/shoutouts";
	inline const char *ENDPOINT_USERS="users";
	inline const char *ENDPOINT_EVENTSUB="eventsub/subscriptions";
	inline const char *ENDPOINT_EVENTSUB_SUBSCRIPTIONS="eventsub/subscriptions";

	inline QString Endpoint(const QString &path)
	{
		return QStringBuilder<QString,QString>(API_HOST,path);
	}
	
	inline const char *ENDPOINT_EMOTES="emoticons/v1/%1/1.0";

	inline QString Content(const QString &path)
	{
		return QStringBuilder<QString,QString>(CONTENT_HOST,path);
	}
}