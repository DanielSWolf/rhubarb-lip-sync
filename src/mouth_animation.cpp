#include "mouth_animation.h"
#include <boost/utility.hpp>

using std::map;

Shape getShape(Phone phone) {
	switch (phone) {
		case Phone::None:
		case Phone::P:
		case Phone::B:
		case Phone::M:
			return Shape::A;

		case Phone::Unknown:
		case Phone::EH:
		case Phone::T:
		case Phone::D:
		case Phone::K:
		case Phone::G:
		case Phone::CH:
		case Phone::JH:
		case Phone::TH:
		case Phone::DH:
		case Phone::S:
		case Phone::Z:
		case Phone::SH:
		case Phone::ZH:
		case Phone::N:
		case Phone::NG:
		case Phone::R:
			return Shape::B;

		case Phone::IY:
		case Phone::IH:
		case Phone::EY:
		case Phone::ER:
		case Phone::HH:
		case Phone::Y:
			return Shape::C;

		case Phone::UW:
		case Phone::UH:
		case Phone::AH:
		case Phone::AE:
		case Phone::AY:
		case Phone::OY:
		case Phone::W:
			return Shape::D;

		case Phone::AW:
			return Shape::E;

		case Phone::AO:
		case Phone::AA:
		case Phone::OW:
			return Shape::F;

		case Phone::F:
		case Phone::V:
			return Shape::G;

		case Phone::L:
			return Shape::H;

		default:
			throw std::runtime_error("Unexpected Phone value.");
	}
}

map<centiseconds, Shape> animate(const map<centiseconds, Phone> &phones) {
	map<centiseconds, Shape> shapes;
	Shape lastShape = Shape::Invalid;
	for (auto it = phones.cbegin(); it != phones.cend(); it++) {
		Shape shape = getShape(it->second);
		if (shape != lastShape || next(it) == phones.cend()) {
			shapes[it->first] = shape;
			lastShape = shape;
		}
	}
	return shapes;
}
