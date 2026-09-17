#pragma once

#include "FFont.h"
#include "FMaterial.h"
#include "FMesh.h"
#include "FRenderPipeline.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/TMap.h"
#include "Vertices.h"

class FRenderer;
class FTexture;
struct FTextVertex {
  FVector Pos;
  float u, v;
};

enum class EPipelineID : uint8 {
  Simple_Solid,
  Simple_Wireframe,
  Textured,
  Grid,
  RotationGizmo,
  Count,
  Spotlight,
  Text,
  Instance_Text,
  Instance_Simple,
  Gizmo
};

enum class EMeshID : uint8 {
  Cube,
  Cylinder,
  Cone,
  SpotlightCone,
  Arrow,
  Circle,
  RotGizmo,
  SquareArrow,
  Grid,
  Sphere,
  Line,
  Plane,
  Rect,
  TextMesh,
  Count
};

enum class EMaterialID : uint8 {
  Simple,
  Grid,
  RotGizmo,
  Spotlight,
  Text,
  Textured,
  Instance_Text,
  Instance_Simple,
  Billboard,
  Gizmo,
};

// 인스턴싱 배치 키 구조체
struct FInstanceBatchKey {
  EMaterialID MaterialID;
  EMeshID MeshID;

  bool operator==(const FInstanceBatchKey &Other) const = default;
};

template <>
struct std::hash<FInstanceBatchKey> {
  size_t operator()(const FInstanceBatchKey &Key) const noexcept {
    return (static_cast<size_t>(Key.MaterialID) << 16) | static_cast<size_t>(Key.MeshID);
  }
};

class FRenderResourceLibrary final {
public:
  // 전역 싱글톤 접근자
  static FRenderResourceLibrary &Get();

  bool Initialize(FRenderer &Renderer);

  // 파이프라인 보관 맵
  TMap<EPipelineID, TSharedPtr<FRenderPipeline>> AllPipelineMap;
  // 메쉬 보관 맵
  TMap<EMeshID, TSharedPtr<FMesh>> AllMeshMap;
  // 머티리얼 보관 맵 (ID 기반)
  TMap<EMaterialID, TSharedPtr<FMaterial>> AllMaterialMap;
  // 텍스쳐 보관 맵
  TMap<FString, TSharedPtr<FTexture>> AllTextureMap;

  //에디터용 아이콘 텍스쳐 보관 맵
  TMap<FString, TSharedPtr<FTexture>> AllEditorTextureMap;

  // 인스턴싱 배치 배열 맵
  TMap<FInstanceBatchKey, TArray<FInstanceData>> AllInstancingArrayMap;

  // 인스턴싱 배열 조회
  TArray<FInstanceData>& GetInstancingArray(EMaterialID MatId, EMeshID MeshId) {
    return AllInstancingArrayMap[{MatId, MeshId}];
  }

  // 파이프라인 조회
  [[nodiscard]] TSharedPtr<FRenderPipeline> GetPipeline(EPipelineID Id) const {
    auto it = AllPipelineMap.find(Id);
    if (it != AllPipelineMap.end())
      return it->second;
    return nullptr;
  }

  // 머티리얼 조회 (ID 기반 전용)
  [[nodiscard]] TSharedPtr<FMaterial> GetMaterial(EMaterialID Id) const {
    auto it = AllMaterialMap.find(Id);
    if (it != AllMaterialMap.end())
      return it->second;
    return nullptr;
  }

  // 메쉬 조회
  TSharedPtr<FMesh> GetMesh(const EMeshID &ID) const {
    auto it = AllMeshMap.find(ID);
    if (it != AllMeshMap.end())
      return it->second;
    return nullptr;
  }

  // 메쉬 등록
  TSharedPtr<FMesh> RegisterMesh(const EMeshID &ID, TSharedPtr<FMesh> inMesh) {
    inMesh->MeshId = ID;
    AllMeshMap[ID] = inMesh;
    return inMesh;
  }

