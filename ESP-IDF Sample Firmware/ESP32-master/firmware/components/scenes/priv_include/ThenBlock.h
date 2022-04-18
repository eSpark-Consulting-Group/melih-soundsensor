#pragma once

#include "Common.h"
#include "Time.h"
#include "ThenBlockVisitor.h"
#include "values/ScalebleValue.h"

namespace storage {
class KeyValueStorage;
}

namespace scenes {

class ThenBlock {
    enum SerializationMarker { TypeMarker, ObjectDataMarker };

public:
    enum class Type : std::uint8_t {
        SetItemValue,
        SetDeviceArmed,
        SwitchHouseMode,
        ExecuteScene,
        CloudCommand,
        SetValueMulticast,
        SetValueMulticast2,
        SendHttpRequest,
        SetValueMulticast3
    };
    using UPtr = std::unique_ptr<ThenBlock>;

    static const auto SaveBufferSize = BufferStreamType::MiddleBufSize;

    static const char* StorPrefix;

    static UPtr create( KVSValue& value );

    static UPtr create( const BlobType& blob );

    static UPtr create( common::IReadStream& stream );

    static UPtr create( const Uuid uuid, storage::KeyValueStorage& stor );

    static bool remove( const Uuid uuid, storage::KeyValueStorage& stor ) {
        return stor.remove( uuid.toHexString( StorPrefix ) );
    }

    ThenBlock();

    ThenBlock( const Uuid& uuid, const Uuid& sceneUuid, const DelayTypeSec& delay );

    const auto& uuid() const {
        return m_uuid;
    }

    const auto& sceneUuid() const {
        return m_sceneUuid;
    }

    const auto& delay() const {
        return m_delay;
    }

    const auto& creationTime() const {
        return m_creationTime;
    }

    static StringType storKey( const Uuid& uuid );

    StringType storKey() const;

    bool save( storage::KeyValueStorage& stor ) const;

    virtual ~ThenBlock() = default;

    virtual Type type() const = 0;

    virtual void accept( ThenBlockConstVisitor& visitor ) const = 0;

    virtual void accept( ThenBlockVisitor& visitor ) = 0;

    virtual bool write( common::IWriteStream& stream ) const = 0;

    virtual bool read( common::IReadStream& stream ) = 0;

protected:
    Uuid m_uuid;
    Uuid m_sceneUuid;
    DelayTypeSec m_delay;
    TimePointType m_creationTime;
};

class ThenBlockSetItemValue : public ThenBlock {
public:
    ThenBlockSetItemValue() = default;

    ThenBlockSetItemValue( const Uuid& uuid,
                           const Uuid& sceneUuid,
                           const DelayTypeSec& delay,
                           const char* itemId,
                           ValueTypeUPtr&& itemValue );

    ThenBlockSetItemValue( const Uuid& uuid,
                           const Uuid& sceneUuid,
                           const DelayTypeSec& delay,
                           StringType&& itemId,
                           ValueTypeUPtr&& itemValue );

    inline const StringType& itemId() const {
        return m_itemId;
    }

    inline const ValueType& itemValue() const {
        return *m_itemValue;
    }

    Type type() const override {
        return Type::SetItemValue;
    }

    void accept( ThenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

    void accept( ThenBlockVisitor& visitor ) override {
        visitor.visit( *this );
    }

    bool read( common::IReadStream& stream ) override;

    bool write( common::IWriteStream& stream ) const override;

private:
    StringType m_itemId;
    ValueTypeUPtr m_itemValue;
};

class ThenBlockSendHttpRequest : public ThenBlock {
public:
    ThenBlockSendHttpRequest() = default;

    ThenBlockSendHttpRequest( const Uuid& uuid,
                              const Uuid& sceneUuid,
                              const DelayTypeSec& delay,
                              StringType&& url,
                              StringType&& request,
                              StringType&& user,
                              StringType&& password,
                              StringType&& contentType,
                              StringType&& content,
                              std::map<StringType, StringType> headers,
                              bool skipSecurity );

