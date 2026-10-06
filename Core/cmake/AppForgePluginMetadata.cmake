# Run at build time by appforge_add_plugin (cmake -P): writes the metadata read by moc, dated now.
#   -DINPUT=<metadata configured by appforge_add_plugin> -DOUTPUT=<file named in Q_PLUGIN_METADATA>
string(TIMESTAMP _build_date "%Y-%m-%dT%H:%M:%SZ" UTC)
file(READ "${INPUT}" _metadata)
string(REPLACE "@APPFORGE_PLUGIN_BUILD_DATE@" "${_build_date}" _metadata "${_metadata}")
file(WRITE "${OUTPUT}" "${_metadata}")
