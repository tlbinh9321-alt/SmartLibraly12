#include "controllers/ReservationController.h"

#include <algorithm>

#include "controllers/ApiSupport.h"
#include "utils/StringUtils.h"

void ReservationController::registerRoutes(Router& router) {
    router.add("GET", "/api/reservations", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const std::string status = StringUtils::toUpper(req.get("status"));
            const std::string resourceText = req.get("resourceId"), memberText = req.get("memberId");
            const int resourceId = resourceText.empty() ? 0 : Api::parseId(resourceText, "resource");
            const int memberId = memberText.empty() ? 0 : Api::parseId(memberText, "member");
            Json list = Json::array();
            for (const Reservation& r : app_.reservations.getAll()) {
                if (!status.empty() && toString(r.getStatus()) != status) continue;
                if (resourceId && r.getResourceId() != resourceId) continue;
                if (memberId && r.getMemberId() != memberId) continue;
                list.push(mapper_.reservation(r));
            }
            return Api::ok("Reservations retrieved.", list);
        });
    });

    router.add("GET", "/api/reservations/queues", [this](const HttpRequest&) {
        return Api::guarded([&] { return Api::ok("Reservation queues retrieved.", mapper_.reservationQueues()); });
    });

    router.add("POST", "/api/reservations", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const Json body = Api::parseBody(req);
            const int memberId = Api::requiredInt(body, "memberId", "Invalid member ID.");
            const int resourceId = Api::requiredInt(body, "resourceId", "Invalid resource ID.");
            const Reservation r = app_.reservations.reserve(memberId, resourceId);
            const Json data = mapper_.reservation(r);
            const int position = static_cast<int>(data.find("position")->asNumber());
            return Api::ok("Reservation created. Queue position: " + std::to_string(position) + ".", data, 201);
        });
    });

    router.add("DELETE", "/api/reservations/{id}", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const Reservation r = app_.reservations.cancel(Api::parseId(req.param("id"), "reservation"));
            return Api::ok("Reservation cancelled.", mapper_.reservation(r));
        });
    });
}
