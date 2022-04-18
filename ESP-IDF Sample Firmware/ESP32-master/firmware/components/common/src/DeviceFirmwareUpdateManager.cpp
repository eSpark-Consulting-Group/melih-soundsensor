#include "DeviceFirmwareUpdateManager.h"
#include <DebugUtils.h>

namespace common {

DeviceFirmwareUpdateManager::DownloadingIterator::DownloadingIterator( DeviceFirmwareUpdateManager& owner,
                                                                       DevicesScopes::iterator scopesIterator )
: m_owner( owner )
, m_scopesIterator( scopesIterator ) {
    if( *this ) {
        m_targetsIterator = m_scopesIterator->targets.begin();
        adjust();
    }
}

DeviceFirmwareUpdateManager::DownloadingIterator::operator bool() const {
    return m_scopesIterator != m_owner.get().m_scopes.end();
}

DeviceFirmwareUpdateManager::DownloadingIterator& DeviceFirmwareUpdateManager::DownloadingIterator::operator++() {
    ++m_targetsIterator;
    adjust();
    return *this;
}

const DeviceFirmwareUpdateManager::ExtendedLink& DeviceFirmwareUpdateManager::DownloadingIterator::getLink() {
    return m_targetsIterator->second.link;
}

void DeviceFirmwareUpdateManager::DownloadingIterator::adjust() {
    while( *this ) {
        if( m_targetsIterator == m_scopesIterator->targets.end() ) {
            ++m_scopesIterator;
            m_targetsIterator = *this ? m_scopesIterator->targets.begin() : TargetsInfo::iterator{};
        }
        else if( m_owner.get().getDownloadingStatus( getLink() ) == DownloadingStatus::Initial ) {
            break;
        }
        else {
            ++m_targetsIterator;
        }
    }
}

DeviceFirmwareUpdateManager::UpdatingIterator::UpdatingIterator( DeviceFirmwareUpdateManager& owner,
                                                                 DevicesScopes::iterator scopesIterator )
: m_owner( owner )
, m_scopesIterator( scopesIterator ) {
    if( *this ) {
        m_devicesIterator = m_scopesIterator->devices.begin();
        m_targetsIterator = m_scopesIterator->targets.begin();
        adjust();
    }
}

DeviceFirmwareUpdateManager::UpdatingIterator::operator bool() const {
    return m_scopesIterator != m_owner.get().m_scopes.end();
}

DeviceFirmwareUpdateManager::UpdatingIterator& DeviceFirmwareUpdateManager::UpdatingIterator::operator++() {
    ++m_targetsIterator;
    adjust();
    return *this;
}

const DeviceFirmwareUpdateManager::ExtendedLink& DeviceFirmwareUpdateManager::UpdatingIterator::getLink() const {
    return m_targetsIterator->second.link;
}

const DeviceFirmwareUpdateManager::Device& DeviceFirmwareUpdateManager::UpdatingIterator::getDevice() const {
    return *m_devicesIterator;
}

const DeviceFirmwareUpdateManager::Target& DeviceFirmwareUpdateManager::UpdatingIterator::getTarget() const {
    return m_targetsIterator->first;
}

DeviceFirmwareUpdateManager::DevicesScopes::const_iterator
DeviceFirmwareUpdateManager::UpdatingIterator::getScopesIterator() const {
    return m_scopesIterator;
}

void DeviceFirmwareUpdateManager::UpdatingIterator::adjust() {
    if( m_targetsIterator == m_scopesIterator->targets.end() ) {
        ++m_devicesIterator;

        if( m_devicesIterator == m_scopesIterator->devices.end() ) {
            ++m_scopesIterator;

            m_targetsIterator = *this ? m_scopesIterator->targets.begin() : TargetsInfo::iterator{};
            m_devicesIterator = *this ? m_scopesIterator->devices.begin() : Devices::iterator{};
        }
        else {
            m_targetsIterator = m_scopesIterator->targets.begin();
        }
    }
}

bool DeviceFirmwareUpdateManager::ExtendedLinkComparator::operator()( const ExtendedLink& a,
                                                                      const ExtendedLink& b ) const {
    return a.image < b.image;
}
DeviceFirmwareUpdateManager::DeviceFirmwareUpdateManager( Delegate& delegate,
                                                          size_t maxDownloadsCount,
                                                          size_t maxStoredImagesCount )
: m_delegate( delegate )
, m_maxDownloadsCount( maxDownloadsCount )
, m_maxStoredImagesCount( maxStoredImagesCount ) {}

DeviceFirmwareUpdateManager::~DeviceFirmwareUpdateManager() {
    for( auto& linkToPath : m_linksPool ) {
        if( !linkToPath.second.path.empty() ) {
            remove( linkToPath.second.path.c_str() );
        }
    }
}

void DeviceFirmwareUpdateManager::addUpdateScope( Devices&& devices, TargetsToLinks&& targetsToLinks ) {
    DevicesScope scope;
    scope.devices = std::move( devices );
    for( auto& targetToLink : targetsToLinks ) {
        const auto it = m_linksPool.emplace( std::move( targetToLink.second ), LinkInfo{} ).first;
        it->second.usageCount += scope.devices.size();
        scope.targets.emplace( targetToLink.first, TargetInfo{ it->first } );
    }

    m_scopes.push_back( std::move( scope ) );

    if( !m_downloadingIterator ) {
        m_downloadingIterator = DownloadingIterator( *this, std::prev( m_scopes.end() ) );
    }

    if( !m_updatingIterator ) {
        m_updatingIterator = UpdatingIterator( *this, std::prev( m_scopes.end() ) );
    }

    doDownloadStep();
}

const DeviceFirmwareUpdateManager::ExtendedLink* DeviceFirmwareUpdateManager::getActiveUpdateLink() const {
    return m_activeUpdatingIterator ? &m_activeUpdatingIterator.getLink() : nullptr;
}

void DeviceFirmwareUpdateManager::onFirmwareUpdateFinished( const Device& device, const Target& target, bool result ) {
    const auto isCallbackValid = m_activeUpdatingIterator && m_activeUpdatingIterator.getDevice() == device &&
                                 m_activeUpdatingIterator.getTarget() == target;
    if( isCallbackValid ) {
        onUpdateFinished( m_activeUpdatingIterator,
                          result ? TargetUpdateResult::UpdateDone : TargetUpdateResult::UpdateFailed );
        doUpdateStep();
    }
}

void DeviceFirmwareUpdateManager::doDownloadStep() {
    while( m_downloadingIterator &&                                                   // has more links for downloading
           ( m_downloadRequired || m_storedImagesCount < m_maxStoredImagesCount ) &&  // check max stored images count
           m_downloadsCount < m_maxDownloadsCount                                     // check max parallel downloads
    ) {
        const auto& link = m_downloadingIterator.getLink();
        auto path = m_delegate.getDestinationForLink( link.image );

        auto downloader = m_delegate.createFirmwareDownloader( link, path, [=]( const Downloader& downloader ) {
            const auto isOk = downloader.isOk();
            const auto progress = downloader.getProgress();

            m_delegate.addFirmwareUpdateTask( [=]() {
                m_delegate.onFirmwareDownloadStatus( link, isOk, progress );

                if( !isOk || progress == MaxProgress ) {
                    onDownloadFinished( link, isOk ? path : Path{} );
                }
            } );
        } );

        onDownloadStarted( link, std::move( downloader ) );
        ++m_downloadingIterator;
    }
}

void DeviceFirmwareUpdateManager::doUpdateStep() {
    while( !m_activeUpdatingIterator && m_updatingIterator ) {
        const auto& link = m_updatingIterator.getLink();
        auto linkIt = m_linksPool.find( link );
        const auto downloadStatus = linkIt->second.downloadingStatus;

        if( downloadStatus == DownloadingStatus::Started ) {
            break;
        }

        if( downloadStatus == DownloadingStatus::Initial ) {
            m_downloadRequired = true;
            doDownloadStep();
            break;
        }

        const auto& device = m_updatingIterator.getDevice();
        const auto& target = m_updatingIterator.getTarget();
        const auto& path = linkIt->second.path;

        if( m_targetsResults.empty() ) {
            m_delegate.onFirmwareUpdateStarted( device );
        }

        if( downloadStatus == DownloadingStatus::Failed ) {
            onUpdateFinished( m_updatingIterator, TargetUpdateResult::DownloadFailed );
        }
        else {
            const bool started =
                m_delegate.startFirmwareUpdate( device, target, path, [this, device, target]( bool result ) {
                    onFirmwareUpdateFinished( device, target, result );
                } );

            if( started ) {
                onUpdateStarted();
            }
            else {
                onUpdateFinished( m_updatingIterator, TargetUpdateResult::UpdateFailed );
            }
        }

        ++m_updatingIterator;
    }
}

void DeviceFirmwareUpdateManager::cleanUpScopes() {
    m_storedImagesCount = 0;
    m_downloadsCount = 0;
    while( m_scopes.begin() != m_updatingIterator.getScopesIterator() ) {
        for( auto& targetToLink : m_scopes.begin()->targets ) {
            auto linkIt = m_linksPool.find( targetToLink.second.link );

            if( linkIt->second.usageCount == 0 ) {
                m_linksPool.erase( linkIt );
            }
        }

        m_scopes.erase( m_scopes.begin() );
    }
}

DeviceFirmwareUpdateManager::DownloadingStatus DeviceFirmwareUpdateManager::getDownloadingStatus(
    const ExtendedLink& link ) const {
    auto it = m_linksPool.find( link );
    return it->second.downloadingStatus;
}

void DeviceFirmwareUpdateManager::onDownloadStarted( const ExtendedLink& link, DownloaderConstUPtr downloader ) {
    auto it = m_linksPool.find( link );
    if( downloader ) {
        it->second.downloader = std::move( downloader );
        it->second.downloadingStatus = DownloadingStatus::Started;
        ++m_downloadsCount;
        m_downloadRequired = false;
    }
    else {
        it->second.downloadingStatus = DownloadingStatus::Failed;
        it->second.usageCount = 0;
    }
}

void DeviceFirmwareUpdateManager::onDownloadFinished( const ExtendedLink& link, Path path ) {
    auto it = m_linksPool.find( link );
    it->second.downloader = nullptr;
    it->second.path = std::move( path );
    --m_downloadsCount;
    m_downloadRequired = false;

    if( !it->second.path.empty() ) {
        ++m_storedImagesCount;
        it->second.downloadingStatus = DownloadingStatus::Done;
        doUpdateStep();
    }
    else {
        it->second.downloadingStatus = DownloadingStatus::Failed;
        it->second.usageCount = 0;
    }

    doDownloadStep();
}

void DeviceFirmwareUpdateManager::onUpdateStarted() {
    m_activeUpdatingIterator = m_updatingIterator;
}

void DeviceFirmwareUpdateManager::onUpdateFinished( UpdatingIterator updatingIterator, TargetUpdateResult result ) {
    const auto& link = updatingIterator.getLink();
    auto linkIt = m_linksPool.find( link );

    if( result != TargetUpdateResult::DownloadFailed ) {
        if( --linkIt->second.usageCount == 0 ) {
            remove( linkIt->second.path.c_str() );
            --m_storedImagesCount;
            linkIt->second.path.clear();
        }
    }

    m_targetsResults.emplace( updatingIterator.getTarget(), result );

    m_activeUpdatingIterator = UpdatingIterator{ *this, m_scopes.end() };

    if( m_targetsResults.size() == updatingIterator.getScopesIterator()->targets.size() ) {
        auto targetResults = std::move( m_targetsResults );
        m_delegate.onFirmwareUpdateFinished( updatingIterator.getDevice(), targetResults );
    }

    cleanUpScopes();
}

}  // namespace common
