#include "SohMenu.h"
#include <ship/window/gui/GuiMenuBar.h>
#include <ship/window/gui/GuiElement.h>
#include <ship/utils/StringHelper.h>
#include <spdlog/fmt/fmt.h>

#ifdef __ANDROID__
#include <atomic>
#include <jni.h>

namespace {
std::atomic<bool> sAndroidMenuVisible{ false };
std::atomic<float> sAndroidMenuTouchX{ 0.0f };
std::atomic<float> sAndroidMenuTouchY{ 0.0f };
std::atomic<bool> sAndroidMenuTouchMoved{ false };
std::atomic<bool> sAndroidMenuTouchPressed{ false };
std::atomic<bool> sAndroidMenuTouchReleased{ false };

void InjectAndroidMenuTouch() {
    const bool moved = sAndroidMenuTouchMoved.exchange(false, std::memory_order_relaxed);
    const bool pressed = sAndroidMenuTouchPressed.exchange(false, std::memory_order_relaxed);
    const bool released = sAndroidMenuTouchReleased.exchange(false, std::memory_order_relaxed);
    if (!moved && !pressed && !released) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(sAndroidMenuTouchX.load(std::memory_order_relaxed),
                        sAndroidMenuTouchY.load(std::memory_order_relaxed));
    if (pressed) {
        io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
    }
    if (released) {
        io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    }
}
} // namespace

extern "C" JNIEXPORT jboolean JNICALL
Java_com_dishii_soh_MainActivity_nativeDispatchMenuTouch(JNIEnv* env, jobject obj, jfloat x, jfloat y, jint action) {
    if (!sAndroidMenuVisible.load(std::memory_order_relaxed)) {
        return JNI_FALSE;
    }

    sAndroidMenuTouchX.store(x, std::memory_order_relaxed);
    sAndroidMenuTouchY.store(y, std::memory_order_relaxed);
    sAndroidMenuTouchMoved.store(true, std::memory_order_relaxed);

    // MotionEvent: ACTION_DOWN=0, ACTION_UP=1, ACTION_MOVE=2, ACTION_CANCEL=3.
    if (action == 0) {
        sAndroidMenuTouchPressed.store(true, std::memory_order_relaxed);
    } else if (action == 1 || action == 3) {
        sAndroidMenuTouchReleased.store(true, std::memory_order_relaxed);
    }

    return JNI_TRUE;
}
#endif

extern "C" {
extern PlayState* gPlayState;
}

extern std::unordered_map<s16, const char*> warpPointSceneList;

