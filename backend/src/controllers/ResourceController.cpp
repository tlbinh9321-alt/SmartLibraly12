#include "controllers/ResourceController.h"

#include <algorithm>
#include <set>

#include "controllers/ApiSupport.h"
#include "utils/StringUtils.h"

void ResourceController::registerRoutes(Router& router) {
    router.add("GET", "/api/resources", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            SearchCriteria criteria;
            criteria.query = req.get("q");
            criteria.type = req.get("type");
            criteria.genre = req.get("genre");
            const std::string availability = StringUtils::toLower(req.get("availability"));
            if (availability == "available") criteria.availableOnly = true;
            else if (availability == "unavailable") criteria.availableOnly = false;
            Json list = Json::array();
            for (const LibraryResource* r : app_.search.filter(criteria)) list.push(mapper_.resource(*r));
            return Api::ok("Resources retrieved.", list);
        });
    });

    router.add("GET", "/api/genres", [this](const HttpRequest&) {
        return Api::guarded([&] {
            std::set<std::string> genres;
            for (const auto& r : app_.library.resources()) genres.insert(r->getGenre());
            Json list = Json::array();
            for (const std::string& g : genres) list.push(g);
            return Api::ok("Genres retrieved.", list);
        });
    });

    router.add("GET", "/api/resources/{id}", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const int id = Api::parseId(req.param("id"), "resource");
            const LibraryResource& resource = app_.resources.getById(id);
            std::vector<Loan> history;
            for (const Loan& l : app_.loans.getAllLoans())
                if (l.getResourceId() == id) history.push_back(l);
            Json detail = mapper_.resource(resource);
            detail.set("borrowingHistory", mapper_.loanList(history));
            detail.set("reservationQueue", mapper_.queueOf(id));
            Json ready = Json::array();
            for (const Reservation& r : app_.reservations.getAll())
                if (r.getResourceId() == id && r.getStatus() == ReservationStatus::READY) ready.push(mapper_.reservation(r));
            detail.set("readyReservations", ready);
            return Api::ok("Resource retrieved.", detail);
        });
    });

    router.add("POST", "/api/resources", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const Json body = Api::parseBody(req);
            const Json* type = body.find("type");
            if (!type || !type->isString()) throw ValidationException("Resource type is required (BOOK, EBOOK or JOURNAL).");
            const LibraryResource& created = app_.resources.create(type->asString(), Api::toFieldMap(body));
            return Api::ok("Resource created successfully.", mapper_.resource(created), 201);
        });
    });

    router.add("PUT", "/api/resources/{id}", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const int id = Api::parseId(req.param("id"), "resource");
            const LibraryResource& updated = app_.resources.update(id, Api::toFieldMap(Api::parseBody(req)));
            return Api::ok("Resource updated successfully.", mapper_.resource(updated));
        });
    });

    router.add("DELETE", "/api/resources/{id}", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            app_.resources.remove(Api::parseId(req.param("id"), "resource"));
            return Api::ok("Resource deleted successfully.");
        });
    });
}
