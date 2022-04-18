#pragma once

#include "Bus.h"
#include "Shutdownable.h"

#include <string>
#include <cinttypes>
#include <tuple>
#include <vector>
#include <memory>

namespace common {
class JsonStreamGenerator;
};  // namespace common

namespace core {

class PagesManager : public SingletonBaseImpl<PagesManager> {
    friend class SingletonBaseImpl<PagesManager>;

public:
    using ParentType = SingletonBaseImpl<PagesManager>;
    using ClbkRet = ParentType::ClbkRet;

    using SPtr = std::shared_ptr<PagesManager>;
    using WPtr = std::weak_ptr<PagesManager>;

    using PageId = uint32_t;

    // Default pages information.
    constexpr static char BedroomName[] = "Bedroom";
    constexpr static char LivingroomName[] = "Living room";
    constexpr static const auto MaxNameLen = 30;

public:
    ~PagesManager();

    bool setPageName( const PageId& pageId, const std::string& name, bool is_edited = true );

    std::tuple<int, std::string> createPage( const std::string& name, PageId& resultId );

    bool deletePage( const PageId& pageId );

    bool deleteAllPages();

    bool listPages( common::JsonStreamGenerator& jsonGenerator );

    bool listPagesWithFilter( common::JsonStreamGenerator& jsonGenerator, std::vector<std::string> ids );

    bool listPage( common::JsonStreamGenerator& jsonGenerator, const PageId& pageId );

    bool setPageOrder( const PageId* pageIds, uint8_t count );

    bool isPageWithId( const PageId& pageId );

    bool isPageWithName( const std::string& name );

    std::string getPageName( const PageId& pageId );

    std::string getPageName( const std::string& pageId );

private:  // SingletonBaseImpl
    bool initializeImpl() override;

    ClbkRet updateComponent( UpdContext& ctx ) override {
        return ClbkRet::Continue;
    }

private:
    using SingletonBaseImpl<PagesManager>::SingletonBaseImpl;
};

}  // namespace core
