#include <iostream>
#include <memory>
#include <string>
#include "../ip_config.h"
#include <grpcpp/grpcpp.h>
#include <thread>
#include <map>
#include "./cmake/build/csd-to-metric-collector-grpc.grpc.pb.h"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using StorageEngineInstance::CSDMetricList;
using StorageEngineInstance::MonitoringContainer;
using StorageEngineInstance::Result;
using namespace std;
struct CSDInfo
{
  string id = "";
  string ip = "";
  float cpu_usage = 0;
  float mem_usage = 0;
  float disk_usage = 0;
  float network = 0;
  int block_count = 0;
};
map<string, CSDInfo> csdMap;
class StorageEngineMetricCollector
{

public:
  class MetricCollectorServiceImpl final : public MonitoringContainer::Service
  {
  public:
    Status SetCSDMetricsInfo(ServerContext *context, const CSDMetricList *csdMetricsInfo, Result *csdMetricresponse) override
    {

      for (int i = 0; i < csdMetricsInfo->csd_metric_list_size() ; i++)
      {
        const CSDMetricList::CSDMetric &status = csdMetricsInfo->csd_metric_list(i);
        CSDInfo newCSD;
        newCSD.id = status.id();
        newCSD.ip = status.ip();
        newCSD.cpu_usage = status.cpu_usage();
        newCSD.mem_usage = status.memory_usage();
        newCSD.disk_usage = status.disk_usage();
        newCSD.network = status.network();
        newCSD.block_count = status.working_block_count();
        csdMap[status.ip()] = newCSD;

        cout << "CSD IP: " << status.ip();
        cout << ", CPU Usage: " << status.cpu_usage();
        cout << "%, Memory Usage: " << status.memory_usage();
        cout << "%, disk Usage: " << status.disk_usage();
        cout << "%, network: " << status.network();
      }
      for (const auto &pair : csdMap)
      {
        const string &key = pair.first;
        const CSDInfo &csdInfo = pair.second;
        // cout << "콜렉터cpu: " << pair.second.cpuUsage << " , 메모리: " << pair.second.memUsage;
      }
      csdMetricresponse->set_value("Metric stream ended.");

      return Status::OK;
    }
  };

  void RunMetricCollector()
  {

    string server_address((string)CSD_METRIC_INTERFACE_IP + ":" + (to_string)(CSD_METRIC_COLLECTOR_PORT));
    MetricCollectorServiceImpl service;

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    unique_ptr<Server> server(builder.BuildAndStart());
    cout << "Server listening on " << server_address << endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
  };
};

class CSDAnalysisModule
{
public:
  void calculate()
  {
    while (true)
    {
      // 점수계산
      float cpuw = 0.5, memw = 0.5, diskw = 0, networkw = 0;
      float scoreArr[csdMap.size()];
      int i = 0;
      for (const auto &pair : csdMap)
      {
        const string &key = pair.first;
        const CSDInfo &csdInfo = pair.second;
        cout << "ip: " << pair.first;
        cout << ", cpu: " << pair.second.cpu_usage;
        cout << ", mem: " << pair.second.mem_usage;
        scoreArr[i] = 100 - ((cpuw * pair.second.cpu_usage) + (memw * pair.second.mem_usage));
        i++;
      }
      cout << endl;
      for (int i = 0; i < csdMap.size(); i++)
      {
        cout << i << "번 csd 점수: " << scoreArr[i] << endl;
      }
      cout << endl;
      sleep(2);
    }
  }
};
int main(int argc, char **argv)
{
  StorageEngineMetricCollector metricCollector;
  CSDAnalysisModule csdAnalysisModule;
  thread receiveCSDsMetric(&StorageEngineMetricCollector::RunMetricCollector, &metricCollector);
  thread csdAnalsis(&CSDAnalysisModule::calculate, &csdAnalysisModule);

  receiveCSDsMetric.detach();

  csdAnalsis.join();
  return 0;
}
