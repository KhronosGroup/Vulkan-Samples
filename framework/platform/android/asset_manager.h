#pragma once
#include <android/asset_manager.h>

#include <string>

namespace vkb
{
namespace fs
{
class AssetManager final
{
    public:
        // Singleton class
        ~AssetManager() = default;
        AssetManager(const AssetManager&) = delete;
        AssetManager(AssetManager&&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager& operator=(AssetManager&&) = delete;

        static AssetManager& get();
        static void set_android_asset_manager(AAssetManager* asset_manager);
        static AAssetManager* get_android_asset_manager();

        static std::vector<uint8_t> read_binary_file(const std::string& filename);
    private:
        AAsset* open_asset(const std::string& filename);
        void close_asset(AAsset* asset);
        size_t get_asset_size(AAsset* asset);
        size_t read_asset(AAsset* asset, void* buffer, size_t count);

        AssetManager() = default;
        AAssetManager* android_asset_manager = nullptr;
};
}
}
