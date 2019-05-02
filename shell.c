#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

int fon_reg = 0;
int miss = 0;

struct List {
	char *word;
	int num;
	struct List *next;
};
struct Stream {
	char *read_file;
	char *write;
	char *write_end;
	int input;
	int output;
	int close;
	char **prog;
	int pid;
	char *del;
	struct Stream *next;
};


void rm_list(struct List *p,int a){
	if (p) {
		rm_list(p -> next,a);
		if (a) 
			free(p -> word);
		free(p);
	}
}

void rm_struct(struct Stream *p){
	if (p) {
		rm_struct(p -> next);
		if (p -> read_file) 
			free(p -> read_file);
		if (p -> write) 
			free(p -> write);
		if (p -> write_end) 
			free(p -> write_end);
		if (p -> del) 
			free(p -> del);
		int i = 0;
		while ((p -> prog)[i]){
			free((p -> prog)[i]);
			i++;
		}
		free(p -> prog);
		
		free(p);
	}
	return;
}

char **list_to_mass(struct List* p) {
	char **mas = NULL;
	int size = 0;
	if(p == (void*)-1)
		return NULL;
	while (p) {
		if ((p -> num) == 0) {
			size++;
			mas = realloc(mas, sizeof(void *)*(size + 1));
			mas [size - 1] = p -> word;
			p = p ->next;
		} else {
			break;
		}
	}
	if (size) {
		mas[size] = NULL;
	}
	return mas;
}

void printlist(struct List *p) {
	struct List *tmp = p;
	while (tmp) {
		printf("%s %d\n",tmp -> word,tmp -> num);
		tmp = tmp -> next;
	}
}

struct List *readstr(int fd, int isrec){
	struct List *spisok = NULL;
	char c;
	char *param = NULL;
	int g, len = 0;
	int f = 1;
	int phr = 1;
		
	while ((g = read(fd,&c,1)) == 1){
		if ((c == 10) && (!phr)){
			printf("> ");
			fflush(stdout);
		}
		if (c == '`') {
			int pipe1[2];
			int pipe2[2];
			pipe(pipe1);
			pipe(pipe2);
			int pid = fork();
			if (!pid){				// pipe1 -- to shell; pipe2 -- from shell
				close(pipe1[1]);	
				close(pipe2[0]);
				dup2(pipe1[0],0);
				dup2(pipe2[1],1);
				close(pipe1[0]);
				close(pipe2[1]);
				execl("/proc/self/exe","m","1", NULL);
				exit(1);
			}
			close(pipe1[0]);
			close(pipe2[1]);
			char b;
			while((read(fd,&b,1) == 1) && (b != '`')) {
				write(pipe1[1],&b,1);
				if (b == 10) {
					write(1,"> ",2);
				}
				
			}
			close(pipe1[1]);
			spisok = readstr(pipe2[0],1);
			close(pipe2[0]);
			waitpid(pid,NULL,0);
			if (spisok) {
				struct List *tmp = spisok;
				while(tmp -> next) {
					tmp = tmp -> next;
				}
				tmp -> next = readstr(fd,1);
				return spisok;
			}
			read(fd,&c,1);				
		}

		if (c == '"') {	
			phr = !phr;
			continue;
		}
		if (isspace(c) && phr){
			if (f && (c != 10)){
				continue;

			} else {
				break;
			}
		}
		f = 0;
		len++;
		param = realloc(param,len + 1);
		param[len - 1] = c;
	}
	if (len) {
		
		if (param != NULL) {
				param[len] = '\0';
		}
		spisok = malloc(sizeof(*spisok));
		if (strcmp(param,"<") == 0) {
			spisok -> num = 1;
		} else {
			if (strcmp(param,">") == 0) {
				 spisok -> num = 2;
			} else {
				if (strcmp(param,">>") == 0) {
					spisok -> num = 3; 
				} else {
					if (strcmp(param, "|") == 0) {
						spisok -> num = 4;
					} else {
						if (strcmp(param,"&") == 0) {
							spisok -> num = 5;
						} else { 
							if (strcmp(param,";") == 0) {
								spisok -> num = 6;
							} else {
								if (strcmp(param,"||") == 0) {
									spisok -> num = 7;
								} else {
						 			spisok -> num = 0;
								}
							}
						}
					}
				}
			}
		}
	
		spisok -> word = param;
		if ((c != 10) && (g == 1)) {
			spisok -> next = readstr(fd,1);
		} else {
			if (phr == 1) {
				spisok -> next = NULL;
			} else {
				fprintf(stderr,"unexpected EOF while looking for matching `\"' \n");
				fprintf(stderr,"syntax error: unexpected end of file \n");
				return NULL;
				
			}
		}
	}
	if(isrec){
		return spisok;
	} else {
		if (spisok){
			return spisok;
		} else {
			if (g != 1) {
				return ((void *) -1);
			} else {
				return NULL;
			}
		}
	}
}

