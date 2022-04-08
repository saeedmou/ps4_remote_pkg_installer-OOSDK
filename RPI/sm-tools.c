#include "sm-tools.h"
#include "common.h"
#include "stdio.h"
#include <dirent.h> // for directory
#include "KPutil.h" //for notify
#include "util.h"

typedef char* handler_callback(sb_Stream* s, json_t* root);
struct route_desc {
	const char* path;
	handler_callback* handler;
	bool need_partial_match;
};
union json_value_ts {
	const json_t* jval;
	const char* sval;
	int64_t ival;
};


static bool handle_get_info(sb_Stream* s, json_t* root);
static bool handle_get_version(sb_Stream* s, json_t* root);
static bool handle_get_endpoints(sb_Stream* s, json_t* root);
static bool handle_get_apps(sb_Stream* s, json_t* root);
static char check_directory();

static const struct route_desc routes_handlers[] = {
	{ "get-info", &handle_get_info, false },
	{ "get-version", &handle_get_version, false },
	{ "get-endpoints", &handle_get_endpoints, false },
	{ "get-apps", &handle_get_apps, false },
};

static void set_cors_header(sb_Stream* s) {
	sb_send_header(s, "Content-Type", "application/json");
	sb_send_header(s, "Access-Control-Allow-Origin", "*");
	sb_send_header(s, "Access-Control-Allow-Methods", "*");
	sb_send_header(s, "Access-Control-Allow-Headers", "*");
}

static void kick_result_header_json(sb_Stream* s) {
	sb_send_status(s, 200, "OK");
	set_cors_header(s);
	sb_send_header(s, "Connection", "close");
}

char* sm_tools_main_handler(sb_Stream* s, json_t* root) {
	size_t count;
	int i;
	char* endpoint;
	handler_callback* handler = NULL;
	const json_t* field;
	union json_value_ts val;
	char* output[200];

	KernelPrintOut("\r\n***** Handle BY [sm_tools] *****\r\n");
	KernelPrintOut("[sm_tools] request path: (%s)\r\n","test");
	count = ARRAY_SIZE(routes_handlers);
	KernelPrintOut("[sm_tools] Routes Number: (%d)\r\n", count);

	field = json_getProperty(root, "endpoint");
	if (!field) {
		kick_result_header_json(s);
		sb_writef(s, "{ \"status\": \"error\",\n \"message\": \"%s\" }\n", "No 'endpoint' parameter specified.");
		return "\"No endpoint' parameter specified.\"";
	}
	if (json_getType(field) != JSON_TEXT) {
		kick_result_header_json(s);
		sb_writef(s, "{ \"status\": \"error\",\n \"message\": \"%s\" }\n", "Invalid type for parameter 'endpoint'.");
		return("Invalid type for parameter 'endpoint'.");
	}
	val.sval = json_getValue(field);
	endpoint = val.sval;
	KernelPrintOut("[sm_tools] Endpoint: (%s)\r\n", endpoint);

	for (i = 0; i < count; ++i) {
		if (routes_handlers[i].need_partial_match) {
			if (strstr(endpoint, routes_handlers[i].path) == endpoint) {
				handler = routes_handlers[i].handler;
				break;
			}
		}
		else {
			if (strcmp(endpoint, routes_handlers[i].path) == 0) {
				handler = routes_handlers[i].handler;
				break;
			}
		}
	}

	if (!handler) {
		kick_result_header_json(s);
		sb_writef(s, "{ \"status\": \"error\",\n \"message\": \"%s\" ,\n \"request_endpoint\": \"%s\",\n \"handler\":\"sm-tools\"}\n", "No endpoint for this request", endpoint);
		return "No endpoint for this request";
	}

	return (*handler)(s,root);
}

static bool handle_get_info(sb_Stream* s, json_t* root) {
	kick_result_header_json(s);
	sb_writef(s, "{ \"status\": \"success\",\n \"message\": \"%s\" ,\n \"handler\":\"sm-tools\"}\n", "information is ready");
	return true;
}

