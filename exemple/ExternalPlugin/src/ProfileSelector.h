// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>

#include "AppForge/Component/Component.h"

class QWidget;

// Lets the user of the application choose a profile among those it offers, in the list of ProfileSelectorLogic, which
// ExternalPlugin builds it with. It provides the interface "display", a QWidget showing that list: it needs a
// QApplication. Held by a settings file, its profile is the one the user finds again the next time.
class ProfileSelector : public AppForge::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Lets the user choose a profile among those it offers")
    Q_CLASSINFO("category", "Example")
    Q_PROPERTY(QString profiles READ profiles WRITE setProfiles NOTIFY profilesChanged)
    Q_PROPERTY(QString profile READ profile WRITE setProfile NOTIFY profileChanged)

  public:
    // Their names, separated by commas.
    [[nodiscard]] QString profiles() const;
    void setProfiles(const QString& profiles);
    // The one chosen, which the list only shows when it is one of profiles().
    [[nodiscard]] QString profile() const;
    void setProfile(const QString& profile);

    // Provides display as the interface "display": only its logic does, once.
    void setDisplay(QWidget* display);

  signals:
    void profilesChanged(const QString& profiles);
    void profileChanged(const QString& profile);

  private:
    QString m_profiles = QStringLiteral("Default,Work,Home");
    QString m_profile = QStringLiteral("Default");
};
