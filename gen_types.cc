// Copyright 2014 Ben Smith. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "gen_types.h"

#include <zlib.h>

const int kNumTypes = 36;
Type* g_type_map[kNumTypes];

StructField TYPE_z_stream_fields[] = {
    {"next_in", &TYPE_uint8_p, offsetof(z_stream, next_in)},
    {"avail_in", &TYPE_uint32, offsetof(z_stream, avail_in)},
    {"total_in", &TYPE_uint32, offsetof(z_stream, total_in)},
    {"next_out", &TYPE_uint8_p, offsetof(z_stream, next_out)},
    {"avail_out", &TYPE_uint32, offsetof(z_stream, avail_out)},
    {"total_out", &TYPE_uint32, offsetof(z_stream, total_out)}, };
StructType TYPE_z_stream(33, "z_stream", sizeof(z_stream),
                         sizeof(TYPE_z_stream_fields) /
                             sizeof(TYPE_z_stream_fields[0]),
                         TYPE_z_stream_fields);
PointerType TYPE_z_stream_p(34, &TYPE_z_stream);

FunctionType TYPE_deflate(35, &TYPE_int32, &TYPE_z_stream_p, &TYPE_int32);
