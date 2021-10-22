#include <SPIRV/Pass.h>

using namespace SPIRV;
using namespace spvtools;
using namespace spvtools::opt;

SPIRV::Pass::Pass(ShaderState * state, const char * name)
	: m_State(state)
	, m_Name(name)
{
}

const char * SPIRV::Pass::name() const
{
	return m_Name;
}