    Type type() const override {
        return Type::SendHttpRequest;
    }

    void accept( ThenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

    void accept( ThenBlockVisitor& visitor ) override {
        visitor.visit( *this );
    }

    bool read( common::IReadStream& stream ) override;

    bool write( common::IWriteStream& stream ) const override;

    inline std::string url() const {
        return m_url;
    }

    inline std::string user() const {
        return m_user;
    }

    inline std::string password() const {
        return m_password;
    }

    inline std::string request() const {
        return m_request;
    }

    inline std::string contentType() const {
        return m_contentType;
    }

    inline std::string content() const {
        return m_content;
    }

    inline std::map<StringType, StringType> headers() const {
        return m_headers;
    }

    inline bool skipSecurity() const {
        return m_skipSecurity;
    }

private:
    StringType m_url;
    StringType m_request;
    StringType m_user;
    StringType m_password;
    StringType m_contentType;
    StringType m_content;
    std::map<StringType, StringType> m_headers;
    bool m_skipSecurity;
};

class[[deprecated]] ThenBlockSetValueMulticast : public ThenBlock {
public:
    using UPtr = std::unique_ptr<ThenBlockSetValueMulticast>;
    using ItemsIdList = std::list<StringType>;
    using ValueToItemsMap = std::map<ValueTypeUPtr, ItemsIdList>;

    using ThenBlock::ThenBlock;

    inline const auto& valueToItems() const {
        return m_valToItems;
    }

    inline auto& valueToItems() {
        return m_valToItems;
    }

    int itemsCount() const;

    Type type() const override {
        return Type::SetValueMulticast;
    }

    void accept( ThenBlockConstVisitor & visitor ) const override {
        visitor.visit( *this );
    }

    void accept( ThenBlockVisitor & visitor ) override {
        visitor.visit( *this );
    }

    bool read( common::IReadStream & stream ) override;

    bool write( common::IWriteStream & stream ) const override;

private:
    ValueToItemsMap m_valToItems;
};

class ThenBlockSetValueMulticast2 : public ThenBlock {
public:
    using UPtr = std::unique_ptr<ThenBlockSetValueMulticast2>;
    using KeyType = std::pair<common::IValue::IValueId, ValueTypeUPtr>;
    using ItemsIdList = std::list<StringType>;
    using ValueToItemsMap = std::map<KeyType, ItemsIdList>;

    using ThenBlock::ThenBlock;

    inline const auto& valueToItems() const {
        return m_valToItems;
    }

    inline auto& valueToItems() {
        return m_valToItems;
    }

    int itemsCount() const;

    Type type() const override {
        return Type::SetValueMulticast2;
    }

    void accept( ThenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

    void accept( ThenBlockVisitor& visitor ) override {
        visitor.visit( *this );
    }

    bool read( common::IReadStream& stream ) override;

    bool write( common::IWriteStream& stream ) const override;

private:
    ValueToItemsMap m_valToItems;
};

class ThenBlockSetValueMulticast3 : public ThenBlock {
public:
    using UPtr = std::unique_ptr<ThenBlockSetValueMulticast3>;
    using KeyType = std::pair<common::IValue::IValueId, ValueTypeUPtr>;
    using ItemsIdList = std::list<StringType>;
    using ValueToItemsMap = std::map<KeyType, ItemsIdList>;
    using KeysList = std::list<KeyType>;

    using ThenBlock::ThenBlock;

    inline const auto& valueToItems() const {
        return m_valToItems;
    }

    inline auto& valueToItems() {
        return m_valToItems;
    }

    int itemsCount() const;

    Type type() const override {
        return Type::SetValueMulticast3;
    }

