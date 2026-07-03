#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <nlohmann/json.hpp>

namespace appforge::core
{
	using NJson = nlohmann::json;
	namespace json_helper {

		// ✅ Conversion principale
		NJson toJson(const QJsonObject& object);
		QJsonObject fromJson(const NJson& j);

		// ✅ Conversion fine (recommandé)
		NJson toJsonValue(const QJsonValue& value);
		QJsonValue fromJsonValue(const NJson& j);

		// ✅ Helpers fichier
		QByteArray toByteArray(const QJsonObject& object, int indent = 4);
		QJsonObject fromByteArray(const QByteArray& data);
	}
}
