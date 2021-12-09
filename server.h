#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include <QUrlQuery>
#include <optional>
#include <unordered_map>
#include "settings.h"

class Server : public QTcpServer
{
	Q_OBJECT
public:
	Server(QObject *parent=nullptr);
	virtual ~Server();
	bool Listen();
protected:
	std::unordered_map<qintptr,QTcpSocket*> sockets;
	ApplicationSetting settingListenPort;
	const std::unordered_map<QString,QString> ParseHeaders(const QStringList &lines);
	virtual std::optional<QByteArray> SocketRead(qintptr socketID) const;
signals:
	void Print(const QString &message,const QString &operation=QString(),const QString &subsystem=QString("server")) const;
	void Dispatch(qintptr socketID,const QUrlQuery &query,const std::unordered_map<QString,QString> &headers,const QString &content);
public slots:
	virtual void SocketWrite(qintptr socketID,const QString &data) const;
protected slots:
	virtual void DataAvailable(qintptr socketID);
	void ConnectionAvailable();
};
