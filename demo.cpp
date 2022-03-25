#include "memory.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <cstdio>
#include <cstring>

static void draw_history(const memory::Journal& journal) {
    auto events = journal.events();
    std::vector<float> usage;
    for (const auto& event : events) usage.push_back(static_cast<float>(event.used));
    if (!usage.empty())
        ImGui::PlotLines("Resident bytes", usage.data(), static_cast<int>(usage.size()),
                         0, nullptr, 0, FLT_MAX, {0, 70});
}
