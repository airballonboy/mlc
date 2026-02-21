#include "type_system/type_info.h"
#include "tools/logger.h"

TypeInfo TypeInfo::get_from_id(size_t id) {
    for (auto &[_, type] : type_infos) {
        if (type.id == id) {
            return type;
        }
    }
    TODO("unknown typeid");
    return {};
}
