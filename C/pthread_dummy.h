/* pthread_dummy.h: Dummy pthreads implementation for DOS. */

#ifndef PTHREAD_H
#define PTHREAD_H

typedef int pthread_t;
typedef int pthread_mutex_t;
typedef int pthread_cond_t;
typedef int pthread_attr_t;
typedef int pthread_mutexattr_t;
typedef int pthread_condattr_t;

#define PTHREAD_MUTEX_INITIALIZER 0
#define PTHREAD_COND_INITIALIZER 0

static inline int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg) {
  return 0;
}

static inline int pthread_join(pthread_t thread, void **retval) {
  return 0;
}

static inline int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
  return 0;
}

static inline int pthread_mutex_destroy(pthread_mutex_t *mutex) {
  return 0;
}

static inline int pthread_mutex_lock(pthread_mutex_t *mutex) {
  return 0;
}

static inline int pthread_mutex_unlock(pthread_mutex_t *mutex) {
  return 0;
}

static inline int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
  return 0;
}

static inline int pthread_cond_destroy(pthread_cond_t *cond) {
  return 0;
}

static inline int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
  return 0;
}

static inline int pthread_cond_signal(pthread_cond_t *cond) {
  return 0;
}

#endif // PTHREAD_H
