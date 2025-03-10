/*
 *  Copyright (c) 2021 NetEase Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"){}
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*
 * Project: curve
 * Created Date: Thur Oct Jun 28 2021
 * Author: lixiaocui
 */


#ifndef CURVEFS_SRC_CLIENT_METRIC_CLIENT_METRIC_H_
#define CURVEFS_SRC_CLIENT_METRIC_CLIENT_METRIC_H_

#include <bvar/bvar.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include "src/client/client_metric.h"
#include "src/common/s3_adapter.h"

using curve::client::InterfaceMetric;

namespace curvefs {
namespace client {
namespace metric {

struct MDSClientMetric {
    static const std::string prefix;

    InterfaceMetric mountFs;
    InterfaceMetric umountFs;
    InterfaceMetric getFsInfo;
    InterfaceMetric getMetaServerInfo;
    InterfaceMetric getMetaServerListInCopysets;
    InterfaceMetric createPartition;
    InterfaceMetric getCopysetOfPartitions;
    InterfaceMetric listPartition;
    InterfaceMetric allocS3ChunkId;
    InterfaceMetric refreshSession;
    InterfaceMetric getLatestTxId;
    InterfaceMetric commitTx;
    InterfaceMetric allocOrGetMemcacheCluster;

    MDSClientMetric()
        : mountFs(prefix, "mountFs"),
          umountFs(prefix, "umountFs"),
          getFsInfo(prefix, "getFsInfo"),
          getMetaServerInfo(prefix, "getMetaServerInfo"),
          getMetaServerListInCopysets(prefix, "getMetaServerListInCopysets"),
          createPartition(prefix, "createPartition"),
          getCopysetOfPartitions(prefix, "getCopysetOfPartitions"),
          listPartition(prefix, "listPartition"),
          allocS3ChunkId(prefix, "allocS3ChunkId"),
          refreshSession(prefix, "refreshSession"),
          getLatestTxId(prefix, "getLatestTxId"),
          commitTx(prefix, "commitTx"),
          allocOrGetMemcacheCluster(prefix, "allocOrGetMemcacheCluster") {}
};

struct MetaServerClientMetric {
    static const std::string prefix;

    // dentry
    InterfaceMetric getDentry;
    InterfaceMetric listDentry;
    InterfaceMetric createDentry;
    InterfaceMetric deleteDentry;

    // inode
    InterfaceMetric getInode;
    InterfaceMetric batchGetInodeAttr;
    InterfaceMetric batchGetXattr;
    InterfaceMetric createInode;
    InterfaceMetric updateInode;
    InterfaceMetric deleteInode;
    InterfaceMetric appendS3ChunkInfo;

    // tnx
    InterfaceMetric prepareRenameTx;

    // volume extent
    InterfaceMetric updateVolumeExtent;
    InterfaceMetric getVolumeExtent;
    InterfaceMetric updateDeallocatableBlockGroup;

    MetaServerClientMetric()
        : getDentry(prefix, "getDentry"), listDentry(prefix, "listDentry"),
          createDentry(prefix, "createDentry"),
          deleteDentry(prefix, "deleteDentry"), getInode(prefix, "getInode"),
          batchGetInodeAttr(prefix, "batchGetInodeAttr"),
          batchGetXattr(prefix, "batchGetXattr"),
          createInode(prefix, "createInode"),
          updateInode(prefix, "updateInode"),
          deleteInode(prefix, "deleteInode"),
          appendS3ChunkInfo(prefix, "appendS3ChunkInfo"),
          prepareRenameTx(prefix, "prepareRenameTx"),
          updateVolumeExtent(prefix, "updateVolumeExtent"),
          getVolumeExtent(prefix, "getVolumeExtent"),
          updateDeallocatableBlockGroup(prefix,
                                        "updateDeallocatableBlockGroup") {}
};

struct InflightGuard {
    explicit InflightGuard(bvar::Adder<int64_t>* inflight)
    : inflight_(inflight) {
        (*inflight_) << 1;
    }

    ~InflightGuard() {
        (*inflight_) << -1;
    }

    bvar::Adder<int64_t>* inflight_;
};

struct OpMetric {
    bvar::LatencyRecorder latency;
    bvar::Adder<int64_t> inflightOpNum;
    bvar::Adder<uint64_t> ecount;

    explicit OpMetric(const std::string& prefix, const std::string& name)
        : latency(prefix, name + "_lat"),
          inflightOpNum(prefix, name + "_inflight_num"),
          ecount(prefix, name + "_error_num") {}
};

struct ClientOpMetric {
    static const std::string prefix;

    OpMetric opLookup;
    OpMetric opOpen;
    OpMetric opCreate;
    OpMetric opMkNod;
    OpMetric opMkDir;
    OpMetric opLink;
    OpMetric opUnlink;
    OpMetric opRmDir;
    OpMetric opOpenDir;
    OpMetric opReleaseDir;
    OpMetric opReadDir;
    OpMetric opRename;
    OpMetric opGetAttr;
    OpMetric opSetAttr;
    OpMetric opGetXattr;
    OpMetric opListXattr;
    OpMetric opSymlink;
    OpMetric opReadLink;
    OpMetric opRelease;
    OpMetric opFsync;
    OpMetric opFlush;
    OpMetric opRead;
    OpMetric opWrite;


