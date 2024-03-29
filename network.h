#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <queue>

namespace Network
{
	inline const char *CONTENT_TYPE="Content-Type";
	inline const char *CONTENT_TYPE_PLAIN="text/plain";
	inline const char* CONTENT_TYPE_HTML="text/html";
	inline const char *CONTENT_TYPE_JSON="application/json";
	inline const char *CONTENT_TYPE_FORM="application/x-www-form-urlencoded";

	enum class Method
	{
		GET,
		POST,
		PATCH,
		DELETE
	};

	using Header=std::pair<QByteArray,QByteArray>;
	using Headers=std::vector<Header>;
	using Callback=std::function<void(QNetworkReply*)>;

	class Request : public QObject
	{
		Q_OBJECT
	public:
		Request(const QUrl &url,Method method,Callback callback,const QUrlQuery &queryParameters,const Headers &headers,const QByteArray &payload);
		static Request* Send(const QUrl &url,Method method,Callback callback,const QUrlQuery &queryParameters=QUrlQuery{},const Headers &headers=Headers{},const QByteArray &payload=QByteArray{});
	protected:
		QUrl url;
		Method method;
		Callback callback;
		QUrlQuery queryParameters;
		Headers headers;
		QByteArray payload;
		QNetworkRequest request;
		QNetworkReply *reply;
		static std::queue<Request*> queue;
		void Send();
		void DeferredSend();
	protected slots:
		void Finished();
		void DeferredFinished();
	};
}