void wait_fon()
{	

	int pid;
	int status;
	while ((pid = wait4(-1, &status, WNOHANG, NULL)) > 0){	
		if (WIFEXITED(status)){
			printf("Process %d exited with code %d\n", (pid),WEXITSTATUS(status));
		} else {
			printf("Process %d was killed by signal %d\n",(pid),WTERMSIG(status));
		}
			fflush(stdout);
	}
	return;
}

struct Stream *create_struct(struct List *p, int *ferror) {
	static int old = 0;
	struct Stream *Dog = (struct Stream*) malloc(sizeof(struct Stream));
	Dog -> read_file = NULL;
	Dog -> write = NULL;
	Dog -> write_end = NULL;
	Dog -> input = 0;
	Dog -> output = 0;
	Dog -> close = 0;
	Dog -> del = NULL;
	while (p != NULL) {
		switch(p -> num) {
			case 1: 
					if ((p -> next -> word) != NULL) {
						p = p -> next;
						Dog -> read_file = p -> word;
					} else {
						*ferror = 1;
						fprintf(stderr,"Incorrect use '<'\n");
					}
					break; 
			case 2: 	
					if ((p -> next -> word) != NULL) {
						p = p -> next;
						Dog -> write = p -> word;
					} else {
						*ferror = 1;
						fprintf(stderr,"Incorrect use '>'\n");
					}
					break; 					
			case 3: 	
					if ((p -> next -> word) != NULL) {
						p = p -> next;
						Dog -> write_end = p -> word;
					} else {
						*ferror = 1;
						fprintf(stderr,"Incorrect use '>>'\n");
					}
					break; 

			case 4:
					Dog -> del = malloc(sizeof(char));
					strcpy((Dog -> del), "|");	
					break;
			case 6:
					Dog -> del = malloc(sizeof(char));
					strcpy((Dog -> del), ";");	
					break;
			case 7: 
					Dog -> del = malloc(3*sizeof(char));
					strcpy((Dog -> del), "||");	
					break;
	
			default:
					Dog -> del = malloc(sizeof(char));
					 break;
		}
		p = p -> next;
	}
	return Dog;		
}

void analyze_struct(struct Stream *Puppy){
	int fd;
	if (fon_reg) {
		int input = open("/dev/null", O_RDONLY);
		dup2(input,0);
		close(input);
	}

	if (Puppy -> read_file) {
		fd=open(Puppy -> read_file, O_RDONLY);
		if (fd == -1) {
			perror(Puppy -> read_file);
			miss = 1;
		}
		dup2(fd,0);
		close(fd);
	}
	if (Puppy -> write) {
		fd = open(Puppy -> write, O_CREAT|O_WRONLY|O_TRUNC, 0666);
		if (fd == -1) {
			perror(Puppy -> write);
			miss = 1;
		}
		dup2(fd,1);
		close(fd);
	}
	if (Puppy -> write_end) {
		fd = open(Puppy -> write_end, O_CREAT|O_WRONLY|O_APPEND);
		if (fd == -1){
			perror(Puppy -> write_end);
			miss = 1;
		}
		dup2(fd,1);
		close(fd);
	}
	if (Puppy -> input) {
		dup2(Puppy -> input, 0);
		close(Puppy -> input);
	}
	if (Puppy -> output) {
		dup2(Puppy -> output,1);
		close(Puppy -> output);
	}
	if (Puppy -> close) {
		close(Puppy -> close);
	}
	return;
}	
 

struct Stream *create_lists(struct List *p ) {
	struct Stream *Kitty = NULL; //head
	struct Stream *Meow = NULL;  //curr 
	struct List *head = p;
	int ferror;
	char **mass = NULL;
	if (p == NULL) {
		return NULL;
	}
	struct List *q = p;
	