    ClientOpMetric()
        : opLookup(prefix, "opLookup"),
          opOpen(prefix, "opOpen"),
          opCreate(prefix, "opCreate"),
          opMkNod(prefix, "opMkNod"),
          opMkDir(prefix, "opMkDir"),
          opLink(prefix, "opLink"),
          opUnlink(prefix, "opUnlink"),
          opRmDir(prefix, "opRmDir"),
          opOpenDir(prefix, "opOpenDir"),
          opReleaseDir(prefix, "opReleaseDir"),
          opReadDir(prefix, "opReadDir"),
          opRename(prefix, "opRename"),
          opGetAttr(prefix, "opGetAttr"),
          opSetAttr(prefix, "opSetAttr"),
          opGetXattr(prefix, "opGetXattr"),
          opListXattr(prefix, "opListXattr"),
          opSymlink(prefix, "opSymlink"),
          opReadLink(prefix, "opReadLink"),
          opRelease(prefix, "opRelease"),
          opFsync(prefix, "opFsync"),
          opFlush(prefix, "opFlush"),
          opRead(prefix, "opRead"),
          opWrite(prefix, "opWrite") {}
};

struct S3MultiManagerMetric {
    static const std::string prefix;

    bvar::Adder<int64_t> fileManagerNum;
    bvar::Adder<int64_t> chunkManagerNum;
    bvar::Adder<int64_t> writeDataCacheNum;
    bvar::Adder<int64_t> writeDataCacheByte;
    bvar::Adder<int64_t> readDataCacheNum;
    bvar::Adder<int64_t> readDataCacheByte;

    S3MultiManagerMetric() {
        fileManagerNum.expose_as(prefix, "file_manager_num");
        chunkManagerNum.expose_as(prefix, "chunk_manager_num");
        writeDataCacheNum.expose_as(prefix, "write_data_cache_num");
        writeDataCacheByte.expose_as(prefix, "write_data_cache_byte");
        readDataCacheNum.expose_as(prefix, "read_data_cache_num");
        readDataCacheByte.expose_as(prefix, "read_data_cache_byte");
    }
};

struct FSMetric {
    static const std::string prefix;

    std::string fsName;

    InterfaceMetric userWrite;
    InterfaceMetric userRead;
    bvar::Status<uint32_t> userWriteIoSize;
    bvar::Status<uint32_t> userReadIoSize;

    explicit FSMetric(const std::string &name = "")
        : fsName(!name.empty() ? name
                               : prefix + curve::common::ToHexString(this)),
          userWrite(prefix, fsName + "_userWrite"),
          userRead(prefix, fsName + "_userRead"),
          userWriteIoSize(prefix, fsName + "_userWriteIoSize", 0),
          userReadIoSize(prefix, fsName + "_userReadIoSize", 0) {}
};

struct S3Metric {
    static const std::string prefix;

    std::string fsName;
    InterfaceMetric adaptorWrite;
    InterfaceMetric adaptorRead;
    InterfaceMetric adaptorWriteS3;
    InterfaceMetric adaptorWriteDiskCache;
    InterfaceMetric adaptorReadS3;
    InterfaceMetric adaptorReadDiskCache;
    // Write to the backend s3
    InterfaceMetric writeToS3;
    // Read from backend s3 (excluding warmup)
    InterfaceMetric readFromS3;
    // write to the disk cache (excluding warmup)
    InterfaceMetric writeToDiskCache;
    // read from the disk cache (excluding warmup)
    InterfaceMetric readFromDiskCache;
    // write to kv cache (excluding warmup)
    InterfaceMetric writeToKVCache;
    // read from kv cache (excluding warmup)
    InterfaceMetric readFromKVCache;
    bvar::Status<uint32_t> readSize;
    bvar::Status<uint32_t> writeSize;

    explicit S3Metric(const std::string& name = "")
        : fsName(!name.empty() ? name
                               : prefix + curve::common::ToHexString(this)),
          adaptorWrite(prefix, fsName + "_adaptor_write"),
          adaptorRead(prefix, fsName + "_adaptor_read"),
          adaptorWriteS3(prefix, fsName + "_adaptor_write_s3"),
          adaptorWriteDiskCache(prefix, fsName + "_adaptor_write_disk_cache"),
          adaptorReadS3(prefix, fsName + "_adaptor_read_s3"),
          adaptorReadDiskCache(prefix, fsName + "_adaptor_read_disk_cache"),
          writeToS3(prefix, fsName + "_write_to_s3"),
          readFromS3(prefix, fsName + "_read_from_s3"),
          writeToDiskCache(prefix, fsName + "_write_to_disk_cache"),
          readFromDiskCache(prefix, fsName + "_read_from_disk_cache"),
          writeToKVCache(prefix, fsName + "_write_to_kv_cache"),
          readFromKVCache(prefix, fsName + "_read_from_kv_cache"),
          readSize(prefix, fsName + "_adaptor_read_size", 0),
          writeSize(prefix, fsName + "_adaptor_write_size", 0) {}
};

template <typename Tp>
uint64_t LoadAtomicValue(void* atomValue) {
    std::atomic<Tp>* bytes = reinterpret_cast<std::atomic<Tp>*>(atomValue);
    return static_cast<uint64_t>(bytes->load());
}

struct DiskCacheMetric {
    static const std::string prefix;

