#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <fcntl.h>
#include <sys/types.h>

#define MAX_LINE 1024

ssize_t read_line(int fd, char* buf, size_t max){
        size_t i = 0;
        char c;
        while (i < max - 1){
                ssize_t n = read(fd, &c, 1);
                if (n == 0) break;
                if (n < 0) return -1;
                buf[i++] = c;
                if (c == '\n') break;
        }
        buf[i] = '\0';
        return i;
}

int main(){
        char filename[MAX_LINE];
        write(STDOUT_FILENO, "Enter filename: ", 16);
        ssize_t n = read_line(0, filename, MAX_LINE);
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
        }
        if (n == 0){
                 write(STDERR_FILENO, "empty filename\n", 15);
                 std::exit(EXIT_FAILURE);
        }

        int fd[2];
        if (pipe(fd) < 0){
                write(STDERR_FILENO, "pipe error\n", 11);
                std::exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid == -1){
                write(STDERR_FILENO, "fork error\n", 11);
                std::exit(EXIT_FAILURE);
        }else if (pid == 0){
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                execlp("./child", "child", filename, NULL);
                write(STDERR_FILENO, "exec error\n", 11);
                std::exit(EXIT_FAILURE);
        }else{
                close(fd[0]);
                char buffer[MAX_LINE];
                write(STDOUT_FILENO, "Enter numbers: ", 15);
                ssize_t len;
                while((len = read_line(STDIN_FILENO, buffer, MAX_LINE)) > 0){
                        write(fd[1], buffer, len);
                }
                close(fd[1]);
                wait(NULL);
        }
}
