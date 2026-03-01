#include "AssetHandle.h"

#include "AssetManager.h"

namespace se::assets {

template <typename T>
std::shared_ptr<T> AssetHandle<T>::get() const {
    if (!isValid() || !m_AssetManager) return nullptr;
    return m_AssetManager->template getAssetPtr<T>(m_Id);
}

template class AssetHandle<Model>;
template class AssetHandle<Shader>;
template class AssetHandle<Texture>;
template class AssetHandle<Material>;

}  // namespace se::assets
