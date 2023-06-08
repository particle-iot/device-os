/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <mutex>
#include <cstdio>
#include <cstring>
#include <cstdarg>

#include "system_ledger_internal.h"

#include "control/common.h" // FIXME: Move Protobuf utilities to another directory

#include "file_util.h"
#include "endian_util.h"
#include "scope_guard.h"
#include "check.h"

#include "common/ledger.pb.h" // Common definitions
#include "cloud/ledger.pb.h" // Cloud protocol definitions
#include "ledger.pb.h" // Internal definitions

#define PB_LEDGER(_name) particle_ledger_##_name
#define PB_CLOUD(_name) particle_cloud_##_name
#define PB_INTERNAL(_name) particle_firmware_##_name

static_assert(LEDGER_SCOPE_INVALID == (int)PB_LEDGER(LedgerScope_LEDGER_SCOPE_INVALID) &&
        LEDGER_SCOPE_DEVICE == (int)PB_LEDGER(LedgerScope_LEDGER_SCOPE_DEVICE) &&
        LEDGER_SCOPE_PRODUCT == (int)PB_LEDGER(LedgerScope_LEDGER_SCOPE_PRODUCT) &&
        LEDGER_SCOPE_OWNER == (int)PB_LEDGER(LedgerScope_LEDGER_SCOPE_OWNER));

