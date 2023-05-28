#include "types.h"
#include "user.h"
#include "stat.h"

int getcmd(char*, int);
int parserun(char*);
int listproc(void);
int checkspace(char*);
char* getop(char*, char*);

int
main(void) 
{
  static char buf[100];
  int status = 0;

  while (getcmd(buf, sizeof(buf)) >= 0) {
    // 0 for normal, 1 for exit, -1 for error
    status = parserun(buf);

    switch (status) {
      case 1:
        exit();
      case -1:
        printf(1, "bad command for pmanager!\n");
      default:
        break;
    }
  }
  exit();

  return 0;
}

int
getcmd(char *buf, int nbuf)
{
  printf(2, "[pmanager]: ");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

int 
parserun(char *buf)
{
  // opcode: list | kill | execute | memlim | exit
  char op[100];
  char *next = getop(buf, op);

  if (!strcmp(op, "exit")) {
    return 1;
  }

  // List process information
  if (!strcmp(op, "list")) {
    listproc();
    return 0;
  }

  if (!strcmp(op, "kill")) {
    char pidstr[20];
    int pid;

    getop(next, pidstr);

    // bad pid
    if ((pid = atoi(pidstr)) <= 0) {
      return -1;
    }

    int status = kill(pid);

    if (!status) printf(1, "successfully killed pid: %d\n", pid);
    else printf(1, "killed failed\n");

    return status;
  }

  if (!strcmp(op, "execute")) {
    char path[100], stacksizestr[20];
    char *argv[10];
    int stacksize;
    int pid;

    getop(getop(next, path), stacksizestr);

    if ((stacksize = atoi(stacksizestr)) <= 0) {
      printf(1, "stacksize must be int\n");
      return -1;
    }

    argv[0] = path;

    // parent -> no wait.
    if ((pid = fork()) != 0) {
      // wait();
      return 0;
    }

    // children
    int status = exec2(path, argv, stacksize);
    if (status) {
      printf(1, "exec failed\n");
      exit();
    }

    return status;
  }

  if (!strcmp(op, "memlimit")) {
    char pidstr[20], limstr[20];
    int pid, limit;

    getop(getop(next, pidstr), limstr);

    if ((pid = atoi(pidstr)) <= 0 || (limit = atoi(limstr)) < 0) {
      printf(1, "pid and limit must be int\n");
      return -1;
    }

    int status = setmemorylimit(pid, limit);
    if (status) printf(1, "setmemorylimit failed\n");
    else printf(1, "setmemorylimit succeeded\n");

    return status;
  }

  return -1;
}

// Parse op and return string after current op.
char *
getop(char *src, char *dst) 
{
  // implement copy op from src to dst
  while (checkspace(src) > 0) {
    *dst = *src;
    dst++;
    src++;
  }
  
  // add null at the end of string
  *dst = 0;

  // skip spaces for next string
  while (checkspace(src) == 0) {
    src++;
  }

  return src;
}

// -1 for null, 0 for spaces, 1 for others
int
checkspace(char *str)
{
  if (*str == 0) 
    return -1;

  if (*str == ' ' || *str == '\n' || *str == '\t')
    return 0;

  return 1;
}