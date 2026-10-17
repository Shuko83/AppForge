// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "TestComponents.h"

#include <memory>

#include "Nebula/Component/ComponentFactory.h"
#include "Panel.h"
#include "Probe.h"
#include "Sink.h"
#include "Source.h"

// Here rather than next to each class: this file is linked as soon as a test calls registerTestComponents(), the
// others of the library only when the test uses them.
NEBULA_REGISTER_COMPONENT(TestComponents, Probe);
NEBULA_REGISTER_COMPONENT(TestComponents, Source);
NEBULA_REGISTER_COMPONENT(TestComponents, Sink);
NEBULA_REGISTER_COMPONENT(TestComponents, Panel);

void TestComponents::build(Probe& /*probe*/) {}

void TestComponents::build(Source& /*source*/) {}

void TestComponents::build(Sink& /*sink*/) {}

void TestComponents::build(Panel& /*panel*/) {}

Nebula::Plugin& registerTestComponents()
{
    // After the factory, which is created first: destroyed before it.
    static const std::unique_ptr<Nebula::Plugin> plugin = []
    {
        std::unique_ptr<Nebula::Plugin> created = Nebula::Plugin::instantiate<TestComponents>();
        Nebula::ComponentFactory::instance().add(TestComponents::Id, *created);
        return created;
    }();
    return *plugin;
}

QString testComponentId(const QString& name)
{
    return TestComponents::Id + u'.' + name;
}
