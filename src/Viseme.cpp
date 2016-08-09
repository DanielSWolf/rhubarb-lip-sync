#include "Viseme.h"

VisemeOption::VisemeOption(Shape shape) :
	VisemeOption(shape, shape, shape, shape, shape)
{}

VisemeOption::VisemeOption(Shape nearB, Shape nearC, Shape nearD, Shape nearE, Shape nearF) :
	VisemeOption(nearB, nearB, nearC, nearD, nearE, nearF, nearB, nearC, nearB)
{}

VisemeOption::VisemeOption(Shape nearA, Shape nearB, Shape nearC, Shape nearD, Shape nearE, Shape nearF, Shape nearG, Shape nearH, Shape nearX) :
	shapes{ nearA, nearB, nearC, nearD, nearE, nearF, nearG, nearH, nearX }
{
	static_assert(static_cast<int>(Shape::EndSentinel) == 9, "Shape definition has changed.");
}

Shape VisemeOption::getShape(Shape context) const {
	return shapes.at(static_cast<int>(context));
}

bool VisemeOption::operator==(const VisemeOption& rhs) const {
	return shapes == rhs.shapes;
}

bool VisemeOption::operator!=(const VisemeOption& rhs) const {
	return !operator==(rhs);
}

Viseme::Viseme(const VisemeOption& option) :
	options{ { 0_cs, option } }
{}

Viseme::Viseme(const VisemeOption& option1, centiseconds threshold, const VisemeOption& option2) :
	options{ { 0_cs, option1 }, { threshold, option2 } }
{}

Viseme::Viseme(const VisemeOption& option1, centiseconds threshold1, const VisemeOption& option2, centiseconds threshold2, const VisemeOption& option3) :
	options{ { 0_cs, option1 },{ threshold1, option2 }, { threshold2, option3 } }
{}

Shape Viseme::getShape(centiseconds duration, Shape context) const {
	VisemeOption option = std::prev(options.upper_bound(duration))->second;
	return option.getShape(context);
}

bool Viseme::operator==(const Viseme& rhs) const {
	return options == rhs.options;
}

bool Viseme::operator!=(const Viseme& rhs) const {
	return !operator==(rhs);
}
