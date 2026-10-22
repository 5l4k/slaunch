#include <sl/menu/widgets/Widgets.hpp>
#include <sl/menu/cfg/UserCfg.hpp>
#include <sl/menu/net/Http.hpp>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>

namespace sl::menu::widgets {

    namespace {
        constexpr int  kWidgetW = 340;
        // Where the floating widgets sit and which are on is a per-account
        // choice; the scripts themselves are shared in sdmc:/slaunch/widgets.
        inline std::string PosPath() { return cfg::Path("widget_pos.txt"); }
        inline std::string EnPath()  { return cfg::Path("widget_enabled.txt"); }
    }

    void Widgets::Init() {
        net::GlobalInit();
        LoadWidgets();          // constructs LuaWidgets (runs each script) on this thread
        LoadPositions();        // default layout, then any saved positions
        LoadEnabled();          // menu-owned on/off state (default off)
        FitOnScreen();
        m_run = true;
        if (R_SUCCEEDED(threadCreate(&m_thread, &Widgets::ThreadTrampoline, this,
                                     nullptr, 0x8000, 0x3B, -2))) {
            threadStart(&m_thread);
            m_started = true;
        }
    }

    void Widgets::Exit() {
        m_run = false;
        if (m_started) {
            threadWaitForExit(&m_thread);
            threadClose(&m_thread);
            m_started = false;
        }
        m_widgets.clear();      // frees LuaWidget textures/sockets (renderer still alive)
        net::GlobalExit();
    }

    void Widgets::ThreadTrampoline(void *arg) { static_cast<Widgets *>(arg)->ThreadLoop(); }

    void Widgets::ThreadLoop() {
        while (m_run) {
            // Ticked if anything is showing it. Enabled is the user's choice for
            // the floating home-screen copy; tiled means a wall tile is drawing
            // it, which is just as good a reason to keep its data fresh - and it
            // must NOT imply the other, or putting a widget on the wall would
            // switch it on across every layout.
            for (size_t i = 0; i < m_widgets.size(); i++)
                if (IsEnabled((int)i) || IsTiled((int)i)) m_widgets[i]->Update();
            svcSleepThread(166'000'000ULL); // ~6 Hz
        }
    }

    void Widgets::LoadWidgets() {
        m_widgets.clear();
        DIR *d = opendir("sdmc:/slaunch/widgets");
        if (!d) return;
        struct dirent *e;
        while ((e = readdir(d)) != nullptr) {
            const char *name = e->d_name;
            size_t len = strlen(name);
            if (len > 4 && strcasecmp(name + len - 4, ".lua") == 0) {
                std::string path = std::string("sdmc:/slaunch/widgets/") + name;
                m_widgets.push_back(std::make_unique<LuaWidget>(path));
            }
        }
        closedir(d);
    }

    void Widgets::Render(gfx::Gfx *gfx, const ui::Theme &t, bool skip_tiled) {
        for (size_t i = 0; i < m_widgets.size(); i++) {
            Box &b = m_box[i];
            if (!IsEnabled((int)i)) { b.h = 0; continue; }   // off = not drawn / not grabbable
            // Owned by a tile this frame: h = 0 also takes it out of the
            // drag hit-test, which is right - you move it by moving the tile.
            if (skip_tiled && IsTiled((int)i)) { b.h = 0; continue; }
            int ynew = m_widgets[i]->Render(gfx, t, b.x, b.y, b.w);
            b.h = std::max(0, ynew - b.y);   // measured for hit-testing
        }
    }

    // ---- positions ---------------------------------------------------------
    int Widgets::HitTest(int px, int py) const {
        // Topmost first: later widgets draw over earlier ones at the same spot.
        for (int i = (int)m_box.size() - 1; i >= 0; i--) {
            const Box &b = m_box[i];
            if (b.h > 0 && px >= b.x && px < b.x + b.w && py >= b.y && py < b.y + b.h)
                return i;
        }
        return -1;
    }

    bool Widgets::GetBox(int i, int &x, int &y, int &w, int &h) const {
        if (i < 0 || i >= (int)m_box.size()) return false;
        x = m_box[i].x; y = m_box[i].y; w = m_box[i].w; h = m_box[i].h;
        return true;
    }

    void Widgets::MoveBy(int i, int dx, int dy) {
        if (i < 0 || i >= (int)m_box.size()) return;
        Box &b = m_box[i];
        b.x = std::min(std::max(0, b.x + dx), gfx::Gfx::Width  - b.w);
        b.y = std::min(std::max(0, b.y + dy), gfx::Gfx::Height - 40);
    }

