var nacl = require('../dist/2nacl'),
    EmbedElement = require('./test_embed_element'),
    assert = require('assert');

function doNothing() {}

describe('Module', function() {
  var m, t, el;
  beforeEach(function() {
    m = nacl.Module('foo.nmf', 'application/x-pnacl', {
      name: 'foo',
      embedElementConstructor: EmbedElement,
      log: doNothing,
    });

    el = m.element;
    t = m.types;
  });

  it('should not post messages before it is loaded', function() {
    el.setPostMessageCallback(function(msg) {
      assert.fail(undefined, undefined, "Unexpected postMessage");
    });
    m.commit(doNothing);
  });

  it('should post queued messages after it is loaded', function(done) {
    el.setPostMessageCallback(function(msg) {
      assert.equal(el.loaded, true);
      done();
    });
    m.commit(doNothing);
    el.load();
  });
});
