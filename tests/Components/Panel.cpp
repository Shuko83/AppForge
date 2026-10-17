// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Panel.h"

#include "Options.h"

namespace
{

constexpr int PageCount = 2;

} // namespace

Panel::Panel() : m_general(new Options(this))
{
    addPropertiesForQObject(m_general);
    for(int page = 0; page < PageCount; ++page)
    {
        m_pages.append(new Options(this));
        addPropertiesForQObject(m_pages.last(), QStringLiteral("page"));
    }
}

Options& Panel::general() const
{
    return *m_general;
}

QList<Options*> Panel::pages() const
{
    return m_pages;
}
