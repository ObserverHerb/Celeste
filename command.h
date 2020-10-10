#pragma once

#include <QString>
#include <unordered_map>

enum class CommandType
{
	DISPATCH,
	VIDEO
};
using CommandTypeLookup=std::unordered_map<QString,CommandType>;
const CommandTypeLookup COMMAND_TYPES={
	{"dispatch",CommandType::DISPATCH},
	{"video",CommandType::VIDEO}
};

class Command
{
public:
	Command() : Command(QString(),CommandType::DISPATCH,false,QString()) { }
	Command(const QString &name,const CommandType &type) : Command(name,type,false,QString()) { }
	Command(const QString &name,const CommandType &type,bool random,const QString &path) : name(name), type(type), random(random), path(path) { }
	const QString& Name() const { return name; }
	CommandType Type() const { return type; }
	bool Random() const { return random; }
	const QString& Path() { return path; }
protected:
	QString name;
	CommandType type;
	bool random;
	QString path;
};

const QString JSON_KEY_COMMAND_NAME("command");
const QString JSON_KEY_COMMAND_TYPE("type");
const QString JSON_KEY_COMMAND_RANDOM_PATH("random");
const QString JSON_KEY_COMMAND_PATH("path");
