#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <cstdint>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

#define SHM_SIZE 4096

int main(int argc, char *argv[]){
	if (argc != 5){
		write(STDERR_FILENO, "wrong amount of arguments\n", 26);
		std::exit(EXIT_FAILURE);
	}
	char* filename = argv[1];
	char* shm_name = argv[2];
	char* empty_name = argv[3];
	char* full_name = argv[4];

	int shm = shm_open(shm_name, O_RDWR, 0);
	if (shm == -1){
		write(STDERR_FILENO, "failed to open SHM\n", 19);
		std::exit(EXIT_FAILURE);
	}

	char* shm_buf = (char*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	if (shm_buf == MAP_FAILED) {
    	write(STDERR_FILENO, "failed to map SHM\n", 18);
    	std::exit(EXIT_FAILURE);
	}

	sem_t* empty = sem_open(empty_name, 0);
	if (empty == SEM_FAILED){
		write(STDERR_FILENO, "failed to open semaphore\n", 25);
		std::exit(EXIT_FAILURE);
	}

	sem_t* full = sem_open(full_name, 0);
	if (full == SEM_FAILED){
		write(STDERR_FILENO, "failed to open semaphore\n", 25);
		std::exit(EXIT_FAILURE);
	}

	int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0600);
	if(fd == -1){
		write(STDERR_FILENO, "failed to open file descriptor\n", 31);
		std::exit(EXIT_FAILURE);
	} 

	bool running = true;
	while(running){

		sem_wait(full);

		uint32_t* length = (uint32_t*)shm_buf;
		char* text = shm_buf + sizeof(uint32_t);

		if (*length == UINT32_MAX){
			running = false;
		}else if (*length > 0){
			char buffer[SHM_SIZE - sizeof(uint32_t) + 1];
			memcpy(buffer, text, *length);
			buffer[*length] = '\0';
			*length = 0;

			char* token = strtok(buffer, " \t");
			double sum = 0;
			while(token != NULL){
				char *end;
				double value = strtod(token, &end);
				if (end == token){
					write(STDERR_FILENO, "bad number: ", 12);
					write(STDERR_FILENO, token, strlen(token));
					write(STDERR_FILENO, ", skipped\n", 10);
				}else{
					sum += value;
				}
				token = strtok(NULL, " \t");
			}
		std::string s =  std::to_string(sum)+"\n";
		write(fd, s.c_str(), s.size());
		}
		sem_post(empty);
	}

	close(fd);
	sem_close(empty);
	sem_close(full);
	munmap(shm_buf, SHM_SIZE);
	close(shm);
}
