#ifndef SERVER__H
#define SERVER__H
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib, "Ws2_32.lib")
#include <string>
#include <ctime>
#include <sstream>
#include <time.h>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <winsock2.h>
#include <sys/stat.h>
#include <direct.h>

const int BUFFER_SIZE = 1024;
const std::string ROOT_DIRECTORY = "C://temp/"; // default working directory     
const std::string DEFAULT_RESOURCE = "/index.html"; // default index

enum fileType {
	TEXT_HTML,
	TEXT_PLAIN,
	APPLICATION_JSON,
	IMAGE_JPEG,
	IMAGE_PNG,
	IMAGE_GIF,
	NOT_SUPPORTED
};


// Enum for HTTP methods
enum METHOD {
	GET,
	POST,
	PUT,
	DEL,
	HEAD,
	OPTION,
	TRACE,
	NOT_VALID
};

// Enum for supported languages
enum LANGUAGE {
	en,
	he,
	fr
};

// Struct for maintaining socket state
struct SocketState
{
	SOCKET id;                // Socket handle
	int recv;                 // Receiving?
	int send;                 // Sending?
	METHOD method;            // HTTP method
	std::string buffer;       // Data buffer
	std::time_t lastTimeUsed; // Last time used
};

// Struct for HTTP header data
struct HeaderData {
	std::string status;       // HTTP status
	std::string contentType;  // Content type
};

const int HTTP_PORT = 80;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int TIME_OUT = 120; // time in seconds
const timeval MAX_TIME_FOR_SELECT = { TIME_OUT , 0 }; // set for select will throw after TIME_OUT in seconds over.
struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;


bool addSocket(SOCKET id, int what);

void removeSocket(int index);

void acceptConnection(int index);

void receiveRequest(int index);

void sendResponse(int index);

// Processes an HTTP request
void processRequest(int index, METHOD methodType, int size, struct SocketState* sockets);

// Builds an HTTP response header
void buildHeader(std::string& sendMsg, const HeaderData& header);

// Handles an OPTIONS request
void optionsRequest(std::string& sendMsg);

// Sends an HTTP response
void sendResponse(int index);

// Handles a GET or HEAD request
void GetMethodHandler(std::string& response, std::string& socketBuffer, bool HEAD = false);

// Extracts resource and query string from a request
std::string getResourceAndQuery(const std::string& buffer, std::string& queryString);

// Parses a query string into key-value pairs
void parse_query_string(const std::string& queryString, std::unordered_map<std::string, std::string>& query_parameters);

// Handles the 'lang' parameter in the query string
void langParamHandler(LANGUAGE& language, std::unordered_map<std::string, std::string>& query_parameters);

// Creates a file path based on the language and resource path
std::string createFilePath(LANGUAGE language, const std::string& resourcePath);

// Determines if data is binary based on file type
bool isBinaryData(fileType dataType);

// Reads a file into a string
std::string read_file(std::ifstream& file);

// Creates an HTTP response header
void makeHeader(std::string& response, const std::string& status, const std::string& contentType);

// Creates an HTTP response body
void makeBody(std::string& response, const std::string& body, bool isError);

// Gets the content type string based on file type
std::string getContentTypeString(fileType type);

// Handles a POST request
void PostMethodHandler(std::string& sendMsg, const std::string& buffer);

// Trims whitespace from a string
std::string trim(const std::string& str);

// Handles a PUT request
void PutMethodHandler(std::string& sendMsg, const std::string& buffer);

// Handles a DELETE request
void DeleteMethodHandler(std::string& sendMsg, const std::string& buffer);

// Handles unsupported HTTP methods
void NotAllowMethodHandler(std::string& response);

// Handles a TRACE request
void TraceMethodHandler(std::string& response, std::string& socketBuffer);
#endif 