	while(p -> next) {
		if ((p -> num != 4) && (p -> num != 6) && (p -> num != 7)) {
			q = p;
			p = p -> next;

		} else {
			q = p;
			p = p -> next;
			q -> next = NULL;
			mass = list_to_mass(head);
			rm_list(head,0);
			struct Stream *Cat = create_struct (head, &ferror);
			Cat -> prog = mass;
			if (!Kitty) {
				Kitty = Cat;
				Meow = Cat;
				Meow -> next = NULL;
			} else {
				Meow -> next = Cat;
				Meow = Meow -> next;
				Meow -> next = NULL;
			}
			head = p;
		}
		
	}
	mass = list_to_mass(head);
	rm_list(head,0);
	struct Stream *Cat = create_struct (head, &ferror);
	Cat -> prog = mass;
	if (!Kitty) {
		Kitty = Cat;
		Meow = Cat;
		Meow -> next = NULL;
	} else {
		Meow -> next = Cat;
		Meow = Meow -> next;
		Meow -> next = NULL;
	}
	return Kitty; 
}

int Exec(struct Stream *Gaga) {
	int pid = 0;
	if (strcmp((Gaga -> prog)[0],"cd")){
		pid = fork();
		if (!pid) {
			analyze_struct(Gaga);
			if(miss)
				exit(1);
			execvp((Gaga -> prog)[0], Gaga -> prog);
			perror((Gaga -> prog)[0]);
			exit(1);
		} else {
			Gaga -> pid = pid;
		}
	} else {
		if ((Gaga -> prog)[1]){
			int err = chdir((Gaga -> prog)[1]);
			if (err == -1) {
				perror((Gaga -> prog)[1]);
			}
		} else {
			fprintf(stderr,"There is no second param!\n");
		}
	}

	return pid;
}

void Conveer(struct Stream *Gaga) {
	int	open_pipe = 0;
	int fd[2];
	int seq = 0;
	int cor = 0;
	struct Stream *tmp = Gaga;
	while (Gaga) {
		if ((Gaga -> next) && (Gaga -> del)) {
			if (!(strcmp((Gaga -> del),"|"))){
				pipe(fd);
				Gaga -> output = fd[1];
				Gaga -> next -> input = fd[0];
				open_pipe = 1;
			}
		}
		if (!miss) {
			if (!(strcmp((Gaga -> del),"||"))){
				seq = 1;
				if ((Gaga -> next) == NULL){
					fprintf(stderr,"Wrong usage of '||'\n");
					return;
				}
				if (!cor){
					int lion = Exec(Gaga);
					if (lion) {
						int stat;
						waitpid(lion, &stat, 0);
						if (WIFEXITED(stat) && !WEXITSTATUS(stat)){
							cor = 1;
						}
					}
				}
			} else {
					if ((seq == 0) || (cor == 0)){
						Exec(Gaga);
					}
					seq = 0;
					cor = 0;
			}
		}
		if (open_pipe){
			close(fd[1]);
		}
		open_pipe = 0;
		Gaga = Gaga -> next;
	}
	while (tmp) {
		if (tmp -> input) {
			close(tmp -> input);
		}
		if (tmp -> output) {
			close(tmp -> output);
		}
		if (tmp -> close) {
			close(tmp -> close);
		}
		tmp = tmp -> next;
	}
}		

int main(int argc, char ** argv)
{
	signal(SIGINT,SIG_IGN);
	int for_user = (argc == 1) ? 1 : 0;
	char **mass;
	int ferror = 0;
	int status;
	int i = 0;	
	fon_reg = 0;
	struct List *list = NULL;
	while (list != (void*)-1) {
		ferror = 0;
		wait_fon();
		miss = 0;
		if (for_user) {
			printf("[term] $ ");
			fflush(stdout);
		}
		list = readstr(0,0);
		if (!list || (list == (void*)-1)) {	
			continue;
		}
		fon_reg = 0; //check_fon_regim
		{
			struct List *tmp = list;
			while (tmp -> next) {
				if ((tmp -> word)[0] == '&') {
					printf("syntax error: incorrect input &\n");
					ferror = 1;
					break;
				}
				tmp = tmp -> next;
				
			}
			if ((tmp -> word)[0] == '&') {
				fon_reg = 1;
			}
		} 
		if (ferror){
			rm_list(list,1);
			continue;
		}
		printlist(list);	
		struct Stream *Ququ = create_lists(list);
		Conveer(Ququ);
		struct Stream *Check = Ququ;
			if (!fon_reg){
				while (Check) {
					waitpid(Check -> pid, NULL, 0);
					Check = Check -> next;
				}
			} 
			rm_struct(Ququ);
		wait_fon();
	}
	if (for_user)
		printf("logout\n");	
	return 0;
}
