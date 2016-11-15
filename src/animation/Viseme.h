#pragma once
#include "Shape.h"
#include "centiseconds.h"
#include <map>
#include <array>

class VisemeOption {
public:
	VisemeOption(Shape shape);
	VisemeOption(Shape nearB, Shape nearC, Shape nearD, Shape nearE, Shape nearF);
	VisemeOption(Shape nearA, Shape nearB, Shape nearC, Shape nearD, Shape nearE, Shape nearF, Shape nearG, Shape nearH, Shape nearX);
	Shape getShape(Shape context) const;
	bool operator==(const VisemeOption& rhs) const;
	bool operator!=(const VisemeOption& rhs) const;

private:
	std::array<Shape, 9> shapes;
};

class Viseme {
public:
	Viseme(const VisemeOption& option);
	Viseme(const VisemeOption& option1, centiseconds threshold, const VisemeOption& option2);
	Viseme(const VisemeOption& option1, centiseconds threshold1, const VisemeOption& option2, centiseconds threshold2, const VisemeOption& option3);
	Shape getShape(centiseconds duration, Shape context) const;
	bool operator==(const Viseme& rhs) const;
	bool operator!=(const Viseme& rhs) const;

private:
	std::map<centiseconds, VisemeOption> options;
};
