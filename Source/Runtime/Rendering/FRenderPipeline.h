#pragma once

#include "Runtime/Material/FBlendDesc.h"
#include "Runtime/Material/FRasterizerDesc.h"
#include "Runtime/Material/FDepthStencilDesc.h"
#include "Runtime/Core/FString.h"
#include "Vertices.h"
#include <d3d11.h>
#include <wrl/client.h>

#include "Runtime/Core/IntTypes.h"

struct FRenderPipelineDesc
{
  FString VertexShaderFilePath;
  FString PixelShaderFilePath;
  FBlendDesc Blend;
  FRasterizerDesc Rasterizer;
  FDepthStencilDesc DepthStencil;
  bool bIsInstancing = false;
};

struct FRenderPipelineCreateInfo
{
  FRenderPipelineDesc Desc;

  Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> PixelShader;
  Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;

  Microsoft::WRL::ComPtr<ID3D11RasterizerState> RasterizerState;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilState> DepthStencilState;
  Microsoft::WRL::ComPtr<ID3D11SamplerState> SamplerState;
  Microsoft::WRL::ComPtr<ID3D11BlendState> BlendState;

  bool bIsInstancing = false;
};

class FRenderPipeline final {
  friend class FRenderer;
  friend class FLineBatcher;
  friend class FRenderResourceLibrary;

public:
  explicit FRenderPipeline(FRenderPipelineCreateInfo CreateInfo);

  FRenderPipelineDesc GetPipelineDesc() const { return desc; }
  void SetStencilRef(UINT InRef) { StencilRef = InRef; }
  UINT GetStencilRef() const { return StencilRef; }

private:
  FRenderPipelineDesc desc;
  UINT StencilRef = 0;

  void Bind(ID3D11DeviceContext &Context) const;

  Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> PixelShader;
  Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;

  Microsoft::WRL::ComPtr<ID3D11RasterizerState> RasterizerState;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilState> DepthStencilState;
  Microsoft::WRL::ComPtr<ID3D11SamplerState> SamplerState;
  Microsoft::WRL::ComPtr<ID3D11BlendState> BlendState;
};
