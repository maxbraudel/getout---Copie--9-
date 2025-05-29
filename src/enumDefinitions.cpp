#include "entities.h" // Include for full definition of EntityName
#include "elementsOnMap.h" // Include for ElementName
#include <magic_enum.hpp>
#include <algorithm>
#include <cctype>

std::string entityNameToString(EntityName entityName) {
    // Use magic_enum to convert enum to string automatically
    return std::string(magic_enum::enum_name(entityName));
}

std::string elementNameToString(ElementName elementName) {
    // Use magic_enum to convert enum to string automatically
    return std::string(magic_enum::enum_name(elementName));
}

std::string blockNameToString(BlockName blockName) {
    // Use magic_enum to convert enum to string automatically
    return std::string(magic_enum::enum_name(blockName));
}