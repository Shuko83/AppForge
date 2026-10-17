// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Settings/SettingsFile.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <algorithm>
#include <optional>
#include <utility>

#include "Application/ProjectJson.h"
#include "Nebula/Application/ForgeApplication.h"
#include "Nebula/Component/Component.h"

namespace Nebula
{

namespace
{

Q_LOGGING_CATEGORY(lcSettings, "nebula.settings")

// The property of entry; not valid when its object has none of that name.
QMetaProperty propertyOf(const SettingsEntry& entry)
{
    const QMetaObject* metaObject = entry.object->metaObject();
    return metaObject->property(metaObject->indexOfProperty(entry.property.toLatin1().constData()));
}

// The value of the property of entry, as a file holds it; null when JSON cannot hold it.
QJsonValue currentValue(const SettingsEntry& entry)
{
    return ProjectJson::toJson(propertyOf(entry).read(entry.object)).value_or(QJsonValue());
}

// The value content holds for entry, as a project holds the properties of a component: in its "properties", or in
// those of one of its "objects", found by its name, then in the order they were added. Undefined when it holds none.
QJsonValue valueOf(const QJsonObject& content, const SettingsEntry& entry)
{
    const QJsonObject component = content.value(entry.component->objectName()).toObject();
    if(entry.object == entry.component)
    {
        return component.value(u"properties").toObject().value(entry.property);
    }
    const std::optional<ProjectJson::ObjectKey> key = ProjectJson::objectKey(*entry.component, entry.object);
    qsizetype index = 0; // Of the objects of its name met so far
    const QJsonArray objects = component.value(u"objects").toArray();
    for(const QJsonValue& value : objects)
    {
        const QJsonObject object = value.toObject();
        if(key && object.value(u"name").toString() == key->name && index++ == key->index)
        {
            return object.value(u"properties").toObject().value(entry.property);
        }
    }
    return QJsonValue(QJsonValue::Undefined);
}

// Sets the value content holds for entry, where valueOf() finds it, to value, adding what is missing for it.
void setValue(QJsonObject& content, const SettingsEntry& entry, const QJsonValue& value)
{
    // holder, a component or an object, with value in its "properties".
    const auto set = [&entry, &value](QJsonObject holder)
    {
        QJsonObject properties = holder.value(u"properties").toObject();
        properties.insert(entry.property, value);
        holder.insert(u"properties", properties);
        return holder;
    };

    const QString name = entry.component->objectName();
    QJsonObject component = content.value(name).toObject();
    if(entry.object == entry.component)
    {
        component = set(component);
    }
    else
    {
        const std::optional<ProjectJson::ObjectKey> key = ProjectJson::objectKey(*entry.component, entry.object);
        if(!key)
        {
            return;
        }
        QJsonArray objects = component.value(u"objects").toArray();
        qsizetype found = -1;
        qsizetype index = 0; // Of the objects of its name met so far
        for(qsizetype position = 0; position < objects.size() && found < 0; ++position)
        {
            if(objects.at(position).toObject().value(u"name").toString() == key->name && index++ == key->index)
            {
                found = position;
            }
        }
        // Added when missing, after as many objects of its name as were added before it.
        while(found < 0)
        {
            QJsonObject added;
            added.insert(u"name", key->name);
            added.insert(u"properties", QJsonObject());
            objects.append(added);
            if(index++ == key->index)
            {
                found = objects.size() - 1;
            }
        }
        objects.replace(found, set(objects.at(found).toObject()));
        component.insert(u"objects", objects);
    }
    content.insert(name, component);
}

// What the settings file filePath holds: nothing when it does not exist. A file that cannot be read is set aside, a
// ".bak" added to its name, for writing it again not to lose what is in it, and problem tells it; nullopt when it
// cannot be set aside: it must not be written then.
std::optional<QJsonObject> readSettings(const QString& filePath, QString& problem)
{
    QFile file(filePath);
    if(!file.exists())
    {
        return QJsonObject();
    }
    QString why;
    if(file.open(QIODevice::ReadOnly))
    {
        QJsonParseError parsed{};
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parsed);
        if(parsed.error == QJsonParseError::NoError && document.isObject())
        {
            return document.object();
        }
        why = parsed.error == QJsonParseError::NoError ? QStringLiteral("it does not hold a JSON object")
                                                       : parsed.errorString();
        file.close();
    }
    else
    {
        why = file.errorString();
    }
    const QString backup = filePath + QStringLiteral(".bak");
    QFile::remove(backup); // The one of a former time
    if(!QFile::rename(filePath, backup))
    {
        problem = QStringLiteral("%1 cannot be read (%2), nor set aside: it is not written")
                      .arg(QDir::toNativeSeparators(filePath), why);
        return std::nullopt;
    }
    problem = QStringLiteral("%1 cannot be read (%2): it is set aside as %3 and written again")
                  .arg(QDir::toNativeSeparators(filePath), why, QDir::toNativeSeparators(backup));
    return QJsonObject();
}

// Writes content to the settings file filePath, creating its directory when needed; false, with error set, when it
// cannot.
bool writeSettings(const QString& filePath, const QJsonObject& content, QString& error)
{
    QSaveFile file(filePath);
    if(!QDir().mkpath(QFileInfo(filePath).absolutePath()) || !file.open(QIODevice::WriteOnly) ||
       file.write(QJsonDocument(content).toJson()) < 0 || !file.commit())
    {
        error = QStringLiteral("%1 cannot be written: %2").arg(QDir::toNativeSeparators(filePath), file.errorString());
        return false;
    }
    return true;
}

} // namespace

