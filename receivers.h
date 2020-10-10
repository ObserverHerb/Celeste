#pragma once

#include <QObject>
#include <QTimer>
#include "command.h"

class MessageReceiver : public QObject
{
	Q_OBJECT
public:
	MessageReceiver(QObject *parent=nullptr) noexcept;
signals:
	void Succeeded();
	void Failed();
	void Print(const QString text);
public slots:
	virtual void Process(const QString data)=0;
};

class StatusReceiver : public MessageReceiver
{
	Q_OBJECT
public:
	StatusReceiver(QObject *parent=nullptr) noexcept : MessageReceiver(parent) { }
protected:
	void Interpret(const QString text);
};

class AuthenticationReceiver : public StatusReceiver
{
	Q_OBJECT
public:
	AuthenticationReceiver(QObject *parent=nullptr) noexcept : StatusReceiver(parent) { }
public slots:
	void Process(const QString data) override;
};


class ChannelJoinReceiver : public StatusReceiver
{
	Q_OBJECT
public:
	ChannelJoinReceiver(QObject *parent=nullptr) noexcept;
protected:
	QTimer failureDelay;
public slots:
	void Process(const QString data) override;
protected slots:
	void Fail();
};

class ChatMessageReceiver : public MessageReceiver // must catch std::runtime_error and std::out_of_range
{
	Q_OBJECT
public:
	ChatMessageReceiver(QObject *parent=nullptr);
	void AttachCommand(const Command &command);
protected:
	std::unordered_map<QString,Command> commands;
signals:
	void PlayVideo(const QString path);
	void Speak(const QString sentence);
	void ShowVoices();
	void DispatchCommand(const Command &command);
public slots:
	void Process(const QString data) override;
};
