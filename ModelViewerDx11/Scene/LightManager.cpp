#include "LightManager.h"
#include "Light.h"

namespace scene
{
    LightManager* LightManager::mInstance = nullptr;
    uint32 LightManager::mIdCount = 0;

    LightManager* LightManager::GetInstance()
    {
        if (!mInstance)
        {
            mInstance = new LightManager;
        }
        return mInstance;
    }

    void LightManager::Release()
    {
        delete mInstance;
        mInstance = nullptr;
    }

    void LightManager::AddLight(Light* const light, uint32& outId)
    {
        if (mLightCount >= MAX_LIGHT_NUM)
        {
            return;
        }
        outId = mIdCount++;
        LightContainer* newLight = new LightContainer(light, outId);
        mLights[mLightCount++] = newLight;
    }

    void LightManager::FindLight(uint32 id, Light* outLight)
    {
        for (const LightContainer* const container : mLights)
        {
            if (container->Id == id)
            {
                outLight = container->Light;
                break;
            }
        }
    }

    void LightManager::RemoveLight(uint32 id)
    {
        for (LightContainer* const container : mLights)
        {
            if (container->Id == id)
            {
                delete container->Light;
                container->Light = nullptr;

                container->Light = mLights[mLightCount - 1]->Light;
                container->Id = mLights[mLightCount - 1]->Id;

                mLights[mLightCount - 1]->Light = nullptr;
                delete mLights[mLightCount - 1];
                mLights[mLightCount - 1] = nullptr;
                break;
            }
        }
        --mLightCount;
    }

    LightManager::LightManager()
        : mLights{ nullptr, }
        , mLightCount(0)
    {
    }

    LightManager::~LightManager()
    {
        for (LightContainer* light : mLights)
        {
            if (light)
            {
                delete light->Light;
                light->Light = nullptr;

                delete light;
            }
        }
    }
}
