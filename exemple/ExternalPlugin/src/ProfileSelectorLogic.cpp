// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ProfileSelectorLogic.h"

#include <QSignalBlocker>

#include "ProfileSelector.h"

ProfileSelectorLogic::ProfileSelectorLogic(ProfileSelector& selector) : QObject(&selector), m_selector(selector)
{
    showProfiles();
    connect(&m_selector, &ProfileSelector::profilesChanged, this, &ProfileSelectorLogic::showProfiles);
    connect(&m_selector, &ProfileSelector::profileChanged, this,
            [this](const QString& profile) { m_list.setCurrentIndex(m_list.findText(profile)); });
    // Only what the user selects: not the selection the list makes as it is filled.
    connect(&m_list, &QComboBox::textActivated, &m_selector, &ProfileSelector::setProfile);
    m_selector.setDisplay(&m_list);
}

void ProfileSelectorLogic::showProfiles()
{
    const QSignalBlocker blocker(m_list);
    m_list.clear();
    for(const QString& profile : m_selector.profiles().split(u',', Qt::SkipEmptyParts))
    {
        m_list.addItem(profile.trimmed());
    }
    m_list.setCurrentIndex(m_list.findText(m_selector.profile())); // None when it is not one of them
}
