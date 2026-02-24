#include "EffectFactory.hpp"
#include "effects/AdvancedEffects.hpp"
#include "effects/BasicEffects.hpp"
#include <algorithm>
#include <array>
std::shared_ptr<bwp::transition::TransitionEffect>
bwp::transition::createEffectByName(const std::string &name) {
  if (name == "Fade") {
    return std::make_shared<FadeEffect>();
  }
  if (name == "Slide") {
    return std::make_shared<SlideEffect>();
  }
  if (name == "Wipe") {
    return std::make_shared<WipeEffect>();
  }
  if (name == "Expanding Circle") {
    return std::make_shared<ExpandingCircleEffect>();
  }
  if (name == "Expanding Square") {
    return std::make_shared<ExpandingSquareEffect>();
  }
  if (name == "Dissolve") {
    return std::make_shared<DissolveEffect>();
  }
  if (name == "Zoom") {
    return std::make_shared<ZoomEffect>();
  }
  if (name == "Morph") {
    return std::make_shared<MorphEffect>();
  }
  if (name == "Angled Wipe") {
    auto effect = std::make_shared<AngledWipeEffect>();
    effect->setAngle(45);
    effect->setSoftEdge(true);
    return effect;
  }
  if (name == "Pixelate") {
    return std::make_shared<PixelateEffect>();
  }
  if (name == "Blinds") {
    return std::make_shared<BlindsEffect>();
  }
  return std::make_shared<FadeEffect>();
}
std::vector<std::string> bwp::transition::getAvailableEffectNames() {
  static const std::array<const char *, 11> names = {
      "Fade", "Slide", "Wipe", "Expanding Circle", "Expanding Square",
      "Dissolve", "Zoom", "Morph", "Angled Wipe", "Pixelate", "Blinds"};
  return std::vector<std::string>(names.begin(), names.end());
}
