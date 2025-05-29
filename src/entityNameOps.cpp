#include "entityNameOps.h"
#include <stdexcept>

std::string entityNameToString(EntityName entityName) {
    switch (entityName) {
        case EntityName::ANTAGONIST:
            return "antagonist";
        case EntityName::PLAYER:
            return "player";
        default:
            throw std::invalid_argument("Unknown EntityName");
    }
}

EntityName stringToEntityName(const std::string& str) {
    if (str == "antagonist") {
        return EntityName::ANTAGONIST;
    } else if (str == "player") {
        return EntityName::PLAYER;
    } else {
        throw std::invalid_argument("Unknown entity type string: " + str);
    }
}
