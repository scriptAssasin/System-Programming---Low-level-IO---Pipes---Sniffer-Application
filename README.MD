Compilation instructions (Makefile):
1. make -> Compile only
2. make run -> To compile and run
3. make clean -> To delete the .o files and the executable
Starting with the filesystem structure: There are the following
folders:
• Fifos -> The Fifos folder stores the named pipes that
are created during program execution
• Includes -> The Includes folder contains the files
header that have been used by the program
• Outputs -> In the Outputs folder the .out files are produced which
are created during the execution of the workers
• Queue -> The Queues folder contains the source code that
used for queues that have been used in
program. I had implemented this code in the framework
of the Data Structures and Programming Techniques course
in the second semester. I have kept the file name ADTQueue.c
• Worker -> The Worker folder contains the source code
which the workers use in order to parse it
text and urls and generate the .out files
Outside of the folders, sniffer.c is contained with its main
program (discussed below), the Makefile and finder.sh

sniffer.c Structure

The main function has the role of “manager”. So initially
a pipe is created in order to achieve communication
between manager and listener. The manager making a fork after the
pipe creation creates the listener, as a child, the listener calls
the dup2 system call in order to redirect stdout
at the write end of the pipe. Then via execvp is called
inotifywait with arguments -m -e create -e moved_to.
The -m argument is to prevent inotifywait from terminating at
first event, and the rest of the arguments are for "listening" only
create and move events
The manager in turn then, within a while, reads from the
pipe the events from inotifywait. Then as long as the Queue with them
available workers is empty, a new named pipe is created via
whose manager writes the name of the file that
detected by inotifywait. The manager then calls fork
and creates a new worker that reads the filename
through read from the named pipe it does the parsing of the content and
produces <filename>.out.
When the worker completes its work, it calls kill with a signal
SIGSTOP to enter stopped state. Sent after sigchld signal
to the manager where the relevant flag becomes true and the manager through it
waitpid and with arguments WNOHANG | WUNTRACED detects which ones
processes are in stop state and inserts them into the queue with them
available workers

worker.c Structure

The worker initially reads the contents from the named pipe, and at
then creates <filename>.out, where <filename> is its name
file read. In order to parse the text
from <filename>, the text is tokenized with spaces delimiter
and then each token goes through a regex that it recognizes
the http links. Any tokens recognized by the regex as http links,
are added to a queue. This queue is then traversed, h
which contains the entire links. With the appropriate tokenizations with
delimiters . and / the url is parsed and the location is extracted, where
each location is then inserted into a final_link_queue. It's happening
destroy the first queue, and then traverse the
final_link_queue, where there is also a check for duplicates, where
the number of duplicates is recorded. Finally, after "cleaning" the
final queue from duplicates is written to <filename>.out each
location from the queue and the number of its appearances