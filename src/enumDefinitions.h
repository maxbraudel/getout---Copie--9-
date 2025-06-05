#pragma once

#include <string>
#include <magic_enum.hpp>

// Forward declaration from entities.h
enum class EntityName {
    ANTAGONIST2,
    ANTAGONIST,
    PLAYER,
    SHARK,
    GIRAFFE,
};


// Define an enum for element texture types
enum class ElementName {
    COCONUT_TREE_1,
    COCONUT_TREE_2,
    COCONUT_TREE_3,
    CHARACTER1,
    TEST,
    ANTAGONIST1,
    SHARK,
    COCONUT,
    GIRAFFE
    // Add more element texture types as needed
};

enum class BlockName {
    GRASS_0,
    GRASS_1,
    GRASS_2,
    GRASS_3,
    GRASS_4,
    GRASS_5,
    SAND,
    WATER_0,
    WATER_1,
    WATER_2,
    WATER_3,
    WATER_4,
    ICE_1,
    ICE_2,
    ICE_3
};

// Utility functions for EntityName enum using magic_enum
std::string entityNameToString(EntityName entityName);
std::string elementNameToString(ElementName elementName);
std::string blockNameToString(BlockName blockName);
