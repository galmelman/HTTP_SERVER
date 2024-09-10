# HTTP Server Project

## Overview

This project implements a basic HTTP server in C++ using Winsock. The server is capable of handling multiple concurrent connections and supports various HTTP methods including GET, POST, PUT, DELETE, HEAD, OPTIONS, and TRACE.

## Features

- Multi-client support using select() for non-blocking I/O
- Support for HTTP methods: GET, POST, PUT, DELETE, HEAD, OPTIONS, TRACE
- Basic routing and resource handling
- Multi-language support (English, French, Hebrew)
- Timeout handling for inactive connections
- Binary and text file support

## Requirements

- Windows operating system
- C++ compiler with C++11 support
- Winsock2 library

## Configuration

Before running the server, make sure to set the following constants in `server.h`:

- `ROOT_DIRECTORY`: The root directory for serving files (default: "C://temp/")
- `DEFAULT_RESOURCE`: The default resource to serve if none is specified (default: "/index.html")
- `HTTP_PORT`: The port on which the server will listen (default: 80)
- `MAX_SOCKETS`: Maximum number of concurrent connections (default: 60)
- `TIME_OUT`: Timeout in seconds for inactive connections (default: 120)

## Compilation

To compile the project, use the following command:

```
g++ -o http_server server.cpp -lws2_32
```

Make sure to link against the Winsock2 library (-lws2_32).

## Usage

1. Run the compiled executable:

```
./http_server
```

2. The server will start and listen on the specified HTTP_PORT.

3. Connect to the server using a web browser or an HTTP client like cURL:

```
http://localhost/
```

## Supported HTTP Methods

### GET
Retrieves a resource from the server.

### POST
Creates a new resource on the server.

### PUT
Updates an existing resource on the server.

### DELETE
Removes a resource from the server.

### HEAD
Similar to GET but returns only the headers, not the body.

### OPTIONS
Returns the HTTP methods supported by the server.

### TRACE
Echoes back the received request for debugging purposes.

## Multi-language Support

The server supports serving content in different languages. Use the `lang` query parameter to specify the desired language:

- English (default): `?lang=en`
- French: `?lang=fr`
- Hebrew: `?lang=he`

Example: `http://localhost/index.html?lang=fr`

## Error Handling

The server implements basic error handling, including:

- 404 Not Found for non-existent resources
- 400 Bad Request for malformed requests
- 405 Method Not Allowed for unsupported HTTP methods
- 500 Internal Server Error for server-side issues



## Contributing

Contributions to improve the server are welcome. Please submit pull requests or open issues to discuss proposed changes.

