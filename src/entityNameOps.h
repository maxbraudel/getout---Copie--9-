#pragma once

#include <string>

// Forward declaration from entities.h
enum class EntityName;

// Utility functions for EntityName enum
std::string entityNameToString(EntityName entityName);
EntityName stringToEntityName(const std::string& str);