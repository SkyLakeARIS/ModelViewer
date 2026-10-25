#include "Floor.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Primitive/MeshGenerator.h"
#include "../Util/Macro.h"

namespace scene
{
    Floor::Floor(XMFLOAT2 startPoint, uint32_t gapEachLine, uint32_t numLineX, uint32_t numLineY)
    {
        ASSERT(numLineX >= 2, "numLineX must be 2 or greater");
        ASSERT(numLineY >= 2, "numLineY must be 2 or greater");
        ASSERT(gapEachLine >= 1, "gapEachLine must be 1 or greater");

        renderer::MeshGenerator::CreateGrid(startPoint, numLineX, numLineY, gapEachLine, mMesh);
        for (auto& subMesh : mMesh.SubMeshes)
        {
            subMesh.Material.Factors.Diffuse = XMFLOAT3(0.80f, 0.72f, 0.56f);
            subMesh.Material.Factors.IsLitOn = static_cast<float>(true);
            subMesh.Material.Factors.Ambient = XMFLOAT3(0.2f, 0.2f, 0.2f);
            subMesh.Material.Factors.Shininess = 8;
            subMesh.Material.Factors.Specular = XMFLOAT3(0.16f, 0.144f, 0.112f);
        }
    }

    Floor::~Floor()
    {
    }

    void Floor::SubmitCommand()
    {
    }
}
