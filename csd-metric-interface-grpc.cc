#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../ip_config.h"
#include <string.h>
#include <iomanip>
#include <thread>
#include <chrono>
#include <map>
#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"
#include "../rapidjson/prettywriter.h"
#include <curl/curl.h>
#include <InfluxDBFactory.h>

#include <grpcpp/grpcpp.h>
#include "./cmake/build/csd-to-metric-collector-grpc.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using StorageEngineInstance::CSDMetricList;
using StorageEngineInstance::MonitoringContainer;
using StorageEngineInstance::Result;

using namespace std;
using namespace rapidjson;
struct CSDInfo
{
    string id = "";
    string ip = "";
    float cpuUsage = 0;
    float memUsage = 0;
    float diskUsge = 0;
    float network = 0;
    int workingBlockCount = 0;
};
map<string, CSDInfo> csdMap;

unique_ptr<StorageEngineInstance::MonitoringContainer::Stub> stub_;
shared_ptr<Channel> channel;

class MetricInterface
{
public:
    CSDInfo newCSD;

    void receiveCSDMetric()
    {
        // 소켓 생성
        int server_sock, client_sock;
        int opt = 1;
        struct sockaddr_in server_addr;
        struct sockaddr_storage client_addr;
        socklen_t client_addr_size = sizeof(client_addr);

        // 서버 소켓 생성
        server_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (server_sock == -1)
        {
            cout << "socket() error" << endl;
            exit(1);
        }
        if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
        {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
        // 서버 주소 설정
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(CSD_METRIC_INTERFACE_IP);
        server_addr.sin_port = htons(CSD_METRIC_INTERFACE_PORT);
        // 소켓에 주소 바인딩
        if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            cout << "bind() error" << endl;
            exit(1);
        }
        cout << "bind " << endl;
        // 연결 대기 상태 진입
        if (listen(server_sock, 5) < 0)
        {
            cout << "listen() error" << endl;
            exit(1);
        }
        cout << "listen" << endl;
        while (1)
        {
            // 연결 수락
            if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size)) < 0)
            {
                cout << "accept 실패" << endl;
                perror("accept");
                exit(EXIT_FAILURE);
            }
            // cout << "accept, ";
            string json = "";
            int njson;
            size_t ljson;

            recv(client_sock, &ljson, sizeof(ljson), 0);

            char buffer[ljson];
            memset(buffer, 0, ljson);

            while (1)
            {
                if ((njson = recv(client_sock, buffer, 1024, 0)) == -1)
                {
                    perror("read");
                    exit(1);
                }
                ljson -= njson;
                buffer[njson] = '\0';
                json += buffer;

                if (ljson == 0)
                    break;
            }

            // mapping

            // cout << json << endl;
            parseCSD(buffer, newCSD);
            csdMap[newCSD.ip] = newCSD;

            close(client_sock);
        }
        close(server_sock);
    }

    void sendCSDMetricCollector()
    {
        while (true)
        {
            // grpc서버와 주고 받을 스텁과 채널 생성
            channel = grpc::CreateChannel((string)CSD_METRIC_COLLECTIR_IP + ":" + to_string(CSD_METRIC_COLLECTOR_PORT), grpc::InsecureChannelCredentials());

            stub_ = MonitoringContainer::NewStub(channel);
            CSDMetricList request;
            // CSDMetricList::CSDMetric *csdMetric = request.add_csd_metric_list();
            // map 돌면서 request에 추가
            for (const auto &iter1 : csdMap)
            {
                CSDMetricList::CSDMetric csdMetric;
                const auto &ip = iter1.first;
                const auto &CSDInfo = iter1.second;
                // add CSD info
                csdMetric.set_id(CSDInfo.id);
                csdMetric.set_ip(CSDInfo.ip);
                csdMetric.set_cpu_usage(CSDInfo.cpuUsage);
                csdMetric.set_memory_usage(CSDInfo.memUsage);
                csdMetric.set_disk_usage(CSDInfo.diskUsge);
                csdMetric.set_network(CSDInfo.network);
                csdMetric.set_working_block_count(CSDInfo.workingBlockCount);
                // request.add_csd_metric_list();
                request.add_csd_metric_list()->CopyFrom(csdMetric);
            }
            ClientContext context;
            Result response;
            Status status = stub_->SetCSDMetricsInfo(&context, request, &response);

            if (status.ok())
            {
            }
            else
            {
                cout << "RPC failed with error code... " << status.error_code() << endl;
            }
            this_thread::sleep_for(chrono::seconds(2));
        }
    };

    void parseCSD(const char *json_, CSDInfo &newCSD)
    { // json으로 된 csd정보의 각각의 정보들을 구조체인 CSDInfo에 넣는다.
        Document document;
        document.Parse(json_);
        string index = "";
        index = document["ip"].GetString();
        index = index[5];
        newCSD.id = index;
        newCSD.ip = document["ip"].GetString();
        newCSD.cpuUsage = document["cpuUsage"].GetDouble();
        newCSD.memUsage = document["memUsage"].GetDouble();
        newCSD.diskUsge = document["diskUsage"].GetDouble();
        newCSD.network = document["networkSpeed"].GetDouble();
        newCSD.workingBlockCount = document["workingBlockCount"].GetInt();
    };

    /*static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *response)
    {
        size_t totalSize = size * nmemb;
        response->append(static_cast<char *>(contents), totalSize);
        return totalSize;
    }

    void influxDB()
    {

        // InfluxDB API 엔드포인트 URL 설정
        auto db = influxdb::InfluxDBFactory::Get("http://localhost:8086?db=csd_metricDB");
        db->createDatabaseIfNotExists();
        for (auto i : db->query("SHOW DATABASES"))
            std::cout << i.getTags() << std::endl;
    }*/
};
int main()
{
    MetricInterface metricInterface;

    thread receiveCSDMetric(&MetricInterface::receiveCSDMetric, &metricInterface);
    thread sendCSDMetricCollector(&MetricInterface::sendCSDMetricCollector, &metricInterface);
    //thread influxDBThread(&MetricInterface::influxDB, &metricInterface);
    receiveCSDMetric.join();
    sendCSDMetricCollector.join();
    //influxDBThread.join();

    return 0;
}