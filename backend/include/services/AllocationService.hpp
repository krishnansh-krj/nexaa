#pragma once
#include <drogon/drogon.h>
#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
namespace nexa {
class AllocationService {
public:
 using OperationCallback = std::function<void(bool, const std::string&)>;
private:
 std::shared_ptr<drogon::orm::DbClient> dbClient;
 struct CompletionState {
  std::atomic_bool completed{false}; OperationCallback callback;
  explicit CompletionState(OperationCallback cb);
  void finish(bool success, const std::string& message);
 };
 static void refreshRequestStatus(const std::shared_ptr<drogon::orm::Transaction>& trans, unsigned long long requestId, std::function<void()> onSuccess, std::function<void(const std::string&)> onFailure);
public:
 explicit AllocationService(std::shared_ptr<drogon::orm::DbClient> db);
 void allocate(unsigned long long incidentId, unsigned long long resourceId, unsigned int units, std::optional<unsigned long long> requestId, OperationCallback callback);
 void allocate(unsigned long long incidentId, unsigned long long resourceId, unsigned int units, OperationCallback callback);
 void releaseAllocation(unsigned long long allocationId, OperationCallback callback);
};
}
