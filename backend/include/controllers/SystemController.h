#pragma once
#include "controllers/JsonMapper.h"
#include "services/LibraryApp.h"
#include "utils/HttpServer.h"

class SystemController {
public:
    explicit SystemController(LibraryApp& app) : app_(app), mapper_(app) {}
    void registerRoutes(Router& router);

private:
    LibraryApp& app_;
    JsonMapper mapper_;
};
