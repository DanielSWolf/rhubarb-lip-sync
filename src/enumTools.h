#pragma once

#include <vector>
#include <tuple>
#include <map>
#include <boost/algorithm/string.hpp>
#include <exception>
#include <format.h>

template<typename T>
const std::string& getEnumTypeName();

template<typename T>
const std::vector<std::tuple<T, std::string>>& getEnumMembers();

namespace detail {

	template<typename T>
	std::map<std::string, T> createLowerCaseNameToValueMap() {
		std::map<std::string, T> map;
		for (const auto& pair : getEnumMembers<T>()) {
			map[boost::algorithm::to_lower_copy(std::get<std::string>(pair))] = std::get<T>(pair);
		}
		return map;
	}

	template<typename T>
	std::map<T, std::string> createValueToNameMap() {
		std::map<T, std::string> map;
		for (const auto& pair : getEnumMembers<T>()) {
			map[std::get<T>(pair)] = std::get<std::string>(pair);
		}
		return map;
	}

	template<typename T>
	std::vector<T> getEnumValues() {
		std::vector<T> result;
		for (const auto& pair : getEnumMembers<T>()) {
			result.push_back(std::get<T>(pair));
		}
		return result;
	}

}

template<typename T>
bool tryParseEnum(const std::string& s, T& result) {
	static const std::map<std::string, T> lookup = detail::createLowerCaseNameToValueMap<T>();
	auto it = lookup.find(boost::algorithm::to_lower_copy(s));
	if (it == lookup.end()) return false;

	result = it->second;
	return true;
}

template<typename T>
T parseEnum(const std::string& s) {
	T result;
	if (!tryParseEnum(s, result)) {
		throw std::invalid_argument(fmt::format("{} is not a valid {} value.", s, getEnumTypeName<T>()));
	}

	return result;
}

template<typename T>
bool tryEnumToString(T value, std::string& result) {
	static const std::map<T, std::string> lookup = detail::createValueToNameMap<T>();
	auto it = lookup.find(value);
	if (it == lookup.end()) return false;

	result = it->second;
	return true;
}

template<typename T>
std::string enumToString(T value) {
	std::string result;
	if (!tryEnumToString(value, result)) {
		throw std::invalid_argument(fmt::format(
			"{} is not a valid {} value.",
			static_cast<typename std::underlying_type<T>::type>(value),
			getEnumTypeName<T>()));
	}

	return result;
}

template<typename T>
const std::vector<T>& getEnumValues() {
	static const auto result = detail::getEnumValues<T>();
	return result;
}