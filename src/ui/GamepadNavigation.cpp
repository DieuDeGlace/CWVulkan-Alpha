#include "ui/Screens.h"

namespace ui {

bool Interface::controllerBack() {
    if (controllerKeyboardOpen()) {
        finishControllerKeyboard(false);
    } else if (chatOpen) {
        chatOpen = false;
        chatInput.clear();
    } else if (optionsOpen) {
        optionsOpen = false;
    } else if (front != Front::None) {
        switch (front) {
        case Front::Creation: front = Front::Characters; break;
        case Front::WorldCreation: case Front::Connect: front = Front::Worlds; break;
        case Front::Worlds: front = Front::Characters; break;
        case Front::Characters: front = Front::None; startScreen = true; break;
        default: break;
        }
        deleteShown = false;
        frontRequest = FrontRequest::None;
        leftHeld = false;
    } else if (mapOpen) {
        mapOpen = teleportMode = false;
        selectedPortal = {-1, -1};
    } else if (inventoryOpen || craftingOpen || skillsOpen || vendorOpen || identifyOpen || adaptOpen || benchOpen) {
        inventoryOpen = craftingOpen = skillsOpen = vendorOpen = identifyOpen = adaptOpen = benchOpen = false;
        menuRow = false;
        benchReset(); // a held preview remains on its weapon; no item is discarded
    } else if (systemMenu) {
        systemMenu = false;
    } else if (menuRow) {
        menuRow = false;
    } else {
        return false;
    }
    return true;
}

} // namespace ui
