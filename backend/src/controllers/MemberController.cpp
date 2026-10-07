#include "controllers/MemberController.h"

#include <algorithm>

#include "controllers/ApiSupport.h"
#include "utils/StringUtils.h"

void MemberController::registerRoutes(Router& router) {
    router.add("GET", "/api/members", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const std::string q = StringUtils::trim(req.get("q")), type = req.get("type"), status = req.get("status");
            std::vector<const Member*> matches;
            const auto all = app_.members.getAll();
            std::copy_if(all.begin(), all.end(), std::back_inserter(matches), [&](const Member* m) {
                if (!q.empty() && !StringUtils::containsIgnoreCase(m->getFullName(), q) &&
                    !StringUtils::containsIgnoreCase(m->getEmail(), q) && std::to_string(m->getMemberId()) != q) return false;
                if (!type.empty() && !StringUtils::equalsIgnoreCase(m->getMemberType(), type)) return false;
                if (!status.empty() && !StringUtils::equalsIgnoreCase(toString(m->getStatus()), status)) return false;
                return true;
            });
            Json list = Json::array();
            for (const Member* m : matches) list.push(mapper_.member(*m));
            return Api::ok("Members retrieved.", list);
        });
    });

    router.add("GET", "/api/members/{id}", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const int id = Api::parseId(req.param("id"), "member");
            const Member& member = app_.members.getById(id);
            const std::vector<Loan> loans = app_.loans.getLoansForMember(id);
            std::vector<Loan> current;
            std::copy_if(loans.begin(), loans.end(), std::back_inserter(current), [](const Loan& l) { return l.isActive(); });
            Json reservations = Json::array();
            for (const Reservation& r : app_.reservations.getForMember(id)) reservations.push(mapper_.reservation(r));
            Json detail = mapper_.member(member);
            detail.set("currentLoans", mapper_.loanList(current)).set("loanHistory", mapper_.loanList(loans))
                .set("reservations", std::move(reservations));
            return Api::ok("Member retrieved.", detail);
        });
    });

    router.add("POST", "/api/members", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const Json body = Api::parseBody(req);
            const Json* type = body.find("memberType") ? body.find("memberType") : body.find("type");
            if (!type || !type->isString()) throw ValidationException("Member type is required (STUDENT or FACULTY).");
            const Member& created = app_.members.create(type->asString(), Api::toFieldMap(body));
            return Api::ok("Member created successfully.", mapper_.member(created), 201);
        });
    });

    router.add("PUT", "/api/members/{id}", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const int id = Api::parseId(req.param("id"), "member");
            FieldMap changes = Api::toFieldMap(Api::parseBody(req));
            if (changes.count("memberType")) changes["type"] = changes["memberType"];
            const Member& updated = app_.members.update(id, changes);
            return Api::ok("Member updated successfully.", mapper_.member(updated));
        });
    });
}
