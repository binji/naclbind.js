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

"use strict";

require(['zlib'], function(zlib) {

  function makeTestArrayBuffer(length, add, mul) {
    var newAb = new ArrayBuffer(length);
    var view = new Uint8Array(newAb);
    var value = 0;
    for (var i = 0; i < length; ++i) {
      value = ((value + add) * mul) | 0;
      view[i] = value & 255;
    }
    return newAb;
  }

  var ab = makeTestArrayBuffer(16384, 1337, 0xc0dedead);

  zlib.compressHard(ab, 9, 16384).then(function(outputAb) {
    var before = ab.byteLength;
    var after = outputAb.byteLength;
    console.log('compress done! orig = ' + before +
                ' comp = ' + after +
                ' ratio = ' + ((after / before) * 100).toFixed(1) + '%');
  }).catch(function(err) {
    console.log('compress done...\n' + err.stack);
  });

  zlib.compressEasy(ab).then(function(outputAb) {
    var before = ab.byteLength;
    var after = outputAb.byteLength;
    console.log('compressEasy done! orig = ' + before +
                ' comp = ' + after +
                ' ratio = ' + ((after / before) * 100).toFixed(1) + '%');
  }).catch(function(err) {
    console.log('compressEasy done...\n' + err.stack);
  });

});
