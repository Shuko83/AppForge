// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QDialog>
#include <QHash>

#include "AppForge/Settings/SettingsFile.h"

class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

namespace AppForge
{
class ComponentFactory;
class ForgeApplication;
} // namespace AppForge

// Edits a settings file of the application: the name of its file, its directory, and the properties it holds, checked
// among those of the components of the application, then of their objects, that a settings file can hold. A property
// another file holds is listed with the name of that file, and cannot be checked. What was edited is given to the
// file once the dialog is accepted.
class SettingsDialog : public QDialog
{
    Q_OBJECT

  public:
    // file, one of those of application, outlives the dialog.
    SettingsDialog(AppForge::SettingsFile& file, const AppForge::ForgeApplication& application,
                   const AppForge::ComponentFactory& factory, QWidget* parent = nullptr);

  private:
    // Lists the properties of the components of application.
    void fill(const AppForge::ForgeApplication& application, const AppForge::ComponentFactory& factory);
    // Shows where the file is, as its name and its directory are edited; it is not accepted without a name, nor with
    // the one of another file of the application.
    void refreshPath();
    // Asks for the directory of the file.
    void browse();
    // Gives the file what was edited.
    void apply();

    AppForge::SettingsFile& m_file;
    const AppForge::ForgeApplication& m_application;
    QHash<const QTreeWidgetItem*, AppForge::SettingsEntry> m_entries; // The entry each property of the tree is
    QLineEdit* m_fileName = nullptr;
    QLineEdit* m_directory = nullptr;
    QLabel* m_path = nullptr;
    QTreeWidget* m_tree = nullptr;
    QDialogButtonBox* m_buttons = nullptr;
};
