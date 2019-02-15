#include "test.h"

#include <ServerUtilities.h>

TEST(UnauthenticatedUpdateCluster){
	using namespace httpRequests;
	TestContext tc;
	
	//try updating a cluster with no authentication
	auto resp=httpPut(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters/Cluster_1234567890","stuff");
	ENSURE_EQUAL(resp.status,403,
	             "Requests update clusters without authentication should be rejected");
	
	//try updating a cluster with invalid authentication
	resp=httpPut(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters/Cluster_1234567890?token=00112233-4455-6677-8899-aabbccddeeff","stuff");
	ENSURE_EQUAL(resp.status,403,
	             "Requests to update cluster with invalid authentication should be rejected");
}

TEST(UpdateNonexistentCluster){
	using namespace httpRequests;
	TestContext tc;
	
	std::string adminKey=getPortalToken();
	auto resp=httpPut(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters/Cluster_1234567890?token="+adminKey,"stuff");
	ENSURE_EQUAL(resp.status,404,
				 "Requests to update a nonexistant cluster should be rejected");
}

TEST(UpdateCluster){
	using namespace httpRequests;
	TestContext tc;
	
	std::string adminKey=getPortalToken();
	std::string originalConfig=tc.getKubeConfig();
	std::string newConfig=originalConfig+"\n\n\n";

	//create VO to register cluster with
	rapidjson::Document createVO(rapidjson::kObjectType);
	{
		auto& alloc = createVO.GetAllocator();
		createVO.AddMember("apiVersion", currentAPIVersion, alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("name", "testvo1", alloc);
		metadata.AddMember("scienceField", "Logic", alloc);
		createVO.AddMember("metadata", metadata, alloc);
	}
	auto voResp=httpPost(tc.getAPIServerURL()+"/"+currentAPIVersion+"/vos?token="+adminKey,
			     to_string(createVO));
	ENSURE_EQUAL(voResp.status,200,"VO creation request should succeed");
	rapidjson::Document voData;
	voData.Parse(voResp.body.c_str());
	auto voID=voData["metadata"]["id"].GetString();

	//create cluster
	rapidjson::Document request1(rapidjson::kObjectType);
	{
		auto& alloc = request1.GetAllocator();
		request1.AddMember("apiVersion", currentAPIVersion, alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("name", "testcluster", alloc);
		metadata.AddMember("vo", rapidjson::StringRef(voID), alloc);
		metadata.AddMember("organization", "Department of Labor", alloc);
		metadata.AddMember("kubeconfig", rapidjson::StringRef(originalConfig), alloc);
		request1.AddMember("metadata", metadata, alloc);
	}
	auto createResp=httpPost(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters?token="+adminKey,
				 to_string(request1));
	ENSURE_EQUAL(createResp.status,200,
		     "Cluster creation request should succeed");
	ENSURE(!createResp.body.empty());
	rapidjson::Document createData;
	createData.Parse(createResp.body.c_str());
	auto clusterID=createData["metadata"]["id"].GetString();

	auto infoSchema=loadSchema(getSchemaDir()+"/ClusterInfoResultSchema.json");
	
	{ //update cluster's kubeconfig
		rapidjson::Document updateRequest(rapidjson::kObjectType);
		auto& alloc = updateRequest.GetAllocator();
		updateRequest.AddMember("apiVersion", currentAPIVersion, alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("kubeconfig", rapidjson::StringRef(newConfig.c_str()), alloc);
		updateRequest.AddMember("metadata", metadata, alloc);

		auto updateResp=httpPut(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters/"+clusterID+"?token="+adminKey,
					 to_string(updateRequest));
		ENSURE_EQUAL(updateResp.status,200,"Updating the cluster config should succeed");
		
		auto infoResp=httpGet(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters/"+clusterID+"?token="+adminKey);
		ENSURE_EQUAL(infoResp.status,200,"Cluster info request should succeed");
		ENSURE(!infoResp.body.empty());
		rapidjson::Document infoData;
		infoData.Parse(infoResp.body.c_str());
		ENSURE_CONFORMS(infoData,infoSchema);
		ENSURE_EQUAL(infoData["metadata"]["name"].GetString(),std::string("testcluster"),
					 "Cluster name should remain unchanged");
		ENSURE_EQUAL(infoData["metadata"]["owningOrganization"].GetString(),std::string("Department of Labor"),
					 "Cluster organization should remain unchanged");
	}
	{ //update cluster's organization
		rapidjson::Document updateRequest(rapidjson::kObjectType);
		auto& alloc = updateRequest.GetAllocator();
		updateRequest.AddMember("apiVersion", currentAPIVersion, alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("owningOrganization", "Department of the Interior", alloc);
		updateRequest.AddMember("metadata", metadata, alloc);

		auto updateResp=httpPut(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters/"+clusterID+"?token="+adminKey,
					 to_string(updateRequest));
		ENSURE_EQUAL(updateResp.status,200,"Updating the cluster config should succeed");
		
		auto infoResp=httpGet(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters/"+clusterID+"?token="+adminKey);
		ENSURE_EQUAL(infoResp.status,200,"Cluster info request should succeed");
		ENSURE(!infoResp.body.empty());
		rapidjson::Document infoData;
		infoData.Parse(infoResp.body.c_str());
		ENSURE_CONFORMS(infoData,infoSchema);
		ENSURE_EQUAL(infoData["metadata"]["name"].GetString(),std::string("testcluster"),
					 "Cluster name should remain unchanged");
		ENSURE_EQUAL(infoData["metadata"]["owningOrganization"].GetString(),std::string("Department of the Interior"),
					 "Cluster organization should match new value");
	}
	{ //update cluster's organization
		rapidjson::Document updateRequest(rapidjson::kObjectType);
		auto& alloc = updateRequest.GetAllocator();
		updateRequest.AddMember("apiVersion", currentAPIVersion, alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		rapidjson::Value locations(rapidjson::kArrayType);
		rapidjson::Value L1(rapidjson::kObjectType);
		L1.AddMember("lat", 22.7, alloc);
		L1.AddMember("lon", -68, alloc);
		locations.PushBack(L1,alloc);
		rapidjson::Value L2(rapidjson::kObjectType);
		L2.AddMember("lat", 54.66, alloc);
		L2.AddMember("lon", -87.2, alloc);
		locations.PushBack(L2,alloc);
		metadata.AddMember("location", locations, alloc);
		updateRequest.AddMember("metadata", metadata, alloc);

		auto updateResp=httpPut(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters/"+clusterID+"?token="+adminKey,
					 to_string(updateRequest));
		ENSURE_EQUAL(updateResp.status,200,"Updating the cluster config should succeed");
		
		auto infoResp=httpGet(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters/"+clusterID+"?token="+adminKey);
		ENSURE_EQUAL(infoResp.status,200,"Cluster info request should succeed");
		ENSURE(!infoResp.body.empty());
		rapidjson::Document infoData;
		infoData.Parse(infoResp.body.c_str());
		ENSURE_CONFORMS(infoData,infoSchema);
		ENSURE_EQUAL(infoData["metadata"]["name"].GetString(),std::string("testcluster"),
					 "Cluster name should remain unchanged");
		ENSURE_EQUAL(infoData["metadata"]["owningOrganization"].GetString(),std::string("Department of the Interior"),
					 "Cluster organization should remain unchanged");
		ENSURE_EQUAL(infoData["metadata"]["location"].GetArray().Size(),2,"Locations array should have two entries");
		ENSURE_EQUAL(infoData["metadata"]["location"][0]["lat"].GetDouble(),22.7,"First location should have correct lattitude");
		ENSURE_EQUAL(infoData["metadata"]["location"][0]["lon"].GetDouble(),-68,"First location should have correct longitude");
		ENSURE_EQUAL(infoData["metadata"]["location"][1]["lat"].GetDouble(),54.66,"Second location should have correct lattitude");
		ENSURE_EQUAL(infoData["metadata"]["location"][1]["lon"].GetDouble(),-87.2,"Second location should have correct longitude");
	}
}

TEST(MalformedUpdateRequests){
	using namespace httpRequests;
	TestContext tc;
	
	std::string adminKey=getPortalToken();
	//create VO to register cluster with
	rapidjson::Document createVO(rapidjson::kObjectType);
	{
		auto& alloc = createVO.GetAllocator();
		createVO.AddMember("apiVersion", currentAPIVersion, alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("name", "testvo1", alloc);
		metadata.AddMember("scienceField", "Logic", alloc);
		createVO.AddMember("metadata", metadata, alloc);
	}
	auto voResp=httpPost(tc.getAPIServerURL()+"/"+currentAPIVersion+"/vos?token="+adminKey,
			     to_string(createVO));
	ENSURE_EQUAL(voResp.status,200,"VO creation request should succeed");
	rapidjson::Document voData;
	voData.Parse(voResp.body.c_str());
	auto voID=voData["metadata"]["id"].GetString();

	auto kubeConfig=tc.getKubeConfig();
	
	//create cluster
	rapidjson::Document request1(rapidjson::kObjectType);
	{
		auto& alloc = request1.GetAllocator();
		request1.AddMember("apiVersion", currentAPIVersion, alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("name", "testcluster", alloc);
		metadata.AddMember("vo", rapidjson::StringRef(voID), alloc);
		metadata.AddMember("organization", "Department of Labor", alloc);
		metadata.AddMember("kubeconfig", rapidjson::StringRef(kubeConfig), alloc);
		request1.AddMember("metadata", metadata, alloc);
	}
	auto createResp=httpPost(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters?token="+adminKey,
				 to_string(request1));
	ENSURE_EQUAL(createResp.status,200,
		     "Cluster creation request should succeed");
	ENSURE(!createResp.body.empty());
	rapidjson::Document createData;
	createData.Parse(createResp.body.c_str());
	auto clusterID=createData["metadata"]["id"].GetString();

	auto clusterUrl=tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters/"+clusterID+"?token="+adminKey;
	
	{ //invalid JSON request body
		auto updateResp=httpPut(clusterUrl,"This is not JSON");
		ENSURE_EQUAL(updateResp.status,400,"Invalid JSON requests should be rejected");
	}
	{ //empty JSON
		rapidjson::Document request(rapidjson::kObjectType);
		auto updateResp=httpPut(clusterUrl,to_string(request));
		ENSURE_EQUAL(updateResp.status,400,"Empty JSON requests should be rejected");
	}
	{ //missing metadata
		rapidjson::Document request(rapidjson::kObjectType);
		auto& alloc = request.GetAllocator();
		request.AddMember("apiVersion", currentAPIVersion, alloc);
		auto updateResp=httpPut(clusterUrl, to_string(request));
		ENSURE_EQUAL(updateResp.status,400,"Requests without metadata should be rejected");
	}
	{ //wrong kubeconfig type
		rapidjson::Document request(rapidjson::kObjectType);
		auto& alloc = request.GetAllocator();
		request.AddMember("apiVersion", currentAPIVersion, alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("kubeconfig", 15, alloc);
		auto updateResp=httpPut(clusterUrl, to_string(request));
		ENSURE_EQUAL(updateResp.status,400,"Requests with invalid kubeconfig should be rejected");
	}
	{ //wrong organization type
		rapidjson::Document request(rapidjson::kObjectType);
		auto& alloc = request.GetAllocator();
		request.AddMember("apiVersion", currentAPIVersion, alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("owningOrganization", true, alloc);
		auto updateResp=httpPut(clusterUrl, to_string(request));
		ENSURE_EQUAL(updateResp.status,400,"Requests with invalid owningOrganization should be rejected");
	}
	{ //wrong location type
		rapidjson::Document request(rapidjson::kObjectType);
		auto& alloc = request.GetAllocator();
		request.AddMember("apiVersion", currentAPIVersion, alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("location", "The Moon", alloc);
		auto updateResp=httpPut(clusterUrl, to_string(request));
		ENSURE_EQUAL(updateResp.status,400,"Requests with invalid location should be rejected");
	}

}

TEST(UpdateClusterForVONotMemberOf){
	using namespace httpRequests;
	TestContext tc;
	
	std::string adminKey=getPortalToken();
	auto createClusterUrl=tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters?token="+adminKey;

	// create VO to register cluster with
	rapidjson::Document createVO(rapidjson::kObjectType);
	{
		auto& alloc = createVO.GetAllocator();
		createVO.AddMember("apiVersion", currentAPIVersion, alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("name", "testvo1", alloc);
		metadata.AddMember("scienceField", "Logic", alloc);
		createVO.AddMember("metadata", metadata, alloc);
	}
	auto voResp=httpPost(tc.getAPIServerURL()+"/"+currentAPIVersion+"/vos?token="+adminKey,
			     to_string(createVO));
	ENSURE_EQUAL(voResp.status,200,"VO creation request should succeed");
	ENSURE(!voResp.body.empty());
	rapidjson::Document voData;
	voData.Parse(voResp.body.c_str());
	auto voID=voData["metadata"]["id"].GetString();	

	auto kubeConfig = tc.getKubeConfig();
	
	//create cluster
	rapidjson::Document request1(rapidjson::kObjectType);
	{
		auto& alloc = request1.GetAllocator();
		request1.AddMember("apiVersion", currentAPIVersion, alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("name", "testcluster", alloc);
		metadata.AddMember("vo", rapidjson::StringRef(voID), alloc);
		metadata.AddMember("organization", "Department of Labor", alloc);
		metadata.AddMember("kubeconfig", rapidjson::StringRef(kubeConfig), alloc);
		request1.AddMember("metadata", metadata, alloc);
	}
	auto createResp=httpPost(createClusterUrl, to_string(request1));
	ENSURE_EQUAL(createResp.status,200,
		     "Cluster creation request should succeed");

	ENSURE(!createResp.body.empty());
	rapidjson::Document createData;
	createData.Parse(createResp.body.c_str());
	auto clusterID=createData["metadata"]["id"].GetString();

	//create second user that isn't part of the created VO
	rapidjson::Document createUser(rapidjson::kObjectType);
	{
		auto& alloc = createUser.GetAllocator();
		createUser.AddMember("apiVersion", currentAPIVersion, alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("name", "Bob", alloc);
		metadata.AddMember("email", "bob@place.com", alloc);
		metadata.AddMember("phone", "555-5555", alloc);
		metadata.AddMember("institution", "Center of the Earth University", alloc);
		metadata.AddMember("admin", false, alloc);
		metadata.AddMember("globusID", "Bob's Globus ID", alloc);
		createUser.AddMember("metadata", metadata, alloc);
	}
	auto userResp=httpPost(tc.getAPIServerURL()+"/"+currentAPIVersion+"/users?token="+adminKey,
			       to_string(createUser));
	ENSURE_EQUAL(userResp.status,200,
		     "User creation request should succeed");
	ENSURE(!userResp.body.empty());
	rapidjson::Document userData;
	userData.Parse(userResp.body.c_str());
	auto userID=userData["metadata"]["id"].GetString();

	//try to update the kubeconfig for the created cluster
	rapidjson::Document updateRequest(rapidjson::kObjectType);
	{
		auto& alloc = updateRequest.GetAllocator();
		updateRequest.AddMember("apiVersion", currentAPIVersion, alloc);
	        rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("kubeconfig", tc.getKubeConfig()+"\n\n\n", alloc);
		updateRequest.AddMember("metadata", metadata, alloc);
	}
	
	auto updateResp=httpPut(tc.getAPIServerURL()+"/"+currentAPIVersion+"/clusters/"+clusterID+"?token="+userID,
				to_string(updateRequest));
	ENSURE_EQUAL(updateResp.status,403,
		     "User who is not part of the VO owning the cluster should not be able to update the cluster");
}
