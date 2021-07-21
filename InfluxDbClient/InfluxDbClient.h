#pragma once
#include <stdint.h>
#include <vector>
#include <map>
#include <variant>
#include <asio.h>
#include "Utils.h"

enum class InfluxDbPrecision { ns, us, ms, s };

class InfluxDbClient : public std::enable_shared_from_this<InfluxDbClient>
{
public:
    const short INFLUXDB_BATCH_MAX_CAPACITY = 5000;
    typedef std::variant<bool, double, uint64_t, int64_t, std::string> FieldValue;

    InfluxDbClient(const std::string& Host, uint16_t Port, const std::string& Token)
        : host(Host)
        , port(std::to_string(Port))
        , token(Token)
        , precision("ms")
        , resolver(ioCtx)
        , socket(ioCtx)
        , valid(false)
    {}

    [[nodiscard]] bool IsValid() { return valid; }
    void SetOrganisation(const std::string& Organisation) { organisation = Organisation; }
    void SetBucket(const std::string& Bucket) { bucket = Bucket; }

    void SetPrecision(InfluxDbPrecision Precision)
    {
        if (Precision == InfluxDbPrecision::ns)
            precision = "ns";
        else if (Precision == InfluxDbPrecision::us)
            precision = "us";
        else if (Precision == InfluxDbPrecision::ms)
            precision = "ms";
        else
            precision = "s";
    }

    virtual void Connect()
    {
        asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, port);
        asio::connect(socket, endpoints);
    }

    void RebuildRequestSkeleton()
    {
        influx.lock();

        req = "POST http://" + host + ":" + port + "/api/v2/write?org=" + organisation + "&bucket=" + bucket + "&precision=" + precision + " HTTP/1.0\r\n";

        if (!token.empty()) [[likely]]
            req += "Authorization: Token " + token + "\r\n";

        req += "Accept: */*\r\n";
        req += "Content-Type: application/json; charset=utf-8\r\n";
        req += "Connection: keep-alive\r\n";
        req += "Content-Length: "; // Note : end of the line voluntarily elided (see Send())

        influx.unlock();
    }

    void AddPacketToQueue(const std::string& Measure, const std::map<std::string, std::string>& Tags, const std::map<std::string, FieldValue>& Fields, int64_t Timestamp = 0)
    {
        influx.lock();
        
        std::string packet = getLine(Measure, Tags, Fields, Timestamp);
        packets.push_back(packet);
        if (packets.size() >= INFLUXDB_BATCH_MAX_CAPACITY) [[unlikely]]
            Send();

        influx.unlock();
    }

    virtual void Send()
    {
        asio::streambuf request;
        std::ostream request_stream(&request);

        influx.lock();

        std::string p = JoinString(packets);
        request_stream << req;

        influx.unlock();

        request_stream << p.size() << "\r\n\r\n";
        request_stream << p;
        asio::write(socket, request);

#ifdef INFLUX_DEBUG
        asio::streambuf response;
        asio::read_until(socket, response, "\r\n");
        std::istream response_stream(&response);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/")
            std::cout << "Invalid response\n";
        if (status_code != 204)
            std::cout << "Response returned with status code " << status_code << "\n";
        else
            std::cout << "Worked";
#endif
    }

public:
    std::string getLine(const std::string& Measure, const std::map<std::string, std::string>& Tags, const std::map<std::string, FieldValue>& Fields, int64_t Timestamp)
    {
        std::string output;
        output += Measure;

        for (auto& tag : Tags)
            output += "," + tag.first + "=" + tag.second;

        output += " ";

        auto&& it = Fields.begin();

        if (Fields.size() >= 1) [[likely]]
        {
            output += it->first + "=" + getFieldValue(it->second);
            std::advance(it, 1);
        }

        for (; it != Fields.end(); it++)
            output += "," + it->first + "=" + getFieldValue(it->second);

        if (Timestamp != 0) [[unlikely]]
            output += " " + std::to_string(Timestamp);

        return output;
    }

    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    std::string getFieldValue(const FieldValue& Value)
    {
        return std::visit(overloaded{
            [](bool value) -> std::string { return value ? "true" : "false"; },
            [](double value) { return std::to_string(value); },
            [](uint64_t value) { return std::to_string(value) + "u"; },
            [](int64_t value) { return std::to_string(value) + "i"; },
            [](const std::string& value) { return "\"" + ReplaceAll(value, "\"", "\\\"") + "\""; },
        }, Value);
    }

    std::string host;
    std::string port;
    std::string token;
    std::string organisation;
    std::string bucket;
    std::string precision;

    asio::io_context ioCtx;
    asio::ip::tcp::resolver resolver;
    asio::ip::tcp::socket socket;
    bool valid;

    std::string req;
    std::vector<std::string> packets;

    std::mutex influx;
};