namespace particle {

using control::common::EncodedString;
using control::common::DecodedCString;
using fs::FsLock;

namespace system {

namespace {

#define DATA_FORMAT_VERSION 1

/*
    The ledger directory is organized as follows:

    /sys/ledger/
    |
    +--- device/ - Device ledger
    |    +--- default/ - Files of the default device ledger (reserved name)
    |         +--- ledger.data - Ledger info
    |         +--- pages/ - Page files
    |              +--- sensors.data - Contents of the page called "sensors"
    |              +--- ...
    |
    +--- product/ - Product ledgers
    |    +--- config/ - Files of the ledger called "config"
    |    |    +--- ledger.data
    |    |    +--- pages/
    |    |         +--- ...
    |    +--- ...
    |
    +--- owner/ - User or organization ledgers
    |    +--- company_info/ - Files of the ledger called "company_info"
    |    |    +--- ledger.data
    |    |    +--- pages/
    |    |         +--- ...
    |    +--- ...
    |
    +--- temp/ - Temporary files
         +--- ...
*/
const auto LEDGER_ROOT_DIR = "/sys/ledger";
const auto DEVICE_SCOPE_DIR_NAME = "device";
const auto PRODUCT_SCOPE_DIR_NAME = "product";
const auto OWNER_SCOPE_DIR_NAME = "owner";
const auto TEMP_DIR_NAME = "temp";
const auto PAGES_DIR_NAME = "pages";
const auto LEDGER_DATA_FILE_NAME = "ledger.data";
const auto PAGE_DATA_FILE_EXT = "data";
const auto DEFAULT_LEDGER_NAME = "default";

const size_t MAX_PATH_LEN = 255;

int g_tempPageFileCount = 0;

// Internal result codes
enum Result {
    RESULT_LEDGER_FILE_NOT_FOUND = 1,
    RESULT_PAGE_FILE_NOT_FOUND = 2
};

/*
    The layout of a ledger data file:

    Field     | Size      | Description
    ----------+-----------+------------
    version   | 4         | Format version number (unsigned integer)
    info_size | 4         | Size of the "info" field (unsigned integer)
    info      | info_size | Protobuf-encoded ledger info (particle.firmware.LedgerInfo)

    All integer fields are encoded in little-endian byte order.
*/
struct LedgerDataHeader {
    uint32_t version;
    uint32_t infoSize;
} __attribute__((packed));

/*
    The layout of a page data file:

    Field     | Size      | Description
    ----------+-----------+------------
    version   | 4         | Format version number (unsigned integer)
    info_size | 4         | Size of the "info" field (unsigned integer)
    data_size | 4         | Size of the "data" field (unsigned integer)
    info      | info_size | Protobuf-encoded page info (particle.firmware.LedgerPageInfo)
    data      | data_size | MessagePack-encoded page contents

    All integer fields are encoded in little-endian byte order.
*/
struct PageDataHeader {
    uint32_t version;
    uint32_t infoSize;
    uint32_t dataSize;
} __attribute__((packed));

int formatLedgerPath(char* buf, size_t size, const char* ledgerName, ledger_scope scope, const char* fmt = "", ...) {
    // Format the prefix part of the path
    const char* scopeName = nullptr;
    switch (scope) {
    case LEDGER_SCOPE_DEVICE: scopeName = DEVICE_SCOPE_DIR_NAME; break;
    case LEDGER_SCOPE_PRODUCT: scopeName = PRODUCT_SCOPE_DIR_NAME; break;
    case LEDGER_SCOPE_OWNER: scopeName = OWNER_SCOPE_DIR_NAME; break;
    default:
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    int n = snprintf(buf, size, "%s/%s/%s/", LEDGER_ROOT_DIR, scopeName, ledgerName);
    if (n < 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    size_t pos = n;
    if (pos >= size) {
        return SYSTEM_ERROR_FILENAME_TOO_LONG;
    }
    // Format the rest of the path
    va_list args;
    va_start(args, fmt);
    n = vsnprintf(buf + pos, size - pos, fmt, args);
    va_end(args);
    if (n < 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    pos += n;
    if (pos >= size) {
        return SYSTEM_ERROR_FILENAME_TOO_LONG;
    }
    return pos;
}

int formatTempPath(char* buf, size_t size, const char* fmt = "", ...) {
    // Format the prefix part of the path
    int n = snprintf(buf, size, "%s/%s/", LEDGER_ROOT_DIR, TEMP_DIR_NAME);
    if (n < 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    size_t pos = n;
    if (pos >= size) {
        return SYSTEM_ERROR_FILENAME_TOO_LONG;
    }
    // Format the rest of the path
    va_list args;
    va_start(args, fmt);
    n = vsnprintf(buf + pos, size - pos, fmt, args);
    va_end(args);
    if (n < 0) {
        return SYSTEM_ERROR_INTERNAL;
    }
    pos += n;
    if (pos >= size) {
        return SYSTEM_ERROR_FILENAME_TOO_LONG;
    }
    return pos;
}

int getPageFilePath(char* buf, size_t size, const char* pageName, const char* ledgerName, ledger_scope scope) {
    CHECK(formatLedgerPath(buf, size, ledgerName, scope, "%s/%s.%s", PAGES_DIR_NAME, pageName, PAGE_DATA_FILE_EXT));
    return 0;
}

int getTempPageFilePath(char* buf, size_t size) {
    CHECK(formatTempPath(buf, size, "page_%04d.%s", ++g_tempPageFileCount, PAGE_DATA_FILE_EXT));
    return 0;
}

int makeBaseDir(char* pathBuf) {
    char* p = strrchr(pathBuf, '/');
    if (p) {
        *p = '\0';
    }
    CHECK(mkdirp(pathBuf));
    if (p) {
        *p = '/';
    }
    return 0;
}

int copyStringVector(const Vector<CString>& src, Vector<CString>& dest) {
    dest.clear();
    if (!dest.reserve(src.size())) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    for (int i = 0; i < src.size(); ++i) {
        CString s = src.at(i);
        if (!s && src.at(i)) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        dest.append(std::move(s));
    }
    return 0;
}

} // namespace

int LedgerManager::init() {
    // Delete temporary files
    char path[MAX_PATH_LEN + 1] = {};
    CHECK(formatTempPath(path, sizeof(path)));
    CHECK(rmrf(path));
    inited_ = true;
    return 0;
}

int LedgerManager::getLedger(const char* name, ledger_scope scope, int apiVersion, RefCountPtr<Ledger>& ledger) {
    std::lock_guard lock(mutex_);
    if (!inited_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!name) {
        name = DEFAULT_LEDGER_NAME;
    }
    // Check if the requested ledger is already instantiated
    for (Ledger* lr: ledgerInstances_) { // TODO: Use binary search
        if (strcmp(lr->name(), name) == 0) {
            ledger = lr;
            return 0;
        }
    }
    // Create a new instance
    auto lr = makeRefCountPtr<Ledger>();
    if (!lr) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    int r = lr->init(name, scope, apiVersion);
    if (r < 0) {
        LOG(ERROR, "Failed to initialize ledger: %d", r);
        return r;
    }
    if (!ledgerInstances_.append(lr.get())) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    ledger = std::move(lr);
    return 0;
}

void LedgerManager::addLedgerRef(Ledger* ledger) {
    std::lock_guard lock(mutex_);
    ++ledger->refCount_;
}

void LedgerManager::releaseLedger(Ledger* ledger) {
    std::lock_guard lock(mutex_);
    if (--ledger->refCount_ == 0) {
        ledgerInstances_.removeOne(ledger);
        delete ledger;
    }
}

LedgerManager* LedgerManager::instance() {
    static LedgerManager mgr;
    return &mgr;
}

Ledger::~Ledger() {
    // Destroy the application data
    if (appData_ && destroyAppData_) {
        destroyAppData_(appData_);
    }
}

int Ledger::init(const char* name, ledger_scope scope, int apiVersion) {
    // TODO: Validate the name
    if (!*name) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (scope == LEDGER_SCOPE_DEVICE && strcmp(name, DEFAULT_LEDGER_NAME) != 0) {
        // As of now, only one device ledger is supported per device
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (scope == LEDGER_SCOPE_INVALID) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (apiVersion < 1 || apiVersion > LEDGER_API_VERSION) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    name_ = name;
    if (!name_) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    scope_ = scope;
    apiVersion_ = apiVersion;
    int r = CHECK(loadLedgerInfo());
    if (r == RESULT_LEDGER_FILE_NOT_FOUND && scope_ == LEDGER_SCOPE_DEVICE) {
        // Initialize the device ledger's directory
        CHECK(saveLedgerInfo());
    }
    return 0;
}

void Ledger::setCallbacks(ledger_page_sync_callback pageSync, ledger_remote_page_change_callback pageChange) {
    std::lock_guard lock(*this);
    pageSyncCallback_ = pageSync;
    pageChangeCallback_ = pageChange;
}

void Ledger::setAppData(void* data, ledger_destroy_app_data_callback destroy) {
    std::lock_guard lock(*this);
    if (appData_ && destroyAppData_) {
        destroyAppData_(appData_);
    }
    appData_ = data;
    destroyAppData_ = destroy;
}

void* Ledger::appData() const {
    std::lock_guard lock(*this);
    return appData_;
}

int Ledger::getPage(const char* name, RefCountPtr<LedgerPage>& page) {
    std::lock_guard lock(*this);
    // TODO: Validate the name
    if (!*name) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // Check if the requested page is already instantiated
    for (LedgerPage* p: pageInstances_) { // TODO: Use binary search
        if (strcmp(p->name(), name) == 0) {
            page = p;
            return 0;
        }
    }
    // Create a new instance
    auto p = makeRefCountPtr<LedgerPage>();
    if (!p) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    int r = p->init(name, this);
    if (r < 0) {
        LOG(ERROR, "Failed to initialize ledger page: %d", r);
        return r;
    }
    if (!pageInstances_.append(p.get())) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    page = std::move(p);
    return 0;
}

void Ledger::addPageRef(LedgerPage* page) {
    std::lock_guard lock(*this);
    ++page->refCount_;
}

void Ledger::releasePage(LedgerPage* page) {
    std::lock_guard lock(*this);
    if (--page->refCount_ == 0) {
        pageInstances_.removeOne(page);
        delete page;
    }
}

int Ledger::getLinkedPageNames(Vector<CString>& names) const {
    std::lock_guard lock(*this);
    CHECK(copyStringVector(linkedPageNames_, names));
    return 0;
}

int Ledger::loadLedgerInfo() {
    FsLock fs;
    // Open the ledger file
    char path[MAX_PATH_LEN + 1] = {};
    CHECK(formatLedgerPath(path, sizeof(path), name_, scope_, "%s", LEDGER_DATA_FILE_NAME));
    lfs_file_t file = {};
    int r = lfs_file_open(fs.instance(), &file, path, LFS_O_RDONLY);
    if (r < 0) {
        if (r == LFS_ERR_NOENT) {
            return RESULT_LEDGER_FILE_NOT_FOUND;
        }
        CHECK_FS(r); // Forward the error
    }
    SCOPE_GUARD({
        int r = lfs_file_close(fs.instance(), &file);
        if (r < 0) {
            LOG(ERROR, "Error while closing file: %d", r);
        }
    });
    // Read the header
    LedgerDataHeader h = {};
    size_t n = CHECK_FS(lfs_file_read(fs.instance(), &file, &h, sizeof(h)));
    if (n != sizeof(h)) {
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    h.version = littleEndianToNative(h.version);
    h.infoSize = littleEndianToNative(h.infoSize);
    if (h.version != DATA_FORMAT_VERSION) {
        LOG(ERROR, "Unsupported version of ledger data format: %u", (unsigned)h.version);
        return SYSTEM_ERROR_LEDGER_UNSUPPORTED_FORMAT;
    }
    size_t fileSize = CHECK_FS(lfs_file_size(fs.instance(), &file));
    if (fileSize != sizeof(h) + h.infoSize) {
        LOG(ERROR, "Unexpected size of ledger data file");
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    // Read the ledger info
    PB_INTERNAL(LedgerInfo) pbInfo = {};
    DecodedCString pbName(&pbInfo.name);
    CHECK(decodeProtobufFromFile(&file, &PB_INTERNAL(LedgerInfo_msg), &pbInfo, h.infoSize));
    if (strcmp(name_, pbName.data) != 0) {
        LOG(ERROR, "Unexpected ledger name");
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    if (static_cast<ledger_scope>(pbInfo.scope) != scope_) {
        LOG(ERROR, "Unexpected ledger scope");
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    return 0;
}

int Ledger::saveLedgerInfo() {
    FsLock fs;
    // Create the ledger file
    char path[MAX_PATH_LEN + 1] = {};
    CHECK(formatLedgerPath(path, sizeof(path), name_, scope_, "%s", LEDGER_DATA_FILE_NAME));
    CHECK(makeBaseDir(path)); // Ensure the ledger directory exists
    lfs_file_t file = {};
    CHECK_FS(lfs_file_open(fs.instance(), &file, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC));
    SCOPE_GUARD({
        int r = lfs_file_close(fs.instance(), &file);
        if (r < 0) {
            LOG(ERROR, "Error while closing file: %d", r);
        }
    });
    // Write the ledger info
    CHECK_FS(lfs_file_seek(fs.instance(), &file, sizeof(LedgerDataHeader), LFS_SEEK_SET));
    PB_INTERNAL(LedgerInfo) pbInfo = {};
    EncodedString pbName(&pbInfo.name, name_, strlen(name_));
    pbInfo.scope = static_cast<PB_LEDGER(LedgerScope)>(scope_);
    size_t infoSize = CHECK(encodeProtobufToFile(&file, &PB_INTERNAL(LedgerInfo_msg), &pbInfo));
    // Write the header
    CHECK_FS(lfs_file_seek(fs.instance(), &file, 0, LFS_SEEK_SET));
    LedgerDataHeader h = {};
    h.version = nativeToLittleEndian(DATA_FORMAT_VERSION);
    h.infoSize = nativeToLittleEndian(infoSize);
    CHECK_FS(lfs_file_write(fs.instance(), &file, &h, sizeof(h)));
    return 0;
}

LedgerPage::~LedgerPage() {
    // Destroy the application data
    if (appData_ && destroyAppData_) {
        destroyAppData_(appData_);
    }
}

int LedgerPage::init(const char* name, Ledger* ledger) {
    name_ = name;
    if (!name_) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    ledger_ = ledger;
    CHECK(loadPageInfo());
    return 0;
}

void LedgerPage::setLocalChangeCallback(ledger_local_page_change_callback callback) {
    std::lock_guard lock(*this);
    changeCallback_ = callback;
}

void LedgerPage::setAppData(void* data, ledger_destroy_app_data_callback destroy) {
    std::lock_guard lock(*this);
    if (appData_ && destroyAppData_) {
        destroyAppData_(appData_);
    }
    appData_ = data;
    destroyAppData_ = destroy;
}

void* LedgerPage::appData() const {
    std::lock_guard lock(*this);
    return appData_;
}

int LedgerPage::openInputStream(std::unique_ptr<LedgerStream>& stream) {
    std::lock_guard lock(*this);
    // The page file doesn't have to exist when opening a page for reading
    char path[MAX_PATH_LEN + 1] = {};
    CHECK(getPageFilePath(path, sizeof(path), name_, ledger_->name(), ledger_->scope()));
    std::unique_ptr<LedgerPageInputStream> s(new(std::nothrow) LedgerPageInputStream());
    if (!s) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK(s->init(path, this));
    stream = std::move(s);
    ++readerCount_;
    return 0;
}

int LedgerPage::openOutputStream(LedgerChangeSource src, std::unique_ptr<LedgerStream>& stream) {
    std::lock_guard lock(*this);
    if (src == LedgerChangeSource::USER && !ledger_->isUserWritable()) {
        return SYSTEM_ERROR_LEDGER_READ_ONLY;
    }
    // TODO: Instead of using an intermediate temporary file, we can update the actual page file
    // in place and rely on LittleFS' copy-on-write properties to ensure the atomicity of the update
    char path[MAX_PATH_LEN + 1] = {};
    CHECK(getTempPageFilePath(path, sizeof(path)));
    CHECK(makeBaseDir(path)); // Ensure the temporary directory exists
    std::unique_ptr<LedgerPageOutputStream> s(new(std::nothrow) LedgerPageOutputStream());
    if (!s) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK(s->init(src, path, this));
    stream = std::move(s);
    return 0;
}

int LedgerPage::inputStreamClosed(LedgerPageInputStream* stream) {
    std::lock_guard lock(*this);
    if (--readerCount_ == 0 && updatedPageFile_) {
        // Update the page file
        CString tempFile(std::move(updatedPageFile_));
        FsLock fs;
        CHECK(updatePageFile(fs.instance(), tempFile, changeSrc_));
    }
    return 0;
}

int LedgerPage::outputStreamClosed(LedgerPageOutputStream* stream, bool flush) {
    std::lock_guard lock(*this);
    FsLock fs;
    if (!flush) {
        // Remove the temporary page file
        CHECK_FS(lfs_remove(fs.instance(), stream->fileName()));
    } else if (readerCount_ == 0) {
        // Update the actual page file
        CHECK(updatePageFile(fs.instance(), stream->fileName(), stream->changeSource()));
    } else {
        // Remove the old temporary file if it exists and keep the new one around until all the
        // streams open for reading are closed
        CString newTempFile = stream->fileName();
        if (!newTempFile) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        if (updatedPageFile_) {
            CHECK_FS(lfs_remove(fs.instance(), updatedPageFile_));
        }
        updatedPageFile_ = std::move(newTempFile);
        changeSrc_ = stream->changeSource();
    }
    return 0;
}

int LedgerPage::updatePageFile(lfs_t* lfs, const char* srcFile, LedgerChangeSource changeSrc) {
    char destPath[MAX_PATH_LEN + 1] = {};
    CHECK(getPageFilePath(destPath, sizeof(destPath), name_, ledger_->name(), ledger_->scope()));
    CHECK(makeBaseDir(destPath)); // Ensure the page directory exists
    CHECK_FS(lfs_rename(lfs, srcFile, destPath));
    if (changeSrc == LedgerChangeSource::SYSTEM && changeCallback_) {
        changeCallback_(reinterpret_cast<ledger_page*>(this), 0 /* flags */, appData_);
    }
    return 0;
}

int LedgerPage::loadPageInfo() {
    FsLock fs;
    // Open the page file
    char path[MAX_PATH_LEN + 1] = {};
    CHECK(formatLedgerPath(path, sizeof(path), ledger_->name(), ledger_->scope(), "pages/%s.%s", name_, PAGE_DATA_FILE_EXT));
    lfs_file_t file = {};
    int r = lfs_file_open(fs.instance(), &file, path, LFS_O_RDONLY);
    if (r < 0) {
        if (r == LFS_ERR_NOENT) {
            return RESULT_PAGE_FILE_NOT_FOUND;
        }
        CHECK_FS(r); // Forward the error
    }
    SCOPE_GUARD({
        int r = lfs_file_close(fs.instance(), &file);
        if (r < 0) {
            LOG(ERROR, "Error while closing file: %d", r);
        }
    });
    // Read the header
    PageDataHeader h = {};
    size_t n = CHECK_FS(lfs_file_read(fs.instance(), &file, &h, sizeof(h)));
    if (n != sizeof(h)) {
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    h.version = littleEndianToNative(h.version);
    h.infoSize = littleEndianToNative(h.infoSize);
    h.dataSize = littleEndianToNative(h.dataSize);
    if (h.version != DATA_FORMAT_VERSION) {
        LOG(ERROR, "Unsupported version of ledger data format: %u", (unsigned)h.version);
        return SYSTEM_ERROR_LEDGER_UNSUPPORTED_FORMAT;
    }
    size_t fileSize = CHECK_FS(lfs_file_size(fs.instance(), &file));
    if (fileSize != sizeof(h) + h.infoSize + h.dataSize) {
        LOG(ERROR, "Unexpected size of ledger page file");
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    // Read the page info
    PB_INTERNAL(LedgerPageInfo) pbInfo = {};
    DecodedCString pbName(&pbInfo.name);
    CHECK(decodeProtobufFromFile(&file, &PB_INTERNAL(LedgerPageInfo_msg), &pbInfo, h.infoSize));
    if (strcmp(name_, pbName.data) != 0) {
        LOG(ERROR, "Unexpected ledger page name");
        return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
    }
    return 0;
}

LedgerPageInputStream::~LedgerPageInputStream() {
    int r = close(0);
    if (r < 0) {
        LOG(ERROR, "Failed to close ledger stream: %d", r);
    }
}

int LedgerPageInputStream::init(const char* pageFile, LedgerPage* page) {
    page_ = page;
    // Open the page file
    FsLock fs;
    int r = lfs_file_open(fs.instance(), &file_, pageFile, LFS_O_RDONLY);
    if (r >= 0) {
        fileOpen_ = true;
    } else if (r != LFS_ERR_NOENT) {
        CHECK_FS(r); // Forward the error
    }
    NAMED_SCOPE_GUARD(g, {
        if (fileOpen_) {
            int r = lfs_file_close(fs.instance(), &file_);
            if (r < 0) {
                LOG(ERROR, "Error while closing file: %d", r);
            }
        }
    });
    if (fileOpen_) {
        // Skip the header and page info. Both have already been validated at this point
        PageDataHeader h = {};
        size_t n = CHECK_FS(lfs_file_read(fs.instance(), &file_, &h, sizeof(h)));
        if (n != sizeof(h)) {
            return SYSTEM_ERROR_LEDGER_INVALID_FORMAT;
        }
        h.infoSize = littleEndianToNative(h.infoSize);
        CHECK_FS(lfs_file_seek(fs.instance(), &file_, h.infoSize, LFS_SEEK_CUR));
        dataOffs_ = CHECK_FS(lfs_file_tell(fs.instance(), &file_));
    }
    g.dismiss();
    streamOpen_ = true;
    return 0;
}

int LedgerPageInputStream::read(char* data, size_t size) {
    if (!streamOpen_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    size_t n = 0;
    if (fileOpen_) {
        FsLock fs;
        n = CHECK_FS(lfs_file_read(fs.instance(), &file_, data, size));
    }
    if (n == 0 && size > 0) {
        return SYSTEM_ERROR_END_OF_STREAM;
    }
    return n;
}

int LedgerPageInputStream::close(bool flush) {
    if (!streamOpen_) {
        return 0;
    }
    if (fileOpen_) {
        FsLock fs;
        CHECK_FS(lfs_file_close(fs.instance(), &file_));
        fileOpen_ = false;
    }
    streamOpen_ = false;
    int r = page_->inputStreamClosed(this);
    if (r < 0) {
        LOG(ERROR, "Failed to flush ledger stream: %d", r);
    }
    return 0;
}

int LedgerPageInputStream::rewind() {
    if (!streamOpen_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (fileOpen_) {
        FsLock fs;
        CHECK_FS(lfs_file_seek(fs.instance(), &file_, dataOffs_, LFS_SEEK_SET));
    }
    return 0;
}

LedgerPageOutputStream::~LedgerPageOutputStream() {
    int r = close(0);
    if (r < 0) {
        LOG(ERROR, "Failed to close ledger stream: %d", r);
    }
}

int LedgerPageOutputStream::init(LedgerChangeSource src, const char* pageFile, LedgerPage* page) {
    fileName_ = pageFile;
    if (!fileName_) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    changeSrc_ = src;
    page_ = page;
    // Open the temporary page file
    FsLock fs;
    CHECK_FS(lfs_file_open(fs.instance(), &file_, fileName_, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_EXCL));
    NAMED_SCOPE_GUARD(g, {
        int r = lfs_file_close(fs.instance(), &file_);
        if (r < 0) {
            LOG(ERROR, "Error while closing file: %d", r);
        }
    });
    // Write the page info
    CHECK_FS(lfs_file_seek(fs.instance(), &file_, sizeof(PageDataHeader), LFS_SEEK_SET));
    PB_INTERNAL(LedgerPageInfo) pbInfo = {};
    EncodedString pbName(&pbInfo.name, page->name(), strlen(page->name()));
    pageInfoSize_ = CHECK(encodeProtobufToFile(&file_, &PB_INTERNAL(LedgerPageInfo_msg), &pbInfo));
    dataOffs_ = CHECK_FS(lfs_file_tell(fs.instance(), &file_));
    g.dismiss();
    open_ = true;
    return 0;
}

int LedgerPageOutputStream::write(const char* data, size_t size) {
    if (!open_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    FsLock fs;
    size_t n = CHECK_FS(lfs_file_write(fs.instance(), &file_, data, size));
    return n;
}

int LedgerPageOutputStream::close(bool flush) {
    if (!open_) {
        return 0;
    }
    FsLock fs;
    if (flush) {
        // Write the header
        size_t fileSize = CHECK_FS(lfs_file_tell(fs.instance(), &file_));
        CHECK_FS(lfs_file_seek(fs.instance(), &file_, 0, LFS_SEEK_SET));
        PageDataHeader h = {};
        h.version = nativeToLittleEndian(DATA_FORMAT_VERSION);
        h.infoSize = nativeToLittleEndian(pageInfoSize_);
        h.dataSize = nativeToLittleEndian(fileSize - sizeof(PageDataHeader) - pageInfoSize_);
        CHECK_FS(lfs_file_write(fs.instance(), &file_, &h, sizeof(h)));
    }
    CHECK_FS(lfs_file_close(fs.instance(), &file_));
    open_ = false;
    int r = page_->outputStreamClosed(this, flush);
    if (r < 0) {
        LOG(ERROR, "Failed to flush ledger stream: %d", r);
    }
    return 0;
}

int LedgerPageOutputStream::rewind() {
    if (!open_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    FsLock fs;
    CHECK_FS(lfs_file_seek(fs.instance(), &file_, dataOffs_, LFS_SEEK_SET));
    CHECK_FS(lfs_file_truncate(fs.instance(), &file_, dataOffs_));
    return 0;
}

} // namespace system

} // namespace particle
