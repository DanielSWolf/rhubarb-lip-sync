#pragma once

#include "tools/EnumConverter.h"

enum class RecognizerType {
	PocketSphinx,
	Phonetic
};

class RecognizerTypeConverter : public EnumConverter<RecognizerType> {
public:
	static RecognizerTypeConverter& get();
protected:
	std::string getTypeName() override;
	member_data getMemberData() override;
};

std::ostream& operator<<(std::ostream& stream, RecognizerType value);

std::istream& operator>>(std::istream& stream, RecognizerType& value);
