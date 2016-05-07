/* J. David's webserver
 *
 * This is a simple webserver.
 * Created November 1999 by J. David Blackstone.
 * CSE 4344 (Network concepts), Prof. Zeigler
 * University of Texas at Arlington
 *
 * Modified by Adam Feinstein, 2015
 */

#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "httpd.h"
#include "webpages.h"
#include "Qik2s9v1.h"

int32_t startup(uint16_t*);
void* accept_request(void* clientPtr);
void execute_cgi(int32_t, const char*, const char*, const char*);
void serve_file(int32_t, const char*);
int32_t get_line(int32_t, char*, int32_t);
void error_die(const char*);

/**********************************************************************/
/* TODO write something */
/**********************************************************************/
void* httpdMain(void* vp)
{
    int32_t server_sock = -1;
    int32_t client_sock = -1;
    uint16_t port = *((uint16_t*)vp);

    struct sockaddr_in client_name;
    uint32_t client_name_len = sizeof(client_name);

    pthread_t accept_request_thread;

    server_sock = startup(&port);
    printf("httpd running on port %d\n", port);

    /* Spin around forever, waiting for incoming requests */
    while (1)
    {
        /* wait for a request, this is blocking */
        client_sock = accept(server_sock, (struct sockaddr*) &client_name, &client_name_len);

        if (client_sock == -1)
        {
            error_die("accept");
        }

        /* Accept the request, create a thread to handle it, go back to waiting
         * TODO is it kosher to reuse the thread handle?
         */
        if (pthread_create(&accept_request_thread, NULL, accept_request, (void*) (&client_sock)) != 0)
        {
            perror("pthread_create");
        }
    }

    close(server_sock);

    return (0);
}

/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int32_t startup(uint16_t* port)
{
    int32_t httpdSocket = 0;
    struct sockaddr_in name;

    /* Create a socket, make sure it's valid */
    httpdSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (httpdSocket == -1)
    {
        error_die("socket");
    }

    /* Set up socket address */
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Assign the address to the socket
     * If the program didn't exit cleanly, it takes some time for
     * the system to unbind the port
     */
    while (bind(httpdSocket, (struct sockaddr*) &name, sizeof(name)) < 0)
    {
        printf("error binding\n");
        sleep(1);
    }

    /* if dynamically allocating a port */
    if (*port == 0)
    {
        /* Get the port the system assigned */
        uint32_t namelen = sizeof(name);
        if (getsockname(httpdSocket, (struct sockaddr*) &name, &namelen) == -1)
        {
            error_die("getsockname");
        }

        /* Return the port as through a parameter */
        *port = ntohs(name.sin_port);
    }

    /* Enable the socket socket to accept connections */
    if (listen(httpdSocket, 5) < 0)
    {
        error_die("listen");
    }

    /* Return the socket */
    return (httpdSocket);
}

/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void* accept_request(void* clientPtr)
{
    char buf[1024];
    int32_t numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i, j;
    struct stat st;
    int32_t cgi = 0; /* becomes true if server decides this is a CGI program */
    char* query_string = NULL;
    int32_t client = *((int32_t*) clientPtr);

    /* Get a line from the client */
    numchars = get_line(client, buf, sizeof(buf));
    i = 0;
    j = 0;

    /* Copy the first part, up until whitespace, into method[] */
    while (!isspace(buf[j]) && (i < sizeof(method) - 1))
    {
        method[i] = buf[j];
        i++;
        j++;
    }
    method[i] = '\0';

    printf("Accepted a %s\n", method);

    /* If this isn't a GET or POST, it's not supported, so return */
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        unimplemented(client);
        return 0;
    }

    /* Skip over whitespace */
    while (isspace(buf[j]) && (j < sizeof(buf)))
    {
        j++;
    }

    /* Read the requested URL out of the buffer */
    i = 0;
    while (!isspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
    {
        url[i] = buf[j];
        i++;
        j++;
    }
    url[i] = '\0';

    if (strcasecmp(method, "POST") == 0)
    {
        /* POSTs should handled by Common Gateway Interface,
         * which runs the script at the given URL
         */
        cgi = 1;
    }
    else if (strcasecmp(method, "GET") == 0)
    {
        query_string = url;

        /* Skip over the query_string until a '?' is found */
        while ((*query_string != '?') && (*query_string != '\0'))
        {
            query_string++;
        }

        /* if a '?' was found, it should be handled by Common Gateway Interface */
        if (*query_string == '?')
        {
            cgi = 1;
            *query_string = '\0';
            query_string++;
        }
    }

    sprintf(path, "htdocs%s", url);

    /* If the root is requested, default to index.html */
    if (path[strlen(path) - 1] == '/')
    {
        strcat(path, "index.html");
    }

    /* If the path does not end in .c, and the file doesn't exist */
    if (!(path[strlen(path) - 2] == '.' && path[strlen(path) - 1] == 'c') &&
            stat(path, &st) == -1)
    {
        /* read & discard headers */
        while ((numchars > 0) && strcmp("\n", buf))
        {
            numchars = get_line(client, buf, sizeof(buf));
        }

        /* Then 404 the client */
        not_found(client);
    }
    else
    {
        if ((st.st_mode & S_IFMT) == S_IFDIR)
        {
            strcat(path, "/index.html");
        }

        if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
        {
            cgi = 1;
        }

        if (!cgi)
        {
            /* If this isn't Common Gateway Interface, serve the file to the client */
            serve_file(client, path);
        }
        else
        {
            /* Otherwise, execute the CGI script */
            execute_cgi(client, path, method, query_string);
        }
    }

    close(client);
    return 0;
}

