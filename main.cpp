// Experimentations
#include <stdint.h>
#include <string>
#include <vector>
#include <map>

#include <iostream>

enum class InfluxDbPrecision { ns, us, ms, s };

class InfluxDbClient
{
public:    
    InfluxDbClient(const std::string& Endpoint, const std::string& Token)
        : token("Token " + Token)
    {
        url = Endpoint;
        if (!url.ends_with("/"))
            url += "/";
    }

    // TODO: Send with a vector of all of this
    void Send(const std::string& Measure, const std::map<std::string, std::string>& Tags, const std::map<std::string, std::string>& Fields, int64_t Timestamp = -1)
    {
        std::string packet = GetLine(Measure, Tags, Fields, Timestamp);
        std::cout << packet;
    }
    
    void SetOrganisation(const std::string& Organisation)
    {
        organisation = Organisation;
    }
    
    void SetBucket(const std::string& Bucket)
    {
        bucket = Bucket;
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
    }
  
private:
    std::string GetLine(const std::string& Measure, const std::map<std::string, std::string>& Tags, const std::map<std::string, std::string>& Fields, int64_t Timestamp)
    {
        std::string output;
        output += Measure;
        
        for (auto& tag: Tags)
            output += "," + tag.first + "=" + tag.second;
        
        output += " ";
        
        auto&& it = Fields.begin();

        if (Fields.size() >= 1) [[likely]]
        {
            output += it->first + "=" + it->second;
            std::advance(it, 1);
        }
        
        for (; it != Fields.end() ; it++)
            output += "," + it->first + "=" + it->second;

        if (Timestamp != -1) [[unlikely]]
            output += " " + std::to_string(Timestamp);

        return output;
    }
            
    std::string token;
    std::string url;
    std::string organisation;
    std::string bucket;
    std::string precision;
};

int main(int, char**)
{
    InfluxDbClient client("http://127.0.0.1:8086/api/v2/", "token");
    client.SetOrganisation("YOUR_ORG");
    client.SetBucket("YOUR_BUCKET");
    client.SetPrecision(InfluxDbPrecision::ms);
    
    client.Send("mem",
        {
            {"host", "host1"},
        },
        {
            {"used_percent", "23.43234543"},
            {"used_str", "\"fezfzefze\""},
        },
        1556896326
    );
}
