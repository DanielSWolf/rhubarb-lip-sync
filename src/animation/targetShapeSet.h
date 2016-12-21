#pragma once

#include "Shape.h"
#include "shapeRule.h"

// Returns the closest shape to the specified one that occurs in the target shape set.
Shape convertToTargetShapeSet(Shape shape, const ShapeSet& targetShapeSet);

// Replaces each shape in the specified set with the closest shape that occurs in the target shape set.
ShapeSet convertToTargetShapeSet(const ShapeSet& shapes, const ShapeSet& targetShapeSet);

// Replaces each shape in each rule with the closest shape that occurs in the target shape set.
ContinuousTimeline<ShapeRule> convertToTargetShapeSet(const ContinuousTimeline<ShapeRule>& shapeRules, const ShapeSet& targetShapeSet);

// Replaces each shape in the specified timeline with the closest shape that occurs in the target shape set.
JoiningContinuousTimeline<Shape> convertToTargetShapeSet(const JoiningContinuousTimeline<Shape>& shapes, const ShapeSet& targetShapeSet);
