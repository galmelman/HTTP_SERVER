#include "server.h"
#include <iostream>

using namespace std;





void main()
{
	// Initialize Winsock
	WSAData wsaData;
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "HTTP Server: Error at WSAStartup()\n";
		return;
	}

	// Create a listening socket
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == listenSocket)
	{
		cout << "HTTP Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	// Set up the server address structure
	sockaddr_in serverService;
	serverService.sin_family = AF_INET;
	serverService.sin_addr.s_addr = INADDR_ANY;
	serverService.sin_port = htons(HTTP_PORT);

	// Bind the listening socket to the address
	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR*)&serverService, sizeof(serverService)))
	{
		cout << "HTTP Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	// Start listening for incoming connections
	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "HTTP Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	addSocket(listenSocket, LISTEN);

	// Main server loop
	while (true)
	{
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		int nfd = select(0, &waitRecv, &waitSend, NULL, &MAX_TIME_FOR_SELECT);
		if (nfd == SOCKET_ERROR)
		{
			cout << "HTTP Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		// Handle incoming connections and data
		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;
				case RECEIVE:
					receiveRequest(i);
					break;
				}
			}
		}

		// Handle outgoing data
		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendResponse(i);
					break;
				}
			}
		}

		// Timeout handling for open sockets
		if (socketsCount > 1)
		{
			int socketsLeft = socketsCount - 1;
			time_t currTime;
			time(&currTime);
			time_t timeToClose = currTime - TIME_OUT;
			for (int i = 1; i < MAX_SOCKETS && socketsLeft != 0; i++)
			{
				if (sockets[i].id != 0 && sockets[i].lastTimeUsed <= timeToClose && sockets[i].lastTimeUsed != 0)
				{
					closesocket(sockets[i].id);
					removeSocket(i);
					socketsLeft -= 1;
				}
				else if (sockets[i].id != 0)
					socketsLeft -= 1;
			}
		}
	}
}

// Adds a new socket to the list of managed sockets
bool addSocket(SOCKET id, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

// Removes a socket from the list of managed sockets
void removeSocket(int index)
{
	cout << "Connection close for socket " << sockets[index].id << endl;
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	sockets[index].id = 0;
	sockets[index].lastTimeUsed = 0;
	socketsCount--;
}

// Accepts a new incoming connection
void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr*)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "HTTP Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "HTTP Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	unsigned long flag = 1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout << "HTTP Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}

	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

// Receives data from a socket
void receiveRequest(int index)
{
	SOCKET msgSocket = sockets[index].id;
	char buffer[BUFFER_SIZE];
	int bytesRecv = recv(msgSocket, buffer, BUFFER_SIZE - 1, 0);
	buffer[bytesRecv] = '\0';
	sockets[index].buffer = buffer;

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "HTTP Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	else
	{
		cout << "HTTP Server: Recieved: " << bytesRecv << " bytes of \"" << sockets[index].buffer << "\" message.\n";

		if (sockets[index].buffer.length() > 0)
		{
			if (strncmp(sockets[index].buffer.c_str(), "OPTIONS", 7) == 0)
			{
				processRequest(index, OPTION, 7, sockets);
			}
			else if (strncmp(sockets[index].buffer.c_str(), "GET", 3) == 0)
			{
				processRequest(index, GET, 3, sockets);
			}
			else if (strncmp(sockets[index].buffer.c_str(), "HEAD", 4) == 0)
			{
				processRequest(index, HEAD, 4, sockets);
			}
			else if (strncmp(sockets[index].buffer.c_str(), "POST", 4) == 0)
			{
				processRequest(index, POST, 4, sockets);
			}
			else if (strncmp(sockets[index].buffer.c_str(), "PUT", 3) == 0)
			{
				processRequest(index, PUT, 3, sockets);
			}
			else if (strncmp(sockets[index].buffer.c_str(), "DELETE", 6) == 0)
			{
				processRequest(index, DEL, 6, sockets);
			}
			else if (strncmp(sockets[index].buffer.c_str(), "TRACE", 5) == 0)
			{
				processRequest(index, TRACE, 5, sockets);
			}
			else
			{
				processRequest(index, NOT_VALID, 0, sockets);
			}
		}
	}

}

