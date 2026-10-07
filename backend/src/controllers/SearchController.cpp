#include "controllers/SearchController.h"

#include "controllers/ApiSupport.h"
#include "utils/StringUtils.h"

void SearchController::registerRoutes(Router& router) {
    router.add("GET", "/api/search", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            SearchCriteria criteria;
            criteria.query = req.get("query");
            criteria.title = req.get("title");
            criteria.author = req.get("author");
            criteria.isbn = req.get("isbn");
            criteria.genre = req.get("genre");
            criteria.type = req.get("type");
            const std::string available = StringUtils::toLower(req.get("available"));
            if (available == "true") criteria.availableOnly = true;
            else if (available == "false") criteria.availableOnly = false;

            const auto results = app_.search.search(criteria);  // throws "Search query cannot be empty."
            Json list = Json::array();
            for (const LibraryResource* r : results) list.push(mapper_.resource(*r));
            Json data = Json::object();
            data.set("count", static_cast<int>(results.size())).set("results", std::move(list));
            return Api::ok(std::to_string(results.size()) + " result(s) found.", data);
        });
    });
}