/**********************************************************************/
/* Execute a CGI script.  Will need to set environment variables as
 * appropriate.
 * Parameters: client socket descriptor
 *             path to the CGI script */
/**********************************************************************/
void execute_cgi(int32_t client, const char* path, const char* method,
                 const char* query_string)
{
    char buf[1024];
    int32_t cgi_output[2];
    int32_t cgi_input[2];
    pid_t pid;
    int32_t status;
    int32_t i;
    char c;
    int32_t numchars = 1;
    int32_t content_length = -1;
    char postContent[1024];

    buf[0] = 'A';
    buf[1] = '\0';

    if (strcasecmp(method, "GET") == 0)
    {
        /* read & discard headers */
        while ((numchars > 0) && strcmp("\n", buf))
        {
            numchars = get_line(client, buf, sizeof(buf));
        }
    }
    else if (strcasecmp(method, "POST") == 0)
    {
        numchars = get_line(client, buf, sizeof(buf));

        while ((numchars > 0) && strcmp("\n", buf))
        {
            buf[15] = '\0';

            if (strcasecmp(buf, "Content-Length:") == 0)
            {
                content_length = atoi(&(buf[16]));
            }

            numchars = get_line(client, buf, sizeof(buf));
        }

        if (content_length == -1)
        {
            bad_request(client);
            return;
        }
    }

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);

    if (pipe(cgi_output) < 0)
    {
        cannot_execute(client);
        return;
    }

    if (pipe(cgi_input) < 0)
    {
        cannot_execute(client);
        return;
    }

    if ((pid = fork()) < 0)
    {
        cannot_execute(client);
        return;
    }

    if (pid == 0)   /* child: CGI script */
    {
        if (path[strlen(path) - 2] == '.' && path[strlen(path) - 1] == 'c')
        {
            /* If the path ends in .c, don't process it as a script
             * Instead run some C code!
             */

            if (strcasecmp(method, "GET") == 0)
            {
                printf("C GET: %s\n", query_string);
            }
            else if (strcasecmp(method, "POST") == 0)
            {
                /* Get the post content */
                memset(postContent, 0, sizeof(postContent));
                read(cgi_input[0], postContent, sizeof(postContent));
            }

            if (0 == strcasecmp(path, "htdocs/motor_control.c"))
            {
                processMotorControl(postContent, content_length);
            }
        }
        else
        {
            /* If the path doesn't end in .c, process it as a script */
            char meth_env[255];
            char query_env[255];
            char length_env[255];

            dup2(cgi_output[1], 1);
            dup2(cgi_input[0], 0);
            close(cgi_output[0]);
            close(cgi_input[1]);
            sprintf(meth_env, "REQUEST_METHOD=%s", method);
            putenv(meth_env);

            if (strcasecmp(method, "GET") == 0)
            {
                sprintf(query_env, "QUERY_STRING=%s", query_string);
                putenv(query_env);
            }
            else if (strcasecmp(method, "POST") == 0)
            {
                sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
                putenv(length_env);
            }

            execl(path, path, NULL);
            exit(0);
        }
    }
    else    /* parent */
    {
        close(cgi_output[1]);
        close(cgi_input[0]);

        if (strcasecmp(method, "POST") == 0)
        {
            for (i = 0; i < content_length; i++)
            {
                recv(client, &c, 1, 0);
                write(cgi_input[1], &c, 1);
            }
        }

        while (read(cgi_output[0], &c, 1) > 0)
        {
            send(client, &c, 1, 0);
        }

        close(cgi_output[0]);
        close(cgi_input[1]);
        waitpid(pid, &status, 0);
    }
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(int32_t client, const char* filename)
{
    FILE* resource = NULL;
    int32_t numchars = 1;
    char buf[1024];

    buf[0] = 'A';
    buf[1] = '\0';

    printf("Serve file %s to %d\n", filename, client);

    while ((numchars > 0) && strcmp("\n", buf))   /* read & discard headers */
    {
        numchars = get_line(client, buf, sizeof(buf));
    }

    resource = fopen(filename, "r");

    if (resource == NULL)
    {
        not_found(client);
    }
    else
    {
        headers(client, filename);

        fgets(buf, sizeof(buf), resource);

        while (!feof(resource))
        {
            send(client, buf, strlen(buf), 0);
            fgets(buf, sizeof(buf), resource);
        }
    }

    fclose(resource);
}

/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int32_t get_line(int32_t sock, char* buf, int32_t size)
{
    int32_t i = 0;
    char c = '\0';
    int32_t n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);

        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);

                if ((n > 0) && (c == '\n'))
                {
                    recv(sock, &c, 1, 0);
                }
                else
                {
                    c = '\n';
                }
            }

            buf[i] = c;
            i++;
        }
        else
        {
            c = '\n';
        }
    }

    buf[i] = '\0';

    return (i);
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char* sc)
{
    perror(sc);
    exit(1);
}
