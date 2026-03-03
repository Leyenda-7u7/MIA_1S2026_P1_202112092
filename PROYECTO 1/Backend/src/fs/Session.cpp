#include "fs/Session.hpp"

namespace session {
static SessionInfo g;

bool isActive() { return g.active; }
SessionInfo get() { return g; }

void start(const SessionInfo& s) {
    g = s;
    g.active = true;
}

void end() {
    g = SessionInfo{};
    g.active = false;
}
}