#pragma once

#include <functional>
#include <memory>
#include <chrono>

#define UNUSED(x) ((void)(x))

template<typename T>
using lambda_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;

std::string formatDuration(std::chrono::duration<double> seconds);