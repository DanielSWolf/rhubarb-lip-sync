#include "RecognizerType.h"

using std::string;

RecognizerTypeConverter& RecognizerTypeConverter::get() {
    static RecognizerTypeConverter converter;
    return converter;
}

string RecognizerTypeConverter::getTypeName() {
    return "RecognizerType";
}

EnumConverter<RecognizerType>::member_data RecognizerTypeConverter::getMemberData() {
    return member_data {
        { RecognizerType::PocketSphinx,    "pocketSphinx" },
        { RecognizerType::Phonetic,        "phonetic" }
    };
}

std::ostream& operator<<(std::ostream& stream, RecognizerType value) {
    return RecognizerTypeConverter::get().write(stream, value);
}

std::istream& operator>>(std::istream& stream, RecognizerType& value) {
    return RecognizerTypeConverter::get().read(stream, value);
}
