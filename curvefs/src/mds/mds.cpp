/*
 *  Copyright (c) 2021 NetEase Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
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
 * Created Date: 2021-05-19
 * Author: chenwei
 */

#include "curvefs/src/mds/mds.h"

#include <brpc/channel.h>
#include <brpc/server.h>
#include <glog/logging.h>

#include <utility>

#include "curvefs/src/mds/mds_service.h"
#include "curvefs/src/mds/space/mds_proxy_manager.h"
#include "src/common/curve_version.h"

namespace brpc {
DECLARE_bool(graceful_quit_on_sigterm);
}  // namespace brpc

namespace curvefs {
namespace mds {

using ::curve::election::LeaderElection;
using ::curve::idgenerator::EtcdIdGenerator;
using ::curve::kvstorage::EtcdClientImp;
using ::curvefs::mds::space::SpaceManagerImpl;
using ::curvefs::mds::space::SpaceServiceImpl;

MDS::MDS()
    : conf_(),
      inited_(false),
      running_(false),
      fsManager_(),
      fsStorage_(),
      metaserverClient_(),
      topology_(),
      options_(),
      etcdClientInited_(false),
      etcdClient_(),
      leaderElection_(),
      status_(),
      etcdEndpoint_(),
      etcdUsername_(),
      etcdPassword_() {}

MDS::~MDS() {}

void MDS::InitOptions(std::shared_ptr<Configuration> conf) {
    conf_ = std::move(conf);
    conf_->GetValueFatalIfFail("mds.listen.addr", &options_.mdsListenAddr);
    conf_->GetValueFatalIfFail("mds.dummy.port", &options_.dummyPort);
    conf_->GetValueFatalIfFail("mds.server.idleTimeoutSec",
                               &options_.idleTimeoutSec);
    conf_->GetValueFatalIfFail("mds.space.calIntervalSec",
                               &options_.mdsSpaceCalIntervalSec);

    InitMetaServerOption(&options_.metaserverOptions);
    InitTopologyOption(&options_.topologyOptions);
    InitScheduleOption(&options_.scheduleOption);
    InitDLockOptions(&options_.dLockOptions);
    InitMdsProxyManagerOptions(&options_.bsMdsProxyOptions);
}

void MDS::InitMetaServerOption(MetaserverOptions* metaserverOption) {
    conf_->GetValueFatalIfFail("metaserver.addr",
                               &metaserverOption->metaserverAddr);
    conf_->GetValueFatalIfFail("metaserver.rpcTimeoutMs",
                               &metaserverOption->rpcTimeoutMs);
    conf_->GetValueFatalIfFail("metaserver.rpcRertyTimes",
                               &metaserverOption->rpcRetryTimes);
    conf_->GetValueFatalIfFail("metaserver.rpcRetryIntervalUs",
                               &metaserverOption->rpcRetryIntervalUs);
}

void MDS::InitTopologyOption(TopologyOption* topologyOption) {
    conf_->GetValueFatalIfFail("mds.topology.TopologyUpdateToRepoSec",
                               &topologyOption->topologyUpdateToRepoSec);
    conf_->GetValueFatalIfFail("mds.topology.MaxPartitionNumberInCopyset",
                               &topologyOption->maxPartitionNumberInCopyset);
    conf_->GetValueFatalIfFail("mds.topology.IdNumberInPartition",
                               &topologyOption->idNumberInPartition);
    conf_->GetValueFatalIfFail("mds.topology.CreatePartitionNumber",
                               &topologyOption->createPartitionNumber);
    conf_->GetValueFatalIfFail("mds.topology.MaxCopysetNumInMetaserver",
                               &topologyOption->maxCopysetNumInMetaserver);
    conf_->GetValueFatalIfFail("mds.topology.UpdateMetricIntervalSec",
                               &topologyOption->UpdateMetricIntervalSec);
}

void MDS::InitScheduleOption(ScheduleOption* scheduleOption) {
    conf_->GetValueFatalIfFail("mds.enable.recover.scheduler",
                               &scheduleOption->enableRecoverScheduler);
    conf_->GetValueFatalIfFail("mds.enable.copyset.scheduler",
                               &scheduleOption->enableCopysetScheduler);
    conf_->GetValueFatalIfFail("mds.enable.leader.scheduler",
                               &scheduleOption->enableLeaderScheduler);
    conf_->GetValueFatalIfFail("mds.copyset.scheduler.balanceRatioPercent",
                               &scheduleOption->balanceRatioPercent);

    conf_->GetValueFatalIfFail("mds.recover.scheduler.intervalSec",
                               &scheduleOption->recoverSchedulerIntervalSec);
    conf_->GetValueFatalIfFail("mds.copyset.scheduler.intervalSec",
                               &scheduleOption->copysetSchedulerIntervalSec);
    conf_->GetValueFatalIfFail("mds.leader.scheduler.intervalSec",
                               &scheduleOption->leaderSchedulerIntervalSec);

    conf_->GetValueFatalIfFail("mds.schduler.operator.concurrent",
                               &scheduleOption->operatorConcurrent);
    conf_->GetValueFatalIfFail("mds.schduler.transfer.limitSec",
                               &scheduleOption->transferLeaderTimeLimitSec);
    conf_->GetValueFatalIfFail("mds.scheduler.add.limitSec",
                               &scheduleOption->addPeerTimeLimitSec);
    conf_->GetValueFatalIfFail("mds.scheduler.remove.limitSec",
                               &scheduleOption->removePeerTimeLimitSec);
    conf_->GetValueFatalIfFail("mds.scheduler.change.limitSec",
                               &scheduleOption->changePeerTimeLimitSec);
    conf_->GetValueFatalIfFail("mds.scheduler.metaserver.cooling.timeSec",
                               &scheduleOption->metaserverCoolingTimeSec);
}

void MDS::InitDLockOptions(DLockOptions* dLockOptions) {
    conf_->GetValueFatalIfFail("dlock.ttl_ms", &dLockOptions->ttlMs);
    conf_->GetValueFatalIfFail("dlock.try_timeout_ms",
                               &dLockOptions->tryTimeoutMs);
    conf_->GetValueFatalIfFail("dlock.try_interval_ms",
                               &dLockOptions->tryIntervalMs);
}

void MDS::InitFsManagerOptions(FsManagerOption* fsManagerOption) {
    conf_->GetValueFatalIfFail("mds.fsmanager.backEndThreadRunInterSec",
                               &fsManagerOption->backEndThreadRunInterSec);

    LOG_IF(ERROR,
           !conf_->GetUInt32Value("mds.fsmanager.client.timeoutSec",
                                  &fsManagerOption->clientTimeoutSec))
        << "Get `mds.fsmanager.client.timeoutSec` from conf error, use "
           "default value: "
        << fsManagerOption->clientTimeoutSec;

    LOG_IF(ERROR,
           !conf_->GetUInt32Value("mds.fsmanager.reloadSpaceConcurrency",
                                  &fsManagerOption->spaceReloadConcurrency))
        << "Get `mds.fsmanager.reloadSpaceConcurrency` from conf error, use "
           "default value: "
        << fsManagerOption->spaceReloadConcurrency;

    ::curve::common::InitS3AdaptorOptionExceptS3InfoOption(
        conf_.get(), &fsManagerOption->s3AdapterOption);
}

void MDS::Init() {
    LOG(INFO) << "Init MDS start";

    InitEtcdClient();

    space::MdsProxyManager::SetProxyOptions(options_.bsMdsProxyOptions);

    fsStorage_ = std::make_shared<PersisKVStorage>(etcdClient_);
    spaceManager_ = std::make_shared<SpaceManagerImpl>(
        etcdClient_, fsStorage_, options_.mdsSpaceCalIntervalSec);
    metaserverClient_ =
        std::make_shared<MetaserverClient>(options_.metaserverOptions);
    auto dlock = std::make_shared<DLock>(options_.dLockOptions, etcdClient_);

    // init topology
    InitTopology(options_.topologyOptions);
    InitTopologyMetricService(options_.topologyOptions);
    InitTopologyManager(options_.topologyOptions);
    InitCoordinator();
    InitHeartbeatManager();
    FsManagerOption fsManagerOption;
    InitFsManagerOptions(&fsManagerOption);

    s3Adapter_ = std::make_shared<S3Adapter>();
    fsManager_ = std::make_shared<FsManager>(
        fsStorage_, spaceManager_, metaserverClient_, topologyManager_,
        s3Adapter_, dlock, fsManagerOption);
    LOG_IF(FATAL, !fsManager_->Init()) << "fsManager Init fail";

    chunkIdAllocator_ = std::make_shared<ChunkIdAllocatorImpl>(etcdClient_);

    inited_ = true;

    LOG(INFO) << "Init MDS success";
}

void MDS::InitTopology(const TopologyOption& option) {
    auto topologyIdGenerator = std::make_shared<DefaultIdGenerator>();
    auto topologyTokenGenerator = std::make_shared<DefaultTokenGenerator>();

    auto codec = std::make_shared<TopologyStorageCodec>();
    auto topologyStorage =
        std::make_shared<TopologyStorageEtcd>(etcdClient_, codec);
    LOG(INFO) << "init topologyStorage success.";

    topology_ = std::make_shared<TopologyImpl>(
        topologyIdGenerator, topologyTokenGenerator, topologyStorage);
    LOG_IF(FATAL, topology_->Init(option) < 0) << "init topology fail.";
    LOG(INFO) << "init topology success.";
}

void MDS::InitTopologyManager(const TopologyOption& option) {
    topologyManager_ =
        std::make_shared<TopologyManager>(topology_, metaserverClient_);
    topologyManager_->Init(option);
    LOG(INFO) << "init topologyManager success.";
}

void MDS::InitTopologyMetricService(const TopologyOption& option) {
    topologyMetricService_ = std::make_shared<TopologyMetricService>(topology_);
    topologyMetricService_->Init(option);
    LOG(INFO) << "init topologyMetricService success.";
}

void MDS::InitCoordinator() {
    auto scheduleMetrics = std::make_shared<ScheduleMetrics>(topology_);
    auto topoAdapter =
        std::make_shared<TopoAdapterImpl>(topology_, topologyManager_);
    coordinator_ = std::make_shared<Coordinator>(topoAdapter);
    coordinator_->InitScheduler(options_.scheduleOption, scheduleMetrics);
}

void MDS::Run() {
    LOG(INFO) << "Run MDS";
    if (!inited_) {
        LOG(ERROR) << "MDS not inited yet!";
        return;
    }

    // set mds version in metric
    curve::common::ExposeCurveVersion();

    LOG_IF(FATAL, topology_->Run()) << "run topology module fail";
    topologyMetricService_->Run();
    coordinator_->Run();
    heartbeatManager_->Run();
    fsManager_->Run();

    brpc::Server server;
    // add heartbeat service
    HeartbeatServiceImpl heartbeatService(heartbeatManager_);
    LOG_IF(FATAL, server.AddService(&heartbeatService,
                                    brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
        << "add heartbeatService error";

    // add mds service
    MdsServiceImpl mdsService(fsManager_, chunkIdAllocator_);
    LOG_IF(FATAL,
           server.AddService(&mdsService, brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
        << "add mdsService error";

    // add topology service
    TopologyServiceImpl topologyService(topologyManager_);
    LOG_IF(FATAL, server.AddService(&topologyService,
                                    brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
        << "add topologyService error";

    SpaceServiceImpl spaceService(spaceManager_.get());
    LOG_IF(FATAL, server.AddService(&spaceService,
                                    brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
        << "add space service error";

    // start rpc server
    brpc::ServerOptions option;
    option.idle_timeout_sec = options_.idleTimeoutSec;
    LOG_IF(FATAL, server.Start(options_.mdsListenAddr.c_str(), &option) != 0)
        << "start brpc server error";
    running_ = true;

    brpc::FLAGS_graceful_quit_on_sigterm = true;
    server.RunUntilAskedToQuit();
}

void MDS::Stop() {
    LOG(INFO) << "Stop MDS";
    if (!running_) {
        LOG(WARNING) << "Stop MDS, but MDS is not running, return OK";
        return;
    }
    brpc::AskToQuit();
    heartbeatManager_->Stop();
    coordinator_->Stop();
    topologyMetricService_->Stop();
    fsManager_->Uninit();
    topology_->Stop();
}

void MDS::StartDummyServer() {
    conf_->ExposeMetric("curvefs_mds_config");
    status_.expose("curvefs_mds_status");
    status_.set_value("follower");

    // set mds version in metric
    curve::common::ExposeCurveVersion();

    LOG_IF(FATAL, 0 != brpc::StartDummyServerAt(options_.dummyPort))
        << "Start dummy server failed";
}

void MDS::StartCompaginLeader() {
    InitEtcdClient();

    LeaderElectionOptions electionOption;
    InitLeaderElectionOption(&electionOption);
    electionOption.etcdCli = etcdClient_;
    electionOption.campaginPrefix = "";

    InitLeaderElection(electionOption);

    while (0 != leaderElection_->CampaignLeader()) {
        LOG(INFO) << leaderElection_->GetLeaderName()
                  << " compagin for leader again";
    }

    LOG(INFO) << "Compagin leader success, I am leader now";
    status_.set_value("leader");
    leaderElection_->StartObserverLeader();
}

void MDS::InitEtcdClient() {
    if (etcdClientInited_) {
        return;
    }

    EtcdConf etcdConf;
    InitEtcdConf(&etcdConf);

    int etcdTimeout = 0;
    int etcdRetryTimes = 0;
    conf_->GetValueFatalIfFail("etcd.operation.timeoutMs", &etcdTimeout);
    conf_->GetValueFatalIfFail("etcd.retry.times", &etcdRetryTimes);

    etcdClient_ = std::make_shared<EtcdClientImp>();

    int r = etcdClient_->Init(etcdConf, etcdTimeout, etcdRetryTimes);
    LOG_IF(FATAL, r != EtcdErrCode::EtcdOK)
        << "Init etcd client error: " << r
        << ", etcd address: " << std::string(etcdConf.Endpoints, etcdConf.len)
        << ", etcdtimeout: " << etcdConf.DialTimeout
        << ", operation timeout: " << etcdTimeout
        << ", etcd retrytimes: " << etcdRetryTimes
        << ", etcd auth enable: " << etcdConf.authEnable;

    LOG_IF(FATAL, !CheckEtcd()) << "Check etcd failed";

    LOG(INFO) << "Init etcd client succeeded, etcd address: "
              << std::string(etcdConf.Endpoints, etcdConf.len)
              << ", etcdtimeout: " << etcdConf.DialTimeout
              << ", operation timeout: " << etcdTimeout
              << ", etcd retrytimes: " << etcdRetryTimes
              << ", etcd auth enable: " << etcdConf.authEnable;

    etcdClientInited_ = true;
}

void MDS::InitEtcdConf(EtcdConf* etcdConf) {
    conf_->GetValueFatalIfFail("etcd.endpoint", &etcdEndpoint_);
    etcdConf->len = etcdEndpoint_.size();
    etcdConf->Endpoints = &etcdEndpoint_[0];
    conf_->GetValueFatalIfFail("etcd.dailtimeoutMs", &etcdConf->DialTimeout);
    // etcd auth config
    bool authEnable = false;
    conf_->GetBoolValue("etcd.auth.enable", &authEnable);
    etcdConf->authEnable = authEnable ? 1 : 0;
    if (authEnable) {
        conf_->GetValueFatalIfFail("etcd.auth.username", &etcdUsername_);
        etcdConf->username = &etcdUsername_[0];
        etcdConf->usernameLen = etcdUsername_.size();
        conf_->GetValueFatalIfFail("etcd.auth.password", &etcdPassword_);
        etcdConf->password = &etcdPassword_[0];
        etcdConf->passwordLen = etcdPassword_.size();
    }

    LOG(INFO) << "etcd.endpoint: " << etcdEndpoint_;
    LOG(INFO) << "etcd.dailtimeoutMs: " << etcdConf->DialTimeout;
}

bool MDS::CheckEtcd() {
    std::string out;
    int r = etcdClient_->Get("test", &out);

    if (r != EtcdErrCode::EtcdOK && r != EtcdErrCode::EtcdKeyNotExist) {
        LOG(ERROR) << "Check etcd error: " << r;
        return false;
    } else {
        LOG(INFO) << "Check etcd ok";
        return true;
    }
}

void MDS::InitLeaderElectionOption(LeaderElectionOptions* option) {
    conf_->GetValueFatalIfFail("mds.listen.addr", &option->leaderUniqueName);
    conf_->GetValueFatalIfFail("leader.sessionInterSec",
                               &option->sessionInterSec);
    conf_->GetValueFatalIfFail("leader.electionTimeoutMs",
                               &option->electionTimeoutMs);
}

void MDS::InitLeaderElection(const LeaderElectionOptions& option) {
    leaderElection_ = std::make_shared<LeaderElection>(option);
}

void MDS::InitHeartbeatOption(HeartbeatOption* heartbeatOption) {
    conf_->GetValueFatalIfFail("mds.heartbeat.intervalMs",
                               &heartbeatOption->heartbeatIntervalMs);
    conf_->GetValueFatalIfFail("mds.heartbeat.misstimeoutMs",
                               &heartbeatOption->heartbeatMissTimeOutMs);
    conf_->GetValueFatalIfFail("mds.heartbeat.offlinetimeoutMs",
                               &heartbeatOption->offLineTimeOutMs);
    conf_->GetValueFatalIfFail("mds.heartbeat.clean_follower_afterMs",
                               &heartbeatOption->cleanFollowerAfterMs);
}

void MDS::InitHeartbeatManager() {
    // init option
    HeartbeatOption heartbeatOption;
    InitHeartbeatOption(&heartbeatOption);

    heartbeatOption.mdsStartTime = steady_clock::now();
    heartbeatManager_ = std::make_shared<HeartbeatManager>(
        heartbeatOption, topology_, coordinator_, spaceManager_);
    heartbeatManager_->Init();
}

void MDS::InitMdsProxyManagerOptions(MdsProxyOptions* options) {
    conf_->GetValueFatalIfFail("bs.mds.maxRetryMs",
                               &options->option.mdsMaxRetryMS);
    conf_->GetValueFatalIfFail("bs.mds.rpcTimeoutMs",
                               &options->option.rpcRetryOpt.rpcTimeoutMs);
    conf_->GetValueFatalIfFail("bs.mds.maxRPCTimeoutMs",
                               &options->option.rpcRetryOpt.maxRPCTimeoutMS);
    conf_->GetValueFatalIfFail("bs.mds.rpcRetryIntervalUs",
                               &options->option.rpcRetryOpt.rpcRetryIntervalUS);
    conf_->GetValueFatalIfFail(
        "bs.mds.maxFailedTimesBeforeChangeMDS",
        &options->option.rpcRetryOpt.maxFailedTimesBeforeChangeAddr);
    conf_->GetValueFatalIfFail(
        "bs.mds.normalRetryTimesBeforeTriggerWait",
        &options->option.rpcRetryOpt.normalRetryTimesBeforeTriggerWait);
    conf_->GetValueFatalIfFail("bs.mds.waitSleepMs",
                               &options->option.rpcRetryOpt.waitSleepMs);
}

}  // namespace mds
}  // namespace curvefs
