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

struct Command
{
	QString name;
	CommandType type;
	bool random;
	QString path;
};

const QString JSON_KEY_COMMAND_NAME("command");
const QString JSON_KEY_COMMAND_TYPE("type");
const QString JSON_KEY_COMMAND_RANDOM_PATH("random");
const QString JSON_KEY_COMMAND_PATH("path");
