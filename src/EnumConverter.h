#pragma once

#include <initializer_list>
#include <utility>
#include <map>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <format.h>

template<typename TEnum>
class EnumConverter {
public:
	EnumConverter() :
		initialized(false)
	{}

	virtual ~EnumConverter() = default;

	virtual boost::optional<std::string> tryToString(TEnum value) {
		initialize();
		auto it = valueToNameMap.find(value);
		return it != valueToNameMap.end()
			? it->second
			: boost::optional<std::string>();
	}

	std::string toString(TEnum value) {
		initialize();
		auto result = tryToString(value);
		if (!result) {
			auto numericValue = static_cast<typename std::underlying_type<TEnum>::type>(value);
			throw std::invalid_argument(fmt::format("{} is not a valid {} value.", numericValue, typeName));
		}

		return *result;
	}

	virtual boost::optional<TEnum> tryParse(const std::string& s) {
		initialize();
		auto it = lowerCaseNameToValueMap.find(boost::algorithm::to_lower_copy(s));
		return it != lowerCaseNameToValueMap.end()
			? it->second
			: boost::optional<TEnum>();
	}

	TEnum parse(const std::string& s) {
		initialize();
		auto result = tryParse(s);
		if (!result) {
			throw std::invalid_argument(fmt::format("{} is not a valid {} value.", s, typeName));
		}

		return *result;
	}

	std::ostream& write(std::ostream& stream, TEnum value) {
		return stream << toString(value);
	}

	std::istream& read(std::istream& stream, TEnum& value) {
		std::string name;
		stream >> name;
		value = parse(name);
		return stream;
	}

	const std::vector<TEnum>& getValues() {
		initialize();
		return values;
	}

protected:
	using member_data = std::vector<std::pair<TEnum, std::string>>;

	virtual std::string getTypeName() = 0;
	virtual member_data getMemberData() = 0;

private:
	void initialize() {
		if (initialized) return;

		typeName = getTypeName();
		for (const auto& pair : getMemberData()) {
			TEnum value = pair.first;
			std::string name = pair.second;
			lowerCaseNameToValueMap[boost::algorithm::to_lower_copy(name)] = value;
			valueToNameMap[value] = name;
			values.push_back(value);
		}
		initialized = true;
	}

	bool initialized;
	std::string typeName;
	std::map<std::string, TEnum> lowerCaseNameToValueMap;
	std::map<TEnum, std::string> valueToNameMap;
	std::vector<TEnum> values;
};