    void Widgets::LoadPositions() {
        // default first-run layout: a right-hand column (draggable afterwards)
        m_box.assign(m_widgets.size(), Box{});
        for (size_t i = 0; i < m_widgets.size(); i++)
            m_box[i] = Box{ gfx::Gfx::Width - kWidgetW - 28, 96 + (int)i * 190, kWidgetW, 0 };

        FILE *fp = fopen(PosPath().c_str(), "r");
        if (!fp) return;
        char line[192];
        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\r\n")] = '\0';
            char *eq = strchr(line, '=');
            if (!eq) continue;
            *eq = '\0';
            const char *name = line;
            int x = 0, y = 0;
            if (sscanf(eq + 1, "%d,%d", &x, &y) != 2) continue;
            // Match to a widget by its display name.
            for (size_t i = 0; i < m_widgets.size(); i++) {
                if (m_widgets[i]->Name() == name) {
                    m_box[i].x = std::min(std::max(0, x), gfx::Gfx::Width  - m_box[i].w);
                    m_box[i].y = std::min(std::max(0, y), gfx::Gfx::Height - 40);
                    break;
                }
            }
        }
        fclose(fp);
    }

    void Widgets::SavePositions() {
        mkdir("sdmc:/slaunch", 0777);
        mkdir("sdmc:/slaunch/config", 0777);
        FILE *fp = fopen(PosPath().c_str(), "w");
        if (!fp) return;
        for (size_t i = 0; i < m_widgets.size(); i++)
            fprintf(fp, "%s=%d,%d\n", m_widgets[i]->Name().c_str(), m_box[i].x, m_box[i].y);
        fclose(fp);
    }

    // ---- enable flags ------------------------------------------------------
    void Widgets::SetEnabled(int i, bool on) {
        if (i < 0 || i >= (int)m_enabled.size()) return;
        m_enabled[i] = on ? 1 : 0;
        SaveEnabled();
        if (on) FitOnScreen();
    }

    // The first-run layout stacks every installed widget down one column,
    // enabled or not, so from the fifth one on the default spot is below the
    // screen - and a saved spot is only clamped to the bottom 40px, under the
    // button hints. Either way the widget was on but invisible. Any enabled
    // widget without a real view moves to the first free slot, scanning the
    // right-hand column bottom to top and then columns further left.
    void Widgets::FitOnScreen() {
        constexpr int kTop = 96, kPitch = 190, kGuessH = 170, kLowest = 160;
        const int W = gfx::Gfx::Width, H = gfx::Gfx::Height;
        auto overlaps = [&](int skip, int x, int y) {
            for (size_t j = 0; j < m_box.size(); j++) {
                if ((int)j == skip || j >= m_enabled.size() || !m_enabled[j]) continue;
                const Box &o = m_box[j];
                const int oh = o.h > 0 ? o.h : kGuessH;
                if (x < o.x + o.w && o.x < x + kWidgetW && y < o.y + oh && o.y < y + kGuessH)
                    return true;
            }
            return false;
        };
        bool moved = false;
        for (size_t i = 0; i < m_box.size() && i < m_enabled.size(); i++) {
            if (!m_enabled[i] || m_box[i].y <= H - kLowest) continue;
            for (int c = 0, done = 0; !done && W - (kWidgetW + 28) * (c + 1) >= 0; c++) {
                const int x = W - kWidgetW - 28 - c * (kWidgetW + 28);
                // Bottom up: the top of the screen is where most layouts put
                // their main row (the Shelf's covers, XMB's bar).
                for (int y = H - kGuessH - 50; y >= kTop; y -= kPitch)
                    if (!overlaps((int)i, x, y)) { m_box[i].x = x; m_box[i].y = y; done = 1; moved = true; break; }
            }
        }
        if (moved) SavePositions();
    }

    void Widgets::LoadEnabled() {
        // Default off: a fresh install (and any newly-added widget) starts
        // disabled, so nothing shows on the home screen until the user turns it on
        // in Theming > Widgets. Saved state in widget_enabled.txt overrides this.
        m_enabled.assign(m_widgets.size(), 0);   // default: off
        m_tiled.assign(m_widgets.size(), 0);
        FILE *fp = fopen(EnPath().c_str(), "r");
        if (!fp) return;
        char line[128];
        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\r\n")] = '\0';
            char *eq = strchr(line, '=');
            if (!eq) continue;
            *eq = '\0';
            for (size_t i = 0; i < m_widgets.size(); i++)
                if (m_widgets[i]->Name() == line) { m_enabled[i] = atoi(eq + 1) ? 1 : 0; break; }
        }
        fclose(fp);
    }

    void Widgets::SaveEnabled() {
        mkdir("sdmc:/slaunch", 0777);
        mkdir("sdmc:/slaunch/config", 0777);
        FILE *fp = fopen(EnPath().c_str(), "w");
        if (!fp) return;
        for (size_t i = 0; i < m_widgets.size(); i++)
            fprintf(fp, "%s=%d\n", m_widgets[i]->Name().c_str(), m_enabled[i] ? 1 : 0);
        fclose(fp);
    }

} // namespace sl::menu::widgets
