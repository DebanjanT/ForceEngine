#pragma once

#include "Force/Asset/Asset.h"
#include "Force/Renderer/Texture.h"

namespace Force
{
    class TextureAsset : public Asset
    {
    public:
        FORCE_ASSET_TYPE(Texture2D)

        TextureAsset() = default;
        ~TextureAsset() override = default;

        Ref<Texture2D> GetTexture() const { return m_Texture; }
        void SetTexture(Ref<Texture2D> texture) { m_Texture = texture; }

        u32 GetWidth() const { return m_Texture ? m_Texture->GetWidth() : 0; }
        u32 GetHeight() const { return m_Texture ? m_Texture->GetHeight() : 0; }

    private:
        friend class TextureImporter;
        Ref<Texture2D> m_Texture;
    };
}
