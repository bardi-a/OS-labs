#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <fcntl.h>
#include <sys/types.h>
#include <cstring>
#include <sys/mman.h>
#include <semaphore.h>
#include <cstdio>
#include <cstdint>

#define MAX_NAME 4096
#define SHM_SIZE 4096

ssize_t read_line(char* buf, size_t max){
        size_t i = 0;
        char c;
        while( i < max - 1){
                ssize_t n = read(STDIN_FILENO, &c, 1);
                if (n < 0) return -1;
                if (n == 0) break;
                buf[i++] = c;
                if (c == '\n') break;
        }
        buf[i] = '\0';
        return i;
}

int main(){
        char filename[MAX_NAME];
        write(STDOUT_FILENO, "Enter filename: ", 16);
        ssize_t n = read(STDIN_FILENO, filename, MAX_NAME-1);
        if (n == -1){
                write(STDERR_FILENO, "read error\n", 11);
                std::exit(EXIT_FAILURE);
        }
        if (n == 0){
                write(STDERR_FILENO, "no filename\n", 12);
                std::exit(EXIT_FAILURE);
        }
        if (filename[n - 1] == '\n'){
               filename[n - 1] = '\0';
               n--;
        }else filename[n] = '\0';
        if (n == 0){
                 write(STDERR_FILENO, "empty filename\n", 15);
                 std::exit(EXIT_FAILURE);
        }

        char shm_name[MAX_NAME - 6];
        write(STDOUT_FILENO, "Enter shared memory object name: ", 33);
        n = read(STDIN_FILENO, shm_name, sizeof(shm_name)-1);
        if (n == -1){
                write(STDERR_FILENO, "read error\n", 11);
                std::exit(EXIT_FAILURE);
        }
        if (n == 0){
                write(STDERR_FILENO, "no name\n", 8);
                std::exit(EXIT_FAILURE);
        }
        if (shm_name[n - 1] == '\n'){
               shm_name[n - 1] = '\0';
               n--;
        }else shm_name[n] = '\0';
        if (n == 0){
                write(STDERR_FILENO, "empty name\n", 11);
                std::exit(EXIT_FAILURE);
        }
        char shm_full[sizeof(shm_name) + 1];
        std::snprintf(shm_full, sizeof(shm_full), "/%s", shm_name);

        int shm = shm_open(shm_full, O_RDWR | O_CREAT | O_TRUNC, 0600);
        if (shm == -1){
                write(STDERR_FILENO, "failed to open SHM\n", 19);
                std::exit(EXIT_FAILURE);
        }
        if (ftruncate(shm, SHM_SIZE) == -1){
                write(STDERR_FILENO, "failed to resize SHM\n", 21);
                std::exit(EXIT_FAILURE);
        }
        
        char* shm_buf = (char*)mmap(NULL, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm, 0);
        if (shm_buf == MAP_FAILED) {
		write(STDERR_FILENO, "failed to map SHM\n", 18);
		std::exit(EXIT_FAILURE);
	}

        char empty_name[MAX_NAME];
        std::snprintf(empty_name, sizeof(empty_name), "%s-sem1", shm_full);  
        sem_t* empty = sem_open(empty_name, O_CREAT, 0600, 1);
        if (empty == SEM_FAILED){
                write(STDERR_FILENO, "failed to create semaphore\n", 27);
                std::exit(EXIT_FAILURE);
        }

        char full_name[MAX_NAME];
        std::snprintf(full_name, sizeof(full_name), "%s-sem2", shm_full);  
        sem_t* full = sem_open(full_name, O_CREAT, 0600, 0);
        if (full == SEM_FAILED){
                write(STDERR_FILENO, "failed to create semaphore\n", 27);
                std::exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid == -1){  
                write(STDERR_FILENO, "fork error\n", 11);
                std::exit(EXIT_FAILURE);
        }else if (pid == 0){
                char* argv[] = {"./child", filename, shm_full, empty_name, full_name, NULL};
                execv("./child", argv);
                write(STDERR_FILENO, "exec error\n", 11);
                std::exit(EXIT_FAILURE);
        }

        bool running = true;
        write(STDOUT_FILENO, "Enter numbers: ", 15);
	while (running) {
		char buf[SHM_SIZE - sizeof(uint32_t)];
		ssize_t bytes = read_line(buf, sizeof(buf));

		if (bytes == -1) {
			write(STDERR_FILENO, "failed to read from standard input\n", 35);
			std::exit(EXIT_FAILURE);
		}

		sem_wait(empty);
                uint32_t* length = (uint32_t*)shm_buf;
		if (bytes > 0) {
                        char* text = shm_buf + sizeof(uint32_t);
                        *length = bytes;
			memcpy(text, buf, bytes);
		} else {
                        *length = UINT32_MAX;
			running = false;
		}
		sem_post(full);
	}

	waitpid(pid, NULL, 0);
	sem_unlink(empty_name);
	sem_close(empty);
        sem_unlink(full_name);
        sem_close(full);
	munmap(shm_buf, SHM_SIZE);
	shm_unlink(shm_full);
	close(shm);
}
