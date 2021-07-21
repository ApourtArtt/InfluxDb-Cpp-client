#include "InfluxDbClient/InfluxDbClientAsync.h"

int main(int, char**)
{
    auto client = std::make_shared<InfluxDbClientAsync>("127.0.0.1", 8086, "Token");
    // or auto client = std::make_shared<InfluxDbClient>("127.0.0.1", 8086, "Token");
    client->Connect();
    std::cout << client->IsValid();
    client->SetOrganisation("Orga");
    client->SetBucket("Bucket");
    client->SetPrecision(InfluxDbPrecision::s);
    client->RebuildRequestSkeleton();

    client->AddPacketToQueue("NewMeasure",
        {
            {"TagKey", "TagValue"},
        },
        {
            {"FieldKeyStr", "Field\"Value with quote in it"},
            {"FieldKeyDouble", 3.14},
            {"FieldInt", -54},
        }
        );
    client->Send();
    // If async :
            Sleep(1000); // Wait a bit for the request to be processed
            client->Close();
    // If not async, nothing to do, since it's blocking
}
