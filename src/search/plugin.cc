#include "plugin.h"

#include <typeindex>
#include <typeinfo>

using namespace std;


void register_plugin_type_plugin(
    const type_info &type, const string &type_name, const string &document) {
    PluginTypeInfo info(type_index(type), type_name, document);
    PluginTypeRegistry::instance()->insert(info);
}

