#include <iostream>
#include <string>
#include <vector>
#include <tuple>

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <assert.h>
#include <errno.h>

using namespace std;

namespace posix {
class Pipe {
public:
    Pipe()
        : rfd(fds[0]), wfd(fds[1])
    {
        int rc = pipe(fds);
        assert (rc == 0);
    }

    int set_nonblocking() {
        int flags;

        if ((flags = fcntl(rfd, F_GETFL, 0)) == -1) {
            flags = 0;
        }

        return fcntl(rfd, F_SETFL, flags|O_NONBLOCK);
    }

    int read(string &msg) {
        set_nonblocking();

        int total = 0;
        char buf[1024];
        int nbytes;

        do {
            nbytes = ::read(rfd, buf, sizeof(buf)-1);
            if (nbytes > 0) {
                buf[nbytes] = '\0';
                msg += (const char*)buf;
                total += nbytes;
            } else {
                usleep(100);
            }
        } while (nbytes > 0 || again);

        return total;
    }

    int as_stdout() {
        return ::dup2(wfd, STDOUT_FILENO);
    }

    int as_stderr() {
        return ::dup2(wfd, STDERR_FILENO);
    }

    void set_wait(bool flag) {
        again = flag;
    }

    ~Pipe() {
        close(rfd);
        close(wfd);
    }
private:
    int fds[2];

    int &rfd;
    int &wfd;

    bool again = true;
};

class Thread {
public:
    Thread() {}

    virtual ~Thread() {}

protected:
    void start() {
        int rc = pthread_create(&tid, nullptr, Thread::routine, (void*)this);
        assert (rc == 0);
    }

    void join() {
        int rc = pthread_join(tid, nullptr);
        assert (rc == 0);
    }

private:
    static void *routine(void *arg) {
        Thread *self = static_cast<Thread*>(arg);

        self->run();

        return (void*)self;
    }

    virtual void run() = 0;

private:
    pthread_t tid;
};

namespace pipe {

namespace async {
class Reader : Thread {
public:
    Reader(Pipe &pipe, string &output)
        : pipe(pipe), output(output)
    {
        start();
    }

    void run() override  {
        pipe.read(output);
    }

    ~Reader() {
        join();
    }

private:
    Pipe &pipe;
    string &output;
};
} // end of NS async
} // end namespace of pipe
} // end of namespace posix

namespace subprocess {

class Popen {
public:
    Popen(string const& cmd) {
        pid = fork();

        assert (pid != -1);

        if (pid == 0) {
            child_routine(cmd);
        }

        assert (pid > 0);
    }

    void child_routine(string const &cmd) {
        output.as_stdout();
        error.as_stderr();

        execlp("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);

        assert (false); // not reachable
    }

    void communicate(string &out, string &err) {
        readers.push_back(new posix::pipe::async::Reader(output, out));
        readers.push_back(new posix::pipe::async::Reader(error, err));
    }

    int wait() {
        int status;

        waitpid(pid, &status, 0);

        output.set_wait(false);
        error.set_wait(false);

        return status;
    }

    virtual ~Popen() {
        for (auto *reader: readers) {
            delete reader;
        }
        readers.clear();
    }

private:
    posix::Pipe output;
    posix::Pipe error;

    pid_t pid;

    vector<posix::pipe::async::Reader*> readers;
};

} // end of namespace suprocess

namespace os {

tuple<int,string,string> execute(string const &cmd) {
    string out, err;
    subprocess::Popen p(cmd);
    p.communicate(out, err);
    return make_tuple(p.wait(), out, err);
}

} // end of namespace os

int main()
{
    string out, err;
    int status;
    
    tie(status, out, err) = os::execute("find /");

    cout << "out:\n" << out << "\n";
    cout << "err:\n" << err << "\n";
    cout << "rc:\n" << status << "\n";

    return 0;
}

