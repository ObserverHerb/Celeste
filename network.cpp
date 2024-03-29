#include "network.h"
#include "globals.h"

QNetworkAccessManager networkManager;

namespace Network
{
	std::queue<Request*> Request::queue;

	Request* Request::Send(const QUrl &url,Method method,Callback callback,const QUrlQuery &queryParameters,const Headers &headers,const QByteArray &payload)
	{
		Request *request=new Request(url,method,callback,queryParameters,headers,payload);
		request->Send();
		return request;
	}

	Request::Request(const QUrl &url,Method method,Callback callback,const QUrlQuery &queryParameters,const Headers &headers,const QByteArray &payload) : url(url),
		method(method),
		callback(callback),
		queryParameters(queryParameters),
		headers(headers),
		payload(payload),
		reply(nullptr)
	{
	}

	void Request::Send()
	{
		for (const Header &header : headers) request.setRawHeader(header.first,header.second);
		switch (method)
		{
		case Method::GET:
		{
			url.setQuery(queryParameters);
			request.setUrl(url);
			if (queue.size() == 0) DeferredSend(); // if it's the first one going in the queue, trigger it
			queue.push(this);
			return;
		}
		case Method::POST:
			request.setUrl(url);
			reply=networkManager.post(request,payload.isEmpty() ? StringConvert::ByteArray(queryParameters.query()) : payload);
			break;
		case Method::PATCH:
			url.setQuery(queryParameters);
			request.setUrl(url);
			reply=networkManager.sendCustomRequest(request,"PATCH"_ba,payload);
			break;
		case Method::DELETE:
			url.setQuery(queryParameters);
			request.setUrl(url);
			reply=networkManager.sendCustomRequest(request,"DELETE"_ba,payload);
			break;
		}
		connect(reply,&QNetworkReply::finished,this,&Request::Finished);
	}

	void Request::DeferredSend()
	{
		reply=networkManager.get(request);
		reply->connect(reply,&QNetworkReply::finished,this,&Request::DeferredFinished);
	}

	void Request::Finished()
	{
		callback(reply);
		reply->deleteLater();
		deleteLater();
	}

	void Request::DeferredFinished()
	{
		// remove this network call and make the next network call if there is one waiting in the queue
		queue.pop();
		if (queue.size() > 0) queue.front()->DeferredSend();
		Finished();
	}
}