var fs = require('fs'),
    path = require('path'),
    zlib = require('zlib');

var input= fs.createReadStream('fbdump3.gz');
//zlib.inflateRaw(input,function(err,buffer){
  //console.log("chunk");
  //if(!err){
    //console.log(buffer.length);
    ////console.log(buffer.toString('base64'));
  //}else{
    //console.log(err);
  //}
//});
//
input.pipe(zlib.createInflate()).pipe(process.stdout);
