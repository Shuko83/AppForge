# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# Its root object and its metadata are generated.
# Its class (NEBULA_PLUGIN) and its components, each registered in it with NEBULA_REGISTER_COMPONENT, are in src/.
nebula_add_plugin(
    NAME ExamplePlugin
    DESCRIPTION "Example plugin: its metadata is read without loading it"
    VERSION 1.0.0
)
