#include "libcds.h"

#include <cds/init.h>
#include <cds/gc/hp.h>

#include <memory>
#include "stdshims.h"

namespace _cdslib
{

static std::unique_ptr<cds::gc::HP> hpGC;

void init()
{
	cds::Initialize();
	hpGC = make_unique<cds::gc::HP>();
}

void deinit()
{
	hpGC.reset();
	cds::Terminate();
}

void thread_init()
{
	cds::threading::Manager::attachThread();
}

void thread_deinit()
{
	cds::threading::Manager::detachThread();
}

}