    void accept( ThenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

    void accept( ThenBlockVisitor& visitor ) override {
        visitor.visit( *this );
    }

    bool read( common::IReadStream& stream ) override;

    bool write( common::IWriteStream& stream ) const override;

    bool addToIndex( KeyType key );

    inline const auto& indexes() const {
        return m_indexes;
    }

private:
    ValueToItemsMap m_valToItems;
    KeysList m_indexes;
};

class ThenBlockSetDeviceArmed final : public ThenBlock {
public:
    ThenBlockSetDeviceArmed() = default;

    ThenBlockSetDeviceArmed( const Uuid& uuid,
                             const Uuid& sceneUuid,
                             const DelayTypeSec& delay,
                             StringType&& deviceId,
                             const bool isArmed );

    inline const StringType& deviceId() const {
        return m_deviceId;
    }

    inline bool isArmed() const {
        return m_isArmed;
    }

    Type type() const override {
        return Type::SetDeviceArmed;
    }

    void accept( ThenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

    void accept( ThenBlockVisitor& visitor ) override {
        visitor.visit( *this );
    }

    bool read( common::IReadStream& stream ) override;

    bool write( common::IWriteStream& stream ) const override;

private:
    StringType m_deviceId;
    bool m_isArmed = false;
};

#ifdef CONFIG_ADVANCED_SCENES
class ThenBlockSwitchHouseMode final : public ThenBlock {
public:
    ThenBlockSwitchHouseMode() = default;

    ThenBlockSwitchHouseMode( const Uuid& uuid,
                              const Uuid& sceneUuid,
                              const DelayTypeSec& delay,
                              StringType&& houseMode );

    inline const auto& houseMode() const {
        return m_houseMode;
    }

    Type type() const override {
        return Type::SwitchHouseMode;
    }

    void accept( ThenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

    void accept( ThenBlockVisitor& visitor ) override {
        visitor.visit( *this );
    }

    bool read( common::IReadStream& stream ) override;

    bool write( common::IWriteStream& stream ) const override;

private:
    StringType m_houseMode;
};

class ThenBlockExecuteScene final : public ThenBlock {
public:
    ThenBlockExecuteScene() = default;

    ThenBlockExecuteScene( const Uuid& uuid,
                           const Uuid& sceneUuid,
                           const DelayTypeSec& delay,
                           StringType&& sceneStorId );

    inline const auto& sceneStorId() const {
        return m_sceneStorId;
    }

    Type type() const override {
        return Type::ExecuteScene;
    }

    void accept( ThenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

    void accept( ThenBlockVisitor& visitor ) override {
        visitor.visit( *this );
    }

    bool read( common::IReadStream& stream ) override;

    bool write( common::IWriteStream& stream ) const override;

private:
    StringType m_sceneStorId;
};
#endif  // CONFIG_ADVANCED_SCENES

class ThenBlockCloudCommand final : public ThenBlock {
public:
    ThenBlockCloudCommand() = default;

    ThenBlockCloudCommand( const Uuid& _uuid,
                           const Uuid& _sceneUuid,
                           const DelayTypeSec& _delay,
                           StringType&& _commandId,
                           StringType&& _capability,
                           StringType&& _command,
                           ValueTypeUPtr&& _value );

    inline const auto& commandId() const {
        return m_commandId;
    }

    inline const auto& capability() const {
        return m_capability;
    }

    inline const auto& command() const {
        return m_command;
    }

    inline const auto& value() const {
        return m_value;
    }

    Type type() const override {
        return Type::CloudCommand;
    }

    void accept( ThenBlockConstVisitor& visitor ) const override {
        visitor.visit( *this );
    }

    void accept( ThenBlockVisitor& visitor ) override {
        visitor.visit( *this );
    }

    bool read( common::IReadStream& stream ) override;

    bool write( common::IWriteStream& stream ) const override;

private:
    StringType m_commandId;
    StringType m_capability;
    StringType m_command;
    ValueTypeUPtr m_value;
};

}  // end of namespace scenes
