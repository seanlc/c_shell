#define _BSD_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>

#define ARG_MAX 20
#define MAX_CMDS 10

char * cmds[MAX_CMDS];
int num_proc = 1;

void error(char * msg)
{
    perror(msg);
    exit(-1);
}
void init_cmds()
{
    for(int i = 0; i < MAX_CMDS; ++i)
	cmds[i] = NULL;
}

void init_argv(char * newargv[])
{
    for(int i = 0; i < ARG_MAX; ++i)
	newargv[i] = NULL;
}


void print_cmds()
{
    printf("value of cmds: \n");
    for(int i = 0; cmds[i] != NULL; ++i)
	printf("cmds[%d] : %s\n", i, cmds[i]);
    printf("value of num_proc: %d\n", num_proc);
}
void parse_command(char * command, char * newargv[])
{
    char * tok = NULL;
    int p = 0;
    tok = strtok(command, " \n");
    while( tok != NULL )
    {
	newargv[p++] = tok;
	tok = strtok(NULL, " \n");
    }
}

int make_proc(int in, int out, char * cmd)
{
    pid_t p;
    char * newargv[ARG_MAX];
    p = vfork();

    switch(p)
    {
	case -1:
	    error("fork");
	    break;
	case 0:
	    init_argv(newargv);
	    parse_command(cmd, newargv);
	    if(in != 0)
	    {
		if((dup2(in,0)) == -1)
		    error("dup2");
		close(in);
	    }
	    if(out != 1)
	    {
		if((dup2(out,1)) == -1)
		    error("dup2");
		close(out);
	    }
	    return execvp(newargv[0], newargv);
	    break;
    }
    return p;
}
void get_comm(char * buf)
{
    printf("#");
    fgets(buf, 255, stdin);
    buf[strcspn(buf,"\n")] = 0;
}

int look_for_pipe(char * cmd)
{
    char * copyOfCmd = NULL;
    char * pipeLoc = NULL;
    int cmdlen = 0;
    if( (pipeLoc = strchr(cmd, '|')) != NULL  )
    {
	copyOfCmd = (char *)malloc(strlen(pipeLoc + 1)+1);
        strcpy(copyOfCmd, pipeLoc + 1);
        cmds[num_proc++] = strtok(copyOfCmd, "|<>&");
	cmdlen = strlen(copyOfCmd);
	strcpy(pipeLoc, pipeLoc + cmdlen + 1);
	return 1;
    }
    return 0;
}

void look_for_sym(char * str, char c, char * fileName)
{
    char * a = NULL;
    char b[256];
    char * fname = NULL;
    int fileLen;
    if((a = strchr(str,c)) != NULL)
    {
	strcpy(b,a+1);
        fname = strtok(b, "<>|& ");
	fileLen = strlen(b);
	strcpy(a, a + fileLen + 1);
        strcpy(fileName, fname);
    }
}
void input_redir(char inRedirect[256])
{
    int fd;
    if(strlen(inRedirect) > 0)
    {
	if((fd = open(inRedirect, O_RDONLY)) == -1  )
	    error("open before input redirection");
	if((dup2(fd,0)) == -1)
	    error("dup2 input redirection");
	close(fd);	    
    }
}
void output_redir(char outRedirect[256])
{
    int fd;
    if(strlen(outRedirect)> 0)
    {
        if((fd = open(outRedirect, O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1)
	    error("open before output redirection");
        if((dup2(fd,1)) == -1)
	    error("dup2 output redirection");
        close(fd);
    }

}
int main(int argc, char * argv[])
{
    char command[1024];
    char * newargv[ARG_MAX];
    char inRedirect[256] = "";
    char outRedirect[256] = "";
    int pipefd[2];
    int in = 0;
    int i;
    
    while(1)
    {
        init_argv(newargv);
        get_comm(command);
        look_for_sym(command, '<', inRedirect);
        look_for_sym(command, '>', outRedirect);
        
        while( look_for_pipe(command) == 1)
		;
        
	cmds[0] = command;
        if(strlen(inRedirect) > 0)
	    in = open(inRedirect, O_RDONLY);
	for(i = 0; i < num_proc -1; ++i)
	{
	    pipe(pipefd);
	    make_proc(in, pipefd[1], cmds[i]);
	    close(pipefd[1]);
	    in = pipefd[0];
	}
	if(in != 0)
	    dup2(in, 0);
        output_redir(outRedirect); 	
	init_argv(newargv);
	parse_command(cmds[i], newargv);
	execvp(newargv[0], newargv);
    }

    return 0;
}
