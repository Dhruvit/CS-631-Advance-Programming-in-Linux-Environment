#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <math.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

/*
* The −1, −C, −l, −n, and −x options override each other,
* the last one specified determines the format used.
* So, we deifne Charcter flage for this to know which options we need to use.
*/
char Clnx1;

void displayError(void);
void displayDirContent(char*);
void displayDirInfo(char *, char);
char* getMonthName(int);

int
main(int argc, char *argv[]){

    char ch, *cwd;
    int argc_tmp = argc;
    char *argv_tmp;
    if(argc == 3){
        argv_tmp = argv[2];
    }

    /*By default ls for current working directory*/
    if((argc == 1)  && ((cwd = getcwd(0,0)) != NULL)){
        displayDirContent(cwd);
        exit(0);
    }

    while((ch = getopt(argc, argv, "AacCdFfhiklnqRrSstuwx1")) != -1){
        switch(ch){
            case 'A':
                printf("A\n");
                break;
            case 'a':
                printf("a\n");
                break;
            case 'c':
                printf("c\n");
                break;
            case 'C':
                //printf("C\n");
                Clnx1 = 'C';
                break;
            case 'd':
                printf("d\n");
                break;
            case 'F':
                printf("F\n");
                break;
            case 'f':
                printf("f\n");
                break;
            case 'h':
                printf("h\n");
                break;
            case 'i':
                printf("i\n");
                break;
            case 'k':
                printf("k\n");
                break;
            case 'l':
                //printf("l\n");
                Clnx1 = 'l';
                break;
            case 'n':
                //printf("n\n");
                Clnx1 = 'n';
                break;
            case 'q':
                printf("q\n");
                break;
            case 'R':
                printf("R\n");
                break;
            case 'r':
                printf("r\n");
                break;
            case 'S':
                printf("S\n");
                break;
            case 's':
                printf("s\n");
                break;
            case 't':
                printf("t\n");
                break;
            case 'u':
                printf("u\n");
                break;
            case 'w':
                printf("w\n");
                break;
            case 'x':
                //printf("x\n");
                Clnx1 = 'x';
                break;
            case '1':
                //printf("1\n");
                Clnx1 = '1';
                break;
            default:
                case '?':
                displayError();
        }
    }
    argc -= optind;
	argv += optind;
	//printf("%c ",Clnx1);

    if((argc_tmp == 2)  && ((cwd = getcwd(0,0)) != NULL)){ // argc = 2
        displayDirInfo(cwd,Clnx1);
    }else if(argc_tmp == 3){ // argc = 3
        chdir(argv_tmp); //need to change the current working directory
        displayDirInfo(argv_tmp,Clnx1);
    }

    exit(0);
}

