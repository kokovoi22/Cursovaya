#pragma once
#include "imgui/imgui.h"
#include <d3d11.h>


struct VisualsTeam {
  bool glow = false;
  ImVec4 glow_color = ImVec4(0.f, 1.f, 0.f, 1.f);
};

struct VisualsEnemy {
  bool glow = false;
  ImVec4 glow_color = ImVec4(1.f, 0.f, 0.f, 1.f);
};

struct VisualsLocal {
  bool fov_changer = false;
  int fov = 90;
};

struct Config {
  VisualsTeam team;
  VisualsEnemy enemy;
  VisualsLocal local;
  bool bhop = false;
  bool anti_flash = false;
};

extern Config g_config;