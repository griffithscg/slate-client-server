#include "ApplicationCommands.h"

#include "Logging.h"
#include "Utilities.h"

crow::response listApplications(PersistentStore& store, const crow::request& req){
	const User user=authenticateUser(store, req.url_params.get("token"));
	log_info(user << " requested to list applications");
	if(!user)
		return crow::response(403,generateError("Not authorized"));
	//All users are allowed to list applications
	
	crow::json::wvalue result;
	result["apiVersion"]="v1alpha1";
	//TODO: compose actual result list
	result["items"]=std::vector<std::string>{};
	return crow::response(result);
}

crow::response fetchApplicationConfig(PersistentStore& store, const crow::request& req, const std::string& appID){
	const User user=authenticateUser(store, req.url_params.get("token"));
	log_info(user << " requested to fetch an application config");
	if(!user)
		return crow::response(403,generateError("Not authorized"));
	//TODO: Can all users obtain configurations for all applications?
	
	//TODO: implement this
	
	crow::json::wvalue result;
	result["apiVersion"]="v1alpha1";
	result["kind"]="Configuration";
	crow::json::wvalue metadata;
	//metadata["name"]=appName;
	result["metadata"]=std::move(metadata);
	crow::json::wvalue spec;
	//spec["body"]=appName;
	result["spec"]=std::move(spec);
	return crow::response(result);
}

crow::response installApplication(PersistentStore& store, const crow::request& req, const std::string& appID){
	const User user=authenticateUser(store, req.url_params.get("token"));
	log_info(user << " requested to install an instance of " << appID);
	if(!user)
		return crow::response(403,generateError("Not authorized"));
	VO vo=validateVO(req.url_params.get("vo"));
	if(!vo)
		return crow::response(400,generateError("Invalid VO"));
	Cluster cluster=validateCluster(req.url_params.get("cluster"));
	if(!cluster)
		return crow::response(400,generateError("Invalid cluster"));
	//TODO: check that user is allowed to install on behalf of vo to cluster
	
	crow::json::rvalue body;
	try{
		body = crow::json::load(req.body);
	}catch(std::runtime_error& err){
		return crow::response(400,generateError("Invalid JSON in request body"));
	}
	if(!body)
		return crow::response(400,generateError("Invalid JSON in request body"));
	if(!body.has("configuration"))
		return crow::response(400,generateError("Missing configuration"));
	if(body["configuration"].t()!=crow::json::type::String)
		return crow::response(400,generateError("Incorrect type for configuration"));
	const std::string config=body["configuration"].s();
	
	//TODO: do something useful
	return crow::response(crow::json::wvalue());
}