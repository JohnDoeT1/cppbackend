#include "collision_detector.h"
#include <algorithm>
#include <cassert>

namespace collision_detector {

CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
    // Проверим, что перемещение ненулевое.
    // Тут приходится использовать строгое равенство, а не приближённое,
    // поскольку при сборе заказов придётся учитывать перемещение даже на небольшое
    // расстояние.
    assert(b.x != a.x || b.y != a.y);
    const double u_x = c.x - a.x;
    const double u_y = c.y - a.y;
    const double v_x = b.x - a.x;
    const double v_y = b.y - a.y;
    const double u_dot_v = u_x * v_x + u_y * v_y;
    const double u_len2 = u_x * u_x + u_y * u_y;
    const double v_len2 = v_x * v_x + v_y * v_y;
    const double proj_ratio = u_dot_v / v_len2;
    const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

    return CollectionResult(sq_distance, proj_ratio);
}

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
    std::vector<GatheringEvent> detected_events;

    static const auto eq_pt = [](geom::Point2D p1, geom::Point2D p2) {
        return p1.x == p2.x && p1.y == p2.y;
    };

    for (size_t g = 0; g < provider.GatherersCount(); ++g) {
        const Gatherer gatherer = provider.GetGatherer(g);

        // Если gatherer не переместился, пропускаем его — по условию
        // «объект не совершил столкновений».
        if (eq_pt(gatherer.start_pos, gatherer.end_pos)) {
            continue;
        }

        for (size_t i = 0; i < provider.ItemsCount(); ++i) {
            const Item item = provider.GetItem(i);

            const auto result = TryCollectPoint(
                gatherer.start_pos, gatherer.end_pos, item.position);

            if (!result.IsCollected(gatherer.width + item.width)) {
                continue;
            }

            detected_events.push_back(GatheringEvent{
                .item_id = i,
                .gatherer_id = g,
                .sq_distance = result.sq_distance,
                .time = result.proj_ratio
            });
        }
    }

    std::sort(detected_events.begin(), detected_events.end(),
              [](const GatheringEvent& lhs, const GatheringEvent& rhs) {
                  return lhs.time < rhs.time;
              });

    return detected_events;
}

}  // namespace collision_detector
