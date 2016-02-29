#pragma once

#include <functional>
#include <memory>

#define UNUSED(x) ((void)(x))

template<typename T>
using lambda_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;
