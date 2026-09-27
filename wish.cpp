#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

static const char kErr[] = "An error has occurred\n";
static void die() { write(STDERR_FILENO, kErr, sizeof(kErr) - 1); }

static std::vector<std::string> g_path = {"/bin"};

static std::vector<std::string> tokenize(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    auto flush = [&] { if (!cur.empty()) { out.push_back(cur); cur.clear(); } };
    for (char c : s) {
        if (c == ' ' || c == '\t' || c == '\n') flush();
        else cur.push_back(c);
    }
    flush();
    return out;
}

static std::string resolve(const std::string& name) {
    for (const auto& dir : g_path) {
        std::string full = dir;
        if (!full.empty() && full.back() != '/') full += '/';
        full += name;
        if (access(full.c_str(), X_OK) == 0) return full;
    }
    return "";
}

static void spawn(const std::vector<std::string>& args) {
    std::string full = resolve(args[0]);
    if (full.empty()) { die(); return; }
    pid_t pid = fork();
    if (pid < 0) { die(); return; }
    if (pid == 0) {
        std::vector<char*> argv;
        for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);
        execv(full.c_str(), argv.data());
        die();
        _exit(1);
    }
    waitpid(pid, nullptr, 0);
}

static void run_line(const std::string& line) {
    auto toks = tokenize(line);
    if (toks.empty()) return;
    if (toks[0] == "exit") {
        if (toks.size() != 1) { die(); return; }
        exit(0);
    }
    if (toks[0] == "cd") {
        if (toks.size() != 2 || chdir(toks[1].c_str()) != 0) die();
        return;
    }
    if (toks[0] == "path") {
        g_path.assign(toks.begin() + 1, toks.end());
        return;
    }
    spawn(toks);
}

static void run(FILE* in, bool interactive) {
    char* line = nullptr;
    size_t cap = 0;
    while (true) {
        if (interactive) { fputs("wish> ", stdout); fflush(stdout); }
        ssize_t n = getline(&line, &cap, in);
        if (n < 0) break;
        run_line(std::string(line, static_cast<size_t>(n)));
    }
    free(line);
}

int main(int argc, char** argv) {
    if (argc == 1) { run(stdin, true); return 0; }
    if (argc == 2) {
        FILE* f = fopen(argv[1], "r");
        if (!f) { die(); return 1; }
        run(f, false);
        fclose(f);
        return 0;
    }
    die();
    return 1;
}
