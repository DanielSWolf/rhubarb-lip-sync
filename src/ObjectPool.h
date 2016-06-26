#pragma once
#include <memory>
#include <functional>
#include <stack>
#include <mutex>

template <class T>
class ObjectPool {
public:
	using ptr_type = std::unique_ptr<T, std::function<void(T*)>>;

	ObjectPool(std::function<T*()> createObject) :
		createObject(createObject)
	{}

	virtual ~ObjectPool() {}

	ptr_type acquire() {
		std::lock_guard<std::mutex> lock(poolMutex);

		if (pool.empty()) {
			pool.push(std::unique_ptr<T>(createObject()));
		}

		ptr_type tmp(pool.top().release(), [this](T* p) {
			std::lock_guard<std::mutex> lock(poolMutex);
			this->pool.push(std::unique_ptr<T>(p));
		});
		pool.pop();
		return std::move(tmp);
	}

	bool empty() const {
		std::lock_guard<std::mutex> lock(poolMutex);
		return pool.empty();
	}

	size_t size() const {
		std::lock_guard<std::mutex> lock(poolMutex);
		return pool.size();
	}

private:
	std::function<T*()> createObject;
	std::stack<std::unique_ptr<T>> pool;
	mutable std::mutex poolMutex;
};
