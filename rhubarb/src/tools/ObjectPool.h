#pragma once
#include <memory>
#include <functional>
#include <stack>
#include <mutex>

template <typename value_type, typename pointer_type = std::unique_ptr<value_type>>
class ObjectPool {
public:
	using wrapper_type = lambda_unique_ptr<value_type>;

	ObjectPool(std::function<pointer_type()> createObject) :
		createObject(createObject)
	{}

	wrapper_type acquire() {
		std::lock_guard<std::mutex> lock(poolMutex);

		if (pool.empty()) {
			pool.push(createObject());
		}

		auto pointer = pool.top();
		pool.pop();
		return wrapper_type(pointer.get(), [this, pointer](value_type*) {
			std::lock_guard<std::mutex> lock(poolMutex);
			this->pool.push(pointer);
		});
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
	std::function<pointer_type()> createObject;
	std::stack<std::shared_ptr<value_type>> pool;
	mutable std::mutex poolMutex;
};
