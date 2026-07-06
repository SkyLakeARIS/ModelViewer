#pragma once
#include "../framework.h"

namespace scene
{
    class Light;

    class LightManager final
    {
    private:
        static const uint32 MAX_LIGHT_NUM = 16;

        struct LightContainer
        {
            LightContainer(Light* const light, uint32 id)
                : Light(light)
                , Id(id)
            {
            }
            Light* Light;
            uint32 Id;
        };
    public:

        static LightManager* GetInstance();
        static void             Release();

        void AddLight(Light* const light, uint32& outId);

        void FindLight(uint32 id, Light* outLight);
        void RemoveLight(uint32 id);
    private:

        LightManager();
        ~LightManager();
    private:
        static LightManager* mInstance;
        static uint32       mIdCount;

        LightContainer* mLights[MAX_LIGHT_NUM];
        uint32_t mLightCount;

        static Light* GlobalLight;
    };
}