#include <stdint.h>
#include <vector>
#include <map>
#include <variant>

#include <asio.hpp> // !! asio dependency

#include "Utils.h"

enum class InfluxDbPrecision { ns, us, ms, s };

class InfluxDbClient
{
public:
    typedef std::variant<bool, double, uint64_t, int64_t, std::string> FieldValue;

    InfluxDbClient(const std::string& Host, uint16_t Port, const std::string& Token)
        : host(Host)
        , port(std::to_string(Port))
        , token(Token)
        , precision("ms")
        , resolver(ioCtx)
        , socket(ioCtx)
    {
        rebuildRequestSkeleton();

        asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, port);
        asio::connect(socket, endpoints);
    }

    // TODO: Send with a vector of all of this
    void Send(const std::string& Measure, const std::map<std::string, std::string>& Tags, const std::map<std::string, FieldValue>& Fields, int64_t Timestamp = 0)
    {
        std::string packet = getLine(Measure, Tags, Fields, Timestamp);
        sendHttp(packet);
    }

    void SetOrganisation(const std::string& Organisation)
    {
        organisation = Organisation;
        rebuildRequestSkeleton();
    }

    void SetBucket(const std::string& Bucket)
    {
        bucket = Bucket;
        rebuildRequestSkeleton();
    }

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

        rebuildRequestSkeleton();
    }

private:
    void sendHttp(const std::string& Packet)
    {
        asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << req;
        request_stream << "Content-Length: " << Packet.size() << "\r\n\r\n";
        request_stream << Packet;
        asio::write(socket, request);
    }

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

    void rebuildRequestSkeleton()
    {
        req = "POST http://" + host + ":" + port + "/api/v2/write?org=" + organisation + "&bucket=" + bucket + "&precision=" + precision + " HTTP/1.0\r\n";

        if (!token.empty()) [[likely]]
            req += "Authorization: Token " + token + "\r\n";

        req += "Accept: */*\r\n";
        req += "Content-Type: application/json; charset=utf-8\r\n";
        req += "Connection: keep-alive\r\n";
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

    std::string req;
};

int main(int, char**)
{
    InfluxDbClient client("127.0.0.1", 8086, "Token");
    client.SetOrganisation("Orga");
    client.SetBucket("test");
    client.SetPrecision(InfluxDbPrecision::s);

    client.Send("NewMeasures",
        {
            {"TagKey", "TagValue"},
        },
        {
            {"FieldKeyStr", "Field\"Value with quote in it"},
            {"FieldKeyDouble", 3.14},
            {"FielInt", -54},
        }
    );
}
