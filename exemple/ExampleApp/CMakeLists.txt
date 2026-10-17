# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# Lists the plugins of <application directory>/plugins and loads them.
cmu_add_executable(
    NAME ExampleApp
    NAMESPACE Nebula
    LINK_LIBRARIES PRIVATE Nebula::Core
)
