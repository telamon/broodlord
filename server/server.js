var fs = require('fs'),
    path = require('path'),
    zlib = require('zlib'),
    net = require('net'),
    assert = require('assert');
var HEADER_SIZE = 20;
var streamHandler = function(stream){
  stream.gFrame={header:{}, bitsRecieved: {} , data:[],state:'HEADER'}
  var decoder = stream.pipe(zlib.createInflate());
  decoder.on('data',function(chunk){
    var header = stream.gFrame.header;
    var data = new Buffer(chunk);
    if(stream.gFrame.state == 'HEADER'){
      header.magic = data.toString('ascii',0,7); 
      assert.equal(header.magic, 'frame\u0000\u0000');
      header.width = data.readInt16LE(8);

      debugger;
      decoder.end();

    }
  });
}
// Mock stream test
streamHandler(fs.createReadStream('fbdump3.gz'));

var server = net.createServer(function(client){
  streamHandler(client);
});
//server.listen(5566);


