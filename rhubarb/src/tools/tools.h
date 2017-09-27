#pragma once

#include <functional>
#include <memory>
#include <chrono>
#include <deque>

#define UNUSED(x) ((void)(x))

template<typename T>
using lambda_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;

std::string formatDuration(std::chrono::duration<double> seconds);

std::string formatTime(time_t time, const std::string& format);

template<unsigned int n, typename iterator_type>
void for_each_adjacent(
	iterator_type begin,
	iterator_type end,
	std::function<void(const std::deque<std::reference_wrapper<const typename iterator_type::value_type>>&)> f)
{
	// Get the first n values
	iterator_type it = begin;
	using element_type = std::reference_wrapper<const typename iterator_type::value_type>;
	std::deque<element_type> values;
	for (unsigned i = 0; i < n; ++i) {
		if (it == end) return;
		values.push_back(std::ref(*it));
		if (i < n - 1) ++it;
	}

	for (; it != end; ++it) {
		f(values);

		values.pop_front();
		values.push_back(*it);
	}
}

template<typename iterator_type>
void for_each_adjacent(
	iterator_type begin,
	iterator_type end,
	std::function<void(const typename iterator_type::reference a, const typename iterator_type::reference b)> f)
{
	for_each_adjacent<2>(begin, end, [&](const std::deque<std::reference_wrapper<const typename iterator_type::value_type>>& args) {
		f(args[0], args[1]);
	});
}

template<typename iterator_type>
void for_each_adjacent(
	iterator_type begin,
	iterator_type end,
	std::function<void(const typename iterator_type::reference a, const typename iterator_type::reference b, const typename iterator_type::reference c)> f)
{
	for_each_adjacent<3>(begin, end, [&](const std::deque<std::reference_wrapper<const typename iterator_type::value_type>>& args) {
		f(args[0], args[1], args[2]);
	});
}
