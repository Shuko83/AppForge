# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The classes of Core, each tested through its public API.
nebula_add_test(TestComponentFactory LINK_LIBRARIES TestComponents)
nebula_add_test(TestComponent LINK_LIBRARIES TestComponents)
nebula_add_test(TestPluginManager LINK_LIBRARIES Nebula::Core)
nebula_add_test(TestUniverse LINK_LIBRARIES TestComponents)
nebula_add_test(TestSettingsFile LINK_LIBRARIES TestComponents)
