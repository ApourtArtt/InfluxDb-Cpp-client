#include "InfluxDbClient/InfluxDbClientAsync.h"

int main(int, char**)
{
    auto client = std::make_shared<InfluxDbClientAsync>("127.0.0.1", 8086, "OusSA08-FUXiDc2ODjLlB6ciFPx1OEbAJ7EZk7-oipMh4rIe2kvs4zlMZS5-zZkGDfYOJVawWlIGr5CUU4OQhQ==");
    client->Connect();
    std::cout << client->IsValid();
    client->SetOrganisation("Ditz");
    client->SetBucket("test");
    client->SetPrecision(InfluxDbPrecision::s);
    client->RebuildRequestSkeleton();

    client->AddPacketToQueue("aaNewMeasures16",
        {
            {"TagKey", "TagValue"},
        },
        {
            {"FieldKeyStr", "Field\"Value with quote in it"},
            {"FieldKeyDouble", 3.14},
            {"FielInt", -54},
        }
        );
    client->Send();
    Sleep(5000);
