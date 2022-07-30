#include "MicroTimer.h"

namespace lmms
{

using namespace std;
using namespace std::chrono;

static_assert(ratio_less_equal<steady_clock::duration::period, micro>::value, 
	"MicroTimer: steady_clock doesn't support microsecond resolution");

MicroTimer::MicroTimer()
{
	reset();
}

void MicroTimer::reset()
{
	begin = steady_clock::now();
}

int MicroTimer::elapsed() const
{
	auto now = steady_clock::now();
	return std::chrono::duration_cast<std::chrono::duration<int, std::micro>>(now - begin).count();
}

} // namespace lmms
