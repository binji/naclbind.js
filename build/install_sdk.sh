# Copyright 2014 Ben Smith. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -o errexit

readonly SCRIPT_DIR=$(dirname $(readlink -f $0))
readonly ROOT_DIR=$(dirname ${SCRIPT_DIR})
readonly OUT_DIR=${ROOT_DIR}/out
readonly NACL_SDK_URL=http://storage.googleapis.com/nativeclient-mirror/nacl/nacl_sdk/nacl_sdk.zip

cd ${OUT_DIR}
wget ${NACL_SDK_URL}
unzip nacl_sdk.zip
nacl_sdk/naclsdk update pepper_canary --force