  // 개별 메쉬 접근자
  [[nodiscard]] TSharedPtr<FMesh> GetCubeMesh() const {
    return GetMesh(EMeshID::Cube);
  }
  [[nodiscard]] TSharedPtr<FMesh> GetCylinderMesh() const {
    return GetMesh(EMeshID::Cylinder);
  }
  [[nodiscard]] TSharedPtr<FMesh> GetConeMesh() const {
    return GetMesh(EMeshID::Cone);
  }
  [[nodiscard]] TSharedPtr<FMesh> GetSpotlightConeMesh() const {
    return GetMesh(EMeshID::SpotlightCone);
  }
  [[nodiscard]] TSharedPtr<FMesh> GetArrowMesh() const {
    return GetMesh(EMeshID::Arrow);
  }
  [[nodiscard]] TSharedPtr<FMesh> GetCircleMesh() const {
    return GetMesh(EMeshID::Circle);
  }
  [[nodiscard]] TSharedPtr<FMesh> GetRotationGizmoMesh() const {
    return GetMesh(EMeshID::RotGizmo);
  }
  [[nodiscard]] TSharedPtr<FMesh> GetSquareArrowMesh() const {
    return GetMesh(EMeshID::SquareArrow);
  }
  [[nodiscard]] TSharedPtr<FMesh> GetGridMesh() const {
    return GetMesh(EMeshID::Grid);
  }
  [[nodiscard]] TSharedPtr<FMesh> GetSphereMesh() const {
    return GetMesh(EMeshID::Sphere);
  }
  [[nodiscard]] TSharedPtr<FMesh> GetLineMesh() const {
    return GetMesh(EMeshID::Line);
  }
  [[nodiscard]] TSharedPtr<FMesh> GetPlaneMesh() const {
    return GetMesh(EMeshID::Plane);
  }
  [[nodiscard]] TSharedPtr<FMesh> GetRectMesh() const {
    return GetMesh(EMeshID::Rect);
  }
  [[nodiscard]] TSharedPtr<FMesh> GetTextMesh() const {
    return GetMesh(EMeshID::TextMesh);
  }

  // 머티리얼 등록 (ID 기반 전용)
  TSharedPtr<FMaterial> RegisterMaterial(EMaterialID Id, TSharedPtr<FMaterial> inMaterial);

  void RegisterTexture(const FString &name, TSharedPtr<FTexture> texture) {
    AllTextureMap[name] = texture;
  }

  void RegisterEditTexture(const FString& name, TSharedPtr<FTexture> texture)
  {
      AllEditorTextureMap[name] = texture;
  }

  // 텍스처 조회. 없으면 nullptr
  [[nodiscard]] TSharedPtr<FTexture> GetTexture(const FString &name) const {
    auto it = AllTextureMap.find(name);
    if (it != AllTextureMap.end())
      return it->second;
    return nullptr;
  }

  [[nodiscard]] TSharedPtr<FTexture> GetEditTexture(const FString& name) const {
      auto it = AllEditorTextureMap.find(name);
      if (it != AllEditorTextureMap.end())
          return it->second;
      return nullptr;
  }

  // 메쉬 전체 해제
  void DestroyAllMeshes() { AllMeshMap.clear(); }

  // 머티리얼 전체 해제
  void DestroyAllMaterials() { AllMaterialMap.clear(); }

  // 파이프라인 전체 해제
  void DestroyAllPipelines() { AllPipelineMap.clear(); }

  void DestroyAllInstancingArray() { AllInstancingArrayMap.clear(); }

  // 전체 머티리얼 맵 조회
  const TMap<EMaterialID, TSharedPtr<FMaterial>> &GetAllMaterials() const {
    return AllMaterialMap;
  }

  // 렌더러 참조 조회
  FRenderer *GetRenderer() const { return RendererRef; }

  // 정점 배열 메쉬 캐싱 생성
  TSharedPtr<FMesh> GetOrCreateMesh(const EMeshID &ID,
                                    const TArray<FVertexData> &vertices);




private:
  bool InitializePipelines(FRenderer &Renderer);
  bool CreateSolidWireframePipeline(FRenderer &Renderer);

  bool CreateCubeMesh(FRenderer &Renderer);
  bool CreateCylinderMesh(FRenderer &Renderer, float Height, uint32 SliceCount,
                          float TopRadius, float BottomRadius);
  bool CreateConeMesh(FRenderer &Renderer);
  bool CreateSpotlightConeMesh(FRenderer &Renderer);
  bool CreateArrowMesh(FRenderer &Renderer);
  bool CreateCircleMesh(FRenderer &Renderer);
  bool CreateRotationGizmoMesh(FRenderer &Renderer);
  bool CreateSquareArrowMesh(FRenderer &Renderer);
  bool CreateGridMesh(FRenderer &Renderer);
  bool CreateSphereMesh(FRenderer &Renderer);
  bool CreateLineMesh(FRenderer &Renderer);
  bool CreatePlaneMesh(FRenderer &Renderer);
  bool CreateRectMesh(FRenderer &Renderer);

  bool CreateInstancingArrayMap();

  // 텍스처 및 머티리얼 일괄 초기화
  bool CreateTextures(FRenderer &Renderer);
  bool InitializeMaterials(FRenderer &Renderer);
  bool CreateEditTextures(FRenderer& Renderer);

  FRenderer *RendererRef = nullptr;
};