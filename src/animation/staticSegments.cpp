#include "staticSegments.h"
#include <vector>
#include <numeric>
#include "tools/nextCombination.h"

using std::vector;
using boost::optional;

int getSyllableCount(const ContinuousTimeline<ShapeRule>& shapeRules, TimeRange timeRange) {
	if (timeRange.empty()) return 0;

	const auto begin = shapeRules.find(timeRange.getStart());
	const auto end = std::next(shapeRules.find(timeRange.getEnd(), FindMode::SampleLeft));

	// Treat every vowel as one syllable
	int syllableCount = 0;
	for (auto it = begin; it != end; ++it) {
		const ShapeRule shapeRule = it->getValue();

		// Disregard phones that are mostly outside the specified time range.
		const centiseconds phoneMiddle = shapeRule.phoneTiming.getMiddle();
		if (phoneMiddle < timeRange.getStart() || phoneMiddle >= timeRange.getEnd()) continue;

		auto phone = shapeRule.phone;
		if (phone && isVowel(*phone)) {
			++syllableCount;
		}
	}

	return syllableCount;
}

// A static segment is a prolonged period during which the mouth shape doesn't change
vector<TimeRange> getStaticSegments(const ContinuousTimeline<ShapeRule>& shapeRules, const JoiningContinuousTimeline<Shape>& animation) {
	// A static segment must contain a certain number of syllables to look distractingly static
	const int minSyllableCount = 3;
	// It must also have a minimum duration. The same number of syllables in fast speech usually looks good.
	const centiseconds minDuration = 75_cs;

	vector<TimeRange> result;
	for (const auto& timedShape : animation) {
		const TimeRange timeRange = timedShape.getTimeRange();
		if (timeRange.getDuration() >= minDuration && getSyllableCount(shapeRules, timeRange) >= minSyllableCount) {
			result.push_back(timeRange);
		}
	}
	
	return result;
}

// Indicates whether this shape rule can potentially be replaced by a modified version that breaks up long static segments
bool canChange(const ShapeRule& rule) {
	return rule.phone && isVowel(*rule.phone) && rule.shapeSet.size() == 1;
}

// Returns a new shape rule that is identical to the specified one, except that it leads to a slightly different visualization
ShapeRule getChangedShapeRule(const ShapeRule& rule) {
	assert(canChange(rule));

	ShapeRule result(rule);
	// So far, I've only encountered B as a static shape.
	// If there is ever a problem with another static shape, this function can easily be extended.
	if (rule.shapeSet == ShapeSet{Shape::B}) {
		result.shapeSet = {Shape::C};
	}
	return result;
}

// Contains the start times of all rules to be changed
using RuleChanges = vector<centiseconds>;

// Replaces the indicated shape rules with slightly different ones, breaking up long static segments
ContinuousTimeline<ShapeRule> applyChanges(const ContinuousTimeline<ShapeRule>& shapeRules, const RuleChanges& changes) {
	ContinuousTimeline<ShapeRule> result(shapeRules);
	for (centiseconds changedRuleStart : changes) {
		const Timed<ShapeRule> timedOriginalRule = *shapeRules.get(changedRuleStart);
		const ShapeRule changedRule = getChangedShapeRule(timedOriginalRule.getValue());
		result.set(timedOriginalRule.getTimeRange(), changedRule);
	}
	return result;
}

class RuleChangeScenario {
public:
	RuleChangeScenario(
		const ContinuousTimeline<ShapeRule>& originalRules,
		const RuleChanges& changes,
		AnimationFunction animate) :
		changedRules(applyChanges(originalRules, changes)),
		animation(animate(changedRules)),
		staticSegments(getStaticSegments(changedRules, animation)) {}

	bool isBetterThan(const RuleChangeScenario& rhs) const {
		// We want zero static segments
		if (staticSegments.size() == 0 && rhs.staticSegments.size() > 0) return true;

		// Short shapes are better than long ones. Minimize sum-of-squares.
		if (getSumOfShapeDurationSquares() < rhs.getSumOfShapeDurationSquares()) return true;

		return false;
	}

	int getStaticSegmentCount() const {
		return staticSegments.size();
	}

