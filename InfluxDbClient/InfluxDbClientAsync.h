#pragma once
#include "InfluxDbClient.h"

class InfluxDbClientAsync : public InfluxDbClient, public std::enable_shared_from_this<InfluxDbClientAsync>
{
public:
	InfluxDbClientAsync(const std::string& Host, uint16_t Port, const std::string& Token)
		: InfluxDbClient(Host, Port, Token)
	{}

    ~InfluxDbClientAsync()
    {
        Close();
    }

	virtual void Connect()
	{
        asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, port);
        asio::connect(socket, endpoints);
	}

    void Close()
    {
        if (thread.joinable())
        {
            ioCtx.reset();
            thread.join();
        }
    }

    virtual void Send()
    {
        asio::streambuf* request = new asio::streambuf;
        std::ostream request_stream(request);

        influx.lock();

        std::string p = JoinString(packets);
        request_stream << req;

        influx.unlock();

        request_stream << p.size() << "\r\n\r\n";
        request_stream << p;
        asio::async_write(socket, *request, [request, self = shared_from_this()](const asio::error_code& err, size_t length)
        {
            delete request;
#ifdef INFLUX_DEBUG
            asio::streambuf response;
            asio::read_until(self->socket, response, "\r\n");
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
        });
        thread = std::thread([self = shared_from_this()]
        {
            self->ioCtx.run();
        });
    }

private:
    std::thread thread;
};
