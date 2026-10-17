// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>

#include "Nebula/Plugin/Plugin.h"

class Panel;
class Probe;
class Sink;
class Source;

// The plugin class of the components of the tests, in the process of the test rather than in a plugin file: see
// registerTestComponents().
class TestComponents : public Nebula::Plugin
{
  public:
    // The plugin id they are registered under: their ids are Test.<class name>.
    static constexpr QLatin1StringView Id{"Test"};

    void build(Probe& probe);
    void build(Source& source);
    void build(Sink& sink);
    void build(Panel& panel);
};

// Registers the components of the tests in ComponentFactory::instance(), under TestComponents::Id, the first time it is
// called: a test calls it before it creates any. Returns their plugin class, which lives until the test exits.
Nebula::Plugin& registerTestComponents();

// The id of the component of the tests named name, e.g. Test.Probe.
[[nodiscard]] QString testComponentId(const QString& name);
