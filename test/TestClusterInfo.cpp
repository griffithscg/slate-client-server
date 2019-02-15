#include "test.h"

#include <ServerUtilities.h>

TEST(UnauthenticatedGetClusterInfo){
	using namespace httpRequests;
	TestContext tc;
	
	//try fetching cluster information with no authentication
	auto infoResp=httpGet(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters/acluster");
	ENSURE_EQUAL(infoResp.status,403,
				 "Requests to get cluster info without authentication should be rejected");
	
	//try fetching cluster information with invalid authentication
	infoResp=httpGet(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters/acluster?token=00112233-4455-6677-8899-aabbccddeeff");
	ENSURE_EQUAL(infoResp.status,403,
				 "Requests to get cluster info with invalid authentication should be rejected");
}

TEST(GetClusterInfo){
	using namespace httpRequests;
	TestContext tc;

	std::string adminKey=getPortalToken();
	
	//add a VO to register a cluster with
	const std::string voName="testvo1";
	std::string voID;
	{
		rapidjson::Document request1(rapidjson::kObjectType);
		auto& alloc = request1.GetAllocator();
		request1.AddMember("apiVersion", currentAPIVersion, alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("name", voName, alloc);
		metadata.AddMember("scienceField", "Logic", alloc);
		request1.AddMember("metadata", metadata, alloc);
		auto voResp=httpPost(tc.getAPIServerURL()+"/"+currentAPIVersion+"/vos?token="+adminKey,to_string(request1));
		ENSURE_EQUAL(voResp.status,200,"Portal admin user should be able to create a VO");
	
		ENSURE(!voResp.body.empty());
		rapidjson::Document voData;
		voData.Parse(voResp.body.c_str());
		voID=voData["metadata"]["id"].GetString();
	}
	
	//add a cluster
	auto kubeConfig=tc.getKubeConfig();
	const std::string clusterName="testcluster";
	{
		rapidjson::Document request1(rapidjson::kObjectType);
		auto& alloc = request1.GetAllocator();
		request1.AddMember("apiVersion", currentAPIVersion, alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("name", clusterName, alloc);
		metadata.AddMember("vo", rapidjson::StringRef(voID), alloc);
		metadata.AddMember("organization", "Department of Labor", alloc);
		metadata.AddMember("kubeconfig", rapidjson::StringRef(kubeConfig), alloc);
		request1.AddMember("metadata", metadata, alloc);
		auto createResp=httpPost(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters?token="+adminKey, to_string(request1));
		ENSURE_EQUAL(createResp.status,200, "Cluster creation should succeed");
	}
	
	//get cluster's info
	auto infoResp=httpGet(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters/"+clusterName+"?token="+adminKey);
	ENSURE_EQUAL(infoResp.status,200,"Portal admin user should be able to fetch cluster info");

	ENSURE(!infoResp.body.empty());
	rapidjson::Document data;
	data.Parse(infoResp.body.c_str());
	auto schema = loadSchema(getSchemaDir()+"/ClusterInfoResultSchema.json");
	ENSURE_CONFORMS(data,schema);
	
	const auto& metadata=data["metadata"];
	ENSURE_EQUAL(metadata["name"].GetString(),clusterName,"Cluster name should match");
	ENSURE_EQUAL(metadata["owningVO"].GetString(),voName,"Cluster owning VO should match");
	ENSURE_EQUAL(metadata["owningOrganization"].GetString(),std::string("Department of Labor"),
	             "Cluster owning organization should match");
	ENSURE(metadata.HasMember("id"));
}
