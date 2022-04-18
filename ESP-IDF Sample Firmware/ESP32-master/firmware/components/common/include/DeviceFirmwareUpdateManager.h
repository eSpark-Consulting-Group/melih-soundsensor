#pragma once

#include <string>
#include <functional>
#include <memory>
#include <set>
#include <map>
#include <vector>
#include <list>
#include <limits>

namespace common {

constexpr size_t DefaultMaxDownloadsCount = 1;
constexpr uint8_t MaxProgress = 100;

class DeviceFirmwareUpdateManager {
public:
    using Device = uint8_t;
    using Link = std::string;
    using Path = std::string;
    using Target = uint32_t;

    using Devices = std::vector<Device>;

    enum class TargetUpdateResult : uint8_t { DownloadFailed, UpdateDone, UpdateFailed };

    struct ExtendedLink {
        Link image;
        Link checksum;
    };

    using TargetsToLinks = std::map<Target, ExtendedLink>;

    using TargetsUpdateResults = std::map<Target, TargetUpdateResult>;

    class Downloader {
    public:
        virtual ~Downloader() = default;

        virtual bool isOk() const = 0;

        virtual uint8_t getProgress() const = 0;

        using ResultCallback = std::function<void( const Downloader& )>;
    };

    using DownloaderConstUPtr = std::unique_ptr<const Downloader>;

    class Delegate {
    public:
        virtual ~Delegate() = default;

        virtual DownloaderConstUPtr createFirmwareDownloader( const ExtendedLink& link,
                                                              const Path& destination,
                                                              Downloader::ResultCallback resultCallback ) = 0;

        virtual bool startFirmwareUpdate( const Device& device,
                                          const Target& target,
                                          const Path& destination,
                                          std::function<void( bool )> resultCallback ) = 0;

        virtual void onFirmwareDownloadStatus( const ExtendedLink& link, bool isOk, uint8_t progress ) = 0;

        virtual void addFirmwareUpdateTask( std::function<void()> task ) = 0;

        virtual void onFirmwareUpdateStarted( const Device& device ) = 0;

        virtual void onFirmwareUpdateFinished( const Device& device, const TargetsUpdateResults& targetsResults ) = 0;

        virtual Path getDestinationForLink( const Link& link ) = 0;
    };

private:
    enum class DownloadingStatus : uint8_t { Initial, Started, Done, Failed };

    struct LinkInfo {
        Path path = "";
        DownloaderConstUPtr downloader = nullptr;
        DownloadingStatus downloadingStatus = DownloadingStatus::Initial;
        size_t usageCount = 0;
    };

    struct ExtendedLinkComparator {
        bool operator()( const ExtendedLink& a, const ExtendedLink& b ) const;
    };

    using LinksPool = std::map<ExtendedLink, LinkInfo, ExtendedLinkComparator>;

    struct TargetInfo {
        std::reference_wrapper<const ExtendedLink> link;
    };

    using TargetsInfo = std::map<Target, TargetInfo>;

    struct DevicesScope {
        Devices devices;
        TargetsInfo targets;
    };

    using DevicesScopes = std::list<DevicesScope>;

    class DownloadingIterator {
    public:
        DownloadingIterator( DeviceFirmwareUpdateManager& owner, DevicesScopes::iterator scopesIterator );

        operator bool() const;

        const ExtendedLink& getLink();

        DownloadingIterator& operator++();

    private:
        void adjust();

    private:
        std::reference_wrapper<DeviceFirmwareUpdateManager> m_owner;
        DevicesScopes::iterator m_scopesIterator;
        TargetsInfo::iterator m_targetsIterator{};
    };

    class UpdatingIterator {
    public:
        UpdatingIterator( DeviceFirmwareUpdateManager& owner, DevicesScopes::iterator scopesIterator );

        operator bool() const;

        const ExtendedLink& getLink() const;

        const Device& getDevice() const;

        const Target& getTarget() const;

        DevicesScopes::const_iterator getScopesIterator() const;

        UpdatingIterator& operator++();

    private:
        void adjust();

    private:
        std::reference_wrapper<DeviceFirmwareUpdateManager> m_owner;
        DevicesScopes::iterator m_scopesIterator;
        Devices::iterator m_devicesIterator{};
        TargetsInfo::iterator m_targetsIterator{};
    };

public:
    DeviceFirmwareUpdateManager( Delegate& delegate,
                                 size_t maxDownloadsCount = DefaultMaxDownloadsCount,
                                 size_t maxStoredImagesCount = std::numeric_limits<size_t>::max() );

    ~DeviceFirmwareUpdateManager();

    void addUpdateScope( Devices&& devices, TargetsToLinks&& targetsToLinks );

    void onFirmwareUpdateFinished( const Device& device, const Target& target, bool result );

    const ExtendedLink* getActiveUpdateLink() const;

private:
    void doDownloadStep();

    void doUpdateStep();

    void cleanUpScopes();

    DownloadingStatus getDownloadingStatus( const ExtendedLink& link ) const;

    void onDownloadStarted( const ExtendedLink& link, DownloaderConstUPtr downloader );

    void onDownloadFinished( const ExtendedLink& link, Path path );

    void onUpdateStarted();

    void onUpdateFinished( UpdatingIterator updatingIterator, TargetUpdateResult result );

private:
    Delegate& m_delegate;
    const size_t m_maxDownloadsCount;
    const size_t m_maxStoredImagesCount;
    bool m_downloadRequired = true;

    LinksPool m_linksPool;
    DevicesScopes m_scopes;
    size_t m_downloadsCount = 0;
    size_t m_storedImagesCount = 0;

    TargetsUpdateResults m_targetsResults;

    DownloadingIterator m_downloadingIterator{ *this, m_scopes.begin() };
    UpdatingIterator m_updatingIterator{ *this, m_scopes.begin() };
    UpdatingIterator m_activeUpdatingIterator{ *this, m_scopes.end() };
};

}  // namespace common
