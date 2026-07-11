cstoolkit_add_target(Core SHARED NAMESPACE AppForge RECURSIVE
    PUBLIC_LINK_LIBRARIES
      Qt6::Core
    PRIVATE_LINK_LIBRARIES
      nlohmann_json::nlohmann_json
)
