/*
 * Main Header file
 */

extern char *exCgiDirectory;
extern char *homeDirectory;
extern int debugMode;
extern char *ipAddress;
extern char *logFile;
extern int port;
extern int cgiEnable;

int Listen();

enum MessageType
{
	UNKNOWN,
	REQUEST,
	RESPONSE
};

enum Method
{
	GET,
	HEAD,
	UNSUPPORTED
};

struct Request
{
	enum Method method;

	char* URI;
	int IfModifiedSince;
};

struct Entity
{
    char *content;
    char *content_type;
    int content_length;
    int isCGI;
    char *last_modified_date;
};

struct Response
{
	int status_code;
	char* code_reason;
	struct Entity entity;

};

struct Message
{
	enum MessageType type;
	union {
		struct Request req;
		struct Response res;
	};
};


