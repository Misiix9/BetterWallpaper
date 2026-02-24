#pragma once
#include "TransitionEffect.hpp"
#include <memory>
#include <string>
#include <vector>
namespace bwp::transition {
std::shared_ptr<TransitionEffect> createEffectByName(const std::string &name);
std::vector<std::string> getAvailableEffectNames();
} 