// Sends a response to a socket
void sendResponse(int index)
{

	bool closeConnection = false;

	size_t bytesSent = 0;
	SOCKET currSocket = sockets[index].id;
	METHOD method = sockets[index].method;
	string sendMsg;


	switch (method)
	{
	case PUT:
		PutMethodHandler(sendMsg, sockets[index].buffer);
		break;
	case DEL:
		DeleteMethodHandler(sendMsg, sockets[index].buffer);
		break;
	case GET:
		GetMethodHandler(sendMsg, sockets[index].buffer);
		break;
	case HEAD:
		GetMethodHandler(sendMsg, sockets[index].buffer,true);
		break;
	case OPTION:
		optionsRequest(sendMsg);
		break;
	case TRACE:
		TraceMethodHandler(sendMsg, sockets[index].buffer);
		break;
	case NOT_VALID:
		NotAllowMethodHandler(sendMsg);
		closeConnection = true;
		break;
	case POST:
		PostMethodHandler(sendMsg, sockets[index].buffer);
		break;
	default:
		break;
	}

	bytesSent = send(currSocket, sendMsg.c_str(), sendMsg.length(), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "HTTP Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "HTTP Server: Sent: " << bytesSent << "\\" << sendMsg.length() << " bytes of \"\n" << sendMsg << "\" message.\n";

	time_t currectTime;
	time(&currectTime);
	sockets[index].lastTimeUsed = currectTime; // update the last time used.

	if (closeConnection)
	{
		closesocket(currSocket);
		removeSocket(index);
	}
	else
	{
		sockets[index].send = IDLE;
		sockets[index].buffer.clear();
	}
}

// Processes an HTTP request
void processRequest(int index, METHOD methodType, int size, SocketState* sockets)
{
	// Set the socket send state to SEND and assign the method type to the socket
	sockets[index].send = SEND;
	sockets[index].method = methodType;
	if (size <= 0)
		return;
	// Remove the processed part of the buffer
	sockets[index].buffer = sockets[index].buffer.substr(size);
}

// Handles an OPTIONS request
void optionsRequest(string& sendMsg)
{
	HeaderData header;
	header.contentType = "204 No Content"; // No content response for OPTIONS request
	header.status = "text/html";

	// Build the header for the OPTIONS response
	buildHeader(sendMsg, header);

	// Add Allow line to the response indicating supported methods
	sendMsg += "Allow: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE\r\n";
	// Add Content-Length to the response indicating no content
	sendMsg += "Content-Length: 0\r\n\r\n";
}

// Builds an HTTP response header
void buildHeader(std::string& sedMsg, const HeaderData& header) {
	time_t t;
	time(&t);
	tm* utc_time = gmtime(&t);
	std::string foramtTIme(80, '\0');
	strftime(&foramtTIme[0], foramtTIme.size(), "%a, %d %b %Y %H:%M:%S GMT", utc_time);
	foramtTIme.resize(strlen(foramtTIme.c_str()));
	std::string lineSuffix("\r\n");
	sedMsg = "HTTP/1.1 " + header.status + lineSuffix;
	sedMsg += "Server: HTTP Web Server" + lineSuffix;
	sedMsg += "Content-Type: " + header.contentType + lineSuffix;
	sedMsg += "Date: " + std::string(ctime(&t));
}

// Extracts resource and query string from a request
string getResourceAndQuery(const string& buffer, string& queryString) {
	// Find the start and end positions of the resource path
	size_t startPos = buffer.find(" ");
	size_t endPos = buffer.find(" ", startPos + 1);

	// Extract the resource path from the buffer
	string resource = buffer.substr(startPos + 1, endPos - startPos - 1);

	// Find the position of the query string (if any)
	size_t queryPos = resource.find("?");

	// If a query string is found, extract it and update the resource path
	if (queryPos != string::npos) {
		queryString = resource.substr(queryPos + 1);
		resource = resource.substr(0, queryPos);
	}

	// Return the resource path
	return resource;
}

// Parses a query string into key-value pairs
void parse_query_string(const string& queryString, unordered_map<string, string>& query_parameters) {
	// Initialize position variables
	size_t pos = 0, end;

	// Iterate over the query string to extract key-value pairs
	while ((end = queryString.find("&", pos)) != string::npos) {
		// Extract a key-value pair
		string pair = queryString.substr(pos, end - pos);
		size_t eqPos = pair.find("=");

		// If a valid key-value pair is found, add it to the map
		if (eqPos != string::npos) {
			query_parameters[pair.substr(0, eqPos)] = pair.substr(eqPos + 1);
		}

		// Update the position to continue parsing
		pos = end + 1;
	}

	// Handle the last key-value pair (if any)
	if (pos < queryString.length()) {
		string pair = queryString.substr(pos);
		size_t eqPos = pair.find("=");

		// If a valid key-value pair is found, add it to the map
		if (eqPos != string::npos) {
			query_parameters[pair.substr(0, eqPos)] = pair.substr(eqPos + 1);
		}
	}
}


// Handles the 'lang' parameter in the query string
void langParamHandler(LANGUAGE& language, unordered_map<string, string>& query_parameters) {
	auto it = query_parameters.find("lang");
	if (it != query_parameters.end()) {
		if (it->second == "fr") {
			language = fr;
		}
		else if (it->second == "he") {
			language = he;
		}
		else {
			language = en;
		}
	}
}

// Creates a file path based on the language and resource path
string createFilePath(LANGUAGE language, const string& resourcePath) {
	string langPath;
	if (language == fr) {
		langPath = "/fr";
	}
	else if (language == he) {
		langPath = "/he";
	}
	else
	{
		langPath = "/en";
	}
	return langPath + resourcePath;
}

// Helper function to get the content type string
string getContentTypeString(fileType type) {
	switch (type) {
	case fileType::TEXT_HTML:
		return "text/html";
	case fileType::TEXT_PLAIN:
		return "text/plain";
	case fileType::APPLICATION_JSON:
		return "application/json";
	case fileType::IMAGE_JPEG:
		return "image/jpeg";
	case fileType::IMAGE_PNG:
		return "image/png";
	case fileType::IMAGE_GIF:
		return "image/gif";
	case fileType::NOT_SUPPORTED:
		return "application/octet-stream";
	default:
		return "application/octet-stream";
	}
}

// Determines if data is binary based on file type
bool isBinaryData(fileType dataType) {
	return (dataType == IMAGE_JPEG || dataType == IMAGE_PNG || dataType == APPLICATION_JSON);
}

// Reads a file into a string
string read_file(ifstream& file) {
	stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

// Creates an HTTP response header
void makeHeader(string& sendMsg, const string& status, const string& contentType) {
	sendMsg = "HTTP/1.1 " + status + "\r\n";
	sendMsg += "Content-Type: " + contentType + "\r\n";
	sendMsg += "Connection: close\r\n";
	sendMsg += "Server: HTTP Web Server\r\n";
}

// Creates an HTTP response body
void makeBody(string& sendMsg, const string& body, bool isError) {
	sendMsg += "Content-Length: " + to_string(body.length()) + "\r\n\r\n";
	sendMsg += body;
}

// Helper function to trim whitespace from both ends of a string
string trim(const string& str) {
	size_t start = str.find_first_not_of(" \t\r\n");
	size_t end = str.find_last_not_of(" \t\r\n");
	return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

// Handles a POST request
void PostMethodHandler(string& sendMsg, const string& buffer) {
	// Find the start of the body in the buffer
	size_t bodyStart = buffer.find("\r\n\r\n");
	if (bodyStart == std::string::npos) {
		makeHeader(sendMsg, "400 Bad Request", "text/plain");
		makeBody(sendMsg, "Could not find request body", true);
		return;
	}

	// Parse headers
	std::unordered_map<std::string, std::string> headers;
	std::istringstream headerStream(buffer.substr(0, bodyStart));
	std::string headerLine;
	while (std::getline(headerStream, headerLine) && headerLine != "\r") {
		size_t colonPos = headerLine.find(':');
		if (colonPos != std::string::npos) {
			std::string key = trim(headerLine.substr(0, colonPos));
			std::string value = trim(headerLine.substr(colonPos + 1));
			headers[key] = value;
		}
	}

	// Extract the body
	std::string body = buffer.substr(bodyStart + 4);
	std::cout << "Received POST data: " << body << std::endl;

	// Parse query string
	std::string queryString;
	std::string resource = getResourceAndQuery(buffer, queryString);
	std::unordered_map<std::string, std::string> queryParams;
	parse_query_string(queryString, queryParams);

	// Handle language
	LANGUAGE lang = en;
	auto langIt = queryParams.find("lang");
	if (langIt != queryParams.end()) {
		if (langIt->second == "fr") lang = fr;
		else if (langIt->second == "he") lang = he;
	}


	// Create file path
	string filePath = ROOT_DIRECTORY + createFilePath(lang, resource);


	// Check if file already exists
	struct stat fileInfo;
	if (stat(filePath.c_str(), &fileInfo) == 0) {
		makeHeader(sendMsg, "409 Conflict", "text/plain");
		makeBody(sendMsg, "Resource already exists", true);
		return;
	}

	// Create directories if they don't exist
	std::string dirPath = filePath.substr(0, filePath.find_last_of("/\\"));
	if (_mkdir(dirPath.c_str()) != 0 && errno != EEXIST) {
		makeHeader(sendMsg, "500 Internal Server Error", "text/plain");
		makeBody(sendMsg, "Failed to create directory", true);
		return;
	}

	
	// Determine if the file should be opened in binary mode
	bool isBinary = filePath.substr(filePath.find_last_of(".") + 1) == "bin";

	// Write body to file
	std::ofstream outFile;
	if (isBinary) {
		outFile.open(filePath, std::ios::binary);
	}
	else {
		outFile.open(filePath);
	}
	

	if (outFile.is_open()) {
		outFile.write(body.c_str(), body.length());
		outFile.close();

		makeHeader(sendMsg, "201 Created", "text/plain");
		makeBody(sendMsg, "Resource created successfully", false);
	}
	else {
		makeHeader(sendMsg, "500 Internal Server Error", "text/plain");
		makeBody(sendMsg, "Failed to create resource", true);
	}
}


 // Handles HTTP GET and HEAD requests by parsing `socketBuffer`, fetching the resource, and storing the response in `sendMsg`  
 // includes only headers if `HEAD` is true.
void GetMethodHandler(string& sendMsg, string& socketBuffer, bool HEAD) {
	unordered_map<string, string> query_parameters;
	LANGUAGE language = en; // default language is English
	fileType dataType = TEXT_HTML;
	string queryString;
	ifstream file;
	string resourcePath = getResourceAndQuery(socketBuffer, queryString);

	if (resourcePath.length() <= 1)
		resourcePath = DEFAULT_RESOURCE;
	if (queryString.length() > 0) {
		parse_query_string(queryString, query_parameters); // Initialize query_parameters.
		langParamHandler(language, query_parameters); // Check if language query sent and handle it.
	}

	string filePath = ROOT_DIRECTORY + createFilePath(language, resourcePath);
	string contentType = getContentTypeString(dataType);
	if (isBinaryData(dataType))
		file.open(filePath, ios::binary);
	else
		file.open(filePath);

	if (!file.is_open()) {
		makeHeader(sendMsg, "404 Not Found", "text/html");
		makeBody(sendMsg, "<html><head><title>Error!</title></head><body><h1>404 Not Found</h1><p>The request could not be completed because the file was not found.</p></body></html>", true);
		return; // Do nothing if file doesn't exist
	}
	else {
		string file_in_str(read_file(file));
		if (HEAD) {
			makeHeader(sendMsg, "204 No Content", contentType);
			sendMsg += "\r\n";
		}
		else {
			makeHeader(sendMsg, "200 OK", contentType);
			makeBody(sendMsg, file_in_str, HEAD);
		}
	}
	file.close();
}

// Handles a PUT request
void PutMethodHandler(string& sendMsg, const string& buffer) {
	// Find the start of the body in the buffer
	size_t bodyStart = buffer.find("\r\n\r\n");
	if (bodyStart == string::npos) {
		makeHeader(sendMsg, "400 Bad Request", "text/plain");
		makeBody(sendMsg, "Could not find request body", true);
		return;
	}

	// Parse headers
	unordered_map< string, string> headers;
	istringstream headerStream(buffer.substr(0, bodyStart));
	string headerLine;
	while (getline(headerStream, headerLine) && headerLine != "\r") {
		size_t colonPos = headerLine.find(':');
		if (colonPos != string::npos) {
			string key = trim(headerLine.substr(0, colonPos));
			string value = trim(headerLine.substr(colonPos + 1));
			headers[key] = value;
		}
	}

	// Extract the body
	string body = buffer.substr(bodyStart + 4);
	cout << "Received PUT data: " << body << std::endl;

	// Parse query string
	string queryString;
	string resource = getResourceAndQuery(buffer, queryString);
	unordered_map<string, string> queryParams;
	parse_query_string(queryString, queryParams);

	// Handle language
	LANGUAGE lang = en;
	auto langIt = queryParams.find("lang");
	if (langIt != queryParams.end()) {
		if (langIt->second == "fr") lang = fr;
		else if (langIt->second == "he") lang = he;
	}

	// Create file path
	string filePath = ROOT_DIRECTORY + createFilePath(lang, resource);

	// Check if file exists
	struct stat fileInfo;
	if (stat(filePath.c_str(), &fileInfo) != 0) {
		makeHeader(sendMsg, "404 Not Found", "text/plain");
		makeBody(sendMsg, "Resource does not exist", true);
		return;
	}

	// Determine if the file should be opened in binary mode
	bool isBinary = filePath.substr(filePath.find_last_of(".") + 1) == "bin";

	// Write body to file (overwrite existing content)
	std::ofstream outFile;
	if (isBinary) {
		outFile.open(filePath, std::ios::binary | std::ios::trunc);
	}
	else {
		outFile.open(filePath, std::ios::trunc);
	}

	if (outFile.is_open()) {
		outFile.write(body.c_str(), body.length());
		outFile.close();

		makeHeader(sendMsg, "200 OK", "text/plain");
		makeBody(sendMsg, "Resource updated successfully", false);
	}
	else {
		makeHeader(sendMsg, "500 Internal Server Error", "text/plain");
		makeBody(sendMsg, "Failed to update resource", true);
	}
}

// Handles a DELETE request
void DeleteMethodHandler(string& sendMsg, const string& buffer) {
	// Parse query string to extract parameters
	string queryString;
	string resourcePath = getResourceAndQuery(buffer, queryString);

	// Parse query parameters to get language
	unordered_map<string, string> queryParameters;
	parse_query_string(queryString, queryParameters);

	// Determine language from query parameters
	LANGUAGE lang = en; // Default language is English
	auto langIt = queryParameters.find("lang");
	if (langIt != queryParameters.end()) {
		if (langIt->second == "fr") {
			lang = fr;
		}
		else if (langIt->second == "he") {
			lang = he;
		}
	}

	// Create the full file path based on the server's root directory and the resource path
	string filePath = ROOT_DIRECTORY + createFilePath(lang, resourcePath);

	// Check if the file exists
	struct stat fileInfo;
	if (stat(filePath.c_str(), &fileInfo) != 0) {
		// File does not exist, return a 404 Not Found response
		makeHeader(sendMsg, "404 Not Found", "text/plain");
		makeBody(sendMsg, "Resource does not exist", true);
		return;
	}

	// Attempt to delete the file
	if (remove(filePath.c_str()) != 0) {
		// If deletion failed, return a 500 Internal Server Error response
		makeHeader(sendMsg, "500 Internal Server Error", "text/plain");
		makeBody(sendMsg, "Failed to delete resource", true);
		return;
	}

	// If deletion was successful, return a 200 OK response
	makeHeader(sendMsg, "200 OK", "text/plain");
	makeBody(sendMsg, "Resource deleted successfully", false);
}

// Handles unsupported HTTP methods
void NotAllowMethodHandler(string& response)
{
	string contentType("text/html");
	string status("405 Method Not Allowed");

	makeHeader(response, status, contentType);

	// Add Allow line to response
	response += "Allow: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE\r\n";
	// Add Content-Length to response
	response += "Content-Length: 0\r\n";

	response += "Connection: close\r\n\r\n";
}

// Handles a TRACE request
void TraceMethodHandler(string& response, string& socketBuffer)
{
	string status("200 OK");
	string contentType("message/http");
	string EchoString = string("TRACE") + socketBuffer;

	makeHeader(response, status, contentType); // make header.
	makeBody(response, EchoString, false); // add body
}

;
