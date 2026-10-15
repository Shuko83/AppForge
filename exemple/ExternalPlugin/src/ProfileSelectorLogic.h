// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QComboBox>
#include <QObject>

class ProfileSelector;

// Logic of a ProfileSelector, built with it by ExternalPlugin: lists its profiles, the one it has selected, in the
// display it provides, and has it choose the one the user selects there.
class ProfileSelectorLogic : public QObject
{
    Q_OBJECT

  public:
    // A child of selector, destroyed with it.
    explicit ProfileSelectorLogic(ProfileSelector& selector);

  private:
    // Lists the profiles of the selector as they are now.
    void showProfiles();

    ProfileSelector& m_selector;
    QComboBox m_list;
};
