#include "UConeComp.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "UClass.h"

IMPLEMENT_UCLASS(UConeComp, UPrimitiveComponent)
UCLASS_META(UConeComp, DisplayName, "Cone")
UCLASS_META(UConeComp, MeshName, "Cone")

void UConeComp::Register(UScene &InScene) {
  FRenderResourceLibrary *Resources = InScene.GetRenderResourceLibrary();
  SetMesh(Resources ? Resources->GetMesh(EMeshID::Cone) : nullptr);
  SetMaterial(Resources ? Resources->GetMaterial(EMaterialID::Simple) : nullptr);
  Super::Register(InScene);
}
