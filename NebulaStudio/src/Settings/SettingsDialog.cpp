// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "SettingsDialog.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMetaProperty>
#include <QPushButton>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <cstdint>
#include <memory>
#include <optional>

#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"

namespace
{

enum Column : std::uint8_t
{
    PropertyColumn,
    ValueColumn,
    FileColumn,
    ColumnCount,
};

} // namespace

SettingsDialog::SettingsDialog(Nebula::SettingsFile& file, const Nebula::Universe& application,
                               const Nebula::ComponentFactory& factory, QWidget* parent)
    : QDialog(parent), m_file(file), m_application(application)
{
    setWindowTitle(tr("Settings file"));

    m_fileName = new QLineEdit(file.fileName());
    m_directory = new QLineEdit(file.directory());
    m_directory->setPlaceholderText(tr("Configuration directory of the user"));
    m_directory->setToolTip(tr("Empty: the configuration directory of the user, for the application that runs\n"
                               "Relative: from the directory of the application that runs"));
    auto* browseButton = new QPushButton(tr("Browse..."));
    connect(browseButton, &QPushButton::clicked, this, &SettingsDialog::browse);
    auto* directoryRow = new QHBoxLayout;
    directoryRow->addWidget(m_directory, 1);
    directoryRow->addWidget(browseButton);
    m_path = new QLabel;
    m_path->setWordWrap(true);
    m_path->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_path->setToolTip(tr("Where the application writes it when it runs here, in NebulaStudio"));

    auto* form = new QFormLayout;
    form->addRow(tr("File name:"), m_fileName);
    form->addRow(tr("Directory:"), directoryRow);
    form->addRow(tr("Written to:"), m_path);

    auto* hint = new QLabel(tr("The properties checked keep the values the user of the application gives them, from "
                               "one run to the next. The values of the project are their default values."));
    hint->setWordWrap(true);

    m_tree = new QTreeWidget;
    m_tree->setColumnCount(ColumnCount);
    m_tree->setHeaderLabels({tr("Property"), tr("Value"), tr("Held by")});
    m_tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    fill(application, factory);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(this, &QDialog::accepted, this, &SettingsDialog::apply);
    // Should the file go meanwhile, with what it would have been given.
    connect(&m_file, &QObject::destroyed, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(hint);
    layout->addWidget(m_tree, 1);
    layout->addWidget(m_buttons);

    connect(m_fileName, &QLineEdit::textChanged, this, &SettingsDialog::refreshPath);
    connect(m_directory, &QLineEdit::textChanged, this, &SettingsDialog::refreshPath);
    refreshPath();
    resize(560, 520);
}

void SettingsDialog::fill(const Nebula::Universe& application, const Nebula::ComponentFactory& factory)
{
    for(const std::unique_ptr<Nebula::Component>& component : application.components())
    {
        auto* componentRow = new QTreeWidgetItem({component->objectName()});
        componentRow->setFlags(Qt::ItemIsEnabled);
        componentRow->setToolTip(PropertyColumn, component->componentId());

        // The properties of object a settings file can hold, under parent.
        const auto list = [this, &application, &component](QTreeWidgetItem* parent, QObject* object,
                                                           const QList<QMetaProperty>& properties)
        {
            for(const QMetaProperty& property : properties)
            {
                const QString name = QString::fromLatin1(property.name());
                if(!Nebula::SettingsFile::canHold(*component, object, name))
                {
                    continue;
                }
                const Nebula::SettingsEntry entry{.component = component.get(), .object = object, .property = name};
                auto* row = new QTreeWidgetItem(parent, {name, property.read(object).toString()});
                row->setToolTip(PropertyColumn, QString::fromLatin1(property.typeName()));
                const Nebula::SettingsFile* holder = application.settingsFile(entry);
                if(holder != nullptr && holder != &m_file)
                {
                    row->setFlags(Qt::NoItemFlags); // Only one file holds a property
                    row->setText(FileColumn, holder->fileName());
                    continue;
                }
                row->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                row->setCheckState(PropertyColumn, holder != nullptr ? Qt::Checked : Qt::Unchecked);
                m_entries.insert(row, entry);
            }
        };

        if(const std::optional<Nebula::ComponentInfo> info = factory.component(component->componentId()))
        {
            list(componentRow, component.get(), info->properties);
        }
        // Then those of its objects, as the property editor shows them: with its own when they have no name, under a
        // row of their name otherwise.
        const QList<Nebula::PropertyObject> objects = component->propertyObjects();
        QStringList names{QString()};
        for(const Nebula::PropertyObject& added : objects)
        {
            if(!names.contains(added.name))
            {
                names.append(added.name);
            }
        }
        for(const QString& name : std::as_const(names))
        {
            auto* group = name.isEmpty() ? componentRow : new QTreeWidgetItem({name});
            group->setFlags(Qt::ItemIsEnabled);
            for(const Nebula::PropertyObject& added : objects)
            {
                if(added.name == name)
                {
                    list(group, added.object, added.properties());
                }
            }
            if(group != componentRow)
            {
                if(group->childCount() > 0)
                {
                    componentRow->addChild(group);
                }
                else
                {
                    delete group; // Nothing to hold
                }
            }
        }

        if(componentRow->childCount() > 0)
        {
            m_tree->addTopLevelItem(componentRow);
            componentRow->setExpanded(true);
        }
        else
        {
            delete componentRow;
        }
    }
    // The groups of the objects, which can have many properties, a widget say, are only open when a file holds one.
    for(QTreeWidgetItemIterator row(m_tree); *row != nullptr; ++row)
    {
        const bool held = (*row)->checkState(PropertyColumn) == Qt::Checked || !(*row)->text(FileColumn).isEmpty();
        for(QTreeWidgetItem* parent = (*row)->parent(); held && parent != nullptr; parent = parent->parent())
        {
            parent->setExpanded(true);
        }
    }
}

void SettingsDialog::refreshPath()
{
    const QString fileName = m_fileName->text().trimmed();
    const Nebula::SettingsFile* named = m_application.settingsFile(fileName);
    const bool taken = named != nullptr && named != &m_file; // Its own name, whatever its case, is its own
    if(fileName.isEmpty())
    {
        m_path->setText(tr("A file name is needed"));
    }
    else if(taken)
    {
        m_path->setText(tr("Another settings file is named %1").arg(named->fileName()));
    }
    else
    {
        m_path->setText(
            QDir::toNativeSeparators(Nebula::SettingsFile::filePath(fileName, m_directory->text().trimmed())));
    }
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(!fileName.isEmpty() && !taken);
}

void SettingsDialog::browse()
{
    // From where the file is now.
    const QString fileName = m_fileName->text().trimmed();
    const QString current = Nebula::SettingsFile::filePath(fileName.isEmpty() ? m_file.fileName() : fileName,
                                                           m_directory->text().trimmed());
    const QString directory = QFileDialog::getExistingDirectory(this, tr("Directory of the settings file"),
                                                                QFileInfo(current).absolutePath());
    if(!directory.isEmpty())
    {
        m_directory->setText(directory);
    }
}

void SettingsDialog::apply()
{
    m_file.setFileName(m_fileName->text().trimmed());
    m_file.setDirectory(m_directory->text().trimmed());
    // In the order of the tree: the one the file holds them in.
    for(QTreeWidgetItemIterator row(m_tree); *row != nullptr; ++row)
    {
        const auto entry = m_entries.constFind(*row);
        if(entry == m_entries.cend())
        {
            continue;
        }
        if((*row)->checkState(PropertyColumn) != Qt::Checked)
        {
            m_file.remove(*entry);
        }
        else if(!m_file.contains(*entry))
        {
            m_file.add(*entry);
        }
    }
}
