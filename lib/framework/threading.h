#pragma once

#include "wzglobal.h"

struct WZ_THREAD;
struct WZ_MUTEX;
struct WZ_SEMAPHORE;

WZ_THREAD *wzThreadCreate(int(*threadFunc)(void *), void *data);
WZ_DECL_NONNULL(1) int wzThreadJoin(WZ_THREAD *thread);
WZ_DECL_NONNULL(1) void wzThreadStart(WZ_THREAD *thread);
void wzYieldCurrentThread();
WZ_MUTEX *wzMutexCreate();
WZ_DECL_NONNULL(1) void wzMutexDestroy(WZ_MUTEX *mutex);
WZ_DECL_NONNULL(1) void wzMutexLock(WZ_MUTEX *mutex);
WZ_DECL_NONNULL(1) void wzMutexUnlock(WZ_MUTEX *mutex);
WZ_SEMAPHORE *wzSemaphoreCreate(int startValue);
WZ_DECL_NONNULL(1) void wzSemaphoreDestroy(WZ_SEMAPHORE *semaphore);
WZ_DECL_NONNULL(1) void wzSemaphoreWait(WZ_SEMAPHORE *semaphore);
WZ_DECL_NONNULL(1) void wzSemaphorePost(WZ_SEMAPHORE *semaphore);

#if !defined(WZ_CC_MINGW)

#include <mutex>
#include <future>

namespace wz
{
	using mutex = std::mutex;
	template <typename R>
	using future = std::future<R>;
	template <typename RA>
	using packaged_task = std::packaged_task<RA>;
}

#else  // Workaround for cross-compiler without std::mutex.

#include <memory>

namespace wz
{
	class mutex
	{
	public:
		mutex() : handle(wzMutexCreate()) {}
		~mutex() { wzMutexDestroy(handle); }

		mutex(mutex const &) = delete;
		mutex &operator =(mutex const &) = delete;

		void lock() { wzMutexLock(handle); }
		//bool try_lock();
		void unlock() { wzMutexUnlock(handle); }

	private:
		WZ_MUTEX *handle;
	};

	template <typename R>
	class future
	{
	public:
		future() = default;
		future(future &&other) : internal(std::move(other.internal)) {}
		future(future const &) = delete;
		future &operator =(future &&other) = default;
		future &operator =(future const &) = delete;
		//std::shared_future<T> share();
		R get() { auto &data = *internal; wzSemaphoreWait(data.sem); return std::move(data.ret); }
		//valid(), wait*();

		struct Internal  // Should really be a promise.
		{
			Internal() : sem(wzSemaphoreCreate(0)) {}
			~Internal() { wzSemaphoreDestroy(sem); }
			R ret;
			WZ_SEMAPHORE *sem;
		};

		std::shared_ptr<Internal> internal;
	};

	template <typename RA>
	class packaged_task;

	template <typename R, typename... A>
	class packaged_task<R(A...)>
	{
	public:
		packaged_task() = default;
		template <typename F>
		explicit packaged_task(F &&f) { function = std::move(f); internal = std::make_shared<typename future<R>::Internal>(); }
		packaged_task(packaged_task &&) = default;
		packaged_task(packaged_task const &) = delete;

		future<R> get_future() { future<R> future; future.internal = internal; return std::move(future); }
		void operator ()(A &&... args) { auto &data = *internal; data.ret = function(std::forward<A>(args)...); wzSemaphorePost(data.sem); }

	private:
		std::function<R(A...)> function;
		std::shared_ptr<typename future<R>::Internal> internal;
	};
}

#endif

