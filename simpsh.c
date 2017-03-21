#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include<getopt.h>
#include<signal.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/time.h>
#include<sys/resource.h>
#include<sys/wait.h>
#include<math.h>
#include<fcntl.h>
#include<ctype.h>
#include<string.h>



#define RDONLY 'r'
#define RDWR 'd'
#define WRONLY 'w'
#define PIPE 'P'

#define COMMAND 'c'
#define WAIT 'W'

#define CLOSE 'l'
#define VERBOSE 'v'
#define PROFILE 'R'
#define ABORT 'a'
#define CATCH 't'
#define IGNORE 'i'
#define DEFAULT 'f'
#define PAUSE 'p'

static int append_flag = 0;
static int cloexec_flag = 0;
static int creat_flag = 0;
static int directory_flag = 0;
static int dsync_flag = 0;
static int excl_flag = 0;
static int nofollow_flag = 0;
static int nonblock_flag = 0;
static int rsync_flag = 0;
static int sync_flag = 0;
static int trunc_flag = 0;

static int verbose_flag = 0;
static int profile_flag = 0;

void resetFlags(){
	append_flag = 0;
	cloexec_flag = 0;
	creat_flag = 0;
	directory_flag = 0;
	dsync_flag = 0;
	excl_flag = 0;
	nofollow_flag = 0;
	nonblock_flag = 0;
	rsync_flag = 0;
	sync_flag = 0;
	trunc_flag = 0;
}

struct rusage usage;
struct rusage usage_children;


void sig_handler(int signum){
	fprintf(stderr, "Caught signal %d.\n", signum);
	exit(signum);
}

struct wait_command {
	pid_t pid;
	char info[1000];
};


double findTimeDiff(struct timeval *begin, struct timeval *end){
	double b = (double)begin->tv_sec + (double)begin->tv_usec/1000000;
	double e = (double)end->tv_sec + (double)end->tv_usec/1000000;
	return e-b; 

}

