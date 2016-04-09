#include "mouthAnimation.h"
#include "logging.h"

using std::map;

Shape getShape(Phone phone) {
	switch (phone) {
		case Phone::None:
		case Phone::P:
		case Phone::B:
		case Phone::M:
			return Shape::A;

		case Phone::Unknown:
		case Phone::IY:
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
		case Phone::Y:
			return Shape::B;

		case Phone::EH:
		case Phone::IH:
		case Phone::AH:
		case Phone::EY:
		case Phone::HH:
			return Shape::C;

		case Phone::AA:
		case Phone::AE:
		case Phone::AY:
		case Phone::AW:
			return Shape::D;

		case Phone::AO:
		case Phone::UH:
		case Phone::OW:
		case Phone::ER:
			return Shape::E;

		case Phone::UW:
		case Phone::OY:
		case Phone::W:
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

Timeline<Shape> animate(const Timeline<Phone> &phones) {
	Timeline<Shape> shapes(phones.getRange());
	for (auto& timedPhone : phones) {
		Timed<Shape> timedShape(static_cast<TimeRange>(timedPhone), getShape(timedPhone.getValue()));
		shapes.set(timedShape);
		logTimedEvent("shape", timedShape);
	}

	return shapes;
}
