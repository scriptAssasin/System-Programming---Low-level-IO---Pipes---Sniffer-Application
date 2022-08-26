#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <regex.h>
#include <signal.h>
#include "ADTQueue.h"

//worker function for url parsing and outpouts creation
void worker(int fd);

//flag to determine if SIGCHLD signal is raised
int flag = 0;

//SIGCHLD signal handler
void sigchld_handler(int sig)
{
  flag = 1;
}

//SIGINT signal handler
void siginthandler(int sig)
{
    exit(0);

}
//manager process
int main(void)
{
  //unnamed pipe for inter-process communication between manager and listener
  int pipe1[2];

  if (pipe(pipe1) < 0)
    exit(1);

  //fork manager to create listener as child of manager
  int pid = fork();

  if (pid == 0)
  {
    // listener at work
    // create a copy of stdout to write end of pipe

    dup2(pipe1[1], 1);
    close(pipe1[0]); // Close both ends of the pipe!
    close(pipe1[1]); // Close both ends of the pipe!

    char *programName = "inotifywait";
    char *arg_list[] = {programName, ".", "-m", "-e", "create", "-e", "moved_to", NULL};
    execvp(programName, arg_list);
    //exec results will be writted to the write end of the pipe
    exit(0);
  }

  else
  {
    signal(SIGINT, siginthandler);
    
    //Queue of available Workers
    Queue workerQueue = queue_create();
    int worker_counter = 0;
    // Manager at work
    while (1)
    {
      // Read data written on pipe by listener, and write to stdout
      char buffer[512];
      int nbytes = read(pipe1[0], buffer, sizeof(buffer));
      if (nbytes > 0)
        write(1, buffer, nbytes);

      //split inotifywait event to get only the name of the file. For example ./ Create <filename> we only need <filename>
      char *token = strtok(buffer, " ");
      int token_counter = 0;
      // loop through the string to extract all other tokens
      char fileName[512];
      while (token != NULL)
      {
        token_counter++;
        //if we are at the 3rd token, we have the file name!
        if (token_counter == 3)
          strcpy(fileName, token);

        token = strtok(NULL, " ");
      }

      int fd;
      pid_t childPID;
      char pathname[512];

      //if there is no available worker
      if (queue_size(workerQueue) == 0)
      {
        //Create a named pipe for communication between manager and worker. The number of named pipes equals to the number of the workers
        worker_counter++;
        char counter_buf[50];
        sprintf(counter_buf, "%d", worker_counter);

        strcpy(pathname, "fifos/pipe");
        strcat(pathname, counter_buf);

        if ((mkfifo(pathname, S_IRWXU | S_IRWXG)) < 0)
          perror("Failed to create FIFO : ");

        //Fork manager to create a worker
        if ((childPID = fork()) < 0)
          exit(EXIT_FAILURE);

        // in the parent process
        if (childPID)
        {
          //Open the Named Pipe
          if ((fd = open(pathname, O_WRONLY)) < 0)
            exit(EXIT_FAILURE);

          //Write the name of the file (event of inotifywait) to the named pipe
          else
            write(fd, fileName, strlen(fileName));
        }
        // in the worker process
        if (!childPID)
        {
          //worker process inside a while loop. When SIGCONT signal raised the worker continues its process.
          while (1)
          {
            //open named pipe pseudofile in order to read the <filename>
            if ((fd = open(pathname, O_RDONLY)) < 0)
              exit(EXIT_FAILURE);

            else
              //Url Parsing and create output files
              worker(fd);

            //We dont need the fifo anymore
            if ((unlink(pathname)) < 0)
              exit(EXIT_FAILURE);

            //raise SIGSTOP signal in order to be "caught" by waitpid
            kill(getpid(), SIGSTOP);
          }
        }
      }
      else
      {
        //if worker is available

        //get pid of available worker
        pid_t available_worker_pid = *(pid_t *)queue_node_value(workerQueue, queue_first(workerQueue));
        //pop that worker from queue
        queue_pop(workerQueue, queue_first(workerQueue));
        //raise SIGCONT signal in order worker to continue
        kill(available_worker_pid, SIGCONT);
      }

      //catch SIGCHLD and raise flag to true in order
      // signal(SIGCHLD, sigchld_handler);

      if (flag)
      {
        //get stopped worker pid
        int stopped_worker_pid;
        //parameter for waitpid
        int status;

        while (1)
        {
          stopped_worker_pid = waitpid(-1, &status, WNOHANG | WUNTRACED);

          if (stopped_worker_pid <= 0)
            break;

          // allocate memory for this
          int *stopper_worker_ptr = malloc(sizeof(int));
          *stopper_worker_ptr = stopped_worker_pid;
          //insert to queue
          queue_insert(workerQueue, stopper_worker_ptr);
        }

        flag = 0;
      }
    }
  }

  return 0;
}
