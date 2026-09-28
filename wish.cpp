#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
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
        else if (c == '>') { flush(); out.emplace_back(1, c); }
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

static bool strip_redir(std::vector<std::string>& args, std::string& outfile) {
    int count = 0;
    size_t pos = 0;
    for (size_t i = 0; i < args.size(); ++i)
        if (args[i] == ">") { ++count; pos = i; }
    if (count == 0) return true;
    if (count > 1 || pos == 0 || args.size() - pos != 2) return false;
    outfile = args[pos + 1];
    args.resize(pos);
    return true;
}

static pid_t spawn(const std::vector<std::string>& args, const std::string& outfile) {
    std::string full = resolve(args[0]);
    if (full.empty()) { die(); return -1; }
    pid_t pid = fork();
    if (pid < 0) { die(); return -1; }
    if (pid == 0) {
        if (!outfile.empty()) {
            int fd = open(outfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { die(); _exit(1); }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        std::vector<char*> argv;
        for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);
        execv(full.c_str(), argv.data());
        die();
        _exit(1);
    }
    return pid;
}

static pid_t run_one(std::vector<std::string> args) {
    if (args.empty()) return -1;

    std::string outfile;
    if (!strip_redir(args, outfile) || args.empty()) { die(); return -1; }

    if (args[0] == "exit") {
        if (args.size() != 1 || !outfile.empty()) { die(); return -1; }
        exit(0);
    }
    if (args[0] == "cd") {
        if (args.size() != 2 || chdir(args[1].c_str()) != 0) die();
        return -1;
    }
    if (args[0] == "path") {
        g_path.assign(args.begin() + 1, args.end());
        return -1;
    }
    return spawn(args, outfile);
}

static void run_line(const std::string& line) {
    auto toks = tokenize(line);
    if (toks.empty()) return;

    std::vector<std::vector<std::string>> groups(1);
    for (auto& t : toks) {
        if (t == "&") { if (!groups.back().empty()) groups.emplace_back(); }
        else groups.back().push_back(std::move(t));
    }

    std::vector<pid_t> pids;
    for (auto& g : groups) {
        pid_t pid = run_one(std::move(g));
        if (pid > 0) pids.push_back(pid);
    }
    for (pid_t pid : pids) waitpid(pid, nullptr, 0);
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
