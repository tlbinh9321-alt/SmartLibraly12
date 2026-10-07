#include "controllers/LoanController.h"

#include <algorithm>

#include "controllers/ApiSupport.h"
#include "utils/StringUtils.h"

void LoanController::registerRoutes(Router& router) {
    router.add("GET", "/api/loans", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const std::string status = StringUtils::toLower(req.get("status"));
            std::vector<Loan> loans = app_.loans.getAllLoans();
            std::vector<Loan> selected;
            const std::string memberText = req.get("memberId"), resourceText = req.get("resourceId");
            const int memberId = memberText.empty() ? 0 : Api::parseId(memberText, "member");
            const int resourceId = resourceText.empty() ? 0 : Api::parseId(resourceText, "resource");
            std::copy_if(loans.begin(), loans.end(), std::back_inserter(selected), [&](const Loan& l) {
                if (memberId && l.getMemberId() != memberId) return false;
                if (resourceId && l.getResourceId() != resourceId) return false;
                if (status == "active") return l.isActive();
                if (status == "overdue") return l.getStatus() == LoanStatus::OVERDUE;
                if (status == "returned") return l.isReturned();
                return true;
            });
            return Api::ok("Loans retrieved.", mapper_.loanList(selected));
        });
    });

    router.add("GET", "/api/loans/{id}", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const int id = Api::parseId(req.param("id"), "loan");
            app_.loans.getAllLoans();  // refresh overdue flags
            const Loan* loan = app_.library.findLoan(id);
            if (!loan) throw NotFoundException("Loan not found.");
            return Api::ok("Loan retrieved.", mapper_.loan(*loan));
        });
    });

    router.add("GET", "/api/loans/{id}/preview", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const int id = Api::parseId(req.param("id"), "loan");
            const ReturnPreview preview = app_.loans.previewReturn(id);
            const Loan* loan = app_.library.findLoan(id);
            Json data = mapper_.loan(*loan);
            data.set("asOfDate", preview.asOfDate).set("overdueDays", preview.overdueDays).set("calculatedFine", preview.fine);
            const Member* member = app_.library.findMember(loan->getMemberId());
            const LibraryResource* resource = app_.library.findResource(loan->getResourceId());
            data.set("finePolicy", app_.fines.describePolicy(*member, *resource));
            return Api::ok(preview.overdueDays > 0 ? "This loan is overdue." : "This loan is not overdue.", data);
        });
    });

    router.add("POST", "/api/loans/borrow", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const Json body = Api::parseBody(req);
            const int memberId = Api::requiredInt(body, "memberId", "Invalid member ID.");
            const int resourceId = Api::requiredInt(body, "resourceId", "Invalid resource ID.");
            const Loan loan = app_.loans.borrowResource(memberId, resourceId);
            return Api::ok("Resource borrowed successfully.", mapper_.loan(loan), 201);
        });
    });

    router.add("POST", "/api/loans/return", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const Json body = Api::parseBody(req);
            const int loanId = Api::requiredInt(body, "loanId", "Invalid loan ID.");
            const auto memberId = Api::optionalInt(body, "memberId", "Invalid member ID.");
            const ReturnResult result = app_.loans.returnResource(loanId, memberId);
            Json notifications = Json::array();
            Json ready = Json::array();
            for (const Reservation& r : result.promoted) {
                ready.push(mapper_.reservation(r));
                const Json info = mapper_.reservation(r);
                notifications.push("Next member is ready: " + info.find("memberName")->asString() + " can now borrow '" +
                                   info.find("resourceTitle")->asString() + "'.");
            }
            Json data = Json::object();
            data.set("loan", mapper_.loan(result.loan)).set("overdueDays", result.overdueDays).set("fine", result.fine)
                .set("readyReservations", std::move(ready)).set("notifications", std::move(notifications));
            std::string message = "Resource returned successfully.";
            if (result.fine > 0) message += " Late fine: $" + StringUtils::formatMoney(result.fine) + ".";
            return Api::ok(message, data);
        });
    });

    router.add("POST", "/api/loans/{id}/pay-fine", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const int id = Api::parseId(req.param("id"), "loan");
            app_.loans.payFine(id);
            return Api::ok("Fine marked as paid.", mapper_.loan(*app_.library.findLoan(id)));
        });
    });
}
