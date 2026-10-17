// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ProfileSelector.h"

#include <QWidget>

#include "ExternalPlugin.h"

NEBULA_REGISTER_COMPONENT(ExternalPlugin, ProfileSelector);

QString ProfileSelector::profiles() const
{
    return m_profiles;
}

void ProfileSelector::setProfiles(const QString& profiles)
{
    if(m_profiles == profiles)
    {
        return;
    }
    m_profiles = profiles;
    emit profilesChanged(m_profiles);
}

QString ProfileSelector::profile() const
{
    return m_profile;
}

void ProfileSelector::setProfile(const QString& profile)
{
    if(m_profile == profile)
    {
        return;
    }
    m_profile = profile;
    emit profileChanged(m_profile);
}

void ProfileSelector::setDisplay(QWidget* display)
{
    provideInterface<QWidget>(QStringLiteral("display"), display);
}
