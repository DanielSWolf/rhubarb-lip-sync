#pragma once

#include <memory>
#include "time/TimeRange.h"
#include <functional>
#include "tools/Lazy.h"

class AudioClip;
class SampleIterator;

class AudioClip {
public:
	using value_type = float;
	using size_type = int64_t;
	using difference_type = int64_t;
	using iterator = SampleIterator;
	using SampleReader = std::function<value_type(size_type)>;

	virtual ~AudioClip() {}
	virtual std::unique_ptr<AudioClip> clone() const = 0;
	virtual int getSampleRate() const = 0;
	virtual size_type size() const = 0;
	TimeRange getTruncatedRange() const;
	SampleReader createSampleReader() const;
	iterator begin() const;
	iterator end() const;
private:
	virtual SampleReader createUnsafeSampleReader() const = 0;
};

using AudioEffect = std::function<std::unique_ptr<AudioClip>(std::unique_ptr<AudioClip>)>;

std::unique_ptr<AudioClip> operator|(std::unique_ptr<AudioClip> clip, AudioEffect effect);

using SampleReader = AudioClip::SampleReader;

class SampleIterator {
public:
	using value_type = AudioClip::value_type;
	using size_type = AudioClip::size_type;
	using difference_type = AudioClip::difference_type;

	SampleIterator();

	size_type getSampleIndex() const;
	void seek(size_type sampleIndex);
	value_type operator*() const;
	value_type operator[](difference_type n) const;

private:
	friend AudioClip;
	SampleIterator(const AudioClip& audioClip, size_type sampleIndex);

	Lazy<SampleReader> sampleReader;
	size_type sampleIndex;
};

inline SampleIterator::size_type SampleIterator::getSampleIndex() const {
	return sampleIndex;
}

inline void SampleIterator::seek(size_type sampleIndex) {
	this->sampleIndex = sampleIndex;
}

inline SampleIterator::value_type SampleIterator::operator*() const {
	return (*sampleReader)(sampleIndex);
}

inline SampleIterator::value_type SampleIterator::operator[](difference_type n) const {
	return (*sampleReader)(sampleIndex + n);
}

inline bool operator==(const SampleIterator& lhs, const SampleIterator& rhs) {
	return lhs.getSampleIndex() == rhs.getSampleIndex();
}

inline bool operator!=(const SampleIterator& lhs, const SampleIterator& rhs) {
	return lhs.getSampleIndex() != rhs.getSampleIndex();
}

inline bool operator<(const SampleIterator& lhs, const SampleIterator& rhs) {
	return lhs.getSampleIndex() < rhs.getSampleIndex();
}

inline bool operator>(const SampleIterator& lhs, const SampleIterator& rhs) {
	return lhs.getSampleIndex() > rhs.getSampleIndex();
}

inline bool operator<=(const SampleIterator& lhs, const SampleIterator& rhs) {
	return lhs.getSampleIndex() <= rhs.getSampleIndex();
}

inline bool operator>=(const SampleIterator& lhs, const SampleIterator& rhs) {
	return lhs.getSampleIndex() >= rhs.getSampleIndex();
}

inline SampleIterator& operator+=(SampleIterator& it, SampleIterator::difference_type n) {
	it.seek(it.getSampleIndex() + n);
	return it;
}

inline SampleIterator& operator-=(SampleIterator& it, SampleIterator::difference_type n) {
	it.seek(it.getSampleIndex() - n);
	return it;
}

inline SampleIterator& operator++(SampleIterator& it) {
	return operator+=(it, 1);
}

inline SampleIterator operator++(SampleIterator& it, int) {
	SampleIterator tmp(it);
	operator++(it);
	return tmp;
}

inline SampleIterator& operator--(SampleIterator& it) {
	return operator-=(it, 1);
}

inline SampleIterator operator--(SampleIterator& it, int) {
	SampleIterator tmp(it);
	operator--(it);
	return tmp;
}

inline SampleIterator operator+(const SampleIterator& it, SampleIterator::difference_type n) {
	SampleIterator result(it);
	result += n;
	return result;
}

inline SampleIterator operator-(const SampleIterator& it, SampleIterator::difference_type n) {
	SampleIterator result(it);
	result -= n;
	return result;
}

inline SampleIterator::difference_type operator-(const SampleIterator& lhs, const SampleIterator& rhs) {
	return lhs.getSampleIndex() - rhs.getSampleIndex();
}
