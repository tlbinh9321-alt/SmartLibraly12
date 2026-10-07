#include "services/PersistenceService.h"

#include "utils/DateUtils.h"

void PersistenceService::save() {
    repository_.save(library_);
    lastSavedAt_ = DateUtils::nowTimestamp();
}

void PersistenceService::load() {
    repository_.load(library_);
}