SettingsFile::SettingsFile(ForgeApplication& application, QString fileName)
    : m_application(application), m_fileName(std::move(fileName))
{
}

SettingsFile::~SettingsFile() = default;

QString SettingsFile::fileName() const
{
    return m_fileName;
}

bool SettingsFile::setFileName(const QString& fileName)
{
    const SettingsFile* named = m_application.settingsFile(fileName);
    if(fileName.isEmpty() || (named != nullptr && named != this)) // Its own name in another case is its own
    {
        return false;
    }
    if(fileName != m_fileName)
    {
        m_fileName = fileName;
        emit changed();
    }
    return true;
}

QString SettingsFile::directory() const
{
    return m_directory;
}

void SettingsFile::setDirectory(const QString& directory)
{
    if(directory == m_directory)
    {
        return;
    }
    m_directory = directory;
    emit changed();
}

QString SettingsFile::filePath() const
{
    return filePath(m_fileName, m_directory);
}

QString SettingsFile::filePath(const QString& fileName, const QString& directory)
{
    if(directory.isEmpty())
    {
        return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + u'/' + fileName;
    }
    // As it is when it is absolute.
    return QDir::cleanPath(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(directory) + u'/' + fileName);
}

const QList<SettingsEntry>& SettingsFile::entries() const
{
    return m_entries;
}

bool SettingsFile::contains(const SettingsEntry& entry) const
{
    return m_entries.contains(entry);
}

bool SettingsFile::add(const SettingsEntry& entry)
{
    const auto known = [&entry](const std::unique_ptr<Component>& component)
    { return component.get() == entry.component; };
    if(entry.component == nullptr || !std::ranges::any_of(m_application.components(), known) ||
       !canHold(*entry.component, entry.object, entry.property) || m_application.settingsFile(entry) != nullptr)
    {
        return false;
    }
    m_entries.append(entry);
    connect(entry.component, &QObject::destroyed, this, &SettingsFile::forget, Qt::UniqueConnection);
    connect(entry.object, &QObject::destroyed, this, &SettingsFile::forget, Qt::UniqueConnection);
    emit changed();
    return true;
}

void SettingsFile::remove(const SettingsEntry& entry)
{
    if(m_entries.removeAll(entry) > 0)
    {
        emit changed();
    }
}

bool SettingsFile::writeDefaults()
{
    if(m_running)
    {
        m_errorString = QStringLiteral("The default values cannot be written while the application runs");
        return false;
    }
    const QString path = filePath();
    QString problem;
    std::optional<QJsonObject> content = readSettings(path, problem);
    if(!content)
    {
        m_errorString = problem;
        return false;
    }
    for(const SettingsEntry& entry : std::as_const(m_entries))
    {
        setValue(*content, entry, currentValue(entry));
    }
    return writeSettings(path, *content, m_errorString);
}

QString SettingsFile::errorString() const
{
    return m_errorString;
}

