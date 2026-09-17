#pragma once

#include "Runtime/Rendering/FFont.h"
#include "Runtime/Rendering/FMesh.h"
#include "UBillBoardComp.h"

class UTextComponent : public UBillBoardComp {
  GENERATED_BODY()
  DECLARE_UCLASS(UTextComponent, UBillBoardComp)

public:
  void SetText(const FWString &InText) {
    Text = InText;
    RebuildTextMesh();
  }
  [[nodiscard]] const FWString &GetText() const { return Text; }
  void SetFont(TSharedPtr<FFont> InFont) { Font = InFont; }

  void Register(UScene& Scene) override;
  void RebuildTextMesh();

  void Serialize(FArchive& Archive) const override;
  virtual void Deserialize(const FArchive& Archive) override;
private:
  TSharedPtr<FFont> Font;
  FWString Text = L"안녕하세요!";

  FMeshDesc MeshData;
};
