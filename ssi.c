#include <sys/wait.h>
#include <readline/readline.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

/*
June 15 2025
CSC 360
Thomas Pietrovito
V00973900
*/

// init jobs struct
typedef struct job{
  int pid;
  char input[1024];
  struct job *next;
} job;

// Gets the current directory to build accuracte shell prompt. Checks for valid username and hostname.
void get_prompt(char *prompt, char *cwd, char *host, int size){
  char *username = getlogin();
  if(!username){
    username = getenv("USER");
    if (!username){
      username = "unknown";
    }
  }
  
  if(gethostname(host, size) == -1){
    printf("error getting hostname.\n");
    host[0] = '\0';
  }
  
  sprintf(prompt, "%s@%s: %s > ", username, host, cwd);
}

// simple check to ensure 'exit' is the only command that will exit the program
int read_input(char *input){
  if (input == NULL){
      printf("\n");
      return 0;
    }

    if (*input == '\0'){
      return 0;
    }
    
    if (strcmp(input, "exit") == 0) {
      return -1;
    }

  return 1;
}

// tokenize the input and append NULL as per execvp() syntax
void tokenize_command(char *input, char **command){
  int n = 0;
  char *token = strtok(input, " ");
  while(token != NULL){
    command[n++] = token;
    token = strtok(NULL, " ");
  }
  command[n] = NULL;
}

// if "cd" or "cd ~" entered, navigate to HOME path 
void home_dir(char *cwd, char **command){
  char *home_path = getenv("HOME");

  if (home_path != NULL){  
    int sts = chdir(home_path);
    
    if (sts == -1){
      printf("%s: No such file or directory.\n", command[1]);
      return;
    }
    else {
      getcwd(cwd, sizeof(cwd));
      return;
    }
  }
  else {
    printf("No home path set.\n");
    return;
  }
}

// change to given directory if it exsits
void change_dir(char *cwd, char **command){
  int cd_sts = chdir(command[1]);
  if (cd_sts == -1){
    printf("%s: No such file or directory\n", command[1]);
  }
  else {
    getcwd(cwd, sizeof(cwd)); 
  }
}

// fork and execute for foreground commands
int fg_execute(char **command){
  int pid = fork();

  if (pid < 0){
    printf("error forking\n");
    exit(1);
  }

  if (pid == 0){
    signal(SIGINT, SIG_DFL);
    execvp(command[0], command);
    printf("%s: No such file or directory\n", command[0]);
    exit(1);
  }  
  return pid;
}

// add a new node to the linked list of jobs
job* add_job(job* head, char *input, int pid){

  job *new_job = (job*)malloc(sizeof(job));
  
  if (new_job == NULL){
    printf("malloc error, unable to add job to bglist\n");
    return head;
  }

  (*new_job).pid = pid;
  strncpy((*new_job).input, input, sizeof((*new_job).input));
  (*new_job).next = NULL;

  if (head == NULL){
    return new_job;
  }
  else {
    job *cur = head;
    while((*cur).next != NULL){
      cur = (*cur).next;
    }
    (*cur).next = new_job;
    return head;
  }
}

// iterates through linked list of jobs, checks wait status of each. If a completed task is found, remove from the list
job* check_bglist(job *head){

  job *prev = NULL;
  job *cur = head;

  while(cur != NULL){
    int pid = waitpid((*cur).pid, NULL, WNOHANG);

    if(pid == 0){
      prev = cur;
      cur = (*cur).next;
      continue;
    }
    
    if(pid < 0){
      printf("waitpid error\n");
      break;
    }
    
    if (pid > 0){
      printf("%d: %s has terminated.\n", (*cur).pid, (*cur).input);

      //remove cur from bglist
      if(prev == NULL){
	head = (*cur).next;
	free(cur);
	cur = head;
      }
      else {
	(*prev).next = (*cur).next;
	free(cur);
	cur = (*prev).next;
      }
    }
  }
  return head;
}

// print the contents of the job linked list
void print_bglist(job *head){

  if(head == NULL){
    printf("no bg jobs currently running\n");
  }
  
  else{
    job *cur = head;
    while(cur != NULL){
      printf("%d: %s\n", (*cur).pid, (*cur).input);
      cur = (*cur).next;
    }
  }
}

int main(){
  
  // init local varibles
  char prompt[256], cwd[256], host[256], job_input[256];
  char *command[1024];

  // set head of linked list
  job *head = NULL;

  // main loop
  while(1){

    // init linked list and signal control (ignoring ^C in the parent process)
    head = check_bglist(head);
    signal(SIGINT, SIG_IGN);
  
    // READ:
    // 1. get the current directory, print the  prompt
    // 2. read the user input into the input buffer (dynamically)
    // 3. free the input buffer if invalid input read, start loop over
    
    getcwd(cwd, sizeof(cwd));
    get_prompt(prompt, cwd, host, sizeof(host));

    char *input = readline(prompt);
    int result = read_input(input);

    if (result == 0){
      free(input);
      continue;
    }

    if (result == -1){

      // kill any lingering bg jobs
      job *cur = head;
      while(cur != NULL){
	kill((*cur).pid, SIGTERM);
	cur = (*cur).next;
      }
      free(input);
      break;
    }
    
    // EVALUATE:
    // 1. copy the input into a seperate array before in-place tokenization -- this is used in the linked list later
    // 2. tokenize the command to satisy the execvp() syntax
    // 3. check cases of "cd", "cd ~", "bglist"

    strncpy(job_input, input, sizeof(job_input)-1);
    job_input[sizeof(job_input) - 1] = '\0';
    tokenize_command(input, command);

    // change directory
    if (strcmp(command[0], "cd") == 0){

      if (command[1] == NULL || strcmp(command[1], "~") == 0){
        home_dir(cwd, command);
      }
      else {
        change_dir(cwd, command);
      }
      free(input);
      continue;
    }

    // print bglist
    if (strcmp(command[0], "bglist") == 0){
      print_bglist(head);
      free(input);
      continue;
    }
    
    // EXECUTE:
    // 1. if command is prefixed with "bg", fork and execute. Add the process to the linked list.
    // 2. if no prefix, then execute as a foreground command.
    
    // bg execution
    if (strcmp(command[0], "bg") == 0){

      if(command[1] == NULL){
	printf("bg: missing command!\n");
	free(input);
	continue;
      }
      
      int pid = fork();
      if(pid < 0){
	printf("error forking");
	exit(1);
      }

      if (pid == 0){

	// detach the process! (this was a huge pitfall for me and I needed to do some serious research)
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);
	setsid();
	
	execvp(command[1], &command[1]);
	printf("%s: No such file or directory\n", command[1]);
	exit(1);
      }

      head = add_job(head, job_input, pid);
      free(input);
      continue;
    }

    // fg execution
    else {
      int child = fg_execute(command);
      signal(SIGINT, SIG_IGN);
      waitpid(child, NULL, 0);
      signal(SIGINT, SIG_DFL);
    }

    // free the input and keep looping until exit command given
    free(input);
    
  }
  
  return 0;
}


   
 