	ContinuousTimeline<ShapeRule> getChangedRules() const {
		return changedRules;
	}

private:
	ContinuousTimeline<ShapeRule> changedRules;
	JoiningContinuousTimeline<Shape> animation;
	vector<TimeRange> staticSegments;

	double getSumOfShapeDurationSquares() const {
		return std::accumulate(animation.begin(), animation.end(), 0.0, [](const double sum, const Timed<Shape>& timedShape) {
			const double duration = std::chrono::duration_cast<std::chrono::duration<double>>(timedShape.getDuration()).count();
			return sum + duration * duration;
		});
	}
};

RuleChanges getPossibleRuleChanges(const ContinuousTimeline<ShapeRule>& shapeRules) {
	RuleChanges result;
	for (auto it = shapeRules.begin(); it != shapeRules.end(); ++it) {
		const ShapeRule rule = it->getValue();
		if (canChange(rule)) {
			result.push_back(it->getStart());
		}
	}
	return result;
}

ContinuousTimeline<ShapeRule> fixStaticSegmentRules(const ContinuousTimeline<ShapeRule>& shapeRules, AnimationFunction animate) {
	// The complexity of this function is exponential with the number of replacements. So let's cap that value.
	const int maxReplacementCount = 3;

	// All potential changes
	const RuleChanges possibleRuleChanges = getPossibleRuleChanges(shapeRules);

	// Find best solution. Start with a single replacement, then increase as necessary.
	RuleChangeScenario bestScenario(shapeRules, {}, animate);
	for (int replacementCount = 1; bestScenario.getStaticSegmentCount() > 0 && replacementCount <= maxReplacementCount; ++replacementCount) {
		// Only the first <replacementCount> elements of `currentRuleChanges` count
		auto currentRuleChanges(possibleRuleChanges);
		do {
			RuleChangeScenario currentScenario(shapeRules, {currentRuleChanges.begin(), currentRuleChanges.begin() + replacementCount}, animate);
			if (currentScenario.isBetterThan(bestScenario)) {
				bestScenario = currentScenario;
			}
		} while (next_combination(currentRuleChanges.begin(), currentRuleChanges.begin() + replacementCount, currentRuleChanges.end()));
	}

	return bestScenario.getChangedRules();
}

// Indicates whether the specified shape rule may result in different shapes depending on context
bool isFlexible(const ShapeRule& rule) {
	return rule.shapeSet.size() > 1;
}

// Extends the specified time range until it starts and ends with a non-flexible shape rule, if possible
TimeRange extendToFixedRules(const TimeRange& timeRange, const ContinuousTimeline<ShapeRule>& shapeRules) {
	auto first = shapeRules.find(timeRange.getStart());
	while (first != shapeRules.begin() && isFlexible(first->getValue())) {
		--first;
	}
	auto last = shapeRules.find(timeRange.getEnd(), FindMode::SampleLeft);
	while (std::next(last) != shapeRules.end() && isFlexible(last->getValue())) {
		++last;
	}
	return TimeRange(first->getStart(), last->getEnd());
}

JoiningContinuousTimeline<Shape> avoidStaticSegments(const ContinuousTimeline<ShapeRule>& shapeRules, AnimationFunction animate) {
	const auto animation = animate(shapeRules);
	const vector<TimeRange> staticSegments = getStaticSegments(shapeRules, animation);
	if (staticSegments.empty()) {
		return animation;
	}

	// Modify shape rules to eliminate static segments
	ContinuousTimeline<ShapeRule> fixedShapeRules(shapeRules);
	for (const TimeRange& staticSegment : staticSegments) {
		// Extend time range to the left and right so we don't lose adjacent rules that might influence the animation
		const TimeRange extendedStaticSegment = extendToFixedRules(staticSegment, shapeRules);

		// Fix shape rules within the static segment
		const auto fixedSegmentShapeRules = fixStaticSegmentRules({extendedStaticSegment, ShapeRule::getInvalid(), fixedShapeRules}, animate);
		for (const auto& timedShapeRule : fixedSegmentShapeRules) {
			fixedShapeRules.set(timedShapeRule);
		}
	}

	return animate(fixedShapeRules);
}
