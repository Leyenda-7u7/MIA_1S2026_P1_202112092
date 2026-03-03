#include "fs/AuthGuard.hpp"
#include "fs/Session.hpp"

namespace fs_guard {
bool requireSession(std::string& outMsg) {
    if (!session::isActive()) {
        outMsg = "Error: no existe una sesión activa.";
        return false;
    }
    return true;
}
}