namespace SohGui {
extern std::shared_ptr<SohMenu> mSohMenu;

using namespace UIWidgets;

void SohMenu::AddSidebarEntry(std::string sectionName, std::string sidebarName, uint32_t columnCount) {
    assert(!sectionName.empty());
    assert(!sidebarName.empty());
    menuEntries.at(sectionName).sidebars.emplace(sidebarName, SidebarEntry{ .columnCount = columnCount });
    menuEntries.at(sectionName).sidebarOrder.push_back(sidebarName);
}

WidgetInfo& SohMenu::AddWidget(WidgetPath& pathInfo, std::string widgetName, WidgetType widgetType) {
    assert(!widgetName.empty());                        // Must be unique
    assert(menuEntries.contains(pathInfo.sectionName)); // Section/header must already exist
    assert(menuEntries.at(pathInfo.sectionName).sidebars.contains(pathInfo.sidebarName)); // Sidebar must already exist
    std::unordered_map<std::string, SidebarEntry>& sidebar = menuEntries.at(pathInfo.sectionName).sidebars;
    uint8_t column = pathInfo.column;
    if (sidebar.contains(pathInfo.sidebarName)) {
        while (sidebar.at(pathInfo.sidebarName).columnWidgets.size() < column + 1) {
            sidebar.at(pathInfo.sidebarName).columnWidgets.push_back({});
        }
    }
    SidebarEntry& entry = sidebar.at(pathInfo.sidebarName);
    entry.columnWidgets.at(column).push_back({ .name = widgetName, .type = widgetType });
    WidgetInfo& widget = entry.columnWidgets.at(column).back();
    switch (widgetType) {
        case WIDGET_CHECKBOX:
        case WIDGET_CVAR_CHECKBOX:
            widget.options = std::make_shared<CheckboxOptions>();
            break;
        case WIDGET_SLIDER_FLOAT:
        case WIDGET_CVAR_SLIDER_FLOAT:
            widget.options = std::make_shared<FloatSliderOptions>();
            break;
        case WIDGET_CVAR_BTN_SELECTOR:
            widget.options = std::make_shared<BtnSelectorOptions>();
            break;
        case WIDGET_SLIDER_INT:
        case WIDGET_CVAR_SLIDER_INT:
            widget.options = std::make_shared<IntSliderOptions>();
            break;
        case WIDGET_COMBOBOX:
        case WIDGET_CVAR_COMBOBOX:
        case WIDGET_AUDIO_BACKEND:
        case WIDGET_VIDEO_BACKEND:
            widget.options = std::make_shared<ComboboxOptions>();
            break;
        case WIDGET_BUTTON:
            widget.options = std::make_shared<ButtonOptions>();
            break;
        case WIDGET_WINDOW_BUTTON:
            widget.options = std::make_shared<WindowButtonOptions>();
            break;
        case WIDGET_CVAR_COLOR_PICKER:
        case WIDGET_COLOR_PICKER:
            widget.options = std::make_shared<ColorPickerOptions>();
            break;
        case WIDGET_SEPARATOR_TEXT:
        case WIDGET_TEXT:
            widget.options = std::make_shared<TextOptions>();
            break;
        case WIDGET_SEARCH:
        case WIDGET_SEPARATOR:
        default:
            widget.options = std::make_shared<WidgetOptions>();
    }
    return widget;
}

SohMenu::SohMenu(const std::string& consoleVariable, const std::string& name)
    : Menu(consoleVariable, name, 0, UIWidgets::Colors::LightBlue) {
}

void SohMenu::AddMenuElements() {
    AddMenuSettings();
    AddMenuEnhancements();
    AddMenuRandomizer();
    AddMenuNetwork();
    AddMenuDevTools();

    if (CVarGetInteger(CVAR_SETTING("Menu.SidebarSearch"), 0)) {
        InsertSidebarSearch();
    }

    for (auto& initFunc : MenuInit::GetInitFuncs()) {
        initFunc();
    }

    mMenuElementsInitialized = true;
}

void SohMenu::InitElement() {
    Ship::Menu::InitElement();

    disabledMap = {
        { DISABLE_FOR_NO_VSYNC,
          { [](disabledInfo& info) -> bool {
               return !Ship::Context::GetInstance()->GetWindow()->CanDisableVerticalSync();
           },
            "Disabling VSync not supported" } },
        { DISABLE_FOR_NO_WINDOWED_FULLSCREEN,
          { [](disabledInfo& info) -> bool {
               return !Ship::Context::GetInstance()->GetWindow()->SupportsWindowedFullscreen();
           },
            "Windowed Fullscreen not supported" } },
        { DISABLE_FOR_NO_MULTI_VIEWPORT,
          { [](disabledInfo& info) -> bool {
               return !Ship::Context::GetInstance()->GetWindow()->GetGui()->SupportsViewports();
           },
            "Multi-viewports not supported" } },
        { DISABLE_FOR_NOT_DIRECTX,
          { [](disabledInfo& info) -> bool {
               return Ship::Context::GetInstance()->GetWindow()->GetWindowBackend() !=
                      Ship::WindowBackend::FAST3D_DXGI_DX11;
           },
            "Available Only on DirectX" } },
        { DISABLE_FOR_DIRECTX,
          { [](disabledInfo& info) -> bool {
               return Ship::Context::GetInstance()->GetWindow()->GetWindowBackend() ==
                      Ship::WindowBackend::FAST3D_DXGI_DX11;
           },
            "Not Available on DirectX" } },
        { DISABLE_FOR_MATCH_REFRESH_RATE_ON,
          { [](disabledInfo& info) -> bool { return CVarGetInteger(CVAR_SETTING("MatchRefreshRate"), 0); },
            "Match Refresh Rate is Enabled" } },
        { DISABLE_FOR_ADVANCED_RESOLUTION_ON,
          { [](disabledInfo& info) -> bool { return CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".Enabled", 0); },
            "Advanced Resolution Enabled" } },
        { DISABLE_FOR_VERTICAL_RES_TOGGLE_ON,
          { [](disabledInfo& info) -> bool {
               return CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".VerticalResolutionToggle", 0);
           },
            "Vertical Resolution Toggle Enabled" } },
        { DISABLE_FOR_LOW_RES_MODE_ON,
          { [](disabledInfo& info) -> bool { return CVarGetInteger(CVAR_LOW_RES_MODE, 0); }, "N64 Mode Enabled" } },
        { DISABLE_FOR_NULL_PLAY_STATE,
          { [](disabledInfo& info) -> bool { return gPlayState == NULL; }, "Save Not Loaded" } },
        { DISABLE_FOR_DEBUG_MODE_OFF,
          { [](disabledInfo& info) -> bool { return !CVarGetInteger(CVAR_DEVELOPER_TOOLS("DebugEnabled"), 0); },
            "Debug Mode is Disabled" } },
        { DISABLE_FOR_FRAME_ADVANCE_OFF,
          { [](disabledInfo& info) -> bool { return !(gPlayState != nullptr && gPlayState->frameAdvCtx.enabled); },
            "Frame Advance is Disabled" } },
        { DISABLE_FOR_ADVANCED_RESOLUTION_OFF,
          { [](disabledInfo& info) -> bool { return !CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".Enabled", 0); },
            "Advanced Resolution is Disabled" } },
        { DISABLE_FOR_VERTICAL_RESOLUTION_OFF,
          { [](disabledInfo& info) -> bool {
               return !CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".VerticalResolutionToggle", 0);
           },
            "Vertical Resolution Toggle is Off" } },
    };
}

void SohMenu::UpdateElement() {
    Ship::Menu::UpdateElement();
}

void SohMenu::Draw() {
#ifdef __ANDROID__
    const bool visible = IsVisible();
    sAndroidMenuVisible.store(visible, std::memory_order_relaxed);
    if (visible) {
        InjectAndroidMenuTouch();
    } else {
        sAndroidMenuTouchMoved.store(false, std::memory_order_relaxed);
        sAndroidMenuTouchPressed.store(false, std::memory_order_relaxed);
        sAndroidMenuTouchReleased.store(false, std::memory_order_relaxed);
    }
#endif
    Ship::Menu::Draw();
}

void SohMenu::DrawElement() {
    if (mMenuElementsInitialized) {
        Ship::Menu::DrawElement();
    }
}
} // namespace SohGui
