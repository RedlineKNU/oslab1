#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

static const char kErr[] = "An error has occurred\n";
static void die() { write(STDERR_FILENO, kErr, sizeof(kErr) - 1); }

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    char* line = nullptr;
    size_t cap = 0;
    while (true) {
        fputs("wish> ", stdout);
        fflush(stdout);
        ssize_t n = getline(&line, &cap, stdin);
        if (n < 0) break;
        std::string cmd(line, static_cast<size_t>(n));
        while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == ' ' || cmd.back() == '\t'))
            cmd.pop_back();
        size_t start = cmd.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        cmd = cmd.substr(start);
        if (cmd == "exit") {
            free(line);
            return 0;
        }
        die();
    }
    free(line);
    return 0;
}
