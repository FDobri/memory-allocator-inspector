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

int main(int argc, char** argv) {
    const bool smoke = argc == 2 && std::strcmp(argv[1], "--smoke") == 0;
    memory::Arena arena(8192, 256);
    memory::Pool pool(64, 128, 64, 256);
    std::vector<memory::Block> blocks;
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    if (!SDL_Init(SDL_INIT_VIDEO) ||
        !SDL_CreateWindowAndRenderer("Memory inspector", 980, 620, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        std::fprintf(stderr, "%s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
    int bytes = 32, alignment = 3;
    bool select_pool = false;
    const char* feedback = "";
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) running = false;
        }
        if (!running) break;
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Allocations", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
        ImGui::Checkbox("Inspect fixed-size pool", &select_pool);
        try {
            if (select_pool) {
                ImGui::Text("Pool usage %zu / 8192 bytes", pool.used());
                if (ImGui::Button("Allocate slot")) { blocks.push_back(pool.allocate()); feedback = ""; }
                ImGui::SameLine();
                if (ImGui::Button("Release oldest slot") && !blocks.empty()) {
                    pool.release(blocks.front()); blocks.erase(blocks.begin()); feedback = "";
                }
            } else {
                ImGui::Text("Arena usage %zu / 8192 bytes", arena.used());
                ImGui::SliderInt("Request bytes", &bytes, 1, 512);
                ImGui::SliderInt("Alignment log2", &alignment, 0, 8);
                if (ImGui::Button("Allocate block")) { arena.allocate(bytes, 1u << alignment); feedback = ""; }
                ImGui::SameLine();
                if (ImGui::Button("Reset arena")) { arena.reset(); feedback = ""; }
            }
        } catch (const std::bad_alloc&) { feedback = "Allocation exceeds remaining capacity."; }
        ImGui::TextUnformatted(feedback);
        draw_history(select_pool ? pool.journal() : arena.journal());
        ImGui::End();
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 21, 24, 29, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
        if (smoke) running = false;
    }
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
