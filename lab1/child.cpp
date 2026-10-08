#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>

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

int main(int argc, char **argv){
        if (argc < 2){
                write(STDERR_FILENO, "no filename\n", 12);
                std::exit(EXIT_FAILURE);
        }
        int file_fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (file_fd == -1){
                write(STDERR_FILENO, "open failed\n", 12);
                std::exit(EXIT_FAILURE);
        }
        ssize_t len;
        char buffer[MAX_LINE];
        while((len = read_line(STDIN_FILENO, buffer, MAX_LINE)) > 0){
                char* token = strtok(buffer, " \t\n");
                double sum = 0;
                while(token != NULL){
                        double value = strtod(token, NULL);
                        sum += value;
                        token = strtok(NULL, " \t\n");
                }
                std::string s =  std::to_string(sum)+"\n";
                write(file_fd, s.c_str(), s.length());
        }
        if (len == -1){
                write(STDERR_FILENO, "read error\n", 11);
                std::exit(EXIT_FAILURE);
        }
        close(file_fd);
        return 0;
}
