// Smart Library & Resource Management System - server entry point.
// Wires the layers together:  HTTP -> controllers -> services -> models -> files.
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "controllers/LoanController.h"
#include "controllers/MemberController.h"
#include "controllers/ReservationController.h"
#include "controllers/ResourceController.h"
#include "controllers/SearchController.h"
#include "controllers/SystemController.h"
#include "services/LibraryApp.h"
#include "utils/HttpServer.h"

#ifndef SMARTLIBRARY_ROOT
#define SMARTLIBRARY_ROOT "."
#endif

namespace {
struct Options {
    std::string host = "127.0.0.1";
    int port = 8080;
    std::string frontendDir = std::string(SMARTLIBRARY_ROOT) + "/frontend";
    std::string dataDir = std::string(SMARTLIBRARY_ROOT) + "/backend/data";
    std::string reportsDir = std::string(SMARTLIBRARY_ROOT) + "/backend/reports";
    bool startEmpty = false;
};

void printUsage() {
    std::cout << "Usage: smartlibrary [--port N] [--host IP] [--data DIR] [--reports DIR] [--frontend DIR] [--empty]\n"
                 "  --empty   when no saved data exists, start with an empty library instead of demo data\n";
}

bool parseArgs(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) throw std::runtime_error("Missing value for " + arg);
            return argv[++i];
        };
        if (arg == "--port") options.port = std::stoi(value());
        else if (arg == "--host") options.host = value();
        else if (arg == "--data") options.dataDir = value();
        else if (arg == "--reports") options.reportsDir = value();
        else if (arg == "--frontend") options.frontendDir = value();
        else if (arg == "--empty") options.startEmpty = true;
        else if (arg == "--help" || arg == "-h") return false;
        else throw std::runtime_error("Unknown option: " + arg);
    }
    return true;
}
}  // namespace

int main(int argc, char** argv) {
    try {
        Options options;
        if (!parseArgs(argc, argv, options)) { printUsage(); return 0; }

        LibraryApp app(options.dataDir, options.reportsDir);

        // LOAD STATE on start-up (or create first-run data).
        if (app.persistence.hasSavedData()) {
            app.persistence.load();
            std::cout << "Loaded saved state from " << app.persistence.dataDirectory() << "\n";
        } else {
            if (!options.startEmpty) app.seedDemoData();
            app.persistence.save();
            std::cout << (options.startEmpty ? "Started with an empty library" : "First run: loaded DEMO data")
                      << " and saved it to " << app.persistence.dataDirectory() << "\n";
        }

        // SAVE STATE whenever a service reports a change.
        app.library.setChangeListener([&app] {
            try { app.persistence.save(); }
            catch (const std::exception& e) { std::cerr << "WARNING: could not save state: " << e.what() << "\n"; }
        });

        Router router;
        ResourceController resourceController(app);
        MemberController memberController(app);
        LoanController loanController(app);
        ReservationController reservationController(app);
        SearchController searchController(app);
        SystemController systemController(app);
        resourceController.registerRoutes(router);
        memberController.registerRoutes(router);
        loanController.registerRoutes(router);
        reservationController.registerRoutes(router);
        searchController.registerRoutes(router);
        systemController.registerRoutes(router);

        std::cout << "Frontend: " << options.frontendDir << "\nReports : " << options.reportsDir << "\n";
        HttpServer server(options.host, options.port, options.frontendDir, router);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
