#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <regex.h>
#include "ADTQueue.h"

void worker(int fd)
{
    char res[256];
    int count, create_file;

    // read from named pipe
    if (read(fd, res, sizeof(res)) < 0)
        exit(EXIT_FAILURE);

    // create the <filename>.out and store it at outputs folder
    char res_file[512];
    strcpy(res_file, "outputs/");
    strcat(res_file, res);
    strcat(res_file, ".out");

    //create <filename>.out if it doesnt already exist
    if ((create_file = open(res_file, O_WRONLY | O_APPEND | O_CREAT, 0644)) < 0)
        exit(EXIT_FAILURE);

    // read content from <filename>
    char str_read[4096];
    int filedes;

    filedes = open(res, O_RDONLY, 0);
    read(filedes, (char *)str_read, 4096);
    close(filedes);

    // Initialize POSIX Regex
    regex_t regex;
    int reti = regcomp(&regex, "http?:\\/\\/(www\\.)?", REG_EXTENDED);

    if (reti)
    {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }

    Queue link_queue = queue_create();

    // Tokenize with delimiter space
    char *token = strtok(str_read, " ");

    // loop through the string to extract all other tokens
    while (token != NULL)
    {

        reti = regexec(&regex, token, 0, NULL, 0);
        if (!reti)
        {
            char *temp = malloc(512);
            strcpy(temp, token);
            // if the token is type of http link add to queue
            queue_insert(link_queue, temp);
        }

        token = strtok(NULL, " ");
    }

    Queue final_link_queue = queue_create();

    // iterate link_queue
    for (queueNode node = queue_first(link_queue); node != queue_EOF; node = queue_next(link_queue, node))
    {
        char *val = (char *)queue_node_value(link_queue, node);

        char *www;

        www = strstr(val, "http://www.");

        //if the link contains www
        if (www)
        {
            int token_count = 0;
            // tokenize based on . and /
            char *location_token = strtok(val, "./");
            char final_location[512] = "";

            // Extract the location of the string
            while (location_token != NULL)
            {
                token_count++;

                if (token_count == 3 || token_count == 4)
                {
                    strcat(final_location, location_token);
                    strcat(final_location, ".");
                }

                if (token_count == 5)
                    strcat(final_location, location_token);

                location_token = strtok(NULL, "./");
            }

            //insert location of the http link to the final link queue
            char *temp = malloc(512);
            strcpy(temp, final_location);

            queue_insert(final_link_queue, temp);
        }
        //if the string doesnt contain www
        else
        {
            //same with www functionality above
            int token_count = 0;
            char *location_token = strtok(val, "./");
            char final_location[512] = "";

            while (location_token != NULL)
            {
                token_count++;

                if (token_count == 2 || token_count == 3)
                {
                    strcat(final_location, location_token);
                    strcat(final_location, ".");
                }

                if (token_count == 4)
                    strcat(final_location, location_token);

                location_token = strtok(NULL, "./");
            }

            if (final_location[strlen(final_location) - 1] == '.')
                final_location[strlen(final_location) - 1] = '\0';

            char *temp = malloc(512);
            strcpy(temp, final_location);

            queue_insert(final_link_queue, temp);
        }
    }

    // destroy the first queue
    queue_destroy(link_queue);

    // iterate the location queue
    for (queueNode node = queue_first(final_link_queue); node != queue_EOF; node = queue_next(final_link_queue, node))
    {
        char *location = (char *)queue_node_value(final_link_queue, node);

        //count how many dublicates exist
        int counter = 0;

        for (queueNode node_nested = queue_first(final_link_queue); node_nested != queue_EOF; node_nested = queue_next(final_link_queue, node_nested))
        {
            char *temp = (char *)queue_node_value(final_link_queue, node_nested);

            if (!strcmp(location, temp))
                counter++;
        }

        // remove the dublicates
        int counter_nested = 0;

        for (queueNode node_nested2 = queue_first(final_link_queue); node_nested2 != queue_EOF; node_nested2 = queue_next(final_link_queue, node_nested2))
        {
            char *temp = (char *)queue_node_value(final_link_queue, node_nested2);

            if (!strcmp(location, temp))
            {
                counter_nested++;
                if (counter_nested != 1)
                    queue_pop(final_link_queue, node_nested2);
            }
        }

        //append a whitespace at the end of the link
        strcat(location, " ");
        write(create_file, location, strlen(location));

        //occurrences of string 
        char link_score[10];
        sprintf(link_score, "%d", counter);
        strcat(link_score, "\n");
        write(create_file, link_score, strlen(link_score));

        counter = 0;
    }

    queue_destroy(final_link_queue);
}