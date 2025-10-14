module;

#include "core/phone.h"
#include "core/shape.h"
#include "time/continuous-timeline.h"
#include "time/timed-logging.h"

export module rhubarb.animation:mouth_animation;

import :pause_animation;
import :rough_animation;
import :shape_rule;
import :static_segments;
import :target_shape_set;
import :timing_optimization;
import :tweening;

export JoiningContinuousTimeline<Shape> animate(
    const BoundedTimeline<Phone>& phones, const ShapeSet& targetShapeSet
) {
    // Create timeline of shape rules
    ContinuousTimeline<ShapeRule> shapeRules = getShapeRules(phones);

    // Modify shape rules to only contain allowed shapes -- plus X, which is needed for pauses and
    // will be replaced later
    ShapeSet targetShapeSetPlusX = targetShapeSet;
    targetShapeSetPlusX.insert(Shape::X);
    shapeRules = convertToTargetShapeSet(shapeRules, targetShapeSetPlusX);

    // Animate in multiple steps
    const auto performMainAnimationSteps = [&targetShapeSet](const auto& shapeRules) {
        JoiningContinuousTimeline<Shape> animation = animateRough(shapeRules);
        animation = optimizeTiming(animation);
        animation = animatePauses(animation);
        animation = insertTweens(animation);
        animation = convertToTargetShapeSet(animation, targetShapeSet);
        return animation;
    };
    const JoiningContinuousTimeline<Shape> result =
        avoidStaticSegments(shapeRules, performMainAnimationSteps);

    for (const auto& timedShape : result) {
        logTimedEvent("shape", timedShape);
    }

    return result;
}
