#include "Application.hpp"
#include <iostream>

int main(int argc, char **argv) {
  auto *app = bwp::gui::Application::create();
  return app->run(argc, argv);
}
