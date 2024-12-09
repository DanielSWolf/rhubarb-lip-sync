#include "mouthAnimation.h"

#include "pauseAnimation.h"
#include "roughAnimation.h"
#include "ShapeRule.h"
#include "staticSegments.h"
#include "targetShapeSet.h"
#include "time/timedLogging.h"
#include "timingOptimization.h"
#include "tweening.h"

JoiningContinuousTimeline<Shape> animate(
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