int main(int argc, char* argv[]){
	struct option long_options[] =
	{
		{"append",no_argument,&append_flag,1},
		{"cloexec",no_argument,&cloexec_flag,1},
		{"creat",no_argument,&creat_flag,1},
		{"directory",no_argument,&directory_flag,1},
		{"dsync",no_argument,&dsync_flag,1},
		{"excl",no_argument,&excl_flag,1},
		{"nofollow",no_argument,&nofollow_flag,1},
		{"nonblock",no_argument,&nonblock_flag,1},
		{"rsync",no_argument,&rsync_flag,1},
		{"sync",no_argument,&sync_flag,1},
		{"trunc",no_argument,&trunc_flag,1},


		{"rdonly",required_argument,0,RDONLY},
		{"wronly",required_argument,0,WRONLY},
		{"rdwr",required_argument,0,RDWR},
		{"pipe",no_argument,0,PIPE},

		{"command",required_argument,0,COMMAND},
		{"wait",no_argument,0,WAIT},
		
		{"close",required_argument,0,CLOSE},
		{"verbose",no_argument,0,VERBOSE},
		{"profile",no_argument,0,PROFILE},
		{"abort",no_argument,0,ABORT},
		{"catch",required_argument,0,CATCH},
		{"ignore",required_argument,0,IGNORE},
		{"default",required_argument,0,DEFAULT},
		{"pause",no_argument,0,PAUSE},
		{0,0,0,0}
	};
	

	int i;
 
	int fd_s = 10;
	int fd_c = 0;
	int* fd = malloc(fd_s*sizeof(int));
	if (fd == NULL){
		fprintf(stderr, "Error with malloc.\n");
		exit(1);
	}


	int command_size = 100;
	int cur_command_size = 0;
	struct wait_command* wait_comm = (struct wait_command*)malloc(command_size*sizeof(struct wait_command));
	if (wait_comm == NULL){
		fprintf(stderr, "Error with malloc.\n");
		exit(1);
	}
	for (i = 0; i< command_size;i++){
		wait_comm[i].pid = -10;
	}

	int opt_flags = 0;
	char* carr[100];
	int carr_index = 0;
	int opt_index = 0;
	
	int x; //for getopt_long
	for (i =0;i<100;i++){
		carr[i] = NULL;
	}

	struct timeval begin_user;
	struct timeval end_user;
	struct timeval begin_sys;
	struct timeval end_sys;


	while (1){
		
		x = getopt_long(argc,argv,"",long_options,&opt_index);
		if (x == -1)
			break;

		//start usage tracking
		if(getrusage(RUSAGE_SELF, &usage) == -1){
			fprintf(stderr, "Error getting resource usage.\n");
		}
		else{
			begin_user = usage.ru_utime;
			begin_sys = usage.ru_stime;
		}


		if (fd_s == fd_c){
			fd_s *= 10;
			fd = (int*)realloc((void*)fd, fd_s*sizeof(int));
			if (fd == NULL){
				fprintf(stderr, "Error with realloc.\n");
				exit(1);
			}
		}
	
		if (cur_command_size == command_size){
			command_size *= 10;
			wait_comm = (struct wait_command*)realloc((void*)wait_comm, command_size*sizeof(struct wait_command));
			if(wait_comm == NULL){
				fprintf(stderr, "Error with realloc.\n");
				exit(1);
			}
			for(i=cur_command_size;i<command_size;i++)
				wait_comm[i].pid = -10;
		}

		opt_flags = (append_flag*O_APPEND) | (cloexec_flag*O_CLOEXEC) | (creat_flag*O_CREAT) | (directory_flag * O_DIRECTORY) |
			(dsync_flag*O_DSYNC) | (excl_flag*O_EXCL) | (nofollow_flag*O_NOFOLLOW) | (nonblock_flag*O_NONBLOCK) | (rsync_flag*O_RSYNC) |
			(sync_flag*O_SYNC) | (trunc_flag*O_TRUNC);		

		switch(x){
			case 0:
			{
				if (verbose_flag){
					printf("--%s\n", long_options[opt_index].name);
				}
				break;
			}
			case RDONLY:
			{
				if (verbose_flag){
					printf("--%s %s\n","rdonly",optarg);
				}
				int ifd = open(optarg, O_RDONLY | opt_flags,0644);
				opt_flags = 0;
				resetFlags();

				if (ifd < 0){
					fprintf(stderr,"Error with opening file with rdonly.\n");
					exit(1);
				
				}
				else{
					fd[fd_c] = ifd;
					fd_c++;
				}
				
				if (profile_flag){
					if(getrusage(RUSAGE_SELF, &usage) == -1)
						fprintf(stderr,"Error getting end resource usage.\n");
					else{
						end_user = usage.ru_utime;
						end_sys = usage.ru_stime;
						double uTime = findTimeDiff(&begin_user,&end_user);
						double sTime = findTimeDiff(&begin_sys, &end_sys);
						printf("User: %f\tSystem: %f\n",uTime,sTime);
					}
				}
				break;
			}
			case WRONLY:
			{
				if (verbose_flag){
					printf("--%s %s\n","wronly",optarg);
				}
				int ofd = open (optarg, O_WRONLY | opt_flags,0644);
				opt_flags = 0;
				resetFlags();

				if (ofd < 0){
					fprintf(stderr, "Error with opening file with wronly.\n");
					exit(1);
				}
				else{
					fd[fd_c] = ofd;
					fd_c++;
				}
				if (profile_flag){
					if(getrusage(RUSAGE_SELF, &usage) == -1)
						fprintf(stderr,"Error getting end resource usage.\n");
					else{
						end_user = usage.ru_utime;
						end_sys = usage.ru_stime;
						double uTime = findTimeDiff(&begin_user,&end_user);
						double sTime = findTimeDiff(&begin_sys, &end_sys);
						printf("User: %f\tSystem: %f\n",uTime,sTime);
					}
				}

				break;
			}
			case RDWR:
			{
				if (verbose_flag){
					printf("--%s %s\n","rdwr",optarg);
				}
				int iofd = open(optarg, O_RDWR | opt_flags,0644);
				opt_flags = 0;
				resetFlags();

				if (iofd < 0){
					fprintf(stderr, "Error with opening file with rdwr.\n");
					exit(1);
				}
				else{
					fd[fd_c] = iofd;
					fd_c++;
				}
				if (profile_flag){
					if(getrusage(RUSAGE_SELF, &usage) == -1)
						fprintf(stderr,"Error getting end resource usage.\n");
					else{
						end_user = usage.ru_utime;
						end_sys = usage.ru_stime;
						double uTime = findTimeDiff(&begin_user,&end_user);
						double sTime = findTimeDiff(&begin_sys, &end_sys);
						printf("User: %f\tSystem: %f\n",uTime,sTime);
					}
				}

				break;
			}
			case PIPE:
			{
				if(verbose_flag)
					printf("--%s\n","pipe");
				int p_fds[2];
				if(pipe(p_fds) == -1){
					fprintf(stderr,"Error with making the pipe.\n");
				}
				fd[fd_c++] = p_fds[0];
				//because we're incrementing twice, have to check here as well
				if(fd_c == fd_s){
					fd_s *= 10;
					fd = (int*)realloc((void*)fd, fd_s*sizeof(int));
					if (fd == NULL){
						fprintf(stderr,"Error with realloc.\n");
						exit(1);
					}
				}
	
				fd[fd_c++] = p_fds[1];
				if (profile_flag){
					if(getrusage(RUSAGE_SELF, &usage) == -1)
						fprintf(stderr,"Error getting end resource usage.\n");
					else{
						end_user = usage.ru_utime;
						end_sys = usage.ru_stime;
						double uTime = findTimeDiff(&begin_user,&end_user);
						double sTime = findTimeDiff(&begin_sys, &end_sys);
						printf("User: %f\tSystem: %f\n",uTime,sTime);
					}
				}

				break;
			}
			case VERBOSE:
			{
				verbose_flag = 1;
				break;	
			}
			case PROFILE:
			{
				profile_flag = 1;
				break;
			}
			case COMMAND:
			{
				int ioe[3];
				int ioe_index;

				optind--;
				if(verbose_flag){
					printf("--command ");
					for (i = optind; i<argc;i++){
						if(argv[i][0] != '-' || argv [i][1] != '-')
							printf("%s ",argv[i]);
						else break;
		
					}
					printf("\n");
				}
				
				strcpy(wait_comm[cur_command_size].info, "");
				for (; optind < argc && (argv[optind][1]!='-' || argv[optind][0] != '-');optind++){
					char* s_temp = argv[optind];
					if(ioe_index < 3){
						if(atoi(s_temp) >= fd_c || fd[atoi(s_temp)]<0 ){
							fprintf(stderr,"Invalid file descriptor argument to command.\n");
							exit(1);
						}
		
						ioe[ioe_index] = atoi(s_temp);

						ioe_index++;
					}
					else{
						carr[carr_index] = s_temp;
						carr_index++;				
						strcat(wait_comm[cur_command_size].info, s_temp);
						strcat(wait_comm[cur_command_size].info, " ");
					}
					
				} 
			


				pid_t pid = fork();
				if (pid == 0){
					//child
					if (dup2(fd[ioe[0]], 0) == -1)
						fprintf(stderr, "Error with dup2.\n");
					if (dup2(fd[ioe[1]], 1) == -1)
						fprintf(stderr, "Error with dup2.\n");
					if (dup2(fd[ioe[2]], 2) == -1)
						fprintf(stderr, "Error with dup2.\n");
					
					for (i = 0;i<fd_c;i++){
						if(close(fd[i]) == -1){
							fprintf(stderr, "Error with close.\n");
							exit(1);
						}
						
					}	
					

					if(execvp(carr[0], carr) == -1)
						fprintf(stderr,"Error with execvp.\n");
				}
				else if(pid == -1){
					fprintf(stderr, "Error with fork.\n");
					exit(1);
				}
				else{
					wait_comm[cur_command_size].pid = pid;
					cur_command_size++;
					
				}
				for (i = 0; i<carr_index;i++)
					carr[i] = NULL;
				ioe_index = 0;
				carr_index = 0;

				if (profile_flag){
					if(getrusage(RUSAGE_SELF, &usage) == -1)
						fprintf(stderr,"Error getting end resource usage.\n");
					else{
						end_user = usage.ru_utime;
						end_sys = usage.ru_stime;
						double uTime = findTimeDiff(&begin_user,&end_user);
						double sTime = findTimeDiff(&begin_sys, &end_sys);
						printf("User: %f\tSystem: %f\n",uTime,sTime);
					}
				}


				break;
			}
			case WAIT:
			{
				if (verbose_flag)
					printf("--%s\n","wait");
				fd_c--;
				for (i = 0 ;i < fd_c;i++){
					close(fd[i]);
					fd[i] = -1;
				}

				int status_info;
				pid_t w = 0;
				do{
					w = waitpid(-1,&status_info,0);
					if (w == -1){
				//		fprintf(stderr,"Error while waiting for child process.\n");
						break;
					}
					
					if(WIFEXITED(status_info)){
						printf("%d ", WEXITSTATUS(status_info));
					}
					for (i = 0;i<cur_command_size;i++){
						if(wait_comm[i].pid != -10 && w == wait_comm[i].pid){
							printf("%s\n",wait_comm[i].info);
							break;
						}
					}
						
				}while(w != -1);
				if (profile_flag){
					if(getrusage(RUSAGE_SELF, &usage) == -1)
						fprintf(stderr,"Error getting end resource usage.\n");
					else{
						end_user = usage.ru_utime;
						end_sys = usage.ru_stime;
						double uTime = findTimeDiff(&begin_user,&end_user);
						double sTime = findTimeDiff(&begin_sys, &end_sys);
						printf("User: %f\tSystem: %f\n",uTime,sTime);
					}
					if(getrusage(RUSAGE_CHILDREN, &usage_children) == -1)
						fprintf(stderr,"Error getting end resource usage of children.\n");
					else{
						struct timeval c_utime = usage_children.ru_utime;
						struct timeval c_stime = usage_children.ru_stime;
						double uTime = findTimeDiff(&begin_user, &c_utime);
						double sTime = findTimeDiff(&begin_user, &c_stime);
						printf("User: %f\tSystem: %f\n",uTime,sTime);
						
					}
				}

				break;
			}
			case CLOSE:
			{
				if(verbose_flag)
					printf("--%s %s\n","close",optarg);

				
				if (isdigit(*optarg) && atoi(optarg) < fd_c){
					if (close(fd[atoi(optarg)]) == -1)
						fprintf(stderr, "Error closing file.\n");
				}
				else{
					fprintf(stderr, "Error with file descriptor argument to close.\n");
					exit(1);
				}
				
				
				fd[atoi(optarg)] = -2;

				if (profile_flag){
					if(getrusage(RUSAGE_SELF, &usage) == -1)
						fprintf(stderr,"Error getting end resource usage.\n");
					else{
						end_user = usage.ru_utime;
						end_sys = usage.ru_stime;
						double uTime = findTimeDiff(&begin_user,&end_user);
						double sTime = findTimeDiff(&begin_sys, &end_sys);
						printf("User: %f\tSystem: %f\n",uTime,sTime);
					}
				}

				break;
			}
			case ABORT:
			{
				if(verbose_flag)
					printf("--%s\n", "abort");
				char * breaker = NULL;
				*breaker = 'b';
				break;
			}
			case CATCH:
			{
				if(verbose_flag)
					printf("--%s %s\n","catch", optarg);
				
				int signum = atoi(optarg);
				signal(signum, &sig_handler);
				if (profile_flag){
					if(getrusage(RUSAGE_SELF, &usage) == -1)
						fprintf(stderr,"Error getting end resource usage.\n");
					else{
						end_user = usage.ru_utime;
						end_sys = usage.ru_stime;
						double uTime = findTimeDiff(&begin_user,&end_user);
						double sTime = findTimeDiff(&begin_sys, &end_sys);
						printf("User: %f\tSystem: %f\n",uTime,sTime);
					}
				}

				break;
			}
			case IGNORE:
			{
				if (verbose_flag)
					printf("--%s %s\n","ignore",optarg);
				signal(atoi(optarg),SIG_IGN);
				if (profile_flag){
					if(getrusage(RUSAGE_SELF, &usage) == -1)
						fprintf(stderr,"Error getting end resource usage.\n");
					else{
						end_user = usage.ru_utime;
						end_sys = usage.ru_stime;
						double uTime = findTimeDiff(&begin_user,&end_user);
						double sTime = findTimeDiff(&begin_sys, &end_sys);
						printf("User: %f\tSystem: %f\n",uTime,sTime);
					}
				}

				break;
			}
			case DEFAULT:
			{
				if (verbose_flag)
					printf("--%s %s\n","default",optarg);
				signal(atoi(optarg),SIG_DFL);
				if (profile_flag){
					if(getrusage(RUSAGE_SELF, &usage) == -1)
						fprintf(stderr,"Error getting end resource usage.\n");
					else{
						end_user = usage.ru_utime;
						end_sys = usage.ru_stime;
						double uTime = findTimeDiff(&begin_user,&end_user);
						double sTime = findTimeDiff(&begin_sys, &end_sys);
						printf("User: %f\tSystem: %f\n",uTime,sTime);
					}
				}

				break;
		
			}
			case PAUSE:
			{
				if(verbose_flag)
					printf("--%s\n","pause");
				pause();
				if (profile_flag){
					if(getrusage(RUSAGE_SELF, &usage) == -1)
						fprintf(stderr,"Error getting end resource usage.\n");
					else{
						end_user = usage.ru_utime;
						end_sys = usage.ru_stime;
						double uTime = findTimeDiff(&begin_user,&end_user);
						double sTime = findTimeDiff(&begin_sys, &end_sys);
						printf("User: %f\tSystem: %f\n",uTime,sTime);
					}
				}

				break;			
			}
		}
	}
	return 0;
}
