#include "UStaticMesh.h"

void UStaticMesh::Load(UStaticMeshDesc& Desc)
{
	LoadInternal(Desc);
	Mesh = Desc.Mesh;
}
