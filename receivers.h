#pragma once

#include <QObject>
#include <QTimer>
#include <vector>
#include "command.h"
#include "viewers.h"

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
	ChatMessageReceiver(QObject *parent) : ChatMessageReceiver(std::vector<Command>(),parent) { }
	ChatMessageReceiver(std::vector<Command> builtInCommands,QObject *parent=nullptr);
	void AttachCommand(const Command &command);
	const Command RandomCommand() const;
protected:
	std::unordered_map<QString,Command> commands;
	std::unordered_map<QString,std::reference_wrapper<Command>> commandAliases;
	std::vector<Command> userCommands;
	Viewers viewers;
	void IdentifyViewer(const QString &name);
	Command* FindCommand(const QString &name);
signals:
	void Alert(const QString &text);
	void ArrivalConfirmed(const Viewer &viewer);
	void PlayVideo(const QString &path);
	void PlayAudio(const QString &user,const QString &message,const QString &path);
	void Speak(const QString sentence);
	void ShowVoices();
	void DispatchCommand(const Command &command);
public slots:
	void Process(const QString data) override;
};