bool SettingsFile::canHold(const Component& component, const QObject* object, const QString& property)
{
    const QList<QMetaProperty> properties = ProjectJson::savedProperties(component, object);
    return std::ranges::any_of(properties, [&property](const QMetaProperty& saved)
                               { return QLatin1StringView(saved.name()) == property; });
}

void SettingsFile::start()
{
    m_running = true;
    m_startedPath = filePath();
    QString problem;
    const std::optional<QJsonObject> content = readSettings(m_startedPath, problem);
    if(!problem.isEmpty())
    {
        fail(problem);
    }
    m_writable = content.has_value();
    m_content = content.value_or(QJsonObject());

    bool complete = true; // Whether the file holds a value for each entry
    for(const SettingsEntry& entry : std::as_const(m_entries))
    {
        const QMetaProperty property = propertyOf(entry);
        m_started.append(StartedEntry{.entry = entry, .defaultValue = property.read(entry.object)});
        const QJsonValue value = valueOf(m_content, entry);
        if(value.isUndefined() || !property.write(entry.object, value.toVariant()))
        {
            if(!value.isUndefined())
            {
                fail(QStringLiteral("%1 holds for %2 of %3 a value it cannot have: it is given its default value")
                         .arg(QDir::toNativeSeparators(m_startedPath), entry.property, entry.component->objectName()));
            }
            setValue(m_content, entry, currentValue(entry));
            complete = false;
        }
    }
    if(!complete && m_writable && !writeSettings(m_startedPath, m_content, m_errorString))
    {
        m_writable = false; // Told once
        fail(m_errorString);
    }

    // Once their values are given: those are not changes to write.
    const QMetaMethod take = metaObject()->method(metaObject()->indexOfSlot("takeValues()"));
    for(const StartedEntry& started : std::as_const(m_started))
    {
        const QMetaProperty property = propertyOf(started.entry);
        if(property.hasNotifySignal())
        {
            const QMetaObject::Connection followed =
                connect(started.entry.object, property.notifySignal(), this, take, Qt::UniqueConnection);
            if(followed) // Not when another property of the object has the same notify signal
            {
                m_followed.append(followed);
            }
        }
    }
}

void SettingsFile::stop()
{
    for(const QMetaObject::Connection& followed : std::as_const(m_followed))
    {
        disconnect(followed);
    }
    m_followed.clear();
    // Those no notify signal tells the changes of as well.
    for(const StartedEntry& started : std::as_const(m_started))
    {
        setValue(m_content, started.entry, currentValue(started.entry));
    }
    m_writePending = false;
    if(m_writable && !writeSettings(m_startedPath, m_content, m_errorString))
    {
        fail(m_errorString);
    }
    for(const StartedEntry& started : std::as_const(m_started))
    {
        propertyOf(started.entry).write(started.entry.object, started.defaultValue);
    }
    m_started.clear();
    m_content = {};
    m_running = false;
}

void SettingsFile::fail(const QString& why)
{
    m_errorString = why;
    qCWarning(lcSettings).noquote() << why;
    emit failed(why);
}

void SettingsFile::takeValues()
{
    const QObject* changed = sender();
    for(const StartedEntry& started : std::as_const(m_started))
    {
        if(started.entry.object == changed)
        {
            setValue(m_content, started.entry, currentValue(started.entry));
        }
    }
    // Once for the changes made meanwhile: a property can change at every event.
    if(!m_writePending)
    {
        m_writePending = true;
        QTimer::singleShot(0, this, &SettingsFile::writeTaken);
    }
}

void SettingsFile::writeTaken()
{
    if(!m_writePending) // Written meanwhile, as the application stopped
    {
        return;
    }
    m_writePending = false;
    if(m_writable && !writeSettings(m_startedPath, m_content, m_errorString))
    {
        m_writable = false; // Told once
        fail(m_errorString);
    }
}

void SettingsFile::forget(QObject* object)
{
    const auto of = [object](const SettingsEntry& entry)
    { return entry.object == object || entry.component == object; };
    m_started.removeIf([&of](const StartedEntry& started) { return of(started.entry); });
    if(m_entries.removeIf(of) > 0)
    {
        emit changed();
    }
}

} // namespace Nebula
