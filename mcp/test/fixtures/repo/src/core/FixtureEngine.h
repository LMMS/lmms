#ifndef FIXTUREENGINE_H
#define FIXTUREENGINE_H

namespace fixture {

class FixtureEngine
{
public:
	FixtureEngine() = default;
	int sampleRate() const;

private:
	int m_sampleRate = 44100;
};

} // namespace fixture

#endif
