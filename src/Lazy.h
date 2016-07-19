#pragma once
#include <memory>
#include <mutex>

template<typename T>
class Lazy {
public:
	using value_type = T;

	explicit Lazy(std::function<T()> createValue) :
		createValue(createValue)
	{}

	explicit operator bool() const {
		return static_cast<bool>(_value);
	}

	T& value() {
		init();
		return *_value;
	}

	const T& value() const {
		init();
		return *_value;
	}

	T* operator->() {
		return &value();
	}

	const T* operator->() const {
		return &value();
	}

	T& operator*() {
		return value();
	}

	const T& operator*() const {
		return value();
	}
private:
	void init() const {
		std::call_once(initialized, [&] { _value = std::make_unique<T>(createValue()); });
	}

	std::function<T()> createValue;
	mutable std::once_flag initialized;
	mutable std::unique_ptr<T> _value;
};
