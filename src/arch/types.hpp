// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_TYPES_HPP_
#define ARCH_TYPES_HPP_

#include <string.h>

#include <string>

#include "utils.hpp"

template <class> class scoped_array_t;
struct iovec;

#define DEFAULT_DISK_ACCOUNT (static_cast<file_account_t *>(0))
#define UNLIMITED_OUTSTANDING_REQUESTS (-1)

// TODO: Remove this from this header.

// The linux_tcp_listener_t constructor can throw this exception
class address_in_use_exc_t : public std::exception {
public:
    address_in_use_exc_t(const char* hostname, int port) throw () {
        if (port == 0) {
            info = strprintf("Could not establish sockets on all selected local addresses using the same port");
        } else {
            info = strprintf("The address at %s:%d is reserved or already in use", hostname, port);
        }
    }

    explicit address_in_use_exc_t(const std::string &msg) throw () {
        info.assign(msg);
    }

    ~address_in_use_exc_t() throw () { }

    const char *what() const throw () {
        return info.c_str();
    }
private:
    std::string info;
};

class tcp_socket_exc_t : public std::exception {
public:
    tcp_socket_exc_t(int err) throw () {
        info = strprintf("TCP socket creation failed: %s", strerror(err));
    }

    ~tcp_socket_exc_t() throw () { }

    const char *what() const throw () {
        return info.c_str();
    }

private:
    std::string info;
};

class tcp_conn_read_closed_exc_t : public std::exception {
    const char *what() const throw () {
        return "Network connection read end closed";
    }
};

struct tcp_conn_write_closed_exc_t : public std::exception {
    const char *what() const throw () {
        return "Network connection write end closed";
    }
};


class linux_iocallback_t {
public:
    virtual ~linux_iocallback_t() {}
    virtual void on_io_complete() = 0;

    //TODO Remove this default implementation and actually handle io errors.
    virtual void on_io_failure(int errsv, int64_t offset, int64_t count) {
        crash("I/O operation failed.  (%s) (offset = %" PRIi64 ", count = %" PRIi64 ")",
              errno_string(errsv).c_str(), offset, count);
    }
};

class linux_thread_pool_t;
typedef linux_thread_pool_t thread_pool_t;

class file_account_t;

class linux_iocallback_t;
typedef linux_iocallback_t iocallback_t;

class linux_tcp_bound_socket_t;
typedef linux_tcp_bound_socket_t tcp_bound_socket_t;

class linux_nonthrowing_tcp_listener_t;
typedef linux_nonthrowing_tcp_listener_t non_throwing_tcp_listener_t;

class linux_tcp_listener_t;
typedef linux_tcp_listener_t tcp_listener_t;

class linux_repeated_nonthrowing_tcp_listener_t;
typedef linux_repeated_nonthrowing_tcp_listener_t repeated_nonthrowing_tcp_listener_t;

class linux_tcp_conn_descriptor_t;
typedef linux_tcp_conn_descriptor_t tcp_conn_descriptor_t;

class linux_tcp_conn_t;
typedef linux_tcp_conn_t tcp_conn_t;

enum class file_direct_io_mode_t {
    direct_desired,
    buffered_desired
};



class semantic_checking_file_t {
public:
    semantic_checking_file_t() { }
    virtual ~semantic_checking_file_t() { }
    // May not return -1.  Crashes instead.
    virtual size_t semantic_blocking_read(void *buf, size_t length) = 0;
    // May not return -1.  Crashes instead.
    virtual size_t semantic_blocking_write(const void *buf, size_t length) = 0;

private:
    DISABLE_COPYING(semantic_checking_file_t);
};

// A linux file.  It expects reads and writes and buffers to have an
// alignment of DEVICE_BLOCK_SIZE.
class file_t {
public:
    enum wrap_in_datasyncs_t { NO_DATASYNCS, WRAP_IN_DATASYNCS };

    file_t() { }

    virtual ~file_t() { }
    virtual int64_t get_size() = 0;
    virtual void set_size(int64_t size) = 0;
    virtual void set_size_at_least(int64_t size) = 0;

    virtual void read_async(int64_t offset, size_t length, void *buf,
                            file_account_t *account, linux_iocallback_t *cb) = 0;
    virtual void write_async(int64_t offset, size_t length, const void *buf,
                             file_account_t *account, linux_iocallback_t *cb,
                             wrap_in_datasyncs_t wrap_in_datasyncs) = 0;
    // writev_async doesn't provide the atomicity guarantees of writev.
    virtual void writev_async(int64_t offset, size_t length, scoped_array_t<iovec> &&bufs,
                              file_account_t *account, linux_iocallback_t *cb) = 0;

    virtual void *create_account(int priority, int outstanding_requests_limit) = 0;
    virtual void destroy_account(void *account) = 0;

    virtual bool coop_lock_and_check() = 0;

private:
    DISABLE_COPYING(file_t);
};

class file_account_t {
public:
    file_account_t(file_t *f, int p, int outstanding_requests_limit = UNLIMITED_OUTSTANDING_REQUESTS);
    ~file_account_t();
    void *get_account() { return account; }

private:
    file_t *parent;
    /* account is internally a pointer to a accounting_diskmgr_t::account_t object. It has to be
       a void* because accounting_diskmgr_t is a template, so its actual type depends on what
       IO backend is chosen. */
    // Maybe accounting_diskmgr_t shouldn't be a templated class then.

    void *account;

    DISABLE_COPYING(file_account_t);
};


#endif  // ARCH_TYPES_HPP_
