#include "types.h"
#include "user.h"
#include "stat.h"

int getcmd(char*, int);
int parserun(char*);
int listproc(void);

int
main(void) 
{
  static char buf[100];

  while (getcmd(buf, sizeof(buf)) > 0) {
    parserun(buf);
  }
  exit();
}

int
getcmd(char *buf, int nbuf)
{
  printf(2, "pmanager: ");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

int 
parserun(char *buf)
{
  if (!strcmp(buf, "list")) {
    // need to be syscall! -> for checking procs
    listproc();
  }

}