/*simple ls command */
void
displayDirContent(char *dir){
    DIR *dp;
	struct dirent *dirp;

    /*for getting terminal window size*/
    struct winsize terminal;
    ioctl(0, TIOCGWINSZ, &terminal);

    int totallengthOfContent = 0; /* we need to determine howmany coloumn occupied by output in original terminal size */

    if ((dp = opendir(dir)) == NULL ) {
		fprintf(stderr, "Cannot open '%s': %s\n", dir,strerror(errno));
		exit(1);
	}

    while ((dirp = readdir(dp)) != NULL ){
        totallengthOfContent +=2;
        totallengthOfContent += strlen(dirp->d_name);
    }
    totallengthOfContent -= 1;
    dp = opendir(dir);

    if(totallengthOfContent > terminal.ws_col){
        int rounded = 1 + (float)totallengthOfContent/(float)terminal.ws_col ;
        printf("%d \n", rounded);
    }

	while ((dirp = readdir(dp)) != NULL ){
       if ( !strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..") )
        {} else {
                        printf("%s  ", dirp->d_name);
        }
	}
    printf("\n");

    //printf ("\ncolumns %d\n", terminal.ws_col);

	if(closedir(dp)){
        fprintf(stderr, "Could not close '%s': %s\n",dir,strerror(errno));
	}
}

/*This method for ls -l*/
void
displayDirInfo(char *dir, char flag){

    DIR *dp;
	struct dirent *dirp;
    struct stat sb;
    int noBlocks = 0;
    int maxFileSize = 0;
    if(flag == 'l'){

        if ((dp = opendir(dir)) == NULL ) {
            fprintf(stderr, "Cannot open '%s': %s\n", dir,strerror(errno));
            exit(1);
        }

        while ((dirp = readdir(dp)) != NULL ){
            if (stat(dirp->d_name, &sb) == -1) {
                fprintf(stderr, "Can't stat %s: %s\n", dirp->d_name,strerror(errno));

                if (lstat(dirp->d_name, &sb) == -1) {
                    fprintf(stderr,"Can't stat %s: %s\n", dirp->d_name,strerror(errno));
                    continue;
                    }
            }

            if ( !strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..") )
            {} else {
                noBlocks += (int)sb.st_blocks/2;
                if(maxFileSize < (int)sb.st_size){maxFileSize = (int)sb.st_size;}
            }
        }
        printf("total %d\n", noBlocks);

        dp = opendir(dir);
        while ((dirp = readdir(dp)) != NULL ){

        if ( !strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..") )
        {} else{

        stat(dirp->d_name, &sb);

        /*display file mode*/
        printf( (S_ISDIR(sb.st_mode) ) ? "d" : "-");
        printf( (S_IRUSR & sb.st_mode) ? "r" : "-");
        printf( (S_IWUSR & sb.st_mode) ? "w" : "-");
        printf( (S_IXUSR & sb.st_mode) ? "x" : "-");
        printf( (S_IRGRP & sb.st_mode) ? "r" : "-");
        printf( (S_IWGRP & sb.st_mode) ? "w" : "-");
        printf( (S_IXGRP & sb.st_mode) ? "x" : "-");
        printf( (S_IROTH & sb.st_mode) ? "r" : "-");
        printf( (S_IWOTH & sb.st_mode) ? "w" : "-");
        printf( (S_IXOTH & sb.st_mode) ? "x" : "-");
        printf(" ");

        /* display number of links*/
        printf("%d ", (int)sb.st_nlink);

        /* display owner name*/
        printf("%s ", getpwuid(sb.st_uid)->pw_name);
        /* display group name, for numeric = printf("%d ", (int)sb.st_gid);*/
        printf("%s ", getgrgid(sb.st_gid)->gr_name);

        /* display number of bytes*/
        printf("%5d ",(int)sb.st_size);

        /* display abbreviated month file was ast modified */
        time_t time = sb.st_mtime;
        struct tm *timeInfo =  gmtime(&time);
        printf("%s ", getMonthName(timeInfo->tm_mon));

        /* display day-of-month file was last modified */
        printf("%2d ", timeInfo->tm_mday);

        /* display hour and minute file was last modified*/
        printf("%d:%d ",timeInfo->tm_hour, timeInfo->tm_min);

        /* display pathname*/
        printf("%s  \n", dirp->d_name);
        }

        }
        if(closedir(dp)){
            fprintf(stderr, "Could not close '%s': %s\n",dir,strerror(errno));
        }
    }
}

void
displayError(){
    (void)fprintf(stderr, "Wrong usage : %s\n", strerror(errno));
    exit(EXIT_FAILURE);
}

char*
getMonthName(int n){
    switch(n){
        case 1:
            return "JAN";
            break;
        case 2:
            return "FEB";
            break;
        case 3:
            return "MAR";
            break;
        case 4:
            return "APR";
            break;
        case 5:
            return "MAY";
            break;
        case 6:
            return "JUN";
            break;
        case 7:
            return "JUL";
            break;
        case 8:
            return "AUG";
            break;
        case 9:
            return "SEP";
            break;
        case 10:
            return "OCT";
            break;
        case 11:
            return "NOV";
            break;
        case 12:
            return "DEC";
            break;
        default:
            return "No";
            break;
    }
}
