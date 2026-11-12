# ModelViewerDx11

D3D11로 만든 간단한 모델 뷰어를 Vulkan으로 포팅한 프로젝트입니다.

---

## 개발 환경

- Visual Studio 2022 (`ModelViewerDx11.sln`)
- Vulkan SDK 1.4 이상 (`VULKAN_SDK` 환경변수 사용)
- Vulkan 1.3 이상 지원 GPU (확인: RTX 5070, RTX 4060 Laptop)
- DXC: HLSL 셰이더를 SPIR-V로 컴파일 (`Renderer/Shader/compileShaders.bat`)
- FBX SDK
- DirectXTex (NuGet)

---

## 아키텍처 구조

```
Application          메인 루프 · 씬 객체 소유
├── Window           윈도우 생성 · 메시지 루프
├── Core             DirectInput · Timer
├── Scene            Camera · Light · Sky · Floor · Billboard
├── Util             Define · Macro · Type
└── Renderer         Renderer · FrameContext · Loader · Uploader · VkHelper
    ├── Resources    ResourceManager · PipelineBuilder · Buffer · Image · Texture · Sampler
    │                Model · Mesh · Material
    ├── Shader       DescriptorLayout · DescriptorAllocator · DescriptorSet
    │                Shader · ShaderModule · ShaderSources/*.hlsl
    ├── Importer     ModelImporter (FBX)
    └── Primitive    MeshGenerator
```

---

## 주요 클래스

| 클래스 | 역할 |
|---|---|
| Renderer | 디바이스 · 스왑체인 · 렌더 루프, 패스 시작/종료, 파이프라인 생성 |
| FrameContext | 프레임(in-flight)별 커맨드 버퍼 · 동기화 객체 · 섀도맵 관리 |
| ResourceManager | 셰이더 · 샘플러 · 파이프라인 · 텍스처 · 정점/인덱스 버퍼 소유, 해시로 조회 |
| Loader | 메시 데이터를 리소스로 등록하고 파이프라인 · 디스크립터 세트를 구성해 물체에 전달 |
| DescriptorAllocator | 레이아웃별 디스크립터 풀 관리, 세트 할당 |

---
## 이전 D3D11 모델 뷰어 구조

마지막 D3D11 구조: [5df8283](https://github.com/SkyLakeARIS/ModelViewer/tree/5df82839d8674f6902c193bcd29a093e6d5e9138) (2026-09-21)

- Deferred Shading: G-Buffer 5장(MRT)에 기록한 뒤 라이팅 패스에서 합성
- Render Command Queue: 물체가 직접 그리지 않고 커맨드를 리스트에 담은 뒤,
  64bit SortKey(렌더패스 · 투명 여부 > 셰이더 · 텍스처 > 렌더 상태)로 정렬하고
  직전 상태와 비교해 달라진 것만 바인딩
- 버퍼 관리: 정점/인덱스 버퍼를 청크 단위로 관리하고, 빈 공간 재사용 · 병합 · 자동 확장 지원

```
Application          메인 루프 · 커맨드 리스트 소유
├── Window           윈도우 생성 · 메시지 루프
├── Core             DirectInput · Timer
├── Scene            Camera · Light · Sky · Floor · Billboard
├── UI               DebugPanel
├── Util             Define · Macro · Type
└── Renderer         Renderer (Device · SwapChain · 렌더타겟 · 상태 바인드)
    ├── Resources    ResourceManager · BufferManager · TextureManager
    │                RenderPacket · Model · Mesh · Material
    ├── Shader       ShaderManager · ShaderType · ShaderSources/*.hlsl
    ├── Importer     ModelImporter (FBX)
    └── Primitive    MeshGenerator
```