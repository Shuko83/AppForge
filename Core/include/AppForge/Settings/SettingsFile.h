// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QJsonObject>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVariant>

#include "Core_export.h"

namespace AppForge
{

class Component;
class ForgeApplication;

// A property a settings file holds: one of a component of the application, or of one of the objects the component
// added with Component::addPropertiesForQObject().
struct SettingsEntry
{
    Component* component = nullptr;
    QObject* object = nullptr; // The component itself, or one of its propertyObjects()
    QString property;

    [[nodiscard]] bool operator==(const SettingsEntry& other) const = default;
};

// A file of settings of an application, see ForgeApplication::addSettingsFile(): the values of properties of its
// components, its entries, which the user of the application changes while it runs and finds again the next time it
// starts. The value a project gives such a property is its default value.
// When the application starts, the file gives its values to the properties it holds, once their default values are
// written to it when it does not exist or misses some; while the application runs, the file follows the changes they
// notify; when it stops, the file is written a last time, and the properties have their default values again.
// It holds the properties of any number of components, under their names, as a project holds them; a property is held
// by a single file of the application. What else is in the file is kept: another application can share it.
// Its name is its own among the files of the application, whatever their case and their directories, as two files of
// the same name would be written over each other.
class CORE_EXPORT SettingsFile : public QObject
{
    Q_OBJECT

  public:
    ~SettingsFile() override;

    // As it was added, settings.json, or settings2.json, settings3.json... when another file has that name, until it
    // is given another one.
    [[nodiscard]] QString fileName() const;
    // Returns false, changing nothing, when fileName is empty or the name of another file of the application, whatever
    // its case. Taken into account the next time the application starts.
    bool setFileName(const QString& fileName);
    // Where the file is, see filePath(); empty until it is set.
    [[nodiscard]] QString directory() const;
    // Taken into account the next time the application starts.
    void setDirectory(const QString& directory);
    // filePath() of its file name and its directory.
    [[nodiscard]] QString filePath() const;
    // fileName in directory: in the configuration directory of the user for the application that runs,
    // QStandardPaths::AppConfigLocation, when directory is empty; from the directory of that application when it is
    // relative.
    [[nodiscard]] static QString filePath(const QString& fileName, const QString& directory);

    // In the order they were added.
    [[nodiscard]] const QList<SettingsEntry>& entries() const;
    [[nodiscard]] bool contains(const SettingsEntry& entry) const;
    // Holds entry from the next time the application starts. Returns false, changing nothing, when entry is not a
    // property it can hold, see canHold(), of a component of the application, or when a file of the application holds
    // it already. An entry leaves once its object, or its component, is destroyed.
    bool add(const SettingsEntry& entry);
    // No longer holds entry from the next time the application starts.
    void remove(const SettingsEntry& entry);

    // Writes to its file the values its entries have, those of the project as the application does not run, in place
    // of those it holds for them. Returns false, with errorString() set, while the application runs, nothing being
    // written then, and when the file cannot be written.
    bool writeDefaults();
    // Why writeDefaults() returned false, or failed() was last emitted.
    [[nodiscard]] QString errorString() const;

    // Whether a settings file can hold property of object, component itself or one of its objects: as a project, see
    // ForgeApplication::save(), the property can be written and JSON holds its value.
    [[nodiscard]] static bool canHold(const Component& component, const QObject* object, const QString& property);

  signals:
    // Its file name, its directory or its entries changed.
    void changed();
    // While the application starts or runs, its file could not be read or written; why is a sentence to show. The
    // application runs all the same: without the values it holds, or without saving those the user changes.
    void failed(const QString& why);

  private:
    friend class ForgeApplication; // Creates it, starts and stops it

    // An entry, as it was when the application started.
    struct StartedEntry
    {
        SettingsEntry entry;
        QVariant defaultValue; // The value of the project
    };

    // fileName is one no other file of application has.
    SettingsFile(ForgeApplication& application, QString fileName);

    // Gives the properties it holds the values of its file, writes their default values to it first when it does not
    // exist or misses some, and follows their changes.
    void start();
    // Writes the values of the properties it holds to its file, and gives them their default values again.
    void stop();
    // Tells why through the log and failed().
    void fail(const QString& why);

  private slots:
    // Takes the values the sender, an object of its entries, has now: one of its properties changed. A slot: their
    // notify signals are only known as QMetaMethod.
    void takeValues();
    // Writes the values taken since it last wrote them.
    void writeTaken();
    // Forgets the entries of object, or of its component, being destroyed.
    void forget(QObject* object);

  private:
    ForgeApplication& m_application;
    QString m_fileName;
    QString m_directory;
    QList<SettingsEntry> m_entries;
    QString m_errorString;

    // While the application runs.
    bool m_running = false;
    QString m_startedPath;                     // Where it is written
    bool m_writable = false;                   // Whether it is written: false once that failed
    QJsonObject m_content;                     // What it writes there
    QList<StartedEntry> m_started;             // Its entries when it started: those it follows
    QList<QMetaObject::Connection> m_followed; // To their notify signals
    bool m_writePending = false;               // Whether values were taken since it last wrote them
};

} // namespace AppForge
