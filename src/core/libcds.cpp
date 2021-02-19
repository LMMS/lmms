#include "libcds.h"

#include <cds/init.h>

#include <memory>
#include "stdshims.h"

namespace _cdslib
{

void init()
{
	cds::Initialize();
}

void deinit()
{
	cds::Terminate();
}

void thread_init()
{
	if (! cds::threading::Manager::isThreadAttached()) {
		cds::threading::Manager::attachThread();
	}
}

void thread_deinit()
{
	if (cds::threading::Manager::isThreadAttached()) {
		cds::threading::Manager::detachThread();
	}
}

}
