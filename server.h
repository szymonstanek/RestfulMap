#include <iostream>
#include <fstream>
#include <map>
#include <mutex>
#include <atomic>
#include <thread>
#include <csignal>
#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include "json.hpp"
#include <unistd.h>
#include "globals.h"
std::string logPath;

using namespace Pistache;
using namespace std;
using json = nlohmann::json;

class KeyValueApi
{
public:
    KeyValueApi(Address addr) : httpEndpoint(std::make_shared<Http::Endpoint>(addr)) {}

    void init(size_t thr = 2)
    {
        auto opts = Http::Endpoint::options()
                .threads(static_cast<int>(thr))
                .flags(Tcp::Options::ReuseAddr);
        httpEndpoint->init(opts);
        createRoutes();
        loadStorage();
    }

    void start()
    {
        httpEndpoint->setHandler(router.handler());
        httpEndpoint->serve();
    }

    void stop()
    {
        httpEndpoint->shutdown();
        saveStorage();
    }

    void logEvent(const string& method, const string& key) {
        ofstream logFile(logPath, ios::app);
        if (logFile.is_open()) {
            logFile << "[" << time(nullptr) << "] " << method << " key: " << key << endl;
            logFile.close();
        }
    }
    void loadStorage()
    {
        ifstream i(storagePath);
        if (i) {
            json j;
            i >> j;
            keyValueMap = j.get<map<string, string>>();
        }
    }

    void saveStorage()
    {
        ofstream o(storagePath);
        json j = keyValueMap;
        o << j.dump(4);
    }


    ~KeyValueApi()
    {
        //to-do
    }

private:
    void createRoutes()
    {
        using namespace Rest;
        Routes::Get(router, "/key/:key", Routes::bind(&KeyValueApi::getKeyValue, this));
        Routes::Get(router, "/all", Routes::bind(&KeyValueApi::getAllKeyValue, this));
        Routes::Post(router, "/add/:key", Routes::bind(&KeyValueApi::addKey, this));
        Routes::Post(router, "/add/:key/:value", Routes::bind(&KeyValueApi::addKeyValue, this));
        Routes::Put(router, "/update/:key/:value", Routes::bind(&KeyValueApi::updateKeyValue, this));
        Routes::Delete(router, "/purge/:key", Routes::bind(&KeyValueApi::deleteKey, this));
    }

    void getKeyValue(const Rest::Request& request, Http::ResponseWriter response)
    {
        string key = request.param(":key").as<string>();
        logEvent("GET", key);

        lock_guard<mutex> lock(mapMutex);
        auto it = keyValueMap.find(key);
        if (it != keyValueMap.end())
        {
            json result = {
                    {"key", key},
                    {"value", it->second}
            };
            response.send(Http::Code::Ok, result.dump(), MIME(Application, Json));
        }
        else
        {
            json result = {
                    {"error", "Key not found"}
            };
            response.send(Http::Code::Not_Found, result.dump(), MIME(Application, Json));
        }
    }

    void getAllKeyValue(const Rest::Request& request, Http::ResponseWriter response)
    {
        logEvent("GET ALL", "ALL");
        lock_guard<mutex> lock(mapMutex);
        json result;
        for (const auto& pair : keyValueMap)
        {
            result[pair.first] = pair.second;
        }
        response.send(Http::Code::Ok, result.dump(), MIME(Application, Json));
    }

    void addKey(const Rest::Request& request, Http::ResponseWriter response)
    {
        string key = request.param(":key").as<string>();
        logEvent("POST", key);

        lock_guard<mutex> lock(mapMutex);
        keyValueMap[key] = "";
        json result = {
                {"message", "Key added successfully"}
        };
        response.send(Http::Code::Created, result.dump(), MIME(Application, Json));
    }

    void addKeyValue(const Rest::Request& request, Http::ResponseWriter response)
    {
        string key = request.param(":key").as<string>();
        string value = request.param(":value").as<string>();
        logEvent("POST", key);

        lock_guard<mutex> lock(mapMutex);
        keyValueMap[key] = value;
        json result = {
                {"message", "Key and value added successfully"}
        };
        response.send(Http::Code::Created, result.dump(), MIME(Application, Json));
    }

    void updateKeyValue(const Rest::Request& request, Http::ResponseWriter response)
    {
        string key = request.param(":key").as<string>();
        string value = request.param(":value").as<string>();
        logEvent("PUT", key);

        lock_guard<mutex> lock(mapMutex);
        auto it = keyValueMap.find(key);
        if (it != keyValueMap.end())
        {
            it->second = value;
            json result = {
                    {"message", "Value updated successfully"}
            };
            response.send(Http::Code::Ok, result.dump(), MIME(Application, Json));
        }
        else
        {
            json result = {
                    {"error", "Key not found"}
            };
            response.send(Http::Code::Not_Found, result.dump(), MIME(Application, Json));
        }
    }

    void deleteKey(const Rest::Request& request, Http::ResponseWriter response)
    {
        string key = request.param(":key").as<string>();
        logEvent("DELETE", key);

        lock_guard<mutex> lock(mapMutex);
        auto it = keyValueMap.find(key);
        if (it != keyValueMap.end())
        {
            keyValueMap.erase(it);
            json result = {
                    {"message", "Key deleted successfully"}
            };
            response.send(Http::Code::Ok, result.dump(), MIME(Application, Json));
        }
        else
        {
            json result = {
                    {"error", "Key not found"}
            };
            response.send(Http::Code::Not_Found, result.dump(), MIME(Application, Json));
        }
    }

    std::shared_ptr<Http::Endpoint> httpEndpoint;
    Rest::Router router;
    std::map<std::string, std::string> keyValueMap;
    std::mutex mapMutex;
};