static bool handle_get_version(sb_Stream* s,json_t* root) {
	kick_result_header_json(s);
	sb_writef(s, "{ \"status\": \"success\",\n \"message\": \"%s\" ,\n \"version\": \"%s\" ,\n \"handler\":\"sm-tools\"}\n", "see version","1.0");
	return true;
}

static bool handle_get_endpoints(sb_Stream* s,json_t* root) {
	char* output="";
	char* endpoint;
	size_t count;
	int i;
	char res[100] = "";

	count = ARRAY_SIZE(routes_handlers);
	kick_result_header_json(s);
	sb_writef(s, "{ \"status\": \"success\",\n \"message\": \"%s\" ,\n \"endpoints\": [", "see endpoints");
	for (i = 0; i < count; ++i) {
		if (i < count - 1) {
			sb_writef(s, "\"%s\",", routes_handlers[i].path);
		}
		else {
			sb_writef(s, "\"%s\"", routes_handlers[i].path);

		}
	}
	sb_writef(s, "],\n \"handler\":\"sm-tools\"}\n");
	/*

	for (i = 0; i < count; ++i) {
		strcat(res, routes_handlers[i].path);
		if (i < count - 1) {
			strcat(res,",");
		}
	}
	kick_result_header_json(s);
	sb_writef(s, "{ \"status\": \"success\",\n \"message\": \"%s\" ,\n \"endpoints\": [%s],\n \"handler\":\"sm-tools\"}\n", "see endpoints", res);
*/
	//KernelPrintOut("[sm_tools] output: (%s)\r\n", output);
	return true;

}

static bool handle_get_apps(sb_Stream* s, json_t* root) {
	char* ret = check_directory();
	KernelPrintOut("%s\n", "After return");



	kick_result_header_json(s);
	sb_writef(s, "{ \"status\": \"success\",\n \"message\": \"%s\" ,\n \"CUSA_list\": [%s],\n \"handler\":\"sm-tools\"}\n", "see app list", ret);
	return true;
}

static char check_directory() {
	DIR* dir = opendir("/user/app");
	struct dirent* entry;
	int found = 0;
	char* res_cusas[512];
	if (!dir) {
		return 1;
	}

	// Loop over all entries in directory
	while ((entry = readdir(dir)) != NULL) {
		KernelPrintOut("[sm_tools] FIle #%d\nName= %s\nType=%d\n", found, entry->d_name, entry->d_type);
		if (entry->d_type == DT_DIR ) {
			KernelPrintOut("it is Directory.\n");
		}
		if (entry->d_type == DT_REG) {
			KernelPrintOut("it is File\n");
		}
		if (ends_with(entry->d_name, ".json")) {
			KernelPrintOut("it is Json File\n");
		}
		if (ends_with(entry->d_name, ".pkg")) {
			KernelPrintOut("***it is PKG File***\n");
		}
		// If it's a directory OR a file not ending in .pkg, skip
		/*
		if (entry->d_type == DT_DIR || !ends_with(entry->d_name, ".pkg")) {
			continue;
		}
		*/

		
		if (entry->d_type != DT_DIR || ends_with(entry->d_name, "." || ends_with(entry->d_name, ".."))) {
			continue;
		}
		strcat(res_cusas, "\"");
		strcat(res_cusas, entry->d_name);
		strcat(res_cusas, "\",");
		

		//Notify("RPI-[sm-tools]\nFound: %s", entry->d_name);

		/*
		// Attempt to install PKG file
		if (install_pkg(entry->d_name)) {
			char errorMsg[100];
			snprintf(errorMsg, sizeof(errorMsg), "Cannot install:\n%s", entry->d_name);

			system_notification(errorMsg, "default_icon_download");
		}
		*/


		found++;
	}

	KernelPrintOut("\n\n%s\n\n", res_cusas);


	// No PKG files found
	//if (found == 0) {
	//	Notify("RPI-[sm-tools]\nFound: %d", 0);
	//}

	closedir(dir);
	KernelPrintOut("%s\n", "After Close Dir");

	/*
		if (res_cusas != NULL)
	{
		const unsigned int length = strlen(res_cusas);
		if ((length > 0) && (res_cusas[length - 1] == '&')) res_cusas[length - 1] = '\0';
	}
	*/
	return res_cusas;
}

