#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>


// Location of timetamp file for software watchdog
//char timestamp_watchdog[200] = "/home/carol/watchdog/timestamp.txt";
// Max errors that can be found for a single iteration
// If more than max errors is found, exit the program
unsigned long int max_errors_per_iter = 5000;

// Absolute path for log file, if needed
char absolute_path[200] = "/home/carol/logs/";

// Used to print the log only for some iterations, equal 1 means print every iteration
int iter_interval_print = 1;

char log_file_name[200] = "";
char full_log_file_name[500] = "";

// Saves the last amount of error found for a specific iteration
unsigned long int last_iter_errors = 0;
// Saves the last iteration index that had an error
unsigned long int last_iter_with_errors = 0;

unsigned long int kernels_total_errors = 0;
unsigned long int iteration_number = 0;
double kernel_time_acc = 0;
double kernel_time = 0;
long long it_time_start;


char message[2000];

//code to send signal to server that controls switch PING
#define PORT "8888" // the port client will be connecting to 
#define SERVER_IP "127.0.0.1"
// get sockaddr, IPv4 or IPv6:

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void send_message(char * message){
	int sockfd;  
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(SERVER_IP, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	//printf("client: connecting to %s\n", s);

        
	freeaddrinfo(servinfo); // all done with this structure

        if( send(sockfd , full_log_file_name , 500 , 0) < 0)
        {
            fprintf(stderr, "Send message failed");
        }else{
		send(sockfd , message, 2000 , 0);
	}

	close(sockfd);
}

// ~ ===========================================================================
inline long long get_time() {
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return (tv.tv_sec * 1000000) + tv.tv_usec;
};

// ~ ===========================================================================
unsigned long int set_max_errors_iter(unsigned long int max_errors){
     max_errors_per_iter = max_errors;

     return max_errors_per_iter;
};

// ~ ===========================================================================
// Set the interval the program must print log details, default is 1 (each iteration)
int set_iter_interval_print(int interval){
    if(interval < 1) {
        iter_interval_print = 1;
    }
    else {
        iter_interval_print = interval;
    }

    return iter_interval_print;
};

// ~ ===========================================================================
// Update with current timestamp the file where the software watchdog watchs
//void update_timestamp() {
//    time_t timestamp = time(NULL);
//    char time_s[50];
//    char string[100] = "echo ";
//
//    sprintf(time_s, "%d", (int) timestamp);
//
//    strcat(string, time_s);
//    strcat(string, " > ");
//    strcat(string, timestamp_watchdog);
//    system(string);
//};

// ~ ===========================================================================
// In case the user needs the log to be generated in some exact absolute path
void set_absolute_path(char *path){
    //strcpy(absolute_path, path);
};

// ~ ===========================================================================
// Return the name of the log file generated
char * get_log_file_name(){
    return "";//return full_log_file_name;
};

// ~ ===========================================================================
// Generate the log file name, log info from user about the test to be executed and reset log variables
int start_log_file(char *benchmark_name, char *test_info){

    //update_timestamp();

    time_t file_time;
    struct tm *ptm;
    char day[10], month[10], year[15], hour[10], second[10], minute[10];
    //char log_file_name[180] = "";

    file_time = time(NULL);
    ptm = gmtime(&file_time);

    snprintf(day,       sizeof(day),    "%02d", ptm->tm_mday);
    snprintf(month,     sizeof(month),  "%02d", ptm->tm_mon+1);
    snprintf(year,      sizeof(year),   "%04d", ptm->tm_year+1900);
    snprintf(hour,      sizeof(hour),   "%02d", ptm->tm_hour);
    snprintf(minute,    sizeof(minute), "%02d", ptm->tm_min);
    snprintf(second,    sizeof(second), "%02d", ptm->tm_sec);

    // ~ Get the host name to add inside the log name.
    char host[35] = "Host";
    int host_error = 0;
    host_error = gethostname(host, 35);

    if (host_error != 0) {
        fprintf(stderr, "[ERROR in gethostname(char *, int)] Could not access the host name\n");
        return 1;
    }

    strcpy(log_file_name, year);             strcat(log_file_name, "_");
    strcat(log_file_name, month);            strcat(log_file_name, "_");
    strcat(log_file_name, day);              strcat(log_file_name, "_");

    strcat(log_file_name, hour);             strcat(log_file_name, "_");
    strcat(log_file_name, minute);           strcat(log_file_name, "_");
    strcat(log_file_name, second);           strcat(log_file_name, "_");

    strcat(log_file_name, benchmark_name);   strcat(log_file_name, "_");
    strcat(log_file_name, host);
    strcat(log_file_name, ".log");


    strcpy(full_log_file_name, absolute_path);
    if(strlen(absolute_path) > 0 && absolute_path[strlen(absolute_path)-1] != '/' )
        strcat(full_log_file_name, "/");
    strcat(full_log_file_name, log_file_name);
// ~ printf("%s\n", full_log_file_name);

    //struct stat buf;
    //if (stat(full_log_file_name, &buf) == 0) {
    //    fprintf(stderr, "[ERROR in create_log_file(char *)] File already exists %s\n",full_log_file_name);
    //    return 1;
    //}

    //FILE *file = NULL;

    //file = fopen(full_log_file_name, "a");
    //if (file == NULL){
    //    fprintf(stderr, "[ERROR in create_log_file(char *)] Unable to open file %s\n",full_log_file_name);
    //    return 1;
    //}
    //else 
    if(test_info != NULL) {
        //fprintf(file, "#HEADER %s\n",test_info);
        snprintf(message, sizeof(message),"#HEADER %s\n", test_info);
	send_message(message);
    }
    else {
        //fprintf(file, "#HEADER\n");
	send_message("#HEADER\n");
    }

    snprintf(message, sizeof(message),"#BEGIN Y:%s M:%s D:%s Time:%s:%s:%s\n", year, month, day, hour, minute, second);
    send_message(message);
    //fprintf(file, "#BEGIN Y:%s M:%s D:%s Time:%s:%s:%s\n", year, month, day, hour, minute, second);
    //fflush(file);
    //fclose(file);

    kernels_total_errors = 0;
    iteration_number = 0;
    kernel_time_acc = 0;

    return 0;
};

// ~ ===========================================================================
// Log the string "#END" and reset global variables
int end_log_file(){
    //FILE *file = NULL;

    //file = fopen(full_log_file_name, "a");
    //if (file == NULL){
    //    fprintf(stderr, "[ERROR in log_string(char *)] Unable to open file %s\n",full_log_file_name);
    //    return 1;
    //}

    send_message("#END");
    //fprintf(file, "#END");
    //fflush(file);
    //fclose(file);
    kernels_total_errors = 0;
    iteration_number = 0;
    kernel_time_acc = 0;
    strcpy(log_file_name, "");
    //strcpy(absolute_path, "");
    //strcpy(full_log_file_name, "");

    return 0;
};

// ~ ===========================================================================
// Start time to measure kernel time, also update iteration number and log to file
int start_iteration(){

    //update_timestamp();

/*
    FILE *file = fopen(full_log_file_name, "a");

    if (file == NULL){
        fprintf(stderr, "[ERROR in log_string(char *)] Unable to open file %s\n",full_log_file_name);
        return 1;
    }

    fprintf(file, "#ITER it:%lu\n", iteration_number);
    fflush(file);
    fclose(file);
    iteration_number++;
*/
    it_time_start = get_time();
    return 0;

};

// ~ ===========================================================================
// Finish the measured kernel time log both time (total time and kernel time)
int end_iteration(){

    //update_timestamp();

    kernel_time = (double) (get_time() - it_time_start) / 1000000;
    kernel_time_acc += kernel_time;



    if(iteration_number % iter_interval_print == 0) {

        //FILE *file = fopen(full_log_file_name, "a");

        //if (file == NULL){
        //    fprintf(stderr, "[ERROR in log_string(char *)] Unable to open file %s\n",full_log_file_name);
        //    return 1;
        //}

        snprintf(message, sizeof(message),"#IT Ite:%lu KerTime:%f AccTime:%f\n", iteration_number, kernel_time, kernel_time_acc);
	send_message(message);
        //fprintf(file,"#IT Ite:%lu KerTime:%f AccTime:%f\n", iteration_number, kernel_time, kernel_time_acc);
        //fprintf(file, "#TIME kernel_time:%f\n", kernel_time);
        //fprintf(file, "#ACC_TIME total_time:%f\n", kernel_time_acc);
        //fflush(file);
        //fclose(file);
    }

    iteration_number++;

    return 0;

};

// ~ ===========================================================================
// Update total errors variable and log both errors(total errors and kernel errors)
int log_error_count(unsigned long int kernel_errors){

    //update_timestamp();

    if(kernel_errors < 1) {
        return 0;
    }

    kernels_total_errors += kernel_errors;

    //FILE *file = NULL;
    //file = fopen(full_log_file_name, "a");

    //if (file == NULL){
    //    fprintf(stderr, "[ERROR in log_string(char *)] Unable to open file %s\n",full_log_file_name);
    //    return 1;
    //}

    // (iteration_number-1) because this function is called after end_iteration() that increments iteration_number
    snprintf(message, sizeof(message),"#SDC Ite:%lu KerTime:%f AccTime:%f KerErr:%lu AccErr:%lu\n", iteration_number-1, kernel_time, kernel_time_acc, kernel_errors, kernels_total_errors);
    send_message(message);
    //fprintf(file, "#SDC Ite:%lu KerTime:%f AccTime:%f KerErr:%lu AccErr:%lu\n", iteration_number-1, kernel_time, kernel_time_acc, kernel_errors, kernels_total_errors);
    //fprintf(file, "#SDC kernel_errors:%lu\n", kernel_errors);
    //fprintf(file, "#TOTAL_SDC total_errors:%lu\n", kernels_total_errors);
    //fflush(file);


    if(kernel_errors > max_errors_per_iter){
	send_message("#ABORT too many errors per iteration\n");
        //fprintf(file, "#ABORT too many errors per iteration\n");
        //fflush(file);
        //fclose(file);
        end_log_file();
        exit(1);
    }


    if(kernel_errors == last_iter_errors && (last_iter_with_errors+1) == iteration_number && kernel_errors != 0){
	send_message("#ABORT amount of errors equals of the last iteration\n");
        //fprintf(file, "#ABORT amount of errors equals of the last iteration\n");
        //fflush(file);
        //fclose(file);
        end_log_file();
        exit(1);
    }

    //fclose(file);

    last_iter_errors = kernel_errors;
    last_iter_with_errors = iteration_number;

    return 0;

};

// ~ ===========================================================================
// Print some string with the detail of an error to log file
int log_error_detail(char *string){
    //FILE *file = NULL;

    //file = fopen(full_log_file_name, "a");
    //if (file == NULL){
    //    fprintf(stderr, "[ERROR in log_string(char *)] Unable to open file %s\n",full_log_file_name);
    //    return 1;
    //}

    snprintf(message, sizeof(message),"#ERR %s\n", string);
    send_message(message);
    //fputs("#ERR ", file);
    //fputs(string, file);
    //fprintf(file, "\n");
    //fflush(file);
    //fclose(file);
    return 0;
};
