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
 * Date: Fri Sep  3 17:30:00 CST 2021
 * Author: wuhanqing
 */

#include "curvefs/src/metaserver/common/operator_type.h"

namespace curvefs {
namespace metaserver {

const char* OperatorTypeName(OperatorType type) {
    switch (type) {
        case OperatorType::GetDentry:
            return "GetDentry";
        case OperatorType::ListDentry:
            return "ListDentry";
        case OperatorType::CreateDentry:
            return "CreateDentry";
        case OperatorType::DeleteDentry:
            return "DeleteDentry";
        case OperatorType::GetInode:
            return "GetInode";
        case OperatorType::BatchGetInodeAttr:
            return "BatchGetInodeAttr";
        case OperatorType::BatchGetXAttr:
            return "BatchGetXAttr";
        case OperatorType::CreateInode:
            return "CreateInode";
        case OperatorType::UpdateInode:
            return "UpdateInode";
        case OperatorType::DeleteInode:
            return "DeleteInode";
        case OperatorType::CreateRootInode:
            return "CreateRootInode";
        case OperatorType::CreatePartition:
            return "CreatePartition";
        case OperatorType::DeletePartition:
            return "DeletePartition";
        case OperatorType::PrepareRenameTx:
            return "PrepareRenameTx";
        default:
            return "Unknown";
    }
}

}  // namespace metaserver
}  // namespace curvefs