    std::string fsName;
    InterfaceMetric writeS3;
    bvar::PassiveStatus<uint64_t> usedBytes_;
    bvar::PassiveStatus<uint64_t> totalBytes_;
    InterfaceMetric trim_;

    explicit DiskCacheMetric(const std::string& name = "",
                             std::atomic<int64_t>* usedBytes = nullptr,
                             std::atomic<int64_t>* totalBytes = nullptr)
        : fsName(!name.empty() ? name
                               : prefix + curve::common::ToHexString(this)),
          writeS3(prefix, fsName + "_write_s3"),
          usedBytes_(prefix, fsName + "_diskcache_usedbytes",
                     LoadAtomicValue<int64_t>, usedBytes),
          totalBytes_(prefix, fsName + "_diskcache_totalbytes",
                      LoadAtomicValue<int64_t>, totalBytes),
          trim_(prefix, fsName + "_diskcache_trim") {}
};

struct KVClientManagerMetric {
    static const std::string prefix;

    std::string fsName;
    InterfaceMetric get;
    InterfaceMetric set;
    // kvcache count
    bvar::Adder<uint64_t> count;
    // kvcache hit
    bvar::Adder<uint64_t> hit;
    // kvcache miss
    bvar::Adder<uint64_t> miss;
    // kvcache getQueueSize
    bvar::Adder<uint64_t> getQueueSize;
    // kvcache setQueueSize
    bvar::Adder<uint64_t> setQueueSize;

    explicit KVClientManagerMetric(const std::string& name = "")
        : fsName(!name.empty() ? name
                               : prefix + curve::common::ToHexString(this)),
          get(prefix, fsName + "_get"),
          set(prefix, fsName + "_set"),
          count(prefix, fsName + "_count"),
          hit(prefix, fsName + "_hit"),
          miss(prefix, fsName + "_miss"),
          getQueueSize(prefix, fsName + "_get_queue_size"),
          setQueueSize(prefix, fsName + "_set_queue_size") {}
};

struct MemcacheClientMetric {
    static const std::string prefix;

    std::string fsName;
    InterfaceMetric get;
    InterfaceMetric set;

    explicit MemcacheClientMetric(const std::string& name = "")
        : fsName(!name.empty() ? name
                               : prefix + curve::common::ToHexString(this)),
          get(prefix, fsName + "_get"),
          set(prefix, fsName + "_set") {}
};

struct S3ChunkInfoMetric {
    static const std::string prefix;

    bvar::Adder<int64_t> s3ChunkInfoSize;

    S3ChunkInfoMetric() : s3ChunkInfoSize(prefix, "size") {}
};

struct WarmupManagerS3Metric {
    static const std::string prefix;

    InterfaceMetric warmupS3Cached;
    bvar::Adder<uint64_t> warmupS3CacheSize;

    WarmupManagerS3Metric()
        : warmupS3Cached(prefix, "s3_cached"),
          warmupS3CacheSize(prefix, "s3_cache_size") {}
};

void AsyncContextCollectMetrics(
    std::shared_ptr<S3Metric> s3Metric,
    const std::shared_ptr<curve::common::PutObjectAsyncContext>& context);

void AsyncContextCollectMetrics(
    std::shared_ptr<S3Metric> s3Metric,
    const std::shared_ptr<curve::common::GetObjectAsyncContext>& context);

struct FuseS3ClientIOLatencyMetric {
    static const std::string prefix;

    std::string fsName;

    bvar::LatencyRecorder readAttrLatency;
    bvar::LatencyRecorder readDataLatency;
    bvar::LatencyRecorder writeAttrLatency;
    bvar::LatencyRecorder writeDataLatency;

    explicit FuseS3ClientIOLatencyMetric(const std::string& name = "")
        : fsName(!name.empty() ? name
                               : prefix + curve::common::ToHexString(this)),
          readAttrLatency(prefix, fsName + "_read_attr_latency"),
          readDataLatency(prefix, fsName + "_read_data_latency"),
          writeAttrLatency(prefix, fsName + "_write_attr_latency"),
          writeDataLatency(prefix, fsName + "_write_data_latency") {}
};

}  // namespace metric
}  // namespace client
}  // namespace curvefs

#endif  // CURVEFS_SRC_CLIENT_METRIC_CLIENT_METRIC_H_
