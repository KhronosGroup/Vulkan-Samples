#include "asset_manager.h"

#include "common/logging.h"

namespace vkb
{
namespace fs
{

AssetManager& AssetManager::get()
{
    static AssetManager manager;
    return manager;
}

void AssetManager::set_android_asset_manager(AAssetManager* asset_manager)
{
    get().android_asset_manager = asset_manager;
}

AAssetManager* AssetManager::get_android_asset_manager()
{
    return get().android_asset_manager;
}

std::vector<uint8_t> AssetManager::read_binary_file(const std::string& filename)
{
    auto& assetManager = get();
    auto asset = assetManager.open_asset(filename);
    size_t size = assetManager.get_asset_size(asset);

    if(size == 0)
    {
        // No need to log error here as previous calls will give enough information
        return {};
    }
    else
    {
        std::vector<uint8_t> bytes(size);

        assetManager.read_asset(asset, bytes.data(), size);
        assetManager.close_asset(asset);
        return bytes;
    }
}

AAsset* AssetManager::open_asset(const std::string& filename)
{
    AAsset* asset = AAssetManager_open(android_asset_manager, filename.c_str(), AASSET_MODE_STREAMING);
    if(asset == nullptr)
    {
        LOGE("Failed to open file: {}", filename);
        return nullptr;
    }
    return asset;
}

void AssetManager::close_asset(AAsset* asset)
{
    AAsset_close(asset);
}

size_t AssetManager::get_asset_size(AAsset* asset)
{
    if(asset == nullptr)
    {
        LOGE("Tried to read size of a null asset");
        return 0;
    }
    else
    {
        return AAsset_getLength(asset);
    }
}

size_t AssetManager::read_asset(AAsset* asset, void* buffer, size_t count)
{
    return AAsset_read(asset, buffer, count);
}

}
}
