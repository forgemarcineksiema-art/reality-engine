#include "lab/traversal_session.hpp"

#include <numbers>

namespace reality {
TraversalScene make_traversal_course() {
    TraversalScene scene;
    scene.geometry.bodies = {{1, {-6, 0, 0}, 0, 4, 6, .3, 0, 0, true},
                             {2, {6, 0, 0}, 0, 4, 6, .3, 0, 0, true},
                             {3, {0, 0, -2.8}, 0, 2, .65, .3, 0, 0, true},
                             {4, {-5, .75, 3.5}, 0, 2, 1.2, .25, .375, 0, true},
                             {5, {-1, 1.5, 3.5}, 0, 2, 1.2, .25, 0, 0, true},
                             {6, {7, 1.5, 3.5}, 0, 2, 1.2, .25, 0, 0, true},
                             {7, {2, 1.5, 3.5}, 0, 1, 1.1, .25, 0, 0, true},
                             {8, {-6.5, .01, -4}, 0, 1, .65, .1, 0, 0, true},
                             {9, {-5, 2, -.2}, .55, .25, 1.2, 2, 0, 0, false},
                             {10, {5, 1.2, -.4}, -.45, 1.1, .25, 1.2, 0, 0, false}};
    scene.platforms = {{{7, {2, 0, 0}, 0, 120, 80, 0}, 0, false},
                       {{8, {}, std::numbers::pi / 2, 120, 80, 1}, 0, false}};
    scene.connections = {{{1, {3.8, -2.8}}, {3, {-1.8, 0}}, SurfaceLegKind::Step, 1},
                         {{3, {1.8, 0}}, {2, {-3.8, -2.8}}, SurfaceLegKind::Step, 1},
                         {{1, {-1.2, 3.5}}, {4, {-1.8, 0}}, SurfaceLegKind::Step},
                         {{4, {1.8, 0}}, {5, {-1.8, 0}}, SurfaceLegKind::Step},
                         {{1, {-1.7, -4}}, {8, {-.7, 0}}, SurfaceLegKind::Step},
                         {{5, {1.8, 0}}, {6, {-1.8, 0}}, SurfaceLegKind::Transport, 0, 1, 12}};
    scene.services = {{1, 7, {{{5, {1.8, 0}}, {6, {-1.8, 0}}}}, {{{-.8, 0}, {.8, 0}}}, {}, {}}};
    scene.passages = {{1, 3, {}, {}}};
    scene.stations = {{{1, {-2, 1.3}}, TraversalTask::Eat, 80},
                      {{2, {2, 1.5}}, TraversalTask::Eat, 80},
                      {{1, {-2, -1.7}}, TraversalTask::Rest, 1},
                      {{6, {.5, 0}}, TraversalTask::Maintain, 80}};
    scene.spawns = {{{1, {-2, 0}}, {}, false, 0},         {{1, {-2, 2.5}}, {}, true, 45},
                    {{1, {0, -2}}, {.25, 1.2}, true, 90}, {{1, {1, 1.2}}, {}, true, 135},
                    {{2, {2, 0}}, {}, true, 180},         {{2, {2, 2.5}}, {.25, 1.2}, true, 225},
                    {{2, {0, -2.8}}, {}, true, 270},      {{2, {1, -1.8}}, {}, true, 315}};
    return scene;
}
TraversalScene make_delivery_course() {
    auto scene = make_traversal_course();
    for (auto& station : scene.stations) {
        if (station.kind == TraversalTask::Eat)
            station.initial_material = {1};
        if (station.kind == TraversalTask::Maintain)
            station.initial_material = {2};
    }
    scene.stations[0].initial_stock = 20;
    scene.stations[1].initial_stock = 0;
    scene.spawns[1].hunger = 0;
    scene.initial_deliveries = {{1, 0, 1, 12}};
    return scene;
}
} // namespace reality
