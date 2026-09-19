#include "UPipeline.h"

void UPipeline::Load(UPipelineDesc& Desc)
{
	LoadInternal(Desc);
	Pipeline = Desc.Pipeline;
}
