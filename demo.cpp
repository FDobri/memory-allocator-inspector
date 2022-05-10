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
    if (ImGui::BeginTable("history", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, {0, 290})) {
        for (auto label : {"ID", "Action", "Offset", "Bytes", "Time (ms)", "Lifetime (ms)"})
            ImGui::TableSetupColumn(label);
        ImGui::TableHeadersRow();
        for (auto it = events.rbegin(); it != events.rend(); ++it) {
            const auto ms = std::chrono::duration<double, std::milli>(it->time - journal.start()).count();
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%llu", static_cast<unsigned long long>(it->id));
            ImGui::TableNextColumn(); ImGui::TextUnformatted(it->allocated ? "Allocate" : "Release");
            ImGui::TableNextColumn(); ImGui::Text("%zu", it->offset);
            ImGui::TableNextColumn(); ImGui::Text("%zu", it->bytes);
            ImGui::TableNextColumn(); ImGui::Text("%.1f", ms);
            ImGui::TableNextColumn();
            if (!it->allocated) {
                const auto origin = std::find_if(events.begin(), events.end(), [&](const auto& e) {
                    return e.id == it->id && e.allocated;
                });
                if (origin != events.end())
                    ImGui::Text("%.1f", std::chrono::duration<double, std::milli>(it->time - origin->time).count());
            }
        }
        ImGui::EndTable();
    }
}
