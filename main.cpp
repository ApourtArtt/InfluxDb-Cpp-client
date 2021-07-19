// Experimentations

enum const InfluxDbPrecision
{
    ns, us, ms, s
};

class InfluxDbClient
{
public:
    InfluxDbClient(const std::string& Endpoint)
    {
        url = Endpoint;
        if (!url.endsWith("/")
            url += "/";
    }
            
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
  
private:
    std::string url;
    std::string organisation;
    std::string bucket;
    std::string precision;
};

int main(int, char**)
{
    InfluxDbClient client("http://127.0.0.1:8086/api/v2/");
    client.SetOrganisation("YOUR_ORG");
    client.SetBucket("YOUR_BUCKET");
    client.SetPrecision(InfluxDbPrecision::ms);
}

/*
Organization	Use the org query parameter in your request URL.
Bucket	Use the bucket query parameter in your request URL.
Precision	Use the precision query parameter in your request URL.
Authentication token	Use the Authorization: Token header.
Line protocol	Pass as plain text in your request body.

curl --request POST "http://localhost:8086/api/v2/write?org=YOUR_ORG&bucket=YOUR_BUCKET&precision=s" \
  --header "Authorization: Token YOURAUTHTOKEN" \
  --data-raw "
mem,host=host1 used_percent=23.43234543 1556896326
mem,host=host2 used_percent=26.81522361 1556896326
mem,host=host1 used_percent=22.52984738 1556896336
mem,host=host2 used_percent=27.18294630 1556896336
"
*/
