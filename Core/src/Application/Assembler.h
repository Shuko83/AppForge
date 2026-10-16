// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <memory>
#include <vector>

namespace AppForge
{

class Component;
class Composite;
class ForgeApplication;
class PluginManager;

// Builds the components of an application, or of a composite, from what a project file or a composite file holds, see
// README.md, and writes them back: ForgeApplication uses one for each project it loads and each composite it places.
// The application reads each composite file once, the first time one of its instances is placed, into a definition:
// the structure its instances share and the values they start with. Its components are those of an instance that
// never runs and is bound to nothing, which its bindings are kept apart from: bound, a component can act already, as a
// Window shows what it is given. A composite file that contains itself, directly or through others, is refused, with
// the files that hold it.
// What it could not restore is reported as problems, a sentence each: the rest is restored all the same.
class Assembler
{
  public:
    using Components = std::vector<std::unique_ptr<Component>>;

    explicit Assembler(ForgeApplication& application);

    // What it could not restore so far.
    [[nodiscard]] const QStringList& problems() const;

    // Loads the plugins document holds, as a project holds them: found by their id among those of the application,
    // loaded when they are not, or added from their file, relative to the plugin root, when none has their id.
    void loadPlugins(const QJsonObject& document);
    // Appends to components those document holds, as a project holds them, with their values, then, when bound, binds
    // them as it asks: unbound, its composites are not bound either. The files of its composites are relative to
    // directory. owner is the composite they are of, which its gateways are attached to: nullptr for those of the
    // application, which has none. prefix is put before their names in the problems: the path of the composite they
    // are of, with its separator, when they are. Returns false when a composite file being read contains itself: none
    // of those being read can be read then.
    bool load(Components& components, const QJsonObject& document, const QDir& directory, const QString& prefix,
              bool bound = true, Composite* owner = nullptr);
    // A new instance of the composite file filePath, absolute, with the structure and the values of its definition,
    // read when it is not yet, bound as load() binds; nullptr, with error set, when the file cannot be read, or
    // contains itself.
    std::unique_ptr<Composite> instantiate(const QString& filePath, QString& error, bool bound = true);

    // The definition of filePath, absolute, read when it is not yet; false, with error set, when it cannot be.
    bool define(const QString& filePath, QString& error);

    // components as a project holds them: the "plugins" they are from, the "components" with their values, and the
    // "bindings" of what they consume to what they provide among them. The files of their composites are written
    // relative to directory, absolute when it is empty; those of their plugins relative to pluginRoot.
    [[nodiscard]] static QJsonObject save(const QList<const Component*>& components, const QString& directory,
                                          const PluginManager& plugins, const QDir& pluginRoot);
    [[nodiscard]] static QJsonObject save(const Components& components, const QString& directory,
                                          const PluginManager& plugins, const QDir& pluginRoot);
    // The file a composite file name in directory is, as load() finds it: canonical when it exists.
    [[nodiscard]] static QString compositeFilePath(const QDir& directory, const QString& fileName);

    // What the composite file of definition holds, as it is now: its "plugins", its "components" with the values they
    // start with, and its "bindings", the files of its composites relative to its own.
    [[nodiscard]] static QJsonObject document(const ForgeApplication& application, const QString& filePath);

  private:
    // Binds among components what bindings ask, as a project holds them, prefix naming them in the problems as load()
    // does; unbound, only checks they could be. Returns those that could be.
    QJsonArray bind(const Components& components, const QJsonArray& bindings, const QString& prefix, bool bound);
    // A composite of filePath, its components being those document holds, bound as load() binds; nullptr, with error
    // set, when one of its composites contains itself.
    std::unique_ptr<Composite> build(const QString& filePath, const QJsonObject& document, QString& error, bool bound);
    // Appends why, after the file of the definition being read, when one is.
    void problem(const QString& why);

    ForgeApplication& m_application;
    QStringList m_problems;
    QStringList m_reading; // The composite files whose definition is being read, each in the one before it
    QString m_context;     // Put before the problems: the name of the file being read, when one is
    QString m_cycle;       // Why the composite files being read cannot be, when one contains itself
};

} // namespace AppForge
