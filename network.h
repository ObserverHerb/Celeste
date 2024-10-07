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

	class Request final : public QObject
	{
		Q_OBJECT
	public:
		static Request* Send(const QUrl &url,Method method,Callback callback,const QUrlQuery &queryParameters=QUrlQuery{},const Headers &headers=Headers{},const QByteArray &payload=QByteArray{});
	private:
		Request(const QUrl &url,Method method,Callback callback,const QUrlQuery &queryParameters,const Headers &headers,const QByteArray &payload);
		QUrl url;
		Method method;
		Callback callback;
		QUrlQuery queryParameters;
		Headers headers;
		QByteArray payload;
		QNetworkRequest request;
		QNetworkReply *reply;
		static std::unique_ptr<QNetworkAccessManager> networkManager;
		static std::queue<Request*> queue;
		void Send();
		void DeferredSend();
	private slots:
		void Finished();
		void DeferredFinished();
	};
}