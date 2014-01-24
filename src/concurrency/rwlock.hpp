#ifndef CONCURRENCY_RWLOCK_HPP_
#define CONCURRENCY_RWLOCK_HPP_

#include "concurrency/access.hpp"
#include "concurrency/cond_var.hpp"
#include "containers/intrusive_list.hpp"

class rwlock_in_line_t;

class rwlock_t {
public:
    rwlock_t();
    ~rwlock_t();

private:
    friend class rwlock_in_line_t;
    void add_acq(rwlock_in_line_t *acq);
    void remove_acq(rwlock_in_line_t *acq);

    void pulse_pulsables(rwlock_in_line_t *p);

    // Acquirers, in order by acquisition, with the head containing one of the
    // current acquirer, the tail possibly containing a node that does not yet hold
    // the lock.
    intrusive_list_t<rwlock_in_line_t> acqs_;
    DISABLE_COPYING(rwlock_t);
};

// Construct one of these to get in line for the read/write lock.  Use read_signal()
// and write_signal() to wait for your turn.  (Write accessors might get their read
// signal pulsed before their write signal.)
class rwlock_in_line_t : public intrusive_list_node_t<rwlock_in_line_t> {
public:
    rwlock_in_line_t(rwlock_t *lock, access_t access);
    ~rwlock_in_line_t();

    const signal_t *read_signal() const { return &read_cond_; }
    const signal_t *write_signal() const {
        guarantee(access_ == access_t::write);
        return &write_cond_;
    }

private:
    friend class rwlock_t;
    rwlock_t *const lock_;
    const access_t access_;
    cond_t read_cond_;
    cond_t write_cond_;

    DISABLE_COPYING(rwlock_in_line_t);
};


#endif  // CONCURRENCY_RWLOCK_HPP_