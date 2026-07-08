#include "AfCore/Version.h"

#include <cstring>

using namespace appforge::core;

namespace JsonKeys
{
	const std::string MAJOR = "major";
	const std::string MINOR = "minor";
	const std::string PATCH = "patch";
}

Version::Version(unsigned int major, unsigned int minor, unsigned int patch)
	: _major(major)
	, _minor(minor)
	, _patch(patch)
{
}

unsigned int Version::major() const
{
	return _major;
}

void Version::setMajor(unsigned int value)
{
	_major = value;
}

unsigned int Version::minor() const
{
	return _minor;
}

void Version::setMinor(unsigned int value)
{
	_minor = value;
}

unsigned int Version::patch() const
{
	return _patch;
}

void Version::setPatch(unsigned int value)
{
	_patch = value;
}

QString Version::toString() const
{
	return QString("%1.%2.%3").arg(_major).arg(_minor).arg(_patch);
}

json Version::toJson() const
{
	return json{
		{JsonKeys::MAJOR, _major},
		{JsonKeys::MINOR, _minor},
		{JsonKeys::PATCH, _patch}
	};
}

void Version::fromJson(const json& j)
{
	_major = j.value(JsonKeys::MAJOR, 0);
	_minor = j.value(JsonKeys::MINOR, 0);
	_patch = j.value(JsonKeys::PATCH, 0);